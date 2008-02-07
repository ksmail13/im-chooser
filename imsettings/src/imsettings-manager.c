/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imsettings-manager.c
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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib/gfileutils.h>
#include <glib/gmessages.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include "imsettings/imsettings.h"
#include "imsettings/imsettings-info.h"
#include "imsettings/imsettings-info-private.h"
#include "imsettings/imsettings-request.h"
#include "imsettings/imsettings-observer.h"
#include "imsettings/imsettings-utils.h"
#include "imsettings-manager.h"


#define IMSETTINGS_MANAGER_GET_PRIVATE(_o_)	(G_TYPE_INSTANCE_GET_PRIVATE ((_o_), IMSETTINGS_TYPE_MANAGER, IMSettingsManagerPrivate))


typedef struct _IMSettingsManagerPrivate {
	GHashTable        *im_info_table;
	IMSettingsRequest *gtk_req;
	IMSettingsRequest *xim_req;
	IMSettingsRequest *qt_req;
	gchar             *display_name;
} IMSettingsManagerPrivate;

enum {
	PROP_0,
	PROP_DISPLAY_NAME,
};


G_DEFINE_TYPE (IMSettingsManager, imsettings_manager, IMSETTINGS_TYPE_OBSERVER);

/*
 * Private functions
 */
static void
imsettings_manager_real_set_property(GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (object);

	switch (prop_id) {
	    case PROP_DISPLAY_NAME:
		    if (priv->display_name)
			    g_free(priv->display_name);
		    priv->display_name = g_strdup(g_value_get_string(value));
		    g_object_notify(object, "display_name");
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
imsettings_manager_real_get_property(GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (object);

	switch (prop_id) {
	    case PROP_DISPLAY_NAME:
		    g_value_set_string(value, priv->display_name);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
imsettings_manager_real_finalize(GObject *object)
{
	IMSettingsManagerPrivate *priv;

	priv = IMSETTINGS_MANAGER_GET_PRIVATE (object);

	if (priv->im_info_table)
		g_hash_table_destroy(priv->im_info_table);
	if (priv->gtk_req)
		g_object_unref(priv->gtk_req);
	if (priv->xim_req)
		g_object_unref(priv->xim_req);
	if (priv->qt_req)
		g_object_unref(priv->qt_req);
	if (priv->display_name)
		g_free(priv->display_name);

	if (G_OBJECT_CLASS (imsettings_manager_parent_class)->finalize)
		G_OBJECT_CLASS (imsettings_manager_parent_class)->finalize(object);
}

typedef struct _IMSettingsManagerCollectList {
	GPtrArray   *array;
	const gchar *lang;
} IMSettingsManagerCollectList;

static void
_collect_im_list(gpointer key,
		 gpointer val,
		 gpointer data)
{
	IMSettingsManagerCollectList *v = data;
	IMSettingsInfo *info = IMSETTINGS_INFO (val);

	if (!imsettings_info_is_xim(info)) {
		if (imsettings_info_is_visible(info))
			g_ptr_array_add(v->array, g_strdup(key));
	} else {
		/* need to update the short description with the lang */
		g_object_set(G_OBJECT (info), "language", v->lang, NULL);
		if (imsettings_info_is_visible(info))
			g_ptr_array_add(v->array, g_strdup(imsettings_info_get_short_desc(info)));
	}
}

static GPtrArray *
imsettings_manager_real_get_list(IMSettingsObserver  *imsettings,
				 const gchar         *lang,
				 GError             **error)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (imsettings);
	IMSettingsManagerCollectList v;

	v.array = g_ptr_array_new();
	v.lang = lang;
	g_hash_table_foreach(priv->im_info_table, _collect_im_list, &v);
	if (v.array->len == 0) {
		g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_NOT_AVAILABLE,
			    _("No input methods is available on your system."));
	}
	g_ptr_array_add(v.array, NULL);

	return v.array;
}

static IMSettingsInfo *
imsettings_manager_real_get_info(IMSettingsObserver  *imsettings,
				 const gchar         *module,
				 GError             **error)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (imsettings);
	IMSettingsInfo *info;

	info = g_hash_table_lookup(priv->im_info_table, module);
	if (info == NULL) {
		g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_IM_NOT_FOUND,
			    _("No such input method on your system: %s"), module);
	}

	return info;
}

static gboolean
_remove_pidfile(const gchar  *pidfile,
		GError      **error)
{
	int save_errno;
	gboolean retval = TRUE;

	if (g_unlink(pidfile) == -1) {
		if (error) {
			save_errno = errno;

			g_set_error(error, G_FILE_ERROR,
				    g_file_error_from_errno(save_errno),
				    _("Failed to remove a pidfile: %s"),
				    pidfile);
		}
		retval = FALSE;
	}

	return retval;
}

static gboolean
_start_process(const gchar  *prog_name,
	       const gchar  *prog_args,
	       const gchar  *pidfile,
	       const gchar  *lang,
	       GError      **error)
{
	int fd;
	gchar *contents = NULL;
	gint tried = 0;

	if (prog_name == NULL || prog_name[0] == 0)
		goto end;
  retry:
	if (tried > 1)
		goto end;
	tried++;
	if ((fd = g_open(pidfile, O_CREAT|O_EXCL|O_WRONLY, S_IRUSR|S_IWUSR)) == -1) {
		gsize len = 0;
		int pid, save_errno = errno;

		if (save_errno != EEXIST) {
			g_set_error(error, G_FILE_ERROR,
				    g_file_error_from_errno(save_errno),
				    _("Failed to open a pidfile: %s"),
				    pidfile);
			goto end;
		}
		if (!g_file_get_contents(pidfile, &contents, &len, error))
			goto end;

		if ((pid = atoi(contents)) == 0) {
			/* maybe invalid pidfile. retry after removing a pidfile. */
			g_print("failed to get a pid.\n");
			if (!_remove_pidfile(pidfile, error))
				goto end;

			goto retry;
		} else {
			if (kill((pid_t)pid, 0) == -1) {
				g_print("failed to send a signal.\n");
				/* pid may be invalid. retry after removing a pidfile. */
				if (!_remove_pidfile(pidfile, error))
					goto end;

				goto retry;
			}
		}
		/* the requested IM may be already running */
	} else {
		if (prog_name) {
			gchar *cmd, **argv, **envp = NULL, **env_list;
			static const gchar *env_names[] = {
				"LC_CTYPE",
				NULL
			};
			guint len, i = 0, j = 0;
			GPid pid;

			env_list = g_listenv();
			len = g_strv_length(env_list);
			envp = g_new0(gchar *, G_N_ELEMENTS (env_names) + len + 1);
			for (i = 0; i < len; i++) {
				envp[i] = g_strdup_printf("%s=%s",
							  env_list[i],
							  g_getenv(env_list[i]));
			}
			if (lang) {
				envp[i + j] = g_strdup_printf("%s=%s", env_names[j], lang); j++;
			}
			envp[i + j] = NULL;

			cmd = g_strdup_printf("%s %s", prog_name, (prog_args ? prog_args : ""));
			argv = g_strsplit_set(cmd, " \t", -1);
			if (g_spawn_async(g_get_tmp_dir(), argv, envp,
					  G_SPAWN_STDOUT_TO_DEV_NULL|
					  G_SPAWN_STDERR_TO_DEV_NULL,
					  NULL, NULL, &pid, error)) {
				gchar *s = g_strdup_printf("%d", pid);

				write(fd, s, strlen(s));
				close(fd);
			} else {
				close(fd);
				_remove_pidfile(pidfile, NULL);
			}
			g_strfreev(envp);

			g_free(cmd);
			g_strfreev(argv);
		}
	}
  end:
	if (contents)
		g_free(contents);

	return (*error == NULL);
}

static gboolean
_stop_process(const gchar  *pidfile,
	      const gchar  *type,
	      GError      **error)
{
	gchar *contents = NULL;
	gsize len = 0;
	pid_t pid;
	gboolean retval = FALSE;

	if (!g_file_get_contents(pidfile, &contents, &len, error)) {
		if (g_error_matches(*error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
			/* No pidfile is available. there aren't anything else to do.
			 * basically this is no problem. someone may just did stop an IM
			 * actually not running.
			 */
			g_error_free(*error);
			*error = NULL;
			retval = TRUE;
		} else {
			/* Otherwise that shouldn't be happened.
			 */
			g_return_val_if_reached(FALSE);
		}
	} else {
		if ((pid = atoi(contents)) == 0) {
			/* maybe invalid pidfile. */
			g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_UNABLE_TO_TRACK_IM,
				    _("Couldn't determine the pid for %s process."),
				    type);
			goto end;
		}
		if (kill(pid, SIGTERM) == -1) {
			g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_UNABLE_TO_TRACK_IM,
				    _("Couldn't send a signal to the %s process successfully."),
				    type);
		} else {
			_remove_pidfile(pidfile, NULL);
			retval = TRUE;
		}
	}
  end:
	if (contents)
		g_free(contents);

	return retval;
}

static gchar *
_build_pidfilename(const gchar *base,
		   const gchar *display_name,
		   const gchar *type)
{
	gchar *retval, *hash, *file;

#if 0
	hash = g_compute_checksum_for_string(G_CHECKSUM_MD5, base, -1);
#else
	G_STMT_START {
		gint i;

		hash = g_path_get_basename(base);
		if (strcmp(hash, IMSETTINGS_USER_XINPUT_CONF ".bak") == 0) {
			hash[strlen(IMSETTINGS_USER_XINPUT_CONF)] = 0;
		}
		for (i = 0; hash[i] != 0; i++) {
			if (hash[i] < 0x30 ||
			    (hash[i] >= 0x3a && hash[i] <= 0x3f) ||
			    hash[i] == '\\' ||
			    hash[i] == '`' ||
			    hash[i] >= 0x7b)
				hash[i] = '_';
		}
	} G_STMT_END;
#endif
	file = g_strdup_printf("%s:%s:%s-%s", hash, type, display_name, g_get_user_name());
	retval = g_build_filename(g_get_tmp_dir(), file, NULL);

	g_free(hash);
	g_free(file);

	return retval;
}

static gboolean
_update_symlink(IMSettingsManagerPrivate  *priv,
		IMSettingsInfo            *info,
		GError                   **error)
{
	struct stat st;
	const gchar *homedir, *f;
	gchar *conffile = NULL, *backfile = NULL, *xinputfile = NULL, *p, *n;;
	int save_errno;

	homedir = g_get_home_dir();
	if (homedir == NULL) {
		g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
			    _("Failed to get a place of home directory."));
		return FALSE;
	}
	conffile = g_build_filename(homedir, IMSETTINGS_USER_XINPUT_CONF, NULL);
	backfile = g_build_filename(homedir, IMSETTINGS_USER_XINPUT_CONF ".bak", NULL);
	f = imsettings_info_get_filename(info);
	if (f == NULL) {
		g_assert_not_reached();
	}
	p = g_path_get_dirname(f);
	n = g_path_get_basename(f);
	if (strcmp(p, homedir) == 0 &&
	    strcmp(n, IMSETTINGS_USER_XINPUT_CONF) == 0) {
	} else if (strcmp(p, homedir) == 0 &&
		   strcmp(n, IMSETTINGS_USER_XINPUT_CONF ".bak") == 0) {
		/* try to revert the backup file for the user specific conf file */
		if (g_rename(backfile, conffile) == -1) {
			save_errno = errno;

			g_set_error(error, G_FILE_ERROR,
				    g_file_error_from_errno(save_errno),
				    _("Failed to revert the backup file: %s"),
				    g_strerror(save_errno));
			goto end;
		}
	} else {
		if (lstat(conffile, &st) == 0) {
			if (!S_ISLNK (st.st_mode)) {
				/* .xinputrc was probably made by the hand. */
				if (g_rename(conffile, backfile) == -1) {
					save_errno = errno;
					g_set_error(error, G_FILE_ERROR,
						    g_file_error_from_errno(save_errno),
						    _("Failed to create a backup file: %s"),
						    g_strerror(save_errno));
					goto end;
				}
			} else {
				if (g_unlink(conffile) == -1) {
					save_errno = errno;
					g_set_error(error, G_FILE_ERROR,
						    g_file_error_from_errno(save_errno),
						    _("Failed to remove a .xinputrc file: %s"),
						    g_strerror(save_errno));
					goto end;
				}
			}
		}
		xinputfile = g_strdup(f);
	}
	g_free(n);
	g_free(p);

	if (xinputfile) {
		if (symlink(xinputfile, conffile) == -1) {
			save_errno = errno;

			g_set_error(error, G_FILE_ERROR,
				    g_file_error_from_errno(save_errno),
				    _("Failed to make a symlink: %s"),
				    g_strerror(save_errno));
		}
	}
  end:
	if (conffile)
		g_free(conffile);
	if (backfile)
		g_free(backfile);
	if (xinputfile)
		g_free(xinputfile);

	return (*error == NULL);
}

static gboolean
imsettings_manager_real_start_im(IMSettingsObserver  *imsettings,
				 const gchar         *lang,
				 const gchar         *module,
				 GError             **error)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (imsettings);
	IMSettingsInfo *info;
	const gchar *imm, *xinputfile, *aux_prog = NULL, *aux_args = NULL;
	const gchar *xim_prog = NULL, *xim_args = NULL;
	gchar *pidfile = NULL;
	gboolean retval = FALSE;
	IMSettingsRequest *req;
	DBusConnection *conn;

	g_print("Starting %s...\n", module);

	info = imsettings_manager_real_get_info(imsettings, module, error);
	if (info) {
		xinputfile = imsettings_info_get_filename(info);
		aux_prog = imsettings_info_get_aux_program(info);
		aux_args = imsettings_info_get_aux_args(info);
		pidfile = _build_pidfilename(xinputfile, priv->display_name, "aux");

		/* bring up an auxiliary program */
		if (!_start_process(aux_prog, aux_args, pidfile, lang, error))
			goto end;

		g_free(pidfile);
		xim_prog = imsettings_info_get_xim_program(info);
		xim_args = imsettings_info_get_xim_args(info);
		pidfile = _build_pidfilename(xinputfile, priv->display_name, "xim");

		/* bring up a XIM server */
		if (!_start_process(xim_prog, xim_args, pidfile, lang, error))
			goto end;

		/* FIXME: We need to take care of imsettings per X screens?
		 */
		imm = imsettings_info_get_gtkimm(info);
		imsettings_request_change_to(priv->gtk_req, imm);
#if 0
		imm = imsettings_info_get_xim(info);
		imsettings_request_change_to(priv->xim_req, imm);
		imm = imsettings_info_get_qtimm(info);
		imsettings_request_change_to(priv->qt_req, imm);
#endif

		/* Finally update a symlink on your home */
		if (!_update_symlink(priv, info, error))
			goto end;

		if (*error == NULL) {
			imsettings_manager_load_conf(IMSETTINGS_MANAGER (imsettings));
			conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
			req = imsettings_request_new(conn, IMSETTINGS_INFO_INTERFACE_DBUS);
			imsettings_request_reload(req, FALSE);
			g_object_unref(req);
			retval = TRUE;
		}
	}
  end:
	if (pidfile)
		g_free(pidfile);

	return retval;
}

static gboolean
imsettings_manager_real_stop_im(IMSettingsObserver  *imsettings,
				const gchar         *module,
				gboolean             force,
				GError             **error)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (imsettings);
	IMSettingsInfo *info, *oinfo;
	const gchar *xinputfile, *homedir;
	gchar *pidfile = NULL, *conffile;
	gboolean retval = FALSE;
	GString *strerr = g_string_new(NULL);
	IMSettingsRequest *req;
	DBusConnection *conn;

	g_print("Stopping %s...\n", module);

	info = imsettings_manager_real_get_info(imsettings, module, error);
	if (info) {
		xinputfile = imsettings_info_get_filename(info);
		pidfile = _build_pidfilename(xinputfile, priv->display_name, "aux");

		/* kill an auxiliary program */
		if (!_stop_process(pidfile, "aux", error)) {
			if (force) {
				g_string_append_printf(strerr, "%s\n", (*error)->message);
				g_error_free(*error);
				*error = NULL;
				_remove_pidfile(pidfile, NULL);
			} else {
				goto end;
			}
		}
		g_free(pidfile);
		pidfile = _build_pidfilename(xinputfile, priv->display_name, "xim");

		/* kill a XIM server */
		if (!_stop_process(pidfile, "xim", error)) {
			if (force) {
				g_string_append_printf(strerr, "%s\n", (*error)->message);
				g_error_free(*error);
				*error = NULL;
				_remove_pidfile(pidfile, NULL);
			} else {
				goto end;
			}
		}

		/* FIXME: We need to take care of imsettings per X screens?
		 */
		imsettings_request_change_to(priv->gtk_req, NULL);
#if 0
		imsettings_request_change_to(priv->xim_req, NULL);
		imsettings_request_change_to(priv->qt_req, NULL);
#endif

		/* finally update .xinputrc */
		homedir = g_get_home_dir();
		if (homedir == NULL) {
			g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
				    _("Failed to get a place of home directory."));
			goto end;
		}
		conffile = g_build_filename(homedir, IMSETTINGS_USER_XINPUT_CONF, NULL);
		oinfo = imsettings_info_new(conffile);
		if (oinfo == NULL || imsettings_info_compare(oinfo, info)) {
			if (oinfo)
				g_object_unref(oinfo);
			oinfo = imsettings_info_new(XINPUT_PATH IMSETTINGS_NONE_CONF XINPUT_SUFFIX);
			if (oinfo == NULL) {
				g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_FAILED,
					    _("The appropriate configuration file wasn't found. maybe the installation failed: %s"),
					    XINPUT_PATH IMSETTINGS_NONE_CONF XINPUT_SUFFIX);
				goto end;
			}
			if (!_update_symlink(priv, oinfo, error))
				goto end;
		}
		if (oinfo)
			g_object_unref(oinfo);

		if (*error == NULL) {
			if (force && strerr->len > 0) {
				g_set_error(error, IMSETTINGS_GERROR, IMSETTINGS_GERROR_FAILED,
					    "Errors detected, but forcibly stopped:\n  * %s",
					    strerr->str);
			} else {
				retval = TRUE;
			}
			imsettings_manager_load_conf(IMSETTINGS_MANAGER (imsettings));
			conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
			req = imsettings_request_new(conn, IMSETTINGS_INFO_INTERFACE_DBUS);
			imsettings_request_reload(req, FALSE);
			g_object_unref(req);
		}
	}
  end:
	if (pidfile)
		g_free(pidfile);
	g_string_free(strerr, TRUE);

	return retval;
}

static void
imsettings_manager_class_init(IMSettingsManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	IMSettingsObserverClass *observer_class = IMSETTINGS_OBSERVER_CLASS (klass);

	g_type_class_add_private(klass, sizeof (IMSettingsManagerPrivate));

	object_class->set_property = imsettings_manager_real_set_property;
	object_class->get_property = imsettings_manager_real_get_property;
	object_class->finalize     = imsettings_manager_real_finalize;

	observer_class->get_list = imsettings_manager_real_get_list;
	observer_class->get_info = imsettings_manager_real_get_info;
	observer_class->start_im = imsettings_manager_real_start_im;
	observer_class->stop_im  = imsettings_manager_real_stop_im;

	/* properties */
	g_object_class_install_property(object_class, PROP_DISPLAY_NAME,
					g_param_spec_string("display_name",
							    _("X display name"),
							    _("X display name to use"),
							    NULL,
							    G_PARAM_READWRITE));
}

static void
imsettings_manager_init(IMSettingsManager *manager)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (manager);
	DBusConnection *connection;

	connection = dbus_bus_get(DBUS_BUS_SESSION, NULL);

	priv->im_info_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

	priv->gtk_req = imsettings_request_new(connection, IMSETTINGS_GCONF_INTERFACE_DBUS);
//	priv->xim_req = imsettings_request_new(connection, IMSETTINGS_XIM_INTERFACE_DBUS);
//	priv->qt_req = imsettings_request_new(connection, IMSETTINGS_QT_INTERFACE_DBUS);

	imsettings_manager_load_conf(manager);
}

/*
 * Public functions
 */
IMSettingsManager *
imsettings_manager_new(DBusGConnection *connection,
		       gboolean         replace)
{
	g_return_val_if_fail (connection != NULL, NULL);

	return (IMSettingsManager *)g_object_new(IMSETTINGS_TYPE_MANAGER,
						 "replace", replace,
						 "connection", connection,
						 NULL);
}

void
imsettings_manager_load_conf(IMSettingsManager *manager)
{
	IMSettingsManagerPrivate *priv = IMSETTINGS_MANAGER_GET_PRIVATE (manager);
	IMSettingsInfo *info, *p;
	GPtrArray *list;
	gint i;
	const gchar *name, *homedir;
	gchar *filename;

	if (priv->im_info_table) {
		g_hash_table_remove_all(priv->im_info_table);
	}

	_imsettings_info_init();
	list = _imsettings_info_get_filename_list();
	if (list) {
		for (i = 0; i < list->len; i++) {
			info = imsettings_info_new(g_ptr_array_index(list, i));
			name = imsettings_info_get_short_desc(info);
			if (g_hash_table_lookup(priv->im_info_table, name) == NULL) {
				g_hash_table_insert(priv->im_info_table, g_strdup(name), info);
			} else {
				g_warning(_("Duplicate entry `%s' from %s. SHORT_DESC has to be unique."),
					  name, (gchar *)g_ptr_array_index(list, i));
			}
		}
		/* determine the system default and the user choice */
		filename = g_build_filename(XINPUTRC_PATH, IMSETTINGS_GLOBAL_XINPUT_CONF, NULL);
		info = imsettings_info_new(filename);
		g_free(filename);
		g_object_set(G_OBJECT (info), "is_system_default", TRUE, NULL);
		name = imsettings_info_get_short_desc(info);
		if (name == NULL)
			g_assert_not_reached();

		if ((p = g_hash_table_lookup(priv->im_info_table, name)) == NULL) {
			g_warning(_("No system default IM found at the pre-search phase. Adding..."));
			g_hash_table_insert(priv->im_info_table, g_strdup(name), info);
		} else {
			if (!imsettings_info_compare(info, p)) {
				g_warning(_("Looking up the different object with the same key. it may happens due to the un-unique short description. replacing..."));
				g_hash_table_replace(priv->im_info_table, g_strdup(name), info);
			} else {
				/* reuse the object */
				g_object_set(G_OBJECT (p), "is_system_default", TRUE, NULL);
				g_object_unref(info);
			}
		}
	}

	homedir = g_get_home_dir();
	if (homedir == NULL) {
		g_warning(_("Failed to get a place of home directory."));
		return;
	}
	filename = g_build_filename(homedir, IMSETTINGS_USER_XINPUT_CONF, NULL);
	info = imsettings_info_new(filename);
	g_free(filename);
	if (imsettings_info_is_visible(info)) {
		g_object_set(G_OBJECT (info), "is_user_default", TRUE, NULL);
		name = imsettings_info_get_short_desc(info);
		if (name == NULL || (p = g_hash_table_lookup(priv->im_info_table, name)) == NULL) {
			g_warning(_("No user default IM found at the pre-search phase. Adding..."));
			g_hash_table_insert(priv->im_info_table, g_strdup(name), info);
		} else {
			if (!imsettings_info_compare(info, p)) {
				g_warning(_("Looking up the different object with the same key. it may happens due to the un-unique short description. replacing..."));
				g_hash_table_replace(priv->im_info_table, g_strdup(name), info);
			} else {
				/* reuse the object */
				g_object_set(G_OBJECT (p), "is_user_default", TRUE, NULL);
				g_object_unref(info);
			}
		}
	} else {
		g_object_unref(info);
	}
	/* to be able to revert the user specific .xinputrc file */
	filename = g_build_filename(homedir, IMSETTINGS_USER_XINPUT_CONF ".bak", NULL);
	info = imsettings_info_new(filename);
	g_free(filename);
	if (info) {
		g_object_set(G_OBJECT (info),
			     "ignore", TRUE,
			     NULL);
		name = imsettings_info_get_short_desc(info);
		if (name == NULL || (p = g_hash_table_lookup(priv->im_info_table, name)) == NULL) {
			g_warning(_("No user default IM found at the pre-search phase. Adding..."));
			g_hash_table_insert(priv->im_info_table, g_strdup(name), info);
		} else {
			if (!imsettings_info_compare(info, p)) {
				g_warning(_("Looking up the different object with the same key. it may happens due to the un-unique short description. replacing..."));
				g_hash_table_replace(priv->im_info_table, g_strdup(name), info);
			} else {
				/* reuse the object */
				g_object_set(G_OBJECT (p), "is_user_default", TRUE, NULL);
				g_object_unref(info);
			}
		}
	}
}
