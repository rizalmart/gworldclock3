dnl Process this file with autoconf to produce a configure script.
AC_INIT([gworldclock],[3.0.0])
AM_INIT_AUTOMAKE

dnl I don't know how to get an automated date here...               
dnl AC_DEFINE_UNQUOTED( RELEASE_DATE, `date +"%d %B %Y"` )
AC_DEFINE( RELEASE_DATE, "18 March 2005" )
AH_TEMPLATE( [RELEASE_DATE], [18 March 2005])

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CC

dnl Checks for libraries.
PKG_CHECK_MODULES(GTK3, gtk+-3.0)
AC_SUBST(GTK3_CFLAGS)
AC_SUBST(GTK3_LIBS)
AC_CHECK_LIB(xml2, xmlParseFile)

ALL_LINGUAS=ja
AM_GNU_GETTEXT([external])

localedir='${datadir}/locale'
AC_SUBST(localedir)

dnl Checks for header files.
dnl AC_CHECK_HEADERS(menu.h strings.h)

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

AC_CONFIG_FILES([Makefile po/Makefile.in m4/Makefile ])
AC_OUTPUT
