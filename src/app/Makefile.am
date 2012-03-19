NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	-DDESKTOPFILE="\"$(desktopdir)/$(desktop_DATA)\""	\
	-I$(top_srcdir)/libimchooseui				\
	$(TOOLKIT_CFLAGS)					\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(TOOLKIT_LIBS)						\
	$(NULL)

DEPS =								\
	$(top_builddir)/libimchooseui/libimchooseui.la		\
	$(NULL)

MAINTAINERCLEANFILES =	\
	*.desktop	\
	$(NULL)

DISTCLEANFILES =	\
	*.desktop	\
	$(NULL)

bin_PROGRAMS = im-chooser

im_chooser_SOURCES =						\
	main.c							\
	$(NULL)
im_chooser_LDADD =						\
	$(top_builddir)/libimchooseui/libimchooseui.la		\
	$(NULL)

desktopdir = $(datadir)/applications
desktop_in_files =						\
	im-chooser.desktop.in					\
	$(NULL)

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(NULL)

-include $(top_srcdir)/git.mk
