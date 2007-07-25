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

xinputsh_in_in = xinput.sh.in.in
xinputsh_in = $(xinputsh_in_in:.in=)
xinputsh_DATA = @XINPUTSH@
xinputshdir = @XINIT_PATH@
$(xinputsh_DATA): $(xinputsh_in)
	cp $(xinputsh_in) @XINPUTSH@

icondir = $(datadir)/pixmaps
icon_DATA = im-chooser.png

CONFIGURE_DEPENDENCIES =	\
	xinput.sh.in		\
	$(NULL)

EXTRA_DIST =			\
	$(xinput_in_in_files)	\
	$(xinputsh_in_in)	\
	$(icon_DATA)		\
	$(NULL)

clean-local:
	-rm @XINPUTSH@
	-rm $(xinput_DATA)
	-rm $(xinputsh_DATA)

.PHONY: clean-local
