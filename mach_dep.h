/*
 * machine dependicies
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

/*
 * define that the wizard commands exist
 */
#define WIZARD	1

/*
 * define if you want to limit scores to one per class per userid
 */
#undef LIMITSCORE 

/*
 * where scorefile should live
 */
#define SCOREFILE	"/bnr/contrib/lib/rogue/scorefile"

/*
 * Variables for checking to make sure the system isn't too loaded
 * for people to play
 */

#undef	MAXUSERS	/*40*/	/* max number of users for this game */
#undef	MAXLOAD		/*40*/	/* 10 * max 15 minute load average */

#undef	CHECKTIME	/*15*/	/* number of minutes between load checks */
				/* if not defined checks are only on startup */

#ifdef MAXLOAD
#define	LOADAV			/* defined if rogue should provide loadav() */

#ifdef LOADAV
#define	NAMELIST	"/unix"	/* where the system namelist lives */
#endif
#endif

#ifdef MAXUSERS
#define	UCOUNT			/* defined if rogue should provide ucount() */

#ifdef UCOUNT
#define UTMP	"/etc/utmp"	/* where utmp file lives */
#endif
#endif

#undef AUTHOR /*212*/

/*
 * define the current author of the program for "special handling"
 */
#ifndef AUTHOR
#define AUTHOR 0	/* Default to root if not specified above */
#endif
