dnl Process this file with autoconf to produce a configure script.
AC_INIT([im-chooser], 1.5.0, [https://bugzilla.redhat.com/enter_bug.cgi?product=Fedora&component=im-chooser])

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
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(GTK3, gtk+-3.0, [
	IMCHOOSE_GTK_VERSION=3],[
	PKG_CHECK_MODULES(GTK2, gtk+-2.0)
	IMCHOOSE_GTK_VERSION=2])
PKG_CHECK_MODULES(SM, sm)
PKG_CHECK_MODULES(PANEL, libgnome-control-center >= $CCPANEL_REQUIRED,[
	enable_ccpanel=yes
	PANEL_DIR=$(pkg-config --variable=extensiondir libgnome-control-center)],
	enable_ccpanel=no)
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

AM_CONDITIONAL(ENABLE_CCPANEL, test "x$enable_ccpanel" = "xyes")
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
src/ccpanel/Makefile
src/ccpanel/im-chooser-panel.desktop.in
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
