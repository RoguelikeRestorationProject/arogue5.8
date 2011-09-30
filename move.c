/*
 * Hero movement commands
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * Based on "Super-Rogue"
 * Copyright (C) 1984 Robert D. Kindelberger
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
 * Used to hold the new hero position
 */

static coord nh;

static const char Moves[3][3] = {
    { 'y', 'k', 'u' },
    { 'h', '\0', 'l' },
    { 'b', 'j', 'n' }
};

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */

be_trapped(th, tc)
register struct thing *th;
register coord *tc;
{
    register struct trap *tp;
    register char ch;
    register const char *mname = NULL;
    register bool is_player = (th == &player),
	          can_see;
    register struct linked_list *mitem = NULL;


    /* Can the player see the creature? */
    can_see = (cansee(tc->y, tc->x) && (is_player || !invisible(th)));

    tp = trap_at(tc->y, tc->x);
    /*
     * if he's wearing boots of elvenkind, he won't set off the trap
     * unless its a magic pool (they're not really traps)
     */
    if (is_player					&&
	cur_misc[WEAR_BOOTS] != NULL			&&
	cur_misc[WEAR_BOOTS]->o_which == MM_ELF_BOOTS	&&
	tp->tr_type != POOL)
	    return '\0';

    /*
     * if the creature is flying then it won't set off the trap
     */
     if (on(*th, ISFLY))
	return '\0';

    tp->tr_flags |= ISFOUND;

    if (!is_player) {
	mitem = find_mons(th->t_pos.y, th->t_pos.x);
	mname = monsters[th->t_index].m_name;
    }
    else {
	count = running = FALSE;
	mvwaddch(cw, tp->tr_pos.y, tp->tr_pos.x, tp->tr_type);
    }
    switch (ch = tp->tr_type) {
	case TRAPDOOR:
	    if (is_player) {
		level++;
		pstats.s_hpt -= roll(1, 10);
		msg("You fell into a trap!");
		if (pstats.s_hpt <= 0) death(D_FALL);
		new_level(NORMLEV);
	    }
	    else {
		if (can_see) msg("The %s fell into a trap!", mname);

		/* See if the fall killed the monster */
		if ((th->t_stats.s_hpt -= roll(1, 10)) <= 0) {
		    killed(mitem, FALSE, FALSE);
		}
		else {	/* Just move monster to next level */
		    check_residue(th);

		    /* Erase the monster from the old position */
		    if (isalpha(mvwinch(cw, th->t_pos.y, th->t_pos.x)))
			mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
		    mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');
		    turn_on(*th, ISELSEWHERE);
		    detach(mlist, mitem);
		    attach(tlist, mitem);	/* remember him next level */
		}
	    }
	when BEARTRAP:
	    if (is_stealth(th)) {
		if (is_player) msg("You pass a bear trap.");
		else if (can_see) msg("The %s passes a bear trap.", mname);
	    }
	    else {
		th->t_no_move += BEARTIME;
		if (is_player) msg("You are caught in a bear trap.");
		else if (can_see) msg("The %s is caught in a bear trap.",
					mname);
	    }
	when SLEEPTRAP:
	    if (is_player) {
		msg("A strange white mist envelops you.");
		if (!ISWEARING(R_ALERT)) {
		    msg("You fall asleep.");
		    no_command += SLEEPTIME;
		}
	    }
	    else {
		if (can_see) 
		    msg("A strange white mist envelops the %s.",mname);
		if (on(*th, ISUNDEAD)) {
		    if (can_see) 
			msg("The mist doesn't seem to affect the %s.",mname);
		}
		else {
		    th->t_no_move += SLEEPTIME;
		}
	    }
	when ARROWTRAP:
	    if (swing(th->t_ctype, th->t_stats.s_lvl-1, th->t_stats.s_arm, 1))
	    {
		if (is_player) {
		    msg("Oh no! An arrow shot you.");
		    if ((pstats.s_hpt -= roll(1, 6)) <= 0) {
			msg("The arrow killed you.");
			death(D_ARROW);
		    }
		}
		else {
		    if (can_see) msg("An arrow shot the %s.", mname);
		    if ((th->t_stats.s_hpt -= roll(1, 6)) <= 0) {
			if (can_see) msg("The arrow killed the %s.", mname);
			killed(mitem, FALSE, FALSE);
		    }
		}
	    }
	    else
	    {
		register struct linked_list *item;
		register struct object *arrow;

		if (is_player) msg("An arrow shoots past you.");
		else if (can_see) msg("An arrow shoots by the %s.", mname);
		item = new_item(sizeof *arrow);
		arrow = OBJPTR(item);
		arrow->o_type = WEAPON;
		arrow->contents = NULL;
		arrow->o_which = ARROW;
		arrow->o_hplus = rnd(3) - 1;
		arrow->o_dplus = rnd(3) - 1;
		init_weapon(arrow, ARROW);
		arrow->o_count = 1;
		arrow->o_pos = *tc;
		arrow->o_mark[0] = '\0';
		fall(item, FALSE);
	    }
	when TELTRAP:
	    if (is_player) teleport();
	    else {
		register int rm;
	        struct room *old_room;	/* old room of monster */

		/* 
		 * Erase the monster from the old position 
		 */
		if (isalpha(mvwinch(cw, th->t_pos.y, th->t_pos.x)))
		    mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
		mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');
		/*
		 * check to see if room should go dark
		 */
		if (on(*th, HASFIRE)) {
		    old_room=roomin(&th->t_pos);
		    if (old_room != NULL) {
			register struct linked_list *fire_item;

			for (fire_item = old_room->r_fires; fire_item != NULL;
			     fire_item = next(fire_item)) {
			    if (THINGPTR(fire_item) == th) {
				detach(old_room->r_fires, fire_item);
				destroy_item(fire_item);

				if (old_room->r_fires == NULL) {
				    old_room->r_flags &= ~HASFIRE;
				    if (can_see) light(&hero);
				}
			    }
			}
		    }
		}

		/* Get a new position */
		do {
		    rm = rnd_room();
		    rnd_pos(&rooms[rm], &th->t_pos);
		} until(winat(th->t_pos.y, th->t_pos.x) == FLOOR);

		/* Put it there */
		mvwaddch(mw, th->t_pos.y, th->t_pos.x, th->t_type);
		th->t_oldch = CCHAR( mvwinch(cw, th->t_pos.y, th->t_pos.x) );
		/*
		 * check to see if room that creature appears in should
		 * light up
		 */
	        if (on(*th, HASFIRE)) {
		    register struct linked_list *fire_item;

		    fire_item = creat_item();
		    ldata(fire_item) = (char *) th;
		    attach(rooms[rm].r_fires, fire_item);

		    rooms[rm].r_flags |= HASFIRE;
		    if(cansee(th->t_pos.y, th->t_pos.x) && 
		       next(rooms[rm].r_fires) == NULL)
			light(&hero);
		}
		if (can_see) msg("The %s seems to have disappeared!", mname);
	    }
	when DARTTRAP:
	    if (swing(th->t_ctype, th->t_stats.s_lvl+1, th->t_stats.s_arm, 1)) {
		if (is_player) {
		    msg("A small dart just hit you in the shoulder.");
		    if ((pstats.s_hpt -= roll(1, 4)) <= 0) {
			msg("The dart killed you.");
			death(D_DART);
		    }

		    /* Now the poison */
		    if (!save(VS_POISON, &player, 0)) {
			/* 75% chance it will do point damage - else strength */
			if (rnd(100) < 75) {
			    pstats.s_hpt /= 2;
			    if (pstats.s_hpt == 0) death(D_POISON);
			}
			else if (!ISWEARING(R_SUSABILITY))
				chg_str(-1);
		    }
		}
		else {
		    if (can_see)
			msg("A small dart just hit the %s in the shoulder.",
				mname);
		    if ((th->t_stats.s_hpt -= roll(1,4)) <= 0) {
			if (can_see) msg("The dart killed the %s.", mname);
			killed(mitem, FALSE, FALSE);
		    }
		    if (!save(VS_POISON, th, 0)) {
			th->t_stats.s_hpt /= 2;
			if (th->t_stats.s_hpt <= 0) {
			    if (can_see) msg("The dart killed the %s.", mname);
			    killed(mitem, FALSE, FALSE);
			}
		    }
		}
	    }
	    else {
		if (is_player)
		    msg("A small dart whizzes by your ear and vanishes.");
		else if (can_see)
		    msg("A small dart whizzes by the %s's ear and vanishes.",
			mname);
	    }
        when POOL: {
	    register int i;

	    i = rnd(100);
	    if (is_player) {
		if ((tp->tr_flags & ISGONE)) {
		    if (i < 30) {
			teleport();	   /* teleport away */
			pool_teleport = TRUE;
		    }
		    else if((i < 45) && level > 2) {
			level -= rnd(2) + 1;
			cur_max = level;
			new_level(NORMLEV);
			pool_teleport = TRUE;
			msg("You here a faint groan from below.");
		    }
		    else if(i < 70) {
			level += rnd(4) + 1;
			new_level(NORMLEV);
			pool_teleport = TRUE;
			msg("You find yourself in strange surroundings.");
		    }
		    else if(i > 95) {
			msg("Oh no!!! You drown in the pool!!! --More--");
			wait_for(cw,' ');
			death(D_DROWN);
		    }
		}
	    }
	    else {
		if (i < 60) {
		    if (can_see) {
			/* Drowns */
			if (i < 30) msg("The %s drowned in the pool!", mname);

			/* Teleported to another level */
			else msg("The %s disappeared!", mname);
		    }
		    killed(mitem, FALSE, FALSE);
		}
	    }
	}
    when MAZETRAP:
	if (is_player) {
	    pstats.s_hpt -= roll(1, 10);
	    level++;
	    msg("You fell through a trap door!");
	    if (pstats.s_hpt <= 0) death(D_FALL);
	    new_level(MAZELEV);
	    msg("You are surrounded by twisty passages!");
	}
	else {
	    if (can_see) msg("The %s fell into a trap!", mname);
	    killed(mitem, FALSE, FALSE);
	}
    }

    /* Move the cursor back onto the hero */
    wmove(cw, hero.y, hero.x);

    md_flushinp(); /* flush typeahead */

    return(ch);
}

/*
 * blue_light:
 *	magically light up a room (or level or make it dark)
 */

bool
blue_light(blessed, cursed)
bool blessed, cursed;
{
    register struct room *rp;
    bool ret_val=FALSE;	/* Whether or not affect is known */

    rp = roomin(&hero);	/* What room is hero in? */

    /* Darken the room if the magic is cursed */
    if (cursed) {
	if ((rp == NULL) || !lit_room(rp)) msg(nothing);
	else {
	    rp->r_flags |= ISDARK;
	    if (!lit_room(rp) && (levtype != OUTSIDE || !daytime))
		msg("The %s suddenly goes dark.",
			levtype == OUTSIDE ? "area" : "room");
	    else msg(nothing);
	    ret_val = TRUE;
	}
    }
    else {
	ret_val = TRUE;
	if (rp && !lit_room(rp) &&
	    (levtype != OUTSIDE || !daytime)) {
	    addmsg("The %s is lit", levtype == OUTSIDE ? "area" : "room");
	    if (!terse)
		addmsg(" by a %s blue light.",
		    blessed ? "bright" : "shimmering");
	    endmsg();
	}
	else if (winat(hero.y, hero.x) == PASSAGE)
	    msg("The corridor glows %sand then fades",
		    blessed ? "brightly " : "");
	else {
	    ret_val = FALSE;
	    msg(nothing);
	}
	if (blessed) {
	    register int i;	/* Index through rooms */

	    for (i=0; i<MAXROOMS; i++)
		rooms[i].r_flags &= ~ISDARK;
	}
	else if (rp) rp->r_flags &= ~ISDARK;
    }

    /*
     * Light the room and put the player back up
     */
    light(&hero);
    mvwaddch(cw, hero.y, hero.x, PLAYER);
    return(ret_val);
}

/*
 * corr_move:
 *	Check to see that a move is legal.  If so, return correct character.
 * 	If not, if player came from a legal place, then try to turn him.
 */

corr_move(dy, dx)
int dy, dx;
{
    int legal=0;		/* Number of legal alternatives */
    register int y, x,		/* Indexes though possible positions */
		 locy = 0, locx = 0;	/* Hold delta of chosen location */

    /* New position */
    nh.y = hero.y + dy;
    nh.x = hero.x + dx;

    /* If it is a legal move, just return */
    if (nh.x >= 0 && nh.x < COLS && nh.y > 0 && nh.y < LINES - 2) {
        
	switch (winat(nh.y, nh.x)) {
	    case WALL:
	    case '|':
	    case '-':
		break;
	    default:
		if (diag_ok(&hero, &nh, &player))
			return;
	}
    }

    /* Check legal places surrounding the player -- ignore previous position */
    for (y = hero.y - 1; y <= hero.y + 1; y++) {
	if (y < 1 || y > LINES - 3)
	    continue;
	for (x = hero.x - 1; x <= hero.x + 1; x++) {
	    /* Ignore borders of the screen */
	    if (x < 0 || x > COLS - 1)
		continue;
	    
	    /* 
	     * Ignore where we came from, where we are, and where we couldn't go
	     */
	    if ((x == hero.x - dx && y == hero.y - dy) ||
		(x == hero.x + dx && y == hero.y + dy) ||
		(x == hero.x && y == hero.y))
		continue;

	    switch (winat(y, x)) {
		case WALL:
		case '|':
		case '-':
		    break;
		default:
		    nh.y = y;
		    nh.x = x;
		    if (diag_ok(&hero, &nh, &player)) {
			legal++;
			locy = y - (hero.y - 1);
			locx = x - (hero.x - 1);
		    }
	    }
	}
    }

    /* If we have 2 or more legal moves, make no change */
    if (legal != 1) {
	return;
    }

    runch = Moves[locy][locx];

    /*
     * For mazes, pretend like it is the beginning of a new run at each turn
     * in order to get the lighting correct.
     */
    if (levtype == MAZELEV) firstmove = TRUE;
    return;
}

/*
 * dip_it:
 *	Dip an object into a magic pool
 */
dip_it()
{
	reg struct linked_list *what;
	reg struct object *ob;
	reg struct trap *tp;
	reg int wh, i;

	tp = trap_at(hero.y,hero.x);
	if (tp == NULL || tp->tr_type != POOL) {
	    msg("I see no shimmering pool here");
	    return;
	}
	if (tp->tr_flags & ISGONE) {
	    msg("This shimmering pool appears to have used once already");
	    return;
	}
	if ((what = get_item(pack, "dip", ALL)) == NULL) {
	    msg("");
	    after = FALSE;
	    return;
	}
	ob = OBJPTR(what);
	mpos = 0;
	if (ob == cur_armor		 || 
	    ob == cur_misc[WEAR_BOOTS]	 || ob == cur_misc[WEAR_JEWEL]	 ||
	    ob == cur_misc[WEAR_GAUNTLET]|| ob == cur_misc[WEAR_CLOAK]	 ||
	    ob == cur_misc[WEAR_BRACERS] || ob == cur_misc[WEAR_NECKLACE]||
	    ob == cur_ring[LEFT_1]	 || ob == cur_ring[LEFT_2]	 ||
	    ob == cur_ring[LEFT_3]	 || ob == cur_ring[LEFT_4]	 ||
	    ob == cur_ring[RIGHT_1]	 || ob == cur_ring[RIGHT_2]	 ||
	    ob == cur_ring[RIGHT_3]	 || ob == cur_ring[RIGHT_4]) {
	    msg("You'll have to take it off first.");
	    return;
	}
	tp->tr_flags |= ISGONE;
	if (ob != NULL) {
	    wh = ob->o_which;
	    ob->o_flags |= ISKNOW;
	    i = rnd(100);
	    switch(ob->o_type) {
		case WEAPON:
		    if(i < 50) {		/* enchant weapon here */
			if ((ob->o_flags & ISCURSED) == 0) {
				ob->o_hplus += 1;
				ob->o_dplus += 1;
			}
			else {		/* weapon was prev cursed here */
				ob->o_hplus = rnd(2);
				ob->o_dplus = rnd(2);
			}
			ob->o_flags &= ~ISCURSED;
		        msg("The %s glows blue for a moment.",weaps[wh].w_name);
		    }
		    else if(i < 70) {	/* curse weapon here */
			if ((ob->o_flags & ISCURSED) == 0) {
				ob->o_hplus = -(rnd(2)+1);
				ob->o_dplus = -(rnd(2)+1);
			}
			else {			/* if already cursed */
				ob->o_hplus--;
				ob->o_dplus--;
			}
			ob->o_flags |= ISCURSED;
		        msg("The %s glows red for a moment.",weaps[wh].w_name);
		    }			
		    else
			msg(nothing);
		when ARMOR:
		    if (i < 50) {	/* enchant armor */
			if((ob->o_flags & ISCURSED) == 0)
			    ob->o_ac -= rnd(2) + 1;
			else
			    ob->o_ac = -rnd(3)+ armors[wh].a_class;
			ob->o_flags &= ~ISCURSED;
		        msg("The %s glows blue for a moment",armors[wh].a_name);
		    }
		    else if(i < 75){	/* curse armor */
			if ((ob->o_flags & ISCURSED) == 0)
			    ob->o_ac = rnd(3)+ armors[wh].a_class;
			else
			    ob->o_ac += rnd(2) + 1;
			ob->o_flags |= ISCURSED;
		        msg("The %s glows red for a moment.",armors[wh].a_name);
		    }
		    else
			msg(nothing);
		when STICK: {
		    int j;
		    j = rnd(8) + 1;
		    if(i < 50) {		/* add charges */
			ob->o_charges += j;
		        ws_know[wh] = TRUE;
			if (ob->o_flags & ISCURSED)
			    ob->o_flags &= ~ISCURSED;
		        sprintf(outstring,"The %s %s glows blue for a moment.",
			    ws_made[wh],ws_type[wh]);
			msg(outstring);
		    }
		    else if(i < 65) {	/* remove charges */
			if ((ob->o_charges -= i) < 0)
			    ob->o_charges = 0;
		        ws_know[wh] = TRUE;
			if (ob->o_flags & ISBLESSED)
			    ob->o_flags &= ~ISBLESSED;
			else
			    ob->o_flags |= ISCURSED;
		        sprintf(outstring,"The %s %s glows red for a moment.",
			    ws_made[wh],ws_type[wh]);
			msg(outstring);
		    }
		    else 
			msg(nothing);
		}
		when SCROLL:
		    s_know[wh] = TRUE;
		    msg("The '%s' scroll unfurls.",s_names[wh]);
		when POTION:
		    p_know[wh] = TRUE;
		    msg("The %s potion bubbles for a moment.",p_colors[wh]);
		when RING:
		    if(i < 50) {	 /* enchant ring */
			if ((ob->o_flags & ISCURSED) == 0)
			    ob->o_ac += rnd(2) + 1;
			else
			    ob->o_ac = rnd(2) + 1;
			ob->o_flags &= ~ISCURSED;
		    }
		    else if(i < 80) { /* curse ring */
			if ((ob->o_flags & ISCURSED) == 0)
			    ob->o_ac = -(rnd(2) + 1);
			else
			    ob->o_ac -= (rnd(2) + 1);
			ob->o_flags |= ISCURSED;
		    }
		    r_know[wh] = TRUE;
		    msg("The %s ring vibrates for a moment.",r_stones[wh]);
		when MM:
		    m_know[wh] = TRUE;
		    switch (ob->o_which) {
		    case MM_BRACERS:
		    case MM_PROTECT:
			if(i < 50) {	 /* enchant item */
			    if ((ob->o_flags & ISCURSED) == 0)
				ob->o_ac += rnd(2) + 1;
			    else
				ob->o_ac = rnd(2) + 1;
			    ob->o_flags &= ~ISCURSED;
			}
			else if(i < 80) { /* curse item */
			    if ((ob->o_flags & ISCURSED) == 0)
				ob->o_ac = -(rnd(2) + 1);
			    else
				ob->o_ac -= (rnd(2) + 1);
			    ob->o_flags |= ISCURSED;
			}
			msg("The item vibrates for a moment.");
		    when MM_CHOKE:
		    case MM_DISAPPEAR:
			ob->o_ac = 0;
			msg ("The dust dissolves in the pool!");
		    }
		otherwise:
		msg("The pool bubbles for a moment.");
	    }
	    updpack(FALSE);
	}
	else
	    msg(nothing);
}

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */

do_move(dy, dx)
int dy, dx;
{
    register struct room *rp, *orp;
    register char ch;
    coord old_hero;
    int i, wasfirstmove;

    wasfirstmove = firstmove;
    firstmove = FALSE;
    curprice = -1;		/* if in trading post, we've moved off obj */
    if (player.t_no_move) {
	player.t_no_move--;
	msg("You are still stuck in the bear trap");
	return;
    }
    /*
     * Do a confused move (maybe)
     */
    if ((on(player, ISHUH) && rnd(100) < 80) || 
	(on(player, ISDANCE) && rnd(100) < 80) || 
	(ISWEARING(R_DELUSION) && rnd(100) < 25))
	nh = *rndmove(&player);
    else {
	nh.y = hero.y + dy;
	nh.x = hero.x + dx;
    }

    /*
     * Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did.
     */
    if (nh.x < 0 || nh.x > COLS-1 || nh.y < 1 || nh.y >= LINES - 2
	|| !diag_ok(&hero, &nh, &player))
    {
	after = running = FALSE;
	return;
    }
    if (running && ce(hero, nh))
	after = running = FALSE;
    ch = CCHAR( winat(nh.y, nh.x) );

    /* Take care of hero trying to move close to something frightening */
    if (on(player, ISFLEE)) {
	if (rnd(100) < 10) {
	    turn_off(player, ISFLEE);
	    msg("You regain your composure.");
	}
	else if (DISTANCE(nh.y, nh.x, player.t_dest->y, player.t_dest->x) <
		 DISTANCE(hero.y, hero.x, player.t_dest->y, player.t_dest->x))
			return;
    }

    /* Take care of hero being held */
    if (on(player, ISHELD) && !isalpha(ch))
    {
	msg("You are being held");
	return;
    }

    /* assume he's not in a wall */
    if (!isalpha(ch)) turn_off(player, ISINWALL);

    switch(ch) {
	case '|':
	case '-':
	    if (levtype == OUTSIDE) {
		hero = nh;
		new_level(OUTSIDE);
		return;
	    }
	case WALL:
	case SECRETDOOR:
	    if (off(player, CANINWALL) || running) {
	        after = running = FALSE;

		/* Light if finishing run */
		if (levtype == MAZELEV && lit_room(&rooms[0]))
		    look(FALSE, TRUE);

	        after = running = FALSE;

	        return;
	    }
	    turn_on(player, ISINWALL);
	    break;
	case POOL:
	    if (levtype == OUTSIDE) {
		lake_check(&nh);
		running = FALSE;
		break;
	    }
	case MAZETRAP:
	    if (levtype == OUTSIDE) {
	    running = FALSE;
	    break;
	}
	case TRAPDOOR:
	case TELTRAP:
	case BEARTRAP:
	case SLEEPTRAP:
	case ARROWTRAP:
	case DARTTRAP:
	    ch = be_trapped(&player, &nh);
	    if (ch == TRAPDOOR || ch == TELTRAP || 
		pool_teleport  || ch == MAZETRAP) {
		pool_teleport = FALSE;
		return;
	    }
	    break;
	case GOLD:
	case POTION:
	case SCROLL:
	case FOOD:
	case WEAPON:
	case ARMOR:
	case RING:
	case MM:
	case RELIC:
	case STICK:
	    running = FALSE;
	    take = ch;
	    break;
    	case DOOR:
    	case STAIRS:
	    running = FALSE;
	    break;
	case POST:
	    running = FALSE;
	    new_level(POSTLEV);
	    return;
	default:
	    break;
    }

    if (isalpha(ch)) { /* if its a monster then fight it */
	running = FALSE;
	i = 1;
	if (player.t_ctype == C_FIGHTER)
	   i += pstats.s_lvl/10;
	while (i--)
	    fight(&nh, cur_weapon, FALSE);
	return;
    }

    /*
     * if not fighting then move the hero
     */
    old_hero = hero;	/* Save hero's old position */
    hero = nh;		/* Move the hero */
    rp = roomin(&hero);
    orp = roomin(&old_hero);

    /* Unlight any possible cross-corridor */
    if (levtype == MAZELEV) {
	register bool call_light = FALSE;
	register char wall_check;

	if (wasfirstmove && lit_room(&rooms[0])) {
	    /* Are we moving out of a corridor? */
	    switch (runch) {
		case 'h':
		case 'l':
		    if (old_hero.y + 1 < LINES - 2) {
			wall_check = CCHAR( winat(old_hero.y + 1, old_hero.x) );
			if (!isrock(wall_check)) call_light = TRUE;
		    }
		    if (old_hero.y - 1 > 0) {
			wall_check = CCHAR( winat(old_hero.y - 1, old_hero.x) );
			if (!isrock(wall_check)) call_light = TRUE;
		    }
		    break;
		case 'j':
		case 'k':
		    if (old_hero.x + 1 < COLS) {
			wall_check = CCHAR( winat(old_hero.y, old_hero.x + 1) );
			if (!isrock(wall_check)) call_light = TRUE;
		    }
		    if (old_hero.x - 1 >= 0) {
			wall_check = CCHAR( winat(old_hero.y, old_hero.x - 1) );
			if (!isrock(wall_check)) call_light = TRUE;
		    }
		    break;
		default:
		    call_light = TRUE;
	    }
	    player.t_oldpos = old_hero;
	    if (call_light) light(&old_hero);
	}
    }

    else if (orp != NULL && rp == NULL) {    /* Leaving a room -- darken it */
	orp->r_flags |= FORCEDARK;	/* Fake darkness */
	light(&old_hero);
	orp->r_flags &= ~FORCEDARK;	/* Restore light state */
    }
    else if (rp != NULL && orp == NULL){/* Entering a room */
	light(&hero);
    }
    ch = CCHAR( winat(old_hero.y, old_hero.x) );
    wmove(cw, unc(old_hero));
    waddch(cw, ch);
    wmove(cw, unc(hero));
    waddch(cw, PLAYER);
}

/*
 * do_run:
 *	Start the hero running
 */

do_run(ch)
char ch;
{
    firstmove = TRUE;
    running = TRUE;
    after = FALSE;
    runch = ch;
}

/*
 * getdelta:
 *	Takes a movement character (eg. h, j, k, l) and returns the
 *	y and x delta corresponding to it in the remaining arguments.
 *	Returns TRUE if it could find it, FALSE otherwise.
 */
bool
getdelta(match, dy, dx)
char match;
int *dy, *dx;
{
    register y, x;

    for (y = 0; y < 3; y++)
	for (x = 0; x < 3; x++)
	    if (Moves[y][x] == match) {
		*dy = y - 1;
		*dx = x - 1;
		return(TRUE);
	    }

    return(FALSE);
}

/*
 * isatrap:
 *	Returns TRUE if this character is some kind of trap
 */
isatrap(ch)
reg char ch;
{
	switch(ch) {
		case DARTTRAP:
		case TELTRAP:
		case TRAPDOOR:
		case ARROWTRAP:
		case SLEEPTRAP:
		case BEARTRAP:	return(TRUE);
		case MAZETRAP:
		case POOL:	return(levtype != OUTSIDE);
		default:	return(FALSE);
	}
}

/*
 * Called to illuminate a room.
 * If it is dark, remove anything that might move.
 */

light(cp)
coord *cp;
{
    register struct room *rp;
    register int j, k, x, y;
    register char ch, rch, sch;
    register struct linked_list *item;
    int jlow, jhigh, klow, khigh;	/* Boundaries of lit area */

    if ((rp = roomin(cp)) != NULL) {
	/*
	 * is he wearing ring of illumination? 
	 */
	if (&hero == cp && ISWEARING(R_LIGHT)) /* Must be hero's room */
	    rp->r_flags &= ~ISDARK;
	
	/* If we are in a maze, don't look at the whole room (level) */
	if (levtype == MAZELEV) {
	    int see_radius;

	    see_radius = 1;

	    /* If we are looking at the hero in a rock, broaden our sights */
	    if (&hero == cp || &player.t_oldpos == cp) {
		ch = CCHAR( winat(hero.y, hero.x) );
		if (isrock(ch)) see_radius = 2;
		ch = CCHAR( winat(player.t_oldpos.y, player.t_oldpos.x) );
		if (isrock(ch)) see_radius = 2;
	    }

	    jlow = max(0, cp->y - see_radius - rp->r_pos.y);
	    jhigh = min(rp->r_max.y, cp->y + see_radius + 1 - rp->r_pos.y);
	    klow = max(0, cp->x - see_radius - rp->r_pos.x);
	    khigh = min(rp->r_max.x, cp->x + see_radius + 1 - rp->r_pos.x);
	}
	else {
	    jlow = klow = 0;
	    jhigh = rp->r_max.y;
	    khigh = rp->r_max.x;
	}
	for (j = 0; j < rp->r_max.y; j++)
	{
	    for (k = 0; k < rp->r_max.x; k++)
	    {
		bool see_here = 0, see_before = 0;

		/* Is this in the give area -- needed for maze */
		if ((j < jlow || j >= jhigh) && (k < klow || k >= khigh))
		    continue;

		y = rp->r_pos.y + j;
		x = rp->r_pos.x + k;

		/*
		 * If we are in a maze do not look at this area unless
		 * we can see it from where we are or where we last were
		 * (for erasing purposes).
		 */
		if (levtype == MAZELEV) {
		    /* If we can't see it from here, could we see it before? */
		    if ((see_here = maze_view(y, x)) == FALSE) {
			coord savhero;

			/* Could we see it from where we were? */
			savhero = hero;
			hero = player.t_oldpos;
			see_before = maze_view(y, x);
			hero = savhero;

			if (!see_before) continue;
		    }
		}

		ch = show(y, x);
		wmove(cw, y, x);
		/*
		 * Figure out how to display a secret door
		 */
		if (ch == SECRETDOOR) {
		    if (j == 0 || j == rp->r_max.y - 1)
			ch = '-';
		    else
			ch = '|';
		}
		/* For monsters, if they were previously not seen and
		 * now can be seen, or vice-versa, make sure that will
		 * happen.  This is for dark rooms as opposed to invisibility.
		 *
		 * Call winat() in the test because ch will not reveal
		 * invisible monsters.
		 */
		if (isalpha(winat(y, x))) {
		    struct thing *tp;	/* The monster */

		    item = wake_monster(y, x);
		    tp = THINGPTR(item);

		    /* Previously not seen -- now can see it */
		    if (tp->t_oldch == ' ' && cansee(tp->t_pos.y, tp->t_pos.x)) 
			tp->t_oldch = CCHAR( mvinch(y, x) );

		    /* Previously seen -- now can't see it */
		    else if (!cansee(tp->t_pos.y, tp->t_pos.x) &&
			     roomin(&tp->t_pos) != NULL)
			switch (tp->t_oldch) {
			    /*
			     * Only blank it out if it is in a room and not
			     * the border (or other wall) of the room.
			     */
			     case DOOR:
			     case SECRETDOOR:
			     case '-':
			     case '|':
				break;

			     otherwise:
				tp->t_oldch = ' ';
			}
		}

		/*
		 * If the room is a dark room, we might want to remove
		 * monsters and the like from it (since they might
		 * move).
		 * A dark room.
		 */
		if ((!lit_room(rp) && (levtype != OUTSIDE)) ||
		    (levtype == OUTSIDE && !daytime) ||
		    on(player, ISBLIND) 	||
		    (rp->r_flags & FORCEDARK)	||
		    (levtype == MAZELEV && !see_here && see_before)) {
		    sch = CCHAR( mvwinch(cw, y, x) );	/* What's seen */
		    rch = CCHAR( mvinch(y, x) );	/* What's really there */
		    switch (rch) {
			case DOOR:
			case SECRETDOOR:
			case STAIRS:
			case TRAPDOOR:
			case TELTRAP:
			case BEARTRAP:
			case SLEEPTRAP:
			case ARROWTRAP:
			case DARTTRAP:
			case MAZETRAP:
			case POOL:
			case POST:
			case '|':
			case '-':
			case WALL:
			    if (isalpha(sch)) ch = rch;
			    else if (sch != FLOOR) ch = sch;
			    else ch = ' '; /* Hide undiscoverd things */
			when FLOOR:
			    ch = ' ';
			otherwise:
			    ch = ' ';
		    }
		    /* Take care of our magic bookkeeping. */
		    switch (sch) {
			case MAGIC:
			case BMAGIC:
			case CMAGIC:
			    ch = sch;
		    }
		}
		mvwaddch(cw, y, x, ch);
	    }
	}
    }
}

/*
 * lit_room:
 * 	Called to see if the specified room is lit up or not.
 */

bool
lit_room(rp)
register struct room *rp;
{
    register struct linked_list *fire_item;
    register struct thing *fire_creature;

    if (!(rp->r_flags & ISDARK)) return(TRUE);	/* A definitely lit room */

    /* Is it lit by fire light? */
    if (rp->r_flags & HASFIRE) {
	switch (levtype) {
	    case MAZELEV:
		/* See if a fire creature is in line of sight */
		for (fire_item = rp->r_fires; fire_item != NULL;
		     fire_item = next(fire_item)) {
		    fire_creature = THINGPTR(fire_item);
		    if (maze_view(fire_creature->t_pos.y,
				  fire_creature->t_pos.x)) return(TRUE);
		}

		/* Couldn't find any in line-of-sight */
		return(FALSE);

	    /* We should probably do something special for the outside */
	    otherwise:
		return TRUE;
	}
    }
    return(FALSE);
}

/*
 * rndmove:
 *	move in a random direction if the monster/person is confused
 */

coord *
rndmove(who)
struct thing *who;
{
    register int x, y;
    register int ex, ey, nopen = 0;
    static coord ret;  /* what we will be returning */
    static coord dest;

    ret = who->t_pos;
    /*
     * Now go through the spaces surrounding the player and
     * set that place in the array to true if the space can be
     * moved into
     */
    ey = ret.y + 1;
    ex = ret.x + 1;
    for (y = who->t_pos.y - 1; y <= ey; y++)
	if (y > 0 && y < LINES - 2)
	    for (x = who->t_pos.x - 1; x <= ex; x++)
	    {
		if (x < 0 || x >= COLS)
		    continue;
		if (step_ok(y, x, NOMONST, who) == TRUE)
		{
		    dest.y = y;
		    dest.x = x;
		    if (!diag_ok(&who->t_pos, &dest, who))
			continue;
		    if (rnd(++nopen) == 0)
			ret = dest;
		}
	    }
    return &ret;
}



/*
 * set_trap:
 *	set a trap at (y, x) on screen.
 */

set_trap(tp, y, x)
register struct thing *tp;
register int y, x;
{
    register bool is_player = (tp == &player);
    register char selection = rnd(7) + '1';
    register char ch = 0, och;
    int thief_bonus = 0;
    int s_dext;

    switch (och = CCHAR( mvinch(y, x) )) {
	case WALL:
	case FLOOR:
	case PASSAGE:
	    break;
	default:
	    msg("The trap failed!");
	    return;
    }

    if (is_player && player.t_ctype == C_THIEF) thief_bonus = 30;

    s_dext = (tp == &player) ? dex_compute() : tp->t_stats.s_dext;

    if (ntraps >= MAXTRAPS || ++trap_tries >= MAXTRPTRY || levtype == POSTLEV ||
	rnd(80) >= (s_dext + tp->t_stats.s_lvl/2 + thief_bonus)) {
	if (is_player) msg("The trap failed!");
	return;
    }


    if (is_player) {
	int state = 0; /* 0 -> current screen, 1 -> prompt screen, 2 -> done */

	msg("Which kind of trap do you wish to set? (* for a list): ");
	do {
	    selection = tolower(readchar());
	    switch (selection) {
		case '*':
		  if (state != 1) {
		    wclear(hw);
		    touchwin(hw);
		    mvwaddstr(hw, 2, 0,	"[1] Trap Door\n[2] Bear Trap\n");
		    waddstr(hw,		"[3] Sleep Trap\n[4] Arrow Trap\n");
		    waddstr(hw,		"[5] Teleport Trap\n[6] Dart Trap\n");
		    if (wizard) {
			waddstr(hw,	"[7] Magic pool\n[8] Maze Trap\n");
			waddstr(hw,	"[9] Trading Post\n");
		    }	
		    mvwaddstr(hw, 0, 0, "Which kind of trap do you wish to set? ");
		    draw(hw);
		    state = 1;	/* Now in prompt window */
		  }
		  break;

		case ESCAPE:
		    if (state == 1) {
			clearok(cw, TRUE); /* Set up for redraw */
			touchwin(cw);
		    }
		    msg("");

		    trap_tries--;	/* Don't count this one */
		    after = FALSE;
		    return;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    if (selection < '7' || wizard) {
			if (state == 1) {	/* In prompt window */
			    clearok(cw, TRUE); /* Set up for redraw */
			    touchwin(cw);
			}

			msg("");

			/* Make sure there is a floor below us for trap doors */
			if (selection == '1' && level >= nfloors) {
			    if (state == 1) draw(cw);
			    msg("There is no level below this one.");
			    return;
			}
			state = 2;	/* Finished */
			break;
		    }

		    /* Fall through for non-wizard, unusual trap case */
		default:
		    if (state == 1) {	/* In the prompt window */
			mvwaddstr(hw, 0, 0, "Please enter a selection between 1 and 6:  ");
			draw(hw);
		    }
		    else {	/* Normal window */
			mpos = 0;
			msg("Please enter a selection between 1 and 6:  ");
		    }
	    }
	} while (state != 2);
    }

    switch (selection) {
	case '1': ch = TRAPDOOR;
	when '2': ch = BEARTRAP;
	when '3': ch = SLEEPTRAP;
	when '4': ch = ARROWTRAP;
	when '5': ch = TELTRAP;
	when '6': ch = DARTTRAP;
	when '7': ch = POOL;
	when '8': ch = MAZETRAP;
	when '9': ch = POST;
    }

    mvaddch(y, x, ch);
    traps[ntraps].tr_show = och;
    traps[ntraps].tr_type = ch;
    traps[ntraps].tr_pos.y = y;
    traps[ntraps].tr_pos.x = x;
    if (is_player) 
	traps[ntraps].tr_flags = ISTHIEFSET;
    if (ch == POOL || ch == POST) {
	traps[ntraps].tr_flags |= ISFOUND;
    }

    ntraps++;
}

/*
 * show:
 *	returns what a certain thing will display as to the un-initiated
 */

show(y, x)
register int y, x;
{
    register char ch = CCHAR( winat(y, x) );
    register struct linked_list *it;
    register struct thing *tp;

    if (isatrap(ch)) {
	register struct trap *trp = trap_at(y, x);

	return (trp->tr_flags & ISFOUND) ? ch : trp->tr_show;
    }
    else if (isalpha(ch)) {
	if ((it = find_mons(y, x)) == NULL) {
	    msg("Can't find monster in show");
	    return(mvwinch(stdscr, y, x));
	}
	tp = THINGPTR(it);

	if (on(*tp, ISDISGUISE)) ch = tp->t_disguise; /* As a mimic */

	/* Hide invisible creatures */
	else if (invisible(tp)) {
	    /* We can't see surprise-type creatures through "see invisible" */
	    if (off(player,CANSEE) || on(*tp,CANSURPRISE))
		ch = CCHAR( mvwinch(stdscr, y, x) ); /* Invisible */
	}
	else if (on(*tp, CANINWALL)) {
	    if (isrock(mvwinch(stdscr, y, x))) ch = CCHAR( winch(stdscr) ); /* As Xorn */
	}
    }
    return ch;
}


/*
 * trap_at:
 *	find the trap at (y,x) on screen.
 */

struct trap *
trap_at(y, x)
register int y, x;
{
    register struct trap *tp, *ep;

    ep = &traps[ntraps];
    for (tp = traps; tp < ep; tp++)
	if (tp->tr_pos.y == y && tp->tr_pos.x == x)
	    break;
    if (tp == ep)
	debug((sprintf(prbuf, "Trap at %d,%d not in array", y, x), prbuf));
    return tp;
}
