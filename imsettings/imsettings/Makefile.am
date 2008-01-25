NULL =

INCLUDES =						\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""	\
	-DXINIT_PATH="\"$(XINIT_PATH)\""		\
	-DXINPUT_PATH="\"$(XINPUT_PATH)\""		\
	-DXINPUT_SUFFIX="\"$(XINPUT_SUFFIX)\""		\
	-DXINPUTINFO_PATH="\"$(libexecdir)\""		\
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
	imsettings-marshal.h				\
	$(NULL)

imsettingsincdir = $(includedir)/imsettings
imsettingsinc_HEADERS =					\
	imsettings.h					\
	imsettings-glib-bindings.h			\
	imsettings-info.h				\
	imsettings-observer.h				\
	imsettings-request.h				\
	$(NULL)

imsettings.h: stamp-bindings.h stamp-marshal.h
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
$(top_srcdir)/imsettings/imsettings/imsettings-marshal.h: stamp-marshal.h
	@true
stamp-marshal.h: $(srcdir)/marshal.list Makefile
	( cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=imsettings_marshal $(srcdir)/marshal.list \
			  --header > xgen-imh && \
	(cmp -s xgen-imh imsettings-marshal.h || cp xgen-imh imsettings-marshal.h) && \
	rm -f xgen-imh ) && \
	echo timestamp > $(@F)
$(srcdir)/imsettings-marshal.c: $(srcdir)/marshal.list Makefile
	( cd $(srcdir) && \
	@GLIB_GENMARSHAL@ --prefix=imsettings_marshal $(srcdir)/marshal.list \
			  --body > xgen-imc && \
	cp xgen-imc imsettings-marshal.c) && \
	rm -f xgen-imc

lib_LTLIBRARIES = 					\
	libimsettings.la				\
	$(NULL)

libimsettings_la_SOURCES =				\
	imsettings-info.c				\
	imsettings-marshal.c				\
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
	$(NULL)

CLEANFILES =						\
	$(BUILT_FILES)					\
	stamp-bindings.h				\
	stamp-marshal.h					\
	$(NULL)
