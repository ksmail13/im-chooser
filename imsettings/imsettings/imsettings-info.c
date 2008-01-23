/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imsettings-info.c
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <glib/gi18n-lib.h>
#include "imsettings-info.h"
#include "imsettings-info-private.h"

#define IMSETTINGS_INFO_GET_PRIVATE(_o_)	(G_TYPE_INSTANCE_GET_PRIVATE ((_o_), IMSETTINGS_TYPE_INFO, IMSettingsInfoPrivate))
#define _skip_blanks(_str)				\
	while (1) {					\
		if (*(_str) == 0 || !isspace(*(_str)))	\
			break;				\
		(_str)++;				\
	}
#define _skip_tokens(_str)				\
	while (1) {					\
		if (*(_str) == 0 || isspace(*(_str)))	\
			break;				\
		(_str)++;				\
	}


typedef struct _IMSettingsInfoPrivate {
	gchar    *filename;
	gchar    *gtkimm;
	gchar    *qtimm;
	gchar    *xim;
	gchar    *xim_prog;
	gchar    *xim_args;
	gchar    *prefs_prog;
	gchar    *prefs_args;
	gchar    *aux_prog;
	gchar    *aux_args;
	gchar    *short_desc;
	gchar    *long_desc;
	gboolean  ignore;
	gboolean  is_system_default;
	gboolean  is_user_default;
} IMSettingsInfoPrivate;

enum {
	PROP_0 = 0,
	PROP_FILENAME,
	PROP_GTK_IMM,
	PROP_QT_IMM,
	PROP_XIM,
	PROP_IGNORE_FLAG,
	PROP_XIM_PROG,
	PROP_XIM_PROG_ARGS,
	PROP_PREFS_PROG,
	PROP_PREFS_PROG_ARGS,
	PROP_AUX_PROG,
	PROP_AUX_PROG_ARGS,
	PROP_SHORT_DESC,
	PROP_LONG_DESC,
	PROP_IS_SYSTEM_DEFAULT,
	PROP_IS_USER_DEFAULT,
};


static GPtrArray *_imsettings_filename_list = NULL;


G_DEFINE_TYPE (IMSettingsInfo, imsettings_info, G_TYPE_OBJECT);

/*
 * Private functions
 */
static void
imsettings_info_notify_properties(GObject     *object,
				  const gchar *filename)
{
	GString *cmd, *str;
	gchar *basename, *p, buffer[256];
	static const gchar *_xinput_tokens[] = {
		"GTK_IM_MODULE=",
		"QT_IM_MODULE=",
		"XIM=",
		"IM_CHOOSER_IGNORE_ME=",
		"XIM_PROGRAM=",
		"XIM_ARGS=",
		"PREFERENCE_PROGRAM=",
		"PREFERENCE_ARGS=",
		"AUXILIARY_PROGRAM=",
		"AUXILIARY_ARGS=",
		"SHORT_DESC=",
		"LONG_DESC=",
		NULL
	};
	static const gchar *properties[] = {
		"gtkimm",
		"qtimm",
		"xim",
		"ignore",
		"xim_prog",
		"xim_args",
		"prefs_prog",
		"prefs_args",
		"aux_prog",
		"aux_args",
		"short_desc",
		"long_desc",
		NULL
	};
	gint i;
	FILE *fp;
	guint prop;
	struct stat st;

	cmd = g_string_new(NULL);
	str = g_string_new(NULL);
	basename = g_path_get_basename(filename);
	g_string_append(cmd, "export IMSETTINGS_DISABLE_USER_XINPUTRC=yes; export IMSETTINGS_ONLY_EVALUATE_VARIABLES=yes;");
	g_string_append_printf(cmd, ". %s;",
			       (strcmp(basename, IMSETTINGS_GLOBAL_XINPUT_CONF) == 0 ?
				XINIT_PATH G_DIR_SEPARATOR_S IMSETTINGS_XINPUT_SH :
				filename));
	for (i = 0; _xinput_tokens[i] != NULL; i++) {
		size_t len = strlen(_xinput_tokens[i]);

		p = g_strndup(_xinput_tokens[i], len - 1);
		g_string_append_printf(cmd, "echo \"%s$%s\";", _xinput_tokens[i], p);
		g_free(p);
	}
	if (lstat(filename, &st) == -1 ||
	    (fp = popen(cmd->str, "r")) == NULL) {
		/* error happens. don't list. */
		g_object_set(object, "ignore", TRUE, NULL);
	} else {
		while (!feof(fp)) {
			if ((p = fgets(buffer, 255, fp)) != NULL) {
				_skip_blanks(p);
				while (*p != 0) {
					prop = 0;
					for (i = 0; _xinput_tokens[i] != NULL; i++) {
						size_t len = strlen(_xinput_tokens[i]);

						if (strncmp(p, _xinput_tokens[i], len) == 0) {
							prop = i + 2;
							p += len;
							for (; *p != 0 && !isspace(*p); p++)
								g_string_append_c(str, *p);
							break;
						}
					}
					switch (prop) {
					    case PROP_0:
					    case PROP_FILENAME:
						    _skip_tokens(p);
						    break;
					    case PROP_GTK_IMM:
					    case PROP_QT_IMM:
					    case PROP_XIM:
					    case PROP_XIM_PROG:
					    case PROP_XIM_PROG_ARGS:
					    case PROP_PREFS_PROG:
					    case PROP_PREFS_PROG_ARGS:
					    case PROP_AUX_PROG:
					    case PROP_AUX_PROG_ARGS:
					    case PROP_SHORT_DESC:
					    case PROP_LONG_DESC:
						    g_object_set(object,
								 properties[prop - 2], str->str,
								 NULL);
						    break;
					    case PROP_IGNORE_FLAG:
						    g_object_set(object,
								 "ignore",
								 (g_ascii_strcasecmp(str->str, "true") == 0 ||
								  g_ascii_strcasecmp(str->str, "yes") == 0 ||
								  g_ascii_strcasecmp(str->str, "1") == 0),
								 NULL);
						    break;
					    default:
						    break;
					}
					g_string_erase(str, 0, -1);
					_skip_blanks(p);
				}
			}
		}
		pclose(fp);
	}
	g_string_free(cmd, TRUE);
	g_string_free(str, TRUE);
}

static void
imsettings_info_set_property(GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	IMSettingsInfoPrivate *priv = IMSETTINGS_INFO_GET_PRIVATE (object);

#define _set_str_prop(_m_)						\
	G_STMT_START {							\
		if (priv->_m_)						\
			g_free(priv->_m_);				\
		priv->_m_ = g_strdup(g_value_get_string(value));	\
		g_object_notify(object, # _m_);				\
	} G_STMT_END
#define _set_bool_prop(_m_)				\
	G_STMT_START {					\
		priv->_m_ = g_value_get_boolean(value);	\
		g_object_notify(object, # _m_);		\
	} G_STMT_END

	switch (prop_id) {
	    case PROP_FILENAME:
		    _set_str_prop(filename);
		    if (priv->filename == NULL) {
			    /* special case to deal with the user-own conf file */
			    g_object_set(object,
					 "short_desc", IMSETTINGS_USER_SPECIFIC_SHORT_DESC,
					 "long_desc", IMSETTINGS_USER_SPECIFIC_LONG_DESC,
					 NULL);
		    } else {
			    imsettings_info_notify_properties(object, priv->filename);
		    }
		    break;
	    case PROP_GTK_IMM:
		    _set_str_prop(gtkimm);
		    break;
	    case PROP_QT_IMM:
		    _set_str_prop(qtimm);
		    break;
	    case PROP_XIM:
		    _set_str_prop(xim);
		    break;
	    case PROP_IGNORE_FLAG:
		    _set_bool_prop(ignore);
		    break;
	    case PROP_XIM_PROG:
		    _set_str_prop(xim_prog);
		    break;
	    case PROP_XIM_PROG_ARGS:
		    _set_str_prop(xim_args);
		    break;
	    case PROP_PREFS_PROG:
		    _set_str_prop(prefs_prog);
		    break;
	    case PROP_PREFS_PROG_ARGS:
		    _set_str_prop(prefs_args);
		    break;
	    case PROP_AUX_PROG:
		    _set_str_prop(aux_prog);
		    break;
	    case PROP_AUX_PROG_ARGS:
		    _set_str_prop(aux_args);
		    break;
	    case PROP_SHORT_DESC:
		    _set_str_prop(short_desc);
		    break;
	    case PROP_LONG_DESC:
		    _set_str_prop(long_desc);
		    break;
	    case PROP_IS_SYSTEM_DEFAULT:
		    _set_bool_prop(is_system_default);
		    break;
	    case PROP_IS_USER_DEFAULT:
		    _set_bool_prop(is_user_default);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}

#undef _set_bool_prop
#undef _set_str_prop
}

static void
imsettings_info_get_property(GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	IMSettingsInfoPrivate *priv = IMSETTINGS_INFO_GET_PRIVATE (object);

#define _get_str_prop(_m_)						\
	g_value_set_string(value, imsettings_info_get_ ## _m_(IMSETTINGS_INFO (object)))
#define _get_bool_prop(_m_)			\
	g_value_set_boolean(value, priv->_m_)

	switch (prop_id) {
	    case PROP_FILENAME:
		    _get_str_prop(filename);
		    break;
	    case PROP_GTK_IMM:
		    _get_str_prop(gtkimm);
		    break;
	    case PROP_QT_IMM:
		    _get_str_prop(qtimm);
		    break;
	    case PROP_XIM:
		    _get_str_prop(xim);
		    break;
	    case PROP_IGNORE_FLAG:
		    _get_bool_prop(ignore);
		    break;
	    case PROP_XIM_PROG:
		    _get_str_prop(xim_program);
		    break;
	    case PROP_XIM_PROG_ARGS:
		    _get_str_prop(xim_args);
		    break;
	    case PROP_PREFS_PROG:
		    _get_str_prop(prefs_program);
		    break;
	    case PROP_PREFS_PROG_ARGS:
		    _get_str_prop(prefs_args);
		    break;
	    case PROP_AUX_PROG:
		    _get_str_prop(aux_program);
		    break;
	    case PROP_AUX_PROG_ARGS:
		    _get_str_prop(aux_args);
		    break;
	    case PROP_SHORT_DESC:
		    _get_str_prop(short_desc);
		    break;
	    case PROP_LONG_DESC:
		    _get_str_prop(long_desc);
		    break;
	    case PROP_IS_SYSTEM_DEFAULT:
		    _get_bool_prop(is_system_default);
		    break;
	    case PROP_IS_USER_DEFAULT:
		    _get_bool_prop(is_user_default);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
imsettings_info_finalize(GObject *object)
{
	IMSettingsInfoPrivate *priv = IMSETTINGS_INFO_GET_PRIVATE (object);

#define _my_free(_o_)				\
	if (priv->_o_)				\
		g_free(priv->_o_)

	_my_free(gtkimm);
	_my_free(qtimm);
	_my_free(xim);
	_my_free(xim_prog);
	_my_free(xim_args);
	_my_free(prefs_prog);
	_my_free(prefs_args);
	_my_free(aux_prog);
	_my_free(aux_args);
	_my_free(short_desc);
	_my_free(long_desc);

#undef _my_free
}

static void
imsettings_info_class_init(IMSettingsInfoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private(klass, sizeof (IMSettingsInfoPrivate));

	object_class->set_property = imsettings_info_set_property;
	object_class->get_property = imsettings_info_get_property;
	object_class->finalize     = imsettings_info_finalize;

	/* properties */
	g_object_class_install_property(object_class, PROP_FILENAME,
					g_param_spec_string("filename",
							    _("Filename"),
							    _("A filename referring to the IM information."),
							    NULL,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_GTK_IMM,
					g_param_spec_string("gtkimm",
							    _("GTK+ immodule"),
							    _("A module name used for GTK+"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_QT_IMM,
					g_param_spec_string("qtimm",
							    _("Qt immodule"),
							    _("A module name used for Qt"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_XIM,
					g_param_spec_string("xim",
							    _("XIM name"),
							    _("XIM server name"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_IGNORE_FLAG,
					g_param_spec_boolean("ignore",
							     _("Ignore flag"),
							     _("A flag to not list up."),
							     FALSE,
							     G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_XIM_PROG,
					g_param_spec_string("xim_prog",
							    _("XIM server program name"),
							    _("XIM server program to be run"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_XIM_PROG_ARGS,
					g_param_spec_string("xim_args",
							    _("XIM server program's options"),
							    _("Command line options for XIM server program"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_PREFS_PROG,
					g_param_spec_string("prefs_prog",
							    _("Preference program name"),
							    _("Preference program to be run"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_PREFS_PROG_ARGS,
					g_param_spec_string("prefs_args",
							    _("Preference program's options"),
							    _("Command line options for preference program"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_AUX_PROG,
					g_param_spec_string("aux_prog",
							    _("Auxiliary program name"),
							    _("Auxiliary program to be run"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_AUX_PROG_ARGS,
					g_param_spec_string("aux_args",
							    _("Auxiliary program's options"),
							    _("Command line options for auxiliary program"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_SHORT_DESC,
					g_param_spec_string("short_desc",
							    _("Short Description"),
							    _("Short Description"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_LONG_DESC,
					g_param_spec_string("long_desc",
							    _("Long Description"),
							    _("Long Description"),
							    NULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_IS_SYSTEM_DEFAULT,
					g_param_spec_boolean("is_system_default",
							     _("System Default"),
							     _("Whether or not IM is a system default."),
							     FALSE,
							     G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_IS_USER_DEFAULT,
					g_param_spec_boolean("is_user_default",
							     _("User Default"),
							     _("Whether or not IM is an user default."),
							     FALSE,
							     G_PARAM_READWRITE));
}

static void
imsettings_info_init(IMSettingsInfo *info)
{
	IMSettingsInfoPrivate *priv = IMSETTINGS_INFO_GET_PRIVATE (info);

	priv->filename = NULL;
	priv->gtkimm = NULL;
	priv->qtimm = NULL;
	priv->xim = NULL;
	priv->xim_prog = NULL;
	priv->xim_args = NULL;
	priv->prefs_prog = NULL;
	priv->prefs_args = NULL;
	priv->aux_prog = NULL;
	priv->aux_args = NULL;
	priv->short_desc = NULL;
	priv->long_desc = NULL;
}

static GPtrArray *
imsettings_info_update_filename_list(const gchar *_path)
{
	const gchar *path, *name;
	GDir *dir;
	GError *error = NULL;
	gchar *filename;
	struct stat st;
	size_t len, suffixlen = strlen(XINPUT_SUFFIX);
	GPtrArray *retval = NULL;

	if (_path == NULL)
		path = XINPUT_PATH;
	else
		path = _path;

	if ((dir = g_dir_open(path, 0, &error)) == NULL) {
		g_warning("%s\n", error->message);

		return NULL;
	}
	retval = g_ptr_array_new();
	while ((name = g_dir_read_name(dir)) != NULL) {
		len = strlen(name);
		if (len < suffixlen ||
		    strcmp(&name[len - suffixlen], XINPUT_SUFFIX) != 0)
			continue;
		filename = g_build_filename(path, name, NULL);
		if (lstat(filename, &st) == 0) {
			if (S_ISREG (st.st_mode) && !S_ISLNK (st.st_mode))
				g_ptr_array_add(retval, g_strdup(filename));
		}
		g_free(filename);
	}
	g_dir_close(dir);

	if (retval->len == 0) {
		g_ptr_array_free(retval, TRUE);
		retval = NULL;
	}

	return retval;
}

/*
 * Public functions
 */
void
_imsettings_info_init(void)
{
	if (_imsettings_filename_list)
		_imsettings_info_finalize();

	_imsettings_filename_list = imsettings_info_update_filename_list(NULL);
}

void
_imsettings_info_finalize(void)
{
	gint i;

	for (i = 0; i < _imsettings_filename_list->len; i++)
		g_free(g_ptr_array_index(_imsettings_filename_list, i));

	g_ptr_array_free(_imsettings_filename_list, TRUE);
	_imsettings_filename_list = NULL;
}

GPtrArray *
_imsettings_info_get_filename_list(void)
{
	return _imsettings_filename_list;
}

IMSettingsInfo *
imsettings_info_new(const gchar *filename)
{
	return g_object_new(IMSETTINGS_TYPE_INFO,
			    "filename", filename,
			    NULL);
}

#define _IMSETTINGS_DEFUNC_PROPERTY(_t_,_n_,_m_,_v_)			\
	_t_								\
	imsettings_info_get_ ## _n_(IMSettingsInfo *info)		\
	{								\
		IMSettingsInfoPrivate *priv;				\
									\
		g_return_val_if_fail (IMSETTINGS_IS_INFO (info), (_v_)); \
									\
		priv = IMSETTINGS_INFO_GET_PRIVATE (info);		\
									\
		return priv->_m_;					\
	}

_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,filename, filename, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,gtkimm, gtkimm, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,qtimm, qtimm, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,xim, xim, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,xim_program, xim_prog, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,xim_args, xim_args, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,prefs_program, prefs_prog, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,prefs_args, prefs_args, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,aux_program, aux_prog, NULL)
_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,aux_args, aux_args, NULL)

const gchar *
imsettings_info_get_short_desc(IMSettingsInfo *info)
{
	IMSettingsInfoPrivate *priv;

	g_return_val_if_fail (IMSETTINGS_IS_INFO (info), NULL);

	priv = IMSETTINGS_INFO_GET_PRIVATE (info);

	if (priv->short_desc == NULL || priv->short_desc[0] == 0)
		return priv->xim;

	return priv->short_desc;
}

_IMSETTINGS_DEFUNC_PROPERTY (const gchar *,long_desc, long_desc, NULL)

gboolean
imsettings_info_is_visible(IMSettingsInfo *info)
{
	IMSettingsInfoPrivate *priv;

	g_return_val_if_fail (IMSETTINGS_IS_INFO (info), FALSE);

	priv = IMSETTINGS_INFO_GET_PRIVATE (info);

	return !priv->ignore;
}

_IMSETTINGS_DEFUNC_PROPERTY (gboolean,is_system_default, is_system_default, FALSE)
_IMSETTINGS_DEFUNC_PROPERTY (gboolean,is_user_default, is_user_default, FALSE)

gboolean
imsettings_info_compare(const IMSettingsInfo *info1,
			const IMSettingsInfo *info2)
{
	IMSettingsInfoPrivate *priv1, *priv2;

	g_return_val_if_fail (IMSETTINGS_IS_INFO (info1), FALSE);
	g_return_val_if_fail (IMSETTINGS_IS_INFO (info2), FALSE);

	if (info1 == info2)
		return TRUE;

	priv1 = IMSETTINGS_INFO_GET_PRIVATE (info1);
	priv2 = IMSETTINGS_INFO_GET_PRIVATE (info2);

#define _cmp(_o_)							\
	((!priv1->_o_ && !priv2->_o_) ||				\
	 (priv1->_o_ && priv2->_o_ && strcmp(priv1->_o_, priv2->_o_) == 0))

	return (_cmp(gtkimm) &&
		_cmp(qtimm) &&
		_cmp(xim) &&
		_cmp(xim_prog) &&
		_cmp(xim_args) &&
		_cmp(prefs_prog) &&
		_cmp(prefs_args) &&
		_cmp(aux_prog) &&
		_cmp(aux_args) &&
		_cmp(short_desc) &&
		_cmp(long_desc) &&
		(priv1->ignore == priv2->ignore));

#undef _cmp
}
