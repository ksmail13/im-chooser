dnl Process this file with autoconf to produce a configure script.
AC_INIT([im-chooser], 1.3.1, [https://bugzilla.redhat.com/enter_bug.cgi?product=Fedora&component=im-chooser])

. ./requires

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER([config.h])

AC_PROG_LIBTOOL
AC_PROG_INTLTOOL

AC_PROG_CC

GNOME_COMPILE_WARNINGS
GNOME_CXX_WARNINGS

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_MAINTAINER_MODE_DEFINES

CFLAGS="$CFLAGS $WARN_CFLAGS"
CXXFLAGS="$CXXFLAGS $WARN_CXXFLAGS"

dnl ======================================================================
dnl define variables
dnl ======================================================================

dnl ======================================================================
dnl functions testing
dnl ======================================================================

dnl ======================================================================
dnl gettext stuff
dnl ======================================================================
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT

dnl ======================================================================
dnl check pkg-config stuff
dnl ======================================================================
PKG_CHECK_MODULES(IMCHOOSE, gtk+-2.0
			    libgnomeui-2.0)
PKG_CHECK_MODULES(IMSETTINGS, imsettings >= $IMSETTINGS_REQUIRED)
AC_SUBST(IMCHOOSE_CFLAGS)
AC_SUBST(IMCHOOSE_LIBS)
AC_SUBST(IMSETTINGS_CFLAGS)
AC_SUBST(IMSETTINGS_LIBS)

dnl ======================================================================
dnl check another libraries
dnl ======================================================================
AM_PATH_GLIB_2_0($GLIB_REQUIRED, :,
	AC_MSG_ERROR([
*** GLIB $GLIB_REQUIRED or better is required. The latest version of
*** GLIB is always available from ftp://ftp.gtk.org/.]),
    glib)

dnl ======================================================================
dnl options
dnl ======================================================================

dnl ======================================================================
dnl output
dnl ======================================================================
AC_CONFIG_FILES([
Makefile
data/Makefile
data/16x16/Makefile
data/22x22/Makefile
data/24x24/Makefile
data/256x256/Makefile
data/32x32/Makefile
data/48x48/Makefile
po/Makefile.in
src/Makefile
src/im-chooser.desktop.in
])
AC_OUTPUT

dnl ======================================================================
dnl result
dnl ======================================================================
echo ""
echo "========== Build Information =========="
echo " CFLAGS:                     $CFLAGS $IMCHOOSE_CFLAGS $IMSETTINGS_CFLAGS"
echo " LIBS:                       $LDFLAGS $IMCHOOSE_LIBS $IMSETTINGS_LIBS"
echo ""
