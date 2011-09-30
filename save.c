/*
 * save and restore routines
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
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "rogue.h"

typedef struct stat STAT;

extern char version[], encstr[];
/* extern bool _endwin; */

STAT sbuf;

bool
save_game()
{
    register FILE *savef;
    register int c;
    char buf[LINELEN];

    /*
     * get file name
     */
    mpos = 0;
    if (file_name[0] != '\0')
    {
	msg("Save file (%s)? ", file_name);
	do
	{
	    c = readchar();
	    if (c == ESCAPE) return(0);
	} while (c != 'n' && c != 'N' && c != 'y' && c != 'Y');
	mpos = 0;
	if (c == 'y' || c == 'Y')
	{
	    msg("File name: %s", file_name);
	    goto gotfile;
	}
    }

    do
    {
	msg("File name: ");
	mpos = 0;
	buf[0] = '\0';
	if (get_str(buf, cw) == QUIT)
	{
	    msg("");
	    return FALSE;
	}
        msg("");
	strcpy(file_name, buf);
gotfile:
	if ((savef = fopen(file_name, "w")) == NULL)
	    msg(strerror(errno));	/* fake perror() */
    } while (savef == NULL);

    /*
     * write out encrpyted file (after a stat)
     * The fwrite is to force allocation of the buffer before the write
     */
    if (save_file(savef) != 0) {
	msg("Cannot create save file.");
	unlink(file_name);
	return(FALSE);
    }
    else return(TRUE);
}

/*
 * automatically save a file.  This is used if a HUP signal is
 * recieved
 */
void
auto_save(int sig)
{
    register FILE *savef;
    register int i;

    NOOP(sig);

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (file_name[0] != '\0'	&& 
	pstats.s_hpt > 0	&&
	(savef = fopen(file_name, "w")) != NULL)
	save_file(savef);
    exit(1);
}

/*
 * write the saved game on the file
 */
bool
save_file(savef)
register FILE *savef;
{
    int ret;
    int slines = LINES;
    int scols  = COLS;

    wmove(cw, LINES-1, 0);
    draw(cw);
    fwrite("junk", 1, 5, savef);
    fseek(savef, 0L, 0);
    /* _endwin = TRUE; */
    fstat(fileno(savef), &sbuf);

    encwrite(version,strlen(version)+1,savef);
    sprintf(prbuf,"%d x %d\n", LINES, COLS);
    encwrite(prbuf,80,savef);

    msg("");
    ret = rs_save_file(savef);

    fclose(savef);
 
    return(ret);
}

restore(file, envp)
register char *file;
char **envp;
{
    register int inf;
#ifndef _AIX
    extern char **environ;
#endif
    char buf[LINELEN];
    STAT sbuf2;
    int oldcol, oldline;	/* Old number of columns and lines */

    if (strcmp(file, "-r") == 0)
	file = file_name;

    if ((inf = open(file, O_RDONLY)) < 0)
    {
	perror(file);
	return FALSE;
    }

    fflush(stdout);

    encread(buf, strlen(version) + 1, inf);

    if (strcmp(buf, version) != 0)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }

    /*
     * Get the lines and columns from the previous game
     */

    encread(buf, 80, inf);
    sscanf(buf, "%d x %d\n", &oldline, &oldcol);
    fstat(inf, &sbuf2);
    fflush(stdout);

    /*
     * Set the new terminal and make sure we aren't going to a smaller screen.
     */

    initscr();
   
    if (COLS < oldcol || LINES < oldline) {
        endwin();
	printf("\nCannot restart the game on a smaller screen.\n");
	return FALSE;
    }

    cw = newwin(LINES, COLS, 0, 0);
    mw = newwin(LINES, COLS, 0, 0);
    hw = newwin(LINES, COLS, 0, 0);
    msgw = newwin(4, COLS, 0, 0);
    keypad(cw,1);
    keypad(msgw,1);

    mpos = 0;
    mvwprintw(cw, 0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime));

    /*
     * defeat multiple restarting from the same place
     */
    if (!wizard) {
	if (sbuf2.st_nlink != 1) {
            endwin();
	    printf("\nCannot restore from a linked file\n");
	    return FALSE;
	}
    }

    if (rs_restore_file(inf) != 0)
    {
        endwin();
        printf("\nCannot restore file\n");
        return(FALSE);
    }

    if (!wizard)
    {
	if (unlink(file) < 0) {
            close(inf); /* only close if system insists */
            if (unlink(file) < 0) {
                endwin();
	        printf("\nCannot unlink file\n");
	        return FALSE;
            }
	}
    }

    environ = envp;
    strcpy(file_name, file);
    setup();
    clearok(curscr, TRUE);
    touchwin(cw);
    srand(getpid());
    playit();
    /*NOTREACHED*/ 
    return(FALSE);
}

/*
 * perform an encrypted write
 */
encwrite(start, size, outf)
register char *start;
register unsigned size;
register FILE *outf;
{
    register char *ep;
    register num_written = 0;

    ep = encstr;

    while (size--)
    {
	if (putc(*start++ ^ *ep++, outf) == EOF && ferror(outf))
	    return(num_written);
	num_written++;
	if (*ep == '\0')
	    ep = encstr;
    }
    return(num_written);
}

/*
 * perform an encrypted read
 */
encread(start, size, inf)
register char *start;
register unsigned size;
register int inf;
{
    register char *ep;
    register int read_size;

    if ((read_size = read(inf, start, size)) == -1 || read_size == 0)
	return read_size;

    ep = encstr;

    size = read_size;
    while (size--)
    {
	*start++ ^= *ep++;
	if (*ep == '\0')
	    ep = encstr;
    }
    return read_size;
}
