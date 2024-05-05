bin_PROGRAMS    = gworldclock 
bin_SCRIPTS	= tzwatch
gworldclock_SOURCES   = main.c misc.c resize.c rendezvous.c timer.c zones.c options.c main.h misc.h resize.h rendezvous.h timer.h zones.h options.h
man_MANS	= gworldclock.1 tzwatch.1
EXTRA_DIST 	= config.rpath  $(man_MANS)
INCLUDES        = @GTK3_CFLAGS@
LDADD           = @GTK3_LIBS@
AM_CFLAGS	= -DLOCALEDIR="\"$(localedir)\""
CLEANFILES      = *~
DISTCLEANFILES  = .deps/*.P
AM_CPPFLAGS 	= -I/usr/include/libxml2


SUBDIRS = m4 po

ACLOCAL_AMFLAGS = -I m4
