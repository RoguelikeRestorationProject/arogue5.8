/*
 * Draw the nine rooms on the screen
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

do_rooms()
{
    register int i;
    register struct room *rp;
    register struct linked_list *item;
    register struct thing *tp;
    register int left_out;
    coord top;
    coord bsze;
    coord mp;

    /*
     * bsze is the maximum room size
     */
    bsze.x = COLS/3;
    bsze.y = (LINES-2)/3;
    /*
     * Clear things for a new level
     */
    for (rp = rooms; rp < &rooms[MAXROOMS]; rp++) {
	rp->r_flags = 0;
	rp->r_fires = NULL;
    }
    /*
     * Put the gone rooms, if any, on the level
     */
    left_out = rnd(4);
    for (i = 0; i < left_out; i++)
	rooms[rnd_room()].r_flags |= ISGONE;
    /*
     * dig and populate all the rooms on the level
     */
    for (i = 0, rp = rooms; i < MAXROOMS; rp++, i++)
    {
	bool has_gold=FALSE;

	/*
	 * Find upper left corner of box that this room goes in
	 */
	top.x = (i%3)*bsze.x;
	top.y = i/3*bsze.y + 1;
	if (rp->r_flags & ISGONE)
	{
	    /*
	     * Place a gone room.  Make certain that there is a blank line
	     * for passage drawing.
	     */
	    do
	    {
		rp->r_pos.x = top.x + rnd(bsze.x-2) + 1;
		rp->r_pos.y = top.y + rnd(bsze.y-2) + 1;
		rp->r_max.x = -COLS;
		rp->r_max.x = -LINES;
	    } until(rp->r_pos.y > 0 && rp->r_pos.y < LINES-2);
	    continue;
	}
	if (rnd(10) < level-1)
	    rp->r_flags |= ISDARK;
	/*
	 * Find a place and size for a random room
	 */
	do
	{
	    rp->r_max.x = rnd(bsze.x - 4) + 4;
	    rp->r_max.y = rnd(bsze.y - 4) + 4;
	    rp->r_pos.x = top.x + rnd(bsze.x - rp->r_max.x);
	    rp->r_pos.y = top.y + rnd(bsze.y - rp->r_max.y);
	} until (rp->r_pos.y != 0);

	/* Draw the room */
	draw_room(rp);

	/*
	 * Put the gold in
	 */
	if (rnd(100) < 50 && level >= cur_max)
	{
	    register struct linked_list *item;
	    register struct object *cur;
	    coord tp;

	    has_gold = TRUE;	/* This room has gold in it */

	    item = spec_item(GOLD, NULL, NULL, NULL);
	    cur = OBJPTR(item);

	    /* Put the gold into the level list of items */
	    attach(lvl_obj, item);

	    /* Put it somewhere */
	    rnd_pos(rp, &tp);
	    mvaddch(tp.y, tp.x, GOLD);
	    cur->o_pos = tp;
	    if (roomin(&tp) != rp) {
		endwin();
		printf("\n");
		abort();
	    }
	}

	/*
	 * Put the monster in
	 */
	if (rnd(100) < (has_gold ? 80 : 25) + vlevel/2)
	{
	    item = new_item(sizeof *tp);
	    tp = THINGPTR(item);
	    do
	    {
		rnd_pos(rp, &mp);
	    } until(mvwinch(stdscr, mp.y, mp.x) == FLOOR);
	    new_monster(item, randmonster(FALSE, FALSE), &mp, FALSE);
	    /*
	     * See if we want to give it a treasure to carry around.
	     */
	    carry_obj(tp, monsters[tp->t_index].m_carry);

	    /*
	     * If it has a fire, mark it
	     */
	    if (on(*tp, HASFIRE)) {
		register struct linked_list *fire_item;

		fire_item = creat_item();
		ldata(fire_item) = (char *) tp;
		attach(rp->r_fires, fire_item);
		rp->r_flags |= HASFIRE;
	    }
	}
    }
}


/*
 * Draw a box around a room
 */

draw_room(rp)
register struct room *rp;
{
    register int j, k;

    move(rp->r_pos.y, rp->r_pos.x+1);
    vert(rp->r_max.y-2);				/* Draw left side */
    move(rp->r_pos.y+rp->r_max.y-1, rp->r_pos.x);
    horiz(rp->r_max.x);					/* Draw bottom */
    move(rp->r_pos.y, rp->r_pos.x);
    horiz(rp->r_max.x);					/* Draw top */
    vert(rp->r_max.y-2);				/* Draw right side */
    /*
     * Put the floor down
     */
    for (j = 1; j < rp->r_max.y-1; j++)
    {
	move(rp->r_pos.y + j, rp->r_pos.x+1);
	for (k = 1; k < rp->r_max.x-1; k++)
	    addch(FLOOR);
    }
}

/*
 * horiz:
 *	draw a horizontal line
 */

horiz(cnt)
register int cnt;
{
    while (cnt--)
	addch('-');
}

/*
 * rnd_pos:
 *	pick a random spot in a room
 */

rnd_pos(rp, cp)
register struct room *rp;
register coord *cp;
{
    cp->x = rp->r_pos.x + rnd(rp->r_max.x-2) + 1;
    cp->y = rp->r_pos.y + rnd(rp->r_max.y-2) + 1;
}



/*
 * roomin:
 *	Find what room some coordinates are in. NULL means they aren't
 *	in any room.
 */

struct room *
roomin(cp)
register coord *cp;
{
    register struct room *rp;

    for (rp = rooms; rp < &rooms[MAXROOMS]; rp++)
	if (inroom(rp, cp))
	    return rp;
    return NULL;
}

/*
 * vert:
 *	draw a vertical line
 */

vert(cnt)
register int cnt;
{
    register int x, y;

    getyx(stdscr, y, x);
    x--;
    while (cnt--) {
	move(++y, x);
	addch('|');
    }
}
