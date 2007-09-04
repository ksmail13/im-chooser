/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * xinput.c
 * Copyright (C) 2006-2007 Red Hat, Inc. All rights reserved.
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "xinput.h"
#include "im-chooser-private.h"


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


struct _XInputData {
	gchar    *file;
	gchar    *gtkimm;
	gchar    *qtimm;
	gchar    *xim;
	gchar    *prefs_prog;
	gchar    *prefs_args;
	gchar    *short_desc;
	gchar    *long_desc;
	gboolean  ignore;
};

const gchar *_xinput_tokens[] = {
	"GTK_IM_MODULE=",
	"QT_IM_MODULE=",
	"XIM=",
	"IM_CHOOSER_IGNORE_ME=",
	"PREFERENCE_PROGRAM=",
	"PREFERENCE_ARGS=",
	"SHORT_DESC=",
	"LONG_DESC=",
	NULL
};
typedef enum {
	GTK_IM_MODULE = 1,
	QT_IM_MODULE,
	XIM,
	IM_CHOOSER_IGNORE_ME,
	PREFERENCE_PROGRAM,
	PREFERENCE_ARGS,
	SHORT_DESC,
	LONG_DESC,
} token_type_t;

/*
 * Private Functions
 */

/*
 * Public Functions
 */
GSList *
xinput_get_im_list(const gchar *path)
{
	GSList *retval = NULL;
	GDir *dir;
	const gchar *name;
	gchar *filename;
	struct stat st;
	size_t len, suffixlen = strlen(XINPUT_SUFFIX);

	if (path == NULL) {
		path = XINPUT_PATH;
	}
	if ((dir = g_dir_open(path, 0, NULL)) == NULL)
		return NULL;
	while ((name = g_dir_read_name(dir)) != NULL) {
		len = strlen(name);
		if (len < suffixlen ||
		    strcmp(&name[len - suffixlen], XINPUT_SUFFIX) != 0)
			continue;
		filename = g_build_filename(path, name, NULL);
		if (lstat(filename, &st) == 0) {
			if (S_ISREG (st.st_mode) && !S_ISLNK (st.st_mode)) {
				retval = g_slist_append(retval, g_strdup(filename));
			}
		}
		g_free(filename);
	}
	g_dir_close(dir);

	return retval;
}

GHashTable *
xinput_get_lang_table(const gchar *path)
{
	GHashTable *retval = NULL;
	GDir *dir;
	const gchar *name;
	gchar *filename, *linkname;
	struct stat st;

	if (path == NULL) {
		path = XINPUT_PATH;
	}
	retval = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	if ((dir = g_dir_open(path, 0, NULL)) == NULL)
		return retval;
	while ((name = g_dir_read_name(dir)) != NULL) {
		filename = g_build_filename(path, name, NULL);
		if (lstat(filename, &st) == 0) {
			if (S_ISLNK (st.st_mode)) {
				linkname = g_file_read_link(filename, NULL);
				if (linkname) {
					g_hash_table_insert(retval, g_strdup(name), linkname);
				}
			}
		}
		g_free(filename);
	}
	g_dir_close(dir);

	return retval;
}

XInputData *
xinput_data_new(const gchar *file)
{
	XInputData *retval;
	FILE *fp;
	gchar buffer[256], *p, *name;
	gint i;
	GString *str, *cmd;
	token_type_t type;
	struct stat st;

	if (file == NULL) {
		/* special xinput data for user-own xinput file */
		retval = g_new0(XInputData, 1);
		retval->file = NULL;
		retval->gtkimm = NULL;
		retval->qtimm = NULL;
		retval->xim = NULL;
		retval->prefs_prog = NULL;
		retval->prefs_args = NULL;
		retval->short_desc = g_strdup(IM_USER_SPECIFIC_LABEL);
		retval->long_desc = g_strdup(_("xinputrc was modified by the user"));
		retval->ignore = FALSE;

		return retval;
	}
	str = g_string_new(NULL);
	cmd = g_string_new(NULL);
	name = g_path_get_basename(file);
	g_string_append(cmd, "export IM_CHOOSER_DISABLE_USER_XINPUTRC=yes; export IM_CHOOSER_ONLY_EVALUATE_VARIABLES=yes;");
	if (strcmp(name, IM_GLOBAL_XINPUT_CONF) == 0) {
		g_string_append_printf(cmd, ". %s;", XINIT_PATH G_DIR_SEPARATOR_S IM_XINPUT_SH);
	} else {
		g_string_append_printf(cmd, ". %s;", file);
	}
	for (i = 0; _xinput_tokens[i] != NULL; i++) {
		size_t len = strlen(_xinput_tokens[i]);

		p = g_strndup(_xinput_tokens[i], len - 1);
		g_string_append_printf(cmd, " echo \"%s$%s\";", _xinput_tokens[i], p);
		g_free(p);
	}
	if (lstat(file, &st) == -1 ||
	    (fp = popen(cmd->str, "r")) == NULL) {
		return NULL;
	}

	retval = g_new(XInputData, 1);
	retval->file = g_strdup(file);
	retval->gtkimm = NULL;
	retval->qtimm = NULL;
	retval->xim = NULL;
	retval->prefs_prog = NULL;
	retval->prefs_args = NULL;
	retval->short_desc = NULL;
	retval->long_desc = NULL;
	retval->ignore = FALSE;

	while (!feof(fp)) {
		if ((p = fgets(buffer, 255, fp)) != NULL) {
			_skip_blanks(p);
			while (*p != 0) {
				type = 0;
				for (i = 0; _xinput_tokens[i] != NULL; i++) {
					size_t len = strlen(_xinput_tokens[i]);
					if (strncmp(p, _xinput_tokens[i], len) == 0) {
						type = i + 1;
						p += len;
						for (; *p != 0 && !isspace(*p); p++) {
							g_string_append_c(str, *p);
						}
						break;
					}
				}
				switch (type) {
				    case 0:
					    _skip_tokens(p);
					    break;
				    case GTK_IM_MODULE:
					    retval->gtkimm = g_strdup(str->str);
					    break;
				    case QT_IM_MODULE:
					    retval->qtimm = g_strdup(str->str);
					    break;
				    case XIM:
					    retval->xim = g_strdup(str->str);
					    break;
				    case IM_CHOOSER_IGNORE_ME:
					    if (g_ascii_strcasecmp(str->str, "true") == 0 ||
						g_ascii_strcasecmp(str->str, "yes") == 0 ||
						g_ascii_strcasecmp(str->str, "1") == 0) {
						    retval->ignore = TRUE;
					    }
					    break;
				    case PREFERENCE_PROGRAM:
					    retval->prefs_prog = g_strdup(str->str);
					    break;
				    case PREFERENCE_ARGS:
					    retval->prefs_args = g_strdup(str->str);
					    break;
				    case SHORT_DESC:
					    retval->short_desc = g_strdup(str->str);
					    break;
				    case LONG_DESC:
					    retval->long_desc = g_strdup(str->str);
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

#define _my_free(n)				\
	if ((n) != NULL && (n)[0] == 0) {	\
		g_free(n);			\
		(n) = NULL;			\
	}

	_my_free(retval->gtkimm);
	_my_free(retval->qtimm);
	_my_free(retval->xim);
	_my_free(retval->prefs_prog);
	_my_free(retval->prefs_args);
	_my_free(retval->short_desc);
	_my_free(retval->long_desc);

#undef _my_free

	g_string_free(cmd, TRUE);
	g_string_free(str, TRUE);

	return retval;
}

void
xinput_data_free(gpointer data)
{
	XInputData *xinput;

	g_return_if_fail (data != NULL);

	xinput = data;
	if (xinput->file)
		g_free(xinput->file);
	if (xinput->gtkimm)
		g_free(xinput->gtkimm);
	if (xinput->qtimm)
		g_free(xinput->qtimm);
	if (xinput->xim)
		g_free(xinput->xim);
	if (xinput->prefs_prog)
		g_free(xinput->prefs_prog);
	if (xinput->prefs_args)
		g_free(xinput->prefs_args);
	if (xinput->short_desc)
		g_free(xinput->short_desc);
	if (xinput->long_desc)
		g_free(xinput->long_desc);
	g_free(xinput);
}

gpointer
xinput_data_get_value(XInputData      *xinput,
		      XInputValueType  type)
{
	gpointer retval = NULL;

	g_return_val_if_fail (xinput != NULL, NULL);

	switch (type) {
	    case XINPUT_VALUE_XIM:
		    retval = xinput->xim;
		    break;
	    case XINPUT_VALUE_GTKIMM:
		    retval = xinput->gtkimm;
		    break;
	    case XINPUT_VALUE_QTIMM:
		    retval = xinput->qtimm;
		    break;
	    case XINPUT_VALUE_IGNORE_ME:
		    retval = GINT_TO_POINTER (xinput->ignore);
		    break;
	    case XINPUT_VALUE_PREFS_PROG:
		    retval = xinput->prefs_prog;
		    break;
	    case XINPUT_VALUE_PREFS_ARGS:
		    retval = xinput->prefs_args;
		    break;
	    case XINPUT_VALUE_SHORT_DESC:
		    retval = xinput->short_desc;
		    break;
	    case XINPUT_VALUE_LONG_DESC:
		    retval = xinput->long_desc;
		    break;
	    case XINPUT_VALUE_FILENAME:
		    retval = xinput->file;
		    break;
	    default:
		    g_warning("Unknown name type: %d", type);
		    break;
	}

	return retval;
}

gchar *
xinput_data_get_short_description(XInputData *xinput)
{
	gchar *retval;

	g_return_val_if_fail (xinput != NULL, NULL);

	if ((retval = xinput_data_get_value(xinput, XINPUT_VALUE_SHORT_DESC)) == NULL)
		retval = xinput_data_get_value(xinput, XINPUT_VALUE_XIM);

	return retval;
}
