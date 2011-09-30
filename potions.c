/*
 * Function(s) for dealing with potions
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



/*
 * Increase player's constitution
 */

add_const(cursed)
bool cursed;
{
    /* Do the potion */
    if (cursed) {
	msg("You feel less healthy now.");
	pstats.s_const--;
	if (pstats.s_const <= 0)
	    death(D_CONSTITUTION);
    }
    else {
	msg("You feel healthier now.");
	pstats.s_const = min(pstats.s_const + 1, 25);
    }

    /* Adjust the maximum */
    if (max_stats.s_const < pstats.s_const)
	max_stats.s_const = pstats.s_const;
}

/*
 * Increase player's dexterity
 */

add_dexterity(cursed)
bool cursed;
{
    int ring_str;	/* Value of ring strengths */

    /* Undo any ring changes */
    ring_str = ring_value(R_ADDHIT);
    pstats.s_dext -= ring_str;

    /* Now do the potion */
    if (cursed) {
	msg("You feel less dextrous now.");
	pstats.s_dext--;
    }
    else {
	msg("You feel more dextrous now.  Watch those hands!");
	pstats.s_dext = min(pstats.s_dext + 1, 25);
    }

    /* Adjust the maximum */
    if (max_stats.s_dext < pstats.s_dext)
	max_stats.s_dext = pstats.s_dext;

    /* Now put back the ring changes */
    if (ring_str)
	pstats.s_dext += ring_str;
}

/*
 * add_haste:
 *	add a haste to the player
 */

add_haste(blessed)
bool blessed;
{
    int hasttime;

    if (blessed) hasttime = HASTETIME*2;
    else hasttime = HASTETIME;

    if (on(player, ISSLOW)) { /* Is person slow? */
	extinguish(noslow);
	noslow();

	if (blessed) hasttime = HASTETIME/2;
	else return;
    }

    if (on(player, ISHASTE)) {
	msg("You faint from exhaustion.");
	no_command += rnd(hasttime);
	lengthen(nohaste, roll(hasttime,hasttime));
    }
    else {
	turn_on(player, ISHASTE);
	fuse(nohaste, 0, roll(hasttime, hasttime), AFTER);
    }
}

/*
 * Increase player's intelligence
 */
add_intelligence(cursed)
bool cursed;
{
    int ring_str;	/* Value of ring strengths */

    /* Undo any ring changes */
    ring_str = ring_value(R_ADDINTEL);
    pstats.s_intel -= ring_str;

    /* Now do the potion */
    if (cursed) {
	msg("You feel slightly less intelligent now.");
	pstats.s_intel--;
    }
    else {
	msg("You feel more intelligent now.  What a mind!");
	pstats.s_intel = min(pstats.s_intel + 1, 25);
    }

    /* Adjust the maximum */
    if (max_stats.s_intel < pstats.s_intel)
	    max_stats.s_intel = pstats.s_intel;

    /* Now put back the ring changes */
    if (ring_str)
	pstats.s_intel += ring_str;
}

/*
 * this routine makes the hero move slower 
 */
add_slow()
{
    if (on(player, ISHASTE)) { /* Already sped up */
	extinguish(nohaste);
	nohaste();
    }
    else {
	msg("You feel yourself moving %sslower.",
		on(player, ISSLOW) ? "even " : "");
	if (on(player, ISSLOW))
	    lengthen(noslow, roll(HASTETIME,HASTETIME));
	else {
	    turn_on(player, ISSLOW);
	    player.t_turn = TRUE;
	    fuse(noslow, 0, roll(HASTETIME,HASTETIME), AFTER);
	}
    }
}

/*
 * Increase player's strength
 */

add_strength(cursed)
bool cursed;
{

    if (cursed) {
	msg("You feel slightly weaker now.");
	chg_str(-1);
    }
    else {
	msg("You feel stronger now.  What bulging muscles!");
	chg_str(1);
    }
}

/*
 * Increase player's wisdom
 */

add_wisdom(cursed)
bool cursed;
{
    int ring_str;	/* Value of ring strengths */

    /* Undo any ring changes */
    ring_str = ring_value(R_ADDWISDOM);
    pstats.s_wisdom -= ring_str;

    /* Now do the potion */
    if (cursed) {
	msg("You feel slightly less wise now.");
	pstats.s_wisdom--;
    }
    else {
	msg("You feel wiser now.  What a sage!");
	pstats.s_wisdom = min(pstats.s_wisdom + 1, 25);
    }

    /* Adjust the maximum */
    if (max_stats.s_wisdom < pstats.s_wisdom)
	max_stats.s_wisdom = pstats.s_wisdom;

    /* Now put back the ring changes */
    if (ring_str)
	pstats.s_wisdom += ring_str;
}


/* 
 * Lower a level of experience 
 */

lower_level(who)
short who;
{
    int fewer, nsides = 0;

    if (--pstats.s_lvl == 0)
	death(who);		/* All levels gone */
    msg("You suddenly feel less skillful.");
    pstats.s_exp /= 2;
    switch (player.t_ctype) {
	case C_FIGHTER:	nsides = HIT_FIGHTER;
	when C_MAGICIAN:nsides = HIT_MAGICIAN;
	when C_CLERIC:	nsides = HIT_CLERIC;
	when C_THIEF:	nsides = HIT_THIEF;
    }
    fewer = max(1, roll(1,nsides) + const_bonus());
    pstats.s_hpt -= fewer;
    max_stats.s_hpt -= fewer;
    if (pstats.s_hpt < 1)
	pstats.s_hpt = 1;
    if (max_stats.s_hpt < 1)
	death(who);
}

quaff(which, flag, is_potion)
int which;
int flag;
bool is_potion;
{
    register struct object *obj = NULL;
    register struct linked_list *item, *titem;
    register struct thing *th;
    bool cursed, blessed;
    char buf[LINELEN];

    blessed = FALSE;
    cursed = FALSE;
    item = NULL;

    if (which < 0) {	/* figure out which ourselves */
	item = get_item(pack, "quaff", POTION);
	/*
	 * Make certain that it is somethings that we want to drink
	 */
	if (item == NULL)
	    return;

	obj = OBJPTR(item);
	/* remove it from the pack */
	inpack--;
	detach(pack, item);

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	cursed = (obj->o_flags & ISCURSED) != 0;
	blessed = (obj->o_flags & ISBLESSED) != 0;

	which = obj->o_which;
    }
    else {
	cursed = flag & ISCURSED;
	blessed = flag & ISBLESSED;
    }

    switch(which)
    {
	case P_CLEAR:
	    if (cursed) {
		if (off(player, ISCLEAR))
		{
		    msg("Wait, what's going on here. Huh? What? Who?");
		    if (find_slot(unconfuse))
			lengthen(unconfuse, rnd(8)+HUHDURATION);
		    else
			fuse(unconfuse, 0, rnd(8)+HUHDURATION, AFTER);
		    turn_on(player, ISHUH);
		}
		else msg("You feel dizzy for a moment, but it quickly passes.");
	    }
	    else {
		if (blessed) {	/* Make player immune for the whole game */
		    extinguish(unclrhead);  /* If we have a fuse, put it out */
		    msg("A strong blue aura surrounds your head.");
		}
		else {	/* Just light a fuse for how long player is safe */
		    if (off(player, ISCLEAR)) {
			fuse(unclrhead, 0, CLRDURATION, AFTER);
			msg("A faint blue aura surrounds your head.");
		    }
		    else {  /* If we have a fuse lengthen it, else we
			     * are permanently clear.
			     */
		        if (find_slot(unclrhead) == FALSE)
			    msg("Your blue aura continues to glow strongly.");
			else {
			    lengthen(unclrhead, CLRDURATION);
			    msg("Your blue aura brightens for a moment.");
			}
		    }
		}
		turn_on(player, ISCLEAR);
		/* If player is confused, unconfuse him */
		if (on(player, ISHUH)) {
		    extinguish(unconfuse);
		    unconfuse();
		}
	    }
	when P_HEALING:
	    if (cursed) {
		if (!save(VS_POISON, &player, 0)) {
		    msg("You feel very sick now.");
		    pstats.s_hpt /= 2;
		    pstats.s_const--;
		    if (pstats.s_const <= 0 || pstats.s_hpt <= 0) 
			death(D_POISON);
		}
		else msg("You feel momentarily sick.");
	    }
	    else {
		if (blessed) {
		    if ((pstats.s_hpt += roll(pstats.s_lvl, 8)) > max_stats.s_hpt)
			pstats.s_hpt = ++max_stats.s_hpt;
		    if (on(player, ISHUH)) {
			extinguish(unconfuse);
			unconfuse();
		    }
		}
		else {
		    if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_stats.s_hpt)
		    pstats.s_hpt = ++max_stats.s_hpt;
		}
		msg("You begin to feel %sbetter.",
			blessed ? "much " : "");
		sight();
		if (is_potion) p_know[P_HEALING] = TRUE;
	    }
	when P_ABIL:
	    {
		int ctype;

		/*
		 * if blessed then fix all attributes
		 */
		if (blessed) {
		    add_intelligence(FALSE);
		    add_dexterity(FALSE);
		    add_strength(FALSE);
		    add_wisdom(FALSE);
		    add_const(FALSE);
		}
		/* probably will be own ability */
		else {
		    if (rnd(100) < 70) 
			ctype = player.t_ctype;
		    else do {
			ctype = rnd(4);
			} while (ctype == player.t_ctype);

		    /* Small chance of doing constitution instead */
		    if (rnd(100) < 10) 
			add_const(cursed);
		    else switch (ctype) {
			case C_FIGHTER: add_strength(cursed);
			when C_MAGICIAN: add_intelligence(cursed);
			when C_CLERIC:	add_wisdom(cursed);
			when C_THIEF:	add_dexterity(cursed);
			otherwise:	msg("You're a strange type!");
		    }
		}
		if (is_potion) p_know[P_ABIL] = TRUE;
	    }
	when P_MFIND:
	    /*
	     * Potion of monster detection, if there are monters, detect them
	     */
	    if (mlist != NULL)
	    {
		msg("You begin to sense the presence of monsters.");
		waddstr(cw, morestr);
		overlay(mw, cw);
		draw(cw);
		wait_for(cw,' ');
		msg("");
		if (is_potion) p_know[P_MFIND] = TRUE;
	    }
	    else
		msg("You have a strange feeling for a moment, then it passes.");
	when P_TFIND:
	    /*
	     * Potion of magic detection.  Show the potions and scrolls
	     */
	    if (lvl_obj != NULL)
	    {
		register struct linked_list *mobj;
		register struct object *tp;
		bool show;

		show = FALSE;
		wclear(hw);
		for (mobj = lvl_obj; mobj != NULL; mobj = next(mobj)) {
		    tp = OBJPTR(mobj);
		    if (is_magic(tp)) {
			char mag_type=MAGIC;

			/* Mark cursed items or bad weapons */
			if ((tp->o_flags & ISCURSED) ||
			    (tp->o_type == WEAPON &&
			     (tp->o_hplus < 0 || tp->o_dplus < 0)))
				mag_type = CMAGIC;
			else if ((tp->o_flags & ISBLESSED) ||
				 (tp->o_type == WEAPON &&
				  (tp->o_hplus > 0 || tp->o_dplus > 0)))
					mag_type = BMAGIC;
			show = TRUE;
			mvwaddch(hw, tp->o_pos.y, tp->o_pos.x, mag_type);
		    }
		    if (is_potion) p_know[P_TFIND] = TRUE;
		}
		for (titem = mlist; titem != NULL; titem = next(titem)) {
		    register struct linked_list *pitem;

		    th = THINGPTR(titem);
		    for(pitem = th->t_pack; pitem != NULL; pitem = next(pitem)){
			tp = OBJPTR(pitem);
			if (is_magic(tp)) {
			    char mag_type=MAGIC;

			    /* Mark cursed items or bad weapons */
			    if ((tp->o_flags & ISCURSED) ||
				(tp->o_type == WEAPON &&
				 (tp->o_hplus < 0 || tp->o_dplus < 0)))
				    mag_type = CMAGIC;
			    else if ((tp->o_flags & ISBLESSED) ||
				     (tp->o_type == WEAPON &&
				      (tp->o_hplus > 0 || tp->o_dplus > 0)))
					    mag_type = BMAGIC;
			    show = TRUE;
			    mvwaddch(hw, th->t_pos.y, th->t_pos.x, mag_type);
			}
			if (is_potion) p_know[P_TFIND] = TRUE;
		    }
		}
		if (show) {
		    msg("You sense the presence of magic on this level.");
		    waddstr(cw, morestr);
		    overlay(hw,cw);
		    draw(cw);
		    wait_for(cw,' ');
		    msg("");
		    break;
		}
	    }
	    msg("You have a strange feeling for a moment, then it passes.");
	when P_SEEINVIS:
	    if (cursed) {
		if (!find_slot(sight))
		{
		    msg("A cloak of darkness falls around you.");
		    turn_on(player, ISBLIND);
		    fuse(sight, 0, SEEDURATION, AFTER);
		    light(&hero);
		}
		else
		    lengthen(sight, SEEDURATION);
	    }
	    else {
		if (off(player, CANSEE)) {
		    turn_on(player, CANSEE);
		    msg("Your eyes begin to tingle.");
		    fuse(unsee, 0, blessed ? SEEDURATION*3 :SEEDURATION, AFTER);
		    light(&hero);
		}
		else if (find_slot(unsee) != FALSE)
		    lengthen(unsee, blessed ? SEEDURATION*3 : SEEDURATION);
		sight();
	    }
	when P_PHASE:
	    if (cursed) {
		msg("You can't move.");
		no_command = FREEZETIME;
	    }
	    else {
		int duration;

		if (blessed) duration = 3;
		else duration = 1;

		if (on(player, CANINWALL))
		    lengthen(unphase, duration*PHASEDURATION);
		else {
		    fuse(unphase, 0, duration*PHASEDURATION, AFTER);
		    turn_on(player, CANINWALL);
		}
		msg("You feel %slight-headed!",
		    blessed ? "very " : "");
	    }
	when P_FLY: {
	    int duration;
	    bool say_message;

	    say_message = TRUE;

	    if (blessed) duration = 3;
	    else duration = 1;

	    if (on(player, ISFLY)) {
		if (find_slot(land))
		    lengthen(land, duration*FLYTIME);
		else {
		    msg("Nothing happens.");	/* Flying by cloak */
		    say_message = FALSE;
		}
	    }
	    else {
		fuse(land, 0, duration*FLYTIME, AFTER);
		turn_on(player, ISFLY);
	    }
	    if (say_message) 
		msg("You feel %slighter than air!", blessed ? "much " : "");
	}
	when P_RAISE:
	    if (cursed) lower_level(D_POTION);
	    else {
		msg("You suddenly feel %smore skillful",
			blessed ? "much " : "");
		p_know[P_RAISE] = TRUE;
		raise_level(TRUE);
		if (blessed) raise_level(TRUE);
	    }
	when P_HASTE:
	    if (cursed) {	/* Slow player down */
		add_slow();
	    }
	    else {
		if (off(player, ISSLOW))
		    msg("You feel yourself moving %sfaster.",
			blessed ? "much " : "");
		add_haste(blessed);
		if (is_potion) p_know[P_HASTE] = TRUE;
	    }
	when P_RESTORE: {
	    register int i;

	    msg("Hey, this tastes great.  It make you feel warm all over.");
	    if (lost_str) {
		extinguish(res_strength);
		lost_str = 0;
	    }
	    for (i=0; i<lost_dext; i++)
		extinguish(un_itch);
	    lost_dext = 0;
	    res_strength();
	    res_wisdom();
	    res_dexterity(0);
	    res_intelligence();
	    pstats.s_const = max_stats.s_const;
	}
	when P_INVIS:
	    if (off(player, ISINVIS)) {
		turn_on(player, ISINVIS);
		msg("You have a tingling feeling all over your body");
		fuse(appear, 0, blessed ? GONETIME*3 : GONETIME, AFTER);
		PLAYER = IPLAYER;
		light(&hero);
	    }
	    else {
		if (find_slot(appear)) {
		    msg("Your tingling feeling surges.");
		    lengthen(appear, blessed ? GONETIME*3 : GONETIME);
		}
		else msg("Nothing happens.");	/* Using cloak */
	    }

	otherwise:
	    msg("What an odd tasting potion!");
	    return;
    }
    status(FALSE);
    if (is_potion && p_know[which] && p_guess[which])
    {
	free(p_guess[which]);
	p_guess[which] = NULL;
    }
    else if (is_potion && 
	     !p_know[which] && 
	     askme &&
	     (obj->o_flags & ISKNOW) == 0 &&
	     (obj->o_flags & ISPOST) == 0 &&
	     p_guess[which] == NULL)
    {
	msg(terse ? "Call it: " : "What do you want to call it? ");
	if (get_str(buf, cw) == NORM)
	{
	    p_guess[which] = new((unsigned int) strlen(buf) + 1);
	    strcpy(p_guess[which], buf);
	}
    }
    if (item != NULL) o_discard(item);
    updpack(TRUE);
}


/*
 * res_dexterity:
 *	Restore player's dexterity
 *	if called with zero the restore fully
 */

res_dexterity(howmuch)
int howmuch;
{
    short save_max;
    int ring_str;

    /* Discount the ring value */
    ring_str = ring_value(R_ADDHIT);
    pstats.s_dext -= ring_str;

    if (pstats.s_dext < max_stats.s_dext ) {
	if (howmuch == 0)
	    pstats.s_dext = max_stats.s_dext;
	else
	    pstats.s_dext = min(pstats.s_dext+howmuch, max_stats.s_dext);
    }

    /* Redo the rings */
    if (ring_str) {
	save_max = max_stats.s_dext;
	pstats.s_dext += ring_str;
	max_stats.s_dext = save_max;
    }
}


/*
 * res_intelligence:
 *	Restore player's intelligence
 */

res_intelligence()
{
    short save_max;
    int ring_str;

    /* Discount the ring value */
    ring_str = ring_value(R_ADDINTEL);
    pstats.s_intel -= ring_str;

    if (pstats.s_intel < max_stats.s_intel ) pstats.s_intel = max_stats.s_intel;

    /* Redo the rings */
    if (ring_str) {
	save_max = max_stats.s_intel;
	pstats.s_intel += ring_str;
	max_stats.s_intel = save_max;
    }
}

/*
 * res_wisdom:
 *	Restore player's wisdom
 */

res_wisdom()
{
    short save_max;
    int ring_str;

    /* Discount the ring value */
    ring_str = ring_value(R_ADDWISDOM);
    pstats.s_wisdom -= ring_str;

    if (pstats.s_wisdom < max_stats.s_wisdom )
	pstats.s_wisdom = max_stats.s_wisdom;

    /* Redo the rings */
    if (ring_str) {
	save_max = max_stats.s_wisdom;
	pstats.s_wisdom += ring_str;
	max_stats.s_wisdom = save_max;
    }
}
