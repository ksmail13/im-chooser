NULL =

INCLUDES =							\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""		\
	-I$(top_srcdir)/imsettings				\
	$(IMSETTINGS_CFLAGS)					\
	$(QT_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(QT_LIBS)						\
	$(NULL)

libexec_PROGRAMS = qt-im-settings-daemon

qt_im_settings_daemon_SOURCES =					\
	main.cpp						\
	moc_qt-imsettings.cpp					\
	moc_qt-imsettings-glue.cpp				\
	qt-imsettings.cpp					\
	qt-imsettings-glue.cpp					\
	$(NULL)

noinst_HEADERS =						\
	qt-imsettings.h						\
	$(NULL)

main.cpp: qt-imsettings-glue.h

qt-imsettings.cpp: moc_qt-imsettings.cpp

moc_qt-imsettings.cpp: qt-imsettings.h Makefile
	$(MOC) qt-imsettings.h -o $@

moc_qt-imsettings-glue.cpp: qt-imsettings-glue.h Makefile
	$(MOC) qt-imsettings-glue.h -o $@

qt-imsettings-glue.h qt-imsettings-glue.cpp: introspection.xml Makefile
	qdbusxml2cpp -c IMSettingsQtDBusAdaptor -a $(srcdir)/qt-imsettings-glue $(srcdir)/introspection.xml com.redhat.imsettings.Qt

servicedir = $(DBUS_SERVICE_DIR)
service_DATA = qt-im-settings-daemon.service

%.service: %.service.in $(top_builddir)/config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST =							\
	introspection.xml					\
	qt-im-settings-daemon.service.in			\
	$(NULL)

DISTCLEANFILES =						\
	qt-im-settings-daemon.service				\
	$(NULL)

CLEANFILES =							\
	moc_qt-imsettings.cpp					\
	moc_qt-imsettings-glue.cpp				\
	qt-imsettings-glue.cpp					\
	qt-imsettings-glue.h					\
	$(NULL)
