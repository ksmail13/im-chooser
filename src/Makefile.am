NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	-DDESKTOPFILE="\"$(DESKTOPFILE)\""			\
	$(TOOLKIT_CFLAGS)					\
	$(IMSETTINGS_CFLAGS)					\
	$(SM_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(TOOLKIT_LIBS)						\
	$(IMSETTINGS_LIBS)					\
	$(SM_LIBS)						\
	$(NULL)

noinst_LTLIBRARIES =						\
	libeggdesktopfile.la					\
	libeggsmclient.la					\
	$(NULL)

libeggdesktopfile_la_SOURCES =					\
	eggdesktopfile.c					\
	eggdesktopfile.h					\
	$(NULL)

libeggsmclient_la_CFLAGS =					\
	-DEGG_SM_CLIENT_BACKEND_XSMP				\
	$(NULL)
libeggsmclient_la_LIBADD =					\
	$(TOOLKIT_LIBS)						\
	libeggdesktopfile.la					\
	-lSM -lICE						\
	$(NULL)
libeggsmclient_la_SOURCES =					\
	eggsmclient.c						\
	eggsmclient.h						\
	eggsmclient-private.h					\
	eggsmclient-xsmp.c					\
	$(NULL)

bin_PROGRAMS = im-chooser

im_chooser_SOURCES =						\
	main.c							\
	im-chooser-simple.c					\
	im-chooser-simple.h					\
	$(NULL)
im_chooser_LDADD =						\
	libeggsmclient.la					\
	$(NULL)

desktopdir = $(datadir)/applications
desktop_in_files =						\
	im-chooser.desktop.in					\
	xfce4-im-chooser.desktop.in				\
	$(NULL)

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(NULL)
