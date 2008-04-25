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

enum {
	PROP_0,
	PROP_PARENT_WINDOW,
	LAST_PROP
};

typedef enum {
	ACTION_IM_START,
	ACTION_IM_STARTING,
	ACTION_IM_STOP,
	ACTION_IM_STOPPING,
	ACTION_IM_UPDATE_PREFS,
	ACTION_COMPLETE,
	ACTION_END
} IMChooserSimpleAction;

typedef struct _IMChooserSimpleActionData {
	IMChooserSimpleAction action;
	gpointer              data;
	GDestroyNotify        destroy;
} IMChooserSimpleActionData;

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
	GtkWidget          *progress;
	GtkWidget          *label;
	IMSettingsRequest  *imsettings;
	IMSettingsRequest  *imsettings_info;
	gchar             **im_list;
	gchar              *initial_im;
	gchar              *current_im;
	gchar              *default_im;
	gboolean            initialized;
	DBusConnection     *conn;
	GQueue             *actionq;
	gboolean            ignore_actions;
	guint               idle_source;
};


static IMChooserSimpleActionData *_action_new                      (IMChooserSimpleAction      act,
                                                                    gpointer                   data,
								    GDestroyNotify             func);
static void                       _action_free                     (IMChooserSimpleActionData *a);
static void                       im_chooser_simple_show_progress  (IMChooserSimple           *im,
								    const gchar               *message,
								    ...);
static gboolean                   im_chooser_simple_action_loop    (gpointer                   data);
static void                       _im_chooser_simple_update_im_list(IMChooserSimple           *im);


static GObjectClass *parent_class = NULL;
static guint         signals[LAST_SIGNAL] = { 0 };


/*
 * signal callback functions
 */
static void
im_chooser_simple_destroy_idle_cb(gpointer data)
{
	IMChooserSimple *im = IM_CHOOSER_SIMPLE (data);

	im->idle_source = 0;
}

static void
im_chooser_simple_enable_im_on_toggled(GtkToggleButton *button,
				       gpointer         user_data)
{
	gboolean flag;
	IMChooserSimple *im;
	IMChooserSimpleActionData *a;

	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
	g_return_if_fail (IM_IS_CHOOSER_SIMPLE (user_data));

	im = IM_CHOOSER_SIMPLE (user_data);

	flag = gtk_toggle_button_get_active(button);

	gtk_widget_set_sensitive(im->widget_scrolled, flag);
	if (flag) {
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		gchar *name;

		if (im->initialized) {
#if 0
			im_chooser_simple_show_progress(im, _("Updating Input Method list"));
#endif
			model = gtk_tree_view_get_model(GTK_TREE_VIEW (im->widget_im_list));
			gtk_list_store_clear(GTK_LIST_STORE (model));
			g_strfreev(im->im_list);
			im->im_list = imsettings_request_get_im_list(im->imsettings_info);
			_im_chooser_simple_update_im_list(im);
		}
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (im->widget_im_list));
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(model, &iter, 1, &name, -1);
			if (im->current_im) {
				if (im->initialized) {
					a = _action_new(ACTION_IM_STOP,
							g_strdup(im->current_im),
							g_free);
					g_queue_push_tail(im->actionq, a);
				}
			}
			if (im->initialized) {
				a = _action_new(ACTION_IM_START,
						g_strdup(name),
						g_free);
				g_queue_push_tail(im->actionq, a);
			}
		}
	} else {
		if (im->current_im) {
			if (im->initialized) {
				a = _action_new(ACTION_IM_STOP,
						g_strdup(im->current_im),
						g_free);
				g_queue_push_tail(im->actionq, a);
			}
			g_free(im->current_im);
		}
		im->current_im = NULL;
	}
	a = _action_new(ACTION_IM_UPDATE_PREFS, GINT_TO_POINTER (flag), NULL);
	g_queue_push_tail(im->actionq, a);
	a = _action_new(ACTION_COMPLETE, NULL, NULL);
	g_queue_push_tail(im->actionq, a);

	if (im->idle_source == 0)
		im->idle_source = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
						  im_chooser_simple_action_loop,
						  im,
						  im_chooser_simple_destroy_idle_cb);
}

static void
im_chooser_simple_im_list_on_changed(GtkTreeSelection *selection,
				     gpointer          user_data)
{
	IMChooserSimple *im = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name;
	IMChooserSimpleActionData *a;

	if (im->initialized == FALSE)
		return;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 1, &name, -1);
		if (im->current_im &&
		    strcmp(im->current_im, name) != 0) {
			a = _action_new(ACTION_IM_STOP,
					g_strdup(im->current_im),
					g_free);
			g_queue_push_tail(im->actionq, a);
			a = _action_new(ACTION_IM_START,
					g_strdup(name),
					g_free);
			g_queue_push_tail(im->actionq, a);
			a = _action_new(ACTION_IM_UPDATE_PREFS,
					GINT_TO_POINTER (TRUE),
					NULL);
			g_queue_push_tail(im->actionq, a);
			a = _action_new(ACTION_COMPLETE, NULL, NULL);
			g_queue_push_tail(im->actionq, a);

			if (im->idle_source == 0)
				im->idle_source = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
								  im_chooser_simple_action_loop,
								  im,
								  im_chooser_simple_destroy_idle_cb);
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
static IMChooserSimpleActionData *
_action_new(IMChooserSimpleAction act,
	    gpointer              data,
	    GDestroyNotify        func)
{
	IMChooserSimpleActionData *retval = g_new0(IMChooserSimpleActionData, 1);

	retval->action = act;
	retval->data = data;
	retval->destroy = func;

	return retval;
}

static void
_action_free(IMChooserSimpleActionData *a)
{
	if (a->destroy)
		a->destroy(a->data);
	g_free(a);
}

static gint
_action_compare(gconstpointer a,
		gconstpointer b)
{
	const IMChooserSimpleActionData *aa = a;
	IMChooserSimpleAction ab = GPOINTER_TO_INT (b);

	return aa->action - ab;
}

static void
im_chooser_simple_im_start_cb(DBusGProxy *proxy,
			      gboolean    ret,
			      GError     *error,
			      gpointer    data)
{
	IMChooserSimple *im = data;
	GList *l;

	if (ret) {
	} else {
		g_free(im->current_im);
		im->current_im = NULL;

		if (error) {
			GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW (im->progress),
								GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
								GTK_MESSAGE_ERROR,
								GTK_BUTTONS_OK,
								_("Failed to start Input Method"));

			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dlg),
								 error->message);
			gtk_dialog_run(GTK_DIALOG (dlg));
			im->ignore_actions = TRUE;
		}
	}

	l = g_queue_find_custom(im->actionq,
				GINT_TO_POINTER (ACTION_IM_STARTING),
				_action_compare);
	if (l)
		g_queue_delete_link(im->actionq, l);
}

static void
im_chooser_simple_im_stop_cb(DBusGProxy *proxy,
			     gboolean    ret,
			     GError     *error,
			     gpointer    data)
{
	IMChooserSimple *im = data;
	GList *l;

	if (ret) {
		g_free(im->current_im);
		im->current_im = NULL;
	} else {
		if (error) {
			GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW (im->progress),
								GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
								GTK_MESSAGE_ERROR,
								GTK_BUTTONS_OK,
								_("Failed to start Input Method"));

			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dlg),
								 error->message);
			gtk_dialog_run(GTK_DIALOG (dlg));
			im->ignore_actions = TRUE;
		}
	}

	l = g_queue_find_custom(im->actionq,
				GINT_TO_POINTER (ACTION_IM_STOPPING),
				_action_compare);
	if (l)
		g_queue_delete_link(im->actionq, l);
}

static void
im_chooser_simple_show_progress(IMChooserSimple *im,
				const gchar     *message,
				...)
{
	va_list args;
	gchar *s;

	if (message) {
		gtk_widget_set_sensitive(im->widget, FALSE);

		va_start(args, message);

		s = g_strdup_vprintf(message, args);
		gtk_label_set_markup(GTK_LABEL (im->label), s);
		g_free(s);

		va_end(args);

		gtk_window_set_modal(GTK_WINDOW (im->progress), TRUE);
		gtk_window_set_position(GTK_WINDOW (im->progress),
					GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_widget_show(im->progress);
	} else {
		gtk_window_set_modal(GTK_WINDOW (im->progress), FALSE);
		gtk_widget_hide(im->progress);

		gtk_widget_set_sensitive(im->widget, TRUE);
	}

	while (g_main_context_pending(NULL))
		g_main_context_iteration(NULL, TRUE);
}

static gboolean
im_chooser_simple_action_loop(gpointer data)
{
	IMChooserSimple *im = data;
	IMChooserSimpleActionData *a;
	gboolean retval = FALSE;
	gchar *prog, *args;

	if (!g_queue_is_empty(im->actionq)) {
		a = g_queue_pop_head(im->actionq);
		switch (a->action) {
		    case ACTION_IM_START:
			    if (im->initialized && !im->ignore_actions) {
				    im_chooser_simple_show_progress(im, _("Starting Input Method - %s"), a->data);
				    a->action = ACTION_IM_STARTING;
				    g_queue_push_head(im->actionq, a);
				    g_free(im->current_im);
				    im->current_im = g_strdup(a->data);
				    imsettings_request_start_im_async(im->imsettings,
								      a->data,
								      TRUE,
								      im_chooser_simple_im_start_cb,
								      im);
			    } else {
				    /* just ignore an action */
				    _action_free(a);
			    }
			    break;
		    case ACTION_IM_STOP:
			    if (im->initialized && !im->ignore_actions) {
				    im_chooser_simple_show_progress(im, _("Stopping Input Method - %s"), a->data);
				    a->action = ACTION_IM_STOPPING;
				    g_queue_push_head(im->actionq, a);
				    imsettings_request_stop_im_async(im->imsettings,
								     a->data,
								     TRUE,
								     TRUE,
								     im_chooser_simple_im_stop_cb,
								     im);
			    } else {
				    /* just ignore an action */
				    _action_free(a);
			    }
			    break;
		    case ACTION_IM_STARTING:
		    case ACTION_IM_STOPPING:
			    if (!im->ignore_actions) {
				    /* requeue to wait for finishing an action */
				    g_queue_push_head(im->actionq, a);
			    } else {
				    _action_free(a);
			    }
			    break;
		    case ACTION_IM_UPDATE_PREFS:
			    if (im->current_im && !im->ignore_actions &&
				imsettings_request_get_preferences_program(im->imsettings_info,
									   im->current_im,
									   &prog,
									   &args) &&
				g_file_test(prog,
					    G_FILE_TEST_EXISTS |
					    G_FILE_TEST_IS_EXECUTABLE))
				    gtk_widget_set_sensitive(im->button_im_config,
							     GPOINTER_TO_INT (a->data));
			    else
				    gtk_widget_set_sensitive(im->button_im_config,
							     FALSE);
			    _action_free(a);
			    break;
		    case ACTION_COMPLETE:
			    /* get back from "in-progress" mode */
			    im_chooser_simple_show_progress(im, NULL);
			    im->ignore_actions = FALSE;

			    g_signal_emit(im, signals[CHANGED], 0, NULL);
			    break;
		    default:
			    g_warning("Unknown action id: %d\n", a->action);
			    _action_free(a);
			    break;
		}
		retval = TRUE;
	}

	return retval;
}

static void
im_chooser_simple_set_property(GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	IMChooserSimple *im = IM_CHOOSER_SIMPLE (object);

	switch (prop_id) {
	    case PROP_PARENT_WINDOW:
		    gtk_window_set_transient_for(GTK_WINDOW (im->progress),
						 GTK_WINDOW (g_value_get_object(value)));
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
im_chooser_simple_get_property(GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	switch (prop_id) {
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
im_chooser_simple_finalize(GObject *object)
{
	IMChooserSimple *simple = IM_CHOOSER_SIMPLE (object);

	g_object_unref(simple->imsettings);
	g_object_unref(simple->imsettings_info);
	g_strfreev(simple->im_list);
	g_free(simple->initial_im);
	g_free(simple->current_im);
	g_free(simple->default_im);
	dbus_connection_unref(simple->conn);
	g_queue_free(simple->actionq);
}

static void
im_chooser_simple_class_init(IMChooserSimpleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->set_property = im_chooser_simple_set_property;
	object_class->get_property = im_chooser_simple_get_property;
	object_class->finalize = im_chooser_simple_finalize;

	/* properties */
	g_object_class_install_property(object_class, PROP_PARENT_WINDOW,
					g_param_spec_object("parent_window",
							    _("Parent Window"),
							    _("GtkWindow that points to the parent window"),
							    GTK_TYPE_WINDOW,
							    G_PARAM_READWRITE));

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
	gchar *locale = setlocale(LC_CTYPE, NULL);
	GtkWidget *image, *hbox, *vbox;

	im->widget = NULL;
	im->initialized = FALSE;
	im->actionq = g_queue_new();
	g_queue_init(im->actionq);
	im->progress = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	im->label = gtk_label_new(NULL);
	im->ignore_actions = FALSE;
	im->idle_source = 0;

	hbox = gtk_hbox_new(FALSE, 12);
	vbox = gtk_vbox_new(FALSE, 12);
	/* setup a progress window */
	gtk_window_set_title(GTK_WINDOW (im->progress), _("Work in progress..."));
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW (im->progress), TRUE);
	gtk_window_set_decorated(GTK_WINDOW (im->progress), FALSE);
	/* setup widgets in the progress window */
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
					 GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC (image), 0.5, 0.0);
	gtk_label_set_line_wrap(GTK_LABEL (im->label), TRUE);
	gtk_misc_set_alignment(GTK_MISC (im->label), 0.0, 0.0);

	gtk_box_pack_start(GTK_BOX (vbox), im->label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (im->progress), 5);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

	gtk_container_add(GTK_CONTAINER (im->progress), hbox);
	gtk_widget_show_all(hbox);

	im->conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	im->imsettings = imsettings_request_new(im->conn, IMSETTINGS_INTERFACE_DBUS);
	im->imsettings_info = imsettings_request_new(im->conn, IMSETTINGS_INFO_INTERFACE_DBUS);

	imsettings_request_set_locale(im->imsettings, locale);
	imsettings_request_set_locale(im->imsettings_info, locale);

	/* get all the info of the xinput script */
	im->im_list = imsettings_request_get_im_list(im->imsettings_info);

	/* get current im */
	im->current_im = NULL;
	im->default_im = NULL;
	im->initial_im = NULL;
}

static void
_im_chooser_simple_update_im_list(IMChooserSimple *im)
{
	GtkListStore *list;
	GtkTreeIter iter, *cur_iter = NULL, *def_iter = NULL, *first_iter = NULL;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkRequisition requisition;
	guint count = 0;
	gint i, priority = 0;
	gchar *user_im, *system_im, *running_im;

	user_im = imsettings_request_get_current_user_im(im->imsettings_info);
	system_im = imsettings_request_get_current_system_im(im->imsettings_info);
	running_im = imsettings_request_what_im_is_running(im->imsettings);
	if (im->im_list == NULL)
		goto end;

	count = g_strv_length(im->im_list);
	list = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	for (i = 0; im->im_list[i] != NULL; i++) {
		GString *string = g_string_new(NULL);

		priority = 50;
		gtk_list_store_append(list, &iter);
		g_string_append(string, "<i>");
		g_string_append_printf(string, _("Use %s"), im->im_list[i]);
		if (imsettings_request_is_xim(im->imsettings_info, im->im_list[i])) {
			priority = 100;
			g_string_append(string, _(" (legacy)"));
		}
		if (strcmp(system_im, im->im_list[i]) == 0) {
			priority = 10;
			g_string_append(string, _(" (recommended)"));
			if (!im->default_im)
				im->default_im = g_strdup(im->im_list[i]);
			def_iter = gtk_tree_iter_copy(&iter);
		}
		if (im->current_im == NULL &&
		    strcmp(running_im, im->im_list[i]) == 0) {
			im->current_im = g_strdup(im->im_list[i]);
			if (im->initial_im == NULL)
				im->initial_im = g_strdup(im->current_im);
			cur_iter = gtk_tree_iter_copy(&iter);
		}
		g_string_append(string, "</i>");
		gtk_list_store_set(list, &iter,
				   0, string->str,
				   1, im->im_list[i],
				   2, priority,
				   -1);
		g_string_free(string, TRUE);
	}
	if (cur_iter == NULL && def_iter == NULL) {
		cur_iter = first_iter;
		first_iter = NULL;
	} else if (cur_iter == NULL) {
		cur_iter = def_iter;
		def_iter = NULL;
	}
	if (cur_iter != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW (im->widget_im_list), GTK_TREE_MODEL (list));
		column = gtk_tree_view_get_column(GTK_TREE_VIEW (im->widget_im_list), 0);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL (list), cur_iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW (im->widget_im_list), path, column, FALSE);
		gtk_tree_path_free(path);
		gtk_tree_iter_free(cur_iter);
		g_signal_emit_by_name(column, "clicked", 0, NULL);
	}
	if (first_iter)
		gtk_tree_iter_free(first_iter);
	if (def_iter)
		gtk_tree_iter_free(def_iter);
	g_object_unref(list);
	g_free(user_im);
	g_free(system_im);

  end:
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
