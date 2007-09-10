/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * im-chooser-simple.c
 * Copyright (C) 2007 Red Hat, Inc. All rights reserved.
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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib/gi18n.h>
#include "xinput.h"
#include "im-chooser-simple.h"
#include "im-chooser-private.h"


enum {
	CHANGED,
	NOTIFY_N_IM,
	LAST_SIGNAL
};

struct _IMChooserSimpleClass {
	GObjectClass parent_class;

	void (* changed)     (IMChooserSimple *im);
	void (* notify_n_im) (IMChooserSimple *im,
			      guint            n);
};

struct _IMChooserSimple {
	GObject     parent_instance;
	GtkWidget  *widget;
	GtkWidget  *checkbox_is_im_enabled;
	GtkWidget  *widget_scrolled;
	GtkWidget  *widget_im_list;
	GtkWidget  *button_im_config;
	GHashTable *im_table;
	GHashTable *lang_table;
	XInputData *initial_im;
	XInputData *current_im;
	XInputData *default_im;
	gboolean    initialized;
};

struct _IMChooserSimpleHashData {
	IMChooserSimple *im;
	GSList          *list;
};

struct _IMChooserSimpleListData {
	XInputData *data;
	gchar      *name;
	gboolean    is_recommended;
	gboolean    is_legacy;
	gboolean    is_unknown;
};

typedef struct _IMChooserSimpleHashData		IMChooserSimpleHashData;
typedef struct _IMChooserSimpleListData		IMChooserSimpleListData;

static XInputData *_im_chooser_simple_get_xinput           (IMChooserSimple *im,
                                                            const gchar     *filename);
static XInputData *_im_chooser_simple_get_system_default_im(IMChooserSimple *im);
static XInputData *_im_chooser_simple_get_current_im       (IMChooserSimple *im);
static void        _im_chooser_simple_set_im_to_list       (gpointer         key,
                                                            gpointer         value,
                                                            gpointer         data);
static void        _im_chooser_simple_create_symlink       (IMChooserSimple *im);


static GObjectClass *parent_class = NULL;
static guint         signals[LAST_SIGNAL] = { 0 };


/*
 * signal callback functions
 */
static void
im_chooser_simple_enable_im_on_toggled(GtkToggleButton *button,
					gpointer         user_data)
{
	gboolean flag;
	IMChooserSimple *im;
	gchar *prog;

	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
	g_return_if_fail (IM_IS_CHOOSER_SIMPLE (user_data));

	im = IM_CHOOSER_SIMPLE (user_data);

	flag = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive(im->widget_scrolled, flag);
	if (im->current_im &&
	    (prog = xinput_data_get_value(im->current_im, XINPUT_VALUE_PREFS_PROG)) != NULL &&
	    g_file_test(prog, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_EXECUTABLE))
		gtk_widget_set_sensitive(im->button_im_config, flag);
	else
		gtk_widget_set_sensitive(im->button_im_config, FALSE);

	if (flag) {
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		XInputData *xinput;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (im->widget_im_list));
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(model, &iter, 1, &xinput, -1);
			im->current_im = xinput;
		}
	} else {
		im->current_im = NULL;
	}
	if (im->initialized)
		_im_chooser_simple_create_symlink(im);
}

static void
im_chooser_simple_im_list_on_changed(GtkTreeSelection *selection,
				     gpointer          user_data)
{
	IMChooserSimple *im = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	XInputData *xinput;
	gchar *prog;

	if (im->initialized == FALSE)
		return;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 1, &xinput, -1);
		if (im->current_im != xinput) {
			im->current_im = xinput;
			if (im->current_im &&
			    (prog = xinput_data_get_value(im->current_im, XINPUT_VALUE_PREFS_PROG)) != NULL &&
			    g_file_test(prog, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_EXECUTABLE))
				gtk_widget_set_sensitive(im->button_im_config, TRUE);
			else
				gtk_widget_set_sensitive(im->button_im_config, FALSE);
			_im_chooser_simple_create_symlink(im);
		}
	}
}

static void
im_chooser_simple_prefs_button_on_clicked(GtkButton *button,
					  gpointer   user_data)
{
	IMChooserSimple *im = IM_CHOOSER_SIMPLE (user_data);
	gchar *prog = xinput_data_get_value(im->current_im, XINPUT_VALUE_PREFS_PROG);
	gchar *args = xinput_data_get_value(im->current_im, XINPUT_VALUE_PREFS_ARGS);
	gchar *cmdline = g_strconcat(prog, args, NULL);

	g_spawn_command_line_async(cmdline, NULL);

	g_free(cmdline);
}

/*
 * private functions
 */
static void
im_chooser_simple_class_init(IMChooserSimpleClass *klass)
{
	parent_class = g_type_class_peek_parent(klass);

	/* signals */
	signals[CHANGED] = g_signal_new("changed",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (IMChooserSimpleClass, changed),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
	signals[NOTIFY_N_IM] = g_signal_new("notify_n_im",
					    G_OBJECT_CLASS_TYPE (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (IMChooserSimpleClass, notify_n_im),
					    NULL, NULL,
					    g_cclosure_marshal_VOID__UINT,
					    G_TYPE_NONE, 1,
					    G_TYPE_UINT);
}

static void
im_chooser_simple_instance_init(IMChooserSimple *im)
{
	GSList *l, *im_list;
	XInputData *xinput;

	im->widget = NULL;
	im->initialized = FALSE;

	/* read system-wide info */
	im->im_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, xinput_data_free);
	im->lang_table = xinput_get_lang_table(NULL);
	/* get all the info of the xinput script */
	im_list = xinput_get_im_list(NULL);
	for (l = im_list; l != NULL; l = g_slist_next(l)) {
		xinput = xinput_data_new(l->data);
		if (xinput != NULL) {
			gchar *name = g_path_get_basename(l->data), *key;
			size_t suffixlen = strlen(XINPUT_SUFFIX), len;

			if (name != NULL &&
			    GPOINTER_TO_UINT (xinput_data_get_value(xinput, XINPUT_VALUE_IGNORE_ME)) != TRUE) {
				len = strlen(name);
				if (len > suffixlen) {
					key = xinput_data_get_short_description(xinput);
					if (key == NULL)
						g_warning("Failed to get a short description for `%s'", (gchar *)l->data);
					else
						g_hash_table_replace(im->im_table, key, xinput);
				}
			}
		}
	}

	/* get current im */
	im->current_im = _im_chooser_simple_get_current_im(im);
	im->default_im = _im_chooser_simple_get_system_default_im(im);
	im->initial_im = im->current_im;
}

static XInputData *
_im_chooser_simple_get_xinput(IMChooserSimple *im,
			      const gchar     *filename)
{
	struct stat st;
	gchar *origname = NULL, *name = NULL;
	gchar *key;
	XInputData *retval = NULL;

	if (lstat(filename, &st) == 0) {
		if (S_ISLNK (st.st_mode)) {
			origname = g_file_read_link(filename, NULL);
			if (origname && stat(origname, &st) == 0) {
				name = g_path_get_basename(origname);
				if (strcmp(name, IM_GLOBAL_XINPUT_CONF) == 0) {
					/* obsolete state: need to deal with the global conf */
					XInputData *xinput = xinput_data_new(origname);
					const gchar *key;

					if (xinput == NULL) {
						g_warning("Probably dangling xinputrc symlink");
						goto end;
					}
					key = xinput_data_get_short_description(xinput);
					if (key == NULL) {
						g_warning("Failed to get a short description for `%s'", origname);
						goto end;
					}
					if (strcmp(key, IM_NONE_NAME) == 0)
						goto end;
					if ((retval = g_hash_table_lookup(im->im_table, key)) == NULL) {
						g_warning("System Default xinputrc points to the unknown xinput script.");
						goto end;
					}
					xinput_data_free(xinput);
				} else {
					XInputData *xinput = xinput_data_new(origname);

					if (xinput == NULL) {
						g_warning("Probably dangling xinputrc symlink");
						goto end;
					}
					key = xinput_data_get_short_description(xinput);
					if (key == NULL) {
						g_warning("Failed to get a short description for `%s'", origname);
						goto end;
					}
					if ((retval = g_hash_table_lookup(im->im_table, key)) == NULL) {
						g_warning("%s isn't available", name);
						goto end;
					}
					xinput_data_free(xinput);
				}
			}
		} else {
			/* user-own xinput file */
			retval = xinput_data_new(NULL);
			key = xinput_data_get_short_description(retval);
			g_hash_table_replace(im->im_table, key, retval);
		}
	}
  end:
	if (name)
		g_free(name);
	if (origname)
		g_free(origname);

	return retval;
}

static XInputData *
_im_chooser_simple_get_system_default_im(IMChooserSimple *im)
{
	XInputData *xinput;
	gchar *filename = g_build_filename(XINPUTRC_PATH, IM_GLOBAL_XINPUT_CONF, NULL);
	gchar *name;

	g_return_val_if_fail (filename != NULL, NULL);

	xinput = _im_chooser_simple_get_xinput(im, filename);
	g_free(filename);
	if (xinput) {
		name = xinput_data_get_short_description(xinput);
		if (name && strcmp(name, IM_NONE_NAME) == 0)
			return NULL;
	}

	return xinput;
}

static XInputData *
_im_chooser_simple_get_current_im(IMChooserSimple *im)
{
	XInputData *retval;
	gchar *filename, *name;
	const gchar *home;

	home = g_get_home_dir();
	if (home == NULL) {
		g_error("Failed to get a place of home directory.");
		return NULL;
	}
	filename = g_build_filename(home, IM_USER_XINPUT_CONF, NULL);
	retval = _im_chooser_simple_get_xinput(im, filename);
	if (retval == NULL)
		retval = _im_chooser_simple_get_system_default_im(im);
	if (filename)
		g_free(filename);
	if (retval) {
		name = xinput_data_get_short_description(retval);
		if (name && strcmp(name, IM_NONE_NAME) == 0)
			return NULL;
	}

	return retval;
}

static gint
_im_chooser_simple_sort_data(gconstpointer a,
			     gconstpointer b)
{
	const IMChooserSimpleListData *la = a, *lb = b;

	if (la->is_recommended && !lb->is_recommended) {
		return -1;
		/* appearing multiple recommended IMs isn't likely */
	} else if (!la->is_recommended && lb->is_recommended) {
		return 1;
		/* no need to check if both isn't recommended IMs */
	} else if (!la->is_legacy && lb->is_legacy) {
		return -1;
	} else if ((!la->is_legacy && !lb->is_legacy) ||
		   (la->is_legacy && lb->is_legacy)) {
		/* check the IM name to be sorted out */
		return strcmp(la->name, lb->name);
	} else if (la->is_legacy && !lb->is_legacy) {
		return 1;
	}
	g_warning("[BUG] Unknown state to sort the data.");

	return 0;
}

static void
_im_chooser_simple_set_im_to_list(gpointer key,
				  gpointer value,
				  gpointer data)
{
	IMChooserSimpleHashData *hdata = data;
	gchar *name, *gtkimm, *qtimm;
	XInputData *xinput = value;
	IMChooserSimpleListData *ldata = g_new0(IMChooserSimpleListData, 1);

	ldata->data = xinput;
	name = xinput_data_get_short_description(xinput);
	ldata->name = name;
	gtkimm = xinput_data_get_value(xinput, XINPUT_VALUE_GTKIMM);
	qtimm = xinput_data_get_value(xinput, XINPUT_VALUE_QTIMM);
	if ((gtkimm == NULL || strcmp(gtkimm, "xim") == 0) &&
	    (qtimm == NULL || strcmp(qtimm, "xim") == 0)) {
		ldata->is_legacy = TRUE;
	} else {
		ldata->is_legacy = FALSE;
	}
	if (strcmp(name, IM_USER_SPECIFIC_LABEL) == 0) {
		ldata->is_unknown = TRUE;
	} else {
		ldata->is_unknown = FALSE;
	}
	if (xinput == hdata->im->default_im) {
		ldata->is_recommended = TRUE;
	} else {
		ldata->is_recommended = FALSE;
	}
	hdata->list = g_slist_insert_sorted(hdata->list, ldata, _im_chooser_simple_sort_data);
}

static void
_im_chooser_simple_update_im_list(IMChooserSimple *im)
{
	GtkListStore *list;
	GtkTreeIter iter, *cur_iter = NULL;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	IMChooserSimpleHashData hdata;
	GtkRequisition requisition;
	GSList *l;
	guint count;

	hdata.list = NULL;
	hdata.im = im;
	g_hash_table_foreach(im->im_table, _im_chooser_simple_set_im_to_list, &hdata);

	count = g_slist_length(hdata.list);
	list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	for (l = hdata.list; l != NULL; l = g_slist_next(l)) {
		IMChooserSimpleListData *ldata = l->data;
		GString *string = g_string_new(NULL);

		gtk_list_store_append(list, &iter);
		g_string_append(string, "<i>");
		g_string_append_printf(string, _("Use %s"), ldata->name);
		if (ldata->is_legacy && !ldata->is_unknown)
			g_string_append(string, _(" (Legacy)"));
		if (ldata->is_recommended && !ldata->is_unknown) {
			g_string_append(string, _(" (Recommend)"));
			if (im->current_im == NULL && im->default_im == ldata->data)
				cur_iter = gtk_tree_iter_copy(&iter);
		}
		g_string_append(string, "</i>");
		if (im->current_im && im->current_im == ldata->data)
			cur_iter = gtk_tree_iter_copy(&iter);
		gtk_list_store_set(list, &iter,
				   0, string->str,
				   1, ldata->data,
				   -1);
		g_string_free(string, TRUE);
	}
	if (cur_iter == NULL) {
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL (list), &iter);
		cur_iter = gtk_tree_iter_copy(&iter);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW (im->widget_im_list), GTK_TREE_MODEL (list));
	column = gtk_tree_view_get_column(GTK_TREE_VIEW (im->widget_im_list), 0);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL (list), cur_iter);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW (im->widget_im_list), path, column, FALSE);
	gtk_tree_path_free(path);
	gtk_tree_iter_free(cur_iter);
	g_object_unref(list);

	if (count <= 1) {
		if (count == 0) {
			gtk_widget_set_sensitive(im->checkbox_is_im_enabled, FALSE);
			gtk_widget_hide(im->widget_scrolled);
		}
	} else {
		gtk_widget_show(im->widget_scrolled);
		gtk_widget_set_sensitive(im->checkbox_is_im_enabled, TRUE);
	}
	gtk_widget_size_request(im->widget_im_list, &requisition);
	if (requisition.height > 120)
		requisition.height = 120;
	gtk_widget_set_size_request(im->widget_im_list, -1, requisition.height);

	g_signal_emit(im, signals[NOTIFY_N_IM], 0, count);
}

static void
_im_chooser_simple_create_symlink(IMChooserSimple *im)
{
	struct stat st;
	const gchar *home;
	gchar *file = NULL, *bakfile = NULL, *xinputfile = NULL;

	home = g_get_home_dir();
	if (home == NULL) {
		g_error("Failed to get a place of home directory.");
		return;
	}
	file = g_build_filename(home, IM_USER_XINPUT_CONF, NULL);
	bakfile = g_build_filename(home, IM_USER_XINPUT_CONF ".bak", NULL);
	if (lstat(file, &st) == 0) {
		if (!S_ISLNK (st.st_mode)) {
			/* xinputrc was probably made by the user */
			if (rename(file, bakfile) == -1) {
				g_warning("Failed to create a backup file.");
				g_free(bakfile);
				g_free(file);
				return;
			}
		} else {
			if (unlink(file) == -1) {
				g_warning("Failed to remove a .xinputrc file.");
				g_free(bakfile);
				g_free(file);
				return;
			}
		}
	}
	if (im->current_im == NULL) {
		/* current IM is none */
		xinputfile = g_build_filename(XINPUT_PATH, IM_NONE_NAME XINPUT_SUFFIX, NULL);
	} else {
		gchar *p = xinput_data_get_value(im->current_im, XINPUT_VALUE_FILENAME);

		if (p == NULL) {
			/* try to revert the backup file for the user specific conf file */
			if (rename(bakfile, file) == -1) {
				g_warning("Failed to revert the backup file.");
				g_free(bakfile);
				g_free(file);
				return;
			}
		} else {
			xinputfile = g_strdup(p);
		}
	}
	if (xinputfile) {
		if (symlink(xinputfile, file) == -1)
			g_warning("Failed to create a symlink %s from %s", file, xinputfile);
	}

	if (file)
		g_free(file);
	if (bakfile)
		g_free(bakfile);
	if (xinputfile)
		g_free(xinputfile);

	g_signal_emit(im, signals[CHANGED], 0);
}

/*
 * public functions
 */
GType
im_chooser_simple_get_type(void)
{
	static GType im_type = 0;

	if (!im_type) {
		static const GTypeInfo im_info = {
			.class_size     = sizeof (IMChooserSimpleClass),
			.base_init      = NULL,
			.base_finalize  = NULL,
			.class_init     = (GClassInitFunc)im_chooser_simple_class_init,
			.class_finalize = NULL,
			.class_data     = NULL,
			.instance_size  = sizeof (IMChooserSimple),
			.n_preallocs    = 0,
			.instance_init  = (GInstanceInitFunc)im_chooser_simple_instance_init,
			.value_table    = NULL,
		};

		im_type = g_type_register_static(G_TYPE_OBJECT, "IMChooserSimple",
						 &im_info, 0);
	}

	return im_type;
}

IMChooserSimple *
im_chooser_simple_new(void)
{
	return IM_CHOOSER_SIMPLE (g_object_new(IM_TYPE_CHOOSER_SIMPLE, NULL));
}

GtkWidget *
im_chooser_simple_get_widget(IMChooserSimple *im)
{
	GtkWidget *vbox, *align, *label, *align2, *checkbox;
	GtkWidget *align3, *label3, *align4, *align5, *button;
	GtkWidget *image, *hbox, *list, *scrolled;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	g_return_val_if_fail (IM_IS_CHOOSER_SIMPLE (im), NULL);

	if (im->widget == NULL) {
		/* setup widgets */
		vbox = gtk_vbox_new(FALSE, 2);
		hbox = gtk_hbox_new(FALSE, 1);

		align = gtk_alignment_new(0, 0, 0, 0);
		label = gtk_label_new(_("<b>Input Method</b>"));
		gtk_label_set_use_markup(GTK_LABEL (label), TRUE);
		gtk_container_add(GTK_CONTAINER (align), label);
		gtk_alignment_set_padding(GTK_ALIGNMENT (align), 9, 6, 6, 6);

		align2 = gtk_alignment_new(0, 0, 0, 0);
		im->checkbox_is_im_enabled = checkbox = gtk_check_button_new_with_mnemonic(_("_Enable input method feature"));
		gtk_container_add(GTK_CONTAINER (align2), checkbox);
		gtk_alignment_set_padding(GTK_ALIGNMENT (align2), 3, 6, 6, 6);
		g_signal_connect(checkbox, "toggled",
				 G_CALLBACK (im_chooser_simple_enable_im_on_toggled), im);

		align3 = gtk_alignment_new(0.1, 0, 1.0, 1.0);
		label3 = gtk_label_new(_("<small><i>Note: this change will not take effect until you next log in.</i></small>"));
		gtk_label_set_use_markup(GTK_LABEL (label3), TRUE);
		gtk_label_set_line_wrap(GTK_LABEL (label3), TRUE);
		gtk_misc_set_alignment(GTK_MISC (label3), 0, 0);
		gtk_container_add(GTK_CONTAINER (align3), label3);
		gtk_alignment_set_padding(GTK_ALIGNMENT (align3), 3, 6, 6, 6);

		align4 = gtk_alignment_new(0.1, 0, 1.0, 1.0);
		im->widget_scrolled = scrolled = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled),
					       GTK_POLICY_NEVER,
					       GTK_POLICY_AUTOMATIC);
		im->widget_im_list = list = gtk_tree_view_new();
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW (list), TRUE);
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (list), FALSE);
		gtk_container_add(GTK_CONTAINER (scrolled), list);
		gtk_container_add(GTK_CONTAINER (align4), scrolled);
		gtk_alignment_set_padding(GTK_ALIGNMENT (align4), 0, 6, 18, 6);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("",
								  renderer,
								  "markup", 0,
								  NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW (list), column);

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (list));
		/* doesn't allow to unselect an item */
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
		g_signal_connect(selection, "changed",
				 G_CALLBACK (im_chooser_simple_im_list_on_changed), im);

		align5 = gtk_alignment_new(0.1, 0, 1.0, 1.0);
		im->button_im_config = button = gtk_button_new_with_mnemonic(_("Input Method _Preferences..."));
		image = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON (button), image);
		gtk_container_add(GTK_CONTAINER (align5), button);
		gtk_alignment_set_padding(GTK_ALIGNMENT (align5), 0, 6, 18, 6);
		gtk_box_pack_end(GTK_BOX (hbox), align5, FALSE, FALSE, 0);
		g_signal_connect(button, "clicked",
				 G_CALLBACK (im_chooser_simple_prefs_button_on_clicked), im);

		gtk_container_set_border_width(GTK_CONTAINER (vbox), 0);
		gtk_box_pack_start(GTK_BOX (vbox), align2, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX (vbox), align, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX (vbox), align4, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX (vbox), align3, FALSE, FALSE, 0);

		im->widget = vbox;
	}
	gtk_widget_show_all(im->widget);
	_im_chooser_simple_update_im_list(im);

	gtk_widget_set_sensitive(im->widget_scrolled, FALSE);
	gtk_widget_set_sensitive(im->button_im_config, FALSE);
	gtk_widget_set_sensitive(im->checkbox_is_im_enabled, FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (im->checkbox_is_im_enabled),
				     (im->current_im != NULL));

	im->initialized = TRUE;

	return im->widget;
}

gboolean
im_chooser_simple_is_modified(IMChooserSimple *im)
{
	g_return_val_if_fail (IM_IS_CHOOSER_SIMPLE (im), FALSE);

	return im->initial_im != im->current_im;
}
