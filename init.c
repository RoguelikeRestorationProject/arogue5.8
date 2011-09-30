/*
 * global variable initializaton
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
#include "mach_dep.h"


char *rainbow[NCOLORS] = {

"amber",		"aquamarine",		"beige",
"black",		"blue",			"brown",
"clear",		"crimson",		"ecru",
"gold",			"green",		"grey",
"indigo",		"khaki",		"lavender",
"magenta",		"orange",		"pink",
"plaid",		"purple",		"red",
"silver",		"saffron",		"scarlet",
"tan",			"tangerine", 		"topaz",
"turquoise",		"vermilion",		"violet",
"white",		"yellow",
};

char *sylls[NSYLLS] = {
    "a",   "ab",  "ag",  "aks", "ala", "an",  "ankh","app", "arg", "arze",
    "ash", "ban", "bar", "bat", "bek", "bie", "bin", "bit", "bjor",
    "blu", "bot", "bu",  "byt", "comp","con", "cos", "cre", "dalf",
    "dan", "den", "do",  "e",   "eep", "el",  "eng", "er",  "ere", "erk",
    "esh", "evs", "fa",  "fid", "for", "fri", "fu",  "gan", "gar",
    "glen","gop", "gre", "ha",  "he",  "hyd", "i",   "ing", "ion", "ip",
    "ish", "it",  "ite", "iv",  "jo",  "kho", "kli", "klis","la",  "lech",
    "man", "mar", "me",  "mi",  "mic", "mik", "mon", "mung","mur",
    "nej", "nelg","nep", "ner", "nes", "nes", "nih", "nin", "o",   "od",
    "ood", "org", "orn", "ox",  "oxy", "pay", "pet", "ple", "plu", "po",
    "pot", "prok","re",  "rea", "rhov","ri",  "ro",  "rog", "rok", "rol",
    "sa",  "san", "sat", "see", "sef", "seh", "shu", "ski", "sna",
    "sne", "snik","sno", "so",  "sol", "sri", "sta", "sun", "ta",
    "tab", "tem", "ther","ti",  "tox", "trol","tue", "turs","u",
    "ulk", "um",  "un",  "uni", "ur",  "val", "viv", "vly", "vom", "wah",
    "wed", "werg","wex", "whon","wun", "xo",  "y",   "yot", "yu",
    "zant","zap", "zeb", "zim", "zok", "zon", "zum",
};

char *stones[NSTONES] = {
	"agate",		"alexandrite",		"amethyst",
	"azurite",		"bloodstone",		"cairngorm",
	"carnelian",		"chalcedony",		"chrysoberyl",
	"chrysolite",		"chrysoprase",		"citrine",
	"coral",		"diamond",		"emerald",
	"garnet",		"heliotrope",		"hematite",
	"hyacinth",		"jacinth",		"jade",
	"jargoon",		"jasper",		"kryptonite",
	"lapus lazuli",		"malachite",		"mocca stone",
	"moonstone",		"obsidian",		"olivine",
	"onyx",			"opal",			"pearl",
	"peridot",		"quartz",		"rhodochrosite",
	"rhodolite",		"ruby",			"sapphire",
	"sardonyx",		"serpintine",		"spinel",
	"tiger eye",		"topaz",		"tourmaline",
	"turquoise",		"zircon",
};

char *wood[NWOOD] = {
	"avocado wood",	"balsa",	"banyan",	"birch",
	"cedar",	"cherry",	"cinnibar",	"dogwood",
	"driftwood",	"ebony",	"eucalyptus",	"hemlock",
	"ironwood",	"mahogany",	"manzanita",	"maple",
	"oak",		"pine",		"redwood",	"rosewood",
	"teak",		"walnut",	"zebra wood", 	"persimmon wood",
};

char *metal[NMETAL] = {
	"aluminium",	"bone",		"brass",	"bronze",
	"copper",	"chromium",	"iron",		"lead",
	"magnesium",	"pewter",	"platinum",	"silver",
	"steel",	"tin",		"titanium",	"zinc",
};




/*
 * make sure all the percentages specified in the tables add up to the
 * right amounts
 */
badcheck(name, magic, bound)
char *name;
register struct magic_item *magic;
register int bound;
{
    register struct magic_item *end;

    if (magic[bound - 1].mi_prob == 1000)
	return;
    printf("\nBad percentages for %s:\n", name);
    for (end = &magic[bound] ; magic < end ; magic++)
	printf("%4d%% %s\n", magic->mi_prob, magic->mi_name);
    printf(retstr);
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */

init_colors()
{
    register int i, j;
    bool used[NCOLORS];

    for(i = 0; i < NCOLORS; i++)
        used[i] = FALSE;

    for (i = 0 ; i < MAXPOTIONS ; i++)
    {
	do
	    j = rnd(NCOLORS);
        until (!used[j]);
        used[j] = TRUE;
	p_colors[i] = rainbow[j];
	p_know[i] = FALSE;
	p_guess[i] = NULL;
	if (i > 0)
		p_magic[i].mi_prob += p_magic[i-1].mi_prob;
    }
    badcheck("potions", p_magic, MAXPOTIONS);
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */

init_materials()
{
    register int i, j;
    register char *str;
    bool metused[NMETAL], woodused[NWOOD];

    for(i = 0; i < NWOOD; i++)
        woodused[i] = FALSE;

    for(i = 0; i < NMETAL; i++)
        metused[i] = FALSE;

    for (i = 0 ; i < MAXSTICKS ; i++)
    {
        for (;;)
	    if (rnd(100) > 50)
	    { 
                j = rnd(NMETAL);

                if (!metused[j])
                {
                    ws_type[i] = "wand";
                    str = metal[j];
                    metused[j] = TRUE;
                    break;
                }
            }
            else
            {
                j = rnd(NWOOD);

                if (!woodused[j])
                {
                    ws_type[i] = "staff";
                    str = wood[j];
                    woodused[j] = TRUE;
                    break;
                }
            }

        ws_made[i] = str;
	ws_know[i] = FALSE;
	ws_guess[i] = NULL;
	if (i > 0)
		ws_magic[i].mi_prob += ws_magic[i-1].mi_prob;
    }
    badcheck("sticks", ws_magic, MAXSTICKS);
}

/*
 * do any initialization for miscellaneous magic
 */

init_misc()
{
    register int i;

    for (i=0; i < MAXMM; i++) {
	m_know[i] = FALSE;
	m_guess[i] = NULL;
	if (i > 0)
	    m_magic[i].mi_prob += m_magic[i-1].mi_prob;
    }

    badcheck("miscellaneous magic", m_magic, MAXMM);
}


/*
 * init_names:
 *	Generate the names of the various scrolls
 */

init_names()
{
    register int nsyl;
    register char *cp, *sp;
    register int i, nwords;

    for (i = 0 ; i < MAXSCROLLS ; i++)
    {
	cp = prbuf;
	nwords = rnd(COLS/20) + 1 + (COLS > 40 ? 1 : 0);
	while(nwords--)
	{
	    nsyl = rnd(3)+1;
	    while(nsyl--)
	    {
		sp = sylls[rnd((sizeof sylls) / (sizeof (char *)))];
		while(*sp)
		    *cp++ = *sp++;
	    }
	    *cp++ = ' ';
	}
	*--cp = '\0';
	s_names[i] = (char *) new(strlen(prbuf)+1);
	s_know[i] = FALSE;
	s_guess[i] = NULL;
	strcpy(s_names[i], prbuf);
	if (i > 0)
		s_magic[i].mi_prob += s_magic[i-1].mi_prob;
    }
    badcheck("scrolls", s_magic, MAXSCROLLS);
}

/*
 * init_player:
 *	roll up the rogue
 */

init_player()
{
    int stat_total, ch = 0, wpt = 0, i, j;
    struct linked_list *weap_item, *armor_item, *food_item;
    struct object *obj;
    char *class;

    weap_item = armor_item = NULL;

    if (char_type == -1) {
	/* See what type character will be */
	wclear(hw);
	touchwin(hw);
	mvwaddstr(hw,2,0,"[1] Fighter\n[2] Magician\n[3] Cleric\n[4] Thief");
	mvwaddstr(hw, 0, 0, "What character class do you desire? ");
	draw(hw);
	char_type = (wgetch(hw) - '0');
	while (char_type < 1 || char_type > 4) {
	    mvwaddstr(hw,0,0,"Please enter a character type between 1 and 4: ");
	    draw(hw);
	    char_type = (wgetch(hw) - '0');
	}
	char_type--;
    }
    player.t_ctype = char_type;
    player.t_quiet = 0;
    pack = NULL;

#ifdef WIZARD
    /* 
     * allow me to describe a super character 
     */
    if (wizard && md_getuid() == AUTHOR && strcmp(getenv("SUPER"),"YES") == 0) {
	    pstats.s_str = 25;
	    pstats.s_intel = 25;
	    pstats.s_wisdom = 25;
	    pstats.s_dext = 25;
	    pstats.s_const = 25;
	    pstats.s_charisma = 25;
	    pstats.s_exp = 7500000L;
	    pstats.s_lvl = 20;
	    pstats.s_hpt = 500;
	    pstats.s_carry = totalenc();
	    strcpy(pstats.s_dmg,"3d4");
	    if (player.t_ctype == C_FIGHTER)
		weap_item = spec_item(WEAPON, TWOSWORD, 5, 5);
	    else
		weap_item = spec_item(WEAPON, SWORD, 5, 5);
	    obj = OBJPTR(weap_item);
	    obj->o_flags |= ISKNOW;
	    add_pack(weap_item, TRUE, NULL);
	    cur_weapon = obj;
	    j = PLATE_ARMOR;
	    if (player.t_ctype == C_THIEF)
		j = STUDDED_LEATHER;
	    armor_item = spec_item(ARMOR, j, 10, 0);
	    obj = OBJPTR(armor_item);
	    obj->o_flags |= (ISKNOW | ISPROT);
	    obj->o_weight = armors[j].a_wght;
	    add_pack(armor_item, TRUE, NULL);
	    cur_armor = obj;
	    purse += 10000;
    }
    else 
#endif

    {
	wclear(hw);
	do {
	    if (armor_item != NULL) {
		o_discard(armor_item);
		armor_item = NULL;
	    }
	    if (weap_item != NULL) {
		o_discard(weap_item);
		weap_item = NULL;
	    }
	    pstats.s_lvl = 1;
	    pstats.s_exp = 0L;
	    pstats.s_hpt = 12 + rnd(10);
	    pstats.s_str = 7 + rnd(5);
	    pstats.s_intel = 7 + rnd(5);
	    pstats.s_wisdom = 7 + rnd(5);
	    pstats.s_dext = 7 + rnd(5);
	    pstats.s_const = 14 + rnd(5);
	    pstats.s_charisma = 7 + rnd(5);

	    /* Now for the special ability */
	    switch (char_type) {
		case C_FIGHTER:  pstats.s_str	= (rnd(10) == 7) ? 18 : 16;
		when C_MAGICIAN: pstats.s_intel	= (rnd(10) == 7) ? 18 : 16;
		when C_CLERIC:   pstats.s_wisdom= (rnd(10) == 7) ? 18 : 16;
		when C_THIEF:    pstats.s_dext	= (rnd(10) == 7) ? 18 : 16;
	    }
	    strcpy(pstats.s_dmg,"1d4");
	    stat_total =pstats.s_str  + pstats.s_intel + pstats.s_wisdom +
			pstats.s_dext + pstats.s_const;
	    /*
	     * since the player can re-roll stats at will, keep the maximum
	     * to some reasonable limit
	     */
	    if (stat_total > MAXSTATS)
		pstats.s_const -= (stat_total - MAXSTATS);
	    pstats.s_carry = totalenc();

	    /*
	     * Give the rogue his weaponry.  
	     */
	    do {
		i = rnd(8);	/* number of acceptable weapons */
		switch(i) {
		    case 0: ch = 25; wpt = MACE;
		    when 1: ch = 25; wpt = SWORD;
		    when 2: ch = 20; wpt = BATTLEAXE;
		    when 3: ch = 20; wpt = TRIDENT;
		    when 4: ch = 20; wpt = SPETUM;
		    when 5: ch = 20; wpt = BARDICHE;
		    when 6: ch = 15; wpt = PIKE;
		    when 7: ch = 20; wpt = HALBERD;
		}
	    } while(rnd(100) > ch);
	    if (player.t_ctype == C_FIGHTER)
		wpt = TWOSWORD;
	    weap_item = spec_item(WEAPON, wpt, rnd(2), rnd(2)+1);
	    obj = OBJPTR(weap_item);
	    obj->o_flags |= ISKNOW;
	    /*
	     * And his suit of armor.......
	     * Thieves can only wear leather armor
	     * fighters get better armor on an average
	     */
	    if (player.t_ctype == C_THIEF)
		j = STUDDED_LEATHER;
	    else {
		if (player.t_ctype == C_FIGHTER)
		    i = 50 + rnd(50);
		else
		    i = rnd(100);
		j = 0;
		while (armors[j].a_prob < i)
		    j++;
	    }
	    armor_item = spec_item(ARMOR, j, 0, 0);
	    obj = OBJPTR(armor_item);
	    obj->o_flags |= ISKNOW;
	    obj->o_weight = armors[j].a_wght;
	    switch(player.t_ctype) {
		case C_FIGHTER:	class = "fighter";
		when C_MAGICIAN:class = "magic user";
		when C_CLERIC:	class = "cleric";
		when C_THIEF:	class = "thief";
		otherwise:	class = "unknown";
	    }
	    wmove(hw, 2, 0);
	    wprintw(hw, "You have rolled a %s with the following attributes:",class);
	    wmove(hw,4,0);
	    wprintw(hw, "    Int: %2d", pstats.s_intel);
	    wprintw(hw, "    Str: %2d", pstats.s_str);
	    wprintw(hw, "    Wis: %2d", pstats.s_wisdom); 
	    wprintw(hw, "    Dex: %2d", pstats.s_dext);
	    wprintw(hw, "  Const: %2d", pstats.s_const);
	    wclrtoeol(hw);
	    wmove(hw, 6, 0);
	    wprintw(hw, "     Hp: %2d", pstats.s_hpt);
	    wclrtoeol(hw);
	    mvwaddstr(hw, 8, 5, inv_name(OBJPTR(weap_item), FALSE));
	    wclrtoeol(hw);
	    mvwaddstr(hw, 9, 5, inv_name(OBJPTR(armor_item), FALSE));
	    wclrtoeol(hw);
	    mvwaddstr(hw,0,0,"Would you like to re-roll the character? ");
	    draw(hw);
	} while(wgetch(hw) == 'y');

	obj = OBJPTR(weap_item);
	add_pack(weap_item, TRUE, NULL);
	cur_weapon = obj;
	obj = OBJPTR(armor_item);
	add_pack(armor_item, TRUE, NULL);
	cur_armor = obj;
    }
    /*
     * Give him some food
     */
    food_item = spec_item(FOOD, 0, 0, 0);
    obj = OBJPTR(food_item);
    obj->o_weight = things[TYP_FOOD].mi_wght;
    add_pack(food_item, TRUE, NULL);
    pstats.s_arm = 10;
    max_stats = pstats;
}






/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */

init_stones()
{
    register int i, j;
    bool used[NSTONES];

    for (i = 0; i < NSTONES; i++)
        used[i] = FALSE;

    for (i = 0 ; i < MAXRINGS ; i++)
    {
	do
            j = rnd(NSTONES);
        until (!used[j]);

        used[j] = TRUE;
	r_stones[i] = stones[j];
	r_know[i] = FALSE;
	r_guess[i] = NULL;
	if (i > 0)
		r_magic[i].mi_prob += r_magic[i-1].mi_prob;
    }
    badcheck("rings", r_magic, MAXRINGS);
}

/*
 * init_things
 *	Initialize the probabilities for types of things
 */
init_things()
{
    register struct magic_item *mp;

    for (mp = &things[1] ; mp < &things[NUMTHINGS] ; mp++)
	mp->mi_prob += (mp-1)->mi_prob;
    badcheck("things", things, NUMTHINGS);
}


