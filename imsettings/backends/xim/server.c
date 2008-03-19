/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * server.c
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

#include <stdint.h>
#include <glib/gi18n-lib.h>
#include <X11/Xatom.h>
#include "imsettings/imsettings-marshal.h"
#include "connection.h"
#include "utils.h"
#include "server.h"


#define XIM_SERVER_GET_PRIVATE(_o_)	(G_TYPE_INSTANCE_GET_PRIVATE ((_o_), XIM_TYPE_SERVER, XIMServerPrivate))

#ifdef GNOME_ENABLE_DEBUG
#define d(e)	e
#else
#define d(e)
#endif

typedef struct _XIMServerSource {
	GSource    instance;
	GPollFD    poll_fd;
	XIMServer *xim;
} XIMServerSource;

typedef struct _XIMServerPrivate {
	XIMServerSource *event_loop;
	XIMAtoms        *atoms;
	GHashTable      *conn_table;
	gchar           *target_xim_name;
	Window           selection_window;
	Window           target_xim_window;
	gboolean         initialized;
} XIMServerPrivate;

enum {
	PROP_0,
	PROP_DISPLAY,
	PROP_XIM,
	LAST_PROP
};
enum {
	DESTROY,
	LAST_SIGNAL
};


static gboolean _source_prepare                 (GSource     *source,
                                                 gint        *timeout);
static gboolean _source_check                   (GSource     *source);
static gboolean _source_dispatch                (GSource     *source,
                                                 GSourceFunc  callback,
                                                 gpointer     data);
static void     _source_finalize                (GSource     *source);


static guint signals[LAST_SIGNAL] = { 0 };
static GSourceFuncs source_funcs = {
	_source_prepare,
	_source_check,
	_source_dispatch,
	_source_finalize
};

G_DEFINE_TYPE (XIMServer, xim_server, G_TYPE_OBJECT);

/*
 * Private functions
 */
static void
_weak_notify_connection_cb(gpointer  data,
			   GObject  *object)
{
	GHashTable *conn_table = data;
	gconstpointer wc, ws;

	g_object_get(object, "comm_client_window", &wc, NULL);
	g_object_get(object, "comm_server_window", &ws, NULL);
	g_hash_table_remove(conn_table, wc);
	g_hash_table_remove(conn_table, ws);
	d(g_print("%ld: DBG: EOL'd an instance\n", (gulong)wc));
}

static gboolean
_source_prepare(GSource *source,
		gint    *timeout)
{
	XIMServerSource *s = (XIMServerSource *)source;

	/* just waiting for polling fd */
	*timeout = -1;

	return XPending(s->xim->dpy) > 0;
}

static gboolean
_source_check(GSource *source)
{
	XIMServerSource *s = (XIMServerSource *)source;
	gboolean retval = FALSE;

	if (s->poll_fd.revents & G_IO_IN)
		retval = XPending(s->xim->dpy) > 0;

	return retval;
}

static XIMConnection *
_create_connection(XIMServer *server,
		   Window     requestor)
{
	XIMConnection *conn;
	XIMServerPrivate *priv = XIM_SERVER_GET_PRIVATE (server);
	Window wc, ws;

	conn = xim_connection_new(server->dpy,
				  requestor,
				  priv->target_xim_name);
	g_object_get(G_OBJECT (conn), "comm_client_window", &wc, NULL);
	g_object_get(G_OBJECT (conn), "comm_server_window", &ws, NULL);
	d(g_print("%ld: DBG: Adding client %ld\n", wc, requestor));
	g_hash_table_insert(priv->conn_table,
			    (gpointer)wc,
			    conn);
	g_hash_table_insert(priv->conn_table,
			    (gpointer)ws,
			    conn);
	g_object_weak_ref(G_OBJECT (conn),
			  _weak_notify_connection_cb,
			  priv->conn_table);

	return conn;
}

static gboolean
_source_dispatch(GSource     *source,
		 GSourceFunc  callback,
		 gpointer     data)
{
	XIMServerSource *s = (XIMServerSource *)source;
	XIMServerPrivate *priv = XIM_SERVER_GET_PRIVATE (s->xim);

	while (XPending(s->xim->dpy)) {
		XEvent xevent;
		XIMConnection *conn = NULL;
		Window requestor = 0;
		gboolean eol = FALSE, possibly_new = FALSE;

		XNextEvent(s->xim->dpy, &xevent);

		switch (xevent.type) {
		    case SelectionRequest:
			    possibly_new = TRUE;
			    requestor = xevent.xselectionrequest.requestor;
			    break;
		    case SelectionNotify:
			    requestor = xevent.xselection.requestor;
			    eol = TRUE;
			    break;
		    case Expose:
			    d(g_print("EV: Expose\n"));
			    break;
		    case ConfigureNotify:
			    d(g_print("EV: ConfigureNotify\n"));
			    break;
		    case DestroyNotify:
			    d(g_print("EV: DestroyNotify\n"));
			    break;
		    case ClientMessage:
			    if (xevent.xclient.message_type == priv->atoms->atom_imsettings_comm) {
				    if (XGetSelectionOwner(s->xim->dpy,
							   xevent.xclient.data.l[1]) != priv->selection_window) {
					    /* time to say good-bye */
					    g_signal_emit(s->xim, signals[DESTROY], 0, NULL);
				    }
			    } else if (xevent.xclient.message_type == priv->atoms->atom_xim_xconnect) {
				    possibly_new = TRUE;
				    if ((conn = g_hash_table_lookup(priv->conn_table,
								    (gpointer)xevent.xclient.window)) == NULL) {
					    /* this is a request to the XIM server */
					    requestor = xevent.xclient.data.l[0];
				    } else {
					    /* otherwise this is a reply from the XIM server */
					    requestor = xevent.xclient.window;
				    }
			    } else if (xevent.xclient.message_type != priv->atoms->atom_xim_protocol &&
				       xevent.xclient.message_type != priv->atoms->atom_xim_moredata) {
				    d(g_print("EVT: ClientMessage: non XIM message\n"));
			    } else {
				    requestor = xevent.xclient.window;
			    }
			    break;
		    case MappingNotify:
			    d(g_print("EV: MappingNotify\n"));
			    break;
		    default:
			    break;
		}
		if (requestor != 0) {
			conn = g_hash_table_lookup(priv->conn_table, (gpointer)requestor);
			if (conn) {
				xim_connection_forward_event(conn, &xevent);
				if (eol)
					g_object_unref(G_OBJECT (conn));
			} else {
				if (possibly_new) {
					conn = _create_connection(s->xim, requestor);
					xim_connection_forward_event(conn, &xevent);
				} else {
					g_warning("Invalid eventflow detected. no connection is established for %ld. discarding...", requestor);
				}
			}
		}
	}

	return TRUE;
}

static void
_source_finalize(GSource *source)
{
}

static void
xim_server_update_xim_server(gpointer key,
			     gpointer value,
			     gpointer data)
{
	XIMConnection *conn = XIM_CONNECTION (value);

	g_object_set(G_OBJECT (conn), "xim_server", data, NULL);
}

static void
xim_server_set_property(GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	XIMServer *xim = XIM_SERVER (object);
	XIMServerPrivate *priv = XIM_SERVER_GET_PRIVATE (xim);
	const gchar *name;
	Atom a;

	switch (prop_id) {
	    case PROP_DISPLAY:
		    xim->dpy = g_value_get_pointer(value);
		    break;
	    case PROP_XIM:
		    name = g_value_get_string(value);
		    if ((a = xim_lookup_atom(xim->dpy, name)) != None) {
			    Window w = XGetSelectionOwner(xim->dpy, a);
			    gchar *old_xim;

			    if (w != 0) {
				    priv->target_xim_window = w;
				    old_xim = priv->target_xim_name;
				    priv->target_xim_name = g_strdup(name);

				    g_hash_table_foreach(priv->conn_table,
							 xim_server_update_xim_server,
							 priv->target_xim_name);

				    d(g_print("INF: XIM server changes: %s->%s\n", old_xim, name));
				    g_free(old_xim);
			    } else {
				    gchar *s = XGetAtomName(xim->dpy, a);

				    g_warning("No selection owner of %s. XIM server may be not running", s);
				    XFree(s);
			    }
		    } else {
			    g_warning("No such XIM server is running: %s", name);
		    }
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
xim_server_get_property(GObject    *object,
			guint       prop_id,
			GValue     *value,
			GParamSpec *pspec)
{
	XIMServer *xim = XIM_SERVER (object);
	XIMServerPrivate *priv = XIM_SERVER_GET_PRIVATE (xim);

	switch (prop_id) {
	    case PROP_DISPLAY:
		    g_value_set_pointer(value, xim->dpy);
		    break;
	    case PROP_XIM:
		    g_value_set_string(value, priv->target_xim_name);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
xim_server_finalize(GObject *object)
{
	XIMServer *xim = XIM_SERVER (object);
	XIMServerPrivate *priv = XIM_SERVER_GET_PRIVATE (object);

	g_source_destroy((GSource *)priv->event_loop);
	g_hash_table_destroy(priv->conn_table);
	XDestroyWindow(xim->dpy, priv->selection_window);
	g_free(priv->target_xim_name);

	if (G_OBJECT_CLASS (xim_server_parent_class)->finalize)
		G_OBJECT_CLASS (xim_server_parent_class)->finalize(object);
}

static void
xim_server_class_init(XIMServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private(klass, sizeof (XIMServerPrivate));

	object_class->set_property = xim_server_set_property;
	object_class->get_property = xim_server_get_property;
	object_class->finalize     = xim_server_finalize;

	/* properties */
	g_object_class_install_property(object_class, PROP_DISPLAY,
					g_param_spec_pointer("display",
							    _("X display"),
							    _("X display to use"),
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_XIM,
					g_param_spec_string("xim",
							    _("XIM server name"),
							    _("XIM server name to be connected"),
							    NULL,
							     G_PARAM_READWRITE));

	/* signals */
	signals[DESTROY] = g_signal_new("destroy",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (XIMServerClass, destroy),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}

static void
xim_server_init(XIMServer *xim)
{
	XIMServerPrivate *priv = XIM_SERVER_GET_PRIVATE (xim);

	priv->initialized = FALSE;
	priv->atoms = NULL;
	/* object as a value should be destroyed first and it's removed against weak_ref then */
	priv->conn_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	priv->selection_window = 0;
	priv->event_loop = NULL;
	priv->target_xim_name = NULL;
}

/*
 * Public functions
 */
XIMServer *
xim_server_new(Display     *dpy,
	       const gchar *xim)
{
	g_return_val_if_fail (dpy != NULL, NULL);
	g_return_val_if_fail (xim_is_initialized(dpy), NULL);

	return XIM_SERVER (g_object_new(XIM_TYPE_SERVER,
					"display", dpy,
					"xim", xim,
					NULL));
}

gboolean
xim_server_is_initialized(XIMServer *xim)
{
	XIMServerPrivate *priv;

	g_return_val_if_fail (XIM_IS_SERVER (xim), FALSE);

	priv = XIM_SERVER_GET_PRIVATE (xim);

	return priv->initialized;
}

gboolean
xim_server_setup(XIMServer *xim,
		 gboolean   replace)
{
	XIMServerPrivate *priv;
	Window owner;
	Atom atom_server;
	Atom atom_type, *atom_prop;
	unsigned long nitems, bytes, i;
	int mode = PropModePrepend, format;
	gboolean changed = TRUE;
	XClientMessageEvent ev;
	Time timestamp;

	g_return_val_if_fail (XIM_IS_SERVER (xim), FALSE);

	priv = XIM_SERVER_GET_PRIVATE (xim);

	if (priv->initialized)
		return TRUE;

	priv->atoms = xim_get_atoms(xim->dpy);
	atom_server = XInternAtom(xim->dpy, "@server=imsettings", False);
	XFlush(xim->dpy);

	if ((owner = XGetSelectionOwner(xim->dpy, atom_server)) != None) {
		if (!replace) {
			g_warning("XIM Server for IMSettings is already running.");

			return FALSE;
		}
	}
	priv->selection_window = XCreateSimpleWindow(xim->dpy,
						     DefaultRootWindow(xim->dpy),
						     0, 0, 1, 1, 1, 0, 0);
	timestamp = CurrentTime;
	XSetSelectionOwner(xim->dpy, atom_server, priv->selection_window, timestamp);
	XSelectInput(xim->dpy, DefaultRootWindow(xim->dpy), 0);
	XSync(xim->dpy, False);

	if (XGetSelectionOwner(xim->dpy, atom_server) != priv->selection_window) {
		g_warning("Couldn't acquire XIM server selection.");

		return FALSE;
	}

	/* Send ClientMessage to notify new XIM server is running */
	ev.type = ClientMessage;
	ev.window = DefaultRootWindow(xim->dpy);
	ev.message_type = priv->atoms->atom_imsettings_comm;
	ev.format = 32;
	ev.data.l[0] = timestamp;
	ev.data.l[1] = atom_server;
	XSendEvent(xim->dpy, owner, False, NoEventMask, (XEvent *)&ev);

	XGetWindowProperty(xim->dpy, DefaultRootWindow(xim->dpy),
			   priv->atoms->atom_xim_servers, 0, 8192, False, XA_ATOM,
			   &atom_type, &format, &nitems, &bytes,
			   (unsigned char **)(uintptr_t)&atom_prop);
	if (atom_type != XA_ATOM ||
	    format != 32) {
		mode = PropModeReplace;
	} else {
		for (i = 0; i < nitems; i++) {
			if (atom_prop[i] == atom_server) {
				mode = PropModeAppend;
				changed = FALSE;
				break;
			}
		}
	}
	if (nitems)
		XFree(atom_prop);

	XChangeProperty(xim->dpy, DefaultRootWindow(xim->dpy),
			priv->atoms->atom_xim_servers, XA_ATOM, 32, mode,
			(unsigned char *)&atom_server, (changed ? 1 : 0));

	priv->event_loop = (XIMServerSource *)g_source_new(&source_funcs, sizeof (XIMServerSource));
	priv->event_loop->xim = xim;
	priv->event_loop->poll_fd.fd = ConnectionNumber(xim->dpy);
	priv->event_loop->poll_fd.events = G_IO_IN;

	g_source_add_poll((GSource *)priv->event_loop, &priv->event_loop->poll_fd);
	g_source_set_can_recurse((GSource *)priv->event_loop, TRUE);
	g_source_attach((GSource *)priv->event_loop, NULL);

	priv->initialized = TRUE;

	return TRUE;
}
