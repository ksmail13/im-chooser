NULL =
SUBDIRS = ui
if ENABLE_CCPANEL
SUBDIRS += ccpanel
endif

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	-I$(srcdir)/ui						\
	$(TOOLKIT_CFLAGS)					\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(TOOLKIT_LIBS)						\
	$(NULL)

DEPS =								\
	$(builddir)/ui/libimchooseui.la				\
	$(NULL)

bin_PROGRAMS = im-chooser

im_chooser_SOURCES =						\
	main.c							\
	$(NULL)
im_chooser_LDADD =						\
	$(builddir)/ui/libimchooseui.la				\
	$(NULL)

desktopdir = $(datadir)/applications
desktop_in_files =						\
	im-chooser.desktop.in					\
	xfce4-im-chooser.desktop.in				\
	$(NULL)

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(NULL)
