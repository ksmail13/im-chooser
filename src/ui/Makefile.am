NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"libimchooseui\""			\
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

lib_LTLIBRARIES =						\
	 libimchooseui.la					\
	$(NULL)

libimchooseui_la_SOURCES =					\
	im-chooser-simple.c					\
	im-chooser-simple.h					\
	im-chooser-ui.c						\
	im-chooser-ui.h						\
	$(NULL)
libimchooseui_la_LIBADD =					\
	libeggsmclient.la					\
	$(NULL)
