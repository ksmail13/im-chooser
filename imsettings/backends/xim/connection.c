/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * connection.c
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

#include <glib/gi18n-lib.h>
#include <X11/Xatom.h>
#include "utils.h"
#include "connection.h"


#define XIM_CONNECTION_GET_PRIVATE(_o_)	(G_TYPE_INSTANCE_GET_PRIVATE ((_o_), XIM_TYPE_CONNECTION, XIMConnectionPrivate))
#define XIM_CONNECTION_PROTOCOL_NAME(_n_)	(__xim_event_names + __xim_event_map[(_n_)])

#ifdef GNOME_ENABLE_DEBUG
#define d(e)	e
#else
#define d(e)
#endif

typedef struct _XIMConnectionPrivate {
	Display  *dpy;
	XIMAtoms *atoms;
	gchar    *xim_server_name;
	Window    xim_server_window;
	Window    requestor_window;
	Window    comm_client_window;
	Window    comm_server_window;
	Atom      selection;
	gulong    major_transport;
	gulong    minor_transport;
} XIMConnectionPrivate;

enum {
	PROP_0,
	PROP_DISPLAY,
	PROP_XIM_SERVER,
	PROP_REQUESTOR,
	PROP_COMM_CLIENT,
	PROP_COMM_SERVER,
	LAST_PROP
};
enum {
	LAST_SIGNAL
};


//static guint signals[LAST_SIGNAL] = { 0 };
static gchar __xim_event_names[] = {
	"\0"
	"XIM_CONNECT\0"
	"XIM_CONNECT_REPLY\0"
	"XIM_DISCONNECT\0"
	"XIM_DISCONNECT_REPLY\0"
	"XIM_AUTH_REQUIRED\0"
	"XIM_AUTH_REPLY\0"
	"XIM_AUTH_NEXT\0"
	"XIM_AUTH_SETUP\0"
	"XIM_AUTH_NG\0"
	"XIM_ERROR\0"
	"XIM_OPEN\0"
	"XIM_OPEN_REPLY\0"
	"XIM_CLOSE\0"
	"XIM_CLOSE_REPLY\0"
	"XIM_REGISTER_TRIGGERKEYS\0"
	"XIM_TRIGGER_NOTIFY\0"
	"XIM_TRIGGER_NOTIFY_REPLY\0"
	"XIM_SET_EVENT_MASK\0"
	"XIM_ENCODING_NEGOTIATION\0"
	"XIM_ENCODING_NEGOTIATION_REPLY\0"
	"XIM_QUERY_EXTENSION\0"
	"XIM_QUERY_EXTENSION_REPLY\0"
	"XIM_SET_IM_VALUES\0"
	"XIM_SET_IM_VALUES_REPLY\0"
	"XIM_GET_IM_VALUES\0"
	"XIM_GET_IM_VALUES_REPLY\0"
	"XIM_CREATE_IC\0"
	"XIM_CREATE_IC_REPLY\0"
	"XIM_DESTROY_IC\0"
	"XIM_DESTROY_IC_REPLY\0"
	"XIM_SET_IC_VALUES\0"
	"XIM_SET_IC_VALUES_REPLY\0"
	"XIM_GET_IC_VALUES\0"
	"XIM_GET_IC_VALUES_REPLY\0"
	"XIM_SET_IC_FOCUS\0"
	"XIM_UNSET_IC_FOCUS\0"
	"XIM_FORWARD_EVENT\0"
	"XIM_SYNC\0"
	"XIM_SYNC_REPLY\0"
	"XIM_COMMIT\0"
	"XIM_RESET_IC\0"
	"XIM_RESET_IC_REPLY\0"
	"XIM_GEOMETRY\0"
	"XIM_STR_CONVERSION\0"
	"XIM_STR_CONVERSION_REPLY\0"
	"XIM_PREEDIT_START\0"
	"XIM_PREEDIT_START_REPLY\0"
	"XIM_PREEDIT_DRAW\0"
	"XIM_PREEDIT_CARET\0"
	"XIM_PREEDIT_CARET_REPLY\0"
	"XIM_PREEDIT_DONE\0"
	"XIM_STATUS_START\0"
	"XIM_STATUS_DRAW\0"
	"XIM_STATUS_DONE\0"
	"XIM_PREEDITSTATE\0"
};
static const gint __xim_event_map[] = {
	0,
	1, /* XIM_CONNECT = 1 */
	13, /* XIM_CONNECT_REPLY */
	31, /* XIM_DISCONNECT */
	46, /* XIM_DISCONNECT_REPLY */
	0, /* ??? = 5 */
	0,
	0,
	0,
	0,
	67, /* XIM_AUTH_REQUIRED = 10 */
	85, /* XIM_AUTH_REPLY */
	100, /* XIM_AUTH_NEXT */
	114, /* XIM_AUTH_SETUP */
	129, /* XIM_AUTH_NG */
	0, /* ??? = 15 */
	0,
	0,
	0,
	0,
	141, /* XIM_ERROR = 20 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	151, /* XIM_OPEN = 30 */
	160, /* XIM_OPEN_REPLY */
	175, /* XIM_CLOSE */
	185, /* XIM_CLOSE_REPLY */
	201, /* XIM_REGISTER_TRIGGERKEYS */
	226, /* XIM_TRIGGER_NOTIFY */
	245, /* XIM_TRIGGER_NOTIFY_REPLY */
	270, /* XIM_SET_EVENT_MASK */
	289, /* XIM_ENCODING_NEGOTIATION */
	314, /* XIM_ENCODING_NEGOTIATION_REPLY */
	345, /* XIM_QUERY_EXTENSION */
	365, /* XIM_QUERY_EXTENSION_REPLY */
	391, /* XIM_SET_IM_VALUES */
	409, /* XIM_SET_IM_VALUES_REPLY */
	433, /* XIM_GET_IM_VALUES */
	451, /* XIM_GET_IM_VALUES_REPLY */
	0, /* ??? = 46 */
	0,
	0,
	0,
	475, /* XIM_CREATE_IC = 50 */
	489, /* XIM_CREATE_IC_REPLY */
	509, /* XIM_DESTROY_IC */
	524, /* XIM_DESTROY_IC_REPLY */
	545, /* XIM_SET_IC_VALUES */
	563, /* XIM_SET_IC_VALUES_REPLY */
	587, /* XIM_GET_IC_VALUES */
	605, /* XIM_GET_IC_VALUES_REPLY */
	629, /* XIM_SET_IC_FOCUS */
	646, /* XIM_UNSET_IC_FOCUS */
	665, /* XIM_FORWARD_EVENT */
	683, /* XIM_SYNC */
	692, /* XIM_SYNC_REPLY */
	707, /* XIM_COMMIT */
	718, /* XIM_RESET_IC */
	731, /* XIM_RESET_IC_REPLY */
	0, /* ??? = 66 */
	0,
	0,
	0,
	750, /* XIM_GEOMETRY = 70 */
	763, /* XIM_STR_CONVERSION */
	782, /* XIM_STR_CONVERSION_REPLY */
	807, /* XIM_PREEDIT_START */
	825, /* XIM_PREEDIT_START_REPLY */
	849, /* XIM_PREEDIT_DRAW */
	866, /* XIM_PREEDIT_CARET */
	884, /* XIM_PREEDIT_CARET_REPLY */
	908, /* XIM_PREEDIT_DONE */
	925, /* XIM_STATUS_START */
	942, /* XIM_STATUS_DRAW */
	958, /* XIM_STATUS_DONE */
	974, /* XIM_PREEDITSTATE */
	991
};

G_DEFINE_TYPE (XIMConnection, xim_connection, G_TYPE_OBJECT);

/*
 * Private functions
 */
static void
xim_connection_create_agent(XIMConnection *connection)
{
	XIMConnectionPrivate *priv = XIM_CONNECTION_GET_PRIVATE (connection);

	if (priv->comm_client_window != 0)
		XDestroyWindow(priv->dpy, priv->comm_client_window);
	if (priv->comm_server_window != 0)
		XDestroyWindow(priv->dpy, priv->comm_server_window);

	priv->comm_client_window = XCreateSimpleWindow(priv->dpy,
						       DefaultRootWindow(priv->dpy),
						       0, 0, 1, 1, 0, 0, 0);
	priv->comm_server_window = XCreateSimpleWindow(priv->dpy,
						       DefaultRootWindow(priv->dpy),
						       0, 0, 1, 1, 0, 0, 0);

	XSelectInput(priv->dpy, DefaultRootWindow(priv->dpy), 0);
	XSync(priv->dpy, False);
}

static void
xim_connection_set_property(GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	XIMConnectionPrivate *priv = XIM_CONNECTION_GET_PRIVATE (object);
	Atom a;
	const gchar *name;
	Window w;

	switch (prop_id) {
	    case PROP_DISPLAY:
		    priv->dpy = g_value_get_pointer(value);
		    if (priv->xim_server_name && priv->requestor_window != 0)
			    xim_connection_create_agent(XIM_CONNECTION (object));
		    priv->atoms = xim_get_atoms(priv->dpy);
		    break;
	    case PROP_XIM_SERVER:
		    name = g_value_get_string(value);
		    a = xim_lookup_atom(priv->dpy, name);
		    if (a == None) {
			    g_warning("No such XIM server is running: %s", name);
		    } else {
			    w = XGetSelectionOwner(priv->dpy, a);
			    if (w == 0) {
				    gchar *s = XGetAtomName(priv->dpy, a);

				    g_warning("No selection owner of %s", s);
				    XFree(s);
			    } else {
				    priv->xim_server_window = w;
				    priv->xim_server_name = g_strdup(name);
				    priv->selection = a;
			    }
		    }
		    if (priv->dpy && priv->requestor_window != 0)
			    xim_connection_create_agent(XIM_CONNECTION (object));
		    break;
	    case PROP_REQUESTOR:
		    priv->requestor_window = g_value_get_ulong(value);
		    if (priv->dpy && priv->xim_server_name)
			    xim_connection_create_agent(XIM_CONNECTION (object));
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
xim_connection_get_property(GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	XIMConnectionPrivate *priv = XIM_CONNECTION_GET_PRIVATE (object);

	switch (prop_id) {
	    case PROP_DISPLAY:
		    g_value_set_pointer(value, priv->dpy);
		    break;
	    case PROP_XIM_SERVER:
		    g_value_set_string(value, priv->xim_server_name);
		    break;
	    case PROP_REQUESTOR:
		    g_value_set_ulong(value, priv->requestor_window);
		    break;
	    case PROP_COMM_CLIENT:
		    g_value_set_ulong(value, priv->comm_client_window);
		    break;
	    case PROP_COMM_SERVER:
		    g_value_set_ulong(value, priv->comm_server_window);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
xim_connection_finalize(GObject *object)
{
	XIMConnectionPrivate *priv = XIM_CONNECTION_GET_PRIVATE (object);

	g_free(priv->xim_server_name);

	if (G_OBJECT_CLASS (xim_connection_parent_class)->finalize)
		G_OBJECT_CLASS (xim_connection_parent_class)->finalize(object);
}

static void
xim_connection_class_init(XIMConnectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private(klass, sizeof (XIMConnectionPrivate));

	object_class->set_property = xim_connection_set_property;
	object_class->get_property = xim_connection_get_property;
	object_class->finalize     = xim_connection_finalize;

	/* properties */
	g_object_class_install_property(object_class, PROP_DISPLAY,
					g_param_spec_pointer("display",
							     _("X display"),
							     _("X display to use"),
							     G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_XIM_SERVER,
					g_param_spec_string("xim_server",
							    _("XIM server name"),
							    _("XIM server name to communicate"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_REQUESTOR,
					g_param_spec_ulong("requestor",
							   _("Window ID"),
							   _("Window ID for the requestor"),
							   0,
							   G_MAXULONG,
							   0,
							   G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_COMM_CLIENT,
					g_param_spec_ulong("comm_client_window",
							   _("Communication Window ID"),
							   _("Window ID to communicate a client"),
							   0,
							   G_MAXULONG,
							   0,
							   G_PARAM_READABLE));
	g_object_class_install_property(object_class, PROP_COMM_SERVER,
					g_param_spec_ulong("comm_server_window",
							   _("Communication Window ID"),
							   _("Window ID to communicate a XIM server"),
							   0,
							   G_MAXULONG,
							   0,
							   G_PARAM_READABLE));

	/* signals */
}

static void
xim_connection_init(XIMConnection *connection)
{
	XIMConnectionPrivate *priv = XIM_CONNECTION_GET_PRIVATE (connection);

	priv->xim_server_name = NULL;
	priv->atoms = NULL;
	priv->major_transport = 0;
	priv->minor_transport = 0;
}

/*
 * Public functions
 */
XIMConnection *
xim_connection_new(Display     *dpy,
		   Window       requestor,
		   const gchar *xim_server_name)
{
	g_return_val_if_fail (xim_server_name != NULL, NULL);

	return XIM_CONNECTION (g_object_new(XIM_TYPE_CONNECTION,
					"display", dpy,
					"xim_server", xim_server_name,
					"requestor", requestor,
					NULL));
}

void
xim_connection_forward_event(XIMConnection *connection,
			     XEvent    *event)
{
	XIMConnectionPrivate *priv;
	XSelectionRequestEvent *esr;
	XSelectionEvent *es;
	XClientMessageEvent *ecm;
	Window destination = 0;
	static const gchar *event_names[] = {
		"Reserved in the protocol",
		"Reserved in the protocol",
		"KeyPress",
		"KeyRelease",
		"ButtonPress",
		"ButtonRelease",
		"MotionNotify",
		"EnterNotify",
		"LeaveNotify",
		"FocusIn",
		"FocusOut",
		"KeymapNotify",
		"Expose",
		"GraphicsExpose",
		"NoExpose",
		"VisibilityNotify",
		"CreateNotify",
		"DestroyNotify",
		"UnmapNotify",
		"MapNotify",
		"MapRequest",
		"ReparentNotify",
		"ConfigureNotify",
		"ConfigureRequest",
		"GravityNotify",
		"ResizeRequest",
		"CirculateNotify",
		"CirculateRequest",
		"PropertyNotify",
		"SelectionClear",
		"SelectionRequest",
		"SelectionNotify",
		"ColormapNotify",
		"ClientMessage",
		"MappingNotify",
		NULL
	};
	Atom atom_type;
	int format;
	gulong nitems, bytes;
	unsigned char *prop;


	g_return_if_fail (XIM_IS_CONNECTION (connection));
	g_return_if_fail (event != NULL);
	g_return_if_fail (event->type <= MappingNotify);

	priv = XIM_CONNECTION_GET_PRIVATE (connection);

	switch (event->type) {
	    case SelectionRequest:
		    /* from the client */
		    esr = (XSelectionRequestEvent *)event;
		    d(g_print("%ld: FWD: %s to XIM server: %ld(%ld)->%ld(%ld)[->%ld]\n",
			      priv->comm_client_window,
			      event_names[event->type],
			      esr->requestor, priv->requestor_window,
			      esr->owner, priv->comm_server_window,
			      priv->xim_server_window));
		    destination = esr->owner = priv->xim_server_window;
		    esr->requestor = priv->comm_server_window;
		    esr->selection = priv->selection;
		    break;
	    case SelectionNotify:
		    /* from the XIM server */
		    es = (XSelectionEvent *)event;
		    d(g_print("%ld: FWD: %s from XIM server: %ld->%ld(%ld)[->%ld]\n",
			      priv->comm_client_window,
			      event_names[event->type],
			      priv->xim_server_window,
			      es->requestor, priv->comm_client_window,
			      priv->requestor_window));
		    destination = es->requestor = priv->requestor_window;

		    XGetWindowProperty(priv->dpy, priv->comm_server_window,
				       es->property, 0, 1000000L, True, es->target,
				       &atom_type, &format, &nitems, &bytes, &prop);
		    XChangeProperty(priv->dpy, priv->requestor_window,
				    es->property, es->target, 8, PropModeReplace,
				    prop, nitems);
		    XFree(prop);
		    break;
	    case ClientMessage:
		    G_STMT_START {
			    static const gchar *direction[] = {
				    "->", "<-"
			    };
			    static const gchar *protocol[] = {
				    "_XIM_XCONNECT",
				    "_XIM_MOREDATA",
				    "_XIM_PROTOCOL"
			    };
			    gint protocol_type = 0, event_type = 0;
			    gboolean is_reply = FALSE;
			    Window orig;

			    ecm = (XClientMessageEvent *)event;
			    orig = ecm->window;

			    if (ecm->window == priv->comm_server_window) {
				    is_reply = TRUE;
				    destination = ecm->window = priv->requestor_window;
			    } else {
				    destination = ecm->window = priv->xim_server_window;
			    }
			    
			    if (ecm->message_type == priv->atoms->atom_xim_xconnect) {
				    if (orig == priv->comm_server_window) {
					    /* have to look at the transport version to pick up
					     * and deliver Property too perhaps
					     */
					    priv->major_transport = ecm->data.l[1];
					    priv->minor_transport = ecm->data.l[2];
					    priv->xim_server_window = (Window)ecm->data.l[0];

					    ecm->data.l[0] = priv->comm_client_window;
				    } else {
					    /* the client should be set the transport version to 0:0. */
					    XSelectInput(priv->dpy,
							 priv->comm_server_window,
							 StructureNotifyMask);
					    XSelectInput(priv->dpy,
							 priv->comm_client_window,
							 StructureNotifyMask);

					    ecm->data.l[0] = priv->comm_server_window;
				    }
				    protocol_type = 0;

				    d(g_print("%ld: %s: %s on %s from %ld to %ld\n",
					      priv->comm_client_window,
					      direction[(is_reply ? 1 : 0)],
					      protocol[protocol_type],
					      event_names[event->type],
					      orig,
					      ecm->window));
			    } else if (ecm->message_type == priv->atoms->atom_xim_moredata) {
				    protocol_type = 1;
			    } else if (ecm->message_type == priv->atoms->atom_xim_protocol) {
				    protocol_type = 2;
				    if (ecm->format == 32) {
					    XGetWindowProperty(priv->dpy, orig,
							       ecm->data.l[1],
							       0, ecm->data.l[0],
							       True, AnyPropertyType,
							       &atom_type, &format,
							       &nitems, &bytes, &prop);
					    d(g_print("type: %ld, format: %d, nitems: %ld, bytes: %ld - %ld\n", atom_type, format, nitems, bytes, ecm->data.l[0]));
					    if (prop)
						    XChangeProperty(priv->dpy, ecm->window,
								    ecm->data.l[1],
								    XA_STRING, 8,
								    PropModeAppend, prop, nitems);

					    if (atom_type == None) {
						    gchar *s = XGetAtomName(priv->dpy, ecm->data.l[1]);

						    d(g_print("%ld: %s: *** empty property from %s on %s\n",
							      priv->comm_client_window,
							      direction[(is_reply ? 1 : 0)],
							      s,
							      protocol[protocol_type]));
						    XFree(s);
					    } else {
						    event_type = prop[0];
						    XFree(prop);

						    d(g_print("%ld: %s: %s on %s with Property\n",
							      priv->comm_client_window,
							      direction[(is_reply ? 1 : 0)],
							      XIM_CONNECTION_PROTOCOL_NAME (MIN (event_type, G_N_ELEMENTS (__xim_event_map) - 1)),
							      protocol[protocol_type]));
					    }
				    } else if (ecm->format == 8) {
					    event_type = ecm->data.b[0];

					    d(g_print("%ld: %s: %s on %s\n",
						      priv->comm_client_window,
						      direction[(is_reply ? 1 : 0)],
						      XIM_CONNECTION_PROTOCOL_NAME (MIN (event_type, G_N_ELEMENTS (__xim_event_map) - 1)),
						      protocol[protocol_type]));
				    } else {
					    g_warning("Invalid packet.");
				    }
			    } else {
				    gchar *s = XGetAtomName(priv->dpy, ecm->message_type);

				    g_warning("non XIM message: %s", s);
				    XFree(s);
				    destination = 0;
			    }
		    } G_STMT_END;
		    break;
	    default:
		    g_warning("Unsupported event type %s", event_names[event->type]);
		    break;
	}
	if (destination)
		XSendEvent(priv->dpy,
			   destination,
			   False,
			   NoEventMask,
			   event);
}
