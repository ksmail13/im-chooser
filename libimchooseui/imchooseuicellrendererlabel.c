/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imchooseuicellrendererlabel.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include "imchooseui-marshal.h"
#include "imchooseuicellrendererlabel.h"


struct _IMChooseUICellRendererLabelPrivate {
	GtkWidget *child;
	gint       spacing;
};
enum {
	PROP_0,
	PROP_WIDGET,
	PROP_SPACING,
	PROP_END
};
enum {
	SIG_0,
	SIG_CLICKED,
	SIG_END
};

static guint signals[SIG_END] = { 0 };

G_DEFINE_TYPE (IMChooseUICellRendererLabel, imchoose_ui_cell_renderer_label, GTK_TYPE_CELL_RENDERER)

/*< private >*/
static void
_imchoose_ui_cell_renderer_label_finalize(GObject *object)
{
	if (G_OBJECT_CLASS (imchoose_ui_cell_renderer_label_parent_class)->finalize)
		G_OBJECT_CLASS (imchoose_ui_cell_renderer_label_parent_class)->finalize(object);
}

static void
_imchoose_ui_cell_renderer_label_get_property(GObject    *object,
					      guint       prop_id,
					      GValue     *value,
					      GParamSpec *pspec)
{
	IMChooseUICellRendererLabel *celllabel = IMCHOOSE_UI_CELL_RENDERER_LABEL (object);
	IMChooseUICellRendererLabelPrivate *priv = celllabel->priv;

	switch (prop_id) {
	    case PROP_WIDGET:
		    g_value_set_object(value, priv->child);
		    break;
	    case PROP_SPACING:
		    g_value_set_int(value, priv->spacing);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
_imchoose_ui_cell_renderer_label_set_property(GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec)
{
	GtkWidget *widget;
	IMChooseUICellRendererLabel *celllabel = IMCHOOSE_UI_CELL_RENDERER_LABEL (object);
	IMChooseUICellRendererLabelPrivate *priv = celllabel->priv;

	switch (prop_id) {
	    case PROP_WIDGET:
		    widget = GTK_WIDGET (g_value_get_object(value));
		    imchoose_ui_cell_renderer_label_add(IMCHOOSE_UI_CELL_RENDERER_LABEL (object),
							widget);
		    break;
	    case PROP_SPACING:
		    priv->spacing = g_value_get_int(value);
		    break;
	    default:
		    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		    break;
	}
}

static void
_imchoose_ui_cell_renderer_label_render(GtkCellRenderer      *cell,
					cairo_t              *cr,
					GtkWidget            *widget,
					const GdkRectangle   *background_area,
					const GdkRectangle   *cell_area,
					GtkCellRendererState  flags)
{
	IMChooseUICellRendererLabel *celllabel = IMCHOOSE_UI_CELL_RENDERER_LABEL (cell);
	IMChooseUICellRendererLabelPrivate *priv = celllabel->priv;

	if (priv->child) {
		PangoLayout *layout;
		GtkStyleContext *context;
		gint xpad, ypad;
		GdkRGBA selected_label_color;

		layout = gtk_label_get_layout(GTK_LABEL (priv->child));
		context = gtk_widget_get_style_context(widget);
		gtk_cell_renderer_get_padding(cell, &xpad, &ypad);

		if ((flags & GTK_CELL_RENDERER_SELECTED) != 0) {
			gdk_cairo_rectangle(cr, background_area);
			gtk_style_context_get_background_color(context,
							       GTK_STATE_FLAG_NORMAL,
							       &selected_label_color);
			gdk_cairo_set_source_rgba(cr, &selected_label_color);
			cairo_fill(cr);
		}
		cairo_save(cr);

		gdk_cairo_rectangle(cr, cell_area);
		cairo_clip(cr);

		gtk_render_layout(context, cr,
				  cell_area->x + xpad + priv->spacing,
				  cell_area->y + ypad,
				  layout);

		cairo_restore(cr);
	}
}

static void
_imchoose_ui_cell_renderer_label_get_preferred_width(GtkCellRenderer *cell,
						     GtkWidget       *widget,
						     gint            *minimum_size,
						     gint            *natural_size)
{
	IMChooseUICellRendererLabel *celllabel = IMCHOOSE_UI_CELL_RENDERER_LABEL (cell);
	IMChooseUICellRendererLabelPrivate *priv = celllabel->priv;
	gint min_size = 0, nat_size = 0;

	if (priv->child) {
		GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS (priv->child);

		widget_class->get_preferred_width(priv->child, &min_size, &nat_size);
	}
	if (minimum_size)
		*minimum_size = min_size + priv->spacing;
	if (natural_size)
		*natural_size = nat_size + priv->spacing;
}

static void
_imchoose_ui_cell_renderer_label_get_preferred_height(GtkCellRenderer *cell,
						      GtkWidget       *widget,
						      gint            *minimum_size,
						      gint            *natural_size)
{
	IMChooseUICellRendererLabel *celllabel = IMCHOOSE_UI_CELL_RENDERER_LABEL (cell);
	IMChooseUICellRendererLabelPrivate *priv = celllabel->priv;
	gint min_size = 0, nat_size = 0;

	if (priv->child) {
		GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS (priv->child);

		widget_class->get_preferred_height(priv->child, &min_size, &nat_size);
	}
	if (minimum_size)
		*minimum_size = min_size;
	if (natural_size)
		*natural_size = nat_size;
}

static gboolean
_imchoose_ui_cell_renderer_label_activate(GtkCellRenderer      *cell,
					  GdkEvent             *event,
					  GtkWidget            *widget,
					  const gchar          *path,
					  const GdkRectangle   *background_area,
					  const GdkRectangle   *cell_area,
					  GtkCellRendererState  flags)
{
	IMChooseUICellRendererLabel *celllabel = IMCHOOSE_UI_CELL_RENDERER_LABEL (cell);
	IMChooseUICellRendererLabelPrivate *priv = celllabel->priv;
	gint width = 0, height = 0;

	gtk_cell_renderer_get_preferred_width(cell, widget, NULL, &width);
	gtk_cell_renderer_get_preferred_height(cell, widget, NULL, &height);
	if (event) {
		event->button.x -= (cell_area->x + priv->spacing);
		event->button.y -= cell_area->y;
		width -= priv->spacing;
		if (event->button.x >= 0 && event->button.x <= width &&
		    event->button.y >= 0 && event->button.y <= height) {
			event->button.x = 0;
			event->button.y = 0;
			g_signal_emit(cell, signals[SIG_CLICKED], 0, event, path);
		}
	}

	return TRUE;
}

static void
imchoose_ui_cell_renderer_label_class_init(IMChooseUICellRendererLabelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

	g_type_class_add_private(object_class,
				 sizeof (IMChooseUICellRendererLabelPrivate));

	object_class->finalize     = _imchoose_ui_cell_renderer_label_finalize;
	object_class->get_property = _imchoose_ui_cell_renderer_label_get_property;
	object_class->set_property = _imchoose_ui_cell_renderer_label_set_property;

	cell_class->render               = _imchoose_ui_cell_renderer_label_render;
	cell_class->get_preferred_width  = _imchoose_ui_cell_renderer_label_get_preferred_width;
	cell_class->get_preferred_height = _imchoose_ui_cell_renderer_label_get_preferred_height;
	cell_class->activate             = _imchoose_ui_cell_renderer_label_activate;

	g_object_class_install_property(object_class,
					PROP_WIDGET,
					g_param_spec_object("widget",
							    _("Widget"),
							    _("Widget to contain in the cell"),
							    GTK_TYPE_WIDGET,
							    G_PARAM_READWRITE));
	g_object_class_install_property(object_class,
					PROP_SPACING,
					g_param_spec_int("spacing",
							 _("Spacing"),
							 _("The amount of space between label"),
							 0,
							 G_MAXINT,
							 0,
							 G_PARAM_READWRITE));

	signals[SIG_CLICKED] = g_signal_new("clicked",
					    G_OBJECT_CLASS_TYPE (object_class),
					    G_SIGNAL_RUN_LAST,
					    G_STRUCT_OFFSET (IMChooseUICellRendererLabelClass, clicked),
					    NULL, NULL,
					    imchoose_ui_marshal_VOID__BOXED_STRING,
					    G_TYPE_NONE, 2,
					    GDK_TYPE_EVENT,
					    G_TYPE_STRING);
}

static void
imchoose_ui_cell_renderer_label_init(IMChooseUICellRendererLabel *celllabel)
{
	celllabel->priv = G_TYPE_INSTANCE_GET_PRIVATE (celllabel,
						       IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL,
						       IMChooseUICellRendererLabelPrivate);

	g_object_set(celllabel, "mode",
		     GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
	celllabel->priv->spacing = 10;
}

/*< public >*/
GtkCellRenderer *
imchoose_ui_cell_renderer_label_new(void)
{
	return g_object_new(IMCHOOSE_TYPE_UI_CELL_RENDERER_LABEL, NULL);
}

void
imchoose_ui_cell_renderer_label_add(IMChooseUICellRendererLabel *celllabel,
				    GtkWidget                   *widget)
{
	IMChooseUICellRendererLabelPrivate *priv;

	g_return_if_fail (IMCHOOSE_IS_UI_CELL_RENDERER_LABEL (celllabel));
	g_return_if_fail (GTK_IS_WIDGET (widget));

	priv = celllabel->priv;
	if (priv->child) {
		g_object_unref(priv->child);
	}
	priv->child = g_object_ref(widget);
}
