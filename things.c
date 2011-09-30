/*
 * Contains functions for dealing with things like
 * potions and scrolls
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
#include <ctype.h>
#include "rogue.h"

/*
 * print out the number of charges on a stick
 */
char *
charge_str(obj)
register struct object *obj;
{
    static char buf[20];

    if (!(obj->o_flags & ISKNOW))
	buf[0] = '\0';
    else if (terse)
	sprintf(buf, " [%d]", obj->o_charges);
    else
	sprintf(buf, " [%d charges]", obj->o_charges);
    return buf;
}
/*
 * inv_name:
 *	return the name of something as it would appear in an
 *	inventory.
 */
char *
inv_name(obj, drop)
register struct object *obj;
bool drop;
{
    register char *pb;

    pb = prbuf;
    pb[0] = '\0';
    switch(obj->o_type) {
	case SCROLL:
	    if (obj->o_count == 1)
		 sprintf(pb, "A %sscroll ", blesscurse(obj->o_flags));
	    else
		 sprintf(pb, "%d %sscrolls ", 
			obj->o_count, blesscurse(obj->o_flags));
	    pb = &pb[strlen(pb)];
	    if (s_know[obj->o_which] || (obj->o_flags & ISPOST))
		sprintf(pb, "of %s", s_magic[obj->o_which].mi_name);
	    else if (s_guess[obj->o_which])
		sprintf(pb, "called %s", s_guess[obj->o_which]);
	    else
		sprintf(pb, "titled '%s'", s_names[obj->o_which]);
        when POTION:
	    if (obj->o_count == 1)
		 sprintf(pb, "A %spotion ", blesscurse(obj->o_flags));
	    else
		 sprintf(pb, "%d %spotions ", 
			obj->o_count, blesscurse(obj->o_flags));
	    pb = &pb[strlen(pb)];
	    if (obj->o_flags & ISPOST)
		sprintf(pb, "of %s", p_magic[obj->o_which].mi_name);
	    else if (p_know[obj->o_which])
		sprintf(pb, "of %s (%s)", p_magic[obj->o_which].mi_name,
		    p_colors[obj->o_which]);
	    else if (p_guess[obj->o_which])
		sprintf(pb, "called %s (%s)", p_guess[obj->o_which],
		    p_colors[obj->o_which]);
	    else {
		pb = prbuf;
		if (obj->o_count == 1)
		    sprintf(pb, "A%s %s potion",
			    vowelstr(p_colors[obj->o_which]),
			    p_colors[obj->o_which]);
		else
		    sprintf(pb, "%d %s potions",
			    obj->o_count, p_colors[obj->o_which]);
	    }
	when FOOD:
	    if (obj->o_which == 1)
		if (obj->o_count == 1)
		    sprintf(pb, "A%s %s", vowelstr(fruit), fruit);
		else
		    sprintf(pb, "%d %ss", obj->o_count, fruit);
	    else
		if (obj->o_count == 1)
		    strcpy(pb, "Some food");
		else
		    sprintf(pb, "%d rations of food", obj->o_count);
	when WEAPON:
	    if (obj->o_count > 1)
		sprintf(pb, "%d ", obj->o_count);
	    else
		strcpy(pb, "A ");
	    pb = &pb[strlen(pb)];
	    if (obj->o_flags & ISKNOW) {
		strcat(pb, num(obj->o_hplus, obj->o_dplus));
		strcat (pb, " ");
	    }
	    strcat(pb, weaps[obj->o_which].w_name);
	    if (obj->o_count > 1)
		strcat(pb, "s");
	    if (obj == cur_weapon)
		strcat(pb, " (weapon in hand)");
	when ARMOR:
	    if (obj->o_flags & ISKNOW) {
		strcat(pb, num(armors[obj->o_which].a_class - obj->o_ac, 0));
		strcat(pb, " ");
	    }
	    strcat(pb, armors[obj->o_which].a_name);
	    if (obj == cur_armor)
		strcat(pb, " (being worn)");
	when STICK:
	    sprintf(pb, "A %s%s ", 
		blesscurse(obj->o_flags), ws_type[obj->o_which]);
	    pb = &pb[strlen(pb)];
	    if (obj->o_flags & ISPOST)
		sprintf(pb, "of %s", ws_magic[obj->o_which].mi_name);
	    else if (ws_know[obj->o_which])
		sprintf(pb, "of %s%s (%s)", ws_magic[obj->o_which].mi_name,
		    charge_str(obj), ws_made[obj->o_which]);
	    else if (ws_guess[obj->o_which])
		sprintf(pb, "called %s (%s)", ws_guess[obj->o_which],
		    ws_made[obj->o_which]);
	    else {
		pb = prbuf;
		sprintf(pb, "A %s %s", ws_made[obj->o_which],
		    ws_type[obj->o_which]);
	    }
	    if (obj == cur_weapon)
		strcat(prbuf, " (weapon in hand)");
        when RING:
	    if (obj->o_flags & ISPOST)
		sprintf(pb, "A ring of %s", r_magic[obj->o_which].mi_name);
	    else if (r_know[obj->o_which])
		sprintf(pb, "A%s ring of %s (%s)", ring_num(obj),
		    r_magic[obj->o_which].mi_name, r_stones[obj->o_which]);
	    else if (r_guess[obj->o_which])
		sprintf(pb, "A ring called %s (%s)",
		    r_guess[obj->o_which], r_stones[obj->o_which]);
	    else
		sprintf(pb, "A%s %s ring", vowelstr(r_stones[obj->o_which]),
		    r_stones[obj->o_which]);
	    if     (obj == cur_ring[LEFT_1] || obj == cur_ring[LEFT_2] ||
		    obj == cur_ring[LEFT_3] || obj == cur_ring[LEFT_4])  
			strcat(pb, " (on left hand)");
	    if	   (obj == cur_ring[RIGHT_1] || obj == cur_ring[RIGHT_2] ||
		    obj == cur_ring[RIGHT_3] || obj == cur_ring[RIGHT_4]) 
			strcat(pb, " (on right hand)");
	when RELIC:
	    if (obj->o_flags & ISKNOW)
		strcpy(pb, rel_magic[obj->o_which].mi_name);
	    else switch(obj->o_which) {
		case MUSTY_DAGGER:
		    strcpy(pb, "Two very fine daggers marked MDDE");
		when EMORI_CLOAK:
		    strcpy(pb, "A silk cloak");
		when HEIL_ANKH:
		    strcpy(pb, "A golden ankh");
		when MING_STAFF:
		    strcpy(pb, "A finely carved staff");
		when ORCUS_WAND:
		    strcpy(pb, "A sparkling ivory wand");
		when ASMO_ROD:
		    strcpy(pb, "A glistening ebony rod");
		when YENDOR_AMULET:
		    strcpy(pb, "A silver amulet");
		when BRIAN_MANDOLIN:
		    strcpy(pb, "A gleaming mandolin");
		when HRUGGEK_MSTAR:
		    strcpy(pb, "A huge morning star");
		when GERYON_HORN:
		    strcpy(pb, "A jet black horn");
		when YEENOGHU_FLAIL:
		    strcpy(pb, "A shimmering flail");
		otherwise:
		    strcpy(pb, "A magical item");
	    }

	    /* Take care of wielding and wearing */
	    switch (obj->o_which) {
		case EMORI_CLOAK:
		    if (cur_armor == NULL && cur_misc[WEAR_CLOAK] == NULL)
			strcat(pb, " (being worn)");
		when HEIL_ANKH:
		    if (cur_relic[HEIL_ANKH]) strcat(pb, " (in hand)");
		when YENDOR_AMULET:
		    if (cur_relic[YENDOR_AMULET] &&
			cur_misc[WEAR_JEWEL] == NULL)
			strcat(pb, " (in chest)");
		when MUSTY_DAGGER:
		case HRUGGEK_MSTAR:
		case YEENOGHU_FLAIL:
		case MING_STAFF:
		case ASMO_ROD:
		case ORCUS_WAND:
		    if (cur_weapon == obj) strcat(pb, " (weapon in hand)");
	    }
	when MM:
	    if (m_know[obj->o_which])
			strcpy(pb, misc_name(obj));
	    else {
		switch (obj->o_which) {
		    case MM_JUG:
		    case MM_BEAKER:
			strcpy(pb, "A bottle");
		    when MM_KEOGHTOM:
			strcpy(pb, "A jar");
		    when MM_JEWEL:
			strcpy(pb, "An amulet");
		    when MM_BOOK:
		    case MM_SKILLS:
			strcpy(pb, "A book");
		    when MM_ELF_BOOTS:
		    case MM_DANCE:
			strcpy(pb, "A pair of boots");
		    when MM_BRACERS:
			strcpy(pb, "A pair of bracers");
		    when MM_OPEN:
		    case MM_HUNGER:
			strcpy(pb, "A chime");
		    when MM_DISP:
		    case MM_R_POWERLESS:
		    case MM_PROTECT:
			strcpy(pb, "A cloak");
		    when MM_DRUMS:
			strcpy(pb, "A set of drums");
		    when MM_DISAPPEAR:
		    case MM_CHOKE:
			strcpy(pb, "A pouch of dust");
		    when MM_G_DEXTERITY:
		    case MM_G_OGRE:
		    case MM_FUMBLE:
			strcpy(pb, "A pair of gauntlets");
		    when MM_ADAPTION:
		    case MM_STRANGLE:
			strcpy(pb, "A necklace");
		    otherwise:
			strcpy(pb, "A magical item");
		}
		if (m_guess[obj->o_which]) {
		    strcat(pb, " called: ");
		    strcat(pb, m_guess[obj->o_which]);
		}
	    }
	    if (obj == cur_misc[WEAR_BOOTS]	||
		obj == cur_misc[WEAR_BRACERS]	||
		obj == cur_misc[WEAR_CLOAK]	||
		obj == cur_misc[WEAR_GAUNTLET]	||
		obj == cur_misc[WEAR_NECKLACE]	||
		obj == cur_misc[WEAR_JEWEL]) 
		    strcat(pb, " (being worn)");
	when GOLD:
		sprintf(pb, "%d Pieces of Gold", obj->o_count);
	otherwise:
	    debug("Picked up something funny");
	    sprintf(pb, "Something bizarre %s", unctrl(obj->o_type));
    }

    /* Is it marked? */
    if (obj->o_mark[0]) {
	pb = &pb[strlen(pb)];
	sprintf(pb, " <%s>", obj->o_mark);
    }

    if (obj->o_flags & ISPROT)
	strcat(pb, " [protected]");
    if (drop && isupper(prbuf[0]))
	prbuf[0] = tolower(prbuf[0]);
    else if (!drop && islower(*prbuf))
	*prbuf = toupper(*prbuf);
    if (!drop)
	strcat(pb, ".");
    /* 
     * Truncate if long. Use COLS-4 to offset the "pack letter" of a normal
     * inventory listing.
     */
    prbuf[COLS-4] = '\0';	
    return prbuf;
}

/*
 * weap_name:
 *	Return the name of a weapon.
 */
char *
weap_name(obj)
register struct object *obj;
{
    switch (obj->o_type) {
	case WEAPON:
	    return(weaps[obj->o_which].w_name);
	when RELIC:
	    switch (obj->o_which) {
		case MUSTY_DAGGER:
		    return("daggers");
		when YEENOGHU_FLAIL:
		    return("flail");
		when HRUGGEK_MSTAR:
		    return("morning star");
		when MING_STAFF:
		    return("staff");
		when ORCUS_WAND:
		    return("wand");
		when ASMO_ROD:
		    return("rod");
	    }
    }
    return("weapon");
}

/*
 * drop:
 *	put something down
 */
drop(item)
struct linked_list *item;
{
    register char ch = 0;
    register struct linked_list *obj, *nobj;
    register struct object *op;

    if (item == NULL) {
	switch(ch = CCHAR( mvwinch(stdscr, hero.y, hero.x) )) {
	case PASSAGE: 
	case SCROLL:
	case POTION:
	case WEAPON:
	case FLOOR:
	case STICK:
	case ARMOR:
	case POOL:
	case RELIC:
	case GOLD:
	case FOOD:
	case RING:
	case MM:
	    break;
	default:
	    msg("Can't leave it here");
	    return(FALSE);
	}
	if ((obj = get_item(pack, "drop", ALL)) == NULL)
	    return(FALSE);
    }
    else {
	obj = item;
    }
    op = OBJPTR(obj);
    if (!dropcheck(op))
	return(FALSE);

    /*
     * If it is a scare monster scroll, curse it
     */
    if (op->o_type == SCROLL && op->o_which == S_SCARE) {
	if (op->o_flags & ISBLESSED)
	    op->o_flags &= ~ISBLESSED;
	else op->o_flags |= ISCURSED;
    }

    /*
     * Take it out of the pack
     */
    if (op->o_count >= 2 && op->o_group == 0)
    {
	nobj = new_item(sizeof *op);
	op->o_count--;
	op = OBJPTR(nobj);
	*op = *(OBJPTR(obj));
	op->o_count = 1;
	obj = nobj;
    }
    else {
	detach(pack, obj);
        inpack--;
    }
    if(ch == POOL) {
	msg("Your %s sinks out of sight.",inv_name(op,TRUE));
	o_discard(obj);
    }
    else if (levtype == POSTLEV) {
	op->o_pos = hero;	/* same place as hero */
	fall(obj,FALSE);
	if (item == NULL)	/* if item wasn't sold */
	    msg("Thanks for your donation to the fiend's flea market.");
    }
    else {
	/*
	 * Link it into the level object list
	 */
	attach(lvl_obj, obj);
	mvaddch(hero.y, hero.x, op->o_type);
	op->o_pos = hero;
	msg("Dropped %s", inv_name(op, TRUE));
    }
    updpack(FALSE);
    return (TRUE);
}

/*
 * do special checks for dropping or unweilding|unwearing|unringing
 */
dropcheck(op)
register struct object *op;
{
    int save_max;

    if (op == NULL)
	return TRUE;
    if (levtype == POSTLEV) {
	if ((op->o_flags & ISCURSED) && (op->o_flags & ISKNOW)) {
	    msg("The trader does not accept your shoddy merchandise");
	    return(FALSE);
	}
    }

    /* Player will not drop a relic */
    if (op->o_type == RELIC) {
	/*
	 * There is a 1% cumulative chance per relic that trying to get
	 * rid of it will cause the relic to turn on the player.
	 */
	if (rnd(100) < cur_relic[op->o_which]++) {
	    msg("The artifact turns on you.");
	    msg("It crushes your mind!!! -- More --");
	    pstats.s_hpt = -1;
	    wait_for (cw,' ');
	    death(D_RELIC);
	}
	else {
	    if (terse) msg("Can't release it.");
	    else msg("You cannot bring yourself to release it.");
	    return FALSE;
	}
    }

    /* If we aren't wearing it, we can drop it */
    if (!is_current(op)) return TRUE;

    /* At this point, we know we are wearing the item */
    if (op->o_flags & ISCURSED) {
	msg("You can't.  It appears to be cursed.");
	return FALSE;
    }
    if (op == cur_armor) {
	waste_time();
    }
    else if (op->o_type == RING) {
	if (cur_misc[WEAR_GAUNTLET] != NULL) {
	    msg ("You have to remove your gauntlets first!");
	    return FALSE;
	}

	switch (op->o_which) {
	case R_ADDSTR:    save_max = max_stats.s_str;
			  chg_str(-op->o_ac);
			  max_stats.s_str = save_max;
	when R_ADDHIT:    pstats.s_dext -= op->o_ac;
	when R_ADDINTEL:  pstats.s_intel -= op->o_ac;
	when R_ADDWISDOM: pstats.s_wisdom -= op->o_ac;
	when R_SEEINVIS:  if (find_slot(unsee) == 0) {
				turn_off(player, CANSEE);
				msg("The tingling feeling leaves your eyes");
			  }
			  light(&hero);
			  mvwaddch(cw, hero.y, hero.x, PLAYER);
	when R_WARMTH:    turn_off(player, NOCOLD);
	when R_FIRE:   	  turn_off(player, NOFIRE);
	when R_LIGHT: {
			  if(roomin(&hero) != NULL) {
				light(&hero);
				mvwaddch(cw, hero.y, hero.x, PLAYER);
			  }
		      }
	when R_SEARCH:    kill_daemon(ring_search);
	when R_TELEPORT:  kill_daemon(ring_teleport);
	}
    }
    else if (op->o_type == MM) {
	switch (op->o_which) {
	    case MM_ADAPTION:
		turn_off(player, NOGAS);

	    when MM_STRANGLE:
		msg("You can breathe again.....whew!");
		kill_daemon(strangle);

	    when MM_DANCE:
		turn_off(player, ISDANCE);
		msg ("Your feet take a break.....whew!");

	    when MM_FUMBLE:
		kill_daemon(fumble);
	}
    }
    cur_null(op);	/* set current to NULL */
    return TRUE;
}

/*
 * return a new thing
 */
struct linked_list *
new_thing(thing_type)
int thing_type;
{
    register struct linked_list *item;
    register struct object *cur;
    register int j, k;
    register int blesschance, cursechance;

    item = new_item(sizeof *cur);
    cur = OBJPTR(item);
    cur->o_hplus = cur->o_dplus = 0;
    strcpy(cur->o_damage,"0d0");
    strcpy(cur->o_hurldmg,"0d0");
    cur->o_ac = 0;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->contents = NULL;
    cur->o_flags = 0;
    cur->o_weight = 0;
    cur->o_mark[0] = '\0';
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    blesschance = rnd(100);
    cursechance = rnd(100);

    /* Get the type of item (pick one if 'any' is specified) */
    if (thing_type == ALL) j = pick_one(things, NUMTHINGS);
    else j = thing_type;

    /* 
     * make sure he gets his vitamins 
     */
    if (thing_type == ALL && no_food > 3)
	j = 2;	
    /*
     * limit the number of foods on a level because it sometimes
     * gets out of hand in the "deep" levels where there is a 
     * treasure room on most every level with lots of food in it
     */
    while (thing_type == ALL && levtype != POSTLEV && foods_this_level > 2 &&
	   j == 2)
	j = pick_one(things, NUMTHINGS);	/* not too many.... */
    switch (j)
    {
	case TYP_POTION:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(p_magic, MAXPOTIONS);
	    cur->o_weight = things[TYP_POTION].mi_wght;
	    if (cursechance < p_magic[cur->o_which].mi_curse)
		cur->o_flags |= ISCURSED;
	    else if (blesschance < p_magic[cur->o_which].mi_bless)
		cur->o_flags |= ISBLESSED;
	when TYP_SCROLL:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(s_magic, MAXSCROLLS);
	    cur->o_weight = things[TYP_SCROLL].mi_wght;
	    if (cursechance < s_magic[cur->o_which].mi_curse)
		cur->o_flags |= ISCURSED;
	    else if (blesschance < s_magic[cur->o_which].mi_bless)
		cur->o_flags |= ISBLESSED;
	when TYP_FOOD:
	    no_food = 0;
	    cur->o_type = FOOD;
	    cur->o_weight = things[TYP_FOOD].mi_wght;
	    cur->o_count += extras();
	    foods_this_level += cur->o_count;
	    if (rnd(100) > 10)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when TYP_WEAPON:
	    cur->o_type = WEAPON;
	    cur->o_which = rnd(MAXWEAPONS);
	    init_weapon(cur, cur->o_which);
	    if (cursechance < 20)
	    {

		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(2) + 1;
		cur->o_dplus -= rnd(2) + 1;
	    }
	    else if (blesschance < 50) {

		cur->o_hplus += rnd(5) + 1;
		cur->o_dplus += rnd(5) + 1;
	    }
	when TYP_ARMOR:
	    cur->o_type = ARMOR;
	    for (j = 0; j < MAXARMORS; j++)
		if (blesschance < armors[j].a_prob)
		    break;
	    if (j == MAXARMORS)
	    {
		debug("Picked a bad armor %d", blesschance);
		j = 0;
	    }
	    cur->o_which = j;
	    cur->o_ac = armors[j].a_class;
	    cur->o_weight = armors[j].a_wght;
	    if ((k = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_ac += rnd(3)+1;
	    }
	    else if (k < 35)
		cur->o_ac -= rnd(3)+1;
	when TYP_RING:
	    cur->o_type = RING;
	    cur->o_which = pick_one(r_magic, MAXRINGS);
	    cur->o_weight = things[TYP_RING].mi_wght;
	    if (cursechance < r_magic[cur->o_which].mi_curse)
		cur->o_flags |= ISCURSED;
	    else if (blesschance < r_magic[cur->o_which].mi_bless)
		cur->o_flags |= ISBLESSED;
	    switch (cur->o_which)
	    {
		case R_ADDSTR:
		case R_ADDWISDOM:
		case R_ADDINTEL:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    cur->o_ac = rnd(2) + 1;	/* From 1 to 3 */
		    if (cur->o_flags & ISCURSED)
			cur->o_ac = -cur->o_ac;
		    if (cur->o_flags & ISBLESSED) cur->o_ac++;
		when R_DIGEST:
		    if (cur->o_flags & ISCURSED) cur->o_ac = -1;
		    else if (cur->o_flags & ISBLESSED) cur->o_ac = 2;
		    else cur->o_ac = 1;
	    }
	when TYP_STICK:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(ws_magic, MAXSTICKS);
	    fix_stick(cur);
	    if (cursechance < ws_magic[cur->o_which].mi_curse)
		cur->o_flags |= ISCURSED;
	    else if (blesschance < ws_magic[cur->o_which].mi_bless)
		cur->o_flags |= ISBLESSED;
	when TYP_MM:
	    cur->o_type = MM;
	    cur->o_which = pick_one(m_magic, MAXMM);
	    cur->o_weight = things[TYP_MM].mi_wght;
	    if (cursechance < m_magic[cur->o_which].mi_curse)
		cur->o_flags |= ISCURSED;
	    else if (blesschance < m_magic[cur->o_which].mi_bless)
		cur->o_flags |= ISBLESSED;
	    switch (cur->o_which) {
		case MM_JUG:
		    switch(rnd(7)) {
			case 0: cur->o_ac = P_PHASE;
			when 1: cur->o_ac = P_CLEAR;
			when 2: cur->o_ac = P_SEEINVIS;
			when 3: cur->o_ac = P_HEALING;
			when 4: cur->o_ac = P_MFIND;
			when 5: cur->o_ac = P_TFIND;
			when 6: cur->o_ac = P_HASTE;
			when 7: cur->o_ac = P_RESTORE;
		    }
		when MM_OPEN:
		case MM_HUNGER:
		case MM_DRUMS:
		case MM_DISAPPEAR:
		case MM_CHOKE:
		case MM_KEOGHTOM:
		    cur->o_ac = 3 + (rnd(3)+1) * 3;
		when MM_BRACERS:
		    if (cur->o_flags & ISCURSED) 
			cur->o_ac = -(rnd(3)+1);
		    else
			cur->o_ac = rnd(8)+1;
		when MM_PROTECT:
		    if (cur->o_flags & ISCURSED) 
		    	cur->o_ac = -(rnd(3)+1);
		    else
			cur->o_ac = rnd(5)+1;
		when MM_DISP:
		    cur->o_ac = 2;
		when MM_SKILLS:
		    cur->o_ac = rnd(4);	/*  set it to some character class */
		otherwise:
		    cur->o_ac = 0;
	    }
	otherwise:
	    debug("Picked a bad kind of object");
	    wait_for(msg,' ');
    }
    return item;
}

/*
 * provide a new item tailored to specification
 */
struct linked_list *
spec_item(type, which, hit, damage)
int type, which, hit, damage;
{
    register struct linked_list *item;
    register struct object *obj;

    item = new_item(sizeof *obj);
    obj = OBJPTR(item);
    obj->o_count = 1;
    obj->o_group = 0;
    obj->contents = NULL;
    obj->o_type = type;
    obj->o_which = which;
    strcpy(obj->o_damage,"0d0");
    strcpy(obj->o_hurldmg,"0d0");
    obj->o_hplus = 0;
    obj->o_dplus = 0;
    obj->o_flags = 0;
    obj->o_mark[0] = '\0';
    obj->o_text = NULL;
    obj->o_launch = 0;
    obj->o_weight = 0;

    /* Handle special characteristics */
    switch (type) {
	case WEAPON:
	    init_weapon(obj, which);
	    obj->o_hplus = hit;
	    obj->o_dplus = damage;
	    obj->o_ac = 10;

	    if (hit > 0 || damage > 0) obj->o_flags |= ISBLESSED;
	    else if (hit < 0 || damage < 0) obj->o_flags |= ISCURSED;

	when ARMOR:
	    obj->o_ac = armors[which].a_class - hit;
	    if (hit > 0) obj->o_flags |= ISBLESSED;
	    else if (hit < 0) obj->o_flags |= ISCURSED;

	when RING:
	    obj->o_ac = hit;
	    switch (obj->o_which) {
		case R_ADDSTR:
		case R_ADDWISDOM:
		case R_ADDINTEL:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		case R_DIGEST:
		    if (hit > 1) obj->o_flags |= ISBLESSED;
		    else if (hit < 0) obj->o_flags |= ISCURSED;
	    }

	when STICK:
	    fix_stick(obj);
	    obj->o_charges = hit;

	when GOLD:
	    obj->o_type = GOLD;
	    obj->o_count = GOLDCALC;
	    obj->o_ac = 11;
	
	when MM:
	    obj->o_type = MM;
	    obj->o_ac = hit;

	when RELIC:
	    /* Handle weight here since these are all created uniquely */
	    obj->o_weight = things[TYP_RELIC].mi_wght;

    }
    return(item);
}

/*
 * pick an item out of a list of nitems possible magic items
 */
pick_one(magic, nitems)
register struct magic_item *magic;
int nitems;
{
    register struct magic_item *end;
    register int i;
    register struct magic_item *start;

    start = magic;
    for (end = &magic[nitems], i = rnd(1000); magic < end; magic++)
	if (i < magic->mi_prob)
	    break;
    if (magic == end)
    {
	if (wizard)
	{
	    sprintf(outstring,"bad pick_one: %d from %d items", i, nitems);
	    msg(outstring);
	    for (magic = start; magic < end; magic++){
		sprintf(outstring,"%s: %d%%", magic->mi_name, magic->mi_prob);
		msg(outstring);
	    }
	}
	magic = start;
    }
    return (int)(magic - start);
}


/* blesscurse returns whether, according to the flag, the object is
 * blessed, cursed, or neither
 */

char *
blesscurse(flags)
int flags;
{
    if (flags & ISKNOW)  {
	if (flags & ISCURSED) return("cursed ");
	if (flags & ISBLESSED) return("blessed ");
	return("normal ");
    }
    return("");
}

/*
 * extras:
 *	Return the number of extra items to be created
 */
extras()
{
	reg int i;

	i = rnd(100);
	if (i < 4)		/* 4% for 2 more */
	    return (2);
	else if (i < 11)	/* 7% for 1 more */
	    return (1);
	else			/* otherwise no more */
	    return (0);
}
