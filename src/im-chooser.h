/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * im-chooser.h
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
#ifndef __IM_CHOOSER_H__
#define __IM_CHOOSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IM_TYPE_CHOOSER			(im_chooser_get_type())
#define IM_CHOOSER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), IM_TYPE_CHOOSER, IMChooser))
#define IM_CHOOSER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), IM_TYPE_CHOOSER, IMChooserClass))
#define IM_IS_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), IM_TYPE_CHOOSER))
#define IM_IS_CHOOSER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), IM_TYPE_CHOOSER))
#define IM_CHOOSER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), IM_TYPE_CHOOSER, IMChooserClass))


typedef struct _IMChooser	IMChooser;
typedef struct _IMChooserClass	IMChooserClass;

typedef enum {
	IM_MODE_SYSTEM = 0,
	IM_MODE_CUSTOM,
	IM_MODE_NEVER,
	IM_MODE_LEGACY,
} IMChooserMode;
typedef enum {
	IM_SUBMODE_UNKNOWN = 0,
	IM_SUBMODE_USER,
	IM_SUBMODE_SYMLINK,
} IMChooserSubMode;


GType          im_chooser_get_type        (void) G_GNUC_CONST;
IMChooser     *im_chooser_new             (void);
GtkWidget     *im_chooser_get_widget      (IMChooser        *im);
void           im_chooser_get_current_mode(IMChooser        *im,
					   IMChooserMode    *mode,
					   IMChooserSubMode *submode);
gboolean       im_chooser_update_xinputrc (IMChooser        *im,
					   IMChooserMode     mode,
					   IMChooserSubMode  submode,
					   const gchar      *xinputname);
gboolean       im_chooser_validate_mode   (IMChooser        *im,
					   IMChooserMode     mode,
					   IMChooserSubMode  submode);
gboolean       im_chooser_is_modified     (IMChooser        *im);

G_END_DECLS

#endif /* __IM_CHOOSER_H__ */
