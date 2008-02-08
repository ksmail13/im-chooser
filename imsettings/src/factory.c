/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * factory.c
 * Copyright (C) 2008 Red Hat, Inc. All rights reserved.
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
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <X11/Xlib.h>
#include "imsettings/imsettings-request.h"
#include "imsettings/imsettings.h"
#include "imsettings-manager.h"


/*
 * Private functions
 */
static void
disconnected_cb(IMSettingsManager *manager)
{
	GMainLoop *loop = g_object_get_data(G_OBJECT (manager), "imsettings-daemon-main");

	g_main_loop_quit(loop);
}

static void
reload_cb(IMSettingsManager *manager,
	  gboolean           force)
{
	if (force) {
		GMainLoop *loop = g_object_get_data(G_OBJECT (manager), "imsettings-daemon-main");

		g_main_loop_quit(loop);
	} else {
		g_print(_("Reloading...\n"));
		imsettings_manager_load_conf(manager);
	}
}

/*
 * Public functions
 */
int
main(int    argc,
     char **argv)
{
	GError *error = NULL;
	GMainLoop *loop;
	IMSettingsManager *manager;
	gboolean arg_replace = FALSE;
	gchar *arg_display_name = NULL, *display_name = NULL;
	GOptionContext *ctx = g_option_context_new(NULL);
	GOptionEntry entries[] = {
		{"replace", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &arg_replace, N_("Replace the running settings daemon with new instance."), NULL},
		{"display", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &arg_display_name, N_("X display to use"), N_("DISPLAY")},
#if 0
		/* FIXME */
		{"screen", 0, 0, G_OPTION_ARG_INT, &arg_screen, N_("X screen to use"), N_("SCREEN")},
#endif
		{NULL, 0, 0, 0, NULL, NULL, NULL}
	};
	DBusGConnection *gconn;
	Display *display;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, IMSETTINGS_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

	g_type_init();

	/* deal with the arguments */
	g_option_context_add_main_entries(ctx, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		if (error != NULL) {
			g_print("%s\n", error->message);
		} else {
			g_warning(_("Unknown error in parsing the command lines."));
		}
		exit(1);
	}
	g_option_context_free(ctx);

	display = XOpenDisplay(arg_display_name);
	if (display == NULL) {
		g_printerr(_("Failed to open a X display."));
		exit(1);
	}
	if (display_name == NULL)
		display_name = g_strdup(DisplayString(display));

	XCloseDisplay(display);

	gconn = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	manager = imsettings_manager_new(gconn, arg_replace);
	if (manager == NULL) {
		g_print("Failed to create an instance for the settings daemon.\n");
		exit(1);
	}
	g_object_set(G_OBJECT (manager), "display_name", display_name, NULL);

	g_signal_connect(manager, "disconnected",
			 G_CALLBACK (disconnected_cb),
			 NULL);
	g_signal_connect(manager, "reload",
			 G_CALLBACK (reload_cb),
			 NULL);

	if (!imsettings_observer_setup(IMSETTINGS_OBSERVER (manager), IMSETTINGS_SERVICE_DBUS)) {
		g_print("Failed to setup the settings daemon.\n");
		exit(1);
	}

	loop = g_main_loop_new(NULL, FALSE);
	g_object_set_data(G_OBJECT (manager), "imsettings-daemon-main", loop);

	g_main_loop_run(loop);

	g_print("exiting from the loop\n");

	g_free(display_name);
	g_object_unref(manager);
	dbus_g_connection_unref(gconn);

	return 0;
}
