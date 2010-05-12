NULL =
size = 16x16

EXTRA_DIST =		\
	$(icon_DATA)	\
	$(NULL)

icondir = $(datadir)/icons/hicolor/$(size)/apps
icon_DATA = im-chooser.png

gtk_update_icon_cache = gtk-update-icon-cche -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then			\
		echo "Updating Gtk icon cache.";	\
		$(gtk_update_icon_cache);		\
	else						\
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)";	\
	fi

