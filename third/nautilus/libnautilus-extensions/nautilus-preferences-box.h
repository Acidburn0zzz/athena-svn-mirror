/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-prefs-box.h - Interface for preferences box component.

   Copyright (C) 1999, 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef NAUTILUS_PREFERENCES_BOX_H
#define NAUTILUS_PREFERENCES_BOX_H

#include <libgnomeui/gnome-dialog.h>
#include <gtk/gtkhbox.h>
#include <libnautilus-extensions/nautilus-preferences-pane.h>

BEGIN_GNOME_DECLS

#define NAUTILUS_TYPE_PREFS_BOX            (nautilus_preferences_box_get_type ())
#define NAUTILUS_PREFERENCES_BOX(obj)            (GTK_CHECK_CAST ((obj), NAUTILUS_TYPE_PREFS_BOX, NautilusPreferencesBox))
#define NAUTILUS_PREFERENCES_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_PREFS_BOX, NautilusPreferencesBoxClass))
#define NAUTILUS_IS_PREFS_BOX(obj)         (GTK_CHECK_TYPE ((obj), NAUTILUS_TYPE_PREFS_BOX))
#define NAUTILUS_IS_PREFS_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_PREFS_BOX))

typedef struct _NautilusPreferencesBox	   NautilusPreferencesBox;
typedef struct _NautilusPreferencesBoxClass      NautilusPreferencesBoxClass;
typedef struct _NautilusPreferencesBoxDetails    NautilusPreferencesBoxDetails;

struct _NautilusPreferencesBox
{
	/* Super Class */
	GtkHBox				hbox;

	/* Private stuff */
	NautilusPreferencesBoxDetails	*details;
};

struct _NautilusPreferencesBoxClass
{
	GtkHBoxClass	parent_class;

	void (*activate) (GtkWidget * prefs_box, gint entry_number);
};

GtkType    nautilus_preferences_box_get_type (void);
GtkWidget* nautilus_preferences_box_new      (const gchar            *box_title);
GtkWidget* nautilus_preferences_box_add_pane (NautilusPreferencesBox *prefs_box,
					      const gchar            *pane_title,
					      const gchar            *pane_description);
void       nautilus_preferences_box_update   (NautilusPreferencesBox *prefs_box);

END_GNOME_DECLS

#endif /* NAUTILUS_PREFERENCES_BOX_H */


