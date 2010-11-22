NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser-Panel\""			\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	-I$(srcdir)/../ui					\
	$(TOOLKIT_CFLAGS)					\
	$(PANEL_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(TOOLKIT_LIBS)						\
	$(PANEL_LIBS)						\
	$(builddir)/../ui/libimchooseui.la			\
	$(NULL)

DEPS =								\
	$(builddir)/../ui/libimchooseui.la			\
	$(NULL)

ccpanel_LTLIBRARIES =						\
	libim-chooser.la					\
	$(NULL)
ccpaneldir = $(PANEL_DIR)

libim_chooser_la_SOURCES =					\
	main.c							\
	$(NULL)
libim_chooser_la_LDFLAGS =					\
	-export_dynamic						\
	-avoid-version						\
	-module							\
	-no-undefined						\
	-export-symbols-regex '^g_io_module_(load|unload)'	\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = im-chooser-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES =							\
	$(desktop_in_files)					\
	$(desktop_DATA)						\
	$(NULL)
