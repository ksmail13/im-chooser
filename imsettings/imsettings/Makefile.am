NULL =

INCLUDES =						\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""	\
	-DXINPUT_PATH="\"$(XINPUT_PATH)\""		\
	-DXINPUT_SUFFIX="\"$(XINPUT_SUFFIX)\""		\
	-DIMSETTINGS_XINPUT_SH="\"$(XINPUTSH)\""	\
	-I$(top_srcdir)/imsettings/			\
	$(IMSETTINGS_CFLAGS)				\
	$(NULL)

LIBS =							\
	@LDFLAGS@					\
	$(IMSETTINGS_LIBS)				\
	$(NULL)

BUILT_FILES =						\
	imsettings-glib-bindings.h			\
	imsettings-glib-glue.h				\
	$(NULL)

imsettingsincdir = $(includedir)/imsettings
imsettingsinc_HEADERS =					\
	imsettings.h					\
	imsettings-glib-bindings.h			\
	imsettings-info.h				\
	imsettings-observer.h				\
	imsettings-request.h				\
	$(NULL)

imsettings.h: stamp-bindings.h
$(top_srcdir)/imsettings/imsettings/imsettings-glib-bindings.h: stamp-bindings.h Makefile
	@true
stamp-bindings.h: $(srcdir)/imsettings.xml Makefile
	( cd $(srcdir) && \
	dbus-binding-tool --prefix=imsettings \
			  --mode=glib-client \
			  --output=$(srcdir)/imsettings-glib-bindings.h $(srcdir)/imsettings.xml && \
	dbus-binding-tool --prefix=imsettings \
			  --mode=glib-server \
			  --output=$(srcdir)/imsettings-glib-glue.h $(srcdir)/imsettings.xml ) && \
	echo timestamp > $(@F)


lib_LTLIBRARIES = 					\
	libimsettings.la				\
	$(NULL)

libimsettings_la_SOURCES =				\
	imsettings-info.c				\
	imsettings-observer.c				\
	imsettings-request.c				\
	imsettings-utils.c				\
	$(NULL)
libimsettings_la_CFLAGS =				\
	-DG_LOG_DOMAIN="\"IMSettings\""			\
	$(NULL)
libimsettings_la_LDFLAGS =				\
	$(LDFLAGS)					\
	$(NULL)


EXTRA_DIST =						\
	imsettings.xml					\
	imsettings-signals.xml				\
	$(NULL)

CLEANFILES =						\
	$(BUILT_FILES)					\
	stamp-bindings.h				\
	stamp-signals-glue.h				\
	$(NULL)
