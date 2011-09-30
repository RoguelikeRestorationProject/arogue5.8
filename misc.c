/*
 * routines dealing specifically with miscellaneous magic
 *
 * Advanced Rogue
 * Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "curses.h"
#include <ctype.h>
#include "rogue.h"

/*
 * See if a monster has some magic it can use.  Use it and return TRUE if so.
 */
bool
m_use_item(monster, monst_pos, defend_pos)
register struct thing *monster;
register coord *monst_pos, *defend_pos;
{
    register struct linked_list *pitem;
    register struct object *obj;
    register coord *shoot_dir = can_shoot(monst_pos, defend_pos);
    int dist=DISTANCE(monst_pos->y, monst_pos->x, defend_pos->y, defend_pos->x);

    for (pitem=monster->t_pack; pitem; pitem=next(pitem)) {
	obj = OBJPTR(pitem);
	if (obj->o_type != RELIC) continue;	/* Only care about relics now */
	switch (obj->o_which) {
	    case MING_STAFF: {
		static struct object missile = {
		  MISSILE, {0,0}, "", 0, "", "0d4 " , NULL, 0, WS_MISSILE, 100, 1
		};

		if (shoot_dir != NULL) {
		    sprintf(missile.o_hurldmg, "%dd4", monster->t_stats.s_lvl);
		    do_motion(&missile, shoot_dir->y, shoot_dir->x, monster);
		    hit_monster(unc(missile.o_pos), &missile, monster);
		    return(TRUE);
		}
	    }
	    when ASMO_ROD:
		/* The bolt must be able to reach the defendant */
		if (shoot_dir != NULL && dist < BOLT_LENGTH * BOLT_LENGTH) {
		    char *name;

		    switch (rnd(3)) { /* Select a function */
			case 0:	   name = "lightning bolt";
			when 1:	   name = "flame";
			otherwise: name = "ice";
		    }
		    shoot_bolt(	monster, 
				*monst_pos, 
				*shoot_dir, 
				FALSE, 
				monster->t_index, 
				name, 
				roll(monster->t_stats.s_lvl,6));
		    return(TRUE);
		}
	    when BRIAN_MANDOLIN:
		/* The defendant must be the player and within 2 spaces */
		if (ce(*defend_pos, hero) && dist < 9 && no_command == 0 &&
		    rnd(100) < 33) {
		    if (!save(VS_MAGIC, &player, -4) &&
			!ISWEARING(R_ALERT)) {
			msg("Some beautiful music enthralls you.");
			no_command += FREEZETIME;
		    }
		    else msg("You wince at a sour note.");
		    return(TRUE);
		}
	    when GERYON_HORN:
		/* The defendant must be the player and within 2 spaces */
		if (ce(*defend_pos, hero) && dist < 9 &&
		    (off(player, ISFLEE) || player.t_dest != &monster->t_pos)
		    && rnd(100) < 33) {
		    if (!ISWEARING(R_HEROISM) &&
			!save(VS_MAGIC, &player, -4)) {
			    turn_on(player, ISFLEE);
			    player.t_dest = &monster->t_pos;
			    msg("A shrill blast terrifies you.");
		    }
		    else msg("A shrill blast sends chills up your spine.");
		    return(TRUE);
		}
	}
    }
    return(FALSE);
}
 
/*
 * add something to the contents of something else
 */
put_contents(bag, item)
register struct object *bag;		/* the holder of the items */
register struct linked_list *item;	/* the item to put inside  */
{
    register struct linked_list *titem;
    register struct object *tobj;

    bag->o_ac++;
    tobj = OBJPTR(item);
    for (titem = bag->contents; titem != NULL; titem = next(titem)) {
	if ((OBJPTR(titem))->o_which == tobj->o_which)
	    break;
    }
    if (titem == NULL) {	/* if not a duplicate put at beginning */
	attach(bag->contents, item);
    }
    else {
	item->l_prev = titem;
	item->l_next = titem->l_next;
	if (next(titem) != NULL) 
	    (titem->l_next)->l_prev = item;
	titem->l_next = item;
    }
}

/*
 * remove something from something else
 */
take_contents(bag, item)
register struct object *bag;		/* the holder of the items */
register struct linked_list *item;
{

    if (bag->o_ac <= 0) {
	msg("Nothing to take out");
	return;
    }
    bag->o_ac--;
    detach(bag->contents, item);
    if (!add_pack(item, FALSE, NULL))
	put_contents(bag, item);
}


do_bag(item)
register struct linked_list *item;
{

    register struct linked_list *titem = NULL;
    register struct object *obj;
    bool doit = TRUE;

    obj = OBJPTR(item);
    while (doit) {
	msg("What do you want to do? (* for a list): ");
	mpos = 0;
	switch (readchar()) {
	    case EOF:
	    case ESCAPE:
		msg ("");
		doit = FALSE;
	    when '1':
		inventory(obj->contents, ALL);

	    when '2':
		if (obj->o_ac >= MAXCONTENTS) {
		    msg("the %s is full", m_magic[obj->o_which].mi_name);
		    break;
		}
		switch (obj->o_which) {
		case MM_BEAKER: titem = get_item(pack, "put in", POTION);
		when MM_BOOK:   titem = get_item(pack, "put in", SCROLL);
		}
		if (titem == NULL)
		    break;
		detach(pack, titem);
		inpack--;
		put_contents(obj, titem);
	    
	    when '3':
		titem = get_item(obj->contents,"take out",ALL);
		if (titem == NULL)
		    break;
		take_contents(obj, titem);
		
	    when '4': 
		switch (obj->o_which) {
		case MM_BEAKER: 
		    titem = get_item(obj->contents,"quaff",ALL);
		    if (titem == NULL)
			break;
		    obj->o_ac--;
		    detach(obj->contents, titem);
		    quaff((OBJPTR(titem))->o_which, 
			  (OBJPTR(titem))->o_flags & (ISCURSED | ISBLESSED),
			  TRUE);
		    o_discard(titem);
		when MM_BOOK:   
		    if (on(player, ISBLIND)) {
			msg("You can't see to read anything");
			break;
		    }
		    titem = get_item(obj->contents,"read",ALL);
		    if (titem == NULL)
			break;
		    obj->o_ac--;
		    detach(obj->contents, titem);
		    read_scroll((OBJPTR(titem))->o_which, 
			        (OBJPTR(titem))->o_flags & (ISCURSED|ISBLESSED),
				TRUE);
		    o_discard(titem);
		}
		doit = FALSE;

	    otherwise:
		wclear(hw);
		touchwin(hw);
		mvwaddstr(hw,0,0,"The following operations are available:");
		mvwaddstr(hw,2,0,"[1]\tInventory\n");
		wprintw(hw,"[2]\tPut something in the %s\n",
			m_magic[obj->o_which].mi_name);
		wprintw(hw,"[3]\tTake something out of the %s\n",
			m_magic[obj->o_which].mi_name);
		switch(obj->o_which) {
		    case MM_BEAKER: waddstr(hw,"[4]\tQuaff a potion\n");
		    when MM_BOOK:   waddstr(hw,"[4]\tRead a scroll\n");
		}
		waddstr(hw,"[ESC]\tLeave this menu\n");
		mvwaddstr(hw, LINES-1, 0, spacemsg);
		draw(hw);
		wait_for (hw,' ');
		clearok(cw, TRUE);
		touchwin(cw);
	}
    }
}

do_panic()
{
    register int x,y;
    register struct linked_list *mon;
    register struct thing *th;

    for (x = hero.x-2; x <= hero.x+2; x++) {
	for (y = hero.y-2; y <= hero.y+2; y++) {
	    if (y < 1 || x < 0 || y > LINES - 3  || x > COLS - 1) 
		continue;
	    if (isalpha(mvwinch(mw, y, x))) {
		if ((mon = find_mons(y, x)) != NULL) {
		    th = THINGPTR(mon);
		    if (!on(*th, ISUNDEAD) && !save(VS_MAGIC, th, 0)) {
			turn_on(*th, ISFLEE);
			turn_on(*th, WASTURNED);

			/* If monster was suffocating, stop it */
			if (on(*th, DIDSUFFOCATE)) {
			    turn_off(*th, DIDSUFFOCATE);
			    extinguish(suffocate);
			}

			/* If monster held us, stop it */
			if (on(*th, DIDHOLD) && (--hold_count == 0))
				turn_off(player, ISHELD);
			turn_off(*th, DIDHOLD);
		    }
		    runto(th, &hero);
		}
	    }
	}
    }
}

/*
 * print miscellaneous magic bonuses
 */
char *
misc_name(obj)
register struct object *obj;
{
    static char buf[LINELEN];
    char buf1[LINELEN];

    buf[0] = '\0';
    buf1[0] = '\0';
    if (!(obj->o_flags & ISKNOW))
	return (m_magic[obj->o_which].mi_name);
    switch (obj->o_which) {
	case MM_BRACERS:
	case MM_PROTECT:
	    strcat(buf, num(obj->o_ac, 0));
	    strcat(buf, " ");
    }
    switch (obj->o_which) {
	case MM_G_OGRE:
	case MM_G_DEXTERITY:
	case MM_JEWEL:
	case MM_STRANGLE:
	case MM_R_POWERLESS:
	case MM_DANCE:
	    if (obj->o_flags & ISCURSED)
		strcat(buf, "cursed ");
    }
    strcat(buf, m_magic[obj->o_which].mi_name);
    switch (obj->o_which) {
	case MM_JUG:
	    if (obj->o_ac == JUG_EMPTY)
		strcat(buf1, " [empty]");
	    else if (p_know[obj->o_ac])
		sprintf(buf1, " [containing a potion of %s (%s)]",
			p_magic[obj->o_ac].mi_name,
			p_colors[obj->o_ac]);
	    else sprintf(buf1, " [containing a%s %s liquid]", 
			vowelstr(p_colors[obj->o_ac]),
			p_colors[obj->o_ac]);
	when MM_BEAKER:		
	case MM_BOOK: {
	    sprintf(buf1, " [containing %d]", obj->o_ac);
	}
	when MM_OPEN:
	case MM_HUNGER:
	    sprintf(buf1, " [%d ring%s]", obj->o_charges, 
			  obj->o_charges == 1 ? "" : "s");
	when MM_DRUMS:
	    sprintf(buf1, " [%d beat%s]", obj->o_charges, 
			  obj->o_charges == 1 ? "" : "s");
	when MM_DISAPPEAR:
	case MM_CHOKE:
	    sprintf(buf1, " [%d pinch%s]", obj->o_charges, 
			  obj->o_charges == 1 ? "" : "es");
	when MM_KEOGHTOM:
	    sprintf(buf1, " [%d application%s]", obj->o_charges, 
			  obj->o_charges == 1 ? "" : "s");
	when MM_SKILLS:
	    switch (obj->o_ac) {
	    case C_MAGICIAN:	strcpy(buf1, " [magic user]");
	    when C_FIGHTER:	strcpy(buf1, " [fighter]");
	    when C_CLERIC:	strcpy(buf1, " [cleric]");
	    when C_THIEF:	strcpy(buf1, " [thief]");
	    }
    }
    strcat (buf, buf1);
    return buf;
}

use_emori()
{
    char selection;	/* Cloak function */
    int state = 0;	/* Menu state */

    msg("What do you want to do? (* for a list): ");
    do {
	selection = tolower(readchar());
	switch (selection) {
	    case '*':
	      if (state != 1) {
		wclear(hw);
		touchwin(hw);
		mvwaddstr(hw, 2, 0,  "[1] Fly\n[2] Stop flying\n");
		waddstr(hw,	     "[3] Turn invisible\n[4] Turn Visible\n");
		mvwaddstr(hw, 0, 0, "What do you want to do? ");
		draw(hw);
		state = 1;	/* Now in prompt window */
	      }
	      break;

	    case ESCAPE:
		if (state == 1) {
		    clearok(cw, TRUE); /* Set up for redraw */
		    touchwin(cw);
		}
		msg("");

		after = FALSE;
		return;

	    when '1':
	    case '2':
	    case '3':
	    case '4':
		if (state == 1) {	/* In prompt window */
		    clearok(cw, TRUE); /* Set up for redraw */
		    touchwin(cw);
		}

		msg("");

		state = 2;	/* Finished */
		break;

	    default:
		if (state == 1) {	/* In the prompt window */
		    mvwaddstr(hw, 0, 0,
				"Please enter a selection between 1 and 4:  ");
		    draw(hw);
		}
		else {	/* Normal window */
		    mpos = 0;
		    msg("Please enter a selection between 1 and 4:  ");
		}
	}
    } while (state != 2);

    /* We now must have a selection between 1 and 4 */
    switch (selection) {
	case '1':	/* Fly */
	    if (on(player, ISFLY)) {
		extinguish(land);	/* Extinguish in case of potion */
		msg("%slready flying.", terse ? "A" : "You are a");
	    }
	    else {
		msg("You feel lighter than air!");
		turn_on(player, ISFLY);
	    }
	when '2':	/* Stop flying */
	    if (off(player, ISFLY))
		msg("%sot flying.", terse ? "N" : "You are n");
	    else {
		if (find_slot(land))
		    msg("%sot flying by the cloak.",
			terse ? "N" : "You are n");
		else land();
	    }
	when '3':	/* Turn invisible */
	    if (off(player, ISINVIS)) {
		turn_on(player, ISINVIS);
		msg("You have a tingling feeling all over your body");
		PLAYER = IPLAYER;
		light(&hero);
	    }
	    else {
		extinguish(appear);	/* Extinguish in case of potion */
		extinguish(dust_appear);/* dust of disappearance        */
		msg("%slready invisible.", terse ? "A" : "You are a");
	    }
	when '4':	/* Turn visible */
	    if (off(player, ISINVIS))
		msg("%sot invisible.", terse ? "N" : "You are n");
	    else {
		if (find_slot(appear) || find_slot(dust_appear))
		    msg("%sot invisible by the cloak.",
			terse ? "N" : "You are n");
		else appear();
	    }
    }
}

use_mm(which)
int which;
{
    register struct object *obj = NULL;
    register struct linked_list *item = NULL;
    bool cursed, blessed, is_mm;
    char buf[LINELEN];

    cursed = FALSE;
    is_mm = FALSE;

    if (which < 0) {	/* A real miscellaneous magic item  */
	is_mm = TRUE;
	item = get_item(pack, "use", USEABLE);
	/*
	 * Make certain that it is a micellaneous magic item
	 */
	if (item == NULL)
	    return;

	obj = OBJPTR(item);
	cursed = (obj->o_flags & ISCURSED) != 0;
	blessed = (obj->o_flags & ISBLESSED) != 0;
	which = obj->o_which;
    }

    if (obj->o_type == RELIC) {		/* An artifact */
	is_mm = FALSE;
	switch (obj->o_which) {
	    case EMORI_CLOAK:
		use_emori();
	    when BRIAN_MANDOLIN:
		/* Put monsters around us to sleep */
		read_scroll(S_HOLD, 0, FALSE);
	    when GERYON_HORN:
		/* Chase close monsters away */
		msg("The horn blasts a shrill tone.");
		do_panic();
	    when HEIL_ANKH:
	    case YENDOR_AMULET:
		/* Nothing happens by this mode */
		msg("Nothing happens.");
	}
    }
    else switch (which) {		/* Miscellaneous Magic */
	/*
	 * the jug of alchemy manufactures potions when you drink
	 * the potion it will make another after a while
	 */
	case MM_JUG:
	    if (obj->o_ac == JUG_EMPTY) {
		msg("The jug is empty");
		break;
	    }
	    quaff (obj->o_ac, NULL, FALSE);
	    obj->o_ac = JUG_EMPTY;
	    fuse (alchemy, obj, ALCHEMYTIME, AFTER);
	    if (!(obj->o_flags & ISKNOW))
	        whatis(item);

	/*
	 * the beaker of plentiful potions is used to hold potions
	 * the book of infinite spells is used to hold scrolls
	 */
	when MM_BEAKER:
	case MM_BOOK:
	    do_bag(item);

	/*
	 * the chime of opening opens up secret doors
	 */
	when MM_OPEN:
	{
	    register struct linked_list *exit;
	    register struct room *rp;
	    register coord *cp;

	    if (obj->o_charges <= 0) {
		msg("The chime is cracked!");
		break;
	    }
	    obj->o_charges--;
	    msg("chime... chime... hime... ime... me... e...");
	    if ((rp = roomin(&hero)) == NULL) {
		search(FALSE, TRUE); /* Non-failing search for door */
		break;
	    }
	    for (exit = rp->r_exit; exit != NULL; exit = next(exit)) {
		cp = DOORPTR(exit);
		if (winat(cp->y, cp->x) == SECRETDOOR) {
		    mvaddch (cp->y, cp->x, DOOR);
		    if (cansee (cp->y, cp->x))
			mvwaddch(cw, cp->y, cp->x, DOOR);
		}
	    }
	}

	/*
	 * the chime of hunger just makes the hero hungry
	 */
	when MM_HUNGER:
	    if (obj->o_charges <= 0) {
		msg("The chime is cracked!");
		break;
	    }
	    obj->o_charges--;
	    food_left = MORETIME + 5;
	    msg(terse ? "Getting hungry" : "You are starting to get hungry");
	    hungry_state = F_HUNGRY;
	    aggravate();

	/*
	 * the drums of panic make all creatures within two squares run
	 * from the hero in panic unless they save or they are mindless
	 * undead
	 */
	when MM_DRUMS:
	    if (obj->o_charges <= 0) {
		msg("The drum is broken!");
		break;
	    }
	    obj->o_charges--;
	/*
	 * dust of disappearance makes the player invisible for a while
	 */
	when MM_DISAPPEAR:
	    m_know[MM_DISAPPEAR] = TRUE;
	    if (obj->o_charges <= 0) {
		msg("No more dust!");
		break;
	    }
	    obj->o_charges--;
	    msg("aaAAACHOOOooo. Cough. Cough. Sneeze. Sneeze.");
	    if (!find_slot(dust_appear)) {
		turn_on(player, ISINVIS);
		fuse(dust_appear, 0, DUSTTIME, AFTER);
		PLAYER = IPLAYER;
		light(&hero);
	    }
	    else lengthen(dust_appear, DUSTTIME);

	/*
	 * dust of choking and sneezing can kill the hero if he misses
	 * the save
	 */
	when MM_CHOKE:
	    m_know[MM_CHOKE] = TRUE;
	    if (obj->o_charges <= 0) {
		msg("No more dust!");
		break;
	    }
	    obj->o_charges--;
	    msg("aaAAACHOOOooo. Cough. Cough. Sneeze. Sneeze.");
	    if (!save(VS_POISON, &player, 0)) {
		msg ("You choke to death!!! --More--");
		pstats.s_hpt = -1; /* in case he hangs up the phone */
		wait_for(cw,' ');
		death(D_CHOKE);
	    }
	    else {
		msg("You begin to cough and choke uncontrollably");
		if (find_slot(unchoke))
		    lengthen(unchoke, DUSTTIME);
		else
		    fuse(unchoke, 0, DUSTTIME, AFTER);
		turn_on(player, ISHUH);
		turn_on(player, ISBLIND);
		light(&hero);
	    }
		
	when MM_KEOGHTOM:
	    /*
	     * this is a very powerful healing ointment
	     * but it takes a while to put on...
	     */
	    if (obj->o_charges <= 0) {
		msg("The jar is empty!");
		break;
	    }
	    obj->o_charges--;
	    waste_time();
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
	    pstats.s_hpt += roll(pstats.s_lvl, 6);
	    if (pstats.s_hpt > max_stats.s_hpt)
		pstats.s_hpt = max_stats.s_hpt;
	    sight();
	    msg("You begin to feel much better.");
		
	/*
	 * The book has a character class associated with it.
	 * if your class matches that of the book, it will raise your 
	 * level by one. If your class does not match the one of the book, 
	 * it change your class to that of book.
	 * Note that it takes a while to read.
	 */
	when MM_SKILLS:
	    detach (pack, item);
	    inpack--;
	    waste_time();
	    waste_time();
	    waste_time();
	    waste_time();
	    waste_time();
	    if (obj->o_ac == player.t_ctype) {
		msg("You feel more skillful");
		raise_level(TRUE);
	    }
	    else {
		/*
		 * reset his class and then use check_level to reset hit
		 * points and the right level for his exp pts
		 * drop exp pts by 10%
		 */
		long save;

		msg("You feel like a whole new person!");
		/*
		 * if he becomes a thief he has to have leather armor
		 */
		if (obj->o_ac == C_THIEF			&&
		    cur_armor != NULL				&&
		    cur_armor->o_which != LEATHER		&&
		    cur_armor->o_which != STUDDED_LEATHER ) 
			cur_armor->o_which = STUDDED_LEATHER;
		/*
		 * if he's changing from a fighter then may have to change
		 * his sword since only fighter can use two-handed
		 * and bastard swords
		 */
		if (player.t_ctype == C_FIGHTER			&&
		    cur_weapon != NULL				&&
		    cur_weapon->o_type == WEAPON		&&
		   (cur_weapon->o_which== BASWORD		||
		    cur_weapon->o_which== TWOSWORD )) 
			cur_weapon->o_which = SWORD;

		/*
		 * if he was a thief then take out the trap_look() daemon
		 */
		if (player.t_ctype == C_THIEF)
		    kill_daemon(trap_look);
		/*
		 * if he becomes a thief then add the trap_look() daemon
		 */
		if (obj->o_ac == C_THIEF)
		    daemon(trap_look, 0, AFTER);
		char_type = player.t_ctype = obj->o_ac;
		save = pstats.s_hpt;
		max_stats.s_hpt = pstats.s_hpt = 0;
		max_stats.s_lvl = pstats.s_lvl = 0; 
		max_stats.s_exp = pstats.s_exp -= pstats.s_exp/10;
		check_level(TRUE);
		if (pstats.s_hpt > save) /* don't add to current hits */
		    pstats.s_hpt = save;
	    }

	otherwise:
	    msg("What a strange magic item you have!");
    }
    status(FALSE);
    if (is_mm && m_know[which] && m_guess[which]) {
	free(m_guess[which]);
	m_guess[which] = NULL;
    }
    else if (is_mm && 
	     !m_know[which] && 
	     askme &&
	     (obj->o_flags & ISKNOW) == 0 &&
	     m_guess[which] == NULL) {
	msg(terse ? "Call it: " : "What do you want to call it? ");
	if (get_str(buf, cw) == NORM) {
	    m_guess[which] = new((unsigned int) strlen(buf) + 1);
	    strcpy(m_guess[which], buf);
	}
    }
    if (item != NULL && which == MM_SKILLS)
	o_discard(item);
    updpack(TRUE);
}
