/*
 * This file contains misc functions for dealing with armor
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
#include "rogue.h"


/*
 * take_off:
 *	Get the armor off of the players back
 */

take_off()
{
    register struct object *obj;
    register struct linked_list *item;

    /* What does player want to take off? */
    if ((item = get_item(pack, "take off", REMOVABLE)) == NULL)
	return;

    obj = OBJPTR(item);
    if (!is_current(obj)) {
	sprintf(outstring,"Not wearing %c) %s", pack_char(pack, obj),inv_name(obj, TRUE));
	msg(outstring);
	return;
    }

    /* Can the player remove the item? */
    if (!dropcheck(obj)) return;
    updpack(TRUE);

    sprintf(outstring,"Was wearing %c) %s", pack_char(pack, obj),inv_name(obj,TRUE));
    msg(outstring);
}

/*
 * wear:
 *	The player wants to wear something, so let him/her put it on.
 */

wear()
{
    register struct linked_list *item;
    register struct object *obj;
    register int i;
    char buf[LINELEN];


    /* What does player want to wear? */
    if ((item = get_item(pack, "wear", WEARABLE)) == NULL)
	return;

    obj = OBJPTR(item);

    switch (obj->o_type) {
	case ARMOR:
	    if (cur_armor != NULL) {
		addmsg("You are already wearing armor");
		if (!terse) addmsg(".  You'll have to take it off first.");
		endmsg();
		after = FALSE;
		return;
	    }
	    if (cur_misc[WEAR_BRACERS] != NULL) {
		msg("You can't wear armor with bracers of defense.");
		return;
	    }
	    if (cur_misc[WEAR_CLOAK] != NULL || cur_relic[EMORI_CLOAK]) {
		msg("You can't wear armor with a cloak.");
		return;
	    }
	    if (player.t_ctype == C_THIEF	&&
		(obj->o_which != LEATHER && obj->o_which != STUDDED_LEATHER)) {
		if (terse) msg("Thieves can't wear that type of armor");
		else
		 msg("Thieves can only wear leather or studded leather armor");
		return;
	    }
	    waste_time();
	    obj->o_flags |= ISKNOW;
	    cur_armor = obj;
	    addmsg(terse ? "W" : "You are now w");
	    msg("earing %s.", armors[obj->o_which].a_name);

	when MM:
	    switch (obj->o_which) {
	    /*
	     * when wearing the boots of elvenkind the player will not
	     * set off any traps
	     */
	    case MM_ELF_BOOTS:
		if (cur_misc[WEAR_BOOTS] != NULL)
		    msg("already wearing a pair of boots");
		else {
		    msg("wearing %s",inv_name(obj,TRUE));
		    cur_misc[WEAR_BOOTS] = obj;
		}
	    /*
	     * when wearing the boots of dancing the player will dance
	     * uncontrollably
	     */
	    when MM_DANCE:
		if (cur_misc[WEAR_BOOTS] != NULL)
		    msg("already wearing a pair of boots");
		else {
		    msg("wearing %s",inv_name(obj,TRUE));
		    cur_misc[WEAR_BOOTS] = obj;
		    msg("You begin to dance uncontrollably!");
		    turn_on(player, ISDANCE);
		}
	    /*
	     * bracers give the hero protection in he same way armor does.
	     * they cannot be used with armor but can be used with cloaks
	     */
	    when MM_BRACERS:
		if (cur_misc[WEAR_BRACERS] != NULL)
			msg("already wearing bracers");
		else {
		    if (cur_armor != NULL) {
			msg("You can't wear bracers of defense with armor.");
		    }
		    else {
			msg("wearing %s",inv_name(obj,TRUE));
			cur_misc[WEAR_BRACERS] = obj;
		    }
		}

	    /*
	     * The robe (cloak) of powerlessness disallows any spell casting
	     */
	    when MM_R_POWERLESS:
	    /*
	     * the cloak of displacement gives the hero an extra +2 on AC
	     * and saving throws. Cloaks cannot be used with armor.
	     */
	    case MM_DISP:
	    /*
	     * the cloak of protection gives the hero +n on AC and saving
	     * throws with a max of +3 on saves
	     */
	    case MM_PROTECT:
		if (cur_misc[WEAR_CLOAK] != NULL || cur_relic[EMORI_CLOAK])
		    msg("%slready wearing a cloak.", terse ? "A"
							   : "You are a");
		else {
		    if (cur_armor != NULL) {
			msg("You can't wear a cloak with armor.");
		    }
		    else {
			msg("wearing %s",inv_name(obj,TRUE));
			cur_misc[WEAR_CLOAK] = obj;
		    }
		}
	    /*
	     * the gauntlets of dexterity give the hero a dexterity of 18
	     * the gauntlets of ogre power give the hero a strength of 18
	     * the gauntlets of fumbling cause the hero to drop his weapon
	     */
	    when MM_G_DEXTERITY:
	    case MM_G_OGRE:
	    case MM_FUMBLE:
		if (cur_misc[WEAR_GAUNTLET] != NULL)
		    msg("Already wearing a pair of gauntlets.");
		else {
		    msg("wearing %s",inv_name(obj,TRUE));
		    cur_misc[WEAR_GAUNTLET] = obj;
		    if (obj->o_which == MM_FUMBLE)
			daemon(fumble, 0, AFTER);
		}
	    /*
	     * the jewel of attacks does an aggavate monster
	     */
	    when MM_JEWEL:
		if (cur_misc[WEAR_JEWEL] != NULL || cur_relic[YENDOR_AMULET])
		    msg("Already wearing an amulet.");
		else {
		    msg("wearing %s",inv_name(obj,TRUE));
		    cur_misc[WEAR_JEWEL] = obj;
		    aggravate();
		}
	    /*
	     * the necklace of adaption makes the hero immune to
	     * chlorine gas
	     */
	    when MM_ADAPTION:
		if (cur_misc[WEAR_NECKLACE] != NULL)
		    msg("already wearing a necklace");
		else {
		    msg("wearing %s",inv_name(obj,TRUE));
		    cur_misc[WEAR_NECKLACE] = obj;
		    turn_on(player, NOGAS);
		}
	    /*
	     * the necklace of stragulation will try to strangle the
	     * hero to death
	     */
	    when MM_STRANGLE:
		if (cur_misc[WEAR_NECKLACE] != NULL)
		    msg("already wearing a necklace");
		else {
		    msg("wearing %s",inv_name(obj,TRUE));
		    cur_misc[WEAR_NECKLACE] = obj;
		    msg("The necklace is beginning to strangle you!");
		    daemon(strangle, 0, AFTER);
		}
	    otherwise:
		msg("what a strange item you have!");
	    }
	    status(FALSE);
	    if (m_know[obj->o_which] && m_guess[obj->o_which]) {
		free(m_guess[obj->o_which]);
		m_guess[obj->o_which] = NULL;
	    }
	    else if (!m_know[obj->o_which] && 
		     askme &&
		     (obj->o_flags & ISKNOW) == 0 &&
		     m_guess[obj->o_which] == NULL) {
		msg(terse ? "Call it: " : "What do you want to call it? ");
		if (get_str(buf, cw) == NORM) {
		    m_guess[obj->o_which] = new((unsigned int) strlen(buf) + 1);
		    strcpy(m_guess[obj->o_which], buf);
		}
	    }

	when RING:
	    if (cur_misc[WEAR_GAUNTLET] != NULL) {
		msg ("You have to remove your gauntlets first!");
		return;
	    }

	    /* If there is room, put on the ring */
	    for (i=0; i<NUM_FINGERS; i++)
	        if (cur_ring[i] == NULL) {
		    cur_ring[i] = obj;
		    break;
		}
	    if (i == NUM_FINGERS) {	/* No room */
		if (terse) msg("Wearing enough rings");
		else msg("You already have on eight rings");
		return;
	    }

	    /* Calculate the effect of the ring */
	    ring_on(obj);
    }
    updpack(TRUE);
}
