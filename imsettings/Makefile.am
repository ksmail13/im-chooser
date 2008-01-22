NULL =
SUBDIRS = imsettings backends utils src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = imsettings.pc
$(pkgconfig_DATA): $(top_builddir)/config.status

EXTRA_DIST =								\
	imsettings.pc.in						\
	$(NULL)

DISTCLEANFILES =							\
	imsettings.pc							\
	$(NULL)