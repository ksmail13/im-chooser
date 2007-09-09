/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * main.c
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
#include <glib/gi18n.h>
#ifdef USE_GNOME
#include <gnome.h>
#endif /* USE_GNOME */
#include "im-chooser.h"
#include "im-chooser-simple.h"

#ifdef USE_GNOME
#ifdef USE_OLD_UI
static void
_im_changed_cb(IMChooser *im,
	       gpointer   data)
{
	GtkWidget *button = GTK_WIDGET (data);

	gtk_widget_set_sensitive(button, im_chooser_is_modified(im));
}
#else
static void
_im_changed_cb(IMChooserSimple *im,
	       gpointer         data)
{
	GtkWidget *button = GTK_WIDGET (data);

	gtk_widget_set_sensitive(button, im_chooser_simple_is_modified(im));
}

static void
_im_notify_n_im_cb(IMChooserSimple *im,
		   guint            n,
		   gpointer         data)
{
	GtkWidget *window = GTK_WIDGET (data);

	if (n == 0) {
		GtkWidget *dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW (window),
								       GTK_DIALOG_MODAL,
								       GTK_MESSAGE_ERROR,
								       GTK_BUTTONS_OK,
								       _("<span weight=\"bold\" size=\"larger\">No input method is available</span>"));

		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog),
							 _("Please install any input methods before running if you like."));
		g_signal_connect(dialog, "response",
				 G_CALLBACK (gtk_true), NULL);

		while (g_main_context_pending(NULL))
			g_main_context_iteration(NULL, TRUE);

		gtk_dialog_run(GTK_DIALOG (dialog));
		gtk_widget_destroy(dialog);

		exit(1);
	}
}

#endif /* USE_OLD_UI */

static void
_real_style_set(GtkWidget *widget,
		GtkStyle  *prev_style)
{
	GtkDialog *dialog = GTK_DIALOG (widget);

	gtk_container_set_border_width (GTK_CONTAINER (dialog->vbox),
					5);
	gtk_box_set_spacing (GTK_BOX (dialog->action_area),
			     10);
	gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area),
					5);
}
#endif /* USE_GNOME */

static void
_dialog_response_cb(GtkDialog *dialog,
		    gint       response_id,
		    gpointer   data)
{
	switch (response_id) {
	    case GTK_RESPONSE_DELETE_EVENT:
	    case GTK_RESPONSE_OK:
		    break;
#ifdef USE_GNOME
	    case GTK_RESPONSE_APPLY:
		    G_STMT_START {
			    GnomeClient *client;

			    if ((client = gnome_master_client()) == NULL) {
				    g_warning("Failed to get the master client instance.");
				    return;
			    }
			    gnome_client_request_save(client,
						      GNOME_SAVE_GLOBAL,
						      TRUE,
						      GNOME_INTERACT_ANY,
						      FALSE,
						      TRUE);
			    return;
		    } G_STMT_END;
		    break;
#endif /* USE_GNOME */
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
	GtkWidget *window, *widget, *close_button;
#ifdef USE_OLD_UI
	IMChooser *im;
#else
	IMChooserSimple *im;
#endif /* USE_OLD_UI */
#ifdef USE_GNOME
	GnomeProgram *program;
	GtkWidget *logout_button, *logout_image;
#endif /* USE_GNOME */
	gchar *iconfile;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, IMCHOOSE_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

#ifdef USE_GNOME
	program = gnome_program_init("im-chooser", VERSION,
				     LIBGNOMEUI_MODULE,
				     argc, argv,
				     NULL);
#else
	gtk_init(&argc, &argv);
#endif /* USE_GNOME */

	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW (window), _("IM Chooser - Input Method configuration tool"));
	gtk_window_set_resizable(GTK_WINDOW (window), FALSE);
	iconfile = g_build_filename(ICONDIR, "im-chooser.png", NULL);
	gtk_window_set_icon_from_file(GTK_WINDOW (window), iconfile, NULL);
	gtk_container_set_border_width(GTK_CONTAINER (window), 4);
	gtk_container_set_border_width(GTK_CONTAINER (GTK_DIALOG (window)->vbox), 0);
#ifdef USE_GNOME
	close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	logout_button = gtk_button_new_with_mnemonic(_("_Log Out"));
	logout_image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (logout_button), logout_image);
	gtk_dialog_add_action_widget(GTK_DIALOG (window), logout_button, GTK_RESPONSE_APPLY);
#else
	close_button = gtk_button_new_from_stock(GTK_STOCK_OK);
#endif /* USE_GNOME */
	gtk_dialog_add_action_widget(GTK_DIALOG (window), close_button, GTK_RESPONSE_OK);
	gtk_dialog_set_has_separator(GTK_DIALOG (window), FALSE);

#ifdef USE_OLD_UI
	im = im_chooser_new();
	widget = im_chooser_get_widget(im);
#else
	im = im_chooser_simple_new();
	g_signal_connect(im, "notify_n_im",
			 G_CALLBACK (_im_notify_n_im_cb), window);

	widget = im_chooser_simple_get_widget(im);
#endif /* USE_OLD_UI */

	gtk_widget_show_all(window);
	gtk_box_pack_start(GTK_BOX (GTK_DIALOG (window)->vbox), widget, TRUE, TRUE, 0);

#ifdef USE_GNOME
	GTK_WIDGET_GET_CLASS (GTK_DIALOG (window))->style_set = _real_style_set;
	gtk_widget_set_sensitive(logout_button, FALSE);

	g_signal_connect(im, "changed",
			 G_CALLBACK (_im_changed_cb), logout_button);
#endif /* USE_GNOME */

	g_signal_connect(window, "response",
			 G_CALLBACK (_dialog_response_cb), im);

	gtk_main();

	g_object_unref(im);
#ifdef USE_GNOME
	g_object_unref(program);
#endif

	return 0;
}
