/*
 * routines dealing specifically with rings
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
 * how much food does this ring use up?
 */
ring_eat(hand)
register int hand;
{
    if (cur_ring[hand] == NULL)
	return 0;
    switch (cur_ring[hand]->o_which) {
	case R_VAMPREGEN:
	    return 3;
	case R_REGEN:
	    return 2;
	case R_HEALTH:
	case R_SUSABILITY:
	    return 1;
	case R_SEARCH:
	case R_SEEINVIS:
	    return (rnd(100) < 33);
	case R_DIGEST:
	    if (cur_ring[hand]->o_ac >= 0)
		return (-(cur_ring[hand]->o_ac)-1);
	    else
		return (-(cur_ring[hand]->o_ac));
    }
    return 0;
}

ring_on(obj)
register struct object *obj;
{
    register int save_max;
    char buf[LINELEN];

    /*
     * Calculate the effect it has on the poor guy.
     */
    switch (obj->o_which)
    {
	case R_ADDSTR:
	    save_max = max_stats.s_str;
	    chg_str(obj->o_ac);
	    max_stats.s_str = save_max;
	when R_ADDHIT:
	    pstats.s_dext += obj->o_ac;
	when R_ADDINTEL:
	    pstats.s_intel += obj->o_ac;
	when R_ADDWISDOM:
	    pstats.s_wisdom += obj->o_ac;
	when R_SEEINVIS:
	    turn_on(player, CANSEE);
	    msg("Your eyes begin to tingle");
	    light(&hero);
	    mvwaddch(cw, hero.y, hero.x, PLAYER);
	when R_AGGR:
	    aggravate();
	when R_WARMTH:
	    turn_on(player, NOCOLD);
	when R_FIRE:
	    turn_on(player, NOFIRE);
	when R_LIGHT: {
		if(roomin(&hero) != NULL) {
			light(&hero);
			mvwaddch(cw, hero.y, hero.x, PLAYER);
		}
	    }
	when R_SEARCH:
		daemon(ring_search, 0, AFTER);
	when R_TELEPORT:
		daemon(ring_teleport, 0, AFTER);
    }
    status(FALSE);
    if (r_know[obj->o_which] && r_guess[obj->o_which])
    {
	free(r_guess[obj->o_which]);
	r_guess[obj->o_which] = NULL;
    }
    else if (!r_know[obj->o_which] && 
	     askme && 
	     (obj->o_flags & ISKNOW) == 0 &&
	     r_guess[obj->o_which] == NULL) {
	msg(terse ? "Call it: " : "What do you want to call it? ");
	if (get_str(buf, cw) == NORM)
	{
	    r_guess[obj->o_which] = new((unsigned int) strlen(buf) + 1);
	    strcpy(r_guess[obj->o_which], buf);
	}
	msg("");
    }
}

/*
 * print ring bonuses
 */
char *
ring_num(obj)
register struct object *obj;
{
    static char buf[5];

    if (!(obj->o_flags & ISKNOW))
	return "";
    switch (obj->o_which)
    {
	case R_PROTECT:
	case R_ADDSTR:
	case R_ADDDAM:
	case R_ADDHIT:
	case R_ADDINTEL:
	case R_ADDWISDOM:
	case R_DIGEST:
	    buf[0] = ' ';
	    strcpy(&buf[1], num(obj->o_ac, 0));
	when R_AGGR:
	case R_LIGHT:
	case R_HEAVY:
	case R_TELEPORT:
	    if (obj->o_flags & ISCURSED)
		return " cursed";
	    else
		return "";
	otherwise:
	    return "";
    }
    return buf;
}

/* 
 * Return the effect of the specified ring 
 */
ring_value(type)
{
    int result = 0;

    if (ISRING(LEFT_1, type))  result += cur_ring[LEFT_1]->o_ac;
    if (ISRING(LEFT_2, type)) result += cur_ring[LEFT_2]->o_ac;
    if (ISRING(LEFT_3, type)) result += cur_ring[LEFT_3]->o_ac;
    if (ISRING(LEFT_4, type)) result += cur_ring[LEFT_4]->o_ac;
    if (ISRING(RIGHT_1, type)) result += cur_ring[RIGHT_1]->o_ac;
    if (ISRING(RIGHT_2, type))  result += cur_ring[RIGHT_2]->o_ac;
    if (ISRING(RIGHT_3, type)) result += cur_ring[RIGHT_3]->o_ac;
    if (ISRING(RIGHT_4, type))  result += cur_ring[RIGHT_4]->o_ac;
    return(result);
}
