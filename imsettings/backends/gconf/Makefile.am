NULL =

INCLUDES =							\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""		\
	-I$(top_srcdir)/imsettings				\
	$(IMSETTINGS_CFLAGS)					\
	$(GCONF_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(IMSETTINGS_LIBS)					\
	$(GCONF_LIBS)						\
	$(top_builddir)/imsettings/imsettings/libimsettings.la	\
	$(NULL)


libexec_PROGRAMS = gconf-im-settings-daemon

gconf_im_settings_daemon_SOURCES =				\
	main.c							\
	$(NULL)
gconf_im_settings_daemon_CFLAGS =				\
	-DG_LOG_DOMAIN="\"Gconf-IM-Settings-Daemon\""		\
	$(NULL)

gconf-imsettings-glue.h: stamp-glue.h
	@true
stamp-glue.h: $(srcdir)/introspection.xml Makefile
	(cd $(srcdir) && \
	dbus-binding-tool --prefix=gconf_imsettings \
			  --mode=glib-server \
			  --output=$(srcdir)/gconf-imsettings-glue.h $(srcdir)/introspection.xml ) && \
	echo timestamp > $(@F)

servicedir = $(DBUS_SERVICE_DIR)
service_DATA = gconf-im-settings-daemon.service

%.service: %.service.in $(top_builddir)/config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST =							\
	introspection.xml					\
	gconf-im-settings-daemon.service.in			\
	$(NULL)

DISTCLEANFILES =						\
	gconf-im-settings-daemon.service			\
	$(NULL)

CLEANFILES =							\
	gconf-imsettings-glue.h					\
	stamp-glue.h						\
	$(NULL)
