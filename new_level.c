/*
 * new_level: Dig and draw a new level
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
#define TERRASAVE 3

new_level(ltype)
LEVTYPE	ltype;		/* designates type of level to create */
{
    register int rm = 0, i, cnt;
    register char ch;
    register struct linked_list *item;
    register struct thing *tp;
    register struct object *obj;
    int waslit = 0;	/* Was the previous outside level lit? */
    int starty = 0, startx = 0, deltay = 0, deltax = 0;
    bool fresh=TRUE, vert = 0, top;
    struct room *rp;
    struct linked_list *nitem, *savmonst=NULL, *savitems=NULL;
    coord stairs = { 0, 0 };

    if (wizard) {
	msg("Turns: %d", turns);	/* Number of turns for last level */
	mpos = 0;
    }

    /* Start player off right */
    turn_off(player, ISHELD);
    turn_off(player, ISFLEE);
    extinguish(suffocate);
    hold_count = 0;
    trap_tries = 0;

    /* Are we just entering a dungeon?  If so, how big is it? */
    if (ltype != OUTSIDE && nfloors < 0) nfloors = HARDER+10  + rnd(11);

    if (level > max_level)
	max_level = level;

    /* Are we starting a new outside level? */
    if (ltype == OUTSIDE) {
	register int i, j;

	/* Save some information prior to clearing the screen */
	if (level == -1 || mvinch(hero.y, hero.x)  == '-') vert = TRUE;
	else vert = FALSE;

	if (level == -1) {
	    fresh = TRUE;
	    starty = 2;
	    startx = 1;
	    deltay = deltax = 1;
	    level = 0;	/* Restore the level */
	}
	else {	/* Copy several lines of the terrain to the other end */
	    char cch;	/* Copy character */

	    /* Was the area dark (not magically lit)? */
	    if (!(rooms[0].r_flags & ISDARK)) waslit = 1;

	    fresh = FALSE;
	    if ((vert && hero.y == 1) || (!vert && hero.x == 0)) top = TRUE;
	    else top = FALSE;
	    for (i=0; i<TERRASAVE; i++) {
		if (vert)
		    for (j=1; j<COLS-1; j++) {
			if (top) {
			    cch = CCHAR( mvinch(i+2, j) );
			    mvaddch(LINES-6+i, j, cch);
			}
			else {
			    cch = CCHAR( mvinch(LINES-4-i, j) );
			    mvaddch(4-i, j, cch);
			}
		    }
		else
		    for (j=2; j<LINES-3; j++) {
			if (top) {
			    cch = CCHAR( mvinch(j, i+1) );
			    mvaddch(j, COLS-4+i, cch);
			}
			else {
			    cch = CCHAR( mvinch(j, COLS-2-i) );
			    mvaddch(j, 3-i, cch);
			}
		    }
	    }

	    if (vert) {
		startx = deltax = 1;
		if (top) {
		    starty = LINES-4-TERRASAVE;
		    deltay = -1;
		}
		else {
		    starty = TERRASAVE + 2;
		    deltay = 1;
		}
	    }
	    else {
		starty = 2;
		deltay = 1;
		if (top) {
		    startx = COLS-2-TERRASAVE;
		    deltax = -1;
		}
		else {
		    deltax = 1;
		    startx = TERRASAVE + 1;
		}
	    }

	    /* Check if any monsters should be saved */
	    for (item = mlist; item != NULL; item = nitem) {
		nitem = next(item);
		tp = THINGPTR(item);
		if (vert) {
		    if (top) {
			if (tp->t_pos.y < TERRASAVE + 2)
			    tp->t_pos.y += LINES - 5 - TERRASAVE;
			else continue;
		    }
		    else {
			if (tp->t_pos.y > LINES - 4 - TERRASAVE)
			    tp->t_pos.y += 5 + TERRASAVE - LINES;
			else continue;
		    }
		}
		else {
		    if (top) {
			if (tp->t_pos.x < TERRASAVE + 1)
			    tp->t_pos.x += COLS - 2 - TERRASAVE;
			else continue;
		    }
		    else {
			if (tp->t_pos.x > COLS - 2 - TERRASAVE)
			    tp->t_pos.x += 2 + TERRASAVE - COLS;
			else continue;
		    }
		}
		detach(mlist, item);
		attach(savmonst, item);
	    }

	    /* Check if any treasure should be saved */
	    for (item = lvl_obj; item != NULL; item = nitem) {
		nitem = next(item);
		obj = OBJPTR(item);
		if (vert) {
		    if (top) {
			if (obj->o_pos.y < TERRASAVE + 2)
			    obj->o_pos.y += LINES - 5 - TERRASAVE;
			else continue;
		    }
		    else {
			if (obj->o_pos.y > LINES - 4 - TERRASAVE)
			    obj->o_pos.y += 5 + TERRASAVE - LINES;
			else continue;
		    }
		}
		else {
		    if (top) {
			if (obj->o_pos.x < TERRASAVE + 1)
			    obj->o_pos.x += COLS - 2 - TERRASAVE;
			else continue;
		    }
		    else {
			if (obj->o_pos.x > COLS - 2 - TERRASAVE)
			    obj->o_pos.x += 2 + TERRASAVE - COLS;
			else continue;
		    }
		}
		detach(lvl_obj, item);
		attach(savitems, item);
	    }
	}
    }


    wclear(cw);
    wclear(mw);
    if (fresh) clear();
    /*
     * check to see if he missed a UNIQUE, If he did then put it back
     * in the monster table for next time
     */
    for (item = mlist; item != NULL; item = next(item)) {
	tp = THINGPTR(item);
	if (on(*tp, ISUNIQUE)) 
	    monsters[tp->t_index].m_normal = TRUE;
    }
    /*
     * Free up the monsters on the last level
     */
    t_free_list(monst_dead);
    t_free_list(mlist);
    o_free_list(lvl_obj);		/* Free up previous objects (if any) */
    for (rp = rooms; rp < &rooms[MAXROOMS]; rp++)
	t_free_list(rp->r_exit);	/* Free up the exit lists */

    levtype = ltype;
    foods_this_level = 0;		/* food for hero this level */
    if (ltype == POSTLEV) {
	do_post();			/* do post stuff */
    }
    else if (ltype == MAZELEV) {
	do_maze();
	no_food++;
	put_things(ltype);		/* Place objects (if any) */
    }
    else if (ltype == OUTSIDE) {
	init_terrain();
	do_terrain(starty, startx, deltay, deltax, (bool) (fresh || !vert));
	no_food++;
	put_things(ltype);

	/* Should we magically light this area? */
	if (waslit) rooms[0].r_flags &= ~ISDARK;
    }
    else {
	do_rooms();			/* Draw rooms */
	do_passages();			/* Draw passages */
	no_food++;
	put_things(ltype);		/* Place objects (if any) */
    }
    /*
     * Place the staircase down.  Only a small chance for an outside stairway.
     */
    if (ltype != OUTSIDE || roll(1, 4) == 4) {
	cnt = 0;
	do {
	    rm = rnd_room();
	    rnd_pos(&rooms[rm], &stairs);
	} until (mvinch(stairs.y, stairs.x) == FLOOR || cnt++ > 5000);
	addch(STAIRS);
    }
    /*
     * maybe add a trading post 
     */
    if (level > 5 && rnd(11) == 7 && ltype == NORMLEV) {
	cnt = 0;
	do {
	    rm = rnd_room();
	    if (rooms[rm].r_flags & ISTREAS)
		continue;
	    rnd_pos(&rooms[rm], &stairs);
	} until (winat(stairs.y, stairs.x) == FLOOR || cnt++ > 5000);
	addch(POST);
    }
    if (ltype != POSTLEV) {	/* Add monsters that fell through */
	nitem = tlist;
	while (nitem != NULL) {
	    item = nitem;
	    nitem = next(item); /* because detach and attach mess up ptrs */
	    tp = THINGPTR(item);
	    cnt = 0;
	    do {
		rm = rnd_room();
		rnd_pos(&rooms[rm], &tp->t_pos);
	    } until (cnt++ > 5000 || winat(tp->t_pos.y, tp->t_pos.x) == FLOOR);
	    mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, tp->t_type);
	    tp->t_oldch = CCHAR( mvwinch(cw, tp->t_pos.y, tp->t_pos.x) );

	    /*
	     * If it has a fire, mark it
	     */
	    if (on(*tp, HASFIRE)) {
		register struct linked_list *fire_item;

		fire_item = creat_item();
		ldata(fire_item) = (char *) tp;
		attach(rooms[rm].r_fires, fire_item);
		rooms[rm].r_flags |= HASFIRE;
	    }
	    turn_off(*tp, ISELSEWHERE);
	    detach(tlist, item);
	    attach(mlist, item);
	}
    }

    /* Restore any saved monsters */
    for (item = savmonst; item != NULL; item = nitem) {
	nitem = next(item);
	tp = THINGPTR(item);
	mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, tp->t_type);
	tp->t_oldch = CCHAR( mvwinch(cw, tp->t_pos.y, tp->t_pos.x) );

	/*
	 * If it has a fire, mark it
	 */
	if (on(*tp, HASFIRE)) {
	    register struct linked_list *fire_item;

	    fire_item = creat_item();
	    ldata(fire_item) = (char *) tp;
	    attach(rooms[rm].r_fires, fire_item);
	    rooms[rm].r_flags |= HASFIRE;
	}

	detach(savmonst, item);
	attach(mlist, item);
    }

    /* Restore any saved objects */
    for(item = savitems; item != NULL; item = nitem) {
	nitem = next(item);
	obj = OBJPTR(item);
	mvaddch(obj->o_pos.y, obj->o_pos.x, obj->o_type);
	detach(savitems, item);
	attach(lvl_obj, item);
    }


    /*
     * Place the traps (except for trading post)
     */
    ntraps = 0;	/* No traps yet */
    if (levtype == NORMLEV) {
	if (rnd(10) < vlevel) {
	    ntraps = rnd(vlevel/4)+1;
	    if (ntraps > MAXTRAPS)
		ntraps = MAXTRAPS;
	    i = ntraps;
	    while (i--)
	    {
		cnt = 0;
		do {
		    rm = rnd_room();
		    if (rooms[rm].r_flags & ISTREAS)
			continue;
		    rnd_pos(&rooms[rm], &stairs);
		} until (winat(stairs.y, stairs.x) == FLOOR || cnt++ > 5000);

		traps[i].tr_flags = 0;

		/* If we are at the bottom, we can't set a trap door */
		if (level >= nfloors) ch = (char) rnd(7) + 1;
		else ch = (char) rnd(8);

		switch((int) ch) {
		    case 0: ch = TRAPDOOR;
		    when 1: ch = BEARTRAP;
		    when 2: ch = SLEEPTRAP;
		    when 3: ch = ARROWTRAP;
		    when 4: ch = TELTRAP;
		    when 5: ch = DARTTRAP;
		    when 6: ch = POOL;
			    traps[i].tr_flags = ISFOUND;
		    when 7: ch = MAZETRAP;
		}
		addch(ch);
		traps[i].tr_type = ch;
		traps[i].tr_show = FLOOR;
		traps[i].tr_pos = stairs;
	    }
	}
    }
    if (fresh) {	/* A whole new picture */
	cnt = 0;
	do {
	    rm = rnd_room();
	    if (rooms[rm].r_flags & ISTREAS)
		continue;
	    rnd_pos(&rooms[rm], &hero);
	} until(	cnt++ > 5000 ||
		    (winat(hero.y, hero.x) == FLOOR &&
		     DISTANCE(hero.y, hero.x, stairs.y, stairs.x) > 16));
    }
    else {		/* We're extending into an adjacent outside plane */
	rm = 0;
	if (vert) {
	    if (hero.y == 1) hero.y = LINES - 3 - TERRASAVE; /* Top to bottom */
	    else hero.y = TERRASAVE + 1;	/* Bottom to top */
	}
	else {
	    if (hero.x == 0) hero.x = COLS - 1 - TERRASAVE; /* Right to left */
	    else hero.x = TERRASAVE;	/* Left to right */
	}
    }
    oldrp = &rooms[rm];		/* Set the current room */
    player.t_oldpos = player.t_pos;	/* Set the current position */
    if (ISWEARING(R_AGGR) || 
	(cur_misc[WEAR_JEWEL] != NULL && 
	 cur_misc[WEAR_JEWEL]->o_which == MM_JEWEL))
	aggravate();
    light(&hero);
    wmove(cw, hero.y, hero.x);
    waddch(cw, PLAYER);

    if (level > cur_max)
	cur_max = level;

    status(TRUE);
}

/*
 * Pick a room that is really there
 */

rnd_room()
{
    register int rm;

    if (levtype != NORMLEV)
	rm = 0;
    else do
	{
	    rm = rnd(MAXROOMS);
	} while (rooms[rm].r_flags & ISGONE);
    return rm;
}

/*
 * put_things:
 *	put potions and scrolls on this level
 */

put_things(ltype)
LEVTYPE	ltype;		/* designates type of level to create */
{
    register int i, rm, cnt;
    register struct object *cur;
    register struct linked_list *item, *exitptr;
    bool got_unique = FALSE;
    int length, width;
    coord tp, *exit;

    /*
     * The only way to get new stuff is to go down into the dungeon.
     */
    if (level <= cur_max)
	return;

    /* 
     * There is a chance that there is a treasure room on this level 
     * Increasing chance after level 9 
     */
    if (ltype != MAZELEV && rnd(HARDER) < level - 8) {	
	register  j;
	register struct room *rp;

	/* Count the number of free spaces */
	i = 0;	/* 0 tries */
	do {
	    rp = &rooms[rnd_room()];
	    width = rp->r_max.y - 2;
	    length = rp->r_max.x - 2;
	} until ((width*length >= MAXTREAS) || (i++ > MAXROOMS*4));

	/* Mark the room as a treasure room */
	rp->r_flags |= ISTREAS;

	/* Make all the doors secret doors */
	for (exitptr = rp->r_exit; exitptr; exitptr = next(exitptr)) {
	    exit = DOORPTR(exitptr);
	    move(exit->y, exit->x);
	    addch(SECRETDOOR);
	}

	/*
	 * check to see if there are any monsters in room already
	 */
	for (item = mlist; item != NULL; item = next(item)) {
	    register struct thing *tp;

	    tp = THINGPTR(item);
	    if (rp == roomin(&tp->t_pos)) {
		turn_on(*tp, ISMEAN);
		if (off(*tp, CANINWALL)) {
		    tp->t_dest = &hero;
		    turn_on(*tp, ISRUN);
		}
		if (on(*tp, ISUNIQUE))
		    got_unique = TRUE;

		/* If it is a mimic, undisguise it */
		if (on(*tp, ISDISGUISE)) 
		    turn_off(*tp, ISDISGUISE);
	    }
	}


	/* Put in the monsters and treasures */
	for (j=1; j<rp->r_max.y-1; j++)
	    for (i=1; i<rp->r_max.x-1; i++) {
		coord trp;

		trp.y = rp->r_pos.y+j;
		trp.x = rp->r_pos.x+i;

		/* Monsters */
		if ((rnd(100) < (MAXTREAS*100)/(width*length)) &&
		    (mvwinch(mw, rp->r_pos.y+j, rp->r_pos.x+i) == ' ')) {
		    register struct thing *tp;

		    /* Make a monster */
		    item = new_item(sizeof *tp);
		    tp = THINGPTR(item);

		    /* 
		     * Put it there and aggravate it (unless it can escape) 
		     * only put one UNIQUE per treasure room at most
		     */
		    if (got_unique)
			new_monster(item,randmonster(FALSE, TRUE),&trp,TRUE);
		    else 
			new_monster(item,randmonster(FALSE, FALSE),&trp,TRUE);
		    if (on(*tp, ISUNIQUE)) {
			got_unique = TRUE;
			carry_obj(tp, monsters[tp->t_index].m_carry);
		    }
		    turn_on(*tp, ISMEAN);
		    if (off(*tp, CANINWALL)) {
			tp->t_dest = &hero;
			turn_on(*tp, ISRUN);
		    }

		    /* If it is a mimic, undisguise it */
		    if (on(*tp, ISDISGUISE)) 
			turn_off(*tp, ISDISGUISE);

		    if (on(*tp, HASFIRE)) {
			register struct linked_list *fire_item;

			fire_item = creat_item();
			ldata(fire_item) = (char *) tp;
			attach(rp->r_fires, fire_item);
			rp->r_flags |= HASFIRE;
		    }
		}

		/* Treasures */
		if ((rnd(100) < (MAXTREAS*100)/(width*length)) &&
		    (mvinch(rp->r_pos.y+j, rp->r_pos.x+i) == FLOOR)) {
		    item = new_thing(ALL);
		    attach(lvl_obj, item);
		    cur = OBJPTR(item);
	
		    mvaddch(trp.y, trp.x, cur->o_type);
		    cur->o_pos = trp;
		}
	    }
    }

    /*
     * Do MAXOBJ attempts to put things on a level
     */
    for (i = 0; i < MAXOBJ; i++)
	if (rnd(100) < 45) {
	    /*
	     * Pick a new object and link it in the list
	     */
	    item = new_thing(ALL);
	    attach(lvl_obj, item);
	    cur = OBJPTR(item);
	    /*
	     * Put it somewhere
	     */
	    cnt = 0;
	    do {
		rm = rnd_room();
		rnd_pos(&rooms[rm], &tp);
	    } until (winat(tp.y, tp.x) == FLOOR || cnt++ > 500);
	    mvaddch(tp.y, tp.x, cur->o_type);
	    cur->o_pos = tp;
	}
}
