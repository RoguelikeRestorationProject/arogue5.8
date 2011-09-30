/*
 * All the daemon and fuse functions are in here
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

int between = 0;

/*
 * doctor:
 *	A healing daemon that restors hit points after rest
 */

doctor(tp)
register struct thing *tp;
{
    register int ohp;
    register int limit, new_points;
    register struct stats *curp; /* current stats pointer */
    register struct stats *maxp; /* max stats pointer */

    curp = &(tp->t_stats);
    maxp = &(tp->maxstats);
    if (curp->s_hpt == maxp->s_hpt) {
	tp->t_quiet = 0;
	return;
    }
    tp->t_quiet++;
    switch (tp->t_ctype) {
	case C_MAGICIAN:
	    limit = 8 - curp->s_lvl;
	    new_points = curp->s_lvl - 3;
	when C_THIEF:
	    limit = 8 - curp->s_lvl;
	    new_points = curp->s_lvl - 2;
	when C_CLERIC:
	    limit = 8 - curp->s_lvl;
	    new_points = curp->s_lvl - 3;
	when C_FIGHTER:
	    limit = 16 - curp->s_lvl*2;
	    new_points = curp->s_lvl - 5;
	when C_MONSTER:
	    limit = 16 - curp->s_lvl;
	    new_points = curp->s_lvl - 6;
	otherwise:
	    debug("what a strange character you are!");
	    return;
    }
    ohp = curp->s_hpt;
    if (off(*tp, HASDISEASE) && off(*tp, DOROT)) {
	if (curp->s_lvl < 8) {
	    if (tp->t_quiet > limit) {
		curp->s_hpt++;
		tp->t_quiet = 0;
	    }
	}
	else {
	    if (tp->t_quiet >= 3) {
		curp->s_hpt += rnd(new_points)+1;
		tp->t_quiet = 0;
	    }
	}
    }
    if (tp == &player) {
	if (ISRING(LEFT_1, R_REGEN)) curp->s_hpt++;
	if (ISRING(LEFT_2, R_REGEN)) curp->s_hpt++;
	if (ISRING(LEFT_3, R_REGEN)) curp->s_hpt++;
	if (ISRING(LEFT_4, R_REGEN)) curp->s_hpt++;
	if (ISRING(RIGHT_1, R_REGEN)) curp->s_hpt++;
	if (ISRING(RIGHT_2, R_REGEN)) curp->s_hpt++;
	if (ISRING(RIGHT_3, R_REGEN)) curp->s_hpt++;
	if (ISRING(RIGHT_4, R_REGEN)) curp->s_hpt++;
    }
    if (on(*tp, ISREGEN))
	curp->s_hpt += curp->s_lvl/10 + 1;
    if (ohp != curp->s_hpt) {
	if (curp->s_hpt >= maxp->s_hpt) {
	    curp->s_hpt = maxp->s_hpt;
	    if (off(*tp, WASTURNED) && on(*tp, ISFLEE) && tp != &player) {
		turn_off(*tp, ISFLEE);
		tp->t_oldpos = tp->t_pos;	/* Start our trek over */
	    }
	}
    }
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */

swander()
{
    daemon(rollwand, 0, BEFORE);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */

rollwand()
{
    if (++between >= 4)
    {
	/* Theives may not awaken a monster */
	if ((roll(1, 6) == 4) &&
	   ((player.t_ctype != C_THIEF) || (rnd(30) >= dex_compute()))) {
	    if (levtype != POSTLEV)
	        wanderer();
	    kill_daemon(rollwand);
	    fuse(swander, 0, WANDERTIME, BEFORE);
	}
	between = 0;
    }
}
/*
 * this function is a daemon called each turn when the character is a thief
 */
trap_look()
{
    if (rnd(100) < (2*dex_compute() + 5*pstats.s_lvl))
	search(TRUE, FALSE);
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */

unconfuse()
{
    turn_off(player, ISHUH);
    msg("You feel less confused now");
}


/*
 * unsee:
 *	He lost his see invisible power
 */

unsee()
{
    if (!ISWEARING(R_SEEINVIS)) {
	turn_off(player, CANSEE);
	msg("The tingling feeling leaves your eyes");
    }
}

/*
 * unstink:
 *	Remove to-hit handicap from player
 */

unstink()
{
    turn_off(player, HASSTINK);
}

/*
 * unclrhead:
 *	Player is no longer immune to confusion
 */

unclrhead()
{
    turn_off(player, ISCLEAR);
    msg("The blue aura about your head fades away.");
}

/*
 * unphase:
 *	Player can no longer walk through walls
 */

unphase()
{
    turn_off(player, CANINWALL);
    msg("Your dizzy feeling leaves you.");
    if (!step_ok(hero.y, hero.x, NOMONST, &player)) death(D_PETRIFY);
}

/*
 * land:
 *	Player can no longer fly
 */

land()
{
    turn_off(player, ISFLY);
    msg("You regain your normal weight");
    running = FALSE;
}

/*
 * sight:
 *	He gets his sight back
 */

sight()
{
    if (on(player, ISBLIND))
    {
	extinguish(sight);
	turn_off(player, ISBLIND);
	light(&hero);
	msg("The veil of darkness lifts");
    }
}

/*
 * res_strength:
 *	Restore player's strength
 */

res_strength()
{

    /* If lost_str is non-zero, restore that amount of strength,
     * else all of it 
     */
    if (lost_str) {
	chg_str(lost_str);
	lost_str = 0;
    }

    /* Otherwise, put player at the maximum strength */
    else {
	pstats.s_str = max_stats.s_str + ring_value(R_ADDSTR);
    }

    updpack(TRUE);
}

/*
 * nohaste:
 *	End the hasting
 */

nohaste()
{
    turn_off(player, ISHASTE);
    msg("You feel yourself slowing down.");
}

/*
 * noslow:
 *	End the slowing
 */

noslow()
{
    turn_off(player, ISSLOW);
    msg("You feel yourself speeding up.");
}

/*
 * suffocate:
 *	If this gets called, the player has suffocated
 */

suffocate()
{
    death(D_SUFFOCATION);
}

/*
 * digest the hero's food
 */
stomach()
{
    register int oldfood, old_hunger, food_use, i;

    old_hunger = hungry_state;
    if (food_left <= 0)
    {
	/*
	 * the hero is fainting
	 */
	if (no_command || rnd(100) > 20)
	    return;
	no_command = rnd(8)+4;
	if (!terse)
	    addmsg("You feel too weak from lack of food.  ");
	msg("You faint");
	running = FALSE;
	count = 0;
	hungry_state = F_FAINT;
    }
    else
    {
	oldfood = food_left;
	food_use = 0;
	for (i=0; i<MAXRELIC; i++) { /* each relic eats an additional food */
	    if (cur_relic[i])
		food_use++;
	}
	food_use +=    (ring_eat(LEFT_1)  + ring_eat(LEFT_2)  +
			ring_eat(LEFT_3)  + ring_eat(LEFT_4)  +
	    		ring_eat(RIGHT_1) + ring_eat(RIGHT_2) +
			ring_eat(RIGHT_3) + ring_eat(RIGHT_4) + 
			foodlev);
	if (food_use < 1)
	    food_use = 1;
	food_left -= food_use;
	if (food_left < MORETIME && oldfood >= MORETIME) {
	    msg("You are starting to feel weak");
	    running = FALSE;
	    hungry_state = F_WEAK;
	}
	else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
	{
	    msg(terse ? "Getting hungry" : "You are starting to get hungry");
	    running = FALSE;
	    hungry_state = F_HUNGRY;
	}

    }
    if (old_hunger != hungry_state) 
	updpack(TRUE);
    wghtchk();
}
/*
 * daemon for curing the diseased
 */
cure_disease()
{
    turn_off(player, HASDISEASE);
    if (off (player, HASINFEST))
	msg(terse ? "You feel yourself improving"
		: "You begin to feel yourself improving again");
}

/*
 * daemon for adding back dexterity
 */
un_itch()
{
    if (--lost_dext < 1) {
	lost_dext = 0;
	turn_off(player, HASITCH);
    }
    res_dexterity(1);
}
/*
 * appear:
 *	Become visible again
 */
appear()
{
    turn_off(player, ISINVIS);
    PLAYER = VPLAYER;
    msg("The tingling feeling leaves your body");
    light(&hero);
}
/*
 * dust_appear:
 *	dust of disappearance wears off
 */
dust_appear()
{
    turn_off(player, ISINVIS);
    PLAYER = VPLAYER;
    msg("You become visible again");
    light(&hero);
}
/*
 * unchoke:
 * 	the effects of "dust of choking and sneezing" wear off
 */
unchoke()
{
    if (!find_slot(unconfuse))
	turn_off(player, ISHUH);
    if (!find_slot(sight))
	turn_off(player, ISBLIND);
    light(&hero);
    msg("Your throat and eyes return to normal");
}
/*
 * make some potion for the guy in the Alchemy jug
 */
alchemy(obj)
register struct object *obj;
{
    register struct object *tobj = NULL;
    register struct linked_list *item;

    /*
     * verify that the object pointer we have still points to an alchemy
     * jug (hopefully the right one!) because the hero could have thrown
     * it away
     */
    for (item = pack; item != NULL; item = next(item)) {
	tobj = OBJPTR(item);
	if (tobj	 == obj		&& 
	    tobj->o_type == MM		&& 
	    tobj->o_which== MM_JUG	&&
	    tobj->o_ac   == JUG_EMPTY	)
		break;
    }
    if (item == NULL) { 	/* not in the pack, check the level */
	for (item = lvl_obj; item != NULL; item = next(item)) {
	    tobj = OBJPTR(item);
	    if (tobj	     == obj		&& 
		tobj->o_type == MM		&& 
		tobj->o_which== MM_JUG		&&
		tobj->o_ac   == JUG_EMPTY	)
		    break;
	}
    }
    if (item == NULL)	/* can't find it.....too bad */
	return;
    
    switch(rnd(9)) {
	case 0: tobj->o_ac = P_PHASE;
	when 1: tobj->o_ac = P_CLEAR;
	when 2: tobj->o_ac = P_SEEINVIS;
	when 3: tobj->o_ac = P_HEALING;
	when 4: tobj->o_ac = P_MFIND;
	when 5: tobj->o_ac = P_TFIND;
	when 6: tobj->o_ac = P_HASTE;
	when 7: tobj->o_ac = P_RESTORE;
	when 8: tobj->o_ac = P_FLY;
    }
}
/*
 * otto's irresistable dance wears off 
 */

undance()
{
    turn_off(player, ISDANCE);
    msg ("Your feet take a break.....whew!");
}

/* 
 * if he has our favorite necklace of strangulation then take damage every turn
 */
strangle()
{
     if ((pstats.s_hpt -= 6) <= 0) death(D_STRANGLE);
}
/*
 * if he has on the gauntlets of fumbling he might drop his weapon each turn
 */
fumble()
{
    register struct linked_list *item;

    if(cur_weapon!=NULL && cur_weapon->o_type!=RELIC && rnd(100)<3) {
	for (item = pack; item != NULL; item = next(item)) {
	    if (OBJPTR(item) == cur_weapon)
		break;
	}
	if (item != NULL) {
	    drop(item);
	    running = FALSE;
	}
    }
}
/*
 * this is called each turn the hero has the ring of searching on
 */
ring_search()
{
    search(FALSE, FALSE);
}
/*
 * this is called each turn the hero has the ring of teleportation on
 */
ring_teleport()
{
    if (rnd(100) < 2) teleport();
}
