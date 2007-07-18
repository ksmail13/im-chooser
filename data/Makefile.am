NULL =

xinputdir = $(XINPUT_PATH)
xinput_in_in_files =		\
	none.in.in		\
	xim.in.in		\
	$(NULL)
xinput_in_files = $(xinput_in_in_files:.in.in=.in)
xinput_DATA = $(xinput_in_files:.in=$(XINPUT_SUFFIX))
$(xinput_DATA): $(xinput_in_files)
	list='$(xinput_in_files)'; for file in $$list; do	\
		cp $$file `echo $$file | sed -e 's|.in$$|$(XINPUT_SUFFIX)|'`;	\
	done

CONFIGURE_DEPENDENCIES =	\
	xinput.sh.in		\
	$(NULL)

icondir = $(datadir)/pixmaps
icon_DATA = im-chooser.png

EXTRA_DIST =			\
	$(xinput_DATA)		\
	$(icon_DATA)		\
	xinput.sh.in		\
	$(NULL)
