/*
 * This file contains functions for dealing with special player abilities
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "curses.h"
#include "rogue.h"


/*
 * affect:
 *	cleric affecting undead
 */

affect()
{
    register struct linked_list *item;
    register struct thing *tp;
    register const char *mname;
    bool see;
    coord new_pos;

    if (player.t_ctype != C_CLERIC && cur_relic[HEIL_ANKH] == 0) {
	msg("Only clerics can affect undead.");
	return;
    }

    new_pos.y = hero.y + delta.y;
    new_pos.x = hero.x + delta.x;

    if (cansee(new_pos.y, new_pos.x)) see = TRUE;
    else see = FALSE;

    /* Anything there? */
    if (new_pos.y < 0 || new_pos.y > LINES-3 ||
	new_pos.x < 0 || new_pos.x > COLS-1 ||
	mvwinch(mw, new_pos.y, new_pos.x) == ' ') {
	msg("Nothing to affect.");
	return;
    }

    if ((item = find_mons(new_pos.y, new_pos.x)) == NULL) {
	debug("Affect what @ %d,%d?", new_pos.y, new_pos.x);
	return;
    }
    tp = THINGPTR(item);
    mname = monsters[tp->t_index].m_name;

    if (on(player, ISINVIS) && off(*tp, CANSEE)) {
	sprintf(outstring,"%s%s cannot see you", see ? "The " : "It",
	    see ? mname : "");
	msg(outstring);
	return;
    }

    if (off(*tp, TURNABLE) || on(*tp, WASTURNED)) 
	goto annoy;
    turn_off(*tp, TURNABLE);

    /* Can cleric kill it? */
    if (pstats.s_lvl >= 3 * tp->t_stats.s_lvl) {
	unsigned long test;	/* For overflow check */

	sprintf(outstring,"You have destroyed %s%s.", see ? "the " : "it", see ? mname : "");
	msg(outstring);
	test = pstats.s_exp + tp->t_stats.s_exp;

	/* Be sure there is no overflow before increasing experience */
	if (test > pstats.s_exp) pstats.s_exp = test;
	killed(item, FALSE, TRUE);
	check_level(TRUE);
	return;
    }

    /* Can cleric turn it? */
    if (rnd(100) + 1 >
	 (100 * ((2 * tp->t_stats.s_lvl) - pstats.s_lvl)) / pstats.s_lvl) {
	unsigned long test;	/* Overflow test */

	/* Make the monster flee */
	turn_on(*tp, WASTURNED);	/* No more fleeing after this */
	turn_on(*tp, ISFLEE);
	runto(tp, &hero);

	/* Let player know */
	sprintf(outstring,"You have turned %s%s.", see ? "the " : "it", see ? mname : "");
	msg(outstring);

	/* get points for turning monster -- but check overflow first */
	test = pstats.s_exp + tp->t_stats.s_exp/2;
	if (test > pstats.s_exp) pstats.s_exp = test;
	check_level(TRUE);

	/* If monster was suffocating, stop it */
	if (on(*tp, DIDSUFFOCATE)) {
	    turn_off(*tp, DIDSUFFOCATE);
	    extinguish(suffocate);
	}

	/* If monster held us, stop it */
	if (on(*tp, DIDHOLD) && (--hold_count == 0))
		turn_off(player, ISHELD);
	turn_off(*tp, DIDHOLD);
	return;
    }

    /* Otherwise -- no go */
annoy:
    sprintf(outstring,"You do not affect %s%s.", see ? "the " : "it", see ? mname : "");
    msg(outstring);

    /* Annoy monster */
   if (off(*tp, ISFLEE)) runto(tp, &hero);
}

/*
 * the magic user is going to try and cast a spell
 */
cast()
{
    register int i, num_spells, spell_ability;
    int  which_spell;
    bool nohw = FALSE;

    i = num_spells = spell_ability = which_spell = 0;

    if (player.t_ctype != C_MAGICIAN && pstats.s_intel < 16) {
	msg("You are not permitted to cast spells.");
	return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
	cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
	msg("You can't seem to cast a spell!");
	return;
    }
    num_spells = 0;

    /* Get the number of avilable spells */
    if (pstats.s_intel >= 16) 
	num_spells = pstats.s_intel - 15;

    if (player.t_ctype == C_MAGICIAN) 
	num_spells += pstats.s_lvl;

    if (num_spells > MAXSPELLS) 
	num_spells = MAXSPELLS;

    spell_ability = pstats.s_lvl * pstats.s_intel;
    if (player.t_ctype != C_MAGICIAN)
	spell_ability /= 2;

    /* Prompt for spells */
    msg("Which spell are you casting? (* for list): ");

    which_spell = (int) (readchar() - 'a');
    if (which_spell == (int) ESCAPE - (int) 'a') {
	mpos = 0;
	msg("");
	after = FALSE;
	return;
    }
    if (which_spell >= 0 && which_spell < num_spells) nohw = TRUE;

    else if (slow_invent) {
	register char c;

	for (i=0; i<num_spells; i++) {
	    msg("");
	    mvwaddch(cw, 0, 0, '[');
	    waddch(cw, (char) ((int) 'a' + i));
	    waddstr(cw, "] A spell of ");
	    if (magic_spells[i].s_type == TYP_POTION)
		waddstr(cw, p_magic[magic_spells[i].s_which].mi_name);
	    else if (magic_spells[i].s_type == TYP_SCROLL)
		waddstr(cw, s_magic[magic_spells[i].s_which].mi_name);
	    else if (magic_spells[i].s_type == TYP_STICK)
		waddstr(cw, ws_magic[magic_spells[i].s_which].mi_name);
	    waddstr(cw, morestr);
	    draw(cw);
	    do {
		c = readchar();
	    } while (c != ' ' && c != ESCAPE);
	    if (c == ESCAPE)
		break;
	}
	msg("");
	mvwaddstr(cw, 0, 0, "Which spell are you casting? ");
	draw(cw);
    }
    else {
	/* Set up for redraw */
	msg("");
	clearok(cw, TRUE);
	touchwin(cw);

	/* Now display the possible spells */
	wclear(hw);
	touchwin(hw);
	mvwaddstr(hw, 2, 0, "	Cost		Spell");
	mvwaddstr(hw, 3, 0, "-----------------------------------------------");
	for (i=0; i<num_spells; i++) {
	    mvwaddch(hw, i+4, 0, '[');
	    waddch(hw, (char) ((int) 'a' + i));
	    waddch(hw, ']');
	    sprintf(prbuf, "	%3d", magic_spells[i].s_cost);
	    waddstr(hw, prbuf);
	    waddstr(hw, "	A spell of ");
	    if (magic_spells[i].s_type == TYP_POTION)
		waddstr(hw, p_magic[magic_spells[i].s_which].mi_name);
	    else if (magic_spells[i].s_type == TYP_SCROLL)
		waddstr(hw, s_magic[magic_spells[i].s_which].mi_name);
	    else if (magic_spells[i].s_type == TYP_STICK)
		waddstr(hw, ws_magic[magic_spells[i].s_which].mi_name);
	}
	sprintf(prbuf,"[Current spell power = %d]",spell_ability - spell_power);
	mvwaddstr(hw, 0, 0, prbuf);
	waddstr(hw, " Which spell are you casting? ");
	draw(hw);
    }

    if (!nohw) {
	which_spell = (int) (wgetch(hw) - 'a');
	while (which_spell < 0 || which_spell >= num_spells) {
	    if (which_spell == (int) ESCAPE - (int) 'a') {
		after = FALSE;
		return;
	    }
	    wmove(hw, 0, 0);
	    wclrtoeol(hw);
	    waddstr(hw, "Please enter one of the listed spells. ");
	    draw(hw);
	    which_spell = (int) (wgetch(hw) - 'a');
	}
    }

    if ((spell_power + magic_spells[which_spell].s_cost) > spell_ability) {
	msg("Your attempt fails.");
	return;
    }
    if (nohw)
	msg("Your spell is successful.");
    else {
	mvwaddstr(hw, 0, 0, "Your spell is successful.--More--");
	wclrtoeol(hw);
	draw(hw);
	wait_for(hw,' ');
    }
    if (magic_spells[which_spell].s_type == TYP_POTION)
        quaff(	magic_spells[which_spell].s_which,
        	magic_spells[which_spell].s_flag,
		FALSE);
    else if (magic_spells[which_spell].s_type == TYP_SCROLL)
        read_scroll(	magic_spells[which_spell].s_which,
        		magic_spells[which_spell].s_flag,
			FALSE);
    else if (magic_spells[which_spell].s_type == TYP_STICK) {
	 if (!do_zap(	TRUE, 
			magic_spells[which_spell].s_which,
			magic_spells[which_spell].s_flag)) {
	     after = FALSE;
	     return;
	 }
    }
    spell_power += magic_spells[which_spell].s_cost;
}

/* Constitution bonus */

const_bonus()	/* Hit point adjustment for changing levels */
{
    if (pstats.s_const > 6 && pstats.s_const <= 14) 
	return(0);
    if (pstats.s_const > 14) 
	return(pstats.s_const-14);
    if (pstats.s_const > 3) 
	return(-1);
    return(-2);
}


/* Routines for thieves */

/*
 * gsense:
 *	Sense gold
 */

gsense()
{
    /* Only thieves can do this */
    if (player.t_ctype != C_THIEF) {
	msg("You seem to have no gold sense.");
	return;
    }

    if (lvl_obj != NULL) {
	struct linked_list *gitem;
	struct object *cur;
	int gtotal = 0;

	wclear(hw);
	for (gitem = lvl_obj; gitem != NULL; gitem = next(gitem)) {
	    cur = OBJPTR(gitem);
	    if (cur->o_type == GOLD) {
		gtotal += cur->o_count;
		mvwaddch(hw, cur->o_pos.y, cur->o_pos.x, GOLD);
	    }
	}
	if (gtotal) {
	    s_know[S_GFIND] = TRUE;
	    msg("You sense gold!");
	    overlay(hw,cw);
	    return;
	}
    }
    msg("You can sense no gold on this level.");
}

/* 
 * the cleric asks his deity for a spell
 */
pray()
{
    register int i, num_prayers, prayer_ability;
    int which_prayer;
    bool nohw = FALSE;

    which_prayer = num_prayers = prayer_ability = i = 0;

    if (player.t_ctype != C_CLERIC && pstats.s_wisdom < 17 &&
	cur_relic[HEIL_ANKH] == 0) {
	msg("You are not permitted to pray.");
	return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
	cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
	msg("You can't seem to pray!");
	return;
    }
    num_prayers = 0;

    /* Get the number of avilable prayers */
    if (pstats.s_wisdom > 16) 
	num_prayers = (pstats.s_wisdom - 15) / 2;

    if (player.t_ctype == C_CLERIC) 
	num_prayers += pstats.s_lvl;

    if (cur_relic[HEIL_ANKH]) num_prayers += 3;

    if (num_prayers > MAXPRAYERS) 
	num_prayers = MAXPRAYERS;

    prayer_ability = pstats.s_lvl * pstats.s_wisdom;
    if (player.t_ctype != C_CLERIC)
	prayer_ability /= 2;

    if (cur_relic[HEIL_ANKH]) prayer_ability *= 2;

    /* Prompt for prayer */
    msg("Which prayer are you offering? (* for list): ");
    which_prayer = (int) (readchar() - 'a');
    if (which_prayer == (int) ESCAPE - (int) 'a') {
	mpos = 0;
	msg("");
	after = FALSE;
	return;
    }
    if (which_prayer >= 0 && which_prayer < num_prayers) nohw = TRUE;

    else if (slow_invent) {
	register char c;

	for (i=0; i<num_prayers; i++) {
	    msg("");
	    mvwaddch(cw, 0, 0, '[');
	    waddch(cw, (char) ((int) 'a' + i));
	    waddstr(cw, "] A prayer for ");
	    if (cleric_spells[i].s_type == TYP_POTION)
		waddstr(cw, p_magic[cleric_spells[i].s_which].mi_name);
	    else if (cleric_spells[i].s_type == TYP_SCROLL)
		waddstr(cw, s_magic[cleric_spells[i].s_which].mi_name);
	    else if (cleric_spells[i].s_type == TYP_STICK)
		waddstr(cw, ws_magic[cleric_spells[i].s_which].mi_name);
	    waddstr(cw, morestr);
	    draw(cw);
	    do {
		c = readchar();
	    } while (c != ' ' && c != ESCAPE);
	    if (c == ESCAPE)
		break;
	}
	msg("");
	mvwaddstr(cw, 0, 0, "Which prayer are you offering? ");
	draw(cw);
    }
    else {
	/* Set up for redraw */
	msg("");
	clearok(cw, TRUE);
	touchwin(cw);

	/* Now display the possible prayers */
	wclear(hw);
	touchwin(hw);
	mvwaddstr(hw, 2, 0, "	Cost		Prayer");
	mvwaddstr(hw, 3, 0, "-----------------------------------------------");
	for (i=0; i<num_prayers; i++) {
	    mvwaddch(hw, i+4, 0, '[');
	    waddch(hw, (char) ((int) 'a' + i));
	    waddch(hw, ']');
	    sprintf(prbuf, "	%3d", cleric_spells[i].s_cost);
	    waddstr(hw, prbuf);
	    waddstr(hw, "	A prayer for ");
	    if (cleric_spells[i].s_type == TYP_POTION)
		waddstr(hw, p_magic[cleric_spells[i].s_which].mi_name);
	    else if (cleric_spells[i].s_type == TYP_SCROLL)
		waddstr(hw, s_magic[cleric_spells[i].s_which].mi_name);
	    else if (cleric_spells[i].s_type == TYP_STICK)
		waddstr(hw, ws_magic[cleric_spells[i].s_which].mi_name);
	}
	sprintf(prbuf,"[Current prayer ability = %d]",prayer_ability-pray_time);
	mvwaddstr(hw, 0, 0, prbuf);
	waddstr(hw, " Which prayer are you offering? ");
	draw(hw);
    }

    if (!nohw) {
	which_prayer = (int) (wgetch(hw) - 'a');
	while (which_prayer < 0 || which_prayer >= num_prayers) {
	    if (which_prayer == (int) ESCAPE - (int) 'a') {
		after = FALSE;
		return;
	    }
	    wmove(hw, 0, 0);
	    wclrtoeol(hw);
	    mvwaddstr(hw, 0, 0, "Please enter one of the listed prayers.");
	    draw(hw);
	    which_prayer = (int) (wgetch(hw) - 'a');
	}
    }


    if (cleric_spells[which_prayer].s_cost + pray_time > prayer_ability) {
	msg("Your prayer fails.");
	return;
    }

    if (nohw) 
	msg("Your prayer has been granted.");
    else {
	mvwaddstr(hw, 0, 0, "Your prayer has been granted.--More--");
	wclrtoeol(hw);
	draw(hw);
	wait_for(hw,' ');
    }
    if (cleric_spells[which_prayer].s_type == TYP_POTION)
	quaff(		cleric_spells[which_prayer].s_which,
			cleric_spells[which_prayer].s_flag,
			FALSE);
    else if (cleric_spells[which_prayer].s_type == TYP_SCROLL)
	read_scroll(	cleric_spells[which_prayer].s_which,
			cleric_spells[which_prayer].s_flag,
			FALSE);
    else if (cleric_spells[which_prayer].s_type == TYP_STICK) {
	 if (!do_zap(	TRUE, 
			cleric_spells[which_prayer].s_which,
			cleric_spells[which_prayer].s_flag)) {
	     after = FALSE;
	     return;
	 }
    }
    pray_time += cleric_spells[which_prayer].s_cost;
}



/*
 * steal:
 *	Steal in direction given in delta
 */

steal()
{
    register struct linked_list *item;
    register struct thing *tp;
    register const char *mname;
    coord new_pos;
    int thief_bonus = -50;
    bool isinvisible = FALSE;

    new_pos.y = hero.y + delta.y;
    new_pos.x = hero.x + delta.x;

    if (on(player, ISBLIND)) {
	msg("You can't see anything.");
	return;
    }

    /* Anything there? */
    if (new_pos.y < 0 || new_pos.y > LINES-3 ||
	new_pos.x < 0 || new_pos.x > COLS-1 ||
	mvwinch(mw, new_pos.y, new_pos.x) == ' ') {
	msg("Nothing to steal from.");
	return;
    }

    if ((item = find_mons(new_pos.y, new_pos.x)) == NULL)
	debug("Steal from what @ %d,%d?", new_pos.y, new_pos.x);
    tp = THINGPTR(item);
    if (isinvisible = invisible(tp)) mname = "creature";
    else mname = monsters[tp->t_index].m_name;

    /* Can player steal something unnoticed? */
    if (player.t_ctype == C_THIEF) thief_bonus = 10;
    if (on(*tp, ISUNIQUE)) thief_bonus -= 15;
    if (isinvisible) thief_bonus -= 20;
    if (on(*tp, ISINWALL) && off(player, CANINWALL)) thief_bonus -= 50;

    if (rnd(100) <
	(thief_bonus + 2*dex_compute() + 5*pstats.s_lvl -
	 5*(tp->t_stats.s_lvl - 3))) {
	register struct linked_list *s_item, *pack_ptr;
	int count = 0;
	unsigned long test;	/* Overflow check */

	s_item = NULL;	/* Start stolen goods out as nothing */

	/* Find a good item to take */
	for (pack_ptr=tp->t_pack; pack_ptr != NULL; pack_ptr=next(pack_ptr))
	    if ((OBJPTR(pack_ptr))->o_type != RELIC &&
		rnd(++count) == 0)
		s_item = pack_ptr;

	/* 
	 * Find anything?
	 *
	 * if we have a merchant, and his pack is empty then the
	 * rogue has already stolen once
	 */
	if (s_item == NULL) {
	    if (tp->t_index == NUMMONST)
		msg("The %s seems to be shielding his pack from you.", mname);
	    else
	        msg("The %s apparently has nothing to steal.", mname);
	    return;
	}

	/* Take it from monster */
	if (tp->t_pack) detach(tp->t_pack, s_item);

	/* Give it to player */
	if (add_pack(s_item, FALSE, NULL) == FALSE) {
	   (OBJPTR(s_item))->o_pos = hero;
	   fall(s_item, TRUE);
	}

	/* Get points for stealing -- but first check for overflow */
	test = pstats.s_exp + tp->t_stats.s_exp/2;
	if (test > pstats.s_exp) pstats.s_exp = test;

	/*
	 * Do adjustments if player went up a level
	 */
	check_level(TRUE);
    }

    else {
	msg("Your attempt fails.");

	/* Annoy monster (maybe) */
	if (rnd(35) >= dex_compute() + thief_bonus) {
	    if (tp->t_index == NUMMONST) {
		if (!isinvisible)
		    msg("The %s looks insulted and leaves", mname);
		killed(item, FALSE, FALSE);
	    }
	    else
	        runto(tp, &hero);
	}
    }
}
