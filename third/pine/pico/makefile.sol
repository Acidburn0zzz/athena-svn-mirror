# $Id: makefile.sol,v 1.1.1.1 2001-02-19 07:02:08 ghudson Exp $
#
#   Michael Seibel
#   Networks and Distributed Computing
#   Computing and Communications
#   University of Washington
#   Administration Builiding, AG-44
#   Seattle, Washington, 98195, USA
#   Internet: mikes@cac.washington.edu
#
#   Please address all bugs and comments to "pine-bugs@cac.washington.edu"
#
#
#   Pine and Pico are registered trademarks of the University of Washington.
#   No commercial use of these trademarks may be made without prior written
#   permission of the University of Washington.
#
#   Pine, Pico, and Pilot software and its included text are Copyright
#   1989-1998 by the University of Washington.
#
#   The full text of our legal notices is contained in the file called
#   CPYRIGHT, included with this distribution.
#

#
# Makefile for Sun Solaris version of the PINE composer library and 
# stand-alone editor pico.
#
# [Port courtesy of Marc Unangst <mju@mudos.ann-arbor.mi.us> - mss, 921020]
#

RM=          rm -f
LN=          ln -s
MAKE=        make
OPTIMIZE=    # -O
PROFILE=     # -pg
DEBUG=       -g -DDEBUG

# LDCC= /usr/bin/cc
# If you don't have /usr/bin/cc (our Solaris 2.2 doesn't seem to have it,
# it only has /usr/ucb/cc) then change LDCC to the following line and
# give that a try.  This is still using the Solaris compiler but
# leaving out the UCB compatibility includes and libraries.
LDCC= ./cc5.sol

STDCFLAGS=	-Dconst= -Dsv4 -DPOSIX -DJOB_CONTROL -DMOUSE
CFLAGS=         $(OPTIMIZE) $(PROFILE) $(DEBUG) $(EXTRACFLAGS) $(STDCFLAGS)

# switches for library building
LIBCMD=		ar
LIBARGS=	ru
RANLIB=		true

LIBS=		$(EXTRALIBES) -ltermlib

OFILES=		attach.o basic.o bind.o browse.o buffer.o \
		composer.o display.o file.o fileio.o line.o pico_os.o \
		pico.o random.o region.o search.o \
		window.o word.o

HFILES=		headers.h estruct.h edef.h efunc.h pico.h os.h

#
# dependencies for the Unix versions of pico and libpico.a
#
all:		pico pilot
pico pilot:	libpico.a

pico:		main.o
		$(LDCC) $(CFLAGS) main.o libpico.a $(LIBS) -o pico

pilot:		pilot.o
		$(LDCC) $(CFLAGS) pilot.o libpico.a $(LIBS) -o pilot

libpico.a:	$& $(OFILES)
		$(LIBCMD) $(LIBARGS) libpico.a $(OFILES)
		$(RANLIB) libpico.a

clean:
		rm -f *.a *.o *~ pico_os.c os.h pico pilot
		cd osdep; $(MAKE) clean; cd ..

os.h:		osdep/os-sol.h
		$(RM) os.h
		$(LN) osdep/os-sol.h os.h

pico_os.c:	osdep/os-sv4.c
		$(RM) pico_os.c
		$(LN) osdep/os-sv4.c pico_os.c

$(OFILES) main.o pilot.o:	$(HFILES)
pico.o:				ebind.h

osdep/os-sv4.c:	osdep/header osdep/unix osdep/read.pol osdep/raw.ios \
		osdep/spell.unx osdep/term.cap \
		osdep/os-sv4.ic
		cd osdep; $(MAKE) includer os-sv4.c; cd ..
