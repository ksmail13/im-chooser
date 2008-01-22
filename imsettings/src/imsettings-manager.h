/* 
 * imsettings-manager.h
 * Copyright (C) 2008 Red Hat, Inc. All rights reserved.
 * 
 * Authors:
 *   Akira TAGOH  <tagoh@redhat.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __IMSETTINGS_IMSETTINGS_MANAGER_H__
#define __IMSETTINGS_IMSETTINGS_MANAGER_H__

#include <imsettings/imsettings-observer.h>

G_BEGIN_DECLS

#define IMSETTINGS_TYPE_MANAGER			(imsettings_manager_get_type())
#define IMSETTINGS_MANAGER(_o_)			(G_TYPE_CHECK_INSTANCE_CAST ((_o_), IMSETTINGS_TYPE_MANAGER, IMSettingsManager))
#define IMSETTINGS_MANAGER_CLASS(_c_)		(G_TYPE_CHECK_CLASS_CAST ((_c_), IMSETTINGS_TYPE_MANAGER, IMSettingsManagerClass))
#define IMSETTINGS_IS_MANAGER(_o_)		(G_TYPE_CHECK_INSTANCE_TYPE ((_o_), IMSETTINGS_TYPE_MANAGER))
#define IMSETTINGS_IS_MANAGER_CLASS(_c_)	(G_TYPE_CHECK_CLASS_TYPE ((_c_), IMSETTINGS_TYPE_MANAGER))
#define IMSETTINGS_MANAGER_GET_CLASS(_o_)	(G_TYPE_INSTANCE_GET_CLASS ((_o_), IMSETTINGS_TYPE_MANAGER, IMSettingsManagerClass))


typedef struct _IMSettingsManagerClass	IMSettingsManagerClass;
typedef struct _IMSettingsManager	IMSettingsManager;


struct _IMSettingsManagerClass {
	IMSettingsObserverClass parent_class;
};
struct _IMSettingsManager {
	IMSettingsObserver parent_instance;
};


GType              imsettings_manager_get_type (void) G_GNUC_CONST;
IMSettingsManager *imsettings_manager_new      (DBusGConnection   *connection,
						gboolean           replace);
void               imsettings_manager_load_conf(IMSettingsManager *manager);

G_END_DECLS

#endif /* __IMSETTINGS_IMSETTINGS_MANAGER_H__ */
