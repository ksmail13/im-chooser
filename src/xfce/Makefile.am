NULL =

AM_CPPFLAGS =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	-I$(top_srcdir)/libimchooseui				\
	$(TOOLKIT_CFLAGS)					\
	$(XFCE4_CFLAGS)						\
	$(WARN_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(TOOLKIT_LIBS)						\
	$(XFCE4_LIBS)						\
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

bin_PROGRAMS = xfce4-im-chooser

xfce4_im_chooser_SOURCES =					\
	main.c							\
	$(NULL)
xfce4_im_chooser_LDADD =					\
	$(top_builddir)/libimchooseui/libimchooseui.la		\
	$(NULL)

desktopdir = $(datadir)/applications
desktop_in_files =						\
	xfce4-im-chooser.desktop.in				\
	$(NULL)

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(NULL)

-include $(top_srcdir)/git.mk
