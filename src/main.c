/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * main.c
 * Copyright (C) 2006-2008 Red Hat, Inc. All rights reserved.
 * 
 * Authors:
 *   Akira TAGOH  <tagoh@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include "im-chooser-ui.h"
#include <glib/gi18n.h>
#include "eggsmclient.h"
#include "eggdesktopfile.h"

int
main(int    argc,
     char **argv)
{
	GOptionContext *ctx = g_option_context_new(_("[options...]"));
	GtkWidget *window;
	GError *err = NULL;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, IMCHOOSE_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

	setlocale(LC_ALL, "");

	g_type_init();

	g_option_context_add_group(ctx, gtk_get_option_group(FALSE));
	g_option_context_add_group(ctx, egg_sm_client_get_option_group());

	if (!g_option_context_parse(ctx, &argc, &argv, &err)) {
		g_printerr(_("Could not parse arguments: %s\n"), err->message);
		g_error_free(err);
		return 1;
	}
	if (!gtk_init_check(&argc, &argv)) {
		const char *display_name_arg = gdk_get_display_arg_name();
		if (display_name_arg == NULL)
			display_name_arg = getenv("DISPLAY");
		g_warning("cannot open display: %s", display_name_arg ? display_name_arg : "");
		return 1;
	}

	window = im_chooser_ui_get();

	egg_set_desktop_file(DESKTOPFILE);

	gtk_widget_show_all(window);

	gtk_main();

	g_option_context_free(ctx);

	return 0;
}
