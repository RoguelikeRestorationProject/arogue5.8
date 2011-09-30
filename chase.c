/*
 * Code for one object to chase another
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

#include <ctype.h>
#include <limits.h>
#include "curses.h"
#include "rogue.h"
#define	MAXINT	INT_MAX
#define	MININT	INT_MIN

coord ch_ret;				/* Where chasing takes you */





/*
 * Canblink checks if the monster can teleport (blink).  If so, it will
 * try to blink the monster next to the player.
 */

bool
can_blink(tp)
register struct thing *tp;
{
    register int y, x, index=9;
    coord tryp;	/* To hold the coordinates for use in diag_ok */
    bool spots[9], found_one=FALSE;

    /*
     * First, can the monster even blink?  And if so, there is only a 50%
     * chance that it will do so.  And it won't blink if it is running or
     * held.
     */
    if (off(*tp, CANBLINK) || (on(*tp, ISHELD)) ||
	on(*tp, ISFLEE) ||
	(on(*tp, ISSLOW) && off(*tp, ISHASTE) && !(tp->t_turn)) ||
	tp->t_no_move ||
	(rnd(12) < 6)) return(FALSE);


    /* Initialize the spots as illegal */
    do {
	spots[--index] = FALSE;
    } while (index > 0);

    /* Find a suitable spot next to the player */
    for (y=hero.y-1; y<hero.y+2; y++)
	for (x=hero.x-1; x<hero.x+2; x++, index++) {
	    /* Make sure x coordinate is in range and that we are
	     * not at the player's position
	     */
	    if (x<0 || x >= COLS || index == 4) continue;

	    /* Is it OK to move there? */
	    if (step_ok(y, x, NOMONST, tp) &&
		(!isatrap(mvwinch(cw, y, x)) ||
		  rnd(10) >= tp->t_stats.s_intel ||
		  on(*tp, ISFLY))) {
		/* OK, we can go here.  But don't go there if
		 * monster can't get at player from there
		 */
		tryp.y = y;
		tryp.x = x;
		if (diag_ok(&tryp, &hero, tp)) {
		    spots[index] = TRUE;
		    found_one = TRUE;
		}
	    }
	}

    /* If we found one, go to it */
    if (found_one) {
	char rch;	/* What's really where the creatures moves to */

	/* Find a legal spot */
	while (spots[index=rnd(9)] == FALSE) continue;

	/* Get the coordinates */
	y = hero.y + (index/3) - 1;
	x = hero.x + (index % 3) - 1;

	/* Move the monster from the old space */
	mvwaddch(cw, tp->t_pos.y, tp->t_pos.x, tp->t_oldch);

	/* Move it to the new space */
	tp->t_oldch = CCHAR( mvwinch(cw, y, x) );

	/* Display the creature if our hero can see it */
	if (cansee(y, x) &&
	    off(*tp, ISINWALL) &&
	    !invisible(tp))
	    mvwaddch(cw, y, x, tp->t_type);

	/* Fix the monster window */
	mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, ' '); /* Clear old position */
	mvwaddch(mw, y, x, tp->t_type);

	/* Record the new position */
	tp->t_pos.y = y;
	tp->t_pos.x = x;

	/* If the monster is on a trap, trap it */
	rch = CCHAR( mvinch(y, x) );
	if (isatrap(rch)) {
	    if (cansee(y, x)) tp->t_oldch = rch;
	    be_trapped(tp, &(tp->t_pos));
	}
    }

    return(found_one);
}

/* 
 * Can_shoot determines if the monster (er) has a direct line of shot 
 * at the player (ee).  If so, it returns the direction in which to shoot.
 */

coord *
can_shoot(er, ee)
register coord *er, *ee;
{
    static coord shoot_dir;

    /* Make sure we are chasing the player */
    if (!ce((*ee), hero)) return(NULL);

    /* 
     * They must be in the same room or very close (at door)
     */
    if (roomin(er) != roomin(&hero) && DISTANCE(er->y,er->x,ee->y,ee->x) > 1)
	return(NULL);

    /* Do we have a straight shot? */
    if (!straight_shot(er->y, er->x, ee->y, ee->x, &shoot_dir)) return(NULL);
    else return(&shoot_dir);
}

/*
 * chase:
 *	Find the spot for the chaser(er) to move closer to the
 *	chasee(ee).  Returns TRUE if we want to keep on chasing later
 *	FALSE if we reach the goal.
 */

chase(tp, ee, flee, mdead)
register struct thing *tp;
register coord *ee;
bool flee; /* True if destination (ee) is player and monster is running away
	    * or the player is in a wall and the monster can't get to it
	    */
bool *mdead;
{
    int damage, dist, thisdist, monst_dist = MAXINT; 
    struct linked_list *weapon;
    register coord *er = &tp->t_pos; 
    coord *shoot_dir;
    char ch, mch;
    bool next_player = FALSE;

    if (mdead != NULL)
	*mdead = 0;

    /* 
     * set the distance from the chas(er) to the chas(ee) here and then
     * we won't have to reset it unless the chas(er) moves (instead of shoots)
     */
    dist = DISTANCE(er->y, er->x, ee->y, ee->x);

    /*
     * If the thing is confused or it can't see the player,
     * let it move randomly. 
     */
    if ((on(*tp, ISHUH) && rnd(10) < 8) ||
	(on(player, ISINVIS) && off(*tp, CANSEE))) { /* Player is invisible */
	/*
	 * get a valid random move
	 */
	ch_ret = *rndmove(tp);
	dist = DISTANCE(ch_ret.y, ch_ret.x, ee->y, ee->x);
	/*
	 * check to see if random move takes creature away from player
	 * if it does then turn off ISHELD
	 */
	if (dist > 2) {
	    if (on(*tp, DIDHOLD)) {
		 turn_off(*tp, DIDHOLD);
		 turn_on(*tp, CANHOLD);
		 if (--hold_count == 0) 
		     turn_off(player, ISHELD);
	    }

	    /* If monster was suffocating, stop it */
	    if (on(*tp, DIDSUFFOCATE)) {
		turn_off(*tp, DIDSUFFOCATE);
		turn_on(*tp, CANSUFFOCATE);
		extinguish(suffocate);
	    }
	}
    }

    /* If we can breathe, we may do so */
    else if (on(*tp, CANBREATHE)		&&
	     (dist < BOLT_LENGTH*BOLT_LENGTH)	&&
	     (shoot_dir = can_shoot(er, ee))	&&
	     !on(player, ISINWALL)		&&
	     (rnd(100) < 75)) {
		register char *breath = NULL;

		damage = tp->t_stats.s_hpt;
		/* Will it breathe at random */
		if (on(*tp, CANBRANDOM)) {
		    /* Turn off random breath */
		    turn_off(*tp, CANBRANDOM);

		    /* Select type of breath */
		    switch (rnd(10)) {
		    case 0: breath = "acid";
			    turn_on(*tp, NOACID);
		    when 1: breath = "flame";
			    turn_on(*tp, NOFIRE);
		    when 2: breath = "lightning bolt";
			    turn_on(*tp, NOBOLT);
		    when 3: breath = "chlorine gas";
			    turn_on(*tp, NOGAS);
		    when 4: breath = "ice";
			    turn_on(*tp, NOCOLD);
		    when 5: breath = "nerve gas";
			    turn_on(*tp, NOPARALYZE);
		    when 6: breath = "sleeping gas";
			    turn_on(*tp, NOSLEEP);
		    when 7: breath = "slow gas";
			    turn_on(*tp, NOSLOW);
		    when 8: breath = "confusion gas";
			    turn_on(*tp, ISCLEAR);
		    when 9: breath = "fear gas";
			    turn_on(*tp, NOFEAR);
		    }
		}

		/* Or can it breathe acid? */
		else if (on(*tp, CANBACID)) {
		    turn_off(*tp, CANBACID);
		    breath = "acid";
		}

		/* Or can it breathe fire */
		else if (on(*tp, CANBFIRE)) {
		    turn_off(*tp, CANBFIRE);
		    breath = "flame";
		}

		/* Or can it breathe electricity? */
		else if (on(*tp, CANBBOLT)) {
		    turn_off(*tp, CANBBOLT);
		    breath = "lightning bolt";
		}

		/* Or can it breathe gas? */
		else if (on(*tp, CANBGAS)) {
		    turn_off(*tp, CANBGAS);
		    breath = "chlorine gas";
		}

		/* Or can it breathe ice? */
		else if (on(*tp, CANBICE)) {
		    turn_off(*tp, CANBICE);
		    breath = "ice";
		}

		else if (on(*tp, CANBPGAS)) {
		    turn_off(*tp, CANBPGAS);
		    breath = "nerve gas";
		}

		/* can it breathe sleeping gas */
		else if (on(*tp, CANBSGAS)) {
		    turn_off(*tp, CANBSGAS);
		    breath = "sleeping gas";
		}

		/* can it breathe slow gas */
		else if (on(*tp, CANBSLGAS)) {
		    turn_off(*tp, CANBSLGAS);
		    breath = "slow gas";
		}
		/* can it breathe confusion gas */
		else if (on(*tp, CANBCGAS)) {
		    turn_off(*tp, CANBCGAS);
		    breath = "confusion gas";
		}
		/* can it breathe fear gas */
		else {
		    turn_off(*tp, CANBFGAS);
		    breath = "fear gas";
		}

		/* Now breathe -- sets "monst_dead" if it kills someone */
		*mdead = shoot_bolt(	tp, *er, *shoot_dir, FALSE, 
				tp->t_index, breath, damage);

		ch_ret = *er;
		running = FALSE;
		if (*mdead) return(TRUE);
    }

    /* We may shoot missiles if we can */
    else if (on(*tp, CANMISSILE) 		&& 
	    (shoot_dir = can_shoot(er, ee))	&&
	    !on(player, ISINWALL)		&& 
	    (rnd(100) < 75)) {
	    static struct object missile =
	    {
		MISSILE, {0, 0}, "", 0, "", "0d4 " , NULL, 0, WS_MISSILE, 100, 1
	    };

	    sprintf(missile.o_hurldmg, "%dd4", tp->t_stats.s_lvl);
	    do_motion(&missile, shoot_dir->y, shoot_dir->x, tp);
	    hit_monster(unc(missile.o_pos), &missile, tp);
	    turn_off(*tp, CANMISSILE);
	    ch_ret = *er;
	    running = FALSE;
    }

    /* We may use a sonic blast if we can */
    else if (on(*tp, CANSONIC)			&& 
	    (dist < BOLT_LENGTH*2)		&&
	    (shoot_dir = can_shoot(er, ee))	&&
	    !on(player, ISINWALL)		&& 
	    (rnd(100) < 50)) {
	    static struct object blast =
	    {
		MISSILE, {0, 0}, "", 0, "", "150" , NULL, 0, 0, 0, 0
	    };

	    turn_off(*tp, CANSONIC);
	    do_motion(&blast, shoot_dir->y, shoot_dir->x, tp);
	    damage = 150;
	    if (save(VS_BREATH, &player, -3))
		damage /= 2;
	    msg ("The %s's sonic blast hits you", monsters[tp->t_index].m_name);
	    if ((pstats.s_hpt -= damage) <= 0)
		death(tp->t_index);
	    ch_ret = *er;
	    running = FALSE;
    }
    /*
     * If we have a special magic item, we might use it.  We will restrict
     * this options to uniques with relics for now.
     */
    else if (on(*tp, ISUNIQUE) && m_use_item(tp, er, ee)) {
	    ch_ret = *er;
	    running = FALSE;
    }
    /* 
     * If we can shoot or throw something, we might do so.
     * If next to player, then 80% prob will fight.
     */
    else if(on(*tp, CANSHOOT)			&&
	    (shoot_dir = can_shoot(er, ee))	&&
	    !on(player, ISINWALL)		&&
	    (dist > 3 || (rnd(100) > 80))	&&
	    (weapon = get_hurl(tp))) {
		missile(shoot_dir->y, shoot_dir->x, weapon, tp);
		ch_ret = *er;
	}

    /*
     * Otherwise, find the empty spot next to the chaser that is
     * closest to the chasee.
     */
    else {
	register int ey, ex, x, y;
	register struct room *rer, *ree;
	int dist_to_old = MININT; /* Dist from goal to old position */

	/* Get rooms */
	rer = roomin(er);	/* Room the chasER (monster) is in */	
	ree = roomin(ee);	/* Room the chasEE is in */

	/*
	 * This will eventually hold where we move to get closer
	 * If we can't find an empty spot, we stay where we are.
	 */
	dist = flee ? 0 : MAXINT;
	ch_ret = *er;

	/* Are we at our goal already? */
	if (!flee && ce(ch_ret, *ee)) return(FALSE);

	ey = er->y + 1;
	ex = er->x + 1;

	/* Check all possible moves */
	for (x = er->x - 1; x <= ex; x++) {
	    if (x < 0 || x >= COLS) /* Don't try off the board */
		continue;
	    for (y = er->y - 1; y <= ey; y++) {
		coord tryp;

		if ((y < 1) || (y >= LINES - 2)) /* Don't try off the board */
		    continue;

		/* Don't try the player if not going after the player */
		if ((flee || !ce(hero, *ee)) && x == hero.x && y == hero.y) {
		    next_player = TRUE;
		    continue;
		}

		tryp.x = x;
		tryp.y = y;

		/* Is there a monster on this spot closer to our goal?
		 * Don't look in our spot or where we were.
		 */
		if (!ce(tryp, *er) && !ce(tryp, tp->t_oldpos) &&
		    isalpha(mch = CCHAR( mvwinch(mw, y, x) ) )) {
		    int test_dist;

		    test_dist = DISTANCE(y, x, ee->y, ee->x);
		    if (test_dist <= 25 &&   /* Let's be fairly close */
			test_dist < monst_dist) {
			/* Could we really move there? */
			mvwaddch(mw, y, x, ' '); /* Temporarily blank monst */
			if (diag_ok(er, &tryp, tp)) monst_dist = test_dist;
			mvwaddch(mw, y, x, mch); /* Restore monster */
		    }
		}

		/* Can we move onto the spot? */	
		if (!diag_ok(er, &tryp, tp)) continue;

		ch = CCHAR( mvwinch(cw, y, x) );	/* Screen character */

		/* Stepping on player is NOT okay if we are fleeing */
		if (step_ok(y, x, NOMONST, tp) &&
		    (off(*tp, ISFLEE) || ch != PLAYER))
		{
		    /*
		     * If it is a trap, an intelligent monster may not
		     * step on it (unless our hero is on top!)
		     */
		    if ((isatrap(ch))			&& 
			(rnd(10) < tp->t_stats.s_intel) &&
			(!on(*tp, ISFLY))		&&
			(y != hero.y || x != hero.x)) 
			    continue;

		    /*
		     * OK -- this place counts
		     */
		    thisdist = DISTANCE(y, x, ee->y, ee->x);

		    /* Adjust distance if we are being shot at */
		    if (tp->t_wasshot && tp->t_stats.s_intel > 5 &&
			ce(hero, *ee)) {
			/* Move out of line of sight */
			if (straight_shot(tryp.y, tryp.x, ee->y, ee->x, NULL)) {
			    if (flee) thisdist -= SHOTPENALTY;
			    else thisdist += SHOTPENALTY;
			}

			/* But do we want to leave the room? */
			else if (rer && rer == ree && ch == DOOR)
			    thisdist += DOORPENALTY;
		    }

		    /* Don't move to the last position if we can help it
		     * (unless out prey just moved there)
		     */
		    if (ce(tryp, tp->t_oldpos) && (flee || !ce(tryp, hero)))
			dist_to_old = thisdist;

		    else if ((flee && (thisdist > dist)) ||
			(!flee && (thisdist < dist)))
		    {
			ch_ret = tryp;
			dist = thisdist;
		    }
		}
	    }
	}

	/* If we aren't trying to get the player, but he is in our way,
	 * hit him (unless we have been turned)
	 */
	if (next_player && off(*tp, WASTURNED) &&
	    ((flee && ce(ch_ret, *er)) ||
	     (!flee && DISTANCE(er->y, er->x, ee->y, ee->x) < dist)) &&
	    step_ok(tp->t_dest->y, tp->t_dest->x, NOMONST, tp)) {
	    /* Okay to hit player */
	    ch_ret = hero;
	    return(FALSE);
	}


	/* If we can't get closer to the player (if that's our goal)
	 * because other monsters are in the way, just stay put
	 */
	if (!flee && ce(hero, *ee) && monst_dist < MAXINT &&
	    DISTANCE(er->y, er->x, hero.y, hero.x) < dist)
		ch_ret = *er;

	/* Do we want to go back to the last position? */
	else if (dist_to_old != MININT &&      /* It is possible to move back */
	    ((flee && dist == 0) ||	/* No other possible moves */
	     (!flee && dist == MAXINT))) {
	    /* Do we move back or just stay put (default)? */
	    dist = DISTANCE(er->y, er->x, ee->y, ee->x); /* Current distance */
	    if (!flee || (flee && (dist_to_old > dist))) ch_ret = tp->t_oldpos;
	}
    }

    /* May actually hit here from a confused move */
    return(!ce(ch_ret, hero));
}

/*
 * do_chase:
 *	Make one thing chase another.
 */

do_chase(th, flee)
register struct thing *th;
register bool flee; /* True if running away or player is inaccessible in wall */
{
    register struct room *rer, *ree,	/* room of chaser, room of chasee */
			 *orig_rer,	/* Original room of chaser */
			 *new_room;	/* new room of monster */
    int dist = MININT;
    int mindist = MAXINT, maxdist = MININT;
    bool stoprun = FALSE,		/* TRUE means we are there */
	 rundoor;			/* TRUE means run to a door */
    bool mdead = 0;
    char rch, sch;
    coord *last_door=0,			/* Door we just came from */
	   this;			/* Temporary destination for chaser */

    /* Make sure the monster can move */
    if (th->t_no_move != 0) {
	th->t_no_move--;
	return;
    }

    rer = roomin(&th->t_pos);	/* Find room of chaser */
    ree = roomin(th->t_dest);	/* Find room of chasee */
    orig_rer = rer;	/* Original room of chaser (including doors) */

    /*
     * We don't count monsters on doors as inside rooms for this routine
     */
    if ((sch = CCHAR( mvwinch(stdscr, th->t_pos.y, th->t_pos.x) )) == DOOR ||
	sch == PASSAGE) {
	rer = NULL;
    }
    this = *th->t_dest;

    /*
     * If we are not in a corridor and not a Xorn, then if we are running
     * after the player, we run to a door if he is not in the same room.
     * If we are fleeing, we run to a door if he IS in the same room.
     * Note:  We don't bother with doors in mazes.
     */
    if (levtype != MAZELEV && rer != NULL && off(*th, CANINWALL)) {
	if (flee) rundoor = (rer == ree);
	else rundoor = (rer != ree);
    }
    else rundoor = FALSE;

    if (rundoor) {
	register struct linked_list *exitptr;	/* For looping through exits */
	coord *exit;				/* A particular door */
	int exity, exitx;			/* Door's coordinates */
	char dch='\0';				/* Door character */

	if (th->t_doorgoal)
	    dch = CCHAR( mvwinch(stdscr, th->t_doorgoal->y, th->t_doorgoal->x) );
	    
	/* Do we have a valid goal? */
	if ((dch == PASSAGE || dch == DOOR) &&  /* A real door */
	    (!flee || !ce(*th->t_doorgoal, hero))) { /* Player should not
						      * be at door if we are
						      * running away
						      */
	    this = *th->t_doorgoal;
	    dist = 0;	/* Indicate that we have our door */
	}

	/* Go through all the doors */
	else for (exitptr = rer->r_exit; exitptr; exitptr = next(exitptr)) {
	    exit = DOORPTR(exitptr);
	    exity = exit->y;
	    exitx = exit->x;

	    /* Make sure it is a real door */
	    dch = CCHAR( mvwinch(stdscr, exity, exitx) );
	    if (dch == PASSAGE || dch == DOOR) {
		/* Don't count a door if we are fleeing from the player and
		 * he is standing on it
		 */
		if (flee && ce(*exit, hero)) continue;
		
		/* Were we just on this door? */
		if (ce(*exit, th->t_oldpos)) last_door = exit;

		else {
		    dist = DISTANCE(th->t_dest->y, th->t_dest->x, exity, exitx);

		    /* If fleeing, we want to maximize distance from door to
		     * what we flee, and minimize distance from door to us.
		     */
		    if (flee)
		       dist -= DISTANCE(th->t_pos.y, th->t_pos.x, exity, exitx);

		    /* Maximize distance if fleeing, otherwise minimize it */
		    if ((flee && (dist > maxdist)) ||
			(!flee && (dist < mindist))) {
			th->t_doorgoal = exit;	/* Use this door */
			this = *exit;
			mindist = maxdist = dist;
		    }
		}
	    }
	}

	/* Could we not find a door? */
	if (dist == MININT) {
	    /* If we were on a door, go ahead and use it */
	    if (last_door) {
		th->t_doorgoal = last_door;
		this = th->t_oldpos;
		dist = 0;	/* Indicate that we found a door */
	    }
	    else th->t_doorgoal = NULL;	/* No more door goal */
	}

	/* Indicate that we do not want to flee from the door */
	if (dist != MININT) flee = FALSE;
    }
    else th->t_doorgoal = 0;	/* Not going to any door */

    /*
     * this now contains what we want to run to this time
     * so we run to it.  If we hit it we either want to fight it
     * or stop running
     */
    if (!chase(th, &this, flee, &mdead)) {
	if (ce(ch_ret, hero)) {
	    /* merchants try to sell something --> others attack */
	    if (on(*th, CANSELL)) sell(th);
	    else attack(th, NULL, FALSE);
	    return;
	}
	else if (on(*th, NOMOVE))
	    stoprun = TRUE;
    }

    if (mdead) return;	/* Did monster kill someone? */

    if (on(*th, NOMOVE)) return;

    /* If we have a scavenger, it can pick something up */
    if (on(*th, ISSCAVENGE)) {
	register struct linked_list *n_item, *o_item;

	while ((n_item = find_obj(ch_ret.y, ch_ret.x)) != NULL) {
	    char floor = (roomin(&ch_ret) == NULL) ? PASSAGE : FLOOR;
	    register struct object *n_obj, *o_obj;

	    /*
	     * see if he's got one of this group already
	     */
	    o_item = NULL;
	    n_obj = OBJPTR(n_item);
	    detach(lvl_obj, n_item);
	    if (n_obj->o_group) {
		for(o_item = th->t_pack; o_item != NULL; o_item = next(o_item)){
		    o_obj = OBJPTR(o_item);
		    if (o_obj->o_group == n_obj->o_group) {
			o_obj->o_count += n_obj->o_count;
			o_discard(n_item);
			break;
		    }
		}
	    }
	    if (o_item == NULL) {	/* didn't find it */
	        attach(th->t_pack, n_item);
	    }
	    if (cansee(ch_ret.y, ch_ret.x))
	        mvwaddch(cw, ch_ret.y, ch_ret.x, floor);
	    mvaddch(ch_ret.y, ch_ret.x, floor);
	}
    }

    mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
    sch = CCHAR( mvwinch(cw, ch_ret.y, ch_ret.x) ); /* What player sees */
    rch = CCHAR( mvwinch(stdscr, ch_ret.y, ch_ret.x) ); /* What's really there */

    /* Get new room of monster */
    new_room=roomin(&ch_ret);

    /* If we have a tunneling monster, it may be making a tunnel */
    if (on(*th, CANTUNNEL)	&&
	(rch == SECRETDOOR || rch == WALL || rch == '|' || rch == '-')) {
	char nch;	/* The new look to the tunnel */

	if (rch == WALL) nch = PASSAGE;
	else if (levtype == MAZELEV) nch = FLOOR;
	else nch = DOOR;
	addch(nch);

	if (cansee(ch_ret.y, ch_ret.x)) sch = nch; /* Can player see this? */

	/* Does this make a new exit? */
	if (rch == '|' || rch == '-') {
	    struct linked_list *newroom;
	    coord *exit;

	    newroom = new_item(sizeof(coord));
	    exit = DOORPTR(newroom);
	    *exit = ch_ret;
	    attach(new_room->r_exit, newroom);
	}
    }

    /* Mark if the monster is inside a wall */
    if (isrock(mvinch(ch_ret.y, ch_ret.x))) turn_on(*th, ISINWALL);
    else turn_off(*th, ISINWALL);

    /* If the monster can illuminate rooms, check for a change */
    if (on(*th, HASFIRE)) {
	register struct linked_list *fire_item;

	/* Is monster entering a room? */
	if (orig_rer != new_room && new_room != NULL) {
	    fire_item = creat_item();	/* Get an item-only structure */
	    ldata(fire_item) = (char *) th;

	    attach(new_room->r_fires, fire_item);
	    new_room->r_flags |= HASFIRE;

	    if (cansee(ch_ret.y, ch_ret.x) && next(new_room->r_fires) == NULL)
		light(&hero);
	}

	/* Is monster leaving a room? */
	if (orig_rer != new_room && orig_rer != NULL) {
	    /* Find the bugger in the list and delete him */
	    for (fire_item = orig_rer->r_fires; fire_item != NULL;
		 fire_item = next(fire_item)) {
		if (THINGPTR(fire_item) == th)  {	/* Found him! */
		    detach(orig_rer->r_fires, fire_item);
		    destroy_item(fire_item);
		    if (orig_rer->r_fires == NULL) {
			orig_rer->r_flags &= ~HASFIRE;
			if (cansee(th->t_pos.y, th->t_pos.x))
			    light(&th->t_pos);
		    }
		    break;
		}
	    }
	}
    }

    /* If monster is entering player's room and player can see it,
     * stop the player's running.
     */
    if (new_room != orig_rer && new_room != NULL  &&
	new_room == ree && cansee(unc(ch_ret))    &&
	(off(*th, ISINVIS)     || on(player, CANSEE)) &&
	(off(*th, ISSHADOW)    || on(player, CANSEE)) &&
	(off(*th, CANSURPRISE) || ISWEARING(R_ALERT)))
		running = FALSE;

/*
    if (rer != NULL && !lit_room(orig_rer) && sch == FLOOR &&
	DISTANCE(ch_ret.y, ch_ret.x, th->t_pos.y, th->t_pos.x) < 3 &&
	off(player, ISBLIND))
	    th->t_oldch = ' ';
    else
 */
	th->t_oldch = sch;

    /* Let's display those creatures that we can see. */
    if (cansee(unc(ch_ret)) &&
	off(*th, ISINWALL) &&
	!invisible(th))
        mvwaddch(cw, ch_ret.y, ch_ret.x, th->t_type);

    /*
     * Blank out the old position and record the new position --
     * the blanking must be done first in case the positions are the same.
     */
    mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');
    mvwaddch(mw, ch_ret.y, ch_ret.x, th->t_type);

    /* Record monster's last position (if new one is different) */
    if (!ce(ch_ret, th->t_pos)) th->t_oldpos = th->t_pos;
    th->t_pos = ch_ret;		/* Mark the monster's new position */

    /* If the monster is on a trap, trap it */
    sch = CCHAR( mvinch(ch_ret.y, ch_ret.x) );
    if (isatrap(sch)) {
	if (cansee(ch_ret.y, ch_ret.x)) th->t_oldch = sch;
	be_trapped(th, &ch_ret);
    }


    /*
     * And stop running if need be
     */
    if (stoprun && ce(th->t_pos, *(th->t_dest)))
	turn_off(*th, ISRUN);
}


/* 
 * Get_hurl returns the weapon that the monster will "throw" if he has one 
 */

struct linked_list *
get_hurl(tp)
register struct thing *tp;
{
    struct linked_list *arrow, *bolt, *rock;
    register struct linked_list *pitem;
    bool bow=FALSE, crossbow=FALSE, sling=FALSE;

    arrow = bolt = rock = NULL;	/* Don't point to anything to begin with */
    for (pitem=tp->t_pack; pitem; pitem=next(pitem))
	if ((OBJPTR(pitem))->o_type == WEAPON)
	    switch ((OBJPTR(pitem))->o_which) {
		case BOW:	bow = TRUE;
		when CROSSBOW:	crossbow = TRUE;
		when SLING:	sling = TRUE;
		when ROCK:	rock = pitem;
		when ARROW:	arrow = pitem;
		when BOLT:	bolt = pitem;
	    }
    
    /* Use crossbow bolt if possible */
    if (crossbow && bolt) return(bolt);
    if (bow && arrow) return(arrow);
    if (sling && rock) return(rock);
    return(NULL);
}

/*
 * runners:
 *	Make all the running monsters move.
 */

runners()
{
    register struct linked_list *item;
    register struct thing *tp = NULL;

    /*
     * loop thru the list of running (wandering) monsters and see what
     * each one will do this time. 
     *
     * Note: the special case that one of this buggers kills another.
     *	     if this happens than we have to see if the monster killed
     *	     himself or someone else. In case its himself we have to get next
     *	     one immediately. If it wasn't we have to get next one at very
     *	     end in case he killed the next one.
     */

    for (item = mlist; item != NULL; item = next(item)) {
	tp = THINGPTR(item);
	turn_on(*tp, ISREADY);
    }

    for (;;) {
	for (item = mlist; item != NULL; item = next(item)) {
	    tp = THINGPTR(item);

	    if (on(*tp, ISREADY))
		break;
	}

	if (item == NULL)
	    break;

	turn_off(*tp, ISREADY);

	if (on(*tp, ISHELD) && rnd(tp->t_stats.s_lvl) > 11) {
	    turn_off(*tp, ISHELD);
	    turn_on(*tp, ISRUN);
	    turn_off(*tp, ISDISGUISE);
	    tp->t_dest = &hero;
	    if (tp->t_stats.s_hpt < tp->maxstats.s_hpt)
		turn_on(*tp, ISFLEE);
	    if (cansee(tp->t_pos.y, tp->t_pos.x))
		msg("The %s breaks free from the hold spell", 
			monsters[tp->t_index].m_name);
	}
	if (off(*tp, ISHELD) && on(*tp, ISRUN)) {
	    register bool flee;

	    /* Should monster run away? */
	    flee = on(*tp, ISFLEE) ||
		((tp->t_dest == &hero) && on(player, ISINWALL) &&
		 off(*tp, CANINWALL));

	    if (off(*tp, ISSLOW) || tp->t_turn) {
		doctor(tp);
		do_chase(tp, flee);
	    }
	    if (off(*tp, ISDEAD) && off(*tp, ISELSEWHERE) && on(*tp, ISHASTE)) {
		doctor(tp);
		do_chase(tp, flee);
	    }
	    if (off(*tp, ISDEAD) && off(*tp, ISELSEWHERE)) {
		tp->t_turn ^= TRUE;
		tp->t_wasshot = FALSE;	/* Not shot anymore */
	    }
	}
    }
}

/*
 * runto:
 *	Set a monster running after something
 */

runto(runner, spot)
register struct thing *runner;
coord *spot;
{
    /*
     * Start the beastie running
     */
    runner->t_dest = spot;
    turn_on(*runner, ISRUN);
    turn_off(*runner, ISDISGUISE);
}



/*
 * straight_shot:
 *	See if there is a straight line of sight between the two
 *	given coordinates.  If shooting is not NULL, it is a pointer
 *	to a structure which should be filled with the direction
 *	to shoot (if there is a line of sight).  If shooting, monsters
 *	get in the way.  Otherwise, they do not.
 */

bool
straight_shot(ery, erx, eey, eex, shooting)
register int ery, erx, eey, eex;
register coord *shooting;
{
    register int dy, dx;	/* Deltas */
    char ch;

    /* Does the monster have a straight shot at player */
    if ((ery != eey) && (erx != eex) &&
	(abs(ery - eey) != abs(erx - eex))) return(FALSE);

    /* Get the direction to shoot */
    if (eey > ery) dy = 1;
    else if (eey == ery) dy = 0;
    else dy = -1;

    if (eex > erx) dx = 1;
    else if (eex == erx) dx = 0;
    else dx = -1;

    /* Make sure we have free area all the way to the player */
    ery += dy;
    erx += dx;
    while ((ery != eey) || (erx != eex)) {
	switch (ch = CCHAR( winat(ery, erx) )) {
	    case '|':
	    case '-':
	    case WALL:
	    case DOOR:
	    case SECRETDOOR:
	    case FOREST:
		return(FALSE);
	    default:
		if (shooting && isalpha(ch)) return(FALSE);
	}
	ery += dy;
	erx += dx;
    }

    if (shooting) {	/* If we are shooting -- put in the directions */
	shooting->y = dy;
	shooting->x = dx;
    }
    return(TRUE);
}


