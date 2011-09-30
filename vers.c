/*
 * version number.  Whenever a new version number is desired, use
 * sccs to get vers.c.  Environ and encstr are declared here to
 * force them to be loaded before the version number, and therefore
 * not to be written in saved games.
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

char encstr[] = "\354\251\243\332A\201|\301\321p\210\251\327\"\257\365t\341%3\271^`~\203z{\341};\f\341\231\222e\234\351]\321\234";
char version[] = "@(#)vers.c	5.8 (Bell Labs) 1/03/85";
char *release = "5.8.2";
