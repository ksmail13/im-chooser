/* 
 * xinput.c
 * Copyright (C) 2006 Red Hat, Inc. All rights reserved.
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
#include "xinput.h"


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
	gchar    *gtkimm;
	gchar    *qtimm;
	gchar    *xim;
	gboolean  ignore;
};

const gchar *_xinput_tokens[] = {
	"GTK_IM_MODULE=",
	"QT_IM_MODULE=",
	"XIM=",
	"IM_CHOOSER_IGNORE_ME=",
	NULL
};
typedef enum {
	GTK_IM_MODULE = 1,
	QT_IM_MODULE,
	XIM,
	IM_CHOOSER_IGNORE_ME,
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
	gchar buffer[256], *p;
	gint i;
	GString *str, *cmd;
	token_type_t type;
	struct stat st;

	g_return_val_if_fail (file != NULL, NULL);

	str = g_string_new(NULL);
	cmd = g_string_new(NULL);
	g_string_printf(cmd, "export IM_CHOOSER_DISABLE_USER_XINPUTRC=yes; export IM_CHOOSER_ONLY_EVALUATE_VARIABLES=yes;. %s;", file);
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

	retval = g_new0(XInputData, 1);
	retval->gtkimm = NULL;
	retval->qtimm = NULL;
	retval->xim = NULL;
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
				    default:
					    break;
				}
				g_string_erase(str, 0, -1);
				_skip_blanks(p);
			}
		}
	}
	pclose(fp);

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
	if (xinput->gtkimm)
		g_free(xinput->gtkimm);
	if (xinput->qtimm)
		g_free(xinput->qtimm);
	if (xinput->xim)
		g_free(xinput->xim);
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
	    default:
		    g_warning("Unknown name type: %d", type);
		    break;
	}

	return retval;
}
