dnl Process this file with autoconf to produce a configure script.
AC_INIT([im-chooser], 1.7.3, [https://pagure.io/im-chooser/issues])

. `dirname $0`/requires

AM_INIT_AUTOMAKE([1.11 -Wno-portability dist-bzip2])
dnl silent build rules, requires at least automake-1.11.
dnl by either passing --enable-silent-rules to configure or
dnl V=0 to make.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AX_CHECK_ENABLE_DEBUG
AM_CONDITIONAL(ENABLE_DEBUG, test $ax_enable_debug = yes)

LT_PREREQ([2.2])
LT_INIT([disable-static])

AC_PROG_CC

GNOME_COMPILE_WARNINGS
GNOME_CXX_WARNINGS

dnl ======================================================================
dnl define variables
dnl ======================================================================
AM_MISSING_PROG([GIT], [git])
AM_MISSING_PROG([DB2MAN], [db2x_docbook2man])

dnl ======================================================================
dnl functions testing
dnl ======================================================================

dnl ======================================================================
dnl gettext stuff
dnl ======================================================================
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

dnl ======================================================================
dnl check pkg-config stuff
dnl ======================================================================
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(GTK3, gtk+-3.0, [
	IMCHOOSE_GTK_VERSION=3],[
	PKG_CHECK_MODULES(GTK2, gtk+-2.0)
	IMCHOOSE_GTK_VERSION=2])
PKG_CHECK_MODULES(SM, sm)
PKG_CHECK_MODULES(XFCE4, libxfce4util-1.0,[
	enable_xfce=yes],[
	enable_xfce=no])
PKG_CHECK_MODULES(IMSETTINGS, imsettings >= $IMSETTINGS_REQUIRED)
AC_SUBST(GTK3_CFLAGS)
AC_SUBST(GTK3_LIBS)
AC_SUBST(GTK2_CFLAGS)
AC_SUBST(GTK2_LIBS)
AC_SUBST(IMSETTINGS_CFLAGS)
AC_SUBST(IMSETTINGS_LIBS)
AC_SUBST(SM_CFLAGS)
AC_SUBST(SM_LIBS)
AC_SUBST(PANEL_CFLAGS)
AC_SUBST(PANEL_LIBS)
AC_SUBST(PANEL_DIR)
AC_SUBST(XFCE4_CFLAGS)
AC_SUBST(XFCE4_LIBS)

AC_SUBST(IMCHOOSE_GTK_VERSION)

AC_DEFINE_UNQUOTED(IMCHOOSE_GTK_VERSION, $IMCHOOSE_GTK_VERSION, [GTK+ version detected by pkg-config])

AM_CONDITIONAL(ENABLE_XFCE4, test "x$enable_xfce" = "xyes")

if test $IMCHOOSE_GTK_VERSION == 3; then
   	TOOLKIT_CFLAGS="$GTK3_CFLAGS"
	TOOLKIT_LIBS="$GTK3_LIBS"
else
	TOOLKIT_CFLAGS="$GTK2_CFLAGS"
	TOOLKIT_LIBS="$GTK2_LIBS"
fi

AC_SUBST(TOOLKIT_CFLAGS)
AC_SUBST(TOOLKIT_LIBS)

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
AC_ARG_ENABLE(rebuilds,
	[AC_HELP_STRING([--disable-rebuilds],
		[disable all source autogeneration rules])],,
	[enable_rebuilds=yes])

dnl ======================================================================
dnl options - rebuilds
dnl ======================================================================
REBUILD=\#
if test "x$enable_rebuilds" = "xyes"; then
	REBUILD=
fi
AC_SUBST(REBUILD)

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
libimchooseui/Makefile
src/Makefile
src/app/Makefile
src/app/im-chooser.desktop.in
src/xfce/Makefile
src/xfce/xfce4-im-chooser.desktop.in
])
AC_OUTPUT

dnl ======================================================================
dnl result
dnl ======================================================================
echo ""
echo "========== Build Information =========="
echo " CFLAGS:                     $CFLAGS $TOOLKIT_CFLAGS $IMSETTINGS_CFLAGS $SM_CFLAGS"
echo " LIBS:                       $LDFLAGS $TOOLKIT_LIBS $IMSETTINGS_LIBS $SM_LIBS"
echo " GTK version:                $IMCHOOSE_GTK_VERSION"
echo ""
