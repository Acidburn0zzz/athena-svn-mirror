/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-prefs-group-radio.c - Radio button prefs group implementation.

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

#include <config.h>
#include "nautilus-preferences-group.h"
#include "nautilus-gtk-extensions.h"

#include <gnome.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtksignal.h>
#include "nautilus-gtk-macros.h"

/* Signals */
typedef enum
{
	CHANGED,
	LAST_SIGNAL
} RadioGroupSignals;

typedef struct
{
	GtkWidget	*radio_button;
} ButtonInfo;

struct _NautilusPreferencesGroupDetails
{
	GtkWidget	*main_box;
	GtkWidget	*content_box;
	GtkWidget	*description_label;
	gboolean	show_description;

	GList		*items;
};

static const gint PREFERENCES_GROUP_NOT_FOUND = -1;

/* NautilusPreferencesGroupClass methods */
static void        nautilus_preferences_group_initialize_class (NautilusPreferencesGroupClass *klass);
static void        nautilus_preferences_group_initialize       (NautilusPreferencesGroup      *preferences_group);


/* GtkObjectClass methods */
static void        nautilus_preferences_group_destroy          (GtkObject                    *object);


/* Private stuff */
static void        preferences_group_construct                 (NautilusPreferencesGroup *prefs_group,
								const gchar * title);

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusPreferencesGroup,
				   nautilus_preferences_group,
				   GTK_TYPE_FRAME);

/*
 * NautilusPreferencesGroupClass methods
 */
static void
nautilus_preferences_group_initialize_class (NautilusPreferencesGroupClass *preferences_group_class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = GTK_OBJECT_CLASS (preferences_group_class);
	widget_class = GTK_WIDGET_CLASS (preferences_group_class);

 	parent_class = gtk_type_class (gtk_frame_get_type ());
	
	/* GtkObjectClass */
	object_class->destroy = nautilus_preferences_group_destroy;
}

static void
nautilus_preferences_group_initialize (NautilusPreferencesGroup *group)
{
	group->details = g_new (NautilusPreferencesGroupDetails, 1);

	group->details->main_box = NULL;
	group->details->content_box = NULL;
	group->details->description_label = NULL;
	group->details->show_description = FALSE;
	group->details->items = NULL;
}

/*
 * GtkObjectClass methods
 */
static void
nautilus_preferences_group_destroy(GtkObject* object)
{
	NautilusPreferencesGroup *group;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (NAUTILUS_IS_PREFERENCES_GROUP (object));
	
	group = NAUTILUS_PREFERENCES_GROUP (object);

	g_list_free (group->details->items);
	g_free (group->details);
	
	/* Chain */
	if (GTK_OBJECT_CLASS (parent_class)->destroy != NULL)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/*
 * Private stuff
 */
static void
preferences_group_construct (NautilusPreferencesGroup *group,
			     const gchar * title)
{
	g_assert (group != NULL);
	g_assert (title != NULL);

	g_assert (group->details->content_box == NULL);
	g_assert (group->details->main_box == NULL);
	g_assert (group->details->description_label == NULL);

	/* Ourselves */
	gtk_frame_set_shadow_type (GTK_FRAME (group),
				   GTK_SHADOW_ETCHED_IN);

	gtk_frame_set_label (GTK_FRAME (group), title);

	/* Main box */
	group->details->main_box = gtk_vbox_new (FALSE, 0);

	gtk_container_add (GTK_CONTAINER (group),
			   group->details->main_box);

	/* Description label */
	group->details->description_label = gtk_label_new ("Blurb");

	gtk_label_set_justify (GTK_LABEL (group->details->description_label),
			       GTK_JUSTIFY_LEFT);

	gtk_box_pack_start (GTK_BOX (group->details->main_box),
			    group->details->description_label,
			    FALSE,
			    FALSE,
			    0);

	if (group->details->show_description)
	{
		gtk_widget_show (group->details->description_label);
	}

	/* Content box */
	group->details->content_box = 
		gtk_vbox_new (FALSE, 0);

	gtk_box_pack_start (GTK_BOX (group->details->main_box),
			    group->details->content_box,
			    FALSE,
			    FALSE,
			    0);

	gtk_container_set_border_width (GTK_CONTAINER (group->details->content_box),
					4);

	gtk_widget_show (group->details->content_box);
	gtk_widget_show (group->details->main_box);
}

/*
 * NautilusPreferencesGroup public methods
 */
GtkWidget *
nautilus_preferences_group_new (const gchar *title)
{
	NautilusPreferencesGroup *group;

	g_return_val_if_fail (title != NULL, NULL);

	group = NAUTILUS_PREFERENCES_GROUP
		(gtk_widget_new (nautilus_preferences_group_get_type (), NULL));

	preferences_group_construct (group, title);
	
	return GTK_WIDGET (group);
}

GtkWidget *
nautilus_preferences_group_add_item (NautilusPreferencesGroup		*group,
				     const char				*preference_name,
				     NautilusPreferencesItemType	item_type)
{
	GtkWidget *item;

	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (NAUTILUS_IS_PREFERENCES_GROUP (group), NULL);

	g_return_val_if_fail (preference_name != NULL, NULL);

	item = nautilus_preferences_item_new (preference_name, item_type);

	group->details->items = g_list_append (group->details->items, item);
	
	gtk_box_pack_start (GTK_BOX (group->details->content_box),
			    item,
			    TRUE,
			    TRUE,
			    0);

	gtk_widget_show (item);

	return item;
}

void
nautilus_preferences_group_update (NautilusPreferencesGroup *group)
{
	GList *iterator;
	NautilusPreferencesItem *preferences_item;
	gboolean shown;
	char *name = NULL;

	g_return_if_fail (NAUTILUS_IS_PREFERENCES_GROUP (group));
	
	for (iterator = group->details->items; iterator != NULL; iterator = iterator->next) {
		preferences_item = NAUTILUS_PREFERENCES_ITEM (iterator->data);

		name = nautilus_preferences_item_get_name (preferences_item);

		nautilus_preferences_item_update_displayed_value (preferences_item);

		if (nautilus_preferences_is_visible (name)) {
			shown = nautilus_preferences_item_get_control_showing (preferences_item);
		} else {
			shown = FALSE;
		}
		
		nautilus_gtk_widget_set_shown (GTK_WIDGET (iterator->data), shown);
		
		g_free (name);
	}
}

guint
nautilus_preferences_group_get_num_visible_items (const NautilusPreferencesGroup *group)
{
	guint n = 0;
	GList *iterator;

	g_return_val_if_fail (NAUTILUS_IS_PREFERENCES_GROUP (group), 0);

	for (iterator = group->details->items; iterator != NULL; iterator = iterator->next) {
		char *name;
		name = nautilus_preferences_item_get_name (NAUTILUS_PREFERENCES_ITEM (iterator->data));
		if (nautilus_preferences_is_visible (name)) {
			n++;
		}
		g_free (name);
	}

	return n;
}
