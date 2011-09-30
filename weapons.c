/*
 * Functions for dealing with problems brought about by weapons
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
 * do the actual motion on the screen done by an object traveling
 * across the room
 */
do_motion(obj, ydelta, xdelta, tp)
register struct object *obj;
register int ydelta, xdelta;
register struct thing *tp;
{

	/*
	* Come fly with us ...
	*/
	obj->o_pos = tp->t_pos;
	for (; ;) {
		register int ch;
		/*
		* Erase the old one
		*/
		if (!ce(obj->o_pos, tp->t_pos) &&
		    cansee(unc(obj->o_pos)) &&
		    mvwinch(cw, obj->o_pos.y, obj->o_pos.x) != ' ') {
			mvwaddch(cw, obj->o_pos.y, obj->o_pos.x, show(obj->o_pos.y, obj->o_pos.x));
		}
		/*
		* Get the new position
		*/
		obj->o_pos.y += ydelta;
		obj->o_pos.x += xdelta;
		if (shoot_ok(ch = winat(obj->o_pos.y, obj->o_pos.x)) && ch != DOOR && !ce(obj->o_pos, hero)) {
			/*
			* It hasn't hit anything yet, so display it
			* If it alright.
			*/
			if (cansee(unc(obj->o_pos)) &&
			    mvwinch(cw, obj->o_pos.y, obj->o_pos.x) != ' ') {
				mvwaddch(cw, obj->o_pos.y, obj->o_pos.x, obj->o_type);
				draw(cw);
			}
			continue;
		}
		break;
	}
}


/*
 * fall:
 *	Drop an item someplace around here.
 */

fall(item, pr)
register struct linked_list *item;
bool pr;
{
	register struct object *obj;
	register struct room *rp;
	register int i;
	coord *fpos = NULL;

	obj = OBJPTR(item);
	/*
	 * try to drop the item, look up to 3 squares away for now
	 */
	for (i=1; i<4; i++) {
	    if ((fpos = fallpos(&obj->o_pos, FALSE, i)) != NULL)
		break;
	}

	if (fpos != NULL) {
		mvaddch(fpos->y, fpos->x, obj->o_type);
		obj->o_pos = *fpos;
		if ((rp = roomin(&hero)) != NULL &&
		    lit_room(rp)) {
			light(&hero);
			mvwaddch(cw, hero.y, hero.x, PLAYER);
		}
		attach(lvl_obj, item);
		return;
	}


	if (pr) {
            if (obj->o_type == WEAPON) /* BUGFIX: Identification trick */
                msg("The %s vanishes as it hits the ground.", 
                    weaps[obj->o_which].w_name);
            else
                msg("%s vanishes as it hits the ground.", inv_name(obj,TRUE));
	}
	o_discard(item);
}


/*
 * Does the missile hit the monster
 */

hit_monster(y, x, obj, tp)
register int y, x;
struct object *obj;
register struct thing *tp;
{
	static coord mp;

	mp.y = y;
	mp.x = x;
	if (tp == &player) {
		/* Make sure there is a monster where it landed */
		if (!isalpha(mvwinch(mw, y, x))) {
			return(FALSE);
		}
		return(fight(&mp, obj, TRUE));
	} else {
		if (!ce(mp, hero)) {
			return(FALSE);
		}
		return(attack(tp, obj, TRUE));
	}
}

/*
 * init_weapon:
 *	Set up the initial goodies for a weapon
 */

init_weapon(weap, type)
register struct object *weap;
char type;
{
	register struct init_weps *iwp;

	iwp = &weaps[type];
	strcpy(weap->o_damage,iwp->w_dam);
	strcpy(weap->o_hurldmg,iwp->w_hrl);
	weap->o_launch = iwp->w_launch;
	weap->o_flags = iwp->w_flags;
	weap->o_weight = iwp->w_wght;
	if (weap->o_flags & ISMANY) {
		weap->o_count = rnd(8) + 8;
		weap->o_group = newgrp();
	} else {
		weap->o_count = 1;
	}
}

/*
 * missile:
 *	Fire a missile in a given direction
 */

missile(ydelta, xdelta, item, tp)
int ydelta, xdelta;
register struct linked_list *item;
register struct thing *tp;
{
	register struct object *obj;
	register struct linked_list *nitem;
	char ch;

	/*
	* Get which thing we are hurling
	*/
	if (item == NULL) {
		return;
	}
	obj = OBJPTR(item);

#if 0	/* Do we really want to make this check */
	if (is_current(obj)) {		/* Are we holding it? */
	    msg(terse ? "Holding it." : "You are already holding it.");
	    return;
	}
#endif

	if (!dropcheck(obj)) return;	/* Can we get rid of it? */

	if(!(obj->o_flags & ISMISL)) {
	    for(;;) {
		msg(terse ? "Really throw? (y or n): "
			  : "Do you really want to throw %s? (y or n): ",
				inv_name(obj, TRUE));
		mpos = 0;
		ch = readchar();
		if (ch == 'n' || ch == ESCAPE) {
		    after = FALSE;
		    return;
		}
		if (ch == 'y')
		    break;
	    }
	}
	/*
	 * Get rid of the thing. If it is a non-multiple item object, or
	 * if it is the last thing, just drop it. Otherwise, create a new
	 * item with a count of one.
	 */
	if (obj->o_count < 2) {
		detach(tp->t_pack, item);
		if (tp->t_pack == pack) {
			inpack--;
		}
	} 
	else {
		obj->o_count--;
		nitem = (struct linked_list *) new_item(sizeof *obj);
		obj = OBJPTR(nitem);
		*obj = *(OBJPTR(item));
		obj->o_count = 1;
		item = nitem;
	}
	updpack (FALSE);
	do_motion(obj, ydelta, xdelta, tp);
	/*
	* AHA! Here it has hit something. If it is a wall or a door,
	* or if it misses (combat) the monster, put it on the floor
	*/
	if (!hit_monster(unc(obj->o_pos), obj, tp)) {
		fall(item, TRUE);
	}
	mvwaddch(cw, hero.y, hero.x, PLAYER);
}

/*
 * num:
 *	Figure out the plus number for armor/weapons
 */

char *
num(n1, n2)
register int n1, n2;
{
	static char numbuf[LINELEN];

	if (n1 == 0 && n2 == 0) {
		return "+0";
	}
	if (n2 == 0) {
		sprintf(numbuf, "%s%d", n1 < 0 ? "" : "+", n1);
	} else {
		sprintf(numbuf, "%s%d, %s%d", n1 < 0 ? "" : "+", n1, n2 < 0 ? "" : "+", n2);
	}
	return(numbuf);
}

/*
 * wield:
 *	Pull out a certain weapon
 */

wield()
{
	register struct linked_list *item;
	register struct object *obj, *oweapon;

	if ((oweapon = cur_weapon) != NULL) {
	    if (!dropcheck(cur_weapon)) {
		    cur_weapon = oweapon;
		    return;
	    }
	    if (terse)
		addmsg("Was ");
	    else
		addmsg("You were ");
	    msg("wielding %s", inv_name(oweapon, TRUE));
	}
	if ((item = get_item(pack, "wield", WIELDABLE)) == NULL) {
		after = FALSE;
		return;
	}
	obj = OBJPTR(item);
	if (is_current(obj)) {
		msg("Item in use.");
		after = FALSE;
		return;
	}
	if (player.t_ctype != C_FIGHTER &&
	    obj->o_type == WEAPON	&&
	   (obj->o_which == TWOSWORD || obj->o_which == BASWORD)) {
		msg("Only fighters can wield a %s", weaps[obj->o_which].w_name);
		return;
	}
	if (terse) {
		addmsg("W");
	} else {
		addmsg("You are now w");
	}
	msg("ielding %s", inv_name(obj, TRUE));
	cur_weapon = obj;
}

