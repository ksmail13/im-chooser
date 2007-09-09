/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * im-chooser-private.h
 * Copyright (C) 2007 Akira TAGOH
 * 
 * Authors:
 *   Akira TAGOH  <akira@tagoh.org>
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
#ifndef __IM_CHOOSER_PRIVATE_H__
#define __IM_CHOOSER_PRIVATE_H__

/* global config file */
#define IM_GLOBAL_XINPUT_CONF	"xinputrc"
/* user config file */
#define IM_USER_XINPUT_CONF	".xinputrc"
/* name that uses to be determined that IM is never used no matter what */
#define IM_NONE_NAME		"none"
/* name that uses to be determined that XIM is always used no matter what */
#define IM_XIM_NAME		"xim"
/* label that uses to indicate their own .xinputrc */
#define IM_USER_SPECIFIC_LABEL	_("User Specific")
/* label that uses to indicate "unknown" xinput script */
#define IM_UNKNOWN_LABEL	"Unknown"

#endif /* __IM_CHOOSER_PRIVATE_H__ */
