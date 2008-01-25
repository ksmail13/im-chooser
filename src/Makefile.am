NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DICONDIR="\"$(datadir)/pixmaps\""			\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	-I$(top_srcdir)/imsettings				\
	$(IMCHOOSE_CFLAGS)					\
	$(IMSETTINGS_CFLAGS)					\
	$(GNOME_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(IMCHOOSE_LIBS)					\
	$(IMSETTINGS_LIBS)					\
	$(GNOME_LIBS)						\
	$(NULL)

bin_PROGRAMS = im-chooser

im_chooser_SOURCES =						\
	main.c							\
	im-chooser-simple.c					\
	im-chooser-simple.h					\
	$(NULL)
im_chooser_LDADD =						\
	$(top_builddir)/imsettings/imsettings/libimsettings.la	\
	$(NULL)

desktopdir = $(datadir)/applications
desktop_in_files = im-chooser.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(NULL)
