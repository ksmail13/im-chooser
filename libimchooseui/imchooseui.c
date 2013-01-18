/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imchooseui.c
 * Copyright (C) 2007-2012 Red Hat, Inc. All rights reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA  02110-1301  USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <imsettings/imsettings.h>
#include <imsettings/imsettings-client.h>
#include <imsettings/imsettings-utils.h>
#include "imchooseui-marshal.h"
#include "imchooseuicellrendererlabel.h"
#include "imchooseui.h"

#define IMCHOOSEUI_GERROR	(_imchoose_ui_get_quark())

static gboolean _imchoose_ui_label_activate_link(GtkLabel     *label,
                                                 const gchar  *uri,
                                                 gpointer      user_data);
static void     _imchoose_ui_switch_im_finish   (GObject      *source_object,
                                                 GAsyncResult *res,
                                                 gpointer      user_data);

typedef struct _IMChooseUIPrivate {
	IMSettingsClient *client;
	gchar            *default_im;
	gchar            *initial_im;
	gchar            *current_im;
	guint             note_type;
	gboolean          clicked:1;
} IMChooseUIPrivate;
enum {
	POS_ICON = 0,
	POS_LABEL,
	POS_PREFS,
	POS_IMNAME,
	POS_IMINFO,
};
enum {
	SIG_0 = 0,
	SIG_SHOW_PROGRESS,
	SIG_HIDE_PROGRESS,
	SIG_SHOW_ERROR,
	SIG_CHANGED,
	SIG_END
};

static guint signals[SIG_END] = { 0 };

G_DEFINE_TYPE (IMChooseUI, imchoose_ui, G_TYPE_OBJECT);

/*< private >*/
static void
_imchoose_ui_show_progress(IMChooseUI  *ui,
			   const gchar *message,
			   ...)
{
	va_list args;
	gchar *s;

	if (message) {
		va_start(args, message);

		s = g_strdup_vprintf(message, args);
		g_signal_emit(ui, signals[SIG_SHOW_PROGRESS], 0, s);
		g_free(s);

		va_end(args);
	} else {
		g_signal_emit(ui, signals[SIG_HIDE_PROGRESS], 0);
	}
}

static GQuark
_imchoose_ui_get_quark(void)
{
	static GQuark retval = 0;

	if (retval == 0)
		retval = g_quark_from_static_string("imchooseui-error");

	return retval;
}

static void
_imchoose_ui_add_row(IMChooseUI      *ui,
		     GtkListStore    *list,
		     GtkTreeIter     *iter,
		     GtkTreeIter    **def_iter,
		     GtkTreeIter    **cur_iter,
		     IMSettingsInfo  *info,
		     const gchar     *running_im)
{
	GString *string = g_string_new(NULL);
	GString *prefs_string = g_string_new(NULL);
	GdkPixbuf *pixbuf = NULL;
	GtkWidget *label;
	GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
	const gchar *icon_file = imsettings_info_get_icon_file(info);
	const gchar *name = imsettings_info_get_short_desc(info);
	const gchar *imname = imsettings_info_get_im_name(info);
	const gchar *subimname = imsettings_info_get_sub_im_name(info);
	const gchar *prefsname = imsettings_info_get_prefs_program(info);
	IMChooseUIPrivate *priv = ui->priv;

	if (icon_file && icon_file[0] == 0)
		icon_file = NULL;
	if (gtk_icon_theme_has_icon(icon_theme, icon_file)) {
		pixbuf = gtk_icon_theme_load_icon(icon_theme, icon_file, 18, 0, NULL);
	} else if (icon_file && g_file_test(icon_file, G_FILE_TEST_EXISTS)) {
		pixbuf = gdk_pixbuf_new_from_file_at_scale(icon_file,
							   18, 18,
							   TRUE, NULL);
	} else {
		static const char *default_xpm[] = {
			"18 18 1 1",
			" 	c None",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  ",
			"                  "};

		pixbuf = gdk_pixbuf_new_from_xpm_data(default_xpm);
	}
	gtk_list_store_append(list, iter);
	g_string_append(string, "<i>");
	if (strcmp(name, IMSETTINGS_NONE_CONF) == 0) {
		g_string_append(string, _("No Input Method"));
	} else {
		g_string_append_printf(string, _("Use %s"), imname);
		if (subimname) {
			g_string_append_printf(string, "[%s]", subimname);
		}
	}

	if (imsettings_info_is_xim(info))
		g_string_append(string, _(" (legacy)"));
	if (imsettings_info_is_system_default(info)) {
		g_string_append(string, _(" (recommended)"));
		if (!priv->default_im)
			priv->default_im = g_strdup(name);
		*def_iter = gtk_tree_iter_copy(iter);
	}
	if (!priv->current_im &&
	    ((running_im && strcmp(running_im, name) == 0) ||
	     ((!running_im || running_im[0] == 0) &&
	      strcmp(name, IMSETTINGS_NONE_CONF) == 0))) {
		priv->current_im = g_strdup(name);
		if (!priv->initial_im)
			priv->initial_im = g_strdup(name);
		*cur_iter = gtk_tree_iter_copy(iter);
	}
	g_string_append(string, "</i>");

	if (prefsname != NULL && prefsname[0] != 0) {
		g_string_append_printf(prefs_string,
				       _("<a href=\"imsettings-prefs:///%s\">Preferences...</a>"),
				       name);
	}
	/* label isn't really attached to any widgets
	 * but the cell renderer is delegated to render the label.
	 * so the floating reference is needed to cut off here.
	 */
	label = g_object_ref_sink(gtk_label_new(prefs_string->str));
	gtk_label_set_use_markup(GTK_LABEL (label), TRUE);
	g_signal_connect(label, "activate_link",
			 G_CALLBACK (_imchoose_ui_label_activate_link),
			 ui);
	gtk_widget_show(label);

	gtk_list_store_set(list, iter,
			   POS_ICON, pixbuf,
			   POS_LABEL, string->str,
			   POS_PREFS, GTK_LABEL (label),
			   POS_IMNAME, name,
			   POS_IMINFO, info,
			   -1);
	g_object_unref(label);
	g_string_free(string, TRUE);
	g_string_free(prefs_string, TRUE);
}

static gboolean
_imchoose_ui_update_list(IMChooseUI *ui,
			 GtkWidget  *widget,
			 GError    **error)
{
	const gchar *locale = setlocale(LC_CTYPE, NULL), *key;
	IMSettingsClient *client = imsettings_client_new(locale);
	GError *err = NULL;
	gboolean retval = FALSE;
	gint api_version, n_retry = 0, i;
	IMSettingsInfo *info, *none_info = NULL, *active_info = NULL;
	GVariant *v, *vv;
	GVariantIter *viter;
	GtkListStore *list;
	GtkTreeIter iter, *cur_iter = NULL, *def_iter = NULL;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkRequisition requisition;
	IMChooseUIPrivate *priv = ui->priv;
	gsize len, slen = strlen(".conf");

	if (!imsettings_is_enabled()) {
		g_set_error(&err, IMCHOOSEUI_GERROR, 0,
			    _("IMSettings is disabled on the system."));
		goto bail;
	}
	if (!client) {
		g_set_error(&err, IMCHOOSEUI_GERROR, 0,
			    _("Unable to create a client instance."));
		goto bail;
	}
  retry:
	if ((api_version = imsettings_client_get_version(client, NULL, &err)) != IMSETTINGS_SETTINGS_API_VERSION) {
		if (n_retry > 0) {
			GError *e = NULL;

			g_set_error(&e, IMCHOOSEUI_GERROR, 0,
				    _("Unable to communicate to IMSettings service: %s"),
				    err->message);
			g_error_free(err);
			err = e;
			goto bail;
		}
		imsettings_client_reload(client, (api_version < 4), NULL, &err);
		while (imsettings_client_ping(client))
			sleep(1);
		/* this instance may be not a valid anymore */
		g_object_unref(client);
		client = imsettings_client_new(locale);
		n_retry++;
		goto retry;
	}

	active_info = imsettings_client_get_active_im_info(client, NULL, &err);
	if (err)
		goto bail;
	none_info = imsettings_client_get_info_object(client, "none", NULL, &err);
	if (err)
		goto bail;
	v = imsettings_client_get_info_variants(client, NULL, &err);
	if (err)
		goto bail;

	list = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING, GTK_TYPE_LABEL, G_TYPE_STRING, G_TYPE_OBJECT);
	priv->client = client;
	_imchoose_ui_add_row(ui, list,
			     &iter, &def_iter, &cur_iter,
			     none_info,
			     imsettings_info_get_short_desc(active_info));

	g_variant_get(v, "a{sv}", &viter);
	i = 0;
	while (g_variant_iter_next(viter, "{&sv}", &key, &vv)) {
		len = strlen(key);
		if (len > slen &&
		    strcmp(&key[len - slen], ".conf") == 0)
			continue;
		info = imsettings_info_new(vv);
		_imchoose_ui_add_row(ui, list,
				     &iter, &def_iter, &cur_iter,
				     info,
				     imsettings_info_get_short_desc(active_info));

		g_object_unref(info);
		i++;
	}
	g_variant_iter_free(viter);
	g_variant_unref(v);

	if (!cur_iter) {
		cur_iter = def_iter;
		def_iter = NULL;
	}
	if (cur_iter) {
		gtk_tree_view_set_model(GTK_TREE_VIEW (widget), GTK_TREE_MODEL (list));
		column = gtk_tree_view_get_column(GTK_TREE_VIEW (widget), 0);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL (list), cur_iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW (widget), path, column, FALSE);
		gtk_tree_path_free(path);
		gtk_tree_iter_free(cur_iter);
		g_signal_emit_by_name(column, "clicked", 0, NULL);
	}
	if (def_iter)
		gtk_tree_iter_free(def_iter);
	g_object_unref(list);

	gtk_widget_get_preferred_size(widget, &requisition, NULL);
	if (requisition.height > 120)
		requisition.height = 120;
	gtk_widget_set_size_request(widget, -1, requisition.height);

	if (i == 0) {
		g_set_error(&err, IMCHOOSEUI_GERROR, 1,
			    _("Please install any input methods before running if you like."));
		goto bail;
	}
	retval = TRUE;
  bail:
	if (active_info)
		g_object_unref(active_info);
	if (none_info)
		g_object_unref(none_info);
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		}
		g_warning("%s", err->message);
		g_error_free(err);
	}

	return retval;
}

static void
_imchoose_ui_label_clicked(IMChooseUICellRendererLabel *celllabel,
			   GdkEvent                    *event,
			   const gchar                 *path,
			   gpointer                     user_data)
{
	GtkTreeView *tree = GTK_TREE_VIEW (user_data);
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *label;
	gboolean retval;
	IMChooseUI *ui = g_object_get_data(G_OBJECT (tree), "imchoose-ui");
	IMChooseUIPrivate *priv = ui->priv;

	model = gtk_tree_view_get_model(tree);

	if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
		gtk_tree_model_get(model, &iter,
				   POS_PREFS, &label,
				   -1);
		g_return_if_fail (GTK_IS_LABEL (label));
		priv->clicked = TRUE;
		g_signal_emit_by_name(label, "motion-notify-event", event, &retval);
		g_signal_emit_by_name(label, "button-press-event", event, &retval);
		g_signal_emit_by_name(label, "button-release-event", event, &retval);
		g_signal_emit_by_name(label, "leave-notify-event", event, &retval);
	}
}

static gboolean
_imchoose_ui_label_activate_link(GtkLabel    *label,
				 const gchar *uri,
				 gpointer     user_data)
{
	static const gchar prefs_uri[] = "imsettings-prefs:///";
	gsize len = strlen(prefs_uri);
	gboolean retval = FALSE;
	IMChooseUI *ui = IMCHOOSE_UI (user_data);
	IMChooseUIPrivate *priv = ui->priv;

	if (uri &&
	    strncmp(uri, prefs_uri, len) == 0 &&
	    uri[len] != 0) {
		IMSettingsInfo *info;
		GError *err = NULL;
		const gchar *prog = NULL, *args = NULL;
		gchar *cmdline = NULL;

		info = imsettings_client_get_info_object(priv->client, &uri[len],
							 NULL, &err);
		if (err) {
			g_warning("%s", err->message);
			g_signal_emit(ui, signals[SIG_SHOW_ERROR], 0,
				      _("Unable to get the information"),
				      err->message);
			g_error_free(err);
			goto bail;
		}
		prog = imsettings_info_get_prefs_program(info);
		args = imsettings_info_get_prefs_args(info);

		cmdline = g_strconcat(prog, args, NULL);
		g_spawn_command_line_async(cmdline, &err);
		if (err) {
			g_warning("%s", err->message);
			g_signal_emit(ui, signals[SIG_SHOW_ERROR], 0,
				      _("Unable to invoke the preference tool"),
				      err->message);
			g_error_free(err);
			goto bail;
		}
		retval = TRUE;
	  bail:
		g_free(cmdline);
		if (info)
			g_object_unref(info);
	}

	return retval;
}

static void
_imchoose_ui_switch_im_start(IMChooseUI  *ui,
			     const gchar *imname)
{
	IMChooseUIPrivate *priv = ui->priv;

	_imchoose_ui_show_progress(ui, _("Switching Input Method - %s"), imname);
	g_free(priv->current_im);
	priv->current_im = g_strdup(imname);
	imsettings_client_switch_im_start(priv->client, imname,
					  TRUE, NULL,
					  _imchoose_ui_switch_im_finish,
					  ui);
}

static void
_imchoose_ui_switch_im_finish(GObject      *source_object,
			      GAsyncResult *res,
			      gpointer      user_data)
{
	IMChooseUI *ui = IMCHOOSE_UI (user_data);
	IMChooseUIPrivate *priv = ui->priv;
	GError *err = NULL;

	if (!imsettings_client_switch_im_finish(priv->client,
						res, &err)) {
		if (!err) {
			g_set_error(&err, IMCHOOSEUI_GERROR, 0,
				    _("Unknown error during finalizing the request of SwitchIM"));
		}
	}
	_imchoose_ui_show_progress(ui, NULL);
	if (err) {
		g_warning("%s", err->message);
		g_signal_emit(ui, signals[SIG_SHOW_ERROR], 0,
			      _("Failed to switch Input Method"),
			      err->message);
		g_error_free(err);
	} else {
		g_signal_emit(ui, signals[SIG_CHANGED], 0, priv->current_im, priv->initial_im);
	}
}

static void
_imchoose_ui_tree_selection_changed(GtkTreeSelection *selection,
				    gpointer          user_data)
{
	GtkTreeView *tree = GTK_TREE_VIEW (user_data);
	GtkTreeModel *model;
	GtkTreeIter iter;
	IMChooseUI *ui = IMCHOOSE_UI (g_object_get_data(G_OBJECT (tree), "imchoose-ui"));
	IMChooseUIPrivate *priv = ui->priv;
	const gchar *name;

	if (priv->current_im && priv->clicked) {
		model = gtk_tree_view_get_model(tree);
		if (gtk_tree_model_get_iter_first(model, &iter)) {
			do {
				gtk_tree_model_get(model, &iter, POS_IMNAME, &name, -1);
				if (strcmp(priv->current_im, name) == 0) {
					gtk_tree_selection_select_iter(selection, &iter);
					break;
				}
			} while (gtk_tree_model_iter_next(model, &iter));
		} else {
		}
		priv->clicked = FALSE;
	} else {
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(model, &iter, POS_IMNAME, &name, -1);
			if (g_strcmp0(priv->current_im, name) != 0) {
				_imchoose_ui_switch_im_start(ui, name);
			}
		} else {
		}
	}
}

static void
_imchoose_ui_finalize(GObject *object)
{
	IMChooseUI *ui = IMCHOOSE_UI (object);
	IMChooseUIPrivate *priv = ui->priv;

	if (priv->client)
		g_object_unref(priv->client);
	g_free(priv->initial_im);
	g_free(priv->default_im);
	g_free(priv->current_im);

	if (G_OBJECT_CLASS (imchoose_ui_parent_class)->finalize)
		G_OBJECT_CLASS (imchoose_ui_parent_class)->finalize(object);
}

static void
imchoose_ui_class_init(IMChooseUIClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private(object_class,
				 sizeof (IMChooseUIPrivate));

	object_class->finalize     = _imchoose_ui_finalize;

	signals[SIG_SHOW_PROGRESS] = g_signal_new("show-progress",
						  G_OBJECT_CLASS_TYPE (object_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (IMChooseUIClass, show_progress),
						  NULL, NULL,
						  imchoose_ui_marshal_VOID__STRING,
						  G_TYPE_NONE, 1,
						  G_TYPE_STRING);
	signals[SIG_HIDE_PROGRESS] = g_signal_new("hide-progress",
						  G_OBJECT_CLASS_TYPE (object_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (IMChooseUIClass, hide_progress),
						  NULL, NULL,
						  imchoose_ui_marshal_VOID__VOID,
						  G_TYPE_NONE, 0);
	signals[SIG_SHOW_ERROR] = g_signal_new("show-error",
					       G_OBJECT_CLASS_TYPE (object_class),
					       G_SIGNAL_RUN_FIRST,
					       G_STRUCT_OFFSET (IMChooseUIClass, show_error),
					       NULL, NULL,
					       imchoose_ui_marshal_VOID__STRING_STRING,
					       G_TYPE_NONE, 2,
					       G_TYPE_STRING,
					       G_TYPE_STRING);
	signals[SIG_CHANGED] = g_signal_new("changed",
					    G_OBJECT_CLASS_TYPE (object_class),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (IMChooseUIClass, changed),
					    NULL, NULL,
					    imchoose_ui_marshal_VOID__STRING_STRING,
					    G_TYPE_NONE, 2,
					    G_TYPE_STRING,
					    G_TYPE_STRING);
}

static void
imchoose_ui_init(IMChooseUI *ui)
{
	ui->priv = G_TYPE_INSTANCE_GET_PRIVATE (ui,
						IMCHOOSE_TYPE_UI,
						IMChooseUIPrivate);
}

/*< public >*/
GQuark
imchoose_ui_progress_label_quark(void)
{
	static GQuark retval = 0;

	if (retval == 0)
		retval = g_quark_from_static_string("imchooseui-progress-label");

	return retval;
}

IMChooseUI *
imchoose_ui_new(void)
{
	return IMCHOOSE_UI (g_object_new(IMCHOOSE_TYPE_UI, NULL));
}

GtkWidget *
imchoose_ui_get(IMChooseUI  *ui,
		GError     **error)
{
	GtkBuilder *builder;
	GError *err = NULL;
	GtkWidget *retval = NULL;
	GObject *note, *tree;
	guint note_type = 0;
	gchar *uifile;
	const gchar *xmodifiers, *gtk_immodule, *qt_immodule;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	IMChooseUIPrivate *priv;

	g_return_val_if_fail (IMCHOOSE_IS_UI (ui), NULL);

	priv = ui->priv;
	builder = gtk_builder_new();
	uifile = g_build_filename(UIDIR, "imchoose.ui", NULL);

#ifdef GNOME_ENABLE_DEBUG
	if (!g_file_test(uifile, G_FILE_TEST_EXISTS)) {
		/* fallback to uninstalled file */
		g_free(uifile);
		uifile = g_build_filename(BUILDDIR, "imchoose.ui", NULL);
	}
#endif /* GNOME_ENABLE_DEBUG */
	if (gtk_builder_add_from_file(builder, uifile, &err) == 0) {
		goto bail;
	}

	xmodifiers = g_getenv("XMODIFIERS");
	gtk_immodule = g_getenv("GTK_IM_MODULE");
	qt_immodule = g_getenv("QT_IM_MODULE");
	if (xmodifiers != NULL && strcmp(xmodifiers, "@im=imsettings") == 0) {
		note_type |= NOTE_TYPE_X;
	}
	if (gtk_immodule == NULL ||
	    gtk_immodule[0] == 0 ||
	    (strcmp(gtk_immodule, "xim") == 0 &&
	     (note_type & NOTE_TYPE_X))) {
		note_type |= NOTE_TYPE_GTK;
	}
	if (qt_immodule == NULL ||
	    qt_immodule[0] == 0 ||
	    (strcmp(qt_immodule, "xim") == 0 &&
	     (note_type & NOTE_TYPE_X))) {
		note_type |= NOTE_TYPE_QT;
	}
	priv->note_type = note_type;

	note = gtk_builder_get_object(builder, "note");
	if (note) {
		gchar *exception = NULL, *note_string = NULL;
		static const gchar note_exception[5][256] = {
			N_("X applications"),
			N_("GTK+ applications"),
			N_("Qt applications")
		};
		gboolean show = TRUE;

		if (note_type == (NOTE_TYPE_X | NOTE_TYPE_GTK | NOTE_TYPE_QT)) {
			show = FALSE;
		} else if (note_type == (NOTE_TYPE_X | NOTE_TYPE_QT)) {
			/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except X applications and Qt applications</i></small>" */
			exception = g_strdup_printf(_(", except %s and %s"),
						    _(note_exception[0]),
						    _(note_exception[2]));
		} else if (note_type == (NOTE_TYPE_X | NOTE_TYPE_GTK)) {
			/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except X applications and GTK+ applications</i></small>" */
			exception = g_strdup_printf(_(", except %s and %s"),
						    _(note_exception[0]),
						    _(note_exception[1]));
		} else if (note_type == (NOTE_TYPE_GTK | NOTE_TYPE_QT)) {
			/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except GTK+ applications and Qt applications</i></small>" */
			exception = g_strdup_printf(_(", except %s and %s"),
						    _(note_exception[1]),
						    _(note_exception[2]));
		} else if (note_type == NOTE_TYPE_X) {
			/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except X applications</i></small>" */
			exception = g_strdup_printf(_(", except %s"),
						    _(note_exception[0]));
		} else if (note_type == NOTE_TYPE_GTK) {
			/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except GTK+ applications</i></small>" */
			exception = g_strdup_printf(_(", except %s"),
						    _(note_exception[1]));
		} else if (note_type == NOTE_TYPE_QT) {
			/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except Qt applications</i></small>" */
			exception = g_strdup_printf(_(", except %s"),
						    _(note_exception[2]));
		} else {
			exception = g_strdup("");
		}
		/* This will be displayed like "<small><i>Note: this change will not take effect until your next log in, except GTK+ applications</i></small>" */
		note_string = g_strdup_printf(_("<small><i>Note: this change will not take effect until your next log in%s</i></small>"), exception);
		gtk_label_set_markup(GTK_LABEL (note), note_string);
		if (show)
			gtk_widget_show(GTK_WIDGET (note));
		else
			gtk_widget_hide(GTK_WIDGET (note));

		g_free(exception);
		g_free(note_string);
	} else {
		g_set_error(&err, IMCHOOSEUI_GERROR, 0,
			    _("Unable to obtain the object for note"));
		goto bail;
	}
	tree = gtk_builder_get_object(builder, "tree");
	if (!tree) {
		g_set_error(&err, IMCHOOSEUI_GERROR, 0,
			    _("Unable to obtain the object for tree"));
		goto bail;
	}

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("",
							  renderer,
							  "pixbuf", POS_ICON,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (tree), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("",
							  renderer,
							  "markup", POS_LABEL,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (tree), column);
	renderer = imchoose_ui_cell_renderer_label_new();
	column = gtk_tree_view_column_new_with_attributes("",
							  renderer,
							  "widget", POS_PREFS,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (tree), column);
	g_signal_connect(renderer, "clicked",
			 G_CALLBACK (_imchoose_ui_label_clicked), tree);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	g_signal_connect(selection, "changed",
			 G_CALLBACK (_imchoose_ui_tree_selection_changed), tree);

	retval = GTK_WIDGET (g_object_ref(gtk_builder_get_object(builder, "root")));
	g_object_set_data(tree, "imchoose-ui", ui);
	_imchoose_ui_update_list(ui, GTK_WIDGET (tree), &err);
	if (err)
		goto bail;
  bail:
	g_object_unref(builder);
	g_free(uifile);
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		}
		g_warning("%s", err->message);
		g_error_free(err);
	}

	return retval;
}

GtkWidget *
imchoose_ui_get_progress_dialog(IMChooseUI  *ui,
				GError     **error)
{
	GtkBuilder *builder;
	GError *err = NULL;
	GtkWidget *retval = NULL, *label;
	gchar *uifile;

	g_return_val_if_fail (IMCHOOSE_IS_UI (ui), NULL);

	builder = gtk_builder_new();
	uifile = g_build_filename(UIDIR, "imchoose.ui", NULL);

#ifdef GNOME_ENABLE_DEBUG
	if (!g_file_test(uifile, G_FILE_TEST_EXISTS)) {
		/* fallback to uninstalled file */
		g_free(uifile);
		uifile = g_build_filename(BUILDDIR, "imchoose.ui", NULL);
	}
#endif /* GNOME_ENABLE_DEBUG */
	if (gtk_builder_add_from_file(builder, uifile, &err) == 0) {
		goto bail;
	}

	retval = GTK_WIDGET (gtk_builder_get_object(builder, "progress"));
	label = GTK_WIDGET (gtk_builder_get_object(builder, "progress_label"));
	g_object_set_qdata(G_OBJECT (retval), imchoose_ui_progress_label_quark(), label);
  bail:
	g_object_unref(builder);
	g_free(uifile);
	if (err) {
		if (error) {
			*error = g_error_copy(err);
		}
		g_warning("%s", err->message);
		g_error_free(err);
	}

	return retval;
}

gboolean
imchoose_ui_is_logout_required(IMChooseUI *ui)
{
	IMChooseUIPrivate *priv;

	g_return_val_if_fail (IMCHOOSE_IS_UI (ui), TRUE);

	priv = ui->priv;

	return priv->note_type < (NOTE_TYPE_ALL - 1);
}
