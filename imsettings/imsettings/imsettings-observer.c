/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imsettings-observer.c
 * Copyright (C) 2008 Red Hat, Inc. All rights reserved.
 * 
 * Authors:
 *   Akira TAGOH  <tagoh@redhat.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gi18n-lib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "imsettings.h"
#include "imsettings-observer.h"
#include "imsettings-utils.h"
#include "imsettings-marshal.h"

#define IMSETTINGS_OBSERVER_GET_PRIVATE(_o_)	(G_TYPE_INSTANCE_GET_PRIVATE ((_o_), IMSETTINGS_TYPE_OBSERVER, IMSettingsObserverPrivate))


typedef gboolean (* IMSettingsSignalFunction) (IMSettingsObserver *imsettings,
					       DBusMessage        *message,
					       const gchar        *interface,
					       const gchar        *signal_name,
					       guint               class_offset,
					       GError            **error);

typedef struct _IMSettingsObserverPrivate {
	DBusGConnection *connection;
	gchar           *module_name;
	gboolean         replace;
} IMSettingsObserverPrivate;

typedef struct _IMSettingsObserverSignal {
	const gchar              *interface;
	const gchar              *signal_name;
	guint                     class_offset;
	IMSettingsSignalFunction  callback;
} IMSettingsObserverSignal;


enum {
	PROP_0,
	PROP_MODULE,
	PROP_REPLACE,
	PROP_CONNECTION,
	LAST_PROP
};
enum {
	RELOAD,
	STARTIM,
	STOPIM,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (IMSettingsObserver, imsettings_observer, G_TYPE_OBJECT);

/*
 * Private functions
 */
static gboolean
imsettings_get_list(GObject     *object,
		    const gchar *lang,
		    gchar     ***ret,
		    GError     **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	GPtrArray *list;
	gboolean retval = FALSE;
	gint i;

	if (klass->get_list) {
		list = klass->get_list(IMSETTINGS_OBSERVER (object), lang, error);
		if (*error == NULL) {
			*ret = g_strdupv((gchar **)list->pdata);
			retval = TRUE;
		}
		if (list) {
			for (i = 0; i < list->len; i++) {
				if (g_ptr_array_index(list, i))
					g_free(g_ptr_array_index(list, i));
			}
			g_ptr_array_free(list, TRUE);
		}
	}

	return retval;
}

static gboolean
imsettings_start_im(GObject      *object,
		    const gchar  *lang,
		    const gchar  *module,
		    gboolean     *ret,
		    GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);

	*ret = FALSE;
	if (klass->start_im) {
		*ret = klass->start_im(IMSETTINGS_OBSERVER (object), lang, module, error);
	}

	return *ret;
}

static gboolean
imsettings_stop_im(GObject      *object,
		   const gchar  *lang,
		   const gchar  *module,
		   gboolean      force,
		   gboolean     *ret,
		   GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);

	*ret = FALSE;
	if (klass->stop_im) {
		*ret = klass->stop_im(IMSETTINGS_OBSERVER (object), module, force, error);
	}

	return *ret;
}

static gboolean
imsettings_get_xinput_filename(GObject      *object,
			       const gchar  *module,
			       const gchar **ret,
			       GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_get_filename(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_get_im_module_name(GObject      *object,
			      const gchar  *imname,
			      guint32       type,
			      const gchar **ret,
			      GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), imname, error);
		if (*error == NULL) {
			retval = TRUE;
			switch (type) {
			    case IMSETTINGS_IMM_GTK:
				    *ret = imsettings_info_get_gtkimm(info);
				    break;
			    case IMSETTINGS_IMM_QT:
				    *ret = imsettings_info_get_qtimm(info);
				    break;
			    case IMSETTINGS_IMM_XIM:
				    *ret = imsettings_info_get_xim(info);
				    break;
			    default:
				    g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_INVALID_IMM,
						_("Invalid IM module type: %d"), type);
				    retval = FALSE;
				    break;
			}
		}
	}

	return retval;
}

static gboolean
imsettings_get_xim_program(GObject      *object,
			   const gchar  *module,
			   const gchar **progname,
			   const gchar **progargs,
			   GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*progname = imsettings_info_get_xim_program(info);
			*progargs = imsettings_info_get_xim_args(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_get_preferences_program(GObject      *object,
				   const gchar  *module,
				   const gchar **progname,
				   const gchar **progargs,
				   GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*progname = imsettings_info_get_prefs_program(info);
			*progargs = imsettings_info_get_prefs_args(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_get_auxiliary_program(GObject      *object,
				 const gchar  *module,
				 const gchar **progname,
				 const gchar **progargs,
				 GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*progname = imsettings_info_get_aux_program(info);
			*progargs = imsettings_info_get_aux_args(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_get_short_description(GObject      *object,
				 const gchar  *module,
				 const gchar **ret,
				 GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_get_short_desc(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_get_long_description(GObject      *object,
				const gchar  *module,
				const gchar **ret,
				GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_get_long_desc(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_is_system_default(GObject      *object,
			     const gchar  *module,
			     gboolean     *ret,
			     GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_is_system_default(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_is_user_default(GObject      *object,
			   const gchar  *module,
			   gboolean     *ret,
			   GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_is_user_default(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_is_xim(GObject      *object,
		  const gchar  *module,
		  gboolean     *ret,
		  GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_is_xim(info);
			retval = TRUE;
		}
	}

	return retval;
}

static gboolean
imsettings_get_supported_language(GObject      *object,
				  const gchar  *module,
				  const gchar **ret,
				  GError      **error)
{
	IMSettingsObserverClass *klass = IMSETTINGS_OBSERVER_GET_CLASS (object);
	IMSettingsInfo *info;
	gboolean retval = FALSE;

	if (klass->get_info) {
		info = klass->get_info(IMSETTINGS_OBSERVER (object), module, error);
		if (*error == NULL) {
			*ret = imsettings_info_get_supported_language(info);
			retval = TRUE;
		}
	}

	return retval;
}

#include "imsettings-glib-glue.h"


static void
imsettings_observer_set_property(GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	IMSettingsObserverPrivate *priv = IMSETTINGS_OBSERVER_GET_PRIVATE (object);
	DBusGConnection *connection;

	switch (prop_id) {
	    case PROP_MODULE:
		    if (priv->module_name)
			    g_free(priv->module_name);
		    priv->module_name = g_strdup(g_value_get_string(value));
		    g_object_notify(object, "module");
		    break;
	    case PROP_REPLACE:
		    priv->replace = g_value_get_boolean(value);
		    g_object_notify(object, "replace");
		    break;
	    case PROP_CONNECTION:
		    connection = g_value_get_boxed(value);
		    dbus_g_connection_ref(connection);
		    if (priv->connection)
			    dbus_g_connection_unref(priv->connection);
		    priv->connection = connection;
		    g_object_notify(object, "connection");
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
imsettings_observer_get_property(GObject    *object,
				 guint       prop_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	IMSettingsObserverPrivate *priv = IMSETTINGS_OBSERVER_GET_PRIVATE (object);

	switch (prop_id) {
	    case PROP_MODULE:
		    g_value_set_string(value, priv->module_name);
		    break;
	    case PROP_REPLACE:
		    g_value_set_boolean(value, priv->replace);
		    break;
	    case PROP_CONNECTION:
		    g_value_set_boxed(value, priv->connection);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
imsettings_observer_finalize(GObject *object)
{
	IMSettingsObserverPrivate *priv;

	priv = IMSETTINGS_OBSERVER_GET_PRIVATE (object);

	if (priv->module_name)
		g_free(priv->module_name);
	if (priv->connection)
		dbus_g_connection_unref(priv->connection);

	if (G_OBJECT_CLASS (imsettings_observer_parent_class)->finalize)
		G_OBJECT_CLASS (imsettings_observer_parent_class)->finalize(object);
}

static gboolean
imsettings_observer_signal_reload(IMSettingsObserver *imsettings,
				  DBusMessage        *message,
				  const gchar        *interface,
				  const gchar        *signal_name,
				  guint               class_offset,
				  GError            **error)
{
	gboolean force = FALSE;

	dbus_message_get_args(message, NULL, DBUS_TYPE_BOOLEAN, &force, DBUS_TYPE_INVALID);

	g_signal_emit(imsettings, signals[RELOAD], 0, force, NULL);

	return TRUE;
}

static DBusHandlerResult
imsettings_observer_real_message_filter(DBusConnection *connection,
					DBusMessage    *message,
					void           *data)
{
	IMSettingsObserver *imsettings = IMSETTINGS_OBSERVER (data);
	DBusError derror;
	GError *error = NULL;
	IMSettingsObserverSignal signal_table[] = {
		{IMSETTINGS_INTERFACE_DBUS, "Reload",
		 0,
		 imsettings_observer_signal_reload},
		{IMSETTINGS_INFO_INTERFACE_DBUS, "Reload",
		 0,
		 imsettings_observer_signal_reload},
		{NULL, NULL, 0, NULL}
	};
	gint i;

	dbus_error_init(&derror);

	for (i = 0; signal_table[i].interface != NULL; i++) {
		if (dbus_message_is_signal(message, signal_table[i].interface, signal_table[i].signal_name)) {
			if (signal_table[i].callback(imsettings, message,
						     signal_table[i].interface,
						     signal_table[i].signal_name,
						     signal_table[i].class_offset,
						     &error)) {
				return DBUS_HANDLER_RESULT_HANDLED;
			} else {
				if (error) {
					g_warning("Failed to deal with a signal `%s':  %s",
						  signal_table[i].signal_name,
						  error->message);
					g_error_free(error);

					return DBUS_HANDLER_RESULT_HANDLED;
				}
			}
			break;
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
imsettings_observer_real_start_im(IMSettingsObserver *imsettings,
				  const gchar        *lang,
				  const gchar        *module,
				  GError            **error)
{
	g_signal_emit(imsettings, signals[STARTIM], 0, module, NULL);

	return TRUE;
}

static gboolean
imsettings_observer_real_stop_im(IMSettingsObserver *imsettings,
				 const gchar        *module,
				 gboolean            force,
				 GError            **error)
{
	g_signal_emit(imsettings, signals[STOPIM], 0, module, force, NULL);

	return TRUE;
}

static void
imsettings_observer_class_init(IMSettingsObserverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private(klass, sizeof (IMSettingsObserverPrivate));

	object_class->set_property = imsettings_observer_set_property;
	object_class->get_property = imsettings_observer_get_property;
	object_class->finalize = imsettings_observer_finalize;

	klass->message_filter       = imsettings_observer_real_message_filter;
	klass->start_im             = imsettings_observer_real_start_im;
	klass->stop_im              = imsettings_observer_real_stop_im;

	/* properties */
	g_object_class_install_property(object_class, PROP_MODULE,
					g_param_spec_string("module",
							    _("IM module name"),
							    _("An IM module name to be watched signals"),
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_REPLACE,
					g_param_spec_boolean("replace",
							     _("Replace"),
							     _("Replace the running settings daemon with new instance."),
							     FALSE,
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_CONNECTION,
					g_param_spec_boxed("connection",
							   _("DBus connection"),
							   _("An object to be a DBus connection"),
							   DBUS_TYPE_G_CONNECTION,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	/* signals */
	signals[RELOAD] = g_signal_new("reload",
				       G_OBJECT_CLASS_TYPE (klass),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (IMSettingsObserverClass, reload),
				       NULL, NULL,
				       g_cclosure_marshal_VOID__BOOLEAN,
				       G_TYPE_NONE, 1,
				       G_TYPE_BOOLEAN);
	signals[STARTIM] = g_signal_new("start_im",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (IMSettingsObserverClass, s_start_im),
					NULL, NULL,
					g_cclosure_marshal_VOID__STRING,
					G_TYPE_NONE, 1,
					G_TYPE_STRING);
	signals[STOPIM] = g_signal_new("stop_im",
				       G_OBJECT_CLASS_TYPE (klass),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (IMSettingsObserverClass, s_stop_im),
				       NULL, NULL,
				       imsettings_marshal_VOID__STRING_BOOLEAN,
				       G_TYPE_NONE, 2,
				       G_TYPE_STRING, G_TYPE_BOOLEAN);

	dbus_g_object_type_install_info(IMSETTINGS_TYPE_OBSERVER,
					&dbus_glib_imsettings_object_info);
}

static void
imsettings_observer_init(IMSettingsObserver *observer)
{
	IMSettingsObserverPrivate *priv;

	priv = IMSETTINGS_OBSERVER_GET_PRIVATE (observer);
	priv->replace = FALSE;
}

static gboolean
imsettings_observer_setup_dbus(IMSettingsObserver *imsettings,
			       DBusConnection     *connection,
			       const gchar        *service)
{
	IMSettingsObserverPrivate *priv = IMSETTINGS_OBSERVER_GET_PRIVATE (imsettings);
	gint flags, ret;
	DBusError derror;

	dbus_error_init(&derror);
	priv = IMSETTINGS_OBSERVER_GET_PRIVATE (imsettings);

	flags = DBUS_NAME_FLAG_ALLOW_REPLACEMENT;
	if (priv->replace) {
		flags |= DBUS_NAME_FLAG_REPLACE_EXISTING;
	}

	ret = dbus_bus_request_name(connection, service, flags, &derror);
	if (dbus_error_is_set(&derror)) {
		g_printerr("Failed to acquire im-settings-daemon service:\n  %s\n", derror.message);
		dbus_error_free(&derror);

		return FALSE;
	}
	if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_printerr("im-settings-daemon already running. exiting.\n");

		return FALSE;
	} else if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_printerr("Not primary owner of the service, exiting.\n");

		return FALSE;
	}

	return TRUE;
}

/*
 * Public functions
 */
IMSettingsObserver *
imsettings_observer_new(DBusGConnection *connection,
			const gchar     *module)
{
	g_return_val_if_fail (connection != NULL, NULL);

	return (IMSettingsObserver *)g_object_new(IMSETTINGS_TYPE_OBSERVER,
						  "module", module,
						  "connection", connection,
						  NULL);
}

gboolean
imsettings_observer_setup(IMSettingsObserver *imsettings,
			  const gchar        *service)
{
	IMSettingsObserverPrivate *priv;
	DBusError derror;
	DBusConnection *conn;
	gchar *s, *path;

	g_return_val_if_fail (IMSETTINGS_IS_OBSERVER (imsettings), FALSE);

	dbus_error_init(&derror);
	priv = IMSETTINGS_OBSERVER_GET_PRIVATE (imsettings);

	conn = dbus_g_connection_get_connection(priv->connection);

	if (!imsettings_observer_setup_dbus(imsettings, conn, service))
		return FALSE;

	s = g_strdup_printf("type='signal',interface='%s'", service);
	dbus_bus_add_match(conn, s, &derror);
	g_free(s);
	dbus_connection_add_filter(conn,
				   IMSETTINGS_OBSERVER_GET_CLASS (imsettings)->message_filter,
				   imsettings,
				   NULL);
	path = imsettings_generate_dbus_path_from_interface(service);
	dbus_g_connection_register_g_object(priv->connection, path, G_OBJECT (imsettings));
	g_free(path);

	return TRUE;
}
