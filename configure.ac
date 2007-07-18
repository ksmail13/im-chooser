dnl Process this file with autoconf to produce a configure script.
AC_INIT([im-chooser], 0.3.4, [tagoh@redhat.com])

. ./requires

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_PROG_LIBTOOL
AC_PROG_INTLTOOL

AC_PROG_CC

dnl ======================================================================
dnl set CFLAGS
dnl ======================================================================
changequote(,)dnl
if test "x$GCC" = "xyes"; then
	case " $CFLAGS " in
	*[\ \	]-Wall[\ \	]*) ;;
	*) CFLAGS="$CFLAGS -Wall"   ;;
	esac

	case " $CFLAGS " in
	*[\ \	]-Werror[\ \	]*) ;;
	*) CFLAGS="$CFLAGS -Werror" ;;
	esac

	if test "x$enable_ansi" = "xyes"; then
		case " $CFLAGS " in
		*[\ \	]-ansi[\ \	]*) ;;
		*) CFLAGS="$CFLAGS -ansi"   ;;
		esac

		case " $CFLAGS " in
		*[\ \	]-pedantic[\ \	]*)   ;;
		*) CFLAGS="$CFLAGS -pedantic" ;;
		esac
	fi
fi
changequote([,])dnl

dnl ======================================================================
dnl functions testing
dnl ======================================================================

dnl ======================================================================
dnl gettext stuff
dnl ======================================================================
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

ALL_LINGUAS="af am ar as be bg bn bn_IN bs ca cs cy da de el en_GB es et eu_ES fa fi fr gl gu he hi hr hu hy id is it lt ja ka kn ko ku lo lv mk ml mr ms my nb nl nn no nso or pa pl pt pt_BR ro ru si sk sl sq sr sr@Latn sv ta te th tr uk ur vi zh_CN zh_TW zu"
AM_GLIB_GNU_GETTEXT

dnl ======================================================================
dnl check pkg-config stuff
dnl ======================================================================
PKG_CHECK_MODULES(IMCHOOSE, gtk+-2.0)
PKG_CHECK_MODULES(GNOME, libgnomeui-2.0,
	[AC_DEFINE(USE_GNOME,,[use GNOME library])
	 use_gnome="yes"],
	[AC_MSG_RESULT([no])
	 use_gnome="no"])
AC_SUBST(IMCHOOSE_CFLAGS)
AC_SUBST(IMCHOOSE_LIBS)
AC_SUBST(GNOME_CFLAGS)
AC_SUBST(GNOME_LIBS)

dnl ======================================================================
dnl check another libraries
dnl ======================================================================

dnl ======================================================================
dnl options
dnl ======================================================================
AC_ARG_WITH(xinputdir,
	AC_HELP_STRING([--with-xinputdir], [build with the specific xinput script directory])
	,,
	with_xinputdir=no)
AC_ARG_WITH(xinitdir,
	AC_HELP_STRING([--with-xinitdir], [build with the specific xinit directory])
	,,
	with_xinitdir=no)
AC_ARG_WITH(xinput-suffix,
	AC_HELP_STRING([--with-xinput-suffix], [specify the xinput file suffix])
	,,
	with_xinput_suffix=no)

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
dnl options - xinitdir
dnl ======================================================================
AC_MSG_CHECKING(whether to build the specific xinit directory)
if test "x$with_xinitdir" = xno; then
	with_xinitdir="${sysconfdir}/X11/xinit/"
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)
fi
XINIT_PATH=$with_xinitdir
AC_SUBST(XINIT_PATH)

dnl ======================================================================
dnl options - xinitdir
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
src/Makefile
src/im-chooser.desktop.in
data/Makefile
data/none.in
data/xim.in
data/xinput.sh
po/Makefile.in
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
echo " XINIT_PATH:                 $XINIT_PATH"
echo " XINPUT_SUFFIX:              $XINPUT_SUFFIX"
echo " GNOME support:              $use_gnome"
echo ""
