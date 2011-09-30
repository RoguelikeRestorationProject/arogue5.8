#
# Makefile for rogue
#
# Advanced Rogue
# Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
# All rights reserved.
#
# Based on "Rogue: Exploring the Dungeons of Doom"
# Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#
# See the file LICENSE.TXT for full copyright and licensing information.
#

DISTNAME=arogue5.8.2
PROGRAM=arogue58

O=o

HDRS  = rogue.h mach_dep.h network.h
OBJS1 = chase.$(O) command.$(O) daemon.$(O) daemons.$(O) encumb.$(O) \
        fight.$(O) init.$(O) io.$(O) list.$(O) main.$(O) maze.$(O) mdport.$(O)\
        misc.$(O) monsters.$(O) move.$(O) new_level.$(O) options.$(O) \
	outside.$(O) 
OBJS2 = pack.$(O) passages.$(O) player.$(O) potions.$(O) rings.$(O) rip.$(O) \
        rogue.$(O) rooms.$(O) save.$(O) scrolls.$(O) state.$(O) sticks.$(O) \
        things.$(O) trader.$(O) util.$(O) vers.$(O) weapons.$(O) wear.$(O) \
        wizard.$(O) xcrypt.$(O)
OBJS  = $(OBJS1) $(OBJS2)
CFILES= \
      vers.c chase.c command.c daemon.c daemons.c encumb.c \
      fight.c init.c io.c list.c main.c maze.c mdport.c misc.c monsters.c \
      move.c new_level.c options.c outside.c pack.c passages.c player.c \
      potions.c rings.c rip.c rogue.c \
      rooms.c save.c scrolls.c state.c sticks.c things.c trader.c util.c \
      weapons.c wear.c wizard.c xcrypt.c

MISC=	Makefile LICENSE.TXT arogue58.sln arogue58.vcproj
DOCS= arogue58.doc arogue58.html

CC    = gcc
CFLAGS= -g
CRLIB = -lcurses
RM    = rm -f
TAR   = tar
.SUFFIXES: .obj

.c.obj:
	$(CC) $(CFLAGS) /c $*.c

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(CRLIB) -o $@

tags: $(HDRS) $(CFILES)
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

lint:
	lint -hxbc $(CFILES) $(CRLIB) > linterrs

clean:
	$(RM) $(OBJS1)
	$(RM) $(OBJS2)
	$(RM) core a.exe a.out a.exe.stackdump $(PROGRAM) $(PROGRAM).exe $(PROGRAM).tar $(PROGRAM).tar.gz $(PROGRAM).zip

count:
	wc -l $(HDRS) $(CFILES)

realcount:
	cc -E $(CFILES) | ssp - | wc -l

update:
	ar uv .SAVE $(CFILES) $(HDRS) $(MISC)

dist:
	@mkdir dist
	cp $(CFILES) $(HDRS) $(MISC) dist

dist.src:
	make clean
	tar cf $(DISTNAME)-src.tar $(CFILES) $(HDRS) $(MISC) $(DOCS)
	gzip -f $(DISTNAME)-src.tar

dist.irix:
	make clean
	make CC=cc CFLAGS="-woff 1116 -O3" $(PROGRAM)
	tar cf $(DISTNAME)-irix.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-irix.tar

dist.aix:
	make clean
	make CC=xlc CFLAGS="-qmaxmem=16768 -O3 -qstrict" $(PROGRAM)
	tar cf $(DISTNAME)-aix.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-aix.tar

debug.linux:
	make clean
	make CFLAGS="-g -DWIZARD" $(PROGRAM)

dist.linux:
	make clean
	make $(PROGRAM)
	tar cf $(DISTNAME)-linux.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-linux.tar
	
debug.interix: 
	make clean
	make CFLAGS="-g3 -DWIZARD" $(PROGRAM)
	
dist.interix: 
	make clean
	make $(PROGRAM)
	tar cf $(DISTNAME)-interix.tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-interix.tar
	
debug.cygwin:
	make clean
	make CFLAGS="-g3 -DWIZARD" $(PROGRAM)

dist.cygwin:
	make clean
	make CRLIB="-static -lcurses" $(PROGRAM)
	tar cf $(DISTNAME)-cygwin.tar $(PROGRAM).exe LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-cygwin.tar

#
# Use MINGW32-MAKE to build this target
#
dist.mingw32:
	@$(MAKE) --no-print-directory RM="cmd /c del" clean
	@$(MAKE) --no-print-directory CRLIB="-lpdcurses" $(PROGRAM)
	cmd /c del $(DISTNAME)-mingw32.zip
	zip $(DISTNAME)-mingw32.zip $(PROGRAM).exe LICENSE.TXT $(DOCS)
	
dist.msys:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory CRLIB="-lcurses" $(PROGRAM)
	tar cf $(DISTNAME)-msys.tar $(PROGRAM).exe LICENSE.TXT $(DOCS)
	gzip -f $(DISTNAME)-msys.tar
	
debug.djgpp: 
	make clean
	make CFGLAGS="-g3 -DWIZARD" LDFLAGS="-L$(DJDIR)/LIB" CRLIB="-lpdcurses" $(PROGRAM)

dist.djgpp: 
	make clean
	make LDFLAGS="-L$(DJDIR)/LIB" CRLIB="-lpdcurses" $(PROGRAM)
	rm -f $(DISTNAME)-djgpp.zip
	zip $(DISTNAME)-djgpp.zip $(PROGRAM).exe LICENSE.TXT $(DOCS)

#
# Use NMAKE to build this target
#

debug.win32:
	nmake O="obj" RM="-del" clean
	nmake O="obj" CC="CL" CRLIB="..\pdcurses\pdcurses.lib shfolder.lib user32.lib Advapi32.lib" CFLAGS="-DWIZARD -nologo -I..\pdcurses -Ox -wd4033 -wd4716" $(PROGRAM)

dist.win32:
	nmake O="obj" RM="-del" clean
	nmake O="obj" CC="CL" CRLIB="..\pdcurses\pdcurses.lib shfolder.lib user32.lib Advapi32.lib" CFLAGS="-nologo -I..\pdcurses -Ox -wd4033 -wd4716" $(PROGRAM)
	-del $(DISTNAME)-win32.zip
	zip $(DISTNAME)-win32.zip $(PROGRAM).exe LICENSE.TXT $(DOCS)

vers.$(O): vers.c rogue.h
chase.$(O): chase.c rogue.h
command.$(O): command.c rogue.h
daemon.$(O): daemon.c rogue.h
daemons.$(O): daemons.c rogue.h
encumb.$(O): encumb.c rogue.h
fight.$(O): fight.c rogue.h
init.$(O): init.c rogue.h
io.$(O): io.c rogue.h
list.$(O): list.c rogue.h
main.$(O): main.c rogue.h
maze.$(O): maze.c rogue.h
misc.$(O): misc.c rogue.h
monsters.$(O): monsters.c rogue.h
move.$(O): move.c rogue.h
new_level.$(O): new_level.c rogue.h
options.$(O): options.c rogue.h
outside.$(O): outside.c rogue.h
pack.$(O): pack.c rogue.h
passages.$(O): passages.c rogue.h
player.$(O): player.c rogue.h
potions.$(O): potions.c rogue.h
rings.$(O): rings.c rogue.h
rip.$(O): rip.c rogue.h
rogue.$(O): rogue.c rogue.h
rooms.$(O): rooms.c rogue.h
save.$(O): save.c rogue.h
scrolls.$(O): scrolls.c rogue.h
state.$(O): state.c rogue.h
sticks.$(O): sticks.c rogue.h
things.$(O): things.c rogue.h
trader.$(O): trader.c rogue.h
util.$(O): util.c rogue.h
weapons.$(O): weapons.c rogue.h
wear.$(O): wear.c rogue.h
wizard.$(O): wizard.c rogue.h
xcrypt.$(O): xcrypt.c

