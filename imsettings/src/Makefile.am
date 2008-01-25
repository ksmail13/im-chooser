NULL =

INCLUDES =								\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""			\
	-DXINIT_PATH="\"$(XINIT_PATH)\""				\
	-DXINPUTRC_PATH="\"$(XINPUTRC_PATH)\""				\
	-DXINPUT_PATH="\"$(XINPUT_PATH)\""				\
	-DXINPUT_SUFFIX="\"$(XINPUT_SUFFIX)\""				\
	-I$(top_srcdir)/imsettings					\
	$(IMSETTINGS_CFLAGS)						\
	$(NULL)

LIBS =									\
	@LDFLAGS@							\
	$(IMSETTINGS_LIBS)						\
	$(top_builddir)/imsettings/imsettings/libimsettings.la		\
	$(NULL)


noinst_HEADERS =							\
	imsettings-manager.h						\
	$(NULL)

libexec_PROGRAMS = im-settings-daemon im-info-daemon

im_settings_daemon_SOURCES =						\
	factory.c							\
	imsettings-manager.c						\
	$(NULL)
im_settings_daemon_CFLAGS =						\
	-DG_LOG_DOMAIN="\"IM-Settings-Daemon\""				\
	$(IMDAEMON_CFLAGS)						\
	$(NULL)
im_settings_daemon_LDADD =						\
	$(IMDAEMON_LIBS)						\
	$(NULL)

im_info_daemon_SOURCES =						\
	factory_info.c							\
	imsettings-manager.c						\
	$(NULL)
im_info_daemon_CFLAGS =							\
	-DG_LOG_DOMAIN="\"IM-Info-Daemon\""				\
	$(NULL)

servicedir = $(DBUS_SERVICE_DIR)
service_DATA =								\
	im-info-daemon.service						\
	im-settings-daemon.service					\
	$(NULL)

%.service: %.service.in $(top_builddir)/config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST =								\
	im-info-daemon.service.in					\
	im-settings-daemon.service.in					\
	$(NULL)

DISTCLEANFILES =							\
	im-info-daemon.service						\
	im-settings-daemon.service					\
	$(NULL)