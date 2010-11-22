/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * imchoose-module.c
 * Copyright (C) 2010 Akira TAGOH
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

#include <libgnome-control-center/cc-panel.h>
#include <glib/gi18n.h>
#include "im-chooser-simple.h"

#define CC_TYPE_IMCHOOSE_PANEL			cc_imchoose_panel_get_type()
#define CC_IMCHOOSE_PANEL(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj)), CC_TYPE_IMCHOOSE_PANEL, CcIMChoosePanel)
#define CC_IMCHOOSE_PANEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CC_TYPE_IMCHOOSE_PANEL, CcIMChoosePanelClass))
#define CC_IS_IMCHOOSE_PANEL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CC_TYPE_IMCHOOSE_PANEL))
#define CC_IS_IMCHOOSE_PANEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CC_TYPE_IMCHOOSE_PANEL))
#define CC_IMCHOOSE_PANEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CC_TYPE_IMCHOOSE_PANEL, CcIMChoosePanelClass))

typedef struct _CcIMChoosePanel			CcIMChoosePanel;
typedef struct _CcIMChoosePanelClass		CcIMChoosePanelClass;

struct _CcIMChoosePanel {
	CcPanel          parent;
	IMChooserSimple *im;
	GtkWidget       *imchooseui;
};
struct _CcIMChoosePanelClass {
	CcPanelClass parent_class;
};

GType cc_imchoose_panel_get_type(void) G_GNUC_CONST;

G_DEFINE_DYNAMIC_TYPE (CcIMChoosePanel, cc_imchoose_panel, CC_TYPE_PANEL)

/*< private >*/
static void
cc_imchoose_panel_real_dispose(GObject *object)
{
	G_OBJECT_CLASS (cc_imchoose_panel_parent_class)->dispose (object);
}

static void
cc_imchoose_panel_real_finalize(GObject *object)
{
	G_OBJECT_CLASS (cc_imchoose_panel_parent_class)->finalize (object);
}

static void
cc_imchoose_panel_class_init(CcIMChoosePanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = cc_imchoose_panel_real_dispose;
	object_class->finalize = cc_imchoose_panel_real_finalize;
}

static void
cc_imchoose_panel_class_finalize(CcIMChoosePanelClass *klass)
{
}

static void
cc_imchoose_panel_init(CcIMChoosePanel *self)
{
	g_object_set(G_OBJECT (im), "parent_window", GTK_WIDGET (self), NULL);
	self->im = im_chooser_simple_new();
	self->imchooseui = im_chooser_simple_get_widget(self->im);
	gtk_container_add(GTK_CONTAINER (self), self->imchooseui);
}

/*< public >*/
void
g_io_module_load(GIOModule *module)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, IMCHOOSE_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* HAVE_BIND_TEXTDOMAIN_CODESET */
#endif /* ENABLE_NLS */

	cc_imchoose_panel_register_type (G_TYPE_MODULE (module));
	g_io_extension_point_implement(CC_SHELL_PANEL_EXTENSION_POINT,
				       CC_TYPE_IMCHOOSE_PANEL,
				       "im-chooser-panel", 0);
}

void
g_io_module_unload(GIOModule *module)
{
}
