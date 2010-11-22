NULL =

INCLUDES =						\
	-DG_LOG_DOMAIN="\"IM-Chooser-Panel\""		\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""	\
	$(TOOLKIT_CFLAGS)				\
	$(PANEL_CFLAGS)					\
	$(NULL)

LIBS =							\
	@LDFLAGS@					\
	$(TOOLKIT_LIBS)					\
	$(PANEL_LIBS)					\
	$(NULL)

DEPS =							\
	$(builddir)/../ui/libimchooseui.la		\
	$(NULL)

ccpanel_LTLIBRARIES =					\
	libimchoose-module.la				\
	$(NULL)
ccpaneldir = $(PANELS_DIR)

libimchoose_module_la_SOURCES =				\
	imchoose-module.c				\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = im-chooser-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES =						\
	$(desktop_in_files)				\
	$(desktop_DATA)					\
	$(NULL)
