/*
 * All the fighting gets done here
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
#include <string.h>
#include "rogue.h"

#define CONF_DAMAGE	-1
#define PARAL_DAMAGE	-2
#define DEST_DAMAGE	-3

static const struct matrix att_mat[5] = {
/* Base		Max_lvl,	Factor,		Offset,		Range */
{  10,		25,		2,		1,		2 },
{  9,		18,		2,		1,		5 },
{  10,		19,		2,		1,		3 },
{  10,		21,		2,		1,		4 },
{   7,		25,		1,		0,		2 }
};

/*
 * fight:
 *	The player attacks the monster.
 */

fight(mp, weap, thrown)
register coord *mp;
struct object *weap;
bool thrown;
{
    register struct thing *tp;
    register struct linked_list *item;
    register bool did_hit = TRUE;
    bool back_stab = FALSE;

    /*
     * Find the monster we want to fight
     */
    if ((item = find_mons(mp->y, mp->x)) == NULL) {
	return(FALSE); /* must have killed him already */
    }
    tp = THINGPTR(item);
    /*
     * Since we are fighting, things are not quiet so no healing takes
     * place.
     */
    player.t_quiet = 0;
    tp->t_quiet = 0;

    /*
     * if its in the wall, we can't hit it
     */
    if (on(*tp, ISINWALL) && off(player, CANINWALL))
	return(FALSE);

    /*
     * Let him know it was really a mimic (if it was one).
     */
    if (on(*tp, ISDISGUISE) && (tp->t_type != tp->t_disguise) &&
	off(player, ISBLIND))
    {
	msg("Wait! That's a %s!", monsters[tp->t_index].m_name);
	turn_off(*tp, ISDISGUISE);
	did_hit = thrown;
    }
    if (on(*tp, CANSURPRISE) && off(player, ISBLIND) && !ISWEARING(R_ALERT)) {
	msg("Wait! There's a %s!", monsters[tp->t_index].m_name);
	turn_off(*tp, CANSURPRISE);
	did_hit = thrown;
    }
    /*
     * if he's a thief and the creature is asleep then he gets a chance
     * for a backstab
     */
    if (player.t_ctype == C_THIEF			 	 && 
	(!on(*tp, ISRUN) || on(*tp, ISHELD) || tp->t_no_move > 0)&&
	!on(*tp, NOSTAB))
	back_stab = TRUE;

    runto(tp, &hero);

    if (did_hit)
    {
	register const char *mname;

	did_hit = FALSE;
	mname = (on(player, ISBLIND)) ? "it" : monsters[tp->t_index].m_name;
	if (!can_blink(tp) &&
	    ( ((weap != NULL) && (weap->o_type == RELIC)) ||
	     ((off(*tp, MAGICHIT)  || ((weap != NULL) && (weap->o_hplus > 0 || weap->o_dplus > 0)) ) &&
	      (off(*tp, BMAGICHIT) || ((weap != NULL) && (weap->o_hplus > 1 || weap->o_dplus > 1)) ) &&
	      (off(*tp, CMAGICHIT) || ((weap != NULL) && (weap->o_hplus > 2 || weap->o_dplus > 2)) ) ) )
	    && roll_em(&player, tp, weap, thrown, cur_weapon, back_stab))
	{
	    did_hit = TRUE;

	    if (on(*tp, NOMETAL) && weap != NULL &&
		weap->o_type != RELIC && weap->o_flags & ISMETAL) {
		sprintf(outstring,"Your %s passes right through the %s!",
		    weaps[weap->o_which].w_name, mname);
		msg(outstring);
	    }
	    else if (thrown) {
		tp->t_wasshot = TRUE;
		thunk(weap, tp, mname);
	    }
	    else
		hit(weap, tp, NULL, mname, back_stab);

	    /* If the player hit a rust monster, he better have a + weapon */
	    if (on(*tp, CANRUST) && !thrown && (weap != NULL) &&
		weap->o_type != RELIC &&
		(weap->o_flags & ISMETAL) &&
		!(weap->o_flags & ISPROT) &&
		(weap->o_hplus < 1) && (weap->o_dplus < 1)) {
		if (rnd(100) < 50) weap->o_hplus--;
		else weap->o_dplus--;
		msg(terse ? "Your %s weakens!"
			  : "Your %s appears to be weaker now!",
		    weaps[weap->o_which].w_name);
	    }
		
	    /* If the player hit something that shrieks, wake the dungeon */
	    if (on(*tp, CANSHRIEK)) {
		turn_off(*tp, CANSHRIEK);
		msg("The %s emits a piercing shriek.", mname);
		aggravate();
	    }

	    /* If the player hit something that can surprise, it can't now */
	    if (on(*tp, CANSURPRISE)) turn_off(*tp, CANSURPRISE);


	    /* 
	     * Can the player confuse? 
	     */
	    if (on(player, CANHUH) && !thrown) {
		msg("Your hands stop glowing red");
		msg("The %s appears confused.", mname);
		turn_on(*tp, ISHUH);
		turn_off(player, CANHUH);
	    }
	    /*
	     * does the creature explode when hit?
	     */
	    if (on(*tp, CANEXPLODE))
		explode(tp);

	    /* 
	     * Merchants just disappear if hit 
	     */
	    if (on(*tp, CANSELL)) {
		msg("The %s disappears with his wares in a flash.",mname);
		killed(item, FALSE, FALSE);
	    }

	    else if (tp->t_stats.s_hpt <= 0)
		killed(item, TRUE, TRUE);

	    /* If the monster is fairly intelligent and about to die, it
	     * may turn tail and run.
	     */
	    else if ((tp->t_stats.s_hpt < max(10, tp->maxstats.s_hpt/10)) &&
		     (rnd(25) < tp->t_stats.s_intel)) {
		turn_on(*tp, ISFLEE);

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
	else {
	    if (thrown)
		bounce(weap, tp, mname);
	    else
		miss(weap, tp, NULL, mname);
	}
    }
    count = 0;
    return did_hit;
}

/*
 * attack:
 *	The monster attacks the player
 */

attack(mp, weapon, thrown)
register struct thing *mp;
register struct object *weapon;
bool thrown;
{
    register const char *mname;
    register bool did_hit = FALSE;
    register struct object *wielded;	/* The wielded weapon */

    /*
     * Since this is an attack, stop running and any healing that was
     * going on at the time.
     */
    running = FALSE;
    player.t_quiet = 0;
    mp->t_quiet = 0;

    if (on(*mp, ISDISGUISE) && off(player, ISBLIND))
	turn_off(*mp, ISDISGUISE);
    mname = on(player, ISBLIND) ? "it" : monsters[mp->t_index].m_name;

    /*
     * Try to find a weapon to wield.  Wield_weap will return a
     * projector if weapon is a projectile (eg. bow for arrow).
     * If weapon is NULL, it will try to find a suitable weapon.
     */
    wielded = wield_weap(weapon, mp);
    if (weapon == NULL) weapon = wielded;

    if (roll_em(mp, &player, weapon, thrown, wielded, FALSE)) {
	did_hit = TRUE;

	if (thrown) m_thunk(weapon, mp, mname);
	else hit(weapon, mp, mname, NULL, FALSE);

	if (pstats.s_hpt <= 0)
	    death(mp->t_index);	/* Bye bye life ... */

	/*
	 * suprising monsters appear after they shoot at you 
	 */
	if (thrown) {
	    if (on(*mp, CANSURPRISE)) 
		turn_off(*mp, CANSURPRISE);
	}
	if (!thrown) {
	    /*
	     * If a vampire hits, it may take half your hit points
	     */
	    if (on(*mp, CANSUCK) && !save(VS_MAGIC, &player, 0)) {
		if (pstats.s_hpt == 1) death(mp->t_index);
		else {
		  pstats.s_hpt /= 2;
		  msg("You feel your life force being drawn from you.");
		}
	    }

	    /*
	     * Stinking monsters make player weaker (to hit)
	     */
	    if (on(*mp, CANSTINK)) {
		turn_off(*mp, CANSTINK);
		if (!save(VS_POISON, &player, 0)) {
		    msg("The stench of the %s sickens you.", mname);
		    if (on(player, HASSTINK)) lengthen(unstink, STINKTIME);
		    else {
			turn_on(player, HASSTINK);
			fuse(unstink, 0, STINKTIME, AFTER);
		    }
		}
	    }

	    /*
	     * Chilling monster reduces strength each time
	     */
	    if (on(*mp, CANCHILL)) {
		if (!ISWEARING(R_SUSABILITY) && !save(VS_POISON, &player, 0)) {
		    msg("You cringe at the %s's chilling touch.", mname);
		    chg_str(-1);
		    if (lost_str++ == 0)
			fuse(res_strength, 0, CHILLTIME, AFTER);
		    else lengthen(res_strength, CHILLTIME);
		}
	    }

	    /*
	     * itching monsters reduce dexterity (temporarily)
	     */
	    if (on(*mp, CANITCH) && !save(VS_POISON, &player, 0)) {
		msg("The claws of the %s scratch you", mname);
		if(ISWEARING(R_SUSABILITY)) {
		    msg("The scratch has no effect");
		}
		else {
		    turn_on(player, HASITCH);
		    add_dexterity(TRUE);
		    lost_dext++;
		    fuse(un_itch, 0, roll(HEALTIME,SICKTIME), AFTER);
		}
	    }


	    /*
	     * If a hugging monster hits, it may SQUEEEEEEEZE
	     */
	    if (on(*mp, CANHUG)) {
		if (roll(1,20) >= 18 || roll(1,20) >= 18) {
		    msg("The %s squeezes you against itself.", mname);
		    if ((pstats.s_hpt -= roll(2,8)) <= 0)
			death(mp->t_index);
		}
	    }

	    /*
	     * If a disease-carrying monster hits, there is a chance the
	     * player will catch the disease
	     */
	    if (on(*mp, CANDISEASE) &&
		(rnd(pstats.s_const) < mp->t_stats.s_lvl) &&
		off(player, HASDISEASE)) {
		if (ISWEARING(R_HEALTH)) msg("The wound heals quickly.");
		else {
		    turn_on(player, HASDISEASE);
		    fuse(cure_disease, 0, roll(HEALTIME,SICKTIME), AFTER);
		    msg(terse ? "You have been diseased."
			: "You have contracted a disease!");
		}
	    }

	    /*
	     * If a rust monster hits, you lose armor
	     */
	    if (on(*mp, CANRUST)) { 
		if (cur_armor != NULL				&&
		    cur_armor->o_which != LEATHER		&&
		    cur_armor->o_which != STUDDED_LEATHER	&&
		    cur_armor->o_which != PADDED_ARMOR		&&
		    !(cur_armor->o_flags & ISPROT)		&&
		    cur_armor->o_ac < pstats.s_arm+1		) {
			msg(terse ? "Your armor weakens"
			    : "Your armor appears to be weaker now. Oh my!");
			cur_armor->o_ac++;
	        }
		if (cur_misc[WEAR_BRACERS] != NULL		&&
		    cur_misc[WEAR_BRACERS]->o_ac > 0		&&
		    !(cur_misc[WEAR_BRACERS]->o_flags & ISPROT)) {
			cur_misc[WEAR_BRACERS]->o_ac--;
			if (cur_misc[WEAR_BRACERS]->o_ac == 0) {
			    register struct linked_list *item;

			    for (item=pack; item!=NULL; item=next(item)) {
				if (OBJPTR(item) == cur_misc[WEAR_BRACERS]) {
				    detach(pack, item);
				    o_discard(item);
				    break;
				}
			    }
			    msg ("Your bracers crumble and fall off!");
			    cur_misc[WEAR_BRACERS] = NULL;
			    inpack--;
			}
			else {
			    msg("Your bracers weaken!");
			}
		}
	    }
	    /*
	     * If can dissolve and hero has leather type armor
	     */
	    if (on(*mp, CANDISSOLVE) && cur_armor != NULL &&
		(cur_armor->o_which == LEATHER		  ||
		 cur_armor->o_which == STUDDED_LEATHER	  ||
		 cur_armor->o_which == PADDED_ARMOR)	  &&
		!(cur_armor->o_flags & ISPROT) &&
		cur_armor->o_ac < pstats.s_arm+1) {
		msg(terse ? "Your armor dissolves"
		    : "Your armor appears to dissolve. Oh my!");
		cur_armor->o_ac++;
	    }

	    /* If a surprising monster hit you, you can see it now */
	    if (on(*mp, CANSURPRISE)) turn_off(*mp, CANSURPRISE);

	    /*
	     * If an infesting monster hits you, you get a parasite or rot
	     */
	    if (on(*mp, CANINFEST) && rnd(pstats.s_const) < mp->t_stats.s_lvl) {
		if (ISWEARING(R_HEALTH)) msg("The wound quickly heals.");
		else {
		    turn_off(*mp, CANINFEST);
		    msg(terse ? "You have been infested."
			: "You have contracted a parasitic infestation!");
		    infest_dam++;
		    turn_on(player, HASINFEST);
		}
	    }

	    /*
	     * Ants have poisonous bites
	     */
	    if (on(*mp, CANPOISON) && !save(VS_POISON, &player, 0)) {
		if (ISWEARING(R_SUSABILITY))
		    msg(terse ? "Sting has no effect"
			      : "A sting momentarily weakens you");
		else {
		    chg_str(-1);
		    msg(terse ? "A sting has weakened you" :
		    "You feel a sting in your arm and now feel weaker");
		}
	    }
	    /*
	     * does it take wisdom away?
	     */
	    if (on(*mp, TAKEWISDOM)		&& 
		!save(VS_MAGIC, &player, 0)	&&
		!ISWEARING(R_SUSABILITY)) {
			add_wisdom(TRUE);
	    }
	    /*
	     * does it take intelligence away?
	     */
	    if (on(*mp, TAKEINTEL)		&& 
		!save(VS_MAGIC, &player, 0)	&&
		!ISWEARING(R_SUSABILITY)) {
			add_intelligence(TRUE);
	    }
	    /*
	     * Cause fear by touching
	     */
	    if (on(*mp, TOUCHFEAR)) {
		turn_off(*mp, TOUCHFEAR);
		if (!ISWEARING(R_HEROISM)	&&
		    !save(VS_WAND, &player, 0)	&&
		    !(on(player, ISFLEE) && (player.t_dest == &mp->t_pos))) {
			turn_on(player, ISFLEE);
			player.t_dest = &mp->t_pos;
			msg("The %s's touch terrifies you.", mname);
		}
	    }

	    /*
	     * make the hero dance (as in otto's irresistable dance)
	     */
	    if (on(*mp, CANDANCE) 	&& 
		!on(player, ISDANCE)	&&
		!save(VS_MAGIC, &player, -4)) {
		    turn_off(*mp, CANDANCE);
		    turn_on(player, ISDANCE);
		    msg("You begin to dance uncontrollably!");
		    fuse(undance, 0, roll(2,4), AFTER);
	    }

	    /*
	     * Suffocating our hero
	     */
	    if (on(*mp, CANSUFFOCATE) && (rnd(100) < 15) &&
		(find_slot(suffocate) == FALSE)) {
		turn_on(*mp, DIDSUFFOCATE);
		msg("The %s is beginning to suffocate you.", mname);
		fuse(suffocate, 0, roll(4,2), AFTER);
	    }

	    /*
	     * Turning to stone
	     */
	    if (on(*mp, TOUCHSTONE)) {
		turn_off(*mp, TOUCHSTONE);
		if (on(player, CANINWALL))
			msg("The %s's touch has no effect.", mname);
		else {
		    if (!save(VS_PETRIFICATION, &player, 0) && rnd(100) < 15) {
		        msg("Your body begins to solidify.");
		        msg("You are turned to stone !!! --More--");
		        wait_for(cw,' ');
			death(D_PETRIFY);
		    }
		    else {
			msg("The %s's touch stiffens your limbs.", mname);
			no_command += STONETIME;
		    }
		}
	    }

	    /*
	     * Wraiths might drain energy levels
	     */
	    if ((on(*mp, CANDRAIN) || on(*mp, DOUBLEDRAIN)) && rnd(100) < 15) {
		lower_level(mp->t_index);
		if (on(*mp, DOUBLEDRAIN)) lower_level(mp->t_index);
		turn_on(*mp, DIDDRAIN);  
	    }

	    /*
	     * Violet fungi stops the poor guy from moving
	     */
	    if (on(*mp, CANHOLD) && off(*mp, DIDHOLD)) {
		turn_on(player, ISHELD);
		turn_on(*mp, DIDHOLD);
		hold_count++;
	    }

	    /*
	     * Sucker will suck blood and run
	     */
	    if (on(*mp, CANDRAW)) {
		turn_off(*mp, CANDRAW);
		turn_on(*mp, ISFLEE);
		msg("The %s sates itself with your blood.", mname);
		if ((pstats.s_hpt -= 12) <= 0) death(mp->t_index);
	    }

	    /*
	     * Bad smell will force a reduction in strength
	     */
	    if (on(*mp, CANSMELL)) {
		turn_off(*mp, CANSMELL);
		if (save(VS_MAGIC, &player, 0) || ISWEARING(R_SUSABILITY))
		    msg("You smell an unpleasant odor.");
		else {
		    int odor_str = -(rnd(6)+1);

		    msg("You are overcome by a foul odor.");
		    if (lost_str == 0) {
			chg_str(odor_str);
			fuse(res_strength, 0, SMELLTIME, AFTER);
			lost_str -= odor_str;
		    }
		    else lengthen(res_strength, SMELLTIME);
		}
	    }

	    /*
	     * Paralyzation
	     */
	    if (on(*mp, CANPARALYZE)) {
		turn_off(*mp, CANPARALYZE);
		if (!save(VS_PARALYZATION, &player, 0)) {
		    if (on(player, CANINWALL))
			msg("The %s's touch has no effect.", mname);
		    else {
			msg("The %s's touch paralyzes you.", mname);
			no_command += FREEZETIME;
		    }
		}
	    }

	    /*
	     * Painful wounds make you faint
	     */
	     if (on(*mp, CANPAIN)) {
		turn_off(*mp, CANPAIN);
		if (!ISWEARING(R_ALERT) && !save(VS_POISON, &player, 0)) {
			msg("You faint from the painful wound");
			no_command += PAINTIME;
		}
	    }

	    /*
	     * The monsters touch slows the hero down
	     */
	     if (on(*mp, CANSLOW)) {
		turn_off(*mp, CANSLOW);
		if (!save(VS_PARALYZATION, &player, 0)) 
			add_slow();
	    }

	    /*
	     * Rotting
	     */
	    if (on(*mp, CANROT)) {
		if (!ISWEARING(R_HEALTH) && 
		    !save(VS_POISON, &player, 0)     && 
		    off(player, DOROT)) {
		    turn_on(player, DOROT);
		    msg("You feel your skin starting to rot away!");
		}
	    }

	    if (on(*mp, STEALGOLD)) {
		/*
		 * steal some gold
		 */
		register long lastpurse;
		register struct linked_list *item;
		register struct object *obj;

		lastpurse = purse;
		purse -= GOLDCALC + GOLDCALC;
		if (!save(VS_MAGIC, &player, mp->t_stats.s_lvl/10)) {
		    if (on(*mp, ISUNIQUE))
			purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		    purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		}
		if (purse < 0)
		    purse = 0;
		if (purse != lastpurse) {
		    msg("Your purse feels lighter");

		    /* Give the gold to the thief */
		    for (item=mp->t_pack; item != NULL; item=next(item)) {
			obj = OBJPTR(item);
			if (obj->o_type == GOLD) {
			    obj->o_count += lastpurse - purse;
			    break;
			}
		    }

		    /* Did we do it? */
		    if (item == NULL) {	/* Then make some */
			item = new_item(sizeof *obj);
			obj = OBJPTR(item);
			obj->o_type = GOLD;
			obj->o_count = lastpurse - purse;
			obj->o_hplus = obj->o_dplus = 0;
			strcpy(obj->o_damage,"0d0");
                        strcpy(obj->o_hurldmg,"0d0");
			obj->o_ac = 11;
			obj->contents = NULL;
			obj->o_group = 0;
			obj->o_flags = 0;
			obj->o_mark[0] = '\0';
			obj->o_pos = mp->t_pos;

			attach(mp->t_pack, item);
		    }
		}

		turn_on(*mp, ISFLEE);
		turn_on(*mp, ISINVIS);
	    }

	    if (on(*mp, STEALMAGIC)) {
		register struct linked_list *list, *steal;
		register struct object *obj;
		register int nobj;

		/*
		 * steal a magic item, look through the pack
		 * and pick out one we like.
		 */
		steal = NULL;
		for (nobj = 0, list = pack; list != NULL; list = next(list))
		{
		    obj = OBJPTR(list);
		    if (!is_current(obj) &&
			is_magic(obj)	 && 
			rnd(++nobj) == 0)
			    steal = list;
		}
		if (steal != NULL)
		{
		    register struct object *obj;
		    struct linked_list *item;

		    obj = OBJPTR(steal);
		    if (on(*mp, ISUNIQUE))
			monsters[mp->t_index].m_normal = TRUE;
		    item = find_mons(mp->t_pos.y, mp->t_pos.x);
		    killed(item, FALSE, FALSE);
		    if (obj->o_count > 1 && obj->o_group == 0) {
			register int oc;

			oc = --(obj->o_count);
			obj->o_count = 1;
			sprintf(outstring,"The %s stole %s!", mname, inv_name(obj, TRUE));
			msg(outstring);
			obj->o_count = oc;
		    }
		    else {
			sprintf(outstring,"The %s stole %s!", mname, inv_name(obj, TRUE));
			msg(outstring);
			detach(pack, steal);

			/* If this is a relic, clear its holding field */
			if (obj->o_type == RELIC)
			    cur_relic[obj->o_which] = 0;

			o_discard(steal);
		        inpack--;
		    }
		    updpack(FALSE);
		}
	    }
	}
    }
    else {
	/* If the thing was trying to surprise, no good */
	if (on(*mp, CANSURPRISE)) turn_off(*mp, CANSURPRISE);

	else if (thrown) m_bounce(weapon, mp, mname);
	else miss(weapon, mp, mname, NULL);
    }
    if (fight_flush)
	md_flushinp();
    count = 0;
    status(FALSE);
    return(did_hit);
}

/*
 * swing:
 *	returns true if the swing hits
 */

swing(class, at_lvl, op_arm, wplus)
short class;
int at_lvl, op_arm, wplus;
{
    register int res = rnd(20)+1;
    register int need;

    need = att_mat[class].base -
	   att_mat[class].factor *
	   ((min(at_lvl, att_mat[class].max_lvl) -
	    att_mat[class].offset)/att_mat[class].range) +
	   (10 - op_arm);
    if (need > 20 && need <= 25) need = 20;

    return (res+wplus >= need);
}

/*
 * roll_em:
 *	Roll several attacks
 */

roll_em(att_er, def_er, weap, hurl, cur_weapon, back_stab)
struct thing *att_er, *def_er;
struct object *weap;
bool hurl;
struct object *cur_weapon;
bool back_stab;
{
    register struct stats *att, *def;
    register char *cp = NULL;
    register int ndice, nsides, nplus, def_arm;
    bool did_hit = FALSE;
    int prop_hplus, prop_dplus;
    int vampiric_damage;

    /* Get statistics */
    att = &att_er->t_stats;
    def = &def_er->t_stats;

    prop_hplus = prop_dplus = 0;
    if (weap == NULL)
	cp = att->s_dmg;
    else if (hurl) {
	if ((weap->o_flags&ISMISL) && cur_weapon != NULL &&
	  cur_weapon->o_which == weap->o_launch)
	{
	    cp = weap->o_hurldmg;
	    prop_hplus = cur_weapon->o_hplus;
	    prop_dplus = cur_weapon->o_dplus;
	}
	else
	    cp = (weap->o_flags&ISMISL ? weap->o_damage : weap->o_hurldmg);
    }
    else {
	if (weap->o_type == RELIC) {
	    switch (weap->o_which) {
		case MUSTY_DAGGER:	cp = "1d4+1/1d4+1";
		when YEENOGHU_FLAIL:	cp = "3d6/paralyze/confuse";
		when HRUGGEK_MSTAR:	cp = "3d10";
		when MING_STAFF:	cp = "1d8";
		when ASMO_ROD:		cp = "2d8+1";
		when ORCUS_WAND:	cp = "destroy";
	    }
	}
	else cp = weap->o_damage;
	/*
	 * Drain a staff of striking
	 */
	if (weap->o_type == STICK && weap->o_which == WS_HIT
	    && weap->o_charges == 0)
		{
		    strcpy(weap->o_damage,"0d0");
		    weap->o_hplus = weap->o_dplus = 0;
		}
    }
    for (;;)
    {
	int damage;
	int hplus = prop_hplus;
	int dplus = prop_dplus;

	if (weap != NULL && weap->o_type == RELIC) {
	    switch (weap->o_which) {
		case MUSTY_DAGGER:
		    if (att != &pstats || /* Not player or good stats */
			(str_compute() > 15 && dex_compute() > 15)) {

			hplus += 6;
			dplus += 6;

			/* Give an additional strength and dex bonus */
			if (att == &pstats) {
			    hplus += str_plus(str_compute()) +
				     dext_plus(dex_compute());
			    dplus += dext_plus(dex_compute()) +
				     add_dam(str_compute());
			}
			else {
			    hplus += str_plus(att->s_str) +
				     dext_plus(att->s_dext);
			    dplus += dext_plus(att->s_dext) +
				     add_dam(att->s_str);
			}
		    }
		    else {
			hplus -= 3;
			dplus -= 3;
		    }
		when YEENOGHU_FLAIL:
		case HRUGGEK_MSTAR:
		    hplus += 3;
		    dplus += 3;
		when MING_STAFF:
		    hplus += 2;
		    dplus += 2;
	    }
	}
	else if (weap != NULL) {
	    hplus += weap->o_hplus;
	    dplus += weap->o_dplus;
	}

	/* Is attacker weak? */
	if (on(*att_er, HASSTINK)) hplus -= 2;

	if (att == &pstats)	/* Is the attacker the player? */
	{
	    hplus += hitweight();	/* adjust for encumberence */
	    dplus += hung_dam();	/* adjust damage for hungry player */
	    dplus += ring_value(R_ADDDAM);
	}
	if (back_stab || (weap && att != &pstats && on(*att_er, CANBSTAB)))
	    hplus += 4;	/* add in pluses for backstabbing */

	/* Get the damage */
	while (isspace(*cp)) cp++;
	if (!isdigit(*cp)) {
	    if (strncmp(cp, "confuse", 7) == 0) ndice = CONF_DAMAGE;
	    else if (strncmp(cp, "paralyze", 8) == 0) ndice = PARAL_DAMAGE;
	    else if (strncmp(cp, "destroy", 7) == 0) ndice = DEST_DAMAGE;
	    else ndice = 0;
	    nsides = 0;
	    nplus = 0;
	}
	else {
	    char *oldcp;

	    /* Get the number of damage dice */
	    ndice = atoi(cp);
	    if ((cp = strchr(cp, 'd')) == NULL)
		break;

	    /* Skip the 'd' and get the number of sides per die */
	    nsides = atoi(++cp);

	    /* Check for an addition -- save old place in case none is found */
	    oldcp = cp;
	    if ((cp = strchr(cp, '+')) != NULL) nplus = atoi(++cp);
	    else {
		nplus = 0;
		cp = oldcp;
	    }
	}

	if (def == &pstats) { /* Monster attacks player */
	    def_arm = ac_compute() - dext_prot(dex_compute());
	    hplus += str_plus(att->s_str)+dext_plus(att->s_dext);
	}
	else {	/* Player attacks monster */
	    def_arm = def->s_arm - dext_prot(def->s_dext);
	    hplus += str_plus(str_compute())+dext_plus(dex_compute());
	}

	if (swing(att_er->t_ctype, att->s_lvl, def_arm, hplus)) {
	    register int proll;

	    /* Take care of special effects */
	    switch (ndice) {
	      case CONF_DAMAGE:
		if (def == &pstats) { /* Monster attacks player */
		    if (!save(VS_MAGIC, &player, 0) && off(player, ISCLEAR)) {
			msg("You feel disoriented.");
			if (find_slot(unconfuse))
			    lengthen(unconfuse, rnd(8)+HUHDURATION);
			else
			    fuse(unconfuse, 0, rnd(8)+HUHDURATION, AFTER);
			turn_on(player, ISHUH);
		    }
		    else msg("You feel dizzy, but it quickly passes.");
		}
		/* Player hits monster */
		else if (!save(VS_MAGIC, def_er, 0) && off(*def_er, ISCLEAR)) { 
		    msg("The artifact warms with pleasure.");
		    turn_on(*def_er, ISHUH);
		}
		did_hit = TRUE;
	      when PARAL_DAMAGE:
		if (def == &pstats) { /* Monster attacks player */
		    if (!save(VS_MAGIC, &player, 0) && off(player, CANINWALL)) {
			msg("You stiffen up.");
			no_command += FREEZETIME;
		    }
		}
		else if (!save(VS_MAGIC, def_er, 0)) { /* Player hits monster */
		    msg("The artifact hums happily.");
		    turn_off(*def_er, ISRUN);
		    turn_on(*def_er, ISHELD);
		}
		did_hit = TRUE;
	      when DEST_DAMAGE:
		if (def == &pstats) { /* Monster attacks player */
		    msg("You feel a tug at your life force.");
		    if (!save(VS_MAGIC, &player, -4)) {
			msg("The wand devours your soul.");
			def->s_hpt = 0;
		    }
		}
		/* Player hits monster */
		else if (!save(VS_MAGIC, def_er, -4)) {
		    msg("The artifact draws energy.");

		    /* Give the player half the monster's hits */
		    att->s_hpt += def->s_hpt/2;
		    if (att->s_hpt > att_er->maxstats.s_hpt)
			att->s_hpt = att_er->maxstats.s_hpt;

		    /* Kill the monster */
		    def->s_hpt = 0;
		}
		did_hit = TRUE;
	      otherwise:
		/* Heil's ankh always gives maximum damage */
		if (att == &pstats && cur_relic[HEIL_ANKH])
		    proll = ndice * nsides;
		else proll = roll(ndice, nsides);

		if (ndice + nsides > 0 && proll < 1)
		    debug("Damage for %dd%d came out %d.",
				ndice, nsides, proll);
		damage = dplus + proll + nplus;
		if (def == &pstats)
		    damage += add_dam(att->s_str);
		else
		    damage += add_dam(str_compute());

		/* Check for half damage monsters */
		if (on(*def_er, HALFDAMAGE)) damage /= 2;

		/* add in multipliers for backstabbing */
		if (back_stab || 
		    (weap && att != &pstats && on(*att_er, CANBSTAB))) {
		    int mult = 2 + (att->s_lvl-1)/4; /* Normal multiplier */

		    if (mult > 5 && att != &pstats) 
			mult = 5;/*be nice with maximum of 5x for monsters*/
		    if (weap->o_type == RELIC && weap->o_which == MUSTY_DAGGER)
			mult++;
		    damage *= mult;
		}

		/* Check for no-damage and division */
		if (on(*def_er, BLOWDIVIDE)) {
		    damage = 0;
		    creat_mons(def_er, def_er->t_index, FALSE);
		    light(&hero);
		}
		/* check for immunity to metal -- RELICS are always bad */
		if (on(*def_er, NOMETAL) && weap != NULL &&
		    weap->o_type != RELIC && weap->o_flags & ISMETAL) {
		    damage = 0;
		}

		/*
		 * If defender is wearing a cloak of displacement -- no damage
		 * the first time. (unless its a hurled magic missile)
		 */
		if ( ((weap == NULL) || weap->o_type != MISSILE) &&
		    def == &pstats				 &&	
		    cur_misc[WEAR_CLOAK] != NULL		 &&
		    cur_misc[WEAR_CLOAK]->o_which == MM_DISP &&
		    off(*att_er, MISSEDDISP)) {
		    damage = 0;
		    did_hit = FALSE;
		    turn_on(*att_er, MISSEDDISP);
		    if (cansee(att_er->t_pos.y, att_er->t_pos.x) &&
			!invisible(att_er))
			msg("The %s looks amazed",
				monsters[att_er->t_index].m_name);
		}
		else {
		    def->s_hpt -= max(0, damage);	/* Do the damage */
		    did_hit = TRUE;
		}

		vampiric_damage = damage;
		if (def->s_hpt < 0)	/* only want REAL damage inflicted */
		    vampiric_damage += def->s_hpt;
                if (vampiric_damage < 0)
                    vampiric_damage = 0;
		if (att == &pstats && ISWEARING(R_VAMPREGEN) && !hurl) {
		    if ((pstats.s_hpt += vampiric_damage/2) > max_stats.s_hpt)
			pstats.s_hpt = max_stats.s_hpt;
		}
		debug ("hplus=%d dmg=%d", hplus, damage);
	    }
	}
	if ((cp = strchr(cp, '/')) == NULL)
	    break;
	cp++;
    }
    return did_hit;
}

/*
 * prname:
 *	The print name of a combatant
 */

char *
prname(who, upper)
register char *who;
bool upper;
{
    static char tbuf[LINELEN];

    *tbuf = '\0';
    if (who == 0)
	strcpy(tbuf, "you"); 
    else if (on(player, ISBLIND))
	strcpy(tbuf, "it");
    else
    {
	strcpy(tbuf, "the ");
	strcat(tbuf, who);
    }
    if (upper)
	*tbuf = toupper(*tbuf);
    return tbuf;
}

/*
 * hit:
 *	Print a message to indicate a succesful hit
 */

hit(weapon, tp, er, ee, back_stab)
register struct object *weapon;
register struct thing *tp;
register char *er, *ee;
bool back_stab;
{
    register char *s = NULL;
    char 
    	          att_name[80],	/* Name of attacker */
		  def_name[80];/* Name of defender */
    bool see_monst = !invisible(tp);	/* Can the player see the monster? */

    /* What do we call the attacker? */
    if (er == NULL) {	/* Player is attacking */
	strcpy(att_name, prname(er, TRUE));
	strcpy(def_name, see_monst ? prname(ee, FALSE) : "something");
    }
    else {
	strcpy(att_name, see_monst ? prname(er, TRUE) : "Something");

	/* If the monster is using a weapon and we can see it, report it */
	if (weapon != NULL && see_monst) {
	    strcat(att_name, "'s ");
	    strcat(att_name, weap_name(weapon));
	}

	strcpy(def_name, prname(ee, FALSE));
    }

    addmsg(att_name);
    if (terse) {
	if (back_stab)
	    s = " backstab!";
	else
	    s = " hit.";
    }
    else {
	if (back_stab)
	    s = (er == 0 ? " have backstabbed " : " has backstabbed ");
	else {
	    switch (rnd(4))
	    {
		case 0: s = " scored an excellent hit on ";
		when 1: s = " hit ";
		when 2: s = (er == 0 ? " have injured " : " has injured ");
		when 3: s = (er == 0 ? " swing and hit " : " swings and hits ");
	    }
	}
    }
    addmsg(s);
    if (!terse)
	addmsg(def_name);
    endmsg();
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */

miss(weapon, tp, er, ee)
register struct object *weapon;
register struct thing *tp;
register char *er, *ee;
{
    register char *s = NULL;
    char
    	          att_name[80],	/* Name of attacker */
		  def_name[80];/* Name of defender */
    bool see_monst = !invisible(tp);	/* Can the player see the monster? */

    /* What do we call the attacker? */
    if (er == NULL) {	/* Player is attacking */
	strcpy(att_name, prname(er, TRUE));
	strcpy(def_name, see_monst ? prname(ee, FALSE) : "something");
    }
    else {
	strcpy(att_name, see_monst ? prname(er, TRUE) : "Something");

	/* If the monster is using a weapon and we can see it, report it */
	if (weapon != NULL && see_monst) {
	    strcat(att_name, "'s ");
	    strcat(att_name, weap_name(weapon));
	}

	strcpy(def_name, prname(ee, FALSE));
    }

    addmsg(att_name);
    switch (terse ? 0 : rnd(4))
    {
	case 0: s = (er == 0 ? " miss" : " misses");
	when 1: s = (er == 0 ? " swing and miss" : " swings and misses");
	when 2: s = (er == 0 ? " barely miss" : " barely misses");
	when 3: s = (er == 0 ? " don't hit" : " doesn't hit");
    }
    addmsg(s);
    if (!terse)
	addmsg(" %s", def_name);
    endmsg();
}

/*
 * dext_plus:
 *	compute to-hit bonus for dexterity
 */

dext_plus(dexterity)
register int dexterity;
{
	return (dexterity > 10 ? (dexterity-13)/3 : (dexterity-10)/3);
}


/*
 * dext_prot:
 *	compute armor class bonus for dexterity
 */

dext_prot(dexterity)
register int dexterity;
{
    return ((dexterity-10)/2);
}
/*
 * str_plus:
 *	compute bonus/penalties for strength on the "to hit" roll
 */

str_plus(str)
register short str;
{
    return((str-10)/3);
}

/*
 * add_dam:
 *	compute additional damage done for exceptionally high or low strength
 */

add_dam(str)
register short str;
{
    return((str-9)/2);
}

/*
 * hung_dam:
 *	Calculate damage depending on players hungry state
 */
hung_dam()
{
	reg int howmuch = 0;

	switch(hungry_state) {
		case F_OKAY:
		case F_HUNGRY:	howmuch = 0;
		when F_WEAK:	howmuch = -1;
		when F_FAINT:	howmuch = -2;
	}
	return howmuch;
}

/*
 * thunk:
 *	A missile hits a monster
 */

thunk(weap, tp, mname)
register struct object *weap;
register struct thing *tp;	/* Defender */
register char *mname;
{
    char *def_name;	/* Name of defender */

    /* What do we call the defender? */
    if (!cansee(tp->t_pos.y, tp->t_pos.x) || invisible(tp))
	def_name = "something";
    else def_name = prname(mname, FALSE);

    if (weap->o_type == WEAPON){
	sprintf(outstring,"The %s hits %s", weaps[weap->o_which].w_name, def_name);
	msg(outstring);
    }
    else if (weap->o_type == MISSILE){
	sprintf(outstring,"The %s hits %s",ws_magic[weap->o_which].mi_name, def_name);
	msg(outstring);
    }
    else
	msg("You hit %s.", def_name);
}

/*
 * mthunk:
 *	 A missile from a monster hits the player
 */

m_thunk(weap, tp, mname)
register struct object *weap;
register struct thing *tp;
register char *mname;
{
    char *att_name;	/* Name of attacker */

    /* What do we call the attacker? */
    if (!cansee(tp->t_pos.y, tp->t_pos.x) || invisible(tp))
	att_name = "Something";
    else att_name = prname(mname, TRUE);

    if (weap->o_type == WEAPON){
	sprintf(outstring,"%s's %s hits you.", att_name, weaps[weap->o_which].w_name);
	msg(outstring);
    }
    else if (weap->o_type == MISSILE){
	sprintf(outstring,"%s's %s hits you.", att_name, ws_magic[weap->o_which].mi_name);
	msg(outstring);
    }
    else
	msg("%s hits you.", att_name);
}

/*
 * bounce:
 *	A missile misses a monster
 */

bounce(weap, tp, mname)
register struct object *weap;
register struct thing *tp;	/* Defender */
register char *mname;
{
    char *def_name;	/* Name of defender */

    /* What do we call the defender? */
    if (!cansee(tp->t_pos.y, tp->t_pos.x) || invisible(tp))
	def_name = "something";
    else def_name = prname(mname, FALSE);

    if (weap->o_type == WEAPON){
	sprintf(outstring,"The %s misses %s",weaps[weap->o_which].w_name, def_name);
	msg(outstring);
    }
    else if (weap->o_type == MISSILE){
	sprintf(outstring,"The %s misses %s",ws_magic[weap->o_which].mi_name, def_name);
	msg(outstring);
    }
    else
	msg("You missed %s.", def_name);
}

/*
 * m_bounce:
	  A missle from a monster misses the player
 */

m_bounce(weap, tp, mname)
register struct object *weap;
register struct thing *tp;
register char *mname;
{
    char *att_name;	/* Name of attacker */

    /* What do we call the attacker? */
    if (!cansee(tp->t_pos.y, tp->t_pos.x) || invisible(tp))
	att_name = "Something";
    else att_name = prname(mname, TRUE);

    if (weap->o_type == WEAPON){
	sprintf(outstring,"%s's %s misses you.", att_name, weaps[weap->o_which].w_name);
	msg(outstring);
    }
    else if (weap->o_type == MISSILE){
	sprintf(outstring,"%s's %s misses you.", att_name, ws_magic[weap->o_which].mi_name);
	msg(outstring);
    }
    else
	msg("%s misses you.", att_name);
}


/*
 * is_magic:
 *	Returns true if an object radiates magic
 */

is_magic(obj)
register struct object *obj;
{
    switch (obj->o_type)
    {
	case ARMOR:
	    return obj->o_ac != armors[obj->o_which].a_class;
	when WEAPON:
	    return obj->o_hplus != 0 || obj->o_dplus != 0;
	when POTION:
	case SCROLL:
	case STICK:
	case RING:
	case MM:
	case RELIC:
	    return TRUE;
    }
    return FALSE;
}

/*
 * killed:
 *	Called to put a monster to death
 */

killed(item, pr, points)
register struct linked_list *item;
bool pr, points;
{
    register struct thing *tp;
    register struct linked_list *pitem, *nexti;
    const char *monst;

    tp = THINGPTR(item);

    if (pr)
    {
	addmsg(terse ? "Defeated " : "You have defeated ");
	if (on(player, ISBLIND))
	    msg("it.");
	else
	{
	    if (cansee(tp->t_pos.y, tp->t_pos.x) && !invisible(tp))
		monst = monsters[tp->t_index].m_name;
	    else {
		if (terse) monst = "something";
		else monst = "thing";
	    }
	    if (!terse)
		addmsg("the ");
	    msg("%s.", monst);
	}
    }

    /* Take care of any residual effects of the monster */
    check_residue(tp);

    if (points) {
	unsigned long test;	/* For overflow check */

	test = pstats.s_exp + tp->t_stats.s_exp;

	/* Do an overflow check before increasing experience */
	if (test > pstats.s_exp) pstats.s_exp = test;

	/*
	 * Do adjustments if he went up a level
	 */
	check_level(TRUE);
    }

    /*
     * Empty the monsters pack
     */
    pitem = tp->t_pack;

    /*
     * Get rid of the monster.
     */
    mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, ' ');
    mvwaddch(cw, tp->t_pos.y, tp->t_pos.x, tp->t_oldch); 
    detach(mlist, item);
    /*
     * empty his pack
     */
    while (pitem != NULL)
    {
	nexti = next(tp->t_pack);
	(OBJPTR(pitem))->o_pos = tp->t_pos;
	detach(tp->t_pack, pitem);
	if (points) 
	    fall(pitem, FALSE);
	else 
	    o_discard(pitem);
	pitem = nexti;
    }
    turn_on(*tp, ISDEAD);
    attach(monst_dead, item);
}


/*
 * Returns a pointer to the weapon the monster is wielding corresponding to
 * the given thrown weapon.  If no thrown item is given, try to find any
 * decent weapon.
 */

struct object *
wield_weap(thrown, mp)
struct object *thrown;
struct thing *mp;
{
    int look_for = 0,	/* The projectile weapon we are looking for */
	new_rate,	/* The rating of a prospective weapon */
        cand_rate = -1; /* Rating of current candidate -- higher is better */
    register struct linked_list *pitem;
    register struct object *obj, *candidate = NULL;

    if (thrown != NULL) {	/* Using a projectile weapon */
      switch (thrown->o_which) {
	case BOLT:	look_for = CROSSBOW;	/* Find the crossbow */
	when ARROW:	look_for = BOW;		/* Find the bow */
	when ROCK:	look_for = SLING;	/* find the sling */
	otherwise:	return(NULL);
      }
    }
    else if (off(*mp, ISUNIQUE) && off(*mp, CARRYWEAPON)) return(NULL);

    for (pitem=mp->t_pack; pitem; pitem=next(pitem)) {
	obj = OBJPTR(pitem);

	/*
	 * If we have a thrown weapon, just return the first match
	 * we come to.
	 */
	if (thrown != NULL && obj->o_type == WEAPON && obj->o_which == look_for)
	    return(obj);

	/* If we have a usable RELIC, return it */
	if (thrown == NULL && obj->o_type == RELIC) {
	    switch (obj->o_which) {
		case MUSTY_DAGGER:
		case YEENOGHU_FLAIL:
		case HRUGGEK_MSTAR:
		case MING_STAFF:
		case ASMO_ROD:
		case ORCUS_WAND:
		    return(obj);
	    }
	}

	/* Otherwise if it's a usable weapon, it is a good candidate */
	else if (thrown == NULL && obj->o_type == WEAPON) {
	    switch (obj->o_which) {
		case DAGGER:
		    new_rate = 0;
		when BATTLEAXE:
		    new_rate = 1;
		when MACE:
		    new_rate = 2;
		when SWORD:
		    new_rate = 3;
		when PIKE:
		    new_rate = 4;
		when HALBERD:
		case SPETUM:
		    new_rate = 6;
		when BARDICHE:
		    new_rate = 7;
		when TRIDENT:
		    new_rate = 8;
		when BASWORD:
		    new_rate = 9;
		when TWOSWORD:
		    new_rate = 10;
		otherwise:
		    new_rate = -1;
	    }

	    /* Only switch if this is better than the current candidate */
	    if (new_rate > cand_rate) {
		cand_rate = new_rate;
		candidate = obj;
	    }
	}
    }

    return(candidate);
}

explode(tp)
register struct thing *tp;
{

    register int x,y, damage;
    struct linked_list *item;
    struct thing *th;

    msg("the %s explodes!", monsters[tp->t_index].m_name);
    /*
     * check to see if it got the hero
     */
     if (DISTANCE(hero.x, hero.y, tp->t_pos.x, tp->t_pos.y) <= 25) {
	msg("the explosion hits you");
	damage = roll(6,6);
	if (save(VS_WAND, &player, 0))
	    damage /= 2;
	if ((pstats.s_hpt -= damage) <= 0)
	    death(tp->t_index);
    }

    /*
     * now check for monsters in vicinity
     */
     for (x = tp->t_pos.x-5; x<=tp->t_pos.x+5; x++) {
	 if (x < 0 || x > COLS - 1) 
	     continue;
	 for (y = tp->t_pos.y-5; y<=tp->t_pos.y+5; y++) {
	    if (y < 1 || y > LINES - 3)
		continue;
	    if (isalpha(mvwinch(mw, y, x))) {
		if ((item = find_mons(y, x)) != NULL) {
		    th = THINGPTR(item);
		    if (th == tp) /* don't count gas spore */
			continue;
		    damage = roll(6, 6);
		    if (save(VS_WAND, th, 0))
			damage /= 2;
		    if ((tp->t_stats.s_hpt -= damage) <= 0) {
			msg("the explosion kills the %s",
				monsters[th->t_index].m_name);
			killed(item, FALSE, FALSE);
		    }
		}
	    }
	}
    }
}
