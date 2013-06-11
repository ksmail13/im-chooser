NULL =

AM_CPPFLAGS =							\
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

BUILT_SOURCES =		\
	$(man1_MANS)	\
	$(NULL)

SUFFIXES = .1.xml

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

man1_MANS =		\
	im-chooser.1	\
	$(NULL)
man_xml_files = $(man1_MANS:.1=.1.xml)

.xml.1:
	$(AM_V_GEN) $(DB2MAN) $<
.1.xml.1:
	$(AM_V_GEN) $(DB2MAN) $<

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(man_xml_files)	\
	$(man1_MANS)		\
	$(NULL)

-include $(top_srcdir)/git.mk
