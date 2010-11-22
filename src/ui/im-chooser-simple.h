/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * im-chooser-simple.h
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
#ifndef __IM_CHOOSER_SIMPLE_H__
#define __IM_CHOOSER_SIMPLE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IM_TYPE_CHOOSER_SIMPLE			(im_chooser_simple_get_type())
#define IM_CHOOSER_SIMPLE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), IM_TYPE_CHOOSER_SIMPLE, IMChooserSimple))
#define IM_CHOOSER_SIMPLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), IM_TYPE_CHOOSER_SIMPLE, IMChooserSimpleClass))
#define IM_IS_CHOOSER_SIMPLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), IM_TYPE_CHOOSER_SIMPLE))
#define IM_IS_CHOOSER_SIMPLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), IM_TYPE_CHOOSER_SIMPLE))
#define IM_CHOOSER_SIMPLE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), IM_TYPE_CHOOSER_SIMPLE, IMChooserSimpleClass))


typedef struct _IMChooserSimple		IMChooserSimple;
typedef struct _IMChooserSimpleClass	IMChooserSimpleClass;

typedef enum {
	NOTE_TYPE_X = 1 << 0,
	NOTE_TYPE_GTK = 1 << 1,
	NOTE_TYPE_QT = 1 << 2,
	NOTE_TYPE_ALL = 1 << 3,
} NoteType;


GType            im_chooser_simple_get_type   (void) G_GNUC_CONST;
IMChooserSimple *im_chooser_simple_new        (void);
GtkWidget       *im_chooser_simple_get_widget (IMChooserSimple *im);
gboolean         im_chooser_simple_is_modified(IMChooserSimple *im);

G_END_DECLS

#endif /* __IM_CHOOSER_SIMPLE_H__ */
