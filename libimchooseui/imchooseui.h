/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imchooseui.h
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
#ifndef __IM_CHOOSE_UI_H__
#define __IM_CHOOSE_UI_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IMCHOOSE_TYPE_UI		(imchoose_ui_get_type())
#define IMCHOOSE_UI(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), IMCHOOSE_TYPE_UI, IMChooseUI))
#define IMCHOOSE_UI_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), IMCHOOSE_TYPE_UI, IMChooseUIClass))
#define IMCHOOSE_IS_UI(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMCHOOSE_TYPE_UI))
#define IMCHOOSE_IS_UI_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), IMCHOOSE_TYPE_UI))
#define IMCHOOSE_UI_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), IMCHOOSE_TYPE_UI, IMChooseUIClass))


typedef struct _IMChooseUIClass		IMChooseUIClass;
typedef struct _IMChooseUI		IMChooseUI;
typedef struct _IMChooseUIPrivate	IMChooseUIPrivate;

struct _IMChooseUIClass {
	GObjectClass parent_class;

	void (* show_progress) (IMChooseUI  *ui,
				const gchar *message);
	void (* hide_progress) (IMChooseUI  *ui);
	void (* show_error)    (IMChooseUI  *ui,
				const gchar *summary,
				const gchar *details);
	void (* changed)       (IMChooseUI  *ui,
				const gchar *current_im,
				const gchar *initial_im);
};
struct _IMChooseUI {
	GObject            parent_instance;
	IMChooseUIPrivate *priv;
};

typedef enum {
	NOTE_TYPE_X = 1 << 0,
	NOTE_TYPE_GTK = 1 << 1,
	NOTE_TYPE_QT = 1 << 2,
	NOTE_TYPE_ALL = 1 << 3,
} NoteType;


GType       imchoose_ui_get_type            (void) G_GNUC_CONST;
GQuark      imchoose_ui_progress_label_quark(void);
IMChooseUI *imchoose_ui_new                 (void);
GtkWidget  *imchoose_ui_get                 (IMChooseUI  *ui,
					     GError     **error);
GtkWidget  *imchoose_ui_get_progress_dialog (IMChooseUI  *ui,
					     GError     **error);
gboolean    imchoose_ui_is_logout_required  (IMChooseUI  *ui);

G_END_DECLS

#endif /* __IM_CHOOSE_UI_H__ */
