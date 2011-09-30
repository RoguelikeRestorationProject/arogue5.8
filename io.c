/*
 * Various input/output functions
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
#include <ctype.h>
#include <stdarg.h>
#include "rogue.h"

/*
 * msg:
 *	Display a message at the top of the screen.
 */

static char msgbuf[BUFSIZ];
static int newpos = 0;

/*VARARGS1*/
msg(char *fmt, ...)
{
    va_list ap;
    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
	overwrite(cw,msgw);
	wmove(msgw, 0, 0);
	clearok(msgw, FALSE);
	draw(msgw);
	mpos = 0;
	return;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(ap,fmt);
    doadd(fmt, ap);
    va_end(ap);
    endmsg();
}

/*
 * add things to the current message
 */
addmsg(char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    doadd(fmt, ap);
    va_end(ap);
}

/*
 * Display a new msg (giving him a chance to see the previous one if it
 * is up there with the --More--)
 */
endmsg()
{
    strncpy(huh, msgbuf, sizeof(huh));

    huh[ sizeof(huh) - 1 ] = 0;

    if (mpos)
    {
	wmove(msgw, 0, mpos);
	waddstr(msgw, morestr);
	draw(cw);
	draw(msgw);
	wait_for(msgw,' ');
	overwrite(cw,msgw);
	wmove(msgw,0,0);
	touchwin(cw);
    }
    else {
	overwrite(cw,msgw);
	wmove(msgw,0,0);
    }
    waddstr(msgw, msgbuf);
    mpos = newpos;
    newpos = 0;
    msgbuf[0] = '\0';
    draw(cw);
    clearok(msgw, FALSE);
    draw(msgw);
}

doadd(char *fmt, va_list ap)
{
    /*
     * Do the sprintf into newmsg and append to msgbuf
     */

    vsnprintf(&msgbuf[newpos], sizeof(msgbuf)-newpos-1, fmt, ap);

    newpos = strlen(msgbuf);
}

/*
 * step_ok:
 *	returns true if it is ok for type to step on ch
 *	flgptr will be NULL if we don't know what the monster is yet!
 */

step_ok(y, x, can_on_monst, flgptr)
register int y, x, can_on_monst;
register struct thing *flgptr;
{
    /* can_on_monst = MONSTOK if all we care about are physical obstacles */
    register struct linked_list *item;
    char ch;

    /* What is here?  Don't check monster window if MONSTOK is set */
    if (can_on_monst == MONSTOK) ch = CCHAR( mvinch(y, x) );
    else ch = CCHAR( winat(y, x) );

    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	case SECRETDOOR:
	    if (flgptr && on(*flgptr, CANINWALL)) return(TRUE);
	    return FALSE;
	when SCROLL:
	    if (can_on_monst == MONSTOK) return(TRUE); /* Not a real obstacle */
	    /*
	     * If it is a scroll, it might be a scare monster scroll
	     * so we need to look it up to see what type it is.
	     */
	    if (flgptr && flgptr->t_ctype == C_MONSTER) {
		item = find_obj(y, x);
		if (item != NULL &&
		    (OBJPTR(item))->o_which==S_SCARE &&
		    (flgptr == NULL || flgptr->t_stats.s_intel < 16))
			return(FALSE); /* All but smart ones are scared */
	    }
	    return(TRUE);
	otherwise:
	    return (!isalpha(ch));
    }
}
/*
 * shoot_ok:
 *	returns true if it is ok for type to shoot over ch
 */

shoot_ok(ch)
{
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	case SECRETDOOR:
	case FOREST:
	    return FALSE;
	default:
	    return (!isalpha(ch));
    }
}

/*
 * readchar:
 *	flushes stdout so that screen is up to date and then returns
 *	getchar.
 */

readchar()
{
    int ch;

    ch = md_readchar(cw);

    if ((ch == 3) || (ch == 0))
    {
        quit(0);
        return(27);
    }

    return(ch);
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */

status(display)
bool display;	/* is TRUE, display unconditionally */
{
    register struct stats *stat_ptr, *max_ptr;
    register int oy = 0, ox = 0, temp;
    register char *pb;
    static char buf[LINELEN];
    static int hpwidth = 0, s_hungry = -1;
    static int s_lvl = -1, s_pur, s_hp = -1, s_str, maxs_str, 
		s_ac = 0;
    static short s_intel, s_dext, s_wisdom, s_const, s_charisma;
    static short maxs_intel, maxs_dext, maxs_wisdom, maxs_const, maxs_charisma;
    static unsigned long s_exp = 0;
    static int s_carry, s_pack;
    bool first_line=FALSE;

    /* Use a mini status version if we have a small window */
    if (COLS < 80) {
	ministat();
	return;
    }

    stat_ptr = &pstats;
    max_ptr  = &max_stats;

    /*
     * If nothing has changed in the first line, then skip it
     */
    if (!display				&&
	s_lvl == level				&& 
	s_intel == stat_ptr->s_intel		&&
	s_wisdom == stat_ptr->s_wisdom		&&
	s_dext == dex_compute()			&& 
	s_const == stat_ptr->s_const		&&
	s_charisma == stat_ptr->s_charisma	&&
	s_str == str_compute()			&& 
	s_pack == stat_ptr->s_pack		&&
	s_carry == stat_ptr->s_carry		&&
	maxs_intel == max_ptr->s_intel		&& 
	maxs_wisdom == max_ptr->s_wisdom	&&
	maxs_dext == max_ptr->s_dext		&& 
	maxs_const == max_ptr->s_const		&&
	maxs_charisma == max_ptr->s_charisma	&&
	maxs_str == max_ptr->s_str		) goto line_two;

    /* Display the first line */
    first_line = TRUE;
    getyx(cw, oy, ox);
    sprintf(buf, "Int:%d(%d)  Str:%d", stat_ptr->s_intel,
    	max_ptr->s_intel, str_compute());

    /* Maximum strength */
    pb = &buf[strlen(buf)];
    sprintf(pb, "(%d)", max_ptr->s_str);

    pb = &buf[strlen(buf)];
    sprintf(pb, "  Wis:%d(%d)  Dxt:%d(%d)  Const:%d(%d)  Carry:%d(%d)",
	stat_ptr->s_wisdom,max_ptr->s_wisdom,dex_compute(),max_ptr->s_dext,
	stat_ptr->s_const,max_ptr->s_const,stat_ptr->s_pack/10,
	stat_ptr->s_carry/10);

    /* Update first line status */
    s_intel = stat_ptr->s_intel;
    s_wisdom = stat_ptr->s_wisdom;
    s_dext = dex_compute();
    s_const = stat_ptr->s_const;
    s_charisma = stat_ptr->s_charisma;
    s_str = str_compute();
    s_pack = stat_ptr->s_pack;
    s_carry = stat_ptr->s_carry;
    maxs_intel = max_ptr->s_intel;
    maxs_wisdom = max_ptr->s_wisdom;
    maxs_dext = max_ptr->s_dext;
    maxs_const = max_ptr->s_const;
    maxs_charisma = max_ptr->s_charisma;
    maxs_str = max_ptr->s_str;

    /* Print the line */
    mvwaddstr(cw, LINES - 2, 0, buf);
    wclrtoeol(cw);

    /*
     * If nothing has changed since the last status, don't
     * bother.
     */
line_two: 
    if (!display					&&
	s_hp == stat_ptr->s_hpt				&& 
	s_exp == stat_ptr->s_exp			&& 
        s_pur == purse					&& 
	s_ac == ac_compute() - dext_prot(s_dext)	&&
	s_lvl == level 					&& 
	s_hungry == hungry_state			) return;
	
    if (!first_line) getyx(cw, oy, ox);
    if (s_hp != max_ptr->s_hpt)
    {
	temp = s_hp = max_ptr->s_hpt;
	for (hpwidth = 0; temp; hpwidth++)
	    temp /= 10;
    }
    sprintf(buf, "Lvl:%d  Au:%d  Hp:%*d(%*d)  Ac:%d  Exp:%d/%lu  %s",
	level, purse, hpwidth, stat_ptr->s_hpt, hpwidth, max_ptr->s_hpt,
	ac_compute() - dext_prot(s_dext),stat_ptr->s_lvl, stat_ptr->s_exp, 
	cnames[player.t_ctype][min(stat_ptr->s_lvl-1, 10)]);

    /*
     * Save old status
     */
    s_lvl = level;
    s_pur = purse;
    s_hp = stat_ptr->s_hpt;
    s_exp = stat_ptr->s_exp; 
    s_ac = ac_compute() - dext_prot(s_dext);
    mvwaddstr(cw, LINES - 1, 0, buf);
    switch (hungry_state)
    {
	case F_OKAY: ;
	when F_HUNGRY:
	    waddstr(cw, "  Hungry");
	when F_WEAK:
	    waddstr(cw, "  Weak");
	when F_FAINT:
	    waddstr(cw, "  Fainting");
    }
    wclrtoeol(cw);
    s_hungry = hungry_state;
    wmove(cw, oy, ox);
}

ministat()
{
    register int oy, ox, temp;
    static char buf[LINELEN];
    static int hpwidth = 0;
    static int s_lvl = -1, s_pur, s_hp = -1;

    /*
     * If nothing has changed since the last status, don't
     * bother.
     */
    if (s_hp == pstats.s_hpt && s_pur == purse && s_lvl == level)
	    return;
	
    getyx(cw, oy, ox);
    if (s_hp != max_stats.s_hpt)
    {
	temp = s_hp = max_stats.s_hpt;
	for (hpwidth = 0; temp; hpwidth++)
	    temp /= 10;
    }
    sprintf(buf, "Lv: %d  Au: %-5d  Hp: %*d(%*d)",
	level, purse, hpwidth, pstats.s_hpt, hpwidth, max_stats.s_hpt);

    /*
     * Save old status
     */
    s_lvl = level;
    s_pur = purse;
    s_hp = pstats.s_hpt;
    mvwaddstr(cw, LINES - 1, 0, buf);
    wclrtoeol(cw);
    wmove(cw, oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */

wait_for(win,ch)
WINDOW *win;
register char ch;
{
    register char c;

    if (ch == '\n')
        while ((c = wgetch(win)) != '\n' && c != '\r')
	    continue;
    else
        while (wgetch(win) != ch)
	    continue;
}

/*
 * show_win:
 *	function used to display a window and wait before returning
 */

show_win(scr, message)
register WINDOW *scr;
char *message;
{
    mvwaddstr(scr, 0, 0, message);
    touchwin(scr);
    wmove(scr, hero.y, hero.x);
    draw(scr);
    wait_for(scr,' ');
    clearok(cw, TRUE);
    touchwin(cw);
}

/*
 * dbotline:
 *	Displays message on bottom line and waits for a space to return
 */
dbotline(scr,message)
WINDOW *scr;
char *message;
{
	mvwaddstr(scr,LINES-1,0,message);
	draw(scr);
	wait_for(scr,' ');	
}


/*
 * restscr:
 *	Restores the screen to the terminal
 */
restscr(scr)
WINDOW *scr;
{
	clearok(scr,TRUE);
	touchwin(scr);
}

/*
 * netread:
 *	Read a byte, short, or long machine independently
 *	Always returns the value as an unsigned long.
 */

unsigned long
netread(error, size, stream)
int *error;
int size;
FILE *stream;
{
    unsigned long result = 0L,	/* What we read in */
		  partial;	/* Partial value */
    int nextc,	/* The next byte */
	i;	/* To index through the result a byte at a time */

    /* Be sure we have a right sized chunk */
    if (size < 1 || size > 4) {
	*error = 1;
	return(0L);
    }

    for (i=0; i<size; i++) {
	nextc = getc(stream);
	if (nextc == EOF) {
	    *error = 1;
	    return(0L);
	}
	else {
	    partial = (unsigned long) (nextc & 0xff);
	    partial <<= 8*i;
	    result |= partial;
	}
    }

    *error = 0;
    return(result);
}



/*
 * netwrite:
 *	Write out a byte, short, or long machine independently.
 */

netwrite(value, size, stream)
unsigned long value;	/* What to write */
int size;	/* How much to write out */
FILE *stream;	/* Where to write it */
{
    int i;	/* Goes through value one byte at a time */
    char outc;	/* The next character to be written */

    /* Be sure we have a right sized chunk */
    if (size < 1 || size > 4) return(0);

    for (i=0; i<size; i++) {
	outc = (char) ((value >> (8 * i)) & 0xff);
	putc(outc, stream);
    }
    return(size);
}
