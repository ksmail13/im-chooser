NULL =
size = 24x24

EXTRA_DIST =		\
	$(icon_DATA)	\
	$(NULL)

icondir = $(datadir)/icons/hicolor/$(size)/apps
icon_DATA = im-chooser.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then			\
		echo "Updating Gtk icon cache.";	\
		$(gtk_update_icon_cache);		\
	else						\
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)";	\
	fi

uninstall-hook:
	@-if test -z "$(DESTDIR)"; then				\
		rm $(datadir)/icons/hicolor/icon-theme.cache;	\
	fi

-include $(top_srcdir)/git.mk
