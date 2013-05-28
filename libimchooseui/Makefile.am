NULL =

AM_CPPFLAGS =						\
	-DG_LOG_DOMAIN="\"libimchooseui\""		\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""	\
	-DUIDIR="\"$(datadir)/imchooseui\""		\
	-DBUILDDIR="\"$(abs_builddir)\""		\
	-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_4	\
	$(TOOLKIT_CFLAGS)				\
	$(IMSETTINGS_CFLAGS)				\
	$(SM_CFLAGS)					\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(TOOLKIT_LIBS)						\
	$(IMSETTINGS_LIBS)					\
	$(SM_LIBS)						\
	$(NULL)

EXTRA_DIST =							\
	$(ui_DATA)						\
	marshal.list						\
	$(NULL)

MAINTAINERCLEANFILES =						\
	$(built_public_headers)					\
	$(built_private_headers)				\
	$(built_sources)					\
	$(stamp_files)						\
	$(NULL)

DISTCLEANFILES =						\
	$(built_public_headers)					\
	$(built_private_headers)				\
	$(built_sources)					\
	$(stamp_files)						\
	$(NULL)

CLEANFILES =							\
	xgen-ich						\
	xgen-icc						\
	$(NULL)

BUILT_FILES =							\
	$(built_public_headers)					\
	$(built_private_headers)				\
	$(built_sources)					\
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
	-DEGG_SM_CLIENT_BACKEND_DBUS				\
	$(SM_CFLAGS)						\
	$(NULL)
libeggsmclient_la_LIBADD =					\
	$(TOOLKIT_LIBS)						\
	$(SM_LIBS)						\
	libeggdesktopfile.la					\
	$(NULL)
libeggsmclient_la_SOURCES =					\
	eggsmclient.c						\
	eggsmclient.h						\
	eggsmclient-private.h					\
	eggsmclient-dbus-gnome.c				\
	eggsmclient-dbus-kde.c					\
	eggsmclient-xsmp.c					\
	$(NULL)

lib_LTLIBRARIES =						\
	 libimchooseui.la					\
	$(NULL)

built_public_headers =						\
	$(NULL)
built_private_headers =						\
	imchooseui-marshal.h					\
	$(NULL)
built_sources =							\
	imchooseui-marshal.c					\
	$(NULL)
stamp_files =							\
	stamp-marshal.h						\
	$(NULL)
libimchooseui_la_SOURCES =					\
	$(built_sources)					\
	imchooseui.c						\
	imchooseuicellrendererlabel.c				\
	$(NULL)
libimchooseui_la_LIBADD =					\
	libeggsmclient.la					\
	$(NULL)

noinst_HEADERS =						\
	$(built_public_headers)					\
	$(built_private_headers)				\
	imchooseui.h						\
	imchooseuicellrendererlabel.h				\
	$(NULL)

uidir = $(datadir)/imchooseui
ui_DATA =							\
	imchoose.ui						\
	$(NULL)

##
imchooseui-marshal.h: stamp-marshal.h
	@true
stamp-marshal.h: @REBUILD@ marshal.list Makefile
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=imchoose_ui_marshal $(srcdir)/marshal.list --header > xgen-ich \
	&& (cmp -s xgen-ich imchooseui-marshal.h || cp xgen-ich imchooseui-marshal.h) \
	&& rm -f xgen-ich \
	&& echo timestamp > $(@F)
imchooseui-marshal.c: @REBUILD@ imchooseui-marshal.h Makefile
	$(AM_V_GEN) (echo "#include \"imchooseui-marshal.h\""; \
	$(GLIB_GENMARSHAL) --prefix=imchoose_ui_marshal $(srcdir)/marshal.list --body) > xgen-icc \
	&& cp xgen-icc imchooseui-marshal.c \
	&& rm -f xgen-icc

-include $(top_srcdir)/git.mk
