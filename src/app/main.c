/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * main.c
 * Copyright (C) 2006-2012 Red Hat, Inc. All rights reserved.
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
#include <stdlib.h>
#include "imchooseui.h"
#include <glib/gi18n.h>
#include "eggsmclient.h"

static guint progress_id = 0;

static gboolean
_activate_progress_cb(gpointer user_data)
{
	GtkWidget *progress = GTK_WIDGET (user_data);
	GtkWidget *parent = GTK_WIDGET (gtk_window_get_transient_for(GTK_WINDOW (progress)));

	gtk_widget_set_sensitive(parent, FALSE);
	gtk_widget_show(progress);

	return FALSE;
}

static void
_show_progress_cb(IMChooseUI  *ui,
		  const gchar *message,
		  gpointer     user_data)
{
	GtkWidget *progress = GTK_WIDGET (user_data);
	GtkWidget *label = g_object_get_qdata(G_OBJECT (progress),
					      imchoose_ui_progress_label_quark());

	gtk_label_set_markup(GTK_LABEL (label),
			     message);
	if (progress_id != 0)
		g_source_remove(progress_id);
	progress_id = g_timeout_add_seconds(2, _activate_progress_cb, progress);
}

static void
_hide_progress_cb(IMChooseUI *ui,
		  gpointer    user_data)
{
	GtkWidget *progress = GTK_WIDGET (user_data);
	GtkWidget *parent = GTK_WIDGET (gtk_window_get_transient_for(GTK_WINDOW (progress)));

	if (progress_id != 0) {
		g_source_remove(progress_id);
		progress_id = 0;
	}
	gtk_widget_hide(progress);
	gtk_widget_set_sensitive(parent, TRUE);
}

static void
_show_error_cb(IMChooseUI  *ui,
	       const gchar *summary,
	       const gchar *details,
	       gpointer     user_data)
{
	GtkWidget *parent = GTK_WIDGET (user_data);
	GtkWidget *dlg;
	gchar *s, *logfile, *m;

	if (progress_id != 0)
		g_source_remove(progress_id);

	s = g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s</span>", summary);
	dlg = gtk_message_dialog_new_with_markup(GTK_WINDOW (parent),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 s);
	logfile = g_build_filename(g_get_user_cache_dir(), "imsettings", "log", NULL);
	m = g_strdup_printf(_("Please check <a href=\"file://%s\">%s</a> for more details"),
			    logfile, logfile);
	gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG (dlg),
						   "%s\n\n%s",
						   details, m);
	g_free (m);
	g_free (logfile);

	/* for GNOME HIG compliance */
#if 0
	gtk_rc_parse_string("style \"imchoose-ui-message-dialog\" {\n"	\
			    "  GtkDialog::content-area-spacing = 14\n"	\
			    "  GtkDialog::content-area-border = 0\n"	\
			    "}\n"					\
			    "widget \"GtkMessageDialog\" style \"imchoose-ui-message-dialog\"");
#endif

	gtk_dialog_run(GTK_DIALOG (dlg));
	gtk_widget_destroy(dlg);
	g_free(s);
}

static void
_im_changed_cb(IMChooseUI  *ui,
	       const gchar *current_im,
	       const gchar *initial_im,
	       gpointer     user_data)
{
	GtkWidget *button = GTK_WIDGET (user_data);

	gtk_widget_set_sensitive(button, g_strcmp0(current_im, initial_im) != 0);
}

static void
_dialog_response_cb(GtkDialog *dialog,
		    gint       response_id,
		    gpointer   user_data)
{
	switch (response_id) {
	    case GTK_RESPONSE_DELETE_EVENT:
	    case GTK_RESPONSE_OK:
		    break;
	    case GTK_RESPONSE_APPLY:
		    G_STMT_START {
			    GtkWidget *d;

			    if (!egg_sm_client_end_session(EGG_SM_CLIENT_LOGOUT, TRUE)) {
				    d = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
							       GTK_BUTTONS_OK,
							       _("Could not connect to the session manager"));
				    gtk_dialog_run(GTK_DIALOG (d));
				    gtk_widget_destroy(d);
			    }
			    return;
		    } G_STMT_END;
		    break;
	    default:
		    g_warning("Unknown response id: %d", response_id);
		    return;
	}

	gtk_main_quit();
}

int
main(int    argc,
     char **argv)
{
	GOptionContext *ctx = g_option_context_new(_("[options...]"));
	GtkWidget *window, *content_widget, *widget, *progress;
	GtkWidget *close_button, *logout_button, *logout_image;
	GError *err = NULL;
	IMChooseUI *ui;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, IMCHOOSE_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

	setlocale(LC_ALL, "");

	g_type_init();

	g_option_context_add_group(ctx, gtk_get_option_group(FALSE));
	g_option_context_add_group(ctx, egg_sm_client_get_option_group());

	if (!g_option_context_parse(ctx, &argc, &argv, &err)) {
		g_printerr(_("Could not parse arguments: %s\n"), err->message);
		g_error_free(err);
		return 1;
	}
	if (!gtk_init_check(&argc, &argv)) {
		const char *display_name_arg = gdk_get_display_arg_name();
		if (display_name_arg == NULL)
			display_name_arg = getenv("DISPLAY");
		g_warning("cannot open display: %s", display_name_arg ? display_name_arg : "");
		return 1;
	}

	ui = imchoose_ui_new();
	window = gtk_dialog_new();
	widget = imchoose_ui_get(ui, &err);
	if (err) {
		if (err->code != 0) {
			_show_error_cb(ui, _("No input method is available"),
				       err->message, window);
		} else {
			_show_error_cb(ui, _("Unrecoverable error"),
				       err->message, window);
		}
		goto bail;
	}
	progress = imchoose_ui_get_progress_dialog(ui, &err);

	gtk_window_set_title(GTK_WINDOW (window), _("Input Method Selector"));
	gtk_container_set_border_width(GTK_CONTAINER (window), 4);

	if (imchoose_ui_is_logout_required(ui)) {
		close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
		logout_button = gtk_button_new_with_mnemonic(_("Log Out"));
		gtk_widget_set_sensitive(logout_button, FALSE);
		logout_image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON (logout_button), logout_image);
		gtk_dialog_add_action_widget(GTK_DIALOG (window), logout_button, GTK_RESPONSE_APPLY);

		g_signal_connect(ui, "changed",
				 G_CALLBACK (_im_changed_cb),
				 logout_button);
		gtk_widget_show(logout_button);
		gtk_widget_show(logout_image);
	} else {
		close_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	}
	gtk_dialog_add_action_widget(GTK_DIALOG (window), close_button, GTK_RESPONSE_OK);
	gtk_widget_show(close_button);

	content_widget = gtk_dialog_get_content_area(GTK_DIALOG (window));

	gtk_window_set_transient_for(GTK_WINDOW (progress),
				     GTK_WINDOW (window));
	gtk_box_pack_start(GTK_BOX (content_widget), widget, TRUE, TRUE, 0);

	g_signal_connect(window, "delete-event",
			 G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect(window, "response",
			 G_CALLBACK (_dialog_response_cb), NULL);
	g_signal_connect(ui, "show-progress",
			 G_CALLBACK (_show_progress_cb), progress);
	g_signal_connect(ui, "hide-progress",
			 G_CALLBACK (_hide_progress_cb), progress);
	g_signal_connect(ui, "show-error",
			 G_CALLBACK (_show_error_cb), window);

	gtk_widget_show(window);

	gtk_main();
  bail:
	g_object_unref(ui);
	g_option_context_free(ctx);

	return 0;
}
