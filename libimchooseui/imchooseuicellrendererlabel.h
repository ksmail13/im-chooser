/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imchooseuicellrendererlabel.h
 * Copyright (C) 2011-2012 Red Hat, Inc. All rights reserved.
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
#ifndef __IM_CHOOSE_UI_CELL_RENDERER_LABEL_H__
#define __IM_CHOOSE_UI_CELL_RENDERER_LABEL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL		(imchoose_ui_cell_renderer_label_get_type())
#define IMCHOOSE_UI_CELL_RENDERER_LABEL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL, IMChooseUICellRendererLabel))
#define IMCHOOSE_UI_CELL_RENDERER_LABEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL, IMChooseUICellRendererLabelClass))
#define IMCHOOSE_IS_UI_CELL_RENDERER_LABEL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL))
#define IMCHOOSE_IS_UI_CELL_RENDERER_LABEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL))
#define IMCHOOSE_UI_CELL_RENDERER_LABEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL, IMChooseUICellRendererLabelClass))


typedef struct _IMChooseUICellRendererLabelClass	IMChooseUICellRendererLabelClass;
typedef struct _IMChooseUICellRendererLabel		IMChooseUICellRendererLabel;
typedef struct _IMChooseUICellRendererLabelPrivate	IMChooseUICellRendererLabelPrivate;

struct _IMChooseUICellRendererLabelClass {
	GtkCellRendererClass parent_class;

	void (* clicked) (IMChooseUICellRendererLabel *celllabel,
			  GdkEvent                    *event,
			  const gchar                 *path);
};
struct _IMChooseUICellRendererLabel {
	GtkCellRenderer                     parent_instance;
	IMChooseUICellRendererLabelPrivate *priv;
};

GType            imchoose_ui_cell_renderer_label_get_type(void) G_GNUC_CONST;
GtkCellRenderer *imchoose_ui_cell_renderer_label_new     (void);
void             imchoose_ui_cell_renderer_label_add     (IMChooseUICellRendererLabel *celllabel,
							  GtkWidget                   *widget);

G_END_DECLS

#endif /* __IM_CHOOSE_UI_CELL_RENDERER_LABEL_H__ */
