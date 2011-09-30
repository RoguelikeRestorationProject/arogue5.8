/* 
 * Networking information -- should not vary among networking machines
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#define SYSLEN 9
#define LOGLEN 8
#define NUMNET 6
#undef NUMNET
struct network {
    char *system;
    char *rogue;
};
extern struct network Network[];

/* This system's name -- should not be defined if uname() is available */
