/* 
 * xinput.h
 * Copyright (C) 2006 Red Hat, Inc. All rights reserved.
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
#ifndef __XINPUT_H__
#define __XINPUT_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	XINPUT_VALUE_XIM,
	XINPUT_VALUE_GTKIMM,
	XINPUT_VALUE_QTIMM,
	XINPUT_VALUE_IGNORE_ME,
} XInputValueType;

typedef struct _XInputData	XInputData;

GSList     *xinput_get_im_list   (const gchar     *path);
GHashTable *xinput_get_lang_table(const gchar     *path);
XInputData *xinput_data_new      (const gchar     *file);
void        xinput_data_free     (gpointer         data);
gpointer    xinput_data_get_value(XInputData      *xinput,
				  XInputValueType  type);

G_END_DECLS

#endif /* __XINPUT_H__ */
