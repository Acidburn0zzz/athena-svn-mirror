/*
 *  fm-bonobo-provider.h - Bonobo API support
 * 
 *  Copyright (C) 2002 James Willcox
 *                2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Dave Camp <dave@ximian.com>
 *           James Willcox <jwillcox@gnome.org>
 * 
 */

#ifndef FM_BONOBO_PROVIDER_H
#define FM_BONOBO_PROVIDER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define FM_TYPE_BONOBO_PROVIDER  (fm_bonobo_provider_get_type ())
#define FM_BONOBO_PROVIDER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), FM_TYPE_BONOBO_PROVIDER, FMBonoboProvider))
#define FM_IS_BONOBO_PROVIDER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), FM_TYPE_BONOBO_PROVIDER))
typedef struct _FMBonoboProvider       FMBonoboProvider;
typedef struct _FMBonoboProviderClass  FMBonoboProviderClass;

struct _FMBonoboProvider {
	GObject parent_slot;
};

struct _FMBonoboProviderClass {
	GObjectClass parent_slot;
};

GType fm_bonobo_provider_get_type      (void);

G_END_DECLS

#endif
