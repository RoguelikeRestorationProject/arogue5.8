/*
 * File for the fun ends
 * Death or a total win
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

/* Print flags for scoring */
#define REALLIFE 1	/* Print out machine and logname */
#define EDITSCORE 2	/* Edit the current score file */
#define ADDSCORE 3	/* Add a new score */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "curses.h"
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include "network.h"
#include "rogue.h"
#include "mach_dep.h"

/*
 * If you change this structure, change the compatibility routines
 * scoreout() and scorein() to reflect the change.  Also update SCORELEN.
 */

struct sc_ent {
    unsigned long	sc_score;
    char	sc_name[LINELEN];
    char	sc_system[SYSLEN];
    char	sc_login[LOGLEN];
    short	sc_flgs;
    short	sc_level;
    short	sc_ctype;
    short	sc_monster;
    short	sc_quest;
};

#define SCORELEN \
    (sizeof(unsigned long) + LINELEN + SYSLEN + LOGLEN + 5*sizeof(short))

static char *rip[] = {
"                       __________",
"                      /          \\",
"                     /    REST    \\",
"                    /      IN      \\",
"                   /     PEACE      \\",
"                  /                  \\",
"                  |                  |",
"                  |                  |",
"                  |    killed by     |",
"                  |                  |",
"                  |       1984       |",
"                 *|     *  *  *      | *",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______",
    0
};

char	*killname();





void
byebye(sig)
int sig;
{
    NOOP(sig);
    if (!isendwin()) {
        clear();
	endwin();
    }
    printf("\n");
    exit(0);
}


/*
 * death:
 *	Do something really fun when he dies
 */

death(monst)
register short monst;
{
    register char **dp = rip, *killer;
    register struct tm *lt;
    time_t date;
    char buf[80];
    struct tm *localtime();

    time(&date);
    lt = localtime(&date);
    clear();
    move(8, 0);
    while (*dp)
	printw("%s\n", *dp++);
    mvaddstr(14, 28-((strlen(whoami)+1)/2), whoami);
    sprintf(buf, "%lu Points", pstats.s_exp );
    mvaddstr(15, 28-((strlen(buf)+1)/2), buf);
    killer = killname(monst);
    mvaddstr(17, 28-((strlen(killer)+1)/2), killer);
    mvaddstr(18, 26, (sprintf(prbuf, "%4d", 1900+lt->tm_year), prbuf));
    move(LINES-1, 0);
    refresh();
    score(pstats.s_exp, KILLED, monst);
    exit(0);
}

char *
killname(monst)
register short monst;
{
    static char mons_name[LINELEN];
    int i;

    if (monst > NUMMONST) return("a strange monster");

    if (monst >= 0) {
	switch (monsters[monst].m_name[0]) {
	    case 'a':
	    case 'e':
	    case 'i':
	    case 'o':
	    case 'u':
		sprintf(mons_name, "an %s", monsters[monst].m_name);
		break;
	    default:
		sprintf(mons_name, "a %s", monsters[monst].m_name);
	}
	return(mons_name);
    }
    for (i = 0; i< DEATHNUM; i++) {
	if (deaths[i].reason == monst)
	    break;
    }
    if (i >= DEATHNUM)
	return ("strange death");
    return (deaths[i].name);
}


/*
 * score -- figure score and post it.
 */

/* VARARGS2 */
score(amount, flags, monst)
unsigned long amount;
short monst;
{
    static struct sc_ent top_ten[NUMSCORE];
    register struct sc_ent *scp;
    register int i;
    register struct sc_ent *sc2;
    register FILE *outf;
    register char *killer;
    register int prflags = 0;
    register int fd;
    short upquest = 0, wintype = 0, uplevel = 0, uptype = 0;	/* For network updating */
    char upsystem[SYSLEN], uplogin[LOGLEN];
    char *thissys;	/* Holds the name of this system */
    char *compatstr=NULL; /* Holds scores for writing compatible score files */
#define REASONLEN 3
    static char *reason[] = {
	"killed",
	"quit",
	"A total winner",
	"somehow left",
    };
    char *packend;
    memset(top_ten,0,sizeof(top_ten));
    signal(SIGINT, byebye);
    if (flags != WINNER && flags != SCOREIT && flags != UPDATE) {
	if (flags == CHICKEN)
	    packend = "when you quit";
	else
	    packend = "at your untimely demise";
	mvaddstr(LINES - 1, 0, retstr);
	refresh();
	wgetnstr(cw,prbuf,80);
	showpack(packend);
    }
    purse = 0;	/* Steal all the gold */

    /*
     * Open file and read list
     */

    if ((fd = open(score_file, O_RDWR | O_CREAT, 0666)) < 0) 
    {
       printf("\nCannot open score_file.\n");
       return;
    }
    outf = (FILE *) fdopen(fd, "w");

    /* Get this system's name */
    thissys = md_gethostname();

    for (scp = top_ten; scp <= &top_ten[NUMSCORE-1]; scp++)
    {
	scp->sc_score = 0L;
	for (i = 0; i < 80; i++)
	    scp->sc_name[i] = rnd(255);
	scp->sc_quest= RN;
	scp->sc_flgs = RN;
	scp->sc_level = RN;
	scp->sc_monster = RN;
	scp->sc_ctype = 0;
	strncpy(scp->sc_system, thissys, SYSLEN);
	scp->sc_login[0] = '\0';
    }

    /*
     * If this is a SCOREIT optin (rogue -s), don't call byebye.  The
     * endwin() call in byebye() will result in a core dump.
     */
    if (flags == SCOREIT) signal(SIGINT, SIG_DFL);
    else signal(SIGINT, byebye);

    if (flags != SCOREIT && flags != UPDATE)
    {
	mvaddstr(LINES - 1, 0, retstr);
	refresh();
	fflush(stdout);
	wgetnstr(stdscr,prbuf,80);
    }

    /* Check for special options */
    if (strcmp(prbuf, "names") == 0)
	prflags = REALLIFE;
#ifdef WIZARD
    else if (wizard) {
	if (strcmp(prbuf, "edit") == 0) prflags = EDITSCORE;
	else if (strcmp(prbuf, "add") == 0) {
	    prflags = ADDSCORE;
	    waswizard = FALSE;	/* We want the new score recorded */
	}
    }
#endif

    /* Read the score and convert it to a compatible format */
    scorein(top_ten, fd);	/* Convert it */

    /* Get some values if this is an update */
    if (flags == UPDATE) {
	int errcheck, errors = 0;

	upquest = (short) netread(&errcheck, sizeof(short), stdin);
	if (errcheck) errors++;

	if (fread(whoami, 1, LINELEN, stdin) != LINELEN) errors++;

	wintype = (short) netread(&errcheck, sizeof(short), stdin);
	if (errcheck) errors++;

	uplevel = (short) netread(&errcheck, sizeof(short), stdin);
	if (errcheck) errors++;

	uptype = (short) netread(&errcheck, sizeof(short), stdin);
	if (errcheck) errors++;

	if (fread(upsystem, 1, SYSLEN, stdin) != SYSLEN)
		errors++;
	if (fread(uplogin, 1, LOGLEN, stdin) != LOGLEN)
		errors++;
	
	if (errors) {
	    fclose(outf);
	    free(compatstr);
	    return;
	}
    }

    /*
     * Insert player in list if need be
     */
    if (!waswizard) {
	char *login = NULL;

	if (flags != UPDATE) {
	    login = md_getusername();
	}

	if (flags == UPDATE)
	    (void) update(top_ten, amount, upquest, whoami, wintype,
		   uplevel, monst, uptype, upsystem, uplogin);
	else {
#ifdef WIZARD
	    if (prflags == ADDSCORE) {	/* Overlay characteristic by new ones */
	 	char buffer[80];
		int atoi();

		clear();
		mvaddstr(1, 0, "Score: ");
		mvaddstr(2, 0, "Quest (number): ");
		mvaddstr(3, 0, "Name: ");
		mvaddstr(4, 0, "System: ");
		mvaddstr(5, 0, "Login: ");
		mvaddstr(6, 0, "Level: ");
		mvaddstr(7, 0, "Char type: ");
		mvaddstr(8, 0, "Result: ");

		/* Get the score */
		move(1, 7);
		get_str(buffer, stdscr);
		amount = atol(buffer);

		/* Get the character's quest -- must be a number */
		move(2, 16);
		get_str(buffer, stdscr);
		quest_item = atoi(buffer);

		/* Get the character's name */
		move(3, 6);
		get_str(buffer, stdscr);
		strncpy(whoami, buffer, LINELEN);

		/* Get the system */
		move(4, 8);
		get_str(buffer, stdscr);
		strncpy(thissys, buffer, SYSLEN);

		/* Get the login */
		move(5, 7);
		get_str(buffer, stdscr);
		strncpy(login, buffer, LOGLEN);

		/* Get the level */
		move(6, 7);
		get_str(buffer, stdscr);
		level = max_level = (short) atoi(buffer);

		/* Get the character type */
		move(7, 11);
		get_str(buffer, stdscr);
		switch (buffer[0]) {
		    case 'F':
		    case 'f':
		    default:
			player.t_ctype = C_FIGHTER;
			break;

		    case 'C':
		    case 'c':
			player.t_ctype = C_CLERIC;
			break;

		    case 'M':
		    case 'm':
			player.t_ctype = C_MAGICIAN;
			break;

		    case 'T':
		    case 't':
			player.t_ctype = C_THIEF;
			break;
		}

		/* Get the win type */
		move(8, 8);
		get_str(buffer, stdscr);
		switch (buffer[0]) {
		    case 'W':
		    case 'w':
		    case 'T':
		    case 't':
			flags = WINNER;
			break;

		    case 'Q':
		    case 'q':
			flags = CHICKEN;
			break;

		    case 'k':
		    case 'K':
		    default:
			flags = KILLED;
			break;
		}

		/* Get the monster if player was killed */
		if (flags == KILLED) {
		    mvaddstr(9, 0, "Death type: ");
		    get_str(buffer, stdscr);
		    if (buffer[0] == 'M' || buffer[0] == 'm')
			monst = makemonster(FALSE);
		    else monst = getdeath();
		}
	    }
#endif

	    if (update(top_ten, amount, (short) quest_item, whoami, flags,
		    (flags == WINNER) ? (short) max_level : (short) level,
		    monst, player.t_ctype, thissys, login)
#ifdef NUMNET
		&& fork() == 0		/* Spin off network process */
#endif
		) {
#ifdef NUMNET
		/* Send this update to the other systems in the network */
		int i, j;
		char cmd[256];	/* Command for remote execution */
		FILE *rmf, *popen();	/* For input to remote command */

		for (i=0; i<NUMNET; i++)
		    if (Network[i].system[0] != '!' &&
			strcmp(Network[i].system, thissys)) {
			sprintf(cmd,
			   "usend -s -d%s -uNoLogin -!'%s -u' - 2>/dev/null",
				Network[i].system, Network[i].rogue);

			/* Execute the command */
			if ((rmf=popen(cmd, "w")) != NULL) {
			    unsigned long temp;	/* Temporary value */

			    /* Write out the parameters */
			    (void) netwrite((unsigned long) amount,
					  sizeof(unsigned long), rmf);

			    (void) netwrite((unsigned long) monst,
					  sizeof(short), rmf);

			    (void) netwrite((unsigned long) quest_item,
					sizeof(short), rmf);

			    (void) fwrite(whoami, 1, strlen(whoami), rmf);
			    for (j=strlen(whoami); j<LINELEN; j++)
				putc('\0', rmf);

			    (void) netwrite((unsigned long) flags,
					  sizeof(short), rmf);

			    temp = (unsigned long)
				(flags==WINNER ? max_level : level);
			    (void) netwrite(temp, sizeof(short), rmf);

			    (void) netwrite((unsigned long) player.t_ctype,
					  sizeof(short), rmf);

			    (void) fwrite(thissys, 1,
						strlen(thissys), rmf);
			    for (j=strlen(thissys); j<SYSLEN; j++)
				putc('\0', rmf);

			    (void) fwrite(login, 1, strlen(login), rmf);
			    for (j=strlen(login); j<LOGLEN; j++)
				putc('\0', rmf);

			    /* Close off the command */
			    (void) pclose(rmf);
			}
		    }
		    _exit(0);	/* Exit network process */
#endif
	    }
	}
    }

    /*
     * SCOREIT -- rogue -s option.  Never started curses if this option.
     * UPDATE -- network scoring update.  Never started curses if this option.
     * EDITSCORE -- want to delete or change a score.
     */
/*   if (flags != SCOREIT && flags != UPDATE && prflags != EDITSCORE)
	endwin();	*/

    if (flags != UPDATE) {
	if (flags != SCOREIT) {
	    clear();
	    refresh();
	    endwin();
	}
	/*
	* Print the list
	*/
	printf("\nTop %d Adventurers:\nRank     Score\tName\n",
		NUMSCORE);

	for (scp = top_ten; scp <= &top_ten[NUMSCORE-1]; scp++) {
	    char *class;

	    if (scp->sc_score != 0) {
		switch (scp->sc_ctype) {
		    case C_FIGHTER:	class = "fighter";
		    when C_MAGICIAN:	class = "magician";
		    when C_CLERIC:	class = "cleric";
		    when C_THIEF:	class = "thief";
		    otherwise:		class = "unknown";
		}

		/* Make sure we have an in-bound reason */
		if (scp->sc_flgs > REASONLEN) scp->sc_flgs = REASONLEN;

		printf("%3d %10lu\t%s (%s)", scp - top_ten + 1,
		    scp->sc_score, scp->sc_name, class);
   
		if (prflags == REALLIFE) printf(" [in real life %.*s!%.*s]",
				SYSLEN, scp->sc_system, LOGLEN, scp->sc_login);
		printf(":\n\t\t%s on level %d", reason[scp->sc_flgs],
			    scp->sc_level);

		switch (scp->sc_flgs) {
		    case KILLED:
			printf(" by");
			killer = killname(scp->sc_monster);
			printf(" %s", killer);
			break;

		    case WINNER:
			printf(" with the %s",
				rel_magic[scp->sc_quest].mi_name);
			break;
		}

		if (prflags == EDITSCORE)
		{
		    fflush(stdout);
		    fgets(prbuf,80,stdin);
		    printf("\n");
		    if (prbuf[0] == 'd') {
			for (sc2 = scp; sc2 < &top_ten[NUMSCORE-1]; sc2++)
			    *sc2 = *(sc2 + 1);
			top_ten[NUMSCORE-1].sc_score = 0;
			for (i = 0; i < 80; i++)
			    top_ten[NUMSCORE-1].sc_name[i] = rnd(255);
			top_ten[NUMSCORE-1].sc_flgs = RN;
			    top_ten[NUMSCORE-1].sc_level = RN;
			    top_ten[NUMSCORE-1].sc_monster = RN;
			    scp--;
		    }
		    else if (prbuf[0] == 'e') {
			printf("Death type: ");
			fgets(prbuf,80,stdin);
			if (prbuf[0] == 'M' || prbuf[0] == 'm')
			    scp->sc_monster = makemonster(FALSE);
			else scp->sc_monster = getdeath();
			clear();
			refresh();
		    }
		}
		else printf("\n");
	    }
	}
        if ((flags != SCOREIT) && (flags != UPDATE)) {
            printf("\n[Press return to exit]");
            fflush(stdout);
            fgets(prbuf,80,stdin);
        }
/*	if (prflags == EDITSCORE) endwin(); */	/* End editing windowing */
    }
    fseek(outf, 0L, 0);
    /*
     * Update the list file
     */
    scoreout(top_ten, outf);
    fclose(outf);
}

/*
 * scorein:
 *	Convert a character string that has been translated from a
 * score file by scoreout() back to a score file structure.
 */
scorein(scores, fd)
struct sc_ent scores[];
int fd;
{
    int i;
    char scoreline[100];

    for(i = 0; i < NUMSCORE; i++)
    {
        encread((char *) &scores[i].sc_name, LINELEN, fd);
        encread((char *) &scores[i].sc_system, SYSLEN, fd);
        encread((char *) &scores[i].sc_login, LINELEN, fd);
        encread((char *) scoreline, 100, fd);
        sscanf(scoreline, " %lu %d %d %d %d %d \n",
        &scores[i].sc_score,   &scores[i].sc_flgs,
        &scores[i].sc_level,   &scores[i].sc_ctype,
        &scores[i].sc_monster, &scores[i].sc_quest);
    }
}

/*
 * scoreout:
 *	Convert a score file structure to a character string.  We do
 * this for compatibility sake since some machines write out fields in
 * different orders.
 */
scoreout(scores, outf)
struct sc_ent scores[];
FILE *outf;
{
    int i;
    char scoreline[100];

    for(i = 0; i < NUMSCORE; i++)  {
        memset(scoreline,0,100); 
        encwrite((char *) scores[i].sc_name, LINELEN, outf); 
        encwrite((char *) scores[i].sc_system, SYSLEN, outf); 
        encwrite((char *) scores[i].sc_login, LINELEN, outf); 
        sprintf(scoreline, " %lu %d %d %d %d %d \n", 
            scores[i].sc_score,  scores[i].sc_flgs, 
            scores[i].sc_level,  scores[i].sc_ctype, 
            scores[i].sc_monster,scores[i].sc_quest);
        encwrite((char *) scoreline, 100, outf); 
    } 
}

/*
 * showpack:
 *	Display the contents of the hero's pack
 */
showpack(howso)
char *howso;
{
	reg char *iname;
	reg int cnt, packnum;
	reg struct linked_list *item;
	reg struct object *obj;

	idenpack();
	cnt = 1;
	clear();
	mvprintw(0, 0, "Contents of your pack %s:\n",howso);
	packnum = 'a';
	for (item = pack; item != NULL; item = next(item)) {
		obj = OBJPTR(item);
		iname = inv_name(obj, FALSE);
		mvprintw(cnt, 0, "%c) %s\n",packnum++,iname);
		if (++cnt >= LINES - 2 && 
		    next(item) != NULL) {
			cnt = 1;
			mvaddstr(LINES - 1, 0, morestr);
			refresh();
			wait_for(stdscr,' ');
			clear();
		}
	}
	mvprintw(cnt + 1,0,"--- %d  Gold Pieces ---",purse);
	refresh();
}

total_winner()
{
    register struct linked_list *item;
    register struct object *obj;
    register int worth;
    register char c;
    register int oldpurse;

    clear();
    standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    addstr("     Congratulations, you have made it to the light of day!    \n");
    standend();
    addstr("\nYou have joined the elite ranks of those who have escaped the\n");
    addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
    addstr("a great profit and are appointed leader of a ");
    switch (player.t_ctype) {
	case C_MAGICIAN:addstr("magic user's guild.\n");
	when C_FIGHTER:	addstr("fighters guild.\n");
	when C_CLERIC:	addstr("monastery.\n");
	when C_THIEF:	addstr("thief's guild.\n");
	otherwise:	addstr("tavern.\n");
    }
    mvaddstr(LINES - 1, 0, spacemsg);
    refresh();
    wait_for(stdscr,' ');
    clear();
    mvaddstr(0, 0, "   Worth  Item");
    oldpurse = purse;
    for (c = 'a', item = pack; item != NULL; c++, item = next(item))
    {
	obj = OBJPTR(item);
	worth = get_worth(obj);
	if (obj->o_group == 0)
	    worth *= obj->o_count;
	whatis(item);
	mvprintw(c - 'a' + 1, 0, "%c) %6d  %s", c, worth, inv_name(obj, FALSE));
	purse += worth;
    }
    mvprintw(c - 'a' + 1, 0,"   %5d  Gold Pieces          ", oldpurse);
    refresh();
    score(pstats.s_exp + (long) purse, WINNER, '\0');
    exit(0);
}

update(top_ten, amount, quest, whoami, flags, level, monst, ctype, system, login)
struct sc_ent top_ten[];
unsigned long amount;
short quest, flags, level, monst, ctype;
char *whoami, *system, *login;
{
    register struct sc_ent *scp, *sc2;
    int retval=0;	/* 1 if a change, 0 otherwise */

    for (scp = top_ten; scp < &top_ten[NUMSCORE]; scp++) {
	if (amount >= scp->sc_score)
	    break;

#ifdef LIMITSCORE	/* Limits player to one entry per class per uid */
	/* If this good score is the same class and uid, then forget it */
	if (strncmp(scp->sc_login, login, LOGLEN) == 0 &&
	    scp->sc_ctype == ctype &&
	    strncmp(scp->sc_system, system, SYSLEN) == 0) return(0);
#endif
    }

    if (scp < &top_ten[NUMSCORE])
    {
	retval = 1;

#ifdef LIMITSCORE	/* Limits player to one entry per class per uid */
    /* If a lower scores exists for the same login and class, delete it */
    for (sc2 = scp ;sc2 < &top_ten[NUMSCORE]; sc2++) {
	if (sc2->sc_score == 0L) break;	/* End of useful scores */

	if (strncmp(sc2->sc_login, login, LOGLEN) == 0 &&
	    sc2->sc_ctype == ctype &&
	    strncmp(sc2->sc_system, system, SYSLEN) == 0) {
	    /* We want to delete this entry */
	    while (sc2 < &top_ten[NUMSCORE-1]) {
		*sc2 = *(sc2+1);
		sc2++;
	    }
	    sc2->sc_score = 0L;
	}
    }
#endif

	for (sc2 = &top_ten[NUMSCORE-1]; sc2 > scp; sc2--)
	    *sc2 = *(sc2-1);
	scp->sc_score = amount;
	scp->sc_quest = quest;
	strncpy(scp->sc_name, whoami, LINELEN);
	scp->sc_flgs = flags;
	scp->sc_level = level;
	scp->sc_monster = monst;
	scp->sc_ctype = ctype;
	strncpy(scp->sc_system, system, SYSLEN);
	strncpy(scp->sc_login, login, LOGLEN);
    }

    return(retval);
}
