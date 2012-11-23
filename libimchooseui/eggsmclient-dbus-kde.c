/*
 * Copyright (C) 2008,2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "eggsmclient.h"
#include "eggsmclient-private.h"

#include "eggdesktopfile.h"

#include <gio/gio.h>

#define KSM_DBUS_NAME      "org.kde.ksmserver"
#define KSM_DBUS_PATH      "/KSMServer"
#define KSM_DBUS_INTERFACE "org.kde.KSMServerInterface"

#define EGG_TYPE_SM_CLIENT_KDBUS            (egg_sm_client_dbus_kde_get_type ())
#define EGG_SM_CLIENT_KDBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_SM_CLIENT_KDBUS, EggSMClientKDBus))
#define EGG_SM_CLIENT_KDBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_SM_CLIENT_KDBUS, EggSMClientKDBusClass))
#define EGG_IS_SM_CLIENT_KDBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_SM_CLIENT_KDBUS))
#define EGG_IS_SM_CLIENT_KDBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_SM_CLIENT_KDBUS))
#define EGG_SM_CLIENT_KDBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_SM_CLIENT_KDBUS, EggSMClientKDBusClass))

typedef struct _EggSMClientKDBus        EggSMClientKDBus;
typedef struct _EggSMClientKDBusClass   EggSMClientKDBusClass;

struct _EggSMClientKDBus
{
	EggSMClient parent;

	GDBusProxy *sm_proxy;
};

struct _EggSMClientKDBusClass
{
	EggSMClientClass parent_class;
};

G_DEFINE_TYPE (EggSMClientKDBus, egg_sm_client_dbus_kde, EGG_TYPE_SM_CLIENT)

static const gchar introspection_xml[] =
	"<node name='/KSMServer'>"
	"  <interface name='org.kde.KSMServerInterface'>"
	"    <method name='logout'>"
	"      <arg type='i' name='confirm' direction='in'/>"
	"      <arg type='i' name='sdtype' direction='in'/>"
	"      <arg type='i' name='sdmode' direction='in'/>"
	"    </method>"
	"  </interface>"
	"</node>";

static GDBusInterfaceInfo *
sm_client_kdbus_get_interface_info(void)
{
	static gsize has_info = 0;
	static GDBusInterfaceInfo *info = NULL;

	if (g_once_init_enter(&has_info)) {
		GError *err = NULL;
		GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(introspection_xml,
										 &err);

		if (err) {
			g_warning(err->message);
			return NULL;
		}
		info = g_dbus_interface_info_ref(introspection_data->interfaces[0]);
		g_dbus_node_info_unref(introspection_data);

		g_once_init_leave(&has_info, 1);
	}

	return info;
}

static gboolean
sm_client_dbus_kde_end_session(EggSMClient         *client,
				 EggSMClientEndStyle  style,
				 gboolean             request_confirmation)
{
	EggSMClientKDBus *dbus = EGG_SM_CLIENT_KDBUS (client);
	GVariant *value;
	GError *err = NULL;

	value = g_dbus_proxy_call_sync(dbus->sm_proxy,
				       "logout",
				       g_variant_new("(iii)",
						     request_confirmation ? 1 : 0,
						     3,
						     style == EGG_SM_CLIENT_END_SESSION_DEFAULT || style == EGG_SM_CLIENT_LOGOUT ? 3 :
						     style == EGG_SM_CLIENT_REBOOT ? 1 : 2),
				       G_DBUS_CALL_FLAGS_NONE,
				       -1,
				       NULL,
				       &err);
	if (value)
		g_variant_unref(value);
	if (err) {
		g_warning(err->message);
		g_error_free(err);

		return FALSE;
	}

	return TRUE;
}

static void
egg_sm_client_dbus_kde_init(EggSMClientKDBus *dbus)
{
}

static void
egg_sm_client_dbus_kde_class_init(EggSMClientKDBusClass *klass)
{
	EggSMClientClass *sm_client_class = EGG_SM_CLIENT_CLASS (klass);

	sm_client_class->end_session = sm_client_dbus_kde_end_session;
}

EggSMClient *
egg_sm_client_dbus_kde_new(void)
{
	GDBusProxy *proxy;
	GDBusConnection *connection;
	EggSMClientKDBus *dbus;
	GError *err = NULL;
	GVariant *value = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
					      G_DBUS_PROXY_FLAGS_NONE,
					      sm_client_kdbus_get_interface_info(),
					      KSM_DBUS_NAME,
					      KSM_DBUS_PATH,
					      KSM_DBUS_INTERFACE,
					      NULL,
					      &err);
	if (err) {
		g_warning(err->message);
		g_error_free(err);
		return NULL;
	}
	connection = g_dbus_proxy_get_connection(proxy);
	value = g_dbus_connection_call_sync(connection,
					    KSM_DBUS_NAME,
					    KSM_DBUS_PATH,
					    "org.freedesktop.DBus.Peer",
					    "Ping",
					    NULL,
					    NULL,
					    G_DBUS_CALL_FLAGS_NO_AUTO_START,
					    -1,
					    NULL,
					    &err);
	if (err || !value) {
		goto bail;
	}

	dbus = g_object_new(EGG_TYPE_SM_CLIENT_KDBUS, NULL);
	dbus->sm_proxy = proxy;

	return EGG_SM_CLIENT (dbus);
  bail:
	if (value)
		g_variant_unref(value);
	if (err)
		g_error_free(err);
	if (proxy)
		g_object_unref(proxy);

	return NULL;
}
