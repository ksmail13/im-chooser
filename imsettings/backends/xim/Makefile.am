NULL =

INCLUDES =								\
	-DIMSETTINGS_LOCALEDIR="\"$(datadir)/locale\""			\
	-I$(top_srcdir)/imsettings					\
	$(IMSETTINGS_CFLAGS)						\
	$(XIM_CFLAGS)							\
	$(null)

LIBS =									\
	@LDFLAGS@							\
	$(IMSETTINGS_LIBS)						\
	$(XIM_LIBS)							\
	$(top_builddir)/imsettings/imsettings/libimsettings.la		\
	$(NULL)

bin_PROGRAMS = imsettings-xim

imsettings_xim_SOURCES =						\
	connection.c							\
	main.c								\
	server.c							\
	utils.c								\
	$(NULL)
imsettings_xim_CFLAGS =							\
	-DG_LOG_DOMAIN="\"IMSettings-XIM\""				\
	$(NULL)
imsettings_xim_LDADD =							\
	$(NULL)

EXTRA_DIST =								\
	$(NULL)
