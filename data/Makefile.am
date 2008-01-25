NULL =

xinputdir = $(XINPUT_PATH)
xinput_in_in_files =		\
	none.in.in		\
	xim.in.in		\
	$(NULL)
xinput_in_files = $(xinput_in_in_files:.in.in=.in)
xinput_DATA = $(xinput_in_files:.in=$(XINPUT_SUFFIX))
$(xinput_DATA): $(xinput_in_files)
	cp `echo $@ | sed -e 's|$(XINPUT_SUFFIX)$$||'`.in $@

xinputsh_in_in = xinput.sh.in.in
xinputsh_in = $(xinputsh_in_in:.in=)
xinputsh_DATA = @XINPUTSH@
xinputshdir = @XINIT_PATH@
$(xinputsh_DATA): $(xinputsh_in)
	cp $(xinputsh_in) @XINPUTSH@

xinputinfosh_in = xinputinfo.sh.in
xinputinfosh_DATA = $(xinputinfosh_in:.in=)
xinputinfoshdir = $(libexecdir)

icondir = $(datadir)/pixmaps
icon_DATA = im-chooser.png

CONFIGURE_DEPENDENCIES =	\
	xinput.sh.in		\
	$(NULL)

EXTRA_DIST =			\
	$(xinput_in_in_files)	\
	$(xinputsh_in_in)	\
	$(xinputinfosh_in)	\
	$(icon_DATA)		\
	$(NULL)

clean-local:
	-rm $(xinput_DATA)
	-rm $(xinputsh_DATA)
	-rm $(xinputinfosh_DATA)

.PHONY: clean-local
