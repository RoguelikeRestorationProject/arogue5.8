/*
    state.c - Portable Rogue Save State Code

    Copyright (C) 1999, 2000, 2005 Nicholas J. Kisseberth
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name(s) of the author(s) nor the names of other contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

/************************************************************************/
/* Save State Code                                                      */
/************************************************************************/

#define RSID_STATS        0xABCD0001
#define RSID_MSTATS       0xABCD0002
#define RSID_THING        0xABCD0003
#define RSID_OBJECT       0xABCD0004
#define RSID_MAGICITEMS   0xABCD0005
#define RSID_KNOWS        0xABCD0006
#define RSID_GUESSES      0xABCD0007
#define RSID_OBJECTLIST   0xABCD0008
#define RSID_BAGOBJECT    0xABCD0009
#define RSID_MONSTERLIST  0xABCD000A
#define RSID_MONSTERSTATS 0xABCD000B
#define RSID_MONSTERS     0xABCD000C
#define RSID_TRAP         0xABCD000D
#define RSID_WINDOW       0xABCD000E
#define RSID_DAEMONS      0xABCD000F
#define RSID_STICKS       0xABCD0010
#define RSID_IARMOR       0xABCD0011
#define RSID_SPELLS       0xABCD0012
#define RSID_ILIST        0xABCD0013
#define RSID_HLIST        0xABCD0014
#define RSID_DEATHTYPE    0xABCD0015
#define RSID_CTYPES       0XABCD0016
#define RSID_COORDLIST    0XABCD0017
#define RSID_ROOMS        0XABCD0018

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include "rogue.h"

#define READSTAT (format_error || read_error )
#define WRITESTAT (write_error)

static int read_error   = FALSE;
static int write_error  = FALSE;
static int format_error = FALSE;
static int endian = 0x01020304;
#define  big_endian ( *((char *)&endian) == 0x01 )

int
rs_write(FILE *savef, void *ptr, size_t size)
{
    if (write_error)
        return(WRITESTAT);

    if (encwrite(ptr, size, savef) != size)
        write_error = 1;

    return(WRITESTAT);
}

int
rs_read(int inf, void *ptr, size_t size)
{
    if (read_error || format_error)
        return(READSTAT);

    if (encread(ptr, size, inf) != size)
        read_error = 1;
       
    return(READSTAT);
}

int
rs_write_uchar(FILE *savef, unsigned char c)
{
    if (write_error)
        return(WRITESTAT);

    rs_write(savef, &c, 1);

    return(WRITESTAT);
}

int
rs_read_uchar(int inf, unsigned char *c)
{
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, c, 1);

    return(READSTAT);
}

int
rs_write_char(FILE *savef, char c)
{
    if (write_error)
        return(WRITESTAT);

    rs_write(savef, &c, 1);

    return(WRITESTAT);
}

int
rs_read_char(int inf, char *c)
{
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, c, 1);

    return(READSTAT);
}

int
rs_write_chars(FILE *savef, char *c, int count)
{
    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, count);
    rs_write(savef, c, count);

    return(WRITESTAT);
}

int
rs_read_chars(int inf, char *i, int count)
{
    int value = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &value);
    
    if (value != count)
        format_error = TRUE;

    rs_read(inf, i, count);
    
    return(READSTAT);
}

int
rs_write_int(FILE *savef, int c)
{
    unsigned char bytes[4];
    unsigned char *buf = (unsigned char *) &c;

    if (write_error)
        return(WRITESTAT);

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    rs_write(savef, buf, 4);

    return(WRITESTAT);
}

int
rs_read_int(int inf, int *i)
{
    unsigned char bytes[4];
    int input = 0;
    unsigned char *buf = (unsigned char *)&input;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &input, 4);

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    *i = *((int *) buf);

    return(READSTAT);
}

int
rs_write_ints(FILE *savef, int *c, int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, count);

    for(n = 0; n < count; n++)
        if( rs_write_int(savef,c[n]) != 0)
            break;

    return(WRITESTAT);
}

int
rs_read_ints(int inf, int *i, int count)
{
    int n, value;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < count; n++)
        if (rs_read_int(inf, &i[n]) != 0)
            break;
    
    return(READSTAT);
}

int
rs_write_boolean(FILE *savef, bool c)
{
    unsigned char buf = (c == 0) ? 0 : 1;
    
    if (write_error)
        return(WRITESTAT);

    rs_write(savef, &buf, 1);

    return(WRITESTAT);
}

int
rs_read_boolean(int inf, bool *i)
{
    unsigned char buf = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &buf, 1);

    *i = (buf != 0);
    
    return(READSTAT);
}

int
rs_write_booleans(FILE *savef, bool *c, int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, count);

    for(n = 0; n < count; n++)
        if (rs_write_boolean(savef, c[n]) != 0)
            break;

    return(WRITESTAT);
}

int
rs_read_booleans(int inf, bool *i, int count)
{
    int n = 0, value = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < count; n++)
        if (rs_read_boolean(inf, &i[n]) != 0)
            break;
    
    return(READSTAT);
}

int
rs_write_short(FILE *savef, short c)
{
    unsigned char bytes[2];
    unsigned char *buf = (unsigned char *) &c;

    if (write_error)
        return(WRITESTAT);

    if (big_endian)
    {
        bytes[1] = buf[0];
        bytes[0] = buf[1];
        buf = bytes;
    }

    rs_write(savef, buf, 2);

    return(WRITESTAT);
}

int
rs_read_short(int inf, short *i)
{
    unsigned char bytes[2];
    short  input;
    unsigned char *buf = (unsigned char *)&input;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &input, 2);

    if (big_endian)
    {
        bytes[1] = buf[0];
        bytes[0] = buf[1];
        buf = bytes;
    }
    
    *i = *((short *) buf);

    return(READSTAT);
} 

int
rs_write_shorts(FILE *savef, short *c, int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, count);

    for(n = 0; n < count; n++)
        if (rs_write_short(savef, c[n]) != 0)
            break; 

    return(WRITESTAT);
}

int
rs_read_shorts(int inf, short *i, int count)
{
    int n = 0, value = 0;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < value; n++)
        if (rs_read_short(inf, &i[n]) != 0)
            break;
    
    return(READSTAT);
}

int
rs_write_ushort(FILE *savef, unsigned short c)
{
    unsigned char bytes[2];
    unsigned char *buf = (unsigned char *) &c;

    if (write_error)
        return(WRITESTAT);

    if (big_endian)
    {
        bytes[1] = buf[0];
        bytes[0] = buf[1];
        buf = bytes;
    }

    rs_write(savef, buf, 2);

    return(WRITESTAT);
}

int
rs_read_ushort(int inf, unsigned short *i)
{
    unsigned char bytes[2];
    unsigned short  input;
    unsigned char *buf = (unsigned char *)&input;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &input, 2);

    if (big_endian)
    {
        bytes[1] = buf[0];
        bytes[0] = buf[1];
        buf = bytes;
    }
    
    *i = *((unsigned short *) buf);

    return(READSTAT);
} 

int
rs_write_uint(FILE *savef, unsigned int c)
{
    unsigned char bytes[4];
    unsigned char *buf = (unsigned char *) &c;

    if (write_error)
        return(WRITESTAT);

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    rs_write(savef, buf, 4);

    return(WRITESTAT);
}

int
rs_read_uint(int inf, unsigned int *i)
{
    unsigned char bytes[4];
    int  input;
    unsigned char *buf = (unsigned char *)&input;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &input, 4);

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    *i = *((unsigned int *) buf);

    return(READSTAT);
}

int
rs_write_long(FILE *savef, long c)
{
    int c2;
    unsigned char bytes[4];
    unsigned char *buf = (unsigned char *)&c;

    if (write_error)
        return(WRITESTAT);

    if (sizeof(long) == 8)
    {
        c2 = c;
        buf = (unsigned char *) &c2;
    }

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    rs_write(savef, buf, 4);

    return(WRITESTAT);
}

int
rs_read_long(int inf, long *i)
{
    unsigned char bytes[4];
    long input;
    unsigned char *buf = (unsigned char *) &input;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &input, 4);

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    *i = *((long *) buf);

    return(READSTAT);
}

int
rs_write_longs(FILE *savef, long *c, int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef,count);
    
    for(n = 0; n < count; n++)
        rs_write_long(savef, c[n]);

    return(WRITESTAT);
}

int
rs_read_longs(int inf, long *i, int count)
{
    int n = 0, value = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < value; n++)
        if (rs_read_long(inf, &i[n]) != 0)
            break;
    
    return(READSTAT);
}

int
rs_write_ulong(FILE *savef, unsigned long c)
{
    unsigned int c2;
    unsigned char bytes[4];
    unsigned char *buf = (unsigned char *)&c;

    if (write_error)
        return(WRITESTAT);

    if ( (sizeof(long) == 8) && (sizeof(int) == 4) )
    {
        c2 = c;
        buf = (unsigned char *) &c2;
    }

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    rs_write(savef, buf, 4);

    return(WRITESTAT);
}

int
rs_read_ulong(int inf, unsigned long *i)
{
    unsigned char bytes[4];
    unsigned long input;
    unsigned char *buf = (unsigned char *) &input;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read(inf, &input, 4);

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }
    
    *i = *((unsigned long *) buf);

    return(READSTAT);
}

int
rs_write_ulongs(FILE *savef, unsigned long *c, int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef,count);

    for(n = 0; n < count; n++)
        if (rs_write_ulong(savef,c[n]) != 0)
            break;

    return(WRITESTAT);
}

int
rs_read_ulongs(int inf, unsigned long *i, int count)
{
    int n = 0, value = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < count; n++)
        if (rs_read_ulong(inf, &i[n]) != 0)
            break;
    
    return(READSTAT);
}

int
rs_write_marker(FILE *savef, int id)
{
    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, id);

    return(WRITESTAT);
}

int 
rs_read_marker(int inf, int id)
{
    int nid;

    if (read_error || format_error)
        return(READSTAT);

    if (rs_read_int(inf, &nid) == 0)
        if (id != nid)
            format_error = 1;
    
    return(READSTAT);
}



/******************************************************************************/

int
rs_write_string(FILE *savef, char *s)
{
    int len = 0;

    if (write_error)
        return(WRITESTAT);

    len = (s == NULL) ? 0 : (int) strlen(s) + 1;

    rs_write_int(savef, len);
    rs_write_chars(savef, s, len);
            
    return(WRITESTAT);
}

int
rs_read_string(int inf, char *s, int max)
{
    int len = 0;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &len);

    if (len > max)
        format_error = TRUE;

    rs_read_chars(inf, s, len);
    
    return(READSTAT);
}

int
rs_read_new_string(int inf, char **s)
{
    int len=0;
    char *buf=0;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &len);

    if (len == 0)
        buf = NULL;
    else
    { 
        buf = malloc(len);

        if (buf == NULL)            
            read_error = TRUE;
    }

    rs_read_chars(inf, buf, len);

    *s = buf;

    return(READSTAT);
}

int
rs_write_strings(FILE *savef, char *s[], int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, count);

    for(n = 0; n < count; n++)
        if (rs_write_string(savef, s[n]) != 0)
            break;
    
    return(WRITESTAT);
}

int
rs_read_strings(int inf, char **s, int count, int max)
{
    int n     = 0;
    int value = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < count; n++)
        if (rs_read_string(inf, s[n], max) != 0)
            break;
    
    return(READSTAT);
}

int
rs_read_new_strings(int inf, char **s, int count)
{
    int n     = 0;
    int value = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < count; n++)
        if (rs_read_new_string(inf, &s[n]) != 0)
            break;
    
    return(READSTAT);
}

int
rs_write_string_index(FILE *savef, char *master[], int max, const char *str)
{
    int i;

    if (write_error)
        return(WRITESTAT);

    for(i = 0; i < max; i++)
        if (str == master[i])
            return( rs_write_int(savef, i) );

    return( rs_write_int(savef,-1) );
}

int
rs_read_string_index(int inf, char *master[], int maxindex, char **str)
{
    int i;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &i);

    if (i > maxindex)
        format_error = TRUE;
    else if (i >= 0)
        *str = master[i];
    else
        *str = NULL;

    return(READSTAT);
}

int
rs_write_coord(FILE *savef, coord c)
{
    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, c.x);
    rs_write_int(savef, c.y);
    
    return(WRITESTAT);
}

int
rs_read_coord(int inf, coord *c)
{
    coord in;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&in.x);
    rs_read_int(inf,&in.y);

    if (READSTAT == 0) 
    {
        c->x = in.x;
        c->y = in.y;
    }

    return(READSTAT);
}

int
rs_write_coord_list(FILE *savef, struct linked_list *l)
{
    rs_write_marker(savef, RSID_COORDLIST);
    rs_write_int(savef, list_size(l));

    while (l != NULL) 
    {
        rs_write_coord(savef, *(coord *) l->l_data);
        l = l->l_next;
    }
    
    return(WRITESTAT);
}

int
rs_read_coord_list(int inf, struct linked_list **list)
{
    int i, cnt;
    struct linked_list *l = NULL, *previous = NULL, *head = NULL;

    rs_read_marker(inf, RSID_COORDLIST);

    if (rs_read_int(inf,&cnt) != 0)
	return(READSTAT);

    for (i = 0; i < cnt; i++)
    {
	l = new_item(sizeof(coord));
        l->l_prev = previous;

        if (previous != NULL)
	    previous->l_next = l;

        rs_read_coord(inf,(coord *) l->l_data);

	if (previous == NULL)
	    head = l;
                
	previous = l;
    }
            
    if (l != NULL)
	l->l_next = NULL;
    
    *list = head;

    return(READSTAT);
}

int
rs_write_window(FILE *savef, WINDOW *win)
{
    int row,col,height,width;

    if (write_error)
        return(WRITESTAT);

    width  = getmaxx(win);
    height = getmaxy(win);

    rs_write_marker(savef,RSID_WINDOW);
    rs_write_int(savef,height);
    rs_write_int(savef,width);

    for(row=0;row<height;row++)
        for(col=0;col<width;col++)
            if (rs_write_int(savef, mvwinch(win,row,col)) != 0)
                return(WRITESTAT);

    return(WRITESTAT);
}

int
rs_read_window(int inf, WINDOW *win)
{
    int row,col,maxlines,maxcols,value,width,height;
    
    if (read_error || format_error)
        return(READSTAT);

    width  = getmaxx(win);
    height = getmaxy(win);

    rs_read_marker(inf, RSID_WINDOW);

    rs_read_int(inf, &maxlines);
    rs_read_int(inf, &maxcols);

    for(row = 0; row < maxlines; row++)
        for(col = 0; col < maxcols; col++)
        {
            if (rs_read_int(inf, &value) != 0)
                return(READSTAT);

            if ((row < height) && (col < width))
                mvwaddch(win,row,col,value);
        }
        
    return(READSTAT);
}

/******************************************************************************/

void *
get_list_item(struct linked_list *l, int i)
{
    int count;

    for(count = 0; l != NULL; count++, l = l->l_next)
        if (count == i)
	    return(l->l_data);
    
    return(NULL);
}

int
find_list_ptr(struct linked_list *l, void *ptr)
{
    int count;

    for(count = 0; l != NULL; count++, l = l->l_next)
        if (l->l_data == ptr)
            return(count);
    
    return(-1);
}

int
list_size(struct linked_list *l)
{
    int count;
    
    for(count = 0; l != NULL; count++, l = l->l_next)
        ;
    
    return(count);
}

/******************************************************************************/

int
rs_write_levtype(FILE *savef, LEVTYPE c)
{
    int lt;
    
    switch(c)
    {
        case NORMLEV: lt = 1; break;
        case POSTLEV: lt = 2; break;
        case MAZELEV: lt = 3; break;
        case OUTSIDE: lt = 4; break;
        default: lt = -1; break;
    }
    
    rs_write_int(savef,lt);
        
    return(WRITESTAT);
}

int
rs_read_levtype(int inf, LEVTYPE *l)
{
    int lt;
    
    rs_read_int(inf, &lt);
    
    switch(lt)
    {
        case 1: *l = NORMLEV; break;
        case 2: *l = POSTLEV; break;
        case 3: *l = MAZELEV; break;
        case 4: *l = OUTSIDE; break;
        default: *l = NORMLEV; break;
    }
    
    return(READSTAT);
}

int
rs_write_stats(FILE *savef, struct stats *s)
{
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_STATS);
    rs_write_short(savef, s->s_str);
    rs_write_short(savef, s->s_intel);
    rs_write_short(savef, s->s_wisdom);
    rs_write_short(savef, s->s_dext);
    rs_write_short(savef, s->s_const);
    rs_write_short(savef, s->s_charisma);
    rs_write_ulong(savef, s->s_exp);
    rs_write_int(savef, s->s_lvl);
    rs_write_int(savef, s->s_arm);
    rs_write_int(savef, s->s_hpt);
    rs_write_int(savef, s->s_pack);
    rs_write_int(savef, s->s_carry);
    rs_write(savef, s->s_dmg, sizeof(s->s_dmg));

    return(WRITESTAT);
}

int
rs_read_stats(int inf, struct stats *s)
{
    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_STATS);
    rs_read_short(inf,&s->s_str);
    rs_read_short(inf,&s->s_intel);
    rs_read_short(inf,&s->s_wisdom);
    rs_read_short(inf,&s->s_dext);
    rs_read_short(inf,&s->s_const);
    rs_read_short(inf,&s->s_charisma);
    rs_read_ulong(inf,&s->s_exp);
    rs_read_int(inf,&s->s_lvl);
    rs_read_int(inf,&s->s_arm);
    rs_read_int(inf,&s->s_hpt);
    rs_read_int(inf,&s->s_pack);
    rs_read_int(inf,&s->s_carry);

    rs_read(inf,s->s_dmg,sizeof(s->s_dmg));
    
    return(READSTAT);
}

int
rs_write_magic_items(FILE *savef, struct magic_item *i, int count)
{
    int n;
    
    rs_write_marker(savef, RSID_MAGICITEMS);
    rs_write_int(savef, count);

    for(n = 0; n < count; n++)
    {
        rs_write_int(savef,i[n].mi_prob);
    }
    
    return(WRITESTAT);
}

int
rs_read_magic_items(int inf, struct magic_item *mi, int count)
{
    int n;
    int value;

    rs_read_marker(inf, RSID_MAGICITEMS);

    rs_read_int(inf, &value);

    if (value != count)
	format_error = 1;
    else
    {
	for(n = 0; n < value; n++)
        {
	    rs_read_int(inf,&mi[n].mi_prob);
        }
    }
    
    return(READSTAT);
}

int
rs_write_scrolls(FILE *savef)
{
    int i;

    if (write_error)
        return(WRITESTAT);

    for(i = 0; i < MAXSCROLLS; i++)
    {
        rs_write_string(savef, s_names[i]);
	rs_write_boolean(savef,s_know[i]);
        rs_write_string(savef,s_guess[i]);
    }

    return(WRITESTAT);
}

int
rs_read_scrolls(int inf)
{
    int i;

    if (read_error || format_error)
        return(READSTAT);

    for(i = 0; i < MAXSCROLLS; i++)
    {
        rs_read_new_string(inf,&s_names[i]);
        rs_read_boolean(inf,&s_know[i]);
        rs_read_new_string(inf,&s_guess[i]);
    }

    return(READSTAT);
}

int
rs_write_potions(FILE *savef)
{
    int i;

    if (write_error)
        return(WRITESTAT);

    for(i = 0; i < MAXPOTIONS; i++)
    {
	rs_write_string_index(savef,rainbow,NCOLORS,p_colors[i]);
        rs_write_boolean(savef,p_know[i]);
        rs_write_string(savef,p_guess[i]);
    }

    return(WRITESTAT);
}

int
rs_read_potions(int inf)
{
    int i;

    if (read_error || format_error)
        return(READSTAT);

    for(i = 0; i < MAXPOTIONS; i++)
    {
        rs_read_string_index(inf,rainbow,NCOLORS,&p_colors[i]);
	rs_read_boolean(inf,&p_know[i]);
        rs_read_new_string(inf,&p_guess[i]);
    }

    return(READSTAT);
}

int
rs_write_rings(FILE *savef)
{
    int i;

    if (write_error)
        return(WRITESTAT);

    for(i = 0; i < MAXRINGS; i++)
    {
	rs_write_string_index(savef,stones,NSTONES,r_stones[i]);
        rs_write_boolean(savef,r_know[i]);
        rs_write_string(savef,r_guess[i]);
    }

    return(WRITESTAT);
}

int
rs_read_rings(int inf)
{
    int i;

    if (read_error || format_error)
        return(READSTAT);

    for(i = 0; i < MAXRINGS; i++)
    {
        rs_read_string_index(inf,stones,NSTONES,&r_stones[i]);
	rs_read_boolean(inf,&r_know[i]);
        rs_read_new_string(inf,&r_guess[i]);
    }

    return(READSTAT);
}

int
rs_write_sticks(FILE *savef)
{
    int i;

    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_STICKS);

    for (i = 0; i < MAXSTICKS; i++)
    {
        if (strcmp(ws_type[i],"staff") == 0)
        {
            rs_write_int(savef,0);
	    rs_write_string_index(savef,wood,NWOOD,ws_made[i]);
        }
        else
        {
            rs_write_int(savef,1);
	    rs_write_string_index(savef,metal,NMETAL,ws_made[i]);
        }

	rs_write_boolean(savef, ws_know[i]);
        rs_write_string(savef, ws_guess[i]);
    }
 
    return(WRITESTAT);
}
        
int
rs_read_sticks(int inf)
{
    int i = 0, j = 0, list = 0;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_STICKS);

    for(i = 0; i < MAXSTICKS; i++)
    { 
        rs_read_int(inf,&list);
        ws_made[i] = NULL;

        if (list == 0)
        {
	    rs_read_string_index(inf,wood,NWOOD,&ws_made[i]);
            ws_type[i] = "staff";
        }
        else 
        {
	    rs_read_string_index(inf,metal,NMETAL,&ws_made[i]);
	    ws_type[i] = "wand";
        }
	rs_read_boolean(inf, &ws_know[i]);
        rs_read_new_string(inf, &ws_guess[i]);
    }

    return(READSTAT);
}

int
rs_write_daemons(FILE *savef, struct delayed_action *d_list, int count)
{
    int i = 0;
    int func = 0;
        
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_DAEMONS);
    rs_write_int(savef, count);
        
    for(i = 0; i < count; i++)
    {
        if ( d_list[i].d_func == rollwand)
            func = 1;
        else if ( d_list[i].d_func == doctor)
            func = 2;
        else if ( d_list[i].d_func == stomach)
            func = 3;
        else if ( d_list[i].d_func == runners)
            func = 4;
        else if ( d_list[i].d_func == swander)
            func = 5;
        else if ( d_list[i].d_func == trap_look)
            func = 6;
        else if ( d_list[i].d_func == ring_search)
            func = 7;
        else if ( d_list[i].d_func == ring_teleport)
            func = 8;
        else if ( d_list[i].d_func == strangle)
            func = 9;
        else if ( d_list[i].d_func == fumble)
            func = 10;
        else if ( d_list[i].d_func == wghtchk)
            func = 11;
        else if ( d_list[i].d_func == unstink)
            func = 12;
        else if ( d_list[i].d_func == res_strength)
            func = 13;
        else if ( d_list[i].d_func == un_itch)
            func = 14;
        else if ( d_list[i].d_func == cure_disease)
            func = 15;
        else if ( d_list[i].d_func == unconfuse)
            func = 16;
        else if ( d_list[i].d_func == suffocate)
            func = 17;
        else if ( d_list[i].d_func == undance)
            func = 18;
        else if ( d_list[i].d_func == alchemy)
            func = 19;
        else if ( d_list[i].d_func == dust_appear)
            func = 20;
        else if ( d_list[i].d_func == unchoke)
            func = 21;
        else if ( d_list[i].d_func == sight)
            func = 22;
        else if ( d_list[i].d_func == noslow)
            func = 23;
        else if ( d_list[i].d_func == nohaste)
            func = 24;
        else if ( d_list[i].d_func == unclrhead)
            func = 25;
        else if ( d_list[i].d_func == unsee)
            func = 26;
        else if ( d_list[i].d_func == unphase)
            func = 27;
        else if ( d_list[i].d_func == land)
            func = 28;
        else if ( d_list[i].d_func == appear)
            func = 29;
        else if (d_list[i].d_func == NULL)
            func = 0;
        else
            func = -1;

        rs_write_int(savef, d_list[i].d_type);
        rs_write_int(savef, func);
        rs_write_int(savef, d_list[i].d_arg);
        rs_write_int(savef, d_list[i].d_time);
    }
    
    return(WRITESTAT);
}       

int
rs_read_daemons(int inf, struct delayed_action *d_list, int count)
{
    int i = 0;
    int func = 0;
    int value = 0;
    int dummy = 0;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_DAEMONS);
    rs_read_int(inf, &value);

    if (value > count)
        format_error = TRUE;

    
    for(i=0; i < count; i++)
    {
	func = 0;
        rs_read_int(inf, &d_list[i].d_type);
        rs_read_int(inf, &func);
                    
        switch(func)
        {
	    case  1: d_list[i].d_func = rollwand;
		     break;
            case  2: d_list[i].d_func = doctor;
		     break;
            case 3: d_list[i].d_func = stomach;
		     break;
            case  4: d_list[i].d_func = runners;
                     break;
            case  5: d_list[i].d_func = swander;
                     break;
            case  6: d_list[i].d_func = trap_look;
                     break;
            case  7: d_list[i].d_func = ring_search;
                     break;
            case  8: d_list[i].d_func = ring_teleport;
                     break;
            case  9: d_list[i].d_func = strangle;
                     break;
            case 10: d_list[i].d_func = fumble;
                     break;
            case 11: d_list[i].d_func = wghtchk;
                     break;
            case 12: d_list[i].d_func = unstink;
                     break;
            case 13: d_list[i].d_func = res_strength;
                     break;
            case 14: d_list[i].d_func = un_itch;
                     break;
            case 15: d_list[i].d_func = cure_disease;
                     break;
            case 16: d_list[i].d_func = unconfuse;
                     break;
            case 17: d_list[i].d_func = suffocate;
                     break;
            case 18: d_list[i].d_func = undance;
                     break;
            case 19: d_list[i].d_func = alchemy;
                     break;
            case 20: d_list[i].d_func = dust_appear;
                     break;
            case 21: d_list[i].d_func = unchoke;
                     break;
            case 22: d_list[i].d_func = sight;
                     break;
            case 23: d_list[i].d_func = noslow;
                     break;
            case 24: d_list[i].d_func = nohaste;
                     break;
            case 25: d_list[i].d_func = unclrhead;
                     break;
            case 26: d_list[i].d_func = unsee;
                     break;
            case 27: d_list[i].d_func = unphase;
                     break;
            case 28: d_list[i].d_func = land;
                     break;
            case 29: d_list[i].d_func = appear;
                     break;
	    case  0:
            case -1:
            default: d_list[i].d_func = NULL;
                     break;
        }   

        rs_read_int(inf, &d_list[i].d_arg);
        rs_read_int(inf, &d_list[i].d_time);

	if (d_list[i].d_func == NULL) 
	{
	    d_list[i].d_time = 0;
	    d_list[i].d_arg = 0;
	    d_list[i].d_type = 0;
	}
    }
    
    return(READSTAT);
}        

int
rs_write_room(FILE *savef, struct room *r)
{
    struct linked_list *l;
    int i;

    if (write_error)
        return(WRITESTAT);

    rs_write_coord(savef, r->r_pos);
    rs_write_coord(savef, r->r_max);
    rs_write_long(savef, r->r_flags);

    l = r->r_fires;
    i = list_size(l);
        
    rs_write_int(savef, i);

    if (i >0)
	while (l != NULL)
        {
	    i = find_list_ptr(mlist, l->l_data);
            rs_write_int(savef,i);
            l = l->l_next;
        }
    
    rs_write_coord_list(savef, r->r_exit);

    return(WRITESTAT);
}

int
rs_read_room(int inf, struct room *r)
{
    int value = 0, n = 0, i = 0, index = 0, id = 0;
    struct linked_list *fires=NULL, *item = NULL;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_coord(inf,&r->r_pos);
    rs_read_coord(inf,&r->r_max);
    rs_read_long(inf,&r->r_flags);

    rs_read_int(inf, &i);
    fires = NULL;

    while (i>0)
    {
	rs_read_int(inf,&index);
                        
        if (index >= 0)
        {
	    void *data;
            data = get_list_item(mlist,index);
            item = creat_item();
            item->l_data = data;
            if (fires == NULL)
		fires = item;
            else
		attach(fires,item);
        }
        i--;
    }

    r->r_fires=fires; 

    rs_read_coord_list(inf, &r->r_exit);

    return(READSTAT);
}

int
rs_write_rooms(FILE *savef, struct room r[], int count)
{
    int n = 0;

    if (write_error)
        return(WRITESTAT);

    rs_write_int(savef, count);
    
    for(n = 0; n < count; n++)
        rs_write_room(savef, &r[n]);
    
    return(WRITESTAT);
}

int
rs_read_rooms(int inf, struct room *r, int count)
{
    int value = 0, n = 0;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf,&value);

    if (value > count)
        format_error = TRUE;

    for(n = 0; n < value; n++)
        rs_read_room(inf,&r[n]);

    return(READSTAT);
}

rs_write_room_reference(FILE *savef, struct room *rp)
{
    int i, room = -1;
    
    if (write_error)
        return(WRITESTAT);

    for (i = 0; i < MAXROOMS; i++)
        if (&rooms[i] == rp)
            room = i;

    rs_write_int(savef, room);

    return(WRITESTAT);
}

int
rs_read_room_reference(int inf, struct room **rp)
{
    int i;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &i);

    *rp = &rooms[i];
            
    return(READSTAT);
}

int
rs_write_door_reference(FILE *savef, coord *exit)
{
    int i, idx;

    for (i = 0; i < MAXROOMS; i++)
    {
	idx = find_list_ptr(rooms[i].r_exit, exit);

	if (idx != -1)
	    break;
    }

    if (i >= MAXROOMS) 
    {
        rs_write_int(savef,-1);
	rs_write_int(savef,-1);
	if (exit != NULL)
	    abort();
    }
    else
    {
	rs_write_int(savef,i);
	rs_write_int(savef,idx);
    }

    return(WRITESTAT);
}

int
rs_read_door_reference(int inf, coord **exit)
{
    int i, idx;

    rs_read_int(inf, &i);
    rs_read_int(inf, &idx);

    if ( (i == -1) || (idx == -1) )
	*exit = NULL;
    else
	*exit = get_list_item(rooms[i].r_exit, idx);

    return(READSTAT);
}

int
rs_write_traps(FILE *savef, struct trap *trap,int count)
{
    int n;

    rs_write_int(savef, RSID_TRAP);
    rs_write_int(savef, count);
    
    for(n=0; n<count; n++)
    {
        rs_write_char(savef, trap[n].tr_type);
        rs_write_char(savef, trap[n].tr_show);
        rs_write_coord(savef, trap[n].tr_pos);
        rs_write_long(savef, trap[n].tr_flags);
    }
}

int
rs_read_traps(int inf, struct trap *trap, int count)
{
    int id = 0, value = 0, n = 0;

    if (rs_read_int(inf,&id) != 0)
        format_error = TRUE;
    else if (rs_read_int(inf,&value) != 0)
    {
        if (value != count)
	    format_error = TRUE;
    }
    else
    {
	for(n=0;n<value;n++)
        {   
	    rs_read_char(inf,&trap[n].tr_type);
            rs_read_char(inf,&trap[n].tr_show);
            rs_read_coord(inf,&trap[n].tr_pos);
            rs_read_long(inf,&trap[n].tr_flags);
	}
    }
    
    return(READSTAT);
}

int
rs_write_monsters(FILE *savef, struct monster *m, int count)
{
    int n;
    
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_MONSTERS);
    rs_write_int(savef, count);

    for(n=0;n<count;n++)
    {
        rs_write_boolean(savef, m[n].m_normal);
        rs_write_boolean(savef, m[n].m_wander);
        rs_write_short(savef, m[n].m_numsum);
    }
    
    return(WRITESTAT);
}

int
rs_read_monsters(int inf, struct monster *m, int count)
{
    int value = 0, n = 0;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_MONSTERS);

    rs_read_int(inf, &value);

    if (value != count)
        format_error = TRUE;

    for(n = 0; n < count; n++)
    {
        rs_read_boolean(inf, &m[n].m_normal);
        rs_read_boolean(inf, &m[n].m_wander);
	rs_read_short(inf, &m[n].m_numsum);
    }

    return(READSTAT);
}

int
rs_write_object(FILE *savef, struct object *o)
{
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_OBJECT);
    rs_write_int(savef, o->o_type);
    rs_write_coord(savef, o->o_pos);
    rs_write_char(savef, o->o_launch);
    rs_write(savef, o->o_damage, sizeof(o->o_damage));
    rs_write(savef, o->o_hurldmg, sizeof(o->o_hurldmg));
    rs_write_object_list(savef, o->contents);
    rs_write_int(savef, o->o_count);
    rs_write_int(savef, o->o_which);
    rs_write_int(savef, o->o_hplus);
    rs_write_int(savef, o->o_dplus);
    rs_write_int(savef, o->o_ac);
    rs_write_long(savef, o->o_flags);
    rs_write_int(savef, o->o_group);
    rs_write_int(savef, o->o_weight);
    rs_write(savef, o->o_mark, MARKLEN);
       
    
    return(WRITESTAT);
}

int
rs_read_object(int inf, struct object *o)
{
    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_OBJECT);
    rs_read_int(inf, &o->o_type);
    rs_read_coord(inf, &o->o_pos);
    rs_read_char(inf, &o->o_launch);
    rs_read(inf, o->o_damage, sizeof(o->o_damage));
    rs_read(inf, o->o_hurldmg, sizeof(o->o_hurldmg));
    rs_read_object_list(inf,&o->contents);
    rs_read_int(inf, &o->o_count);
    rs_read_int(inf, &o->o_which);
    rs_read_int(inf, &o->o_hplus);
    rs_read_int(inf, &o->o_dplus);
    rs_read_int(inf, &o->o_ac);
    rs_read_long(inf,&o->o_flags);
    rs_read_int(inf, &o->o_group);
    rs_read_int(inf, &o->o_weight);
    rs_read(inf, o->o_mark, MARKLEN);
    
    return(READSTAT);
}

int
rs_write_object_list(FILE *savef, struct linked_list *l)
{
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_OBJECTLIST);
    rs_write_int(savef, list_size(l));

    for( ;l != NULL; l = l->l_next)
        rs_write_object(savef, OBJPTR(l));
    
    return(WRITESTAT);
}

int
rs_read_object_list(int inf, struct linked_list **list)
{
    int i, cnt;
    struct linked_list *l = NULL, *previous = NULL, *head = NULL;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_OBJECTLIST);
    rs_read_int(inf, &cnt);

    for (i = 0; i < cnt; i++) 
    {
        l = new_item(sizeof(struct object));

	l->l_prev = previous;

        if (previous != NULL)
            previous->l_next = l;

        rs_read_object(inf,OBJPTR(l));

        if (previous == NULL)
            head = l;

        previous = l;
    }
            
    if (l != NULL)
        l->l_next = NULL;
    
    *list = head;

    return(READSTAT);
}

int
rs_write_object_reference(FILE *savef, struct linked_list *list, struct object *item)
{
    int i;
    
    if (write_error)
        return(WRITESTAT);

    i = find_list_ptr(list, item);

    rs_write_int(savef, i);

    return(WRITESTAT);
}

int
rs_read_object_reference(int inf, struct linked_list *list, struct object **item)
{
    int i;
    
    if (read_error || format_error)
        return(READSTAT);

    rs_read_int(inf, &i);

    *item = get_list_item(list,i);
            
    return(READSTAT);
}

int
find_thing_coord(struct linked_list *monlist, coord *c)
{
    struct linked_list *mitem;
    struct thing *tp;
    int i = 0;

    for(mitem = monlist; mitem != NULL; mitem = mitem->l_next)
    {
        tp = THINGPTR(mitem);

        if (c == &tp->t_pos)
            return(i);

        i++;
    }

    return(-1);
}

int
find_object_coord(struct linked_list *objlist, coord *c)
{
    struct linked_list *oitem;
    struct object *obj;
    int i = 0;

    for(oitem = objlist; oitem != NULL; oitem = oitem->l_next)
    {
        obj = OBJPTR(oitem);

        if (c == &obj->o_pos)
            return(i);

        i++;
    }

    return(-1);
}

int
rs_write_thing(FILE *savef, struct thing *t)
{
    int i = -1;
    
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_THING);

    if (t == NULL)
    {
        rs_write_int(savef, 0);
        return(WRITESTAT);
    }
    
    rs_write_int(savef, 1);

    rs_write_boolean(savef, t->t_turn);
    rs_write_boolean(savef, t->t_wasshot);
    rs_write_char(savef, t->t_type);
    rs_write_char(savef, t->t_disguise);
    rs_write_char(savef, t->t_oldch);

    rs_write_short(savef, t->t_ctype);
    rs_write_short(savef, t->t_index);
    rs_write_short(savef, t->t_no_move);
    rs_write_short(savef, t->t_quiet);

    rs_write_door_reference(savef, t->t_doorgoal);
 
    rs_write_coord(savef, t->t_pos);
    rs_write_coord(savef, t->t_oldpos);

    /* 
        t_dest can be:
        0,0: NULL
        0,1: location of hero
        0,3: global coord 'delta'
        1,i: location of a thing (monster)
        2,i: location of an object
        3,i: location of gold in a room

        We need to remember what we are chasing rather than 
        the current location of what we are chasing.
    */

    if (t->t_dest == &hero)
    {
        rs_write_int(savef,0);
        rs_write_int(savef,1);
    }
    else if (t->t_dest != NULL)
    {
        i = find_thing_coord(mlist, t->t_dest);
            
        if (i >=0 )
        {
            rs_write_int(savef,1);
            rs_write_int(savef,i);
        }
        else
        {
            i = find_object_coord(lvl_obj, t->t_dest);
            
            if (i >= 0)
            {
                rs_write_int(savef,2);
                rs_write_int(savef,i);
            }
            else
            {
                rs_write_int(savef, 0);
                rs_write_int(savef,1); /* chase the hero anyway */
            }
        }
    }
    else
    {
        rs_write_int(savef,0);
        rs_write_int(savef,0);
    }
    
    rs_write_ulongs(savef, t->t_flags, 16);
    rs_write_object_list(savef, t->t_pack);
    rs_write_stats(savef, &t->t_stats);
    rs_write_stats(savef, &t->maxstats);
    
    return(WRITESTAT);
}

int
rs_read_thing(int inf, struct thing *t)
{
    int listid = 0, index = -1;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_THING);

    rs_read_int(inf, &index);

    if (index == 0)
        return(READSTAT);

    rs_read_boolean(inf, &t->t_turn);
    rs_read_boolean(inf, &t->t_wasshot);
    rs_read_char(inf, &t->t_type);
    rs_read_char(inf, &t->t_disguise);
    rs_read_char(inf, &t->t_oldch);
    rs_read_short(inf, &t->t_ctype);
    rs_read_short(inf, &t->t_index);
    rs_read_short(inf, &t->t_no_move);
    rs_read_short(inf, &t->t_quiet);
    rs_read_door_reference(inf,&t->t_doorgoal);
    rs_read_coord(inf, &t->t_pos);
    rs_read_coord(inf, &t->t_oldpos);

    /* 
	t_dest can be (listid,index):
	    0,0: NULL
            0,1: location of hero
            1,i: location of a thing (monster)
            2,i: location of an object
            3,i: location of gold in a room

	We need to remember what we are chasing rather than 
        the current location of what we are chasing.
    */
            
    rs_read_int(inf, &listid);
    rs_read_int(inf, &index);
    t->t_reserved = -1;

    if (listid == 0) /* hero or NULL */
    {
	if (index == 1)
	    t->t_dest = &hero;
        else
	    t->t_dest = NULL;
    }
    else if (listid == 1) /* monster/thing */
    {
	t->t_dest     = NULL;
        t->t_reserved = index;
    }
    else if (listid == 2) /* object */
    {
	struct object *obj;

        obj = get_list_item(lvl_obj, index);

        if (obj != NULL)
        {
            t->t_dest = &obj->o_pos;
        }
    }
    else
	t->t_dest = NULL;

    rs_read_ulongs(inf, t->t_flags, 16);
    rs_read_object_list(inf, &t->t_pack);
    rs_read_stats(inf, &t->t_stats);
    rs_read_stats(inf, &t->maxstats);
    
    return(READSTAT);
}

int
rs_fix_thing(struct thing *t)
{
    struct thing *tp;

    if (t->t_reserved < 0)
        return;

    tp = get_list_item(mlist,t->t_reserved);

    if (tp != NULL)
    {
        t->t_dest = &tp->t_pos;
    }
}

int
rs_write_thing_list(FILE *savef, struct linked_list *l)
{
    int cnt = 0;
    
    if (write_error)
        return(WRITESTAT);

    rs_write_marker(savef, RSID_MONSTERLIST);

    cnt = list_size(l);

    rs_write_int(savef, cnt);

    if (cnt < 1)
        return(WRITESTAT);

    while (l != NULL) {
        rs_write_thing(savef, (struct thing *)l->l_data);
        l = l->l_next;
    }
    
    return(WRITESTAT);
}

int
rs_read_thing_list(int inf, struct linked_list **list)
{
    int i, cnt;
    struct linked_list *l = NULL, *previous = NULL, *head = NULL;

    if (read_error || format_error)
        return(READSTAT);

    rs_read_marker(inf, RSID_MONSTERLIST);

    rs_read_int(inf, &cnt);

    for (i = 0; i < cnt; i++) 
    {
        l = new_item(sizeof(struct thing));

        l->l_prev = previous;
            
        if (previous != NULL)
            previous->l_next = l;

        rs_read_thing(inf,THINGPTR(l));

        if (previous == NULL)
            head = l;

        previous = l;
    }
        
    if (l != NULL)
        l->l_next = NULL;

    *list = head;
    
    return(READSTAT);
}

int
rs_fix_thing_list(struct linked_list *list)
{
    struct linked_list *item;

    for(item = list; item != NULL; item = item->l_next)
        rs_fix_thing(THINGPTR(item));
}

int
rs_save_file(FILE *savef)
{
    int i;

    if (write_error)
        return(WRITESTAT);

    rs_write_object_list(savef, lvl_obj);
    rs_write_thing(savef, &player);
    rs_write_thing_list(savef, mlist);
    rs_write_thing_list(savef, tlist);
    rs_write_thing_list(savef, monst_dead);
    
    rs_write_traps(savef, traps, MAXTRAPS);             
    rs_write_rooms(savef, rooms, MAXROOMS);
    rs_write_room_reference(savef, oldrp);

    rs_write_object_reference(savef, player.t_pack, cur_armor);

    for(i = 0; i < NUM_FINGERS; i++)
        rs_write_object_reference(savef, player.t_pack, cur_ring[i]);

    for(i = 0; i < NUM_MM; i++)
        rs_write_object_reference(savef, player.t_pack, cur_misc[i]);

    rs_write_ints(savef, cur_relic, MAXRELIC);
    
    rs_write_object_reference(savef, player.t_pack, cur_weapon); 

    rs_write_int(savef, char_type);
    rs_write_int(savef, foodlev);
    rs_write_int(savef, ntraps);
    rs_write_int(savef, trader);
    rs_write_int(savef, curprice);
    rs_write_int(savef, no_move);
    rs_write_int(savef, seed);
    rs_write_int(savef, dnum);
    rs_write_int(savef, max_level);
    rs_write_int(savef, cur_max);
    rs_write_int(savef, lost_dext);
    rs_write_int(savef, no_command);
    rs_write_int(savef, level);
    rs_write_int(savef, purse);
    rs_write_int(savef, inpack);
    rs_write_int(savef, total);
    rs_write_int(savef, no_food);
    rs_write_int(savef, foods_this_level);
    rs_write_int(savef, count);
    rs_write_int(savef, food_left);
    rs_write_int(savef, group);
    rs_write_int(savef, hungry_state);
    rs_write_int(savef, infest_dam);
    rs_write_int(savef, lost_str);
    rs_write_int(savef, lastscore);
    rs_write_int(savef, hold_count);
    rs_write_int(savef, trap_tries);
    rs_write_int(savef, pray_time);
    rs_write_int(savef, spell_power);
    rs_write_int(savef, turns);
    rs_write_int(savef, quest_item);
    rs_write_char(savef, nfloors);
    rs_write(savef, curpurch, 15);
    rs_write_char(savef, PLAYER);
    rs_write_char(savef, take);
    rs_write(savef, prbuf, LINELEN);
    rs_write_char(savef, runch);
    rs_write(savef, whoami, LINELEN);
    rs_write(savef, fruit, LINELEN);
    rs_write_scrolls(savef);
    rs_write_potions(savef);
    rs_write_rings(savef);
    rs_write_sticks(savef);
    for(i = 0; i < MAXMM; i++)
        rs_write_string(savef, m_guess[i]);
    rs_write_window(savef, cw);
    rs_write_window(savef, mw);
    rs_write_window(savef, stdscr);
    rs_write_boolean(savef, pool_teleport);
    rs_write_boolean(savef, inwhgt);
    rs_write_boolean(savef, after);
    rs_write_boolean(savef, waswizard);
    rs_write_booleans(savef, m_know, MAXMM);
    rs_write_boolean(savef, playing);
    rs_write_boolean(savef, running);
    rs_write_boolean(savef, wizard);
    rs_write_boolean(savef, notify);
    rs_write_boolean(savef, fight_flush);
    rs_write_boolean(savef, terse);
    rs_write_boolean(savef, auto_pickup);
    rs_write_boolean(savef, door_stop);
    rs_write_boolean(savef, jump);
    rs_write_boolean(savef, slow_invent);
    rs_write_boolean(savef, firstmove);
    rs_write_boolean(savef, askme);
    rs_write_boolean(savef, in_shell);
    rs_write_boolean(savef, daytime);
    rs_write_coord(savef, delta);
    rs_write_levtype(savef, levtype);

    rs_write_monsters(savef, monsters, NUMMONST+1);

    rs_write_magic_items(savef, things, NUMTHINGS);
    rs_write_magic_items(savef, s_magic, MAXSCROLLS);
    rs_write_magic_items(savef, p_magic, MAXPOTIONS);
    rs_write_magic_items(savef, r_magic, MAXRINGS);
    rs_write_magic_items(savef, ws_magic, MAXSTICKS);
    rs_write_magic_items(savef, m_magic, MAXMM);


    rs_write_coord(savef, ch_ret);
    rs_write_int(savef, demoncnt);
    rs_write_int(savef, fusecnt);
    rs_write_daemons(savef, d_list, MAXDAEMONS);
    rs_write_daemons(savef, f_list, MAXFUSES);
    rs_write_int(savef, between);

    fflush(savef);

    return(WRITESTAT);
}

rs_restore_file(int inf)
{
    int i;
    
    if (read_error || format_error)
        return(READSTAT);
    
    rs_read_object_list(inf, &lvl_obj);
    rs_read_thing(inf, &player);
    rs_read_thing_list(inf, &mlist);
    rs_read_thing_list(inf, &tlist);
    rs_read_thing_list(inf, &monst_dead);

    rs_fix_thing(&player);
    rs_fix_thing_list(mlist);
    rs_fix_thing_list(tlist);
    rs_fix_thing_list(monst_dead);

    rs_read_traps(inf, traps, MAXTRAPS);             
    rs_read_rooms(inf, rooms, MAXROOMS);
    rs_read_room_reference(inf, &oldrp);

    rs_read_object_reference(inf, player.t_pack, &cur_armor);

    for(i = 0; i < NUM_FINGERS; i++)
        rs_read_object_reference(inf, player.t_pack, &cur_ring[i]);

    for(i = 0; i < NUM_MM; i++)
        rs_read_object_reference(inf, player.t_pack, &cur_misc[i]);

    rs_read_ints(inf, cur_relic, MAXRELIC);

    rs_read_object_reference(inf, player.t_pack, &cur_weapon);

    rs_read_int(inf, &char_type);
    rs_read_int(inf, &foodlev);
    rs_read_int(inf, &ntraps);
    rs_read_int(inf, &trader);
    rs_read_int(inf, &curprice);
    rs_read_int(inf, &no_move);
    rs_read_int(inf, &seed);
    rs_read_int(inf, &dnum);
    rs_read_int(inf, &max_level);
    rs_read_int(inf, &cur_max);
    rs_read_int(inf, &lost_dext);
    rs_read_int(inf, &no_command);
    rs_read_int(inf, &level);
    rs_read_int(inf, &purse);
    rs_read_int(inf, &inpack);
    rs_read_int(inf, &total);
    rs_read_int(inf, &no_food);
    rs_read_int(inf, &foods_this_level);
    rs_read_int(inf, &count);
    rs_read_int(inf, &food_left);
    rs_read_int(inf, &group);
    rs_read_int(inf, &hungry_state);
    rs_read_int(inf, &infest_dam);
    rs_read_int(inf, &lost_str);
    rs_read_int(inf, &lastscore);
    rs_read_int(inf, &hold_count);
    rs_read_int(inf, &trap_tries);
    rs_read_int(inf, &pray_time);
    rs_read_int(inf, &spell_power);
    rs_read_int(inf, &turns);
    rs_read_int(inf, &quest_item);
    rs_read_char(inf, &nfloors);
    rs_read(inf, &curpurch, 15);
    rs_read_char(inf, &PLAYER);
    rs_read_char(inf, &take);
    rs_read(inf, &prbuf, LINELEN);
    rs_read_char(inf, &runch);
    rs_read(inf, &whoami, LINELEN);
    rs_read(inf, &fruit, LINELEN);
    rs_read_scrolls(inf);
    rs_read_potions(inf);
    rs_read_rings(inf);
    rs_read_sticks(inf);
    for(i = 0; i < MAXMM; i++)
        rs_read_new_string(inf, &m_guess[i]);
    rs_read_window(inf, cw);
    rs_read_window(inf, mw);
    rs_read_window(inf, stdscr);
    rs_read_boolean(inf, &pool_teleport);
    rs_read_boolean(inf, &inwhgt);
    rs_read_boolean(inf, &after);
    rs_read_boolean(inf, &waswizard);
    rs_read_booleans(inf, m_know, MAXMM);
    rs_read_boolean(inf, &playing);
    rs_read_boolean(inf, &running);
    rs_read_boolean(inf, &wizard);
    rs_read_boolean(inf, &notify);
    rs_read_boolean(inf, &fight_flush);
    rs_read_boolean(inf, &terse);
    rs_read_boolean(inf, &auto_pickup);
    rs_read_boolean(inf, &door_stop);
    rs_read_boolean(inf, &jump);
    rs_read_boolean(inf, &slow_invent);
    rs_read_boolean(inf, &firstmove);
    rs_read_boolean(inf, &askme);
    rs_read_boolean(inf, &in_shell);
    rs_read_boolean(inf, &daytime);
    rs_read_coord(inf, &delta);
    rs_read_levtype(inf, &levtype);

    rs_read_monsters(inf, monsters, NUMMONST+1);

    rs_read_magic_items(inf, things, NUMTHINGS);
    rs_read_magic_items(inf, s_magic, MAXSCROLLS);
    rs_read_magic_items(inf, p_magic, MAXPOTIONS);
    rs_read_magic_items(inf, r_magic, MAXRINGS);
    rs_read_magic_items(inf, ws_magic, MAXSTICKS);
    rs_read_magic_items(inf, m_magic, MAXMM);

    rs_read_coord(inf, &ch_ret);
    rs_read_int(inf, &demoncnt);
    rs_read_int(inf, &fusecnt);
    rs_read_daemons(inf, d_list, MAXDAEMONS);
    rs_read_daemons(inf, f_list, MAXFUSES);
    rs_read_int(inf, &between);

    return(READSTAT);
}
