/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * main.c
 * Copyright (C) 2006-2008 Red Hat, Inc. All rights reserved.
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
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gnome.h>
#include "im-chooser-simple.h"

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

static void
_dialog_response_cb(GtkDialog *dialog,
		    gint       response_id,
		    gpointer   data)
{
	switch (response_id) {
	    case GTK_RESPONSE_DELETE_EVENT:
	    case GTK_RESPONSE_OK:
		    break;
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
	GnomeProgram *program;
	GtkWidget *logout_button, *logout_image;
	GtkWidget *window, *widget, *close_button;
	IMChooserSimple *im;
	gchar *iconfile;
	const gchar *xmodifiers, *gtk_immodule, *qt_immodule;
	guint note_type = 0;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, IMCHOOSE_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
	textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

	program = gnome_program_init("im-chooser", VERSION,
				     LIBGNOMEUI_MODULE,
				     argc, argv,
				     NULL);

	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW (window), _("IM Chooser - Input Method configuration tool"));
	gtk_window_set_resizable(GTK_WINDOW (window), FALSE);
	iconfile = g_build_filename(ICONDIR, "im-chooser.png", NULL);
	if (!g_file_test(iconfile, G_FILE_TEST_EXISTS)) {
		g_free(iconfile);
		iconfile = NULL;
		gtk_window_set_icon_name(GTK_WINDOW (window), "im-chooser");
	} else {
		gtk_window_set_icon_from_file(GTK_WINDOW (window), iconfile, NULL);
		g_free(iconfile);
	}
	gtk_container_set_border_width(GTK_CONTAINER (window), 4);
	gtk_container_set_border_width(GTK_CONTAINER (GTK_DIALOG (window)->vbox), 0);

	im = im_chooser_simple_new();
	xmodifiers = g_getenv("XMODIFIERS");
	gtk_immodule = g_getenv("GTK_IM_MODULE");
	qt_immodule = g_getenv("QT_IM_MODULE");
	if (xmodifiers != NULL && strcmp(xmodifiers, "@im=imsettings") == 0) {
		note_type |= NOTE_TYPE_X;
	}
	if (gtk_immodule == NULL ||
	    gtk_immodule[0] == 0 ||
	    (strcmp(gtk_immodule, "xim") == 0 && (note_type & NOTE_TYPE_X))) {
		note_type |= NOTE_TYPE_GTK;
	}
	if (qt_immodule == NULL ||
	    qt_immodule[0] == 0 ||
	    (strcmp(qt_immodule, "xim") == 0 && (note_type & NOTE_TYPE_X))) {
		note_type |= NOTE_TYPE_QT;
	}
	if (note_type < (NOTE_TYPE_ALL - 1)) {
		/* restarting is required to apply the changes for XIM at least.
		 * so we show up the logout button.
		 */
		close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
		logout_button = gtk_button_new_with_mnemonic(_("Log Out"));
		gtk_widget_set_sensitive(logout_button, FALSE);
		logout_image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON (logout_button), logout_image);
		gtk_dialog_add_action_widget(GTK_DIALOG (window), logout_button, GTK_RESPONSE_APPLY);

		g_signal_connect(im, "changed",
				 G_CALLBACK (_im_changed_cb), logout_button);
	} else {
		close_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	}
	g_object_set(im, "note_type", note_type, NULL);
	gtk_dialog_add_action_widget(GTK_DIALOG (window), close_button, GTK_RESPONSE_OK);
	gtk_dialog_set_has_separator(GTK_DIALOG (window), FALSE);

	g_signal_connect(im, "notify_n_im",
			 G_CALLBACK (_im_notify_n_im_cb), window);
	g_object_set(G_OBJECT (im), "parent_window", window, NULL);

	widget = im_chooser_simple_get_widget(im);

	gtk_widget_show_all(window);
	gtk_box_pack_start(GTK_BOX (GTK_DIALOG (window)->vbox), widget, TRUE, TRUE, 0);

	GTK_WIDGET_GET_CLASS (GTK_DIALOG (window))->style_set = _real_style_set;

	g_signal_connect(window, "response",
			 G_CALLBACK (_dialog_response_cb), im);

	gtk_main();

	g_object_unref(im);
	g_object_unref(program);

	return 0;
}
