/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
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
#include "rogue.h"


/*
 * zap a stick and see what happens
 */
do_zap(gotdir, which, flag)
bool gotdir;
int which;
int flag;
{
    register struct linked_list *item;
    register struct object *obj = NULL;
    register struct thing *tp;
    register int y, x;
    struct linked_list *nitem;
    struct object *nobj;
    bool cursed, blessed, is_stick;

    blessed = FALSE;
    cursed = FALSE;
    is_stick = FALSE;

    if (which == 0) {
	if ((item = get_item(pack, "zap with", ZAPPABLE)) == NULL)
	    return(FALSE);
	obj = OBJPTR(item);

	/* Handle relics specially here */
	if (obj->o_type == RELIC) {
	    switch (obj->o_which) {
		case ORCUS_WAND:
		    msg(nothing);
		    return(TRUE);
		when MING_STAFF:
		    which = WS_MISSILE;
		when ASMO_ROD:
		    switch (rnd(3)) {
			case 0:
			    which = WS_ELECT;
			when 1:
			    which = WS_COLD;
			otherwise:
			    which = WS_FIRE;
		    }
	    }
	    cursed = FALSE;
	    blessed = FALSE;
	}
	else {
	    which = obj->o_which;
	    ws_know[which] = TRUE;
	    cursed = (obj->o_flags & ISCURSED) != 0;
	    blessed = (obj->o_flags & ISBLESSED) != 0;
	    is_stick = TRUE;
	}
    }
    else {
	cursed = flag & ISCURSED;
	blessed = flag & ISBLESSED;
    }
    switch (which) {	/* no direction for these */
	case WS_LIGHT:
	case WS_DRAIN:
	case WS_CHARGE:
	case WS_CURING:
	    break;

	default:
	    if (!get_dir())
		return(FALSE);
	    if (!gotdir) {
		do {
		    delta.y = rnd(3) - 1;
		    delta.x = rnd(3) - 1;
		} while (delta.y == 0 && delta.x == 0);
	    }
    }

    if (is_stick) {
	if (obj->o_charges < 1) {
	    msg(nothing);
	    return(TRUE);
	}
	obj->o_charges--;
    }
    if (which == WS_WONDER) {
	switch (rnd(14)) {
	    case  0: which = WS_ELECT;
	    when  1: which = WS_FIRE;
	    when  2: which = WS_COLD;
	    when  3: which = WS_POLYMORPH;
	    when  4: which = WS_MISSILE;
	    when  5: which = WS_SLOW_M;
	    when  6: which = WS_TELMON;
	    when  7: which = WS_CANCEL;
	    when  8: which = WS_CONFMON;
	    when  9: which = WS_DISINTEGRATE;
	    when 10: which = WS_PETRIFY;
	    when 11: which = WS_PARALYZE;
	    when 12: which = WS_MDEG;
	    when 13: which = WS_FEAR;
	}
	if(ws_magic[which].mi_curse>0 && rnd(100)<=ws_magic[which].mi_curse){
	    cursed = TRUE;
	    blessed = FALSE;
	}
    }

    switch (which) {
	case WS_LIGHT:
	    /*
	     * Reddy Kilowat wand.  Light up the room
	     */
	    blue_light(blessed, cursed);
	when WS_DRAIN:
	    /*
	     * Take away 1/2 of hero's hit points, then take it away
	     * evenly from the monsters in the room or next to hero
	     * if he is in a passage (but leave the monsters alone
	     * if the stick is cursed)
	     */
	    if (pstats.s_hpt < 2) {
		msg("You are too weak to use it.");
		return(TRUE);
	    }
	    if (cursed)
		pstats.s_hpt /= 2;
	    else
		drain(hero.y-1, hero.y+1, hero.x-1, hero.x+1);
	when WS_POLYMORPH:
	case WS_TELMON:
	case WS_CANCEL:
	{
	    register char monster, oldch;
	    register int rm;

	    y = hero.y;
	    x = hero.x;
	    while (shoot_ok(winat(y, x))) {
		y += delta.y;
		x += delta.x;
	    }
	    if (isalpha(monster = CCHAR( mvwinch(mw, y, x) ))) {
		register struct room *rp;

		item = find_mons(y, x);
		tp = THINGPTR(item);
		/* if the monster gets the saving throw, leave the case */
		if (save(VS_MAGIC, tp, 0)) {
		    msg(nothing);
		    break;
		}

		/* Unhold player */
		if (on(*tp, DIDHOLD)) {
		    turn_off(*tp, DIDHOLD);
		    if (--hold_count == 0) turn_off(player, ISHELD);
		}
		/* unsuffocate player */
		if (on(*tp, DIDSUFFOCATE)) {
		    turn_off(*tp, DIDSUFFOCATE);
		    extinguish(suffocate);
		}
		rp = roomin(&tp->t_pos);
		/*
		 * check to see if room should go dark
		 */
		if (on(*tp, HASFIRE)) {
		    if (rp != NULL) {
			register struct linked_list *fire_item;

			for (fire_item = rp->r_fires; fire_item != NULL;
			     fire_item = next(fire_item)) {
			    if (THINGPTR(fire_item) == tp) {
				detach(rp->r_fires, fire_item);
				destroy_item(fire_item);
				if (rp->r_fires == NULL) {
				    rp->r_flags &= ~HASFIRE;
				    if(cansee(tp->t_pos.y,tp->t_pos.x))
					light(&hero);
				}
				break;
			    }
			}
		    }
		}

		if (which == WS_POLYMORPH) {
		    register struct linked_list *pitem;

		    delta.x = x;
		    delta.y = y;
		    detach(mlist, item);
		    oldch = tp->t_oldch;
		    pitem = tp->t_pack; /* save his pack */
		    tp->t_pack = NULL;
		    new_monster(item,rnd(NUMMONST-NUMUNIQUE-1)+1,&delta,FALSE);
		    if (tp->t_pack != NULL) 
			o_free_list (tp->t_pack);
		    tp->t_pack = pitem;
		    monster = tp->t_type;
		    if (isalpha(mvwinch(cw, y, x)))
			mvwaddch(cw, y, x, monster);
		    tp->t_oldch = oldch;
		    /*
		     * should the room light up?
		     */
		    if (on(*tp, HASFIRE)) {
			if (rp) {
			    register struct linked_list *fire_item;

			    fire_item = creat_item();
			    ldata(fire_item) = (char *) tp;
			    attach(rp->r_fires, fire_item);
			    rp->r_flags |= HASFIRE;
			    if (cansee(tp->t_pos.y,tp->t_pos.x) &&
				next(rp->r_fires) == NULL) light(&hero);
			}
		    }
		    msg(terse ? "A new %s!" : "You have created a new %s!",
			monsters[tp->t_index].m_name);
		}
		else if (which == WS_CANCEL) {
		    tp->t_flags[0] &= CANC0MASK;
		    tp->t_flags[1] &= CANC1MASK;
		    tp->t_flags[2] &= CANC2MASK;
		    tp->t_flags[3] &= CANC3MASK;
		    tp->t_flags[4] &= CANC4MASK;
		    tp->t_flags[4] &= CANC5MASK;
		}
		else { /* A teleport stick */
		    if (cursed) {	/* Teleport monster to player */
			if ((y == (hero.y + delta.y)) &&
			    (x == (hero.x + delta.x)))
				msg(nothing);
			else {
			    tp->t_pos.y = hero.y + delta.y;
			    tp->t_pos.x = hero.x + delta.x;
			}
		    }
		    else if (blessed) {	/* Get rid of monster */
			killed(item, FALSE, TRUE);
			return(TRUE);
		    }
		    else {
			register int i=0;

			do {	/* Move monster to another room */
			    rm = rnd_room();
			    rnd_pos(&rooms[rm], &tp->t_pos);
			}until(winat(tp->t_pos.y,tp->t_pos.x)==FLOOR ||i++>500);
			rp = &rooms[rm];
		    }

		    /* Now move the monster */
		    if (isalpha(mvwinch(cw, y, x)))
			mvwaddch(cw, y, x, tp->t_oldch);
		    turn_off(*tp, ISDISGUISE);
		    mvwaddch(mw, y, x, ' ');
		    mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, monster);
		    if (tp->t_pos.y != y || tp->t_pos.x != x)
			tp->t_oldch = CCHAR( mvwinch(cw, tp->t_pos.y, tp->t_pos.x) );
		    /*
		     * check to see if room that creature appears in should
		     * light up
		     */
		    if (on(*tp, HASFIRE)) {
			register struct linked_list *fire_item;

			fire_item = creat_item();
			ldata(fire_item) = (char *) tp;
			attach(rp->r_fires, fire_item);
			rp->r_flags |= HASFIRE;
			if(cansee(tp->t_pos.y, tp->t_pos.x) && 
			   next(rp->r_fires) == NULL)
			    light(&hero);
		    }
		}
		runto(tp, &hero);
	    }
	}
	when WS_MISSILE:
	{
	    static struct object bolt =
	    {
		MISSILE , {0, 0}, "", 0, "", "1d4 " , NULL, 0, WS_MISSILE, 50, 1
	    };

	    sprintf(bolt.o_hurldmg, "%dd4", pstats.s_lvl);
	    do_motion(&bolt, delta.y, delta.x, &player);
	    if (!hit_monster(unc(bolt.o_pos), &bolt, &player))
	       msg("The missile vanishes with a puff of smoke");
	}
	when WS_HIT:
	{
	    register char ch;
	    struct object strike; /* don't want to change sticks attributes */

	    delta.y += hero.y;
	    delta.x += hero.x;
	    ch = CCHAR( winat(delta.y, delta.x) );
	    if (isalpha(ch))
	    {
		strike = *obj;
		strike.o_hplus  = 6;
		if (EQUAL(ws_type[which], "staff"))
		    strcpy(strike.o_damage,"3d8");
		else
		    strcpy(strike.o_damage,"2d8");
		fight(&delta, &strike, FALSE);
	    }
	}
	case WS_SLOW_M:
	    y = hero.y;
	    x = hero.x;
	    while (shoot_ok(winat(y, x))) {
		y += delta.y;
		x += delta.x;
	    }
	    if (isalpha(mvwinch(mw, y, x))) {
		item = find_mons(y, x);
		tp = THINGPTR(item);
		runto(tp, &hero);
		if (on(*tp, ISUNIQUE) && save(VS_MAGIC, tp, 0))
		    msg(nothing);
		else if (on(*tp, NOSLOW))
		    msg(nothing);
		else if (cursed) {
		    if (on(*tp, ISSLOW))
			turn_off(*tp, ISSLOW);
		    else
			turn_on(*tp, ISHASTE);
		}
		else if (blessed) {
		    turn_off(*tp, ISRUN);
		    turn_on(*tp, ISHELD);
		    return(TRUE);
		}
		else {
		    if (on(*tp, ISHASTE))
			turn_off(*tp, ISHASTE);
		    else
			turn_on(*tp, ISSLOW);
		    tp->t_turn = TRUE;
		}
	    }
	when WS_CHARGE:
	    if (ws_know[WS_CHARGE] != TRUE && is_stick)
		msg("This is a wand of charging.");
	    if ((nitem = get_item(pack, "charge", STICK)) != NULL) {
		nobj = OBJPTR(nitem);
	        if ((++(nobj->o_charges) == 1) && (nobj->o_which == WS_HIT))
		    fix_stick(nobj);
	        if (EQUAL(ws_type[nobj->o_which], "staff")) {
		    if (nobj->o_charges > 100) 
		        nobj->o_charges = 100;
		}
	        else {
		    if (nobj->o_charges > 50)
		        nobj->o_charges = 50;
		}
	    }
	when WS_ELECT:
	case WS_FIRE:
	case WS_COLD:
	    {
		char *name;

		if (which == WS_ELECT)
		    name = "lightning bolt";
		else if (which == WS_FIRE)
		    name = "flame";
		else
		    name = "ice";

		shoot_bolt(	&player, hero, 
				delta, TRUE, D_BOLT, 
				name, roll(pstats.s_lvl,6));
	    }
	when WS_PETRIFY: {
	    reg int m1, m2, x1, y1;
	    reg char ch;
	    reg struct linked_list *ll;
	    reg struct thing *lt;

	    y1 = hero.y;
	    x1 = hero.x;
	    do {
		y1 += delta.y;
		x1 += delta.x;
		ch = CCHAR( winat(y1,x1) );
	    } while (ch == PASSAGE || ch == FLOOR);
	    for (m1 = x1 - 1 ; m1 <= x1 + 1 ; m1++) {
		for(m2 = y1 - 1 ; m2 <= y1 + 1 ; m2++) {
		    ch = CCHAR( winat(m2,m1) );
		    if (m1 == hero.x && m2 == hero.y)
			continue;
		    if (ch != ' ') {
			ll = find_obj(m2,m1);
			if (ll != NULL) {
			    detach(lvl_obj,ll);
			    o_discard(ll);
			}
			ll = find_mons(m2,m1);
			if (ll != NULL) {
			    lt = THINGPTR(ll);
			    if (on(*lt, ISUNIQUE)) 
			        monsters[lt->t_index].m_normal = TRUE;
			    check_residue(lt);
			    detach(mlist,ll);
			    t_discard(ll);
			    mvwaddch(mw,m2,m1,' ');
			}
			mvaddch(m2,m1,' ');
			mvwaddch(cw,m2,m1,' ');
		    }
		}
	    }
	    touchwin(cw);
	    touchwin(mw);
	}
	when WS_CONFMON:
	    if (cursed) {
		if (off(player, ISCLEAR)) {
		    if (on(player, ISHUH))
			lengthen(unconfuse, rnd(20)+HUHDURATION);
		    else {
			turn_on(player, ISHUH);
			fuse(unconfuse,0,rnd(20)+HUHDURATION,AFTER);
			msg("Wait, what's going on here. Huh? What? Who?");
		    }
		}
		else msg("You feel dizzy for a moment, but it quickly passes.");
	    }
	    else {
		y = hero.y;
		x = hero.x;
		while (shoot_ok(winat(y, x)))
		{
		    y += delta.y;
		    x += delta.x;
		}
		if (isalpha(mvwinch(mw, y, x)))
		{
		    item = find_mons(y, x);
		    tp = THINGPTR(item);
		    if (save(VS_MAGIC, tp, 0) || on(*tp, ISCLEAR))
			 msg(nothing);
		    else
			 turn_on (*tp, ISHUH);
		    runto(tp, &hero);
		}
	    }
	when WS_PARALYZE:
	    if (cursed) {
		no_command += FREEZETIME;
		msg("You can't move.");
	    }
	    else {
		y = hero.y;
		x = hero.x;
		while (shoot_ok(winat(y, x)))
		{
		    y += delta.y;
		    x += delta.x;
		}
		if (isalpha(mvwinch(mw, y, x)))
		{
		    item = find_mons(y, x);
		    tp = THINGPTR(item);
		    if (save(VS_WAND, tp, 0) || on(*tp, NOPARALYZE))
			msg(nothing);
		    else {
			tp->t_no_move = FREEZETIME;
		    }
		    runto(tp, &hero);
		}
	    }
	    when WS_FEAR:
		y = hero.y;
		x = hero.x;
		while (shoot_ok(winat(y, x)))
		{
		    y += delta.y;
		    x += delta.x;
		}
		if (isalpha(mvwinch(mw, y, x)))
		{
		    item = find_mons(y, x);
		    tp = THINGPTR(item);
		    runto(tp, &hero);
		    if (save(VS_WAND, tp, 0) || 
			on(*tp, ISUNDEAD)    || 
			on(*tp, NOFEAR))
			    msg(nothing);
		    else {
			turn_on(*tp, ISFLEE);
			turn_on(*tp, WASTURNED);

			/* If monster was suffocating, stop it */
			if (on(*tp, DIDSUFFOCATE)) {
			    turn_off(*tp, DIDSUFFOCATE);
			    extinguish(suffocate);
			}

			/* If monster held us, stop it */
			if (on(*tp, DIDHOLD) && (--hold_count == 0))
				turn_off(player, ISHELD);
			turn_off(*tp, DIDHOLD);
		    }
		}
	when WS_MDEG:
	    y = hero.y;
	    x = hero.x;
	    while (shoot_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if (isalpha(mvwinch(mw, y, x)))
	    {
		item = find_mons(y, x);
		tp = THINGPTR(item);
		if (cursed) {
		     tp->t_stats.s_hpt *= 2;
		     msg("The %s appears to be stronger now!", 
			monsters[tp->t_index].m_name);
		}
		else if (on(*tp, ISUNIQUE) && save(VS_WAND, tp, 0))
		     msg (nothing);
		else {
		     tp->t_stats.s_hpt /= 2;
		     msg("The %s appears to be weaker now", 
			monsters[tp->t_index].m_name);
		}
		runto(tp, &hero);
	        if (tp->t_stats.s_hpt < 1)
		     killed(item, TRUE, TRUE);
	    }
	when WS_DISINTEGRATE:
	    y = hero.y;
	    x = hero.x;
	    while (shoot_ok(winat(y, x))) {
		y += delta.y;
		x += delta.x;
	    }
	    if (isalpha(mvwinch(mw, y, x))) {
		item = find_mons(y, x);
		tp = THINGPTR(item);
		turn_on (*tp, ISMEAN);
		runto(tp, &hero);
		if (cursed) {
		    register int m1, m2;
		    coord mp;
		    struct linked_list *titem;
		    char ch;
		    struct thing *th;

		    if (on(*tp, ISUNIQUE)) {
			msg (nothing);
			break;
		    }
		    for (m1=tp->t_pos.x-1 ; m1 <= tp->t_pos.x+1 ; m1++) {
			for(m2=tp->t_pos.y-1 ; m2<=tp->t_pos.y+1 ; m2++) {
			    ch = CCHAR( winat(m2,m1) );
			    if (shoot_ok(ch) && ch != PLAYER) {
				mp.x = m1;	/* create it */
				mp.y = m2;
				titem = new_item(sizeof(struct thing));
				new_monster(titem,(short)tp->t_index,&mp,FALSE);
				th = THINGPTR(titem);
				turn_on (*th, ISMEAN);
				runto(th,&hero);
				if (on(*th, HASFIRE)) {
				    register struct room *rp;

				    rp = roomin(&th->t_pos);
				    if (rp) {
					register struct linked_list *fire_item;

					fire_item = creat_item();
					ldata(fire_item) = (char *) th;
					attach(rp->r_fires, fire_item);
					rp->r_flags |= HASFIRE;
					if (cansee(th->t_pos.y, th->t_pos.x) &&
					    next(rp->r_fires) == NULL)
					    light(&hero);
				    }
				}
			    }
			}
		    }
		}
		else { /* if its a UNIQUE it might still live */
		    tp = THINGPTR(item);
		    if (on(*tp, ISUNIQUE) && save(VS_MAGIC, tp, 0)) {
			tp->t_stats.s_hpt /= 2;
			if (tp->t_stats.s_hpt < 1) {
			     killed(item, FALSE, TRUE);
			     msg("You have disintegrated the %s", 
				    monsters[tp->t_index].m_name);
			}
			else {
			    msg("The %s appears wounded",
				monsters[tp->t_index].m_name);
			}
		    }
		    else {
			msg("You have disintegrated the %s", 
				monsters[tp->t_index].m_name);
			killed (item, FALSE, TRUE);
		    }
		}
	    }
	when WS_CURING:
	    ws_know[WS_CURING] = TRUE;
	    if (cursed) {
		if (!save(VS_POISON, &player, 0)) {
		    msg("You feel extremely sick now");
		    pstats.s_hpt /=2;
		    if (pstats.s_hpt == 0) death (D_POISON);
		}
		if (!save(VS_WAND, &player, 0) && !ISWEARING(R_HEALTH)) {
		    turn_on(player, HASDISEASE);
		    turn_on(player, HASINFEST);
		    turn_on(player, DOROT);
		    fuse(cure_disease, 0, roll(HEALTIME,SICKTIME), AFTER);
		    infest_dam++;
		}
		else msg("You fell momentarily sick");
	    }
	    else {
		if (on(player, HASDISEASE)) {
		    extinguish(cure_disease);
		    cure_disease();
		    msg(terse ? "You feel yourself improving."
			      : "You begin to feel yourself improving again.");
		}
		if (on(player, HASINFEST)) {
		    turn_off(player, HASINFEST);
		    infest_dam = 0;
		    msg(terse ? "You feel yourself improving."
			      : "You begin to feel yourself improving again.");
		}
		if (on(player, DOROT)) {
		    msg("You feel your skin returning to normal.");
		    turn_off(player, DOROT);
		}
		pstats.s_hpt += roll(pstats.s_lvl, blessed ? 6 : 4);
		if (pstats.s_hpt > max_stats.s_hpt)
		    pstats.s_hpt = max_stats.s_hpt;
		msg("You begin to feel %sbetter.", blessed ? "much " : "");
		    
	    }
	otherwise:
	    msg("What a bizarre schtick!");
    }
    return(TRUE);
}


/*
 * drain:
 *	Do drain hit points from player shtick
 */

drain(ymin, ymax, xmin, xmax)
int ymin, ymax, xmin, xmax;
{
    register int i, j, count;
    register struct thing *ick;
    register struct linked_list *item;

    /*
     * First count how many things we need to spread the hit points among
     */
    count = 0;
    for (i = ymin; i <= ymax; i++) {
	if (i < 1 || i > LINES - 3)
	    continue;
	for (j = xmin; j <= xmax; j++) {
	    if (j < 0 || j > COLS - 1)
		continue;
	    if (isalpha(mvwinch(mw, i, j)))
		count++;
	}
    }
    if (count == 0)
    {
	msg("You have a tingling feeling");
	return;
    }
    count = pstats.s_hpt / count;
    pstats.s_hpt /= 2;
    /*
     * Now zot all of the monsters
     */
    for (i = ymin; i <= ymax; i++) {
	if (i < 1 || i > LINES - 3)
	    continue;
	for (j = xmin; j <= xmax; j++) {
	    if (j < 0 || j > COLS - 1)
		continue;
	    if (isalpha(mvwinch(mw, i, j)) &&
	        ((item = find_mons(i, j)) != NULL)) {
		ick = THINGPTR(item);
		if (on(*ick, ISUNIQUE) && save(VS_MAGIC, ick, 0)) 
		    ick->t_stats.s_hpt -= count / 2;
		else
		    ick->t_stats.s_hpt -= count;
		if (ick->t_stats.s_hpt < 1)
		    killed(item, 
			   cansee(i,j)&&(!on(*ick,ISINVIS)||on(player,CANSEE)),
			   TRUE);
		else {
		    runto(ick, &hero);
		    if (cansee(i,j) && (!on(*ick,ISINVIS)||on(player,CANSEE)))
			    msg("The %s appears wounded",
				monsters[ick->t_index].m_name);
		}
	    }
	}
    }
}

/*
 * initialize a stick
 */
fix_stick(cur)
register struct object *cur;
{
    if (EQUAL(ws_type[cur->o_which], "staff")) {
	cur->o_weight = 100;
	cur->o_charges = 5 + rnd(10);
	strcpy(cur->o_damage,"2d3");
	cur->o_hplus = 1;
	cur->o_dplus = 0;
	switch (cur->o_which) {
	    case WS_HIT:
		cur->o_hplus = 3;
		cur->o_dplus = 3;
		strcpy(cur->o_damage,"2d8");
	    when WS_LIGHT:
		cur->o_charges = 20 + rnd(10);
	    }
    }
    else {
	strcpy(cur->o_damage,"1d3");
	cur->o_weight = 60;
	cur->o_hplus = 1;
	cur->o_dplus = 0;
	cur->o_charges = 3 + rnd(5);
	switch (cur->o_which) {
	    case WS_HIT:
		cur->o_hplus = 3;
		cur->o_dplus = 3;
		strcpy(cur->o_damage,"1d8");
	    when WS_LIGHT:
		cur->o_charges = 10 + rnd(10);
	    }
    }
    strcpy(cur->o_hurldmg,"1d1");

}

/*
 * shoot_bolt fires a bolt from the given starting point in the
 * 	      given direction
 */

shoot_bolt(shooter, start, dir, get_points, reason, name, damage)
struct thing *shooter;
coord start, dir;
bool get_points;
short reason;
char *name;
int damage;
{
    register char dirch = 0, ch;
    register bool used, change;
    register short y, x, bounces;
    bool mdead = FALSE;
    coord pos;
    struct {
	coord place;
	char oldch;
    } spotpos[BOLT_LENGTH];

    switch (dir.y + dir.x) {
	case 0: dirch = '/';
	when 1: case -1: dirch = (dir.y == 0 ? '-' : '|');
	when 2: case -2: dirch = '\\';
    }
    pos.y = start.y + dir.y;
    pos.x = start.x + dir.x;
    used = FALSE;
    change = FALSE;

    bounces = 0;	/* No bounces yet */
    for (y = 0; y < BOLT_LENGTH && !used; y++)
    {
	ch = CCHAR( winat(pos.y, pos.x) );
	spotpos[y].place = pos;
	spotpos[y].oldch = CCHAR( mvwinch(cw, pos.y, pos.x) );

	/* Are we at hero? */
	if (ce(pos, hero)) goto at_hero;

	switch (ch)
	{
	    case SECRETDOOR:
	    case '|':
	    case '-':
	    case ' ':
		if (dirch == '-' || dirch == '|') {
		    dir.y = -dir.y;
		    dir.x = -dir.x;
		}
		else {
		    char chx = CCHAR( mvinch(pos.y-dir.y, pos.x) ),
			 chy = CCHAR( mvinch(pos.y, pos.x-dir.x) );
		    bool anychange = FALSE; /* Did we change anthing */

		    if (chy == WALL || chy == SECRETDOOR ||
			chy == '-' || chy == '|') {
			dir.y = -dir.y;
			change ^= TRUE;	/* Change at least one direction */
			anychange = TRUE;
		    }
		    if (chx == WALL || chx == SECRETDOOR ||
			chx == '-' || chx == '|') {
			dir.x = -dir.x;
			change ^= TRUE;	/* Change at least one direction */
			anychange = TRUE;
		    }

		    /* If we didn't make any change, make both changes */
		    if (!anychange) {
			dir.x = -dir.x;
			dir.y = -dir.y;
		    }
		}

		/* Do we change how the bolt looks? */
		if (change) {
		    change = FALSE;
		    if (dirch == '\\') dirch = '/';
		    else if (dirch == '/') dirch = '\\';
		}

		y--;	/* The bounce doesn't count as using up the bolt */

		/* Make sure we aren't in an infinite bounce */
		if (++bounces > BOLT_LENGTH) used = TRUE;
		msg("The %s bounces", name);
		break;
	    default:
		if (isalpha(ch)) {
		    register struct linked_list *item;
		    register struct thing *tp;
		    register const char *mname;
		    bool see_monster = cansee(pos.y, pos.x);

		    item = find_mons(unc(pos));
		    tp = THINGPTR(item);
		    mname = monsters[tp->t_index].m_name;

		    if (!save(VS_BREATH, tp, -(shooter->t_stats.s_lvl/10))) {
			if (see_monster) {
			    if (on(*tp, ISDISGUISE) &&
				(tp->t_type != tp->t_disguise)) {
				msg("Wait! That's a %s!", mname);
				turn_off(*tp, ISDISGUISE);
			    }

			    sprintf(outstring,"The %s hits the %s", name, mname);
			    msg(outstring);
			}

			tp->t_wasshot = TRUE;
			runto(tp, &hero);
			used = TRUE;

			/* Hit the monster -- does it do anything? */
			if ((EQUAL(name,"ice")           &&
			     (on(*tp, NOCOLD) || on(*tp, ISUNDEAD)))          ||
			    (EQUAL(name,"flame")         && on(*tp, NOFIRE))  ||
			    (EQUAL(name,"acid")          && on(*tp, NOACID))  ||
			    (EQUAL(name,"lightning bolt")&& on(*tp,NOBOLT))   ||
			    (EQUAL(name,"nerve gas")     &&on(*tp,NOPARALYZE))||
			    (EQUAL(name,"sleeping gas")  &&
			     (on(*tp, NOSLEEP) || on(*tp, ISUNDEAD)))         ||
			    (EQUAL(name,"slow gas")      && on(*tp,NOSLOW))   ||
			    (EQUAL(name,"fear gas")      && on(*tp,NOFEAR))   ||
			    (EQUAL(name,"confusion gas") && on(*tp,ISCLEAR))  ||
			    (EQUAL(name,"chlorine gas")  && on(*tp,NOGAS))) {
			    if (see_monster){
				sprintf(outstring,"The %s has no effect on the %s.",
					name, mname);
				msg(outstring);
			    }
			}

			/* 
			 * Check for gas with special effects 
			 */
			else if (EQUAL(name, "nerve gas")) {
			    tp->t_no_move = FREEZETIME;
			}
			else if (EQUAL(name, "sleeping gas")) {
			    tp->t_no_move = SLEEPTIME;
			}
			else if (EQUAL(name, "slow gas")) {
			    if (on(*tp, ISHASTE))
				turn_off(*tp, ISHASTE);
			    else
				turn_on(*tp, ISSLOW);
			    tp->t_turn = TRUE;
			}
			else if (EQUAL(name, "fear gas")) {
			    turn_on(*tp, ISFLEE);
			    tp->t_dest = &hero;
			}
			else if (EQUAL(name, "confusion gas")) {
			    turn_on(*tp, ISHUH);
			    tp->t_dest = &hero;
			}
			else if ((EQUAL(name, "lightning bolt")) &&
				 on(*tp, BOLTDIVIDE)) {
				if (creat_mons(tp, tp->t_index, FALSE)) {
				  if (see_monster){
				      sprintf(outstring,"The %s divides the %s.",name,mname);
				      msg(outstring);
				  }
				  light(&hero);
				}
				else if (see_monster){
				    sprintf(outstring,"The %s has no effect on the %s.",
					name, mname);
				    msg(outstring);
				}
			}
			else {
			    if(save(VS_BREATH,tp, -(shooter->t_stats.s_lvl/10)))
				damage /= 2;

			    /* The poor fellow got killed! */
			    if ((tp->t_stats.s_hpt -= damage) <= 0) {
				if (see_monster){
				    sprintf(outstring,"The %s kills the %s", name, mname);
				    msg(outstring);
				}
				else
				 msg("You hear a faint groan in the distance");
				killed(item, FALSE, get_points);

				/* Replace the screen character */
				spotpos[y].oldch = CCHAR( mvwinch(cw, pos.y, pos.x) );

				mdead = TRUE;
			    }
			    else {	/* Not dead, so just scream */
			         if (!see_monster)
				     msg("You hear a scream in the distance");
			    }
			}
		    }
		    else if (isalpha(show(pos.y, pos.x))) {
			if (see_monster) {
			    if (terse)
				msg("%s misses", name);
			    else {
				sprintf(outstring,"The %s whizzes past the %s", name, mname);
				msg(outstring);
			    }
			}
			if (get_points) runto(tp, &hero);
		    }
		}
		else if (pos.y == hero.y && pos.x == hero.x) {
at_hero: 	    if (!save(VS_BREATH, &player,
				-(shooter->t_stats.s_lvl/10))){
			if (terse)
			    msg("The %s hits you", name);
			else
			    msg("You are hit by the %s", name);
			used = TRUE;

			/* 
			 * The Amulet of Yendor protects against all "breath" 
			 *
			 * The following two if statements could be combined 
			 * into one, but it makes the compiler barf, so split 
			 * it up
			 */
			if (cur_relic[YENDOR_AMULET]			    ||
			    (EQUAL(name,"chlorine gas")&&on(player, NOGAS)) ||
			    (EQUAL(name,"sleeping gas")&&ISWEARING(R_ALERT))){
			     msg("The %s has no affect", name);
			}
			else if((EQUAL(name, "flame") && on(player, NOFIRE)) ||
			        (EQUAL(name, "ice")   && on(player, NOCOLD)) ||
			        (EQUAL(name,"fear gas")&&ISWEARING(R_HEROISM))){
			     msg("The %s has no affect", name);
			}
			/* 
			 * Check for gas with special effects 
			 */
			else if (EQUAL(name, "nerve gas")) {
			    msg("The nerve gas paralyzes you.");
			    no_command += FREEZETIME;
			}
			else if (EQUAL(name, "sleeping gas")) {
			    msg("The sleeping gas puts you to sleep.");
			    no_command += SLEEPTIME;
			}
			else if (EQUAL(name, "confusion gas")) {
			    if (off(player, ISCLEAR)) {
				if (on(player, ISHUH))
				    lengthen(unconfuse, rnd(20)+HUHDURATION);
				else {
				    turn_on(player, ISHUH);
				    fuse(unconfuse,0,rnd(20)+HUHDURATION,AFTER);
				    msg("The confusion gas has confused you.");
				}
			    }
			    else msg("You feel dizzy for a moment, but it quickly passes.");
			}
			else if (EQUAL(name, "slow gas")) {
			    add_slow();
			}
			else if (EQUAL(name, "fear gas")) {
			    turn_on(player, ISFLEE);
			    player.t_dest = &shooter->t_pos;
			    msg("The fear gas terrifies you.");
			}
			else {
			    if(save(VS_BREATH,&player, -(shooter->t_stats.s_lvl/10)))
				damage /= 2;
			    if ((pstats.s_hpt -= damage) <= 0) 
				death(reason);
			}
		    }
		    else
			msg("The %s whizzes by you", name);
		}

		mvwaddch(cw, pos.y, pos.x, dirch);
		draw(cw);
	}

	pos.y += dir.y;
	pos.x += dir.x;
    }
    for (x = y - 1; x >= 0; x--)
	mvwaddch(cw, spotpos[x].place.y, spotpos[x].place.x, spotpos[x].oldch);
    return(mdead);
}
