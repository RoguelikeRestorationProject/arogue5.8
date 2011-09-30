/*
 * File with various monster functions in it
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * Based on "Rogue: Exploring the Dungeons of Doom"
 * Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "curses.h"
#include "rogue.h"
#include <ctype.h>
#include <string.h>


/*
 * Check_residue takes care of any effect of the monster 
 */
check_residue(tp)
register struct thing *tp;
{
    /*
     * Take care of special abilities
     */
    if (on(*tp, DIDHOLD) && (--hold_count == 0)) turn_off(player, ISHELD);

    /* If it has lowered player, give him back a level */
    if (on(*tp, DIDDRAIN)) raise_level(FALSE);

    /* If frightened of this monster, stop */
    if (on(player, ISFLEE) &&
	player.t_dest == &tp->t_pos) turn_off(player, ISFLEE);

    /* If monster was suffocating player, stop it */
    if (on(*tp, DIDSUFFOCATE)) extinguish(suffocate);

    /* If something with fire, may darken */
    if (on(*tp, HASFIRE)) {
	register struct room *rp=roomin(&tp->t_pos);
	register struct linked_list *fire_item;

	if (rp) {
	    for (fire_item = rp->r_fires; fire_item != NULL;
		 fire_item = next(fire_item)) {
		if (THINGPTR(fire_item) == tp) {
		    detach(rp->r_fires, fire_item);
		    destroy_item(fire_item);
		    if (rp->r_fires == NULL) {
			rp->r_flags &= ~HASFIRE;
			if (cansee(tp->t_pos.y, tp->t_pos.x)) light(&hero);
		    }
		    break;
		}
	    }
	}
    }
}

/*
 * Creat_mons creates the specified monster -- any if 0 
 */

bool
creat_mons(person, monster, report)
struct thing *person;	/* Where to create next to */
short monster;
bool report;
{
    struct linked_list *nitem;
    register struct thing *tp;
    struct room *rp;
    coord *mp;

    if (levtype == POSTLEV)
	return(FALSE);
    if ((mp = fallpos(&(person->t_pos), FALSE, 2)) != NULL) {
	nitem = new_item(sizeof (struct thing));
	new_monster(nitem,
		    monster == 0 ? randmonster(FALSE, FALSE)
				 : monster,
		    mp,
		    TRUE);
	tp = THINGPTR(nitem);
	runto(tp, &hero);
	tp->t_no_move = 1;	/* since it just got here, it is disoriented */
	carry_obj(tp, monsters[tp->t_index].m_carry/2); /* only half chance */
	if (on(*tp, HASFIRE)) {
	    rp = roomin(&tp->t_pos);
	    if (rp) {
		register struct linked_list *fire_item;

		/* Put the new fellow in the room list */
		fire_item = creat_item();
		ldata(fire_item) = (char *) tp;
		attach(rp->r_fires, fire_item);

		rp->r_flags |= HASFIRE;
	    }
	}

	/* 
	 * If we can see this monster, set oldch to ' ' to make light()
	 * think the creature used to be invisible (ie. not seen here)
	 */
	if (cansee(tp->t_pos.y, tp->t_pos.x)) tp->t_oldch = ' ';
	return(TRUE);
    }
    if (report) msg("You hear a faint cry of anguish in the distance.");
    return(FALSE);
}

/*
 * Genmonsters:
 *	Generate at least 'least' monsters for this single room level.
 *	'Treas' indicates whether this is a "treasure" level.
 */

void
genmonsters(least, treas)
register int least;
bool treas;
{
    reg int i;
    reg struct room *rp = &rooms[0];
    reg struct linked_list *item;
    reg struct thing *mp;
    coord tp;

    for (i = 0; i < level + least; i++) {
	    if (!treas && rnd(100) < 50)	/* put in some little buggers */
		    continue;
	    /*
	     * Put the monster in
	     */
	    item = new_item(sizeof *mp);
	    mp = THINGPTR(item);
	    do {
		    rnd_pos(rp, &tp);
	    } until(mvwinch(stdscr, tp.y, tp.x) == FLOOR);

	    new_monster(item, randmonster(FALSE, FALSE), &tp, FALSE);
	    /*
	     * See if we want to give it a treasure to carry around.
	     */
	    carry_obj(mp, monsters[mp->t_index].m_carry);

	    /* Is it going to give us some light? */
	    if (on(*mp, HASFIRE)) {
		register struct linked_list *fire_item;

		fire_item = creat_item();
		ldata(fire_item) = (char *) mp;
		attach(rp->r_fires, fire_item);
		rp->r_flags |= HASFIRE;
	    }
    }
}

/*
 * id_monst returns the index of the monster given its letter
 */

short
id_monst(monster)
register char monster;
{
    register short result;

    result = NLEVMONS*vlevel;
    if (result > NUMMONST) result = NUMMONST;

    for(; result>0; result--)
	if (monsters[result].m_appear == monster) return(result);
    for (result=(NLEVMONS*vlevel)+1; result <= NUMMONST; result++)
	if (monsters[result].m_appear == monster) return(result);
    return(0);
}


/*
 * new_monster:
 *	Pick a new monster and add it to the list
 */

new_monster(item, type, cp, max_monster)
struct linked_list *item;
short type;
register coord *cp;
bool max_monster;
{
    register struct thing *tp;
    register struct monster *mp;
    register char *ip, *hitp;
    register int i, min_intel, max_intel;
    register int num_dice, num_sides=8, num_extra=0;

    attach(mlist, item);
    tp = THINGPTR(item);
    tp->t_turn = TRUE;
    tp->t_pack = NULL;
    tp->t_index = type;
    tp->t_wasshot = FALSE;
    tp->t_type = monsters[type].m_appear;
    tp->t_ctype = C_MONSTER;
    tp->t_no_move = 0;
    tp->t_doorgoal = 0;
    tp->t_quiet = 0;
    tp->t_pos = tp->t_oldpos = *cp;
    tp->t_oldch = CCHAR( mvwinch(cw, cp->y, cp->x) );
    mvwaddch(mw, cp->y, cp->x, tp->t_type);
    mp = &monsters[tp->t_index];

    /* Figure out monster's hit points */
    hitp = mp->m_stats.s_hpt;
    num_dice = atoi(hitp);
    if ((hitp = strchr(hitp, 'd')) != NULL) {
	num_sides = atoi(++hitp);
	if ((hitp = strchr(hitp, '+')) != NULL)
	    num_extra = atoi(++hitp);
    }

    tp->t_stats.s_lvl = mp->m_stats.s_lvl;
    tp->t_stats.s_arm = mp->m_stats.s_arm;
    strncpy(tp->t_stats.s_dmg,mp->m_stats.s_dmg,sizeof(tp->t_stats.s_dmg));
    tp->t_stats.s_str = mp->m_stats.s_str;
    if (vlevel > HARDER) { /* the deeper, the meaner we get */
	 tp->t_stats.s_lvl += (vlevel - HARDER);
	 num_dice += (vlevel - HARDER)/2;
    }
    if (max_monster)
	tp->t_stats.s_hpt = num_dice * num_sides + num_extra;
    else
	tp->t_stats.s_hpt = roll(num_dice, num_sides) + num_extra;
    tp->t_stats.s_exp = mp->m_stats.s_exp + mp->m_add_exp*tp->t_stats.s_hpt;

    /*
     * just initailize others values to something reasonable for now
     * maybe someday will *really* put these in monster table
     */
    tp->t_stats.s_wisdom = 8 + rnd(4);
    tp->t_stats.s_dext = 8 + rnd(4);
    tp->t_stats.s_const = 8 + rnd(4);
    tp->t_stats.s_charisma = 8 + rnd(4);

    /* Set the initial flags */
    for (i=0; i<16; i++) tp->t_flags[i] = 0;
    for (i=0; i<MAXFLAGS; i++)
	turn_on(*tp, mp->m_flags[i]);

    /* suprising monsters don't always surprise you */
    if (!max_monster		&& on(*tp, CANSURPRISE) && 
	off(*tp, ISUNIQUE)	&& rnd(100) < 20)
	    turn_off(*tp, CANSURPRISE);

    /* If this monster is unique, gen it */
    if (on(*tp, ISUNIQUE)) mp->m_normal = FALSE;

    /* 
     * if is it the quartermaster, then compute his level and exp pts
     * based on the level. This will make it fair when thieves try to
     * steal and give them reasonable experience if they succeed.
     */
    if (on(*tp, CANSELL)) {	
	tp->t_stats.s_exp = vlevel * 100;
	tp->t_stats.s_lvl = vlevel/2 + 1;
	attach(tp->t_pack, new_thing(ALL));
    }

    /* Normally scared monsters have a chance to not be scared */
    if (on(*tp, ISFLEE) && (rnd(4) == 0)) turn_off(*tp, ISFLEE);

    /* Figure intelligence */
    min_intel = atoi(mp->m_intel);
    if ((ip = (char *) strchr(mp->m_intel, '-')) == NULL)
	tp->t_stats.s_intel = min_intel;
    else {
	max_intel = atoi(++ip);
	if (max_monster)
	    tp->t_stats.s_intel = max_intel;
	else
	    tp->t_stats.s_intel = min_intel + rnd(max_intel - min_intel);
    }
    if (vlevel > HARDER) 
	 tp->t_stats.s_intel += ((vlevel - HARDER)/2);
    tp->maxstats = tp->t_stats;

    /* If the monster can shoot, it may have a weapon */
    if (on(*tp, CANSHOOT) && ((rnd(100) < (22 + vlevel)) || max_monster)) {
	struct linked_list *item1;
	register struct object *cur, *cur1;

	item = new_item(sizeof *cur);
	item1 = new_item(sizeof *cur1);
	cur = OBJPTR(item);
	cur1 = OBJPTR(item1);
	cur->o_hplus = (rnd(4) < 3) ? 0
				    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
	cur->o_dplus = (rnd(4) < 3) ? 0
				    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
	cur1->o_hplus = (rnd(4) < 3) ? 0
				    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
	cur1->o_dplus = (rnd(4) < 3) ? 0
				    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
	strcpy(cur->o_damage,"0d0");
        strcpy(cur->o_hurldmg,"0d0");
	strcpy(cur1->o_damage,"0d0");
        strcpy(cur1->o_hurldmg,"0d0");
	cur->o_ac = cur1->o_ac = 11;
	cur->o_count = cur1->o_count = 1;
	cur->o_group = cur1->o_group = 0;
	cur->contents = cur1->contents = NULL;
	if ((cur->o_hplus <= 0) && (cur->o_dplus <= 0)) cur->o_flags = ISCURSED;
	if ((cur1->o_hplus <= 0) && (cur1->o_dplus <= 0))
	    cur1->o_flags = ISCURSED;
	cur->o_flags = cur1->o_flags = 0;
	cur->o_type = cur1->o_type = WEAPON;
	cur->o_mark[0] = cur1->o_mark[0] = '\0';

	/* The monster may use a crossbow, sling, or an arrow */
	i = rnd(100);
	if (i < 10) {
	    cur->o_which = CROSSBOW;
	    cur1->o_which = BOLT;
	    init_weapon(cur, CROSSBOW);
	    init_weapon(cur1, BOLT);
	}
	else if (i < 70) {
	    cur->o_which = BOW;
	    cur1->o_which = ARROW;
	    init_weapon(cur, BOW);
	    init_weapon(cur1, ARROW);
	}
	else {
	    cur->o_which = SLING;
	    cur1->o_which = ROCK;
	    init_weapon(cur, SLING);
	    init_weapon(cur1, ROCK);
	}

	attach(tp->t_pack, item);
	attach(tp->t_pack, item1);
    }


    if (ISWEARING(R_AGGR))
	runto(tp, &hero);
    if (on(*tp, ISDISGUISE))
    {
	char mch = 0;

	if (tp->t_pack != NULL)
	    mch = (OBJPTR(tp->t_pack))->o_type;
	else
	    switch (rnd(10)) {
		case 0: mch = GOLD;
		when 1: mch = POTION;
		when 2: mch = SCROLL;
		when 3: mch = FOOD;
		when 4: mch = WEAPON;
		when 5: mch = ARMOR;
		when 6: mch = RING;
		when 7: mch = STICK;
		when 8: mch = monsters[randmonster(FALSE, FALSE)].m_appear;
		when 9: mch = MM;
	    }
	tp->t_disguise = mch;
    }
}

/*
 * randmonster:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */

short
randmonster(wander, no_unique)
register bool wander, no_unique;
{
    register int d, cur_level, range, i; 

    /* 
     * Do we want a merchant? Merchant is always in place 'NUMMONST' 
     */
    if (wander && monsters[NUMMONST].m_wander && rnd(100) < 3) return NUMMONST;

    cur_level = vlevel;
    range = 4*NLEVMONS;
    i = 0;
    do
    {
	if (i++ > range*10) { /* just in case all have be genocided */
	    i = 0;
	    if (--cur_level <= 0)
		fatal("Rogue could not find a monster to make");
	}
	d = NLEVMONS*(cur_level - 1) + (rnd(range) - (range - 1 - NLEVMONS));
	if (d < 1)
	    d = rnd(NLEVMONS) + 1;
	if (d > NUMMONST - NUMUNIQUE - 1) {
	    if (no_unique)
		d = rnd(range) + (NUMMONST - NUMUNIQUE - 1) - (range - 1);
	    else if (d > NUMMONST - 1)
		d = rnd(range+NUMUNIQUE) + (NUMMONST-1) - (range+NUMUNIQUE-1);
	}
    }
    while  (wander ? !monsters[d].m_wander || !monsters[d].m_normal 
		   : !monsters[d].m_normal);
    return d;
}

/* Sell displays a menu of goods from which the player may choose
 * to purchase something.
 */

sell(tp)
register struct thing *tp;
{
    register struct linked_list *item;
    register struct object *obj;
    register int i, j, min_worth, nitems, goods = 0, chance, which_item;
    char buffer[LINELEN];
    struct {
	int which;
	int plus1, plus2;
	int count;
	int worth;
	char *name;
    } selection[10];

    min_worth = 100000;
    item = find_mons(tp->t_pos.y, tp->t_pos.x); /* Get pointer to monster */

    /* Select the items */
    nitems = rnd(6) + 5;

    for (i=0; i<nitems; i++) {
	selection[i].worth = selection[i].plus1
			   = selection[i].plus2
			   = selection[i].which
			   = selection[i].count
			   = 0;
    }
    switch (rnd(9)) {
	/* Armor */
	case 0:
	case 1:
	    goods = ARMOR;
	    for (i=0; i<nitems; i++) {
		chance = rnd(100);
		for (j = 0; j < MAXARMORS; j++)
		    if (chance < armors[j].a_prob)
			break;
		if (j == MAXARMORS) {
		    debug("Picked a bad armor %d", chance);
		    j = 0;
		}
		selection[i].which = j;
		selection[i].count = 1;
		if (rnd(100) < 40) selection[i].plus1 = rnd(5) + 1;
		else selection[i].plus1 = 0;
		selection[i].name = armors[j].a_name;

		/* Calculate price */
		selection[i].worth = armors[j].a_worth;
		selection[i].worth += 
			2 * s_magic[S_ALLENCH].mi_worth * selection[i].plus1;
		if (min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;

	/* Weapon */
	case 2:
	case 3:
	    goods = WEAPON;
	    for (i=0; i<nitems; i++) {
		selection[i].which = rnd(MAXWEAPONS);
		selection[i].count = 1;
		if (rnd(100) < 35) {
		    selection[i].plus1 = rnd(3);
		    selection[i].plus2 = rnd(3);
		}
		else {
		    selection[i].plus1 = 0;
		    selection[i].plus2 = 0;
		}
		if (weaps[selection[i].which].w_flags & ISMANY)
		    selection[i].count = rnd(15) + 5;
		selection[i].name = weaps[selection[i].which].w_name;
		/*
		 * note: use "count" before adding in the enchantment cost
		 * 	 of an item. This will keep the price of arrows 
		 * 	 and such to a reasonable price.
		 */
		j = selection[i].plus1 + selection[i].plus2;
		selection[i].worth = weaps[selection[i].which].w_worth;
		selection[i].worth *= selection[i].count;
		selection[i].worth += 2 * s_magic[S_ALLENCH].mi_worth * j;
		if (min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;

	/* Staff or wand */
	case 4:
	    goods = STICK;
	    for (i=0; i<nitems; i++) {
		selection[i].which = pick_one(ws_magic, MAXSTICKS);
		selection[i].plus1 = rnd(11) + 5;	/* Charges */
		selection[i].count = 1;
		selection[i].name = ws_magic[selection[i].which].mi_name;
		selection[i].worth = ws_magic[selection[i].which].mi_worth;
		selection[i].worth += 20 * selection[i].plus1;
		if (min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;

	/* Ring */
	case 5:
	    goods = RING;
	    for (i=0; i<nitems; i++) {
		selection[i].which = pick_one(r_magic, MAXRINGS);
		selection[i].plus1 = rnd(2) + 1;  /* Armor class */
		selection[i].count = 1;
		if (rnd(100) < r_magic[selection[i].which].mi_bless + 10)
		    selection[i].plus1 += rnd(2) + 1;
		selection[i].name = r_magic[selection[i].which].mi_name;
		selection[i].worth = r_magic[selection[i].which].mi_worth;

		switch (selection[i].which) {
		case R_DIGEST:
		    if (selection[i].plus1 > 2) selection[i].plus1 = 2;
		    else if (selection[i].plus1 < 1) selection[i].plus1 = 1;
		/* fall thru here to other cases */
		case R_ADDSTR:
		case R_ADDDAM:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDINTEL:
		case R_ADDWISDOM:
		    if (selection[i].plus1 > 0)
			selection[i].worth += selection[i].plus1 * 50;
		}
		if(min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;

	/* scroll */
	case 6:
	    goods = SCROLL;
	    for (i=0; i<nitems; i++) {
		selection[i].which = pick_one(s_magic, MAXSCROLLS);
		selection[i].count = 1;
		selection[i].name = s_magic[selection[i].which].mi_name;
		selection[i].worth = s_magic[selection[i].which].mi_worth;
		if (min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;

	/* potions */
	case 7:
	    goods = POTION;
	    for (i=0; i<nitems; i++) {
		selection[i].which = pick_one(p_magic, MAXPOTIONS);
		selection[i].count = 1;
		selection[i].name = p_magic[selection[i].which].mi_name;
		selection[i].worth = p_magic[selection[i].which].mi_worth;
		if (min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;

	/* Miscellaneous magic */ 
	case 8:
	    goods = MM;
	    for (i=0; i<nitems; i++) { /* don't sell as many mm as others */
		selection[i].which = pick_one(m_magic, MAXMM);
		selection[i].count = 1;
		selection[i].name = m_magic[selection[i].which].mi_name;
		selection[i].worth = m_magic[selection[i].which].mi_worth;

		switch (selection[i].which) {
		case MM_JUG:
		    switch(rnd(9)) {
			case 0: selection[i].plus1 = P_PHASE;
			when 1: selection[i].plus1 = P_CLEAR;
			when 2: selection[i].plus1 = P_SEEINVIS;
			when 3: selection[i].plus1 = P_HEALING;
			when 4: selection[i].plus1 = P_MFIND;
			when 5: selection[i].plus1 = P_TFIND;
			when 6: selection[i].plus1 = P_HASTE;
			when 7: selection[i].plus1 = P_RESTORE;
			when 8: selection[i].plus1 = P_FLY;
		    }
		when MM_OPEN:
		case MM_HUNGER:
		case MM_DRUMS:
		case MM_DISAPPEAR:
		case MM_CHOKE:
		case MM_KEOGHTOM:
		    selection[i].plus1  = 3 + (rnd(3)+1) * 3;
		    selection[i].worth += selection[i].plus1 * 50;
		when MM_BRACERS:
		    selection[i].plus1  = rnd(10)+1;
		    selection[i].worth += selection[i].plus1 * 75;
		when MM_DISP:
		    selection[i].plus1  = 2;
		when MM_PROTECT:
		    selection[i].plus1  = rnd(5)+1;
		    selection[i].worth += selection[i].plus1 * 100;
		when MM_SKILLS:
		    selection[i].plus1 = player.t_ctype;
		otherwise:
		    break;
		}
		if(min_worth > selection[i].worth)
		    min_worth = selection[i].worth;
	    }
	    break;
    }

    /* See if player can afford an item */
    if (min_worth > purse) {
	msg("The %s eyes your small purse and departs.",
			monsters[NUMMONST].m_name);
	/* Get rid of the monster */
	killed(item, FALSE, FALSE);
	return;
    }

    /* Display the goods */
    msg("The %s shows you his wares.--More--", monsters[NUMMONST].m_name);
    wait_for(cw,' ');
    msg("");
    clearok(cw, TRUE);
    touchwin(cw);

    wclear(hw);
    touchwin(hw);
    for (i=0; i < nitems; i++) {
	mvwaddch(hw, i+2, 0, '[');
	waddch(hw, (char) ((int) 'a' + i));
	waddstr(hw, "] ");
	switch (goods) {
	    case ARMOR:
		waddstr(hw, "Some ");
	    when WEAPON:
		if (selection[i].count == 1)
		    waddstr(hw, " A ");
		else {
		    sprintf(buffer, "%2d ", selection[i].count);
		    waddstr(hw, buffer);
		}
	    when STICK:
		wprintw(hw, "A %-5s of ", ws_type[selection[i].which]);
	    when RING:
		waddstr(hw, "A ring of ");
	    when SCROLL:
		waddstr(hw, "A scroll of ");
	    when POTION:
		waddstr(hw, "A potion of ");
	}
	if (selection[i].count > 1)
	    sprintf(buffer, "%s%s ", selection[i].name, "s");
	else
	    sprintf(buffer, "%s ", selection[i].name);
	wprintw(hw, "%-24s", buffer);
	wprintw(hw, " Price:%5d", selection[i].worth);
    }
    sprintf(buffer, "Purse:  %d", purse);
    mvwaddstr(hw, nitems+3, 0, buffer);
    mvwaddstr(hw, 0, 0, "How about one of the following goods? ");
    draw(hw);
    /* Get rid of the monster */
    killed(item, FALSE, FALSE);

    which_item = (int) (wgetch(hw) - 'a');
    while (which_item < 0 || which_item >= nitems) {
	if (which_item == (int) ESCAPE - (int) 'a') {
	    return;
	}
	mvwaddstr(hw, 0, 0, "Please enter one of the listed items. ");
	draw(hw);
	which_item = (int) (wgetch(hw) - 'a');
    }

    if (selection[which_item].worth > purse) {
	msg("You cannot afford it.");
	return;
    }

    purse -= selection[which_item].worth;

    item = spec_item(goods, selection[which_item].which,
		     selection[which_item].plus1, selection[which_item].plus2);

    obj = OBJPTR(item);
    if (selection[which_item].count > 1) {
	obj->o_count = selection[which_item].count;
	obj->o_group = newgrp();
    }
    /* If a stick or ring, let player know the type */
    switch (goods) {
	case RING:  r_know[selection[which_item].which] = TRUE;
	when POTION:p_know[selection[which_item].which] = TRUE;
	when SCROLL:s_know[selection[which_item].which] = TRUE;
	when STICK: ws_know[selection[which_item].which] = TRUE;
	when MM:    m_know[selection[which_item].which] = TRUE;

    }

    if (add_pack(item, FALSE, NULL) == FALSE) {

	obj->o_pos = hero;
	fall(item, TRUE);
    }
}



/*
 * what to do when the hero steps next to a monster
 */
struct linked_list *
wake_monster(y, x)
int y, x;
{
    register struct thing *tp;
    register struct linked_list *it;
    register struct room *trp;
    register const char *mname;
    bool nasty;	/* Will the monster "attack"? */
    char ch;

    if ((it = find_mons(y, x)) == NULL) {
	msg("Can't find monster in show");
	return (NULL);
    }
    tp = THINGPTR(it);
    ch = tp->t_type;

    trp = roomin(&tp->t_pos); /* Current room for monster */
    mname = monsters[tp->t_index].m_name;

    /*
     * Let greedy ones in a room guard gold
     * (except in a maze where lots of creatures would all go for the 
     * same piece of gold)
     */
    if (on(*tp, ISGREED)	&& off(*tp, ISRUN)	&& 
	levtype != MAZELEV	&& trp != NULL		&&
	lvl_obj != NULL) {
	    register struct linked_list *item;
	    register struct object *cur;

	    for (item = lvl_obj; item != NULL; item = next(item)) {
		cur = OBJPTR(item);
		if ((cur->o_type == GOLD) &&
		    (roomin(&cur->o_pos) == trp)) {
		    /* Run to the gold */
		    tp->t_dest = &cur->o_pos;
		    turn_on(*tp, ISRUN);
		    turn_off(*tp, ISDISGUISE);

		    /* Make it worth protecting */
		    cur->o_count += GOLDCALC + GOLDCALC;
		    break;
		}
	    }
    }

    /*
     * Every time he sees mean monster, it might start chasing him
     */
    if (on(*tp, ISMEAN)							&& 
	off(*tp, ISHELD)						&& 
	off(*tp, ISRUN)							&& 
	rnd(100) > 33							&& 
	(!is_stealth(&player) || (on(*tp, ISUNIQUE) && rnd(100) > 95))	&&
	(off(player, ISINVIS) || on(*tp, CANSEE))			||
	(trp != NULL && (trp->r_flags & ISTREAS))) {
	tp->t_dest = &hero;
	turn_on(*tp, ISRUN);
	turn_off(*tp, ISDISGUISE);
    }

    /* See if the monster will bother the player */
    nasty = (on(*tp, ISRUN) && cansee(tp->t_pos.y, tp->t_pos.x));

    /*
     * Let the creature summon if it can.
     * Also check to see if there is room around the player,
     * if not then the creature will wait
     */
    if (on(*tp, CANSUMMON) && nasty		&& 
	rnd(40) < tp->t_stats.s_lvl		&&
	fallpos(&hero, FALSE, 2) != NULL) {
	const char *helpname;
	int fail;
	register int which, i;

	turn_off(*tp, CANSUMMON);
	helpname = monsters[tp->t_index].m_typesum;
	for (which=1; which<NUMMONST; which++) {
	     if (strcmp(helpname, monsters[which].m_name) == 0)
		 break;
	}
	if (which >= NUMMONST)
	     debug("couldn't find summoned one");
	if ((off(*tp, ISINVIS)     || on(player, CANSEE)) &&
	    (off(*tp, ISSHADOW)    || on(player, CANSEE)) &&
	    (off(*tp, CANSURPRISE) || ISWEARING(R_ALERT))) {
	    if (monsters[which].m_normal == FALSE) { /* genocided? */
		msg("The %s appears dismayed", mname);
		monsters[tp->t_index].m_numsum = 0;
	    }
	    else {
		sprintf(outstring,"The %s summons %ss for help", mname, helpname);
		msg(outstring);
	    }
	}
	else {
	    if (monsters[which].m_normal == FALSE) /* genocided? */
		monsters[tp->t_index].m_numsum = 0;
	    else
		msg("%ss seem to appear from nowhere!", helpname);
	}
	/*
	 * try to make all the creatures around player but remember
	 * if unsuccessful
	 */
	for (i=0, fail=0; i<monsters[tp->t_index].m_numsum; i++) {
	     if (!creat_mons(&player, which, FALSE))
		 fail++;	/* remember the failures */
	}
	/*
	 * try once again to make the buggers
	 */
	for (i=0; i<fail; i++)
	     creat_mons(tp, which, FALSE);
	
	/* Now let the poor fellow see all the trouble */
	light(&hero);
    }

    /* 
     * Handle monsters that can gaze and do things while running
     * Player must be able to see the monster and the monster must 
     * not be asleep 
     */
    if (nasty && !invisible(tp)) {
	/*
	 * Confusion
	 */
	if (on(*tp, CANHUH)				 &&
	   (off(*tp, ISINVIS)     || on(player, CANSEE)) &&
	   (off(*tp, CANSURPRISE) || ISWEARING(R_ALERT))) {
	    if (!save(VS_MAGIC, &player, 0)) {
		if (off(player, ISCLEAR)) {
		    if (find_slot(unconfuse))
			lengthen(unconfuse, rnd(20)+HUHDURATION);
		    else {
			fuse(unconfuse, 0, rnd(20)+HUHDURATION, AFTER);
			msg("The %s's gaze has confused you.",mname);
			turn_on(player, ISHUH);
		    }
		}
		else msg("You feel dizzy for a moment, but it quickly passes.");
	    }
	    else if (rnd(100) < 67)
		turn_off(*tp, CANHUH); /* Once you save, maybe that's it */
	}

	/* Sleep */
	if(on(*tp, CANSNORE) &&  
	   no_command == 0 && 
	   !save(VS_PARALYZATION, &player, 0)) {
	    if (ISWEARING(R_ALERT))
		msg("You feel slightly drowsy for a moment.");
	    else {
		msg("The %s's gaze puts you to sleep.", mname);
		no_command += SLEEPTIME;
		if (rnd(100) < 50) turn_off(*tp, CANSNORE);
	    }
	}

	/* Fear */
	if (on(*tp, CANFRIGHTEN)) {
	    turn_off(*tp, CANFRIGHTEN);
	    if (!ISWEARING(R_HEROISM)		&&
		!save(VS_WAND, &player, 0)	&&
	        !(on(player, ISFLEE) && (player.t_dest == &tp->t_pos))) {
		    turn_on(player, ISFLEE);
		    player.t_dest = &tp->t_pos;
		    msg("The sight of the %s terrifies you.", mname);
	    }
	}

	/* blinding creatures */
	if(on(*tp, CANBLIND) && 
	   !find_slot(sight) && 
	   !save(VS_WAND,&player, 0)){
	    msg("The gaze of the %s blinds you", mname);
	    turn_on(player, ISBLIND);
	    fuse(sight, 0, rnd(30)+20, AFTER);
	    light(&hero);
	}

	/* the sight of the ghost can age you! */
	if (on(*tp, CANAGE)) { 
	    turn_off (*tp, CANAGE);
	    if (!save(VS_MAGIC, &player, 0)) {
		msg ("The sight of the %s ages you!", mname);
		pstats.s_const--;
		max_stats.s_const--;
		if (pstats.s_const < 0)
		    death (D_CONSTITUTION);
	    }
	}


	/* Turning to stone */
	if (on(*tp, LOOKSTONE)) {
	    turn_off(*tp, LOOKSTONE);

	    if (on(player, CANINWALL))
		msg("The gaze of the %s has no effect.", mname);
	    else {
		if (!save(VS_PETRIFICATION, &player, 0) && rnd(100) < 15) {
		    pstats.s_hpt = -1;
		    msg("The gaze of the %s petrifies you.", mname);
		    msg("You are turned to stone !!! --More--");
		    wait_for(cw,' ');
		    death(D_PETRIFY);
		}
		else {
		    msg("The gaze of the %s stiffens your limbs.", mname);
		    no_command += STONETIME;
		}
	    }
	}
    }

    return it;
}
/*
 * wanderer:
 *	A wandering monster has awakened and is headed for the player
 */

wanderer()
{
    register int i;
    register struct room *hr = roomin(&hero);
    register struct linked_list *item;
    register struct thing *tp;
    register const long *attr;	/* Points to monsters' attributes */
    int carry;	/* Chance of wanderer carrying anything */
    short rmonst;	/* Our random wanderer */
    bool canteleport = FALSE,	/* Can the monster teleport? */
	 seehim;	/* Is monster within sight? */
    coord cp;

    rmonst = randmonster(TRUE, FALSE);	/* Choose a random wanderer */
    attr = &monsters[rmonst].m_flags[0]; /* Start of attributes */
    for (i=0; i<MAXFLAGS; i++)
	if (*attr++ == CANTELEPORT) {
	    canteleport = TRUE;
	    break;
	}

    /* Find a place for it -- avoid the player's room if can't teleport */
    do {
	do {
	    i = rnd_room();
	} until (canteleport || hr != &rooms[i] || levtype == MAZELEV ||
		 levtype == OUTSIDE || levtype == POSTLEV);

	/* Make sure the monster does not teleport on top of the player */
	do {
	    rnd_pos(&rooms[i], &cp);
	} while (hr == &rooms[i] && ce(cp, hero));
    } until (step_ok(cp.y, cp.x, NOMONST, NULL));

    /* Create a new wandering monster */
    item = new_item(sizeof *tp);
    new_monster(item, rmonst, &cp, FALSE);
    tp = THINGPTR(item);
    turn_on(*tp, ISRUN);
    turn_off(*tp, ISDISGUISE);
    tp->t_dest = &hero;
    tp->t_pos = cp;	/* Assign the position to the monster */
    seehim = cansee(tp->t_pos.y, tp->t_pos.x);
    if (on(*tp, HASFIRE)) {
	register struct room *rp;

	rp = roomin(&tp->t_pos);
	if (rp) {
	    register struct linked_list *fire_item;

	    fire_item = creat_item();
	    ldata(fire_item) = (char *) tp;
	    attach(rp->r_fires, fire_item);

	    rp->r_flags |= HASFIRE;
	    if (seehim && next(rp->r_fires) == NULL)
		light(&hero);
	}
    }

    /* See if we give the monster anything */
    carry = monsters[tp->t_index].m_carry;
    if (off(*tp, ISUNIQUE)) carry /= 2;	/* Non-unique has only a half chance */
    carry_obj(tp, carry);

    /* Alert the player if a monster just teleported in */
    if (hr == &rooms[i] && canteleport && seehim && !invisible(tp)) {
	msg("A %s just teleported in", monsters[rmonst].m_name);
	light(&hero);
	running = FALSE;
    }

    if (wizard)
	msg("Started a wandering %s", monsters[tp->t_index].m_name);
}
