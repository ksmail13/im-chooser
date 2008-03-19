/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * utils.c
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

#include <glib.h>
#include <string.h>
#include <X11/Xatom.h>
#include "utils.h"


void xim_atoms_free(gpointer data);


static GHashTable *atom_tables = NULL;

/*
 * Private functions
 */
void
xim_atoms_free(gpointer data)
{
	XIMAtoms *atoms = data;

	g_free(atoms);
}

/*
 * Public functions
 */
void
xim_init(Display *dpy)
{
	gchar *dpy_name;
	XIMAtoms *atoms;

	g_return_if_fail (dpy != NULL);

	if (atom_tables == NULL)
		atom_tables = g_hash_table_new_full(g_str_hash, g_str_equal,
						    g_free, xim_atoms_free);
	if (xim_is_initialized(dpy))
		return;

	atoms = g_new(XIMAtoms, 1);
	atoms->atom_xim_servers = XInternAtom(dpy, "XIM_SERVERS", False);
	atoms->atom_locales = XInternAtom(dpy, "LOCALES", False);
	atoms->atom_transport = XInternAtom(dpy, "TRANSPORT", False);
	atoms->atom_xim_xconnect = XInternAtom(dpy, "_XIM_XCONNECT", False);
	atoms->atom_xim_protocol = XInternAtom(dpy, "_XIM_PROTOCOL", False);
	atoms->atom_xim_moredata = XInternAtom(dpy, "_XIM_MOREDATA", False);
	atoms->atom_imsettings_comm = XInternAtom(dpy, "_IMSETTINGS_COMM", False);
	XFlush(dpy);

	dpy_name = g_strdup(DisplayString (dpy));
	g_hash_table_insert(atom_tables, dpy_name, atoms);
}

gboolean
xim_is_initialized(Display *dpy)
{
	return xim_get_atoms(dpy) != NULL;
}

void
xim_destroy(Display *dpy)
{
	g_hash_table_remove(atom_tables, DisplayString (dpy));
}

XIMAtoms *
xim_get_atoms(Display *dpy)
{
	g_return_val_if_fail (dpy != NULL, NULL);

	return g_hash_table_lookup(atom_tables, DisplayString (dpy));
}

GQuark
xim_g_error_quark(void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string("xim-error-quark");

	return quark;
}

Atom
xim_lookup_atom(Display     *dpy,
		const gchar *xim_server_name)
{
	XIMAtoms *a;
	Atom atom_type, *atoms, retval = None;
	int format;
	unsigned long nitems, bytes, i;
	unsigned char *prop;
	gchar *s1 = NULL, *s2 = NULL;

	if (xim_server_name == NULL)
		return None;

	s1 = g_strdup_printf("@server=%s", xim_server_name);
	a = xim_get_atoms(dpy);
	XGetWindowProperty(dpy, DefaultRootWindow(dpy), a->atom_xim_servers,
			   0, 1000000L, False, XA_ATOM,
			   &atom_type, &format, &nitems, &bytes, &prop);
	atoms = (Atom *)prop;

	for (i = 0; i < nitems; i++) {
		s2 = XGetAtomName(dpy, atoms[i]);
		if (strcmp(s1, s2) == 0) {
			retval = atoms[i];
			break;
		}
	}

	XFree(atoms);
	g_free(s1);
	if (s2)
		XFree(s2);

	return retval;
}
