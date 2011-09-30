/*
 * functions for dealing with the "outside" level
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "curses.h"
#include "rogue.h"

extern char rnd_terrain(), get_terrain();

/*
 * init_terrain:
 *	Get the single "outside room" set up correctly
 */

void
init_terrain()
{
    register struct room *rp;

    for (rp = rooms; rp < &rooms[MAXROOMS]; rp++) {
	    rp->r_flags = ISGONE;	/* kill all rooms */
	    rp->r_fires = NULL;		/* no fires */
    }
    rp = &rooms[0];			/* point to only room */
    rp->r_flags = ISDARK;		/* outside is always dark */
    rp->r_pos.x = 0;			/* room fills whole screen */
    rp->r_pos.y = 1;
    rp->r_max.x = COLS;
    rp->r_max.y = LINES - 3;
}



void
do_terrain(basey, basex, deltay, deltax, fresh)
int basey, basex, deltay, deltax;
bool fresh;
{
    register cury, curx;	/* Current y and x positions */

    /* Lay out the boundary */
    for (cury=1; cury<LINES-2; cury++) {	/* Vertical "walls" */
	mvaddch(cury, 0, '|');
	mvaddch(cury, COLS-1, '|');
    }
    for (curx=0; curx<COLS; curx++) {		/* Horizontal "walls" */
	mvaddch(1, curx, '-');
	mvaddch(LINES-3, curx, '-');
    }

    /* If we are not continuing, let's start out with a line of terrain */
    if (fresh) {
	char ch;	/* Next char to add */

	/* Move to the starting point (should be (1, 0)) */
	move(basey, basex);
	curx = basex;

	/* Start with some random terrain */
	if (basex == 0) {
	    ch = rnd_terrain();
	    addch(ch);
	}
	else ch = CCHAR( mvinch(basey, basex) );

	curx += deltax;

	/* Fill in the rest of the line */
	while (curx > 0 && curx < COLS-1) {
	    /* Put in the next piece */
	    ch = get_terrain(ch, '\0', '\0', '\0');
	    mvaddch(basey, curx, ch);
	    curx += deltax;
	}

	basey++;	/* Advance to next line */
    }

    /* Fill in the rest of the lines */
    cury = basey;
    while (cury > 1 && cury < LINES - 3) {
	curx = basex;
	while (curx > 0 && curx < COLS-1) {
	    register char left, top_left, top, top_right;
	    register int left_pos, top_pos;

	    /* Get the surrounding terrain */
	    left_pos = curx - deltax;
	    top_pos = cury - deltay;

	    left = CCHAR( mvinch(cury, left_pos) );
	    top_left = CCHAR( mvinch(top_pos, left_pos) );
	    top = CCHAR( mvinch(top_pos, curx) );
	    top_right = CCHAR( mvinch(top_pos, curx + deltax) );

	    /* Put the piece of terrain on the map */
	    mvaddch(cury, curx, get_terrain(left, top_left, top, top_right));

	    /* Get the next x coordinate */
	    curx += deltax;
	}

	/* Get the next y coordinate */
	cury += deltay;
    }
    genmonsters(5, (bool) 0);
}


/*
 * do_paths:
 *	draw at least a single path-way through the terrain
 */


/*
 * rnd_terrain:
 *	return a weighted, random type of outside terrain
 */

char
rnd_terrain()
{
    int chance = rnd(100);

    /* Forest is most likely */
    if (chance < 60) return(FOREST);

    /* Next comes meadow */
    if (chance < 90) return(FLOOR);

    /* Then comes lakes */
    if (chance < 97) return(POOL);

    /* Finally, mountains */
    return(WALL);
}


/*
 * get_terrain:
 *	return a terrain weighted by what is surrounding
 */

char
get_terrain(one, two, three, four)
char one, two, three, four;
{
    register int i;
    int forest = 0, mountain = 0, lake = 0, meadow = 0, total = 0;
    char surrounding[4];

    surrounding[0] = one;
    surrounding[1] = two;
    surrounding[2] = three;
    surrounding[3] = four;

    for (i=0; i<4; i++) 
	switch (surrounding[i]) {
	    case FOREST:
		forest++;
		total++;
	    
	    when WALL:
		mountain++;
		total++;

	    when POOL:
		lake++;
		total++;

	    when FLOOR:
		meadow++;
		total++;
	}

    /* Should we continue mountain? */
    if (rnd(total+1) < mountain) return(WALL);

    /* Should we continue lakes? */
    if (rnd(total+1) < lake) return(POOL);

    /* Should we continue meadow? */
    if (rnd(total+1) < meadow) return(FLOOR);

    /* Should we continue forest? */
    if (rnd(total+2) < forest) return(FOREST);

    /* Return something random */
    return(rnd_terrain());
}


/*
 * lake_check:
 *	Determine if the player would drown
 */

void
lake_check(place)
coord *place;
{
    NOOP(place);
}
