dnl Process this file with autoconf to produce a configure script.
AC_INIT([im-chooser], 0.99.4, [https://bugzilla.redhat.com/enter_bug.cgi?product=Fedora&component=im-chooser])

. ./requires

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

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
DBUS_SERVICE_DIR=`pkg-config --variable session_bus_services_dir dbus-1`
AC_SUBST(DBUS_SERVICE_DIR)
MOC=`pkg-config --variable moc_location QtCore`
[ -z $MOC ] && MOC=/bin/true
AC_SUBST(MOC)

dnl ======================================================================
dnl functions testing
dnl ======================================================================

dnl ======================================================================
dnl gettext stuff
dnl ======================================================================
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

ALL_LINGUAS="af am ar as be bg bn bn_IN bs ca cs cy da de el en_GB es et eu_ES fa fi fr gl gu he hi hr hu hy id is it lt ja ka kn ko ku lo lv mk ml mr ms my nb nl nn nso or pa pl pt pt_BR ro ru si sk sl sq sr sr@latin sv ta te th tr uk ur vi zh_CN zh_TW zu"
AM_GLIB_GNU_GETTEXT

dnl ======================================================================
dnl check pkg-config stuff
dnl ======================================================================
PKG_CHECK_MODULES(IMCHOOSE, gtk+-2.0)
PKG_CHECK_MODULES(IMSETTINGS, dbus-1 >= $DBUS_REQUIRED
			      dbus-glib-1)
PKG_CHECK_MODULES(GCONF, gconf-2.0,
	[use_gconf="yes"],
	[use_gconf="no"])
PKG_CHECK_MODULES(GNOME, libgnomeui-2.0,
	[AC_DEFINE(USE_GNOME,,[use GNOME library])
	 use_gnome="yes"],
	[AC_MSG_RESULT([no])
	 use_gnome="no"])
PKG_CHECK_MODULES(QT, QtCore
		      QtDBus,
	[use_qt="yes"],
	[use_qt="no"])
PKG_CHECK_MODULES(XFCE, gconf-2.0
			libxfce4mcs-manager-1.0
			libxfce4util-1.0
			xfce-mcs-manager,
	[use_xfce="yes"],
	[use_xfce="no"])
PKG_CHECK_MODULES(XIM, x11,
	[use_xim="yes"],
	[use_xim="no"])
PKG_CHECK_MODULES(X11, x11)
PKG_CHECK_MODULES(IMINFO, gthread-2.0
			  gamin)
AC_SUBST(IMCHOOSE_CFLAGS)
AC_SUBST(IMCHOOSE_LIBS)
AC_SUBST(IMSETTINGS_CFLAGS)
AC_SUBST(IMSETTINGS_LIBS)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)
AC_SUBST(GNOME_CFLAGS)
AC_SUBST(GNOME_LIBS)
AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(XFCE_CFLAGS)
AC_SUBST(XFCE_LIBS)
AC_SUBST(XIM_CFLAGS)
AC_SUBST(XIM_LIBS)
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)
AC_SUBST(IMINFO_CFLAGS)
AC_SUBST(IMINFO_LIBS)

AM_CONDITIONAL(ENABLE_GNOME, test x$use_gnome != xno)
AM_CONDITIONAL(ENABLE_GCONF, test x$use_gconf != xno)
AM_CONDITIONAL(ENABLE_QT, test x$use_qt != xno)
AM_CONDITIONAL(ENABLE_XFCE, test x$use_xfce != xno)
AM_CONDITIONAL(ENABLE_XIM, test x$use_xim != xno)

dnl ======================================================================
dnl check another libraries
dnl ======================================================================
AM_PATH_GLIB_2_0($GLIB_REQUIRED, :,
	AC_MSG_ERROR([
*** GLIB $GLIB_REQUIRED_VERSION or better is required. The latest version of
*** GLIB is always available from ftp://ftp.gtk.org/.]),
    glib)

dnl ======================================================================
dnl options
dnl ======================================================================
AC_ARG_WITH(xinputdir,
	AC_HELP_STRING([--with-xinputdir], [build with the specific xinput script directory])
	,,
	with_xinputdir=no)
AC_ARG_WITH(xinputrcdir,
	AC_HELP_STRING([--with-xinputrcdir], [build with the specific xinputrc directory])
	,,
	with_xinputrcdir=no)
AC_ARG_WITH(xinitdir,
	AC_HELP_STRING([--with-xinitdir], [build with the specific xinit script directory])
	,,
	with_xinitdir=no)
AC_ARG_WITH(xinput-suffix,
	AC_HELP_STRING([--with-xinput-suffix], [specify the xinput file suffix])
	,,
	with_xinput_suffix=no)
AC_ARG_WITH(xinputsh,
	AC_HELP_STRING([--with-xinputsh], [specify the xinput shell script name])
	,,
	with_xinputsh=no)

dnl ======================================================================
dnl options - xinputdir
dnl ======================================================================
AC_MSG_CHECKING(whether to build the specific xinput script directory)
if test "x$with_xinputdir" = xno; then
	with_xinputdir="${sysconfdir}/X11/xinit/xinput.d/"
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi
XINPUT_PATH=$with_xinputdir
AC_SUBST(XINPUT_PATH)

dnl ======================================================================
dnl options - xinputrcdir
dnl ======================================================================
AC_MSG_CHECKING(whether to build the specific xinputrc directory)
if test "x$with_xinputrcdir" = xno; then
	with_xinputrcdir="${sysconfdir}/X11/xinit/"
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi
XINPUTRC_PATH=$with_xinputrcdir
AC_SUBST(XINPUTRC_PATH)

dnl ======================================================================
dnl options - xinputsh
dnl ======================================================================
AC_MSG_CHECKING(whether to give the specific xinput.sh name)
if test "x$with_xinputsh" = xno; then
	with_xinputsh="xinput.sh"
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi
XINPUTSH=$with_xinputsh
AC_SUBST(XINPUTSH)

dnl ======================================================================
dnl options - xinitdir
dnl ======================================================================
AC_MSG_CHECKING(whether to build the specific xinit script directory)
if test "x$with_xinitdir" = xno; then
	with_xinitdir="${sysconfdir}/X11/xinit/xinitrc.d/"
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi
XINIT_PATH=$with_xinitdir
AC_SUBST(XINIT_PATH)

dnl ======================================================================
dnl options - xinit-suffix
dnl ======================================================================
AC_MSG_CHECKING(whether to build the specific xinput suffix)
if test "x$with_xinput_suffix" = xno; then
   	with_xinput_suffix=""
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi
XINPUT_SUFFIX=$with_xinput_suffix
AC_SUBST(XINPUT_SUFFIX)

dnl ======================================================================
dnl output
dnl ======================================================================
AC_CONFIG_FILES([
Makefile
data/Makefile
data/none.in
data/xim.in
data/xinput.sh.in
data/xinputinfo.sh
imsettings/backends/Makefile
imsettings/backends/gconf/Makefile
imsettings/backends/qt/Makefile
imsettings/backends/xfce/Makefile
imsettings/backends/xim/Makefile
imsettings/imsettings/Makefile
imsettings/src/Makefile
imsettings/utils/Makefile
imsettings/Makefile
imsettings/imsettings.pc
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
echo " CFLAGS:                     $IMCHOOSE_CFLAGS"
echo " LIBS:                       $IMCHOOSE_LIBS"
echo " XINPUT_PATH:                $XINPUT_PATH"
echo " XINPUTRC_PATH:              $XINPUTRC_PATH"
echo " XINIT_PATH:                 $XINIT_PATH"
echo " XINPUTSH:                   $XINPUTSH"
echo " XINPUT_SUFFIX:              $XINPUT_SUFFIX"
echo " GNOME support:              $use_gnome"
echo " GConf support:              $use_gconf"
echo " Qt support:                 $use_qt"
echo " Xfce support:               $use_xfce"
echo " XIM support:                $use_xim"
echo ""
