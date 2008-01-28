/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * im-chooser-simple.c
 * Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
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
#include "imsettings/imsettings.h"
#include "imsettings/imsettings-request.h"
#include "im-chooser-simple.h"


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
	GObject             parent_instance;
	GtkWidget          *widget;
	GtkWidget          *checkbox_is_im_enabled;
	GtkWidget          *widget_scrolled;
	GtkWidget          *widget_im_list;
	GtkWidget          *button_im_config;
	IMSettingsRequest  *imsettings;
	IMSettingsRequest  *imsettings_info;
	gchar             **im_list;
	gchar              *initial_im;
	gchar              *current_im;
	gchar              *default_im;
	gboolean            initialized;
};


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
	gchar *prog, *args;

	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
	g_return_if_fail (IM_IS_CHOOSER_SIMPLE (user_data));

	im = IM_CHOOSER_SIMPLE (user_data);

	flag = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive(im->widget_scrolled, flag);
	if (im->current_im &&
	    imsettings_request_get_preferences_program(im->imsettings_info, im->current_im, &prog, &args) &&
	    g_file_test(prog, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_EXECUTABLE))
		gtk_widget_set_sensitive(im->button_im_config, flag);
	else
		gtk_widget_set_sensitive(im->button_im_config, FALSE);

	if (flag) {
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		gchar *name;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (im->widget_im_list));
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(model, &iter, 1, &name, -1);
			if (im->current_im) {
				imsettings_request_stop_im(im->imsettings, im->current_im, TRUE);
				g_free(im->current_im);
			}
			im->current_im = g_strdup(name);
			imsettings_request_start_im(im->imsettings, im->current_im);
		}
	} else {
		if (im->current_im) {
			imsettings_request_stop_im(im->imsettings, im->current_im, TRUE);
			g_free(im->current_im);
		}
		im->current_im = NULL;
	}
	g_signal_emit(im, signals[CHANGED], 0, NULL);
}

static void
im_chooser_simple_im_list_on_changed(GtkTreeSelection *selection,
				     gpointer          user_data)
{
	IMChooserSimple *im = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name, *prog, *args;

	if (im->initialized == FALSE)
		return;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 1, &name, -1);
		if (strcmp(im->current_im, name) != 0) {
			if (im->current_im) {
				imsettings_request_stop_im(im->imsettings, im->current_im, TRUE);
				g_free(im->current_im);
			}
			im->current_im = g_strdup(name);
			imsettings_request_start_im(im->imsettings, im->current_im);
			if (imsettings_request_get_preferences_program(im->imsettings_info, name, &prog, &args) &&
			    g_file_test(prog, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_EXECUTABLE))
				gtk_widget_set_sensitive(im->button_im_config, TRUE);
			else
				gtk_widget_set_sensitive(im->button_im_config, FALSE);
			g_signal_emit(im, signals[CHANGED], 0, NULL);
		}
	}
}

static void
im_chooser_simple_prefs_button_on_clicked(GtkButton *button,
					  gpointer   user_data)
{
	IMChooserSimple *im = IM_CHOOSER_SIMPLE (user_data);
	gchar *prog = NULL, *args = NULL, *cmdline = NULL;

	if (imsettings_request_get_preferences_program(im->imsettings_info, im->current_im, &prog, &args)) {
		cmdline = g_strconcat(prog, args, NULL);
		g_spawn_command_line_async(cmdline, NULL);
	}

	if (cmdline)
		g_free(cmdline);
	if (prog)
		g_free(prog);
	if (args)
		g_free(args);
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
	DBusConnection *conn;

	im->widget = NULL;
	im->initialized = FALSE;

	conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	im->imsettings = imsettings_request_new(conn, IMSETTINGS_INTERFACE_DBUS);
	im->imsettings_info = imsettings_request_new(conn, IMSETTINGS_INFO_INTERFACE_DBUS);

	/* get all the info of the xinput script */
	im->im_list = imsettings_request_get_im_list(im->imsettings);

	/* get current im */
	im->current_im = NULL;
	im->default_im = NULL;
	im->initial_im = NULL;
}

static void
_im_chooser_simple_update_im_list(IMChooserSimple *im)
{
	GtkListStore *list;
	GtkTreeIter iter, *cur_iter = NULL;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkRequisition requisition;
	guint count;
	gint i;

	count = g_strv_length(im->im_list);
	list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	for (i = 0; im->im_list[i] != NULL; i++) {
		GString *string = g_string_new(NULL);

		gtk_list_store_append(list, &iter);
		g_string_append(string, "<i>");
		g_string_append_printf(string, _("Use %s"), im->im_list[i]);
		if (imsettings_request_is_xim(im->imsettings_info, im->im_list[i])) {
			g_string_append(string, _(" (legacy)"));
		}
		if (imsettings_request_is_system_default(im->imsettings_info, im->im_list[i])) {
			g_string_append(string, _(" (recommended)"));
			if (!im->default_im)
				im->default_im = g_strdup(im->im_list[i]);
		}
		if (im->current_im == NULL &&
		    imsettings_request_is_user_default(im->imsettings_info, im->im_list[i])) {
			im->current_im = g_strdup(im->im_list[i]);
			if (im->initial_im == NULL)
				im->initial_im = g_strdup(im->current_im);
			cur_iter = gtk_tree_iter_copy(&iter);
		}
		g_string_append(string, "</i>");
		gtk_list_store_set(list, &iter,
				   0, string->str,
				   1, im->im_list[i],
				   -1);
		g_string_free(string, TRUE);
	}
	if (cur_iter == NULL) {
		if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL (list), &iter))
			cur_iter = gtk_tree_iter_copy(&iter);
	}
	if (cur_iter != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW (im->widget_im_list), GTK_TREE_MODEL (list));
		column = gtk_tree_view_get_column(GTK_TREE_VIEW (im->widget_im_list), 0);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL (list), cur_iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW (im->widget_im_list), path, column, FALSE);
		gtk_tree_path_free(path);
		gtk_tree_iter_free(cur_iter);
	}
	g_object_unref(list);

	if (count == 0) {
		gtk_widget_set_sensitive(im->checkbox_is_im_enabled, FALSE);
		gtk_widget_hide(im->widget_scrolled);
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
		label3 = gtk_label_new(_("<small><i>Note: this change will not take effect until you next log in, except GTK+ applications.</i></small>"));
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
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (im->checkbox_is_im_enabled),
				     FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (im->checkbox_is_im_enabled),
				     (im->current_im != NULL));

	im->initialized = TRUE;

	return im->widget;
}

gboolean
im_chooser_simple_is_modified(IMChooserSimple *im)
{
	g_return_val_if_fail (IM_IS_CHOOSER_SIMPLE (im), FALSE);

	return !((im->initial_im == NULL && im->current_im == NULL) ||
		 (im->initial_im != NULL && im->current_im != NULL &&
		  strcmp(im->initial_im,im->current_im) == 0));
}
