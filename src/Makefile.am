NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DICONDIR="\"$(datadir)/pixmaps\""			\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	$(IMCHOOSE_CFLAGS)					\
	$(IMSETTINGS_CFLAGS)					\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(IMCHOOSE_LIBS)					\
	$(IMSETTINGS_LIBS)					\
	$(NULL)

bin_PROGRAMS = im-chooser

im_chooser_SOURCES =						\
	main.c							\
	im-chooser-simple.c					\
	im-chooser-simple.h					\
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
