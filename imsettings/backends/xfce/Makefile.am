NULL =

INCLUDES =							\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""		\
	-DICONDIR="\"$(datadir)/pixmaps\""			\
	$(GCONF_CFLAGS)						\
	$(XFCE_CFLAGS)						\
	$(NULL)

LIBS =								\
	$(GCONF_LIBS)						\
	$(XFCE_LIBS)						\
	$(NULL)


plugindir = $(libdir)/xfce4/mcs-plugins
plugin_LTLIBRARIES =						\
	imsettings_plugin.la					\
	$(NULL)

imsettings_plugin_la_SOURCES =					\
	imsettings_plugin.c					\
	$(NULL)
imsettings_plugin_la_LDFLAGS =					\
	-avoid-version						\
	-module							\
	-no-undefined						\
	$(NULL)

EXTRA_DIST =							\
	$(NULL)

DISTCLEANFILES =						\
	$(NULL)

CLEANFILES =							\
	$(NULL)
