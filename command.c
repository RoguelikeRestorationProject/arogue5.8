/*
 * Read and execute the user commands
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
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include "mach_dep.h"
#include "rogue.h"

/*
 * command:
 *	Process the user commands
 */

command()
{
    register char ch;
    register int ntimes = 1;			/* Number of player moves */
    static char countch, direction, newcount = FALSE;
    struct linked_list *item;
    bool an_after = FALSE;

    if (on(player, ISHASTE)) {
	ntimes++;
	turns--;	/* correct for later */
    }
    if (on(player, ISSLOW) || on(player, ISDANCE)) {
	if (player.t_turn != TRUE) {
	    ntimes--;
	    turns++;
	    an_after = TRUE;
	}
	player.t_turn ^= TRUE;
    }

    /*
     * Let the daemons start up
     */
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    while (ntimes-- > 0)
    {	
	/* One more tick of the clock. */
	if ((++turns % DAYLENGTH) == 0) {
	    daytime ^= TRUE;
	    if (levtype == OUTSIDE) {
		if (daytime) msg("The sun rises above the horizon");
		else msg("The sun sinks below the horizon");
	    }
	    light(&hero);
	}

	look(after, FALSE);
	if (!running) door_stop = FALSE;
	lastscore = purse;
	wmove(cw, hero.y, hero.x);
	if (!((running || count) && jump)) {
	    status(FALSE);
	    wmove(cw, hero.y, hero.x);
	    draw(cw);			/* Draw screen */
	}
	take = 0;
	after = TRUE;
	/*
	 * Read command or continue run
	 */
	if (!no_command)
	{
	    if (running) {
		/* If in a corridor or maze, if we are at a turn with only one
		 * way to go, turn that way.
		 */
		if ((winat(hero.y, hero.x) == PASSAGE || levtype == MAZELEV) &&
		    off(player, ISHUH) && (off(player, ISBLIND))) {
		    int y, x;
		    if (getdelta(runch, &y, &x) == TRUE) {
			corr_move(y, x);
		    }
		}
	        ch = runch;
	    }
	    else if (count) ch = countch;
	    else
	    {
		ch = readchar();
		if (mpos != 0 && !running)	/* Erase message if its there */
		    msg("");
	    }
	}
	else ch = '.';
	if (no_command)
	{
	    if (--no_command == 0)
		msg("You can move again.");
	}
	else
	{
	    /*
	     * check for prefixes
	     */
	    if (isdigit(ch))
	    {
		count = 0;
		newcount = TRUE;
		while (isdigit(ch))
		{
		    count = count * 10 + (ch - '0');
		    if (count > 255)
			count = 255;
		    ch = readchar();
		}
		countch = ch;
		/*
		 * turn off count for commands which don't make sense
		 * to repeat
		 */
		switch (ch) {
		    case 'h': case 'j': case 'k': case 'l':
		    case 'y': case 'u': case 'b': case 'n':
		    case 'H': case 'J': case 'K': case 'L':
		    case 'Y': case 'U': case 'B': case 'N':
		    case 'q': case 'r': case 's': case 'f':
		    case 't': case 'C': case 'I': case '.':
		    case 'z': case 'p':
			break;
		    default:
			count = 0;
		}
	    }

	    /* Save current direction */
	    if (!running) /* If running, it is already saved */
	    switch (ch) {
		case 'h': case 'j': case 'k': case 'l':
		case 'y': case 'u': case 'b': case 'n':
		case 'H': case 'J': case 'K': case 'L':
		case 'Y': case 'U': case 'B': case 'N':
		    runch = tolower(ch);
	    }

	    /* Perform the action */
	    switch (ch) {
		case 'f':
		    if (!on(player, ISBLIND))
		    {
			door_stop = TRUE;
			firstmove = TRUE;
		    }
		    if (count && !newcount)
			ch = direction;
		    else
			ch = readchar();
		    switch (ch)
		    {
			case 'h': case 'j': case 'k': case 'l':
			case 'y': case 'u': case 'b': case 'n':
			    ch = toupper(ch);
		    }
		    direction = ch;
	    }
	    newcount = FALSE;
	    /*
	     * execute a command
	     */
	    if (count && !running)
		count--;
	    switch (ch)
	    {
		case '!' : shell();
		when 'h' : do_move(0, -1);
		when 'j' : do_move(1, 0);
		when 'k' : do_move(-1, 0);
		when 'l' : do_move(0, 1);
		when 'y' : do_move(-1, -1);
		when 'u' : do_move(-1, 1);
		when 'b' : do_move(1, -1);
		when 'n' : do_move(1, 1);
		when 'H' : do_run('h');
		when 'J' : do_run('j');
		when 'K' : do_run('k');
		when 'L' : do_run('l');
		when 'Y' : do_run('y');
		when 'U' : do_run('u');
		when 'B' : do_run('b');
		when 'N' : do_run('n');
		when 't':
		    if((item=get_item(pack,"throw", ALL)) != NULL && get_dir())
			missile(delta.y, delta.x, item, &player);
		    else
			after = FALSE;
		when 'Q' : after = FALSE; quit(-1);
		when 'i' : after = FALSE; inventory(pack, ALL);
		when 'I' : after = FALSE; picky_inven();
		when 'd' : drop(NULL);
		when 'P' : grab(hero.y, hero.x);
		when 'q' : quaff(-1, NULL, TRUE);
		when 'r' : read_scroll(-1, NULL, TRUE);
		when 'e' : eat();
		when 'w' : wield();
		when 'W' : wear();
		when 'T' : take_off();
		when 'o' : option();
		when 'c' : call(FALSE);
		when 'm' : call(TRUE);
		when '>' : after = FALSE; d_level();
		when '<' : after = FALSE; u_level();
		when '?' : after = FALSE; help();
		when '/' : after = FALSE; identify();
		when CTRL('U') : use_mm(-1);
		when CTRL('T') :
		    if (get_dir()) steal();
		    else after = FALSE;
		when 'D' : dip_it();
		when 'G' : gsense();
		when '^' : set_trap(&player, hero.y, hero.x);
		when 's' : search(FALSE, FALSE);
		when 'z' : if (!do_zap(TRUE, NULL, FALSE))
				after=FALSE;
		when 'p' : pray();
		when 'C' : cast();
		when 'a' :
		    if (get_dir())
			affect();
		    else after = FALSE;
		when 'v' : after = FALSE;
			   msg("Advanced Rogue Version %s.",
				release);
		when CTRL('L') : after = FALSE; clearok(curscr, TRUE);
				touchwin(cw); /* MMMMMMMMMM */
		when CTRL('R') : after = FALSE; msg(huh);
		when 'S' : 
		    after = FALSE;
		    if (save_game())
		    {
			wclear(cw);
			draw(cw);
			endwin();
			printf("\n");
			exit(0);
		    }
		when '.' : ;			/* Rest command */
		when ' ' : after = FALSE;	/* Do Nothing */
#ifdef WIZARD
		when CTRL('P') :
		    after = FALSE;
		    if (wizard)
		    {
			wizard = FALSE;
			trader = 0;
			msg("Not wizard any more");
		    }
		    else
		    {
			if (waswizard || passwd())
			{
			    msg("Welcome, oh mighty wizard.");
			    wizard = waswizard = TRUE;
			}
			else
			    msg("Sorry");
		    }
#endif
		when ESCAPE :	/* Escape */
		    door_stop = FALSE;
		    count = 0;
		    after = FALSE;
		when '#':
		    if (levtype == POSTLEV)		/* buy something */
			buy_it();
		    after = FALSE;
		when '$':
		    if (levtype == POSTLEV)		/* price something */
			price_it();
		    after = FALSE;
		when '%':
		    if (levtype == POSTLEV)		/* sell something */
			sell_it();
		    after = FALSE;
		otherwise :
		    after = FALSE;
#ifdef WIZARD
		    if (wizard) switch (ch)
		    {
			case 'M' : create_obj(TRUE, 0, 0);
			when CTRL('W') : wanderer();
			when CTRL('I') : inventory(lvl_obj, ALL);
			when CTRL('Z') : whatis(NULL);
			when CTRL('D') : level++; new_level(NORMLEV);
			when CTRL('F') : overlay(stdscr,cw);
			when CTRL('X') : overlay(mw,cw);
			when CTRL('J') : teleport();
			when CTRL('E') : sprintf(outstring,"food left: %d\tfood level: %d", 
						food_left, foodlev);
				       msg(outstring);
			when CTRL('A') : activity();
			when CTRL('C') : 
			{
			    int tlev;
			    prbuf[0] = '\0';
			    msg("Which level? ");
			    if(get_str(prbuf,cw) == NORM) {
				tlev = atoi(prbuf);
				if(tlev < 1) {
				    mpos = 0;
				    msg("Illegal level.");
				}
				else if (tlev > 199) {
					levtype = MAZELEV;
					level = tlev - 200 + 1;
				}
				else if (tlev > 99) {
					levtype = POSTLEV;
					level = tlev - 100 + 1;
				} 
				else {
					levtype = NORMLEV;
					level = tlev;
				}
				new_level(levtype);
			    }
			}
			when CTRL('N') :
			{
			    if ((item=get_item(pack, "charge", STICK)) != NULL){
				(OBJPTR(item))->o_charges=10000;
			    }
			}
			when CTRL('H') :
			{
			    register int i;
			    register struct object *obj;

			    for (i = 0; i < 9; i++)
				raise_level(TRUE);
			    /*
			     * Give the rogue a sword 
			     */
			    if(cur_weapon==NULL || cur_weapon->o_type!=RELIC) {
				item = spec_item(WEAPON, TWOSWORD, 5, 5);
				add_pack(item, TRUE, NULL);
				cur_weapon = OBJPTR(item);
				cur_weapon->o_flags |= (ISKNOW | ISPROT);
			    }
			    /*
			     * And his suit of armor
			     */
			    if (player.t_ctype == C_THIEF)
				item = spec_item(ARMOR, STUDDED_LEATHER, 10, 0);
			    else
				item = spec_item(ARMOR, PLATE_ARMOR, 7, 0);
			    obj = OBJPTR(item);
			    obj->o_flags |= (ISKNOW | ISPROT);
			    obj->o_weight = armors[PLATE_ARMOR].a_wght;
			    cur_armor = obj;
			    add_pack(item, TRUE, NULL);
			    purse += 20000;
			}
			otherwise :
			    msg("Illegal command '%s'.", unctrl(ch));
			    count = 0;
		    }
		    else
#endif
		    {
			msg("Illegal command '%s'.", unctrl(ch));
			count = 0;
			after = FALSE;
		    }
	    }
	    /*
	     * turn off flags if no longer needed
	     */
	    if (!running)
		door_stop = FALSE;
	}
	/*
	 * If he ran into something to take, let him pick it up.
	 * unless its a trading post
	 */
	if (auto_pickup && take != 0 && levtype != POSTLEV)
	    pick_up(take);
	if (!running)
	    door_stop = FALSE;

	/* If after is true, mark an_after as true so that if
	 * we are hasted, the first "after" will be noted.
	 * if after is FALSE then stay in this loop
	 */
	if (after) an_after = TRUE;
	else ntimes++;
    }

    /*
     * Kick off the rest if the daemons and fuses
     */
    if (an_after)
    {
	/* 
	 * If player is infested, take off a hit point 
	 */
	if (on(player, HASINFEST)) {
	    if ((pstats.s_hpt -= infest_dam) <= 0) death(D_INFESTATION);
	}
	/* 
	 * if player has body rot then take off five hits 
	 */
	if (on(player, DOROT)) {
	     if ((pstats.s_hpt -= 5) <= 0) death(D_ROT);
	}
	do_daemons(AFTER);
	do_fuses(AFTER);
	if (!((running || count) && jump)) look(FALSE, FALSE);


    }
    t_free_list(monst_dead);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */

void
quit(int sig)
{
    NOOP(sig);

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (signal(SIGINT, &quit) != &quit)
	mpos = 0;
    msg("Really quit? ");
    draw(cw);
    if (readchar() == 'y')
    {
	clear();
	move(LINES-1, 0);
	draw(stdscr);
	score(pstats.s_exp + (long) purse, CHICKEN, 0);
	exit(0);
    }
    else
    {
	signal(SIGINT, quit);
	wmove(cw, 0, 0);
	wclrtoeol(cw);
	status(FALSE);
	draw(cw);
	mpos = 0;
	count = 0;
	running = FALSE;
    }
}

/*
 * bugkill:
 *	killed by a program bug instead of voluntarily.
 */

void
bugkill(sig)
int sig;
{
    signal(sig, quit);	/* If we get it again, give up */
    death(D_SIGNAL);	/* Killed by a bug */
}


/*
 * search:
 *	Player gropes about him to find hidden things.
 */

search(is_thief, door_chime)
register bool is_thief, door_chime;
{
    register int x, y;
    register char ch,	/* The trap or door character */
		 sch,	/* Trap or door character (as seen on screen) */
		 mch;	/* Monster, if a monster is on the trap or door */
    register struct linked_list *item;
    register struct thing *mp; /* Status on surrounding monster */

    /*
     * Look all around the hero, if there is something hidden there,
     * give him a chance to find it.  If its found, display it.
     */
    if (on(player, ISBLIND))
	return;
    for (x = hero.x - 1; x <= hero.x + 1; x++)
	for (y = hero.y - 1; y <= hero.y + 1; y++)
	{
	    if (y==hero.y && x==hero.x)
		continue;

	    /* Mch and ch will be the same unless there is a monster here */
	    mch = CCHAR( winat(y, x) );
	    ch = CCHAR( mvwinch(stdscr, y, x) );
	    sch = CCHAR( mvwinch(cw, y, x) );	/* What's on the screen */

	    if (door_chime == FALSE && isatrap(ch)) {
		    register struct trap *tp;

		    /* Is there a monster on the trap? */
		    if (mch != ch && (item = find_mons(y, x)) != NULL) {
			mp = THINGPTR(item);
			if (sch == mch) sch = mp->t_oldch;
		    }
		    else mp = NULL;

		    /* 
		     * is this one found already?
		     */
		    if (isatrap(sch)) 
			continue;	/* give him chance for other traps */
		    tp = trap_at(y, x);
		    /* 
		     * if the thief set it then don't display it.
		     * if its not a thief he has 50/50 shot
		     */
		    if((tp->tr_flags&ISTHIEFSET) || (!is_thief && rnd(100)>50))
			continue;	/* give him chance for other traps */
		    tp->tr_flags |= ISFOUND;

		    /* Let's update the screen */
		    if (mp != NULL && CCHAR(mvwinch(cw, y, x)) == mch)
			mp->t_oldch = ch; /* Will change when monst moves */
		    else mvwaddch(cw, y, x, ch);

		    count = 0;
		    running = FALSE;
		    msg(tr_name(tp->tr_type));
	    }
	    else if (ch == SECRETDOOR) {
		if (door_chime == TRUE || (!is_thief && rnd(100) < 20)) {
		    /* Is there a monster on the door? */
		    if (mch != ch && (item = find_mons(y, x)) != NULL) {
			mp = THINGPTR(item);

			/* Screen will change when monster moves */
			if (sch == mch) mp->t_oldch = ch;
		    }
		    mvaddch(y, x, DOOR);
		    count = 0;
		}
	    }
	}
}


/*
 * help:
 *	Give single character help, or the whole mess if he wants it
 */

help()
{
    register struct h_list *strp = helpstr;
#ifdef WIZARD
    struct h_list *wizp = wiz_help;
#endif
    register char helpch;
    register int cnt;

    msg("Character you want help for (* for all): ");
    helpch = readchar();
    mpos = 0;
    /*
     * If its not a *, print the right help string
     * or an error if he typed a funny character.
     */
    if (helpch != '*') {
	wmove(cw, 0, 0);
	while (strp->h_ch) {
	    if (strp->h_ch == helpch) {
		sprintf(outstring,"%s%s", unctrl(strp->h_ch), strp->h_desc);
		msg(outstring);
		return;
	    }
	    strp++;
	}
#ifdef WIZARD
	if (wizard) {
	    while (wizp->h_ch) {
		if (wizp->h_ch == helpch) {
		    sprintf(outstring,"%s%s", unctrl(wizp->h_ch), wizp->h_desc);
		    msg(outstring);
		    return;
		}
		wizp++;
	    }
	}
#endif

	msg("Unknown character '%s'", unctrl(helpch));
	return;
    }
    /*
     * Here we print help for everything.
     * Then wait before we return to command mode
     */
    wclear(hw);
    cnt = 0;
    while (strp->h_ch) {
	mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(strp->h_ch));
	waddstr(hw, strp->h_desc);
	strp++;
	if (++cnt >= 46 && strp->h_ch) {
	    wmove(hw, LINES-1, 0);
	    wprintw(hw, morestr);
	    draw(hw);
	    wait_for(hw,' ');
	    wclear(hw);
	    cnt = 0;
	}
    }
#ifdef WIZARD
    if (wizard) {
	while (wizp->h_ch) {
	    mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(wizp->h_ch));
	    waddstr(hw, wizp->h_desc);
	    wizp++;
	    if (++cnt >= 46 && wizp->h_ch) {
		wmove(hw, LINES-1, 0);
		wprintw(hw, morestr);
		draw(hw);
		wait_for(hw,' ');
		wclear(hw);
		cnt = 0;
	    }
	}
    }
#endif
    wmove(hw, LINES-1, 0);
    wprintw(hw, spacemsg);
    draw(hw);
    wait_for(hw,' ');
    wclear(hw);
    draw(hw);
    wmove(cw, 0, 0);
    wclrtoeol(cw);
    status(FALSE);
    touchwin(cw);
}
/*
 * identify:
 *	Tell the player what a certain thing is.
 */

identify()
{
    register char ch;
    const char *str;

    msg("What do you want identified? ");
    ch = readchar();
    mpos = 0;
    if (ch == ESCAPE)
    {
	msg("");
	return;
    }
    if (isalpha(ch))
	str = monsters[id_monst(ch)].m_name;
    else switch(ch)
    {
	case '|':
	case '-':
	    str = (levtype == OUTSIDE) ? "boundary of sector"
				       : "wall of a room";
	when GOLD:	str = "gold";
	when STAIRS :	str = (levtype == OUTSIDE) ? "entrance to a dungeon"
						   : "passage leading down";
	when DOOR:	str = "door";
	when FLOOR:	str = (levtype == OUTSIDE) ? "meadow" : "room floor";
	when VPLAYER:	str = "the hero of the game ---> you";
	when IPLAYER:	str = "you (but invisible)";
	when PASSAGE:	str = "passage";
	when POST:	str = "trading post";
	when POOL:	str = (levtype == OUTSIDE) ? "lake"
						   : "a shimmering pool";
	when TRAPDOOR:	str = "trapdoor";
	when ARROWTRAP:	str = "arrow trap";
	when SLEEPTRAP:	str = "sleeping gas trap";
	when BEARTRAP:	str = "bear trap";
	when TELTRAP:	str = "teleport trap";
	when DARTTRAP:	str = "dart trap";
	when MAZETRAP:	str = "entrance to a maze";
	when FOREST:	str = "forest";
	when POTION:	str = "potion";
	when SCROLL:	str = "scroll";
	when FOOD:	str = "food";
	when WEAPON:	str = "weapon";
	when ' ' :	str = "solid rock";
	when ARMOR:	str = "armor";
	when MM:	str = "miscellaneous magic";
	when RING:	str = "ring";
	when STICK:	str = "wand or staff";
	when SECRETDOOR:str = "secret door";
	when RELIC:	str = "artifact";
	otherwise:	str = "unknown character";
    }
    sprintf(outstring,"'%s' : %s", unctrl(ch), str);
    msg(outstring);
}

/*
 * d_level:
 *	He wants to go down a level
 */

d_level()
{
    bool no_phase=FALSE;


    /* If we are at a top-level trading post, we probably can't go down */
    if (levtype == POSTLEV && level == 0 && rnd(100) < 80) {
	msg("I see no way down.");
	return;
    }

    if (winat(hero.y, hero.x) != STAIRS) {
	if (off(player, CANINWALL) ||	/* Must use stairs if can't phase */
	    (levtype == OUTSIDE && rnd(100) < 90)) {
	    msg("I see no way down.");
	    return;
	}

	/* Is there any dungeon left below? */
	if (level >= nfloors) {
	    msg("There is only solid rock below.");
	    return;
	}

	extinguish(unphase);	/* Using phase to go down gets rid of it */
	no_phase = TRUE;
    }

    /* Is this the bottom? */
    if (level >= nfloors) {
	msg("The stairway only goes up.");
	return;
    }

    level++;
    new_level(NORMLEV);
    if (no_phase) unphase();
}

/*
 * u_level:
 *	He wants to go up a level
 */

u_level()
{
    bool no_phase = FALSE;
    register struct linked_list *item;
    struct thing *tp;
    struct object *obj;

    if (winat(hero.y, hero.x) != STAIRS) {
	if (off(player, CANINWALL)) {	/* Must use stairs if can't phase */
	    msg("I see no way up.");
	    return;
	}

	extinguish(unphase);
	no_phase = TRUE;
    }

    if (level == 0) {
	msg("The stairway only goes down.");
	return;
    }

    /*
     * does he have the item he was quested to get?
     */
    if (level == 1) {
	for (item = pack; item != NULL; item = next(item)) {
	    obj = OBJPTR(item);
	    if (obj->o_type == RELIC && obj->o_which == quest_item)
		total_winner();
	}
    }
    /*
     * check to see if he trapped a UNIQUE, If he did then put it back
     * in the monster table for next time
     */
    for (item = tlist; item != NULL; item = next(item)) {
	tp = THINGPTR(item);
	if (on(*tp, ISUNIQUE)) 
	    monsters[tp->t_index].m_normal = TRUE;
    }
    t_free_list(tlist);	/* Monsters that fell below are long gone! */

    if (levtype != POSTLEV) level--;
    if (level > 0) new_level(NORMLEV);
    else {
	level = -1;	/* Indicate that we are new to the outside */
	msg("You emerge into the %s", daytime ? "light" : "night");
	new_level(OUTSIDE); 	/* Leaving the dungeon */
    }

    if (no_phase) unphase();
}

/*
 * Let him escape for a while
 */

shell()
{
    /*
     * Set the terminal back to original mode
     */
    wclear(hw);
    wmove(hw, LINES-1, 0);
    draw(hw);
    endwin();
    in_shell = TRUE;
    fflush(stdout);
    
    md_shellescape();

    printf(retstr);
    fflush(stdout);
    noecho();
    raw();
    keypad(cw,1);
    in_shell = FALSE;
    wait_for(hw,'\n');
    clearok(cw, TRUE);
    touchwin(cw);
    wmove(cw,0,0);
    draw(cw);
}

/*
 * allow a user to call a potion, scroll, or ring something
 */
call(mark)
bool mark;
{
    register struct object *obj;
    register struct linked_list *item;
    register char **guess = NULL, *elsewise = NULL;
    register bool *know;

    if (mark) item = get_item(pack, "mark", ALL);
    else item = get_item(pack, "call", CALLABLE);
    /*
     * Make certain that it is somethings that we want to wear
     */
    if (item == NULL)
	return;
    obj = OBJPTR(item);
    switch (obj->o_type)
    {
	case RING:
	    guess = r_guess;
	    know = r_know;
	    elsewise = (r_guess[obj->o_which] != NULL ?
			r_guess[obj->o_which] : r_stones[obj->o_which]);
	when POTION:
	    guess = p_guess;
	    know = p_know;
	    elsewise = (p_guess[obj->o_which] != NULL ?
			p_guess[obj->o_which] : p_colors[obj->o_which]);
	when SCROLL:
	    guess = s_guess;
	    know = s_know;
	    elsewise = (s_guess[obj->o_which] != NULL ?
			s_guess[obj->o_which] : s_names[obj->o_which]);
	when STICK:
	    guess = ws_guess;
	    know = ws_know;
	    elsewise = (ws_guess[obj->o_which] != NULL ?
			ws_guess[obj->o_which] : ws_made[obj->o_which]);
	when MM:
	    guess = m_guess;
	    know = m_know;
	    elsewise = (m_guess[obj->o_which] != NULL ?
			m_guess[obj->o_which] : "nothing");
	otherwise:
	    if (!mark) {
		msg("You can't call that anything.");
		return;
	    }
	    else know = (bool *) 0;
    }
    if ((obj->o_flags & ISPOST)	|| (know && know[obj->o_which]) && !mark) {
	msg("That has already been identified.");
	return;
    }
    if (mark) {
	if (obj->o_mark[0]) {
	    addmsg(terse ? "M" : "Was m");
	    msg("arked \"%s\"", obj->o_mark);
	}
	msg(terse ? "Mark it: " : "What do you want to mark it? ");
	prbuf[0] = '\0';
    }
    else {
	addmsg(terse ? "C" : "Was c");
	msg("alled \"%s\"", elsewise);
	msg(terse ? "Call it: " : "What do you want to call it? ");
	if (guess[obj->o_which] != NULL)
	    free(guess[obj->o_which]);
	strcpy(prbuf, elsewise);
    }
    if (get_str(prbuf, cw) == NORM) {
	if (mark) {
	    strncpy(obj->o_mark, prbuf, MARKLEN-1);
	    obj->o_mark[MARKLEN-1] = '\0';
	}
	else {
	    guess[obj->o_which] = new((unsigned int) strlen(prbuf) + 1);
	    strcpy(guess[obj->o_which], prbuf);
	}
    }
}
