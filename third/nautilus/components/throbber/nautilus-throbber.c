/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 * Nautilus
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Nautilus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 *
 * This is the throbber (for busy feedback) for the location bar
 *
 */

#include <config.h>
#include "nautilus-throbber.h"

#include <bonobo/bonobo-ui-toolbar-item.h>
#include <eel/eel-debug.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-graphic-effects.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-accessibility.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-macros.h>
#include <libgnome/gnome-util.h>
#include <libnautilus/nautilus-view-standard-main.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-icon-factory.h>
#include <libnautilus-private/nautilus-theme.h>
#include <math.h>

#define THROBBER_DEFAULT_TIMEOUT 100	/* Milliseconds Per Frame */

struct NautilusThrobberDetails {
	BonoboObject *control;
	BonoboPropertyBag *property_bag;
	GList	*image_list;

	GdkPixbuf *quiescent_pixbuf;
	
	int	max_frame;
	int	delay;
	int	current_frame;	
	guint	timer_task;
	
	gboolean ready;
	gboolean small_mode;

	gboolean button_in;
	gboolean button_down;
};


static void nautilus_throbber_load_images            (NautilusThrobber *throbber);
static void nautilus_throbber_unload_images          (NautilusThrobber *throbber);
static void nautilus_throbber_theme_changed          (gpointer          user_data);
static void nautilus_throbber_remove_update_callback (NautilusThrobber *throbber);
static AtkObject *nautilus_throbber_get_accessible   (GtkWidget *widget);

GNOME_CLASS_BOILERPLATE (NautilusThrobber, nautilus_throbber,
			 GtkEventBox, GTK_TYPE_EVENT_BOX)



/* routines to handle setting and getting the configuration properties of the Bonobo control */

enum {
	STYLE,
	THROBBING,
	LOCATION
} MyArgs;


static gboolean
is_throbbing (NautilusThrobber *throbber)
{
	return throbber->details->timer_task != 0;
}

static void
get_bonobo_properties (BonoboPropertyBag *bag,
			BonoboArg *arg,
			guint arg_id,
			CORBA_Environment *ev,
			gpointer user_data)
{
	NautilusThrobber *throbber = NAUTILUS_THROBBER (user_data);

	switch (arg_id) {
	case THROBBING:
	{
		BONOBO_ARG_SET_BOOLEAN (arg, throbber->details->timer_task != 0);
		break;
	}

	case LOCATION:
	{
		char *location = nautilus_theme_get_theme_data ("throbber", "url");
		if (location != NULL) {
			BONOBO_ARG_SET_STRING (arg, location);
			g_free (location);
		} else {
			BONOBO_ARG_SET_STRING (arg, "");			
		}
		
	}

		default:
			g_warning ("Unhandled arg %d", arg_id);
			break;
	}
}

static void
set_bonobo_properties (BonoboPropertyBag *bag,
			const BonoboArg *arg,
			guint arg_id,
			CORBA_Environment *ev,
			gpointer user_data)
{
	NautilusThrobber *throbber = NAUTILUS_THROBBER (user_data);
	switch (arg_id) {
	case THROBBING:
	{
		gboolean throbbing;
		
		throbbing = BONOBO_ARG_GET_BOOLEAN (arg);
		
		if (throbbing != is_throbbing (throbber)) {
				if (throbbing) {
					nautilus_throbber_start (throbber);
				} else {
					nautilus_throbber_stop (throbber);
				}
		}
		
		break;
	}
	case STYLE:
	{
		nautilus_throbber_set_small_mode (throbber, BONOBO_ARG_GET_INT (arg) !=
						  BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;
	}
		default:
			g_warning ("Unhandled arg %d", arg_id);
			break;
	}
}

BonoboObject *
nautilus_throbber_get_control (NautilusThrobber *throbber)
{
	return throbber->details->control;
}

/* loop through all the images taking their union to compute the width and height of the throbber */
static void
get_throbber_dimensions (NautilusThrobber *throbber, int *throbber_width, int* throbber_height)
{
	int current_width, current_height;
	int pixbuf_width, pixbuf_height;
	GList *current_entry;
	GdkPixbuf *pixbuf;
	
	/* start with the quiescent image */
	current_width = gdk_pixbuf_get_width (throbber->details->quiescent_pixbuf);
	current_height = gdk_pixbuf_get_height (throbber->details->quiescent_pixbuf);

	/* loop through all the installed images, taking the union */
	current_entry = throbber->details->image_list;
	while (current_entry != NULL) {	
		pixbuf = GDK_PIXBUF (current_entry->data);
		pixbuf_width = gdk_pixbuf_get_width (pixbuf);
		pixbuf_height = gdk_pixbuf_get_height (pixbuf);
		
		if (pixbuf_width > current_width) {
			current_width = pixbuf_width;
		}
		
		if (pixbuf_height > current_height) {
			current_height = pixbuf_height;
		}
		
		current_entry = current_entry->next;
	}
		
	/* return the result */
	*throbber_width = current_width;
	*throbber_height = current_height;
}

static void
nautilus_throbber_instance_init (NautilusThrobber *throbber)
{
	char *delay_str;
	GtkWidget *widget = GTK_WIDGET (throbber);
	
	
	GTK_WIDGET_UNSET_FLAGS (throbber, GTK_NO_WINDOW);

	gtk_widget_set_events (widget, 
			       gtk_widget_get_events (widget)
			       | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
	
	throbber->details = g_new0 (NautilusThrobberDetails, 1);
	
	/* set up the delay from the theme */
	delay_str = nautilus_theme_get_theme_data ("throbber", "delay");
	
	if (delay_str) {
		throbber->details->delay = atoi (delay_str);
		g_free (delay_str);
	} else {
		throbber->details->delay = THROBBER_DEFAULT_TIMEOUT;		
	}
	
	/* make the bonobo control */
	throbber->details->control = BONOBO_OBJECT (bonobo_control_new (widget));
	eel_add_weak_pointer (&throbber->details->control);
	
	/* attach a property bag with the configure property */
	throbber->details->property_bag = bonobo_property_bag_new (get_bonobo_properties, 
								   set_bonobo_properties, throbber);
	bonobo_control_set_properties (BONOBO_CONTROL (throbber->details->control), 
				       BONOBO_OBJREF (throbber->details->property_bag), NULL);

	bonobo_property_bag_add (throbber->details->property_bag, "throbbing", THROBBING, BONOBO_ARG_BOOLEAN, NULL,
				 "Throbber active", 0);
	bonobo_property_bag_add (throbber->details->property_bag, "location", LOCATION, BONOBO_ARG_STRING, NULL,
				 "associated URL", 0);
	bonobo_property_bag_add (throbber->details->property_bag, "style", STYLE, BONOBO_ARG_INT, NULL, NULL,
				 Bonobo_PROPERTY_WRITEABLE);
	nautilus_throbber_load_images (throbber);
	gtk_widget_show (widget);

	/* add a callback for when the theme changes */
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_THEME,
				      nautilus_throbber_theme_changed,
				      throbber);
}

/* handler for handling theme changes */
static void
nautilus_throbber_theme_changed (gpointer user_data)
{
	NautilusThrobber *throbber;

	throbber = NAUTILUS_THROBBER (user_data);
	gtk_widget_hide (GTK_WIDGET (throbber));
	nautilus_throbber_load_images (throbber);
	gtk_widget_show (GTK_WIDGET (throbber));	
	gtk_widget_queue_resize ( GTK_WIDGET (throbber));
}

/* here's the routine that selects the image to draw, based on the throbber's state */

static GdkPixbuf *
select_throbber_image (NautilusThrobber *throbber)
{
	GList *element;

	if (throbber->details->timer_task == 0) {
		return g_object_ref (throbber->details->quiescent_pixbuf);
	}
	
	if (throbber->details->image_list == NULL) {
		return NULL;
	}

	element = g_list_nth (throbber->details->image_list, throbber->details->current_frame);
	
	return g_object_ref (element->data);
}

/* handle expose events */

static int
nautilus_throbber_expose (GtkWidget *widget, GdkEventExpose *event)
{
	NautilusThrobber *throbber;
	GdkPixbuf *pixbuf, *massaged_pixbuf;
	int x_offset, y_offset, width, height;
	GdkRectangle pix_area, dest;

	g_return_val_if_fail (NAUTILUS_IS_THROBBER (widget), FALSE);

	throbber = NAUTILUS_THROBBER (widget);
	if (!throbber->details->ready) {
		return FALSE;
	}

	pixbuf = select_throbber_image (throbber);
	if (pixbuf == NULL) {
		return FALSE;
	}

	/* Get the right tint on the image */
	if (throbber->details->button_in) {
		if (throbber->details->button_down) {
			massaged_pixbuf = eel_create_darkened_pixbuf (pixbuf, 0.8 * 255, 0.8 * 255);
		} else {
			massaged_pixbuf = eel_create_spotlight_pixbuf (pixbuf);
		}
		g_object_unref (pixbuf);
		pixbuf = massaged_pixbuf;
	}

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	/* Compute the offsets for the image centered on our allocation */
	x_offset = widget->allocation.x + (widget->allocation.width - width) / 2;
	y_offset = widget->allocation.y + (widget->allocation.height - height) / 2;

	pix_area.x = x_offset;
	pix_area.y = y_offset;
	pix_area.width = width;
	pix_area.height = height;

	if (!gdk_rectangle_intersect (&event->area, &pix_area, &dest)) {
		g_object_unref (pixbuf);
		return FALSE;
	}
	
	gdk_pixbuf_render_to_drawable_alpha (
		pixbuf, widget->window, 
		dest.x - x_offset, dest.y - y_offset,
		dest.x, dest.y,
		dest.width, dest.height,
		GDK_PIXBUF_ALPHA_BILEVEL, 128,
		GDK_RGB_DITHER_MAX,
		0, 0);

	g_object_unref (pixbuf);

	return FALSE;
}

static void
nautilus_throbber_map (GtkWidget *widget)
{
	NautilusThrobber *throbber;
	
	throbber = NAUTILUS_THROBBER (widget);
	
	GNOME_CALL_PARENT (GTK_WIDGET_CLASS, map, (widget));
	throbber->details->ready = TRUE;
}

/* here's the actual timeout task to bump the frame and schedule a redraw */

static gboolean 
bump_throbber_frame (gpointer callback_data)
{
	NautilusThrobber *throbber;

	throbber = NAUTILUS_THROBBER (callback_data);
	if (!throbber->details->ready) {
		return TRUE;
	}

	throbber->details->current_frame += 1;
	if (throbber->details->current_frame > throbber->details->max_frame - 1) {
		throbber->details->current_frame = 0;
	}

	gtk_widget_queue_draw (GTK_WIDGET (throbber));
	return TRUE;
}


/* routines to start and stop the throbber */

void
nautilus_throbber_start (NautilusThrobber *throbber)
{
	if (is_throbbing (throbber)) {
		return;
	}

	if (throbber->details->timer_task != 0) {
		gtk_timeout_remove (throbber->details->timer_task);
	}
	
	/* reset the frame count */
	throbber->details->current_frame = 0;
	throbber->details->timer_task = gtk_timeout_add (throbber->details->delay,
							 bump_throbber_frame,
							 throbber);
}

static void
nautilus_throbber_remove_update_callback (NautilusThrobber *throbber)
{
	if (throbber->details->timer_task != 0) {
		gtk_timeout_remove (throbber->details->timer_task);
	}
	
	throbber->details->timer_task = 0;
}

void
nautilus_throbber_stop (NautilusThrobber *throbber)
{
	if (!is_throbbing (throbber)) {
		return;
	}

	nautilus_throbber_remove_update_callback (throbber);
	gtk_widget_queue_draw (GTK_WIDGET (throbber));

}

/* routines to load the images used to draw the throbber */

/* unload all the images, and the list itself */

static void
nautilus_throbber_unload_images (NautilusThrobber *throbber)
{
	GList *current_entry;

	if (throbber->details->quiescent_pixbuf != NULL) {
		g_object_unref (throbber->details->quiescent_pixbuf);
		throbber->details->quiescent_pixbuf = NULL;
	}

	/* unref all the images in the list, and then let go of the list itself */
	current_entry = throbber->details->image_list;
	while (current_entry != NULL) {
		g_object_unref (current_entry->data);
		current_entry = current_entry->next;
	}
	
	g_list_free (throbber->details->image_list);
	throbber->details->image_list = NULL;
}

static GdkPixbuf*
load_themed_image (const char *file_name, const char *image_theme, gboolean small_mode)
{
	GdkPixbuf *pixbuf, *temp_pixbuf;
	char *image_path;
	
	if (image_theme == NULL) {
		image_path = nautilus_theme_get_image_path (file_name);
	} else {
		image_path = nautilus_theme_get_image_path_from_theme (file_name, image_theme);	
	}
	
	if (image_path) {
		pixbuf = gdk_pixbuf_new_from_file (image_path, NULL);
		
		if (small_mode && pixbuf) {
			temp_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
							       gdk_pixbuf_get_width (pixbuf) * 2 / 3,
							       gdk_pixbuf_get_height (pixbuf) * 2 / 3,
							       GDK_INTERP_BILINEAR);
			g_object_unref (pixbuf);
			pixbuf = temp_pixbuf;
		}
		
		g_free (image_path);
		return pixbuf;
	}
	return NULL;
}

/* utility to make the throbber frame name from the index */

static char *
make_throbber_frame_name (int index)
{
	return g_strdup_printf ("throbber/%03d.png", index);
}

/* load all of the images of the throbber sequentially */
static void
nautilus_throbber_load_images (NautilusThrobber *throbber)
{
	int index;
	char *throbber_frame_name, *image_theme, *frames;
	GdkPixbuf *pixbuf;
	GList *image_list;
	
	nautilus_throbber_unload_images (throbber);

	image_theme = nautilus_theme_get_theme_data ("throbber", "image_theme");
	throbber->details->quiescent_pixbuf = load_themed_image ("throbber/rest.png", image_theme, throbber->details->small_mode);

	/* images are of the form throbber/001.png, 002.png, etc, so load them into a list */

	frames = nautilus_theme_get_theme_data ("throbber", "frame_count");
	if (frames != NULL) {
		throbber->details->max_frame = atoi (frames);
		g_free (frames);
	} else {
		throbber->details->max_frame = 16;
	}

	image_list = NULL;
	for (index = 1; index <= throbber->details->max_frame; index++) {
		throbber_frame_name = make_throbber_frame_name (index);
		pixbuf = load_themed_image (throbber_frame_name, image_theme, throbber->details->small_mode);

		g_free (throbber_frame_name);
		if (pixbuf == NULL) {
			throbber->details->max_frame = index - 1;
			break;
		}
		image_list = g_list_prepend (image_list, pixbuf);
	}
	throbber->details->image_list = g_list_reverse (image_list);

	g_free (image_theme);
}

static gboolean
nautilus_throbber_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
	NautilusThrobber *throbber;

	throbber = NAUTILUS_THROBBER (widget);

	if (!throbber->details->button_in) {
		throbber->details->button_in = TRUE;
		gtk_widget_queue_draw (widget);
	}

	return GNOME_CALL_PARENT_WITH_DEFAULT
		(GTK_WIDGET_CLASS, enter_notify_event, (widget, event), FALSE);
}

static gboolean
nautilus_throbber_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
	NautilusThrobber *throbber;

	throbber = NAUTILUS_THROBBER (widget);

	if (throbber->details->button_in) {
		throbber->details->button_in = FALSE;
		gtk_widget_queue_draw (widget);
	}

	return GNOME_CALL_PARENT_WITH_DEFAULT
		(GTK_WIDGET_CLASS, leave_notify_event, (widget, event), FALSE);
}

/* handle button presses by posting a change on the "location" property */

static gboolean
nautilus_throbber_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	NautilusThrobber *throbber;

	throbber = NAUTILUS_THROBBER (widget);

	if (event->button == 1) {
		throbber->details->button_down = TRUE;
		throbber->details->button_in = TRUE;
		gtk_widget_queue_draw (widget);
		return TRUE;
	}

	return GNOME_CALL_PARENT_WITH_DEFAULT
		(GTK_WIDGET_CLASS, button_press_event, (widget, event), FALSE);
}

static void
nautilus_throbber_set_location (NautilusThrobber *throbber)
{
	char *location;
	BonoboArg *location_arg;

	location = nautilus_theme_get_theme_data ("throbber", "url");
	if (location != NULL) {
		location_arg = bonobo_arg_new (BONOBO_ARG_STRING);
		BONOBO_ARG_SET_STRING (location_arg, location);
		bonobo_event_source_notify_listeners_full (
			throbber->details->property_bag->es,
			"Bonobo/Property", "change", "location",
			location_arg, NULL);
		bonobo_arg_release (location_arg);
		g_free (location);
	}
}

static gboolean
nautilus_throbber_button_release_event (GtkWidget *widget, GdkEventButton *event)
{	
	NautilusThrobber *throbber;
	
	throbber = NAUTILUS_THROBBER (widget);

	if (event->button == 1) {
		if (throbber->details->button_in) {
			nautilus_throbber_set_location (throbber);
		}
		throbber->details->button_down = FALSE;
		gtk_widget_queue_draw (widget);
		return TRUE;
	}
	
	return GNOME_CALL_PARENT_WITH_DEFAULT
		(GTK_WIDGET_CLASS, button_release_event, (widget, event), FALSE);
}

void
nautilus_throbber_set_small_mode (NautilusThrobber *throbber, gboolean new_mode)
{
	if (new_mode != throbber->details->small_mode) {
		throbber->details->small_mode = new_mode;
		nautilus_throbber_load_images (throbber);

		gtk_widget_queue_resize (GTK_WIDGET (throbber));
	}
}

/* handle setting the size */

static void
nautilus_throbber_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	int throbber_width, throbber_height;
	NautilusThrobber *throbber = NAUTILUS_THROBBER (widget);

	get_throbber_dimensions (throbber, &throbber_width, &throbber_height);
	
	/* allocate some extra margin so we don't butt up against toolbar edges */
	requisition->width = throbber_width + 8;
   	requisition->height = throbber_height;
}

static void
nautilus_throbber_finalize (GObject *object)
{
	NautilusThrobber *throbber;

	throbber = NAUTILUS_THROBBER (object);

	nautilus_throbber_remove_update_callback (throbber);
	nautilus_throbber_unload_images (throbber);
	
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_THEME,
					 nautilus_throbber_theme_changed, object);
	
	bonobo_object_unref (throbber->details->property_bag);
	
	eel_remove_weak_pointer (&throbber->details->control);

	g_free (throbber->details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
nautilus_throbber_class_init (NautilusThrobberClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (class);
	
	G_OBJECT_CLASS (class)->finalize = nautilus_throbber_finalize;

	widget_class->expose_event = nautilus_throbber_expose;
	widget_class->button_press_event = nautilus_throbber_button_press_event;
	widget_class->button_release_event = nautilus_throbber_button_release_event;
	widget_class->enter_notify_event = nautilus_throbber_enter_notify_event;
	widget_class->leave_notify_event = nautilus_throbber_leave_notify_event;
	widget_class->size_request = nautilus_throbber_size_request;	
	widget_class->map = nautilus_throbber_map;
	widget_class->get_accessible = nautilus_throbber_get_accessible;
}

static AtkObjectClass *a11y_parent_class = NULL;

static void
nautilus_throbber_accessible_initialize (AtkObject *accessible,
					 gpointer   widget)
{
	atk_object_set_name (accessible, _("throbber"));
	atk_object_set_description (accessible, _("provides visual status"));

	a11y_parent_class->initialize (accessible, widget);
}

static void
nautilus_throbber_accessible_class_init (AtkObjectClass *klass)
{
	a11y_parent_class = g_type_class_peek_parent (klass);

	klass->initialize = nautilus_throbber_accessible_initialize;
}

static void
nautilus_throbber_accessible_image_get_size (AtkImage *image,
					     gint     *width,
					     gint     *height)
{
	GtkWidget *widget;

	widget = GTK_ACCESSIBLE (image)->widget;
	if (!widget) {
		*width = *height = 0;
	} else {
		*width = widget->allocation.width;
		*height = widget->allocation.height;
	}
}

static void
nautilus_throbber_accessible_image_interface_init (AtkImageIface *iface)
{
	iface->get_image_size = nautilus_throbber_accessible_image_get_size;
}

/* AtkAction interface */

enum {
	ACTION_ACTIVATE,
	LAST_ACTION
};

static const char *nautilus_throbber_accessible_action_names[] = {
	"activate",
	NULL
};

static const char *nautilus_throbber_accessible_action_descriptions[] = {
	"Activate selected items",
	NULL
};


static gboolean
nautilus_throbber_accessible_do_action (AtkAction *accessible, int i)
{
	GtkWidget *widget;

	g_return_val_if_fail (i < LAST_ACTION, FALSE);

	widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget) {
		return FALSE;
	}
	
	switch (i) {
	case ACTION_ACTIVATE :
		nautilus_throbber_set_location (NAUTILUS_THROBBER (widget));
		return TRUE;
	default:
		return FALSE;
	}
}

static int
nautilus_throbber_accessible_get_n_actions (AtkAction *accessible)
{
	return LAST_ACTION;
}

static const char *
nautilus_throbber_accessible_action_get_description (AtkAction *accessible, 
							   int i)
{
	g_return_val_if_fail (i < LAST_ACTION, NULL);

	return nautilus_throbber_accessible_action_descriptions[i];
}

static const char *
nautilus_throbber_accessible_action_get_name (AtkAction *accessible, int i)
{
	g_return_val_if_fail (i < LAST_ACTION, NULL);

	return nautilus_throbber_accessible_action_names [i];
}

static const char *
nautilus_throbber_accessible_action_get_keybinding (AtkAction *accessible, 
						    int        i)
{
	return NULL;
}

static gboolean
nautilus_throbber_accessible_action_set_description (AtkAction  *accessible, 
						     int         i, 
						     const char *description)
{
	return FALSE;
}

static void
nautilus_throbber_accessible_action_interface_init (AtkActionIface *iface)
{
	iface->do_action = nautilus_throbber_accessible_do_action;
	iface->get_n_actions = nautilus_throbber_accessible_get_n_actions;
	iface->get_description = nautilus_throbber_accessible_action_get_description;
	iface->get_name = nautilus_throbber_accessible_action_get_name;
	iface->get_keybinding = nautilus_throbber_accessible_action_get_keybinding;
	iface->set_description = nautilus_throbber_accessible_action_set_description;
}

static GType
nautilus_throbber_accessible_get_type (void)
{
        static GType type = 0;

	/* Action interface
	   Name etc. ... */
        if (!type) {
                static GInterfaceInfo atk_action_info = {
                        (GInterfaceInitFunc) nautilus_throbber_accessible_action_interface_init,
                        (GInterfaceFinalizeFunc) NULL,
                        NULL
                };              

		static const GInterfaceInfo atk_image_info = {
			(GInterfaceInitFunc) nautilus_throbber_accessible_image_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = eel_accessibility_create_derived_type 
			("NautilusThrobberAccessible",
			 GTK_TYPE_IMAGE,
			 nautilus_throbber_accessible_class_init);
		
                g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                             &atk_action_info);
                g_type_add_interface_static (type, ATK_TYPE_IMAGE,
                                             &atk_image_info);
        }

        return type;
}

static AtkObject *
nautilus_throbber_get_accessible (GtkWidget *widget)
{
	AtkObject *accessible;
	
	if ((accessible = eel_accessibility_get_atk_object (widget))) {
		return accessible;
	}
	
	accessible = g_object_new 
		(nautilus_throbber_accessible_get_type (), NULL);
	
	return eel_accessibility_set_atk_object_return (widget, accessible);
}
