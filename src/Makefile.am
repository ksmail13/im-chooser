NULL =

INCLUDES =							\
	-DG_LOG_DOMAIN="\"IM-Chooser\""				\
	-DXINIT_PATH="\"$(XINIT_PATH)\""			\
	-DXINPUT_PATH="\"$(XINPUT_PATH)\""			\
	-DXINPUTRC_PATH="\"$(XINPUTRC_PATH)\""			\
	-DXINPUT_SUFFIX="\"$(XINPUT_SUFFIX)\""			\
	-DIM_XINPUT_SH="\"$(XINPUTSH)\""			\
	-DICONDIR="\"$(datadir)/pixmaps\""			\
	-DIMCHOOSE_LOCALEDIR="\"$(datadir)/locale\""		\
	$(IMCHOOSE_CFLAGS)					\
	$(GNOME_CFLAGS)						\
	$(NULL)

LIBS =								\
	@LDFLAGS@						\
	$(IMCHOOSE_LIBS)					\
	$(GNOME_LIBS)						\
	$(NULL)

bin_PROGRAMS = im-chooser

im_chooser_SOURCES =						\
	main.c							\
	im-chooser.c						\
	im-chooser.h						\
	xinput.c						\
	xinput.h						\
	$(NULL)

desktopdir = $(datadir)/applications
desktop_in_files = im-chooser.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =			\
	$(desktop_in_files)	\
	$(NULL)
