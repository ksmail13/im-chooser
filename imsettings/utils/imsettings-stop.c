/* 
 * imsettings-stop.c
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
#include <stdlib.h>
#include <glib/gi18n.h>
#include "imsettings/imsettings.h"
#include "imsettings/imsettings-request.h"

int
main(int    argc,
     char **argv)
{
	IMSettingsRequest *imsettings;
	DBusConnection *connection;

	g_type_init();

	if (argc < 2) {
		gchar *progname = g_path_get_basename(argv[0]);

		g_print("Usage: %s <module name>\n", progname);

		g_free(progname);

		exit(1);
	}
	connection = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	imsettings = imsettings_request_new(connection, IMSETTINGS_INTERFACE_DBUS);
	if (imsettings_request_stop_im(imsettings, argv[1])) {
		g_print("Stopped %s\n", argv[1]);
	} else {
		g_printerr("Failed to stop IM process `%s'\n", argv[1]);
		exit(1);
	}
	g_object_unref(imsettings);

	return 0;
}
