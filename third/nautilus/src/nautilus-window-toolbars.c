/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 2000, 2001 Eazel, Inc.
 *
 * Nautilus is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: John Sullivan <sullivan@eazel.com> 
 */

/* nautilus-window-toolbars.c - implementation of nautilus window toolbar operations,
 * split into separate file just for convenience.
 */

#include <config.h>

#include <unistd.h>
#include "nautilus-application.h"
#include "nautilus-window-manage-views.h"
#include "nautilus-window-private.h"
#include "nautilus-window.h"
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-property-bag.h>
#include <bonobo/bonobo-property-bag-client.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-ui-util.h>
#include <eel/eel-gnome-extensions.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-string.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktogglebutton.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-popup-menu.h>
#include <libnautilus-private/nautilus-bonobo-extensions.h>
#include <libnautilus-private/nautilus-bookmark.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-theme.h>

/* FIXME bugzilla.gnome.org 41243: 
 * We should use inheritance instead of these special cases
 * for the desktop window.
 */
#include "nautilus-desktop-window.h"

enum {
	TOOLBAR_ITEM_STYLE_PROP,
	TOOLBAR_ITEM_ORIENTATION_PROP
};

static void
activate_back_or_forward_menu_item (GtkMenuItem *menu_item, 
				    NautilusWindow *window,
				    gboolean back)
{
	int index;
	
	g_assert (GTK_IS_MENU_ITEM (menu_item));
	g_assert (NAUTILUS_IS_WINDOW (window));

	index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), "user_data"));
	nautilus_window_back_or_forward (window, back, index);
}

static void
activate_back_menu_item_callback (GtkMenuItem *menu_item, NautilusWindow *window)
{
	activate_back_or_forward_menu_item (menu_item, window, TRUE);
}

static void
activate_forward_menu_item_callback (GtkMenuItem *menu_item, NautilusWindow *window)
{
	activate_back_or_forward_menu_item (menu_item, window, FALSE);
}

static GtkMenu *
create_back_or_forward_menu (NautilusWindow *window, gboolean back)
{
	GtkMenu *menu;
	GtkWidget *menu_item;
	GList *list_link;
	int index;

	g_assert (NAUTILUS_IS_WINDOW (window));
	
	menu = GTK_MENU (gtk_menu_new ());

	list_link = back ? window->back_list : window->forward_list;
	index = 0;
	while (list_link != NULL)
	{
		menu_item = nautilus_bookmark_menu_item_new (NAUTILUS_BOOKMARK (list_link->data));		
		g_object_set_data (G_OBJECT (menu_item), "user_data", GINT_TO_POINTER (index));
		gtk_widget_show (GTK_WIDGET (menu_item));
  		g_signal_connect_object (menu_item, "activate",
					 back
					 ? G_CALLBACK (activate_back_menu_item_callback)
					 : G_CALLBACK (activate_forward_menu_item_callback),
					 window, 0);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		list_link = g_list_next (list_link);
		++index;
	}

	return menu;
}

static GtkWidget *
get_back_button (NautilusWindow *window)
{
	return GTK_WIDGET (GTK_BIN 
		(window->details->back_button_item)->child);
}

static GtkWidget *
get_forward_button (NautilusWindow *window)
{
	return GTK_WIDGET (GTK_BIN
		(window->details->forward_button_item)->child);
}

static void
menu_position_under_widget (GtkMenu *menu, int *x, int *y,
			    gboolean *push_in, gpointer user_data)
{
	GtkWidget *w;
	int width, height;
	int screen_width, screen_height;
	GtkRequisition requisition;

	w = GTK_WIDGET (user_data);
	
	gdk_drawable_get_size (w->window, &width, &height);
	gdk_window_get_origin (w->window, x, y);
	*y = *y + height;

	gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

	screen_width = gdk_screen_width ();
	screen_height = gdk_screen_height ();

	*x = CLAMP (*x, 0, MAX (0, screen_width - requisition.width));
	*y = CLAMP (*y, 0, MAX (0, screen_height - requisition.height));
}

static gboolean
back_or_forward_button_pressed_callback (GtkWidget *widget, 
					 GdkEventButton *event,
					 gpointer *user_data)
{
	NautilusWindow *window;
	gboolean back;

	g_return_val_if_fail (GTK_IS_BUTTON (widget), FALSE);
	g_return_val_if_fail (NAUTILUS_IS_WINDOW (user_data), FALSE);

	window = NAUTILUS_WINDOW (user_data);

	back = widget == get_back_button (window);
	g_assert (back || widget == get_forward_button (window));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

	gnome_popup_menu_do_popup_modal (GTK_WIDGET (create_back_or_forward_menu (NAUTILUS_WINDOW (user_data), back)),
					 menu_position_under_widget, widget, event, widget, widget);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
	
	return TRUE;
}

static gboolean
back_or_forward_key_pressed_callback (GtkWidget *widget,
				      GdkEventKey *event,
				      gpointer *user_data)
{
	if (event->keyval == GDK_space ||
	    event->keyval == GDK_KP_Space ||
	    event->keyval == GDK_Return ||
	    event->keyval == GDK_KP_Enter) {
		back_or_forward_button_pressed_callback (widget, NULL, user_data);
	}

	return FALSE;
}

static void
back_or_forward_toolbar_item_property_set_cb (BonoboPropertyBag *bag,
					      const BonoboArg   *arg,
					      guint              arg_id,
					      CORBA_Environment *ev,
					      gpointer           user_data)
{
	BonoboControl *control;
	BonoboUIToolbarItem *item;
	GtkOrientation orientation;
	BonoboUIToolbarItemStyle style;

	control = BONOBO_CONTROL (user_data);
	item = BONOBO_UI_TOOLBAR_ITEM (bonobo_control_get_widget (control));

	switch (arg_id) {
	case TOOLBAR_ITEM_ORIENTATION_PROP:
		orientation = BONOBO_ARG_GET_INT (arg);
		bonobo_ui_toolbar_item_set_orientation (item, orientation);

		if (GTK_WIDGET (item)->parent) {
			gtk_widget_queue_resize (GTK_WIDGET (item)->parent);
		}
		break;
	case TOOLBAR_ITEM_STYLE_PROP:
		style = BONOBO_ARG_GET_INT (arg);
		bonobo_ui_toolbar_item_set_style (item, style);
		break;
	}
}

static BonoboUIToolbarItem *
create_back_or_forward_toolbar_item (NautilusWindow *window, 
				     const char     *tooltip,
				     const char     *control_path)
{
	BonoboUIToolbarItem *item;
	BonoboPropertyBag *pb;
	BonoboControl *wrapper;
	GtkWidget *button;

	item = BONOBO_UI_TOOLBAR_ITEM (bonobo_ui_toolbar_item_new ());

	button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	gtk_container_add (GTK_CONTAINER (button),
			   gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT));

	gtk_container_add (GTK_CONTAINER (item), button);

	gtk_widget_show_all (GTK_WIDGET (item));

	gtk_tooltips_set_tip (window->details->tooltips,
			      GTK_WIDGET (button),
			      tooltip, NULL);
	g_signal_connect_object (button, "key_press_event",
				 G_CALLBACK (back_or_forward_key_pressed_callback),
				 window, 0);
	g_signal_connect_object (button, "button_press_event",
				 G_CALLBACK (back_or_forward_button_pressed_callback),
				 window, 0);

	wrapper = bonobo_control_new (GTK_WIDGET (item));
	pb = bonobo_property_bag_new
		(NULL, back_or_forward_toolbar_item_property_set_cb, wrapper);
	bonobo_property_bag_add (pb, "style",
				 TOOLBAR_ITEM_STYLE_PROP,
				 BONOBO_ARG_INT, NULL, NULL,
				 Bonobo_PROPERTY_WRITEABLE);
	bonobo_property_bag_add (pb, "orientation",
				 TOOLBAR_ITEM_ORIENTATION_PROP,
				 BONOBO_ARG_INT, NULL, NULL,
				 Bonobo_PROPERTY_WRITEABLE);
	bonobo_control_set_properties (wrapper, BONOBO_OBJREF (pb), NULL);
	bonobo_object_unref (pb);

	bonobo_ui_component_object_set (window->details->shell_ui,
					control_path,
					BONOBO_OBJREF (wrapper),
					NULL);

	bonobo_object_unref (wrapper);

	return item;
}

static gboolean
location_change_at_idle_callback (gpointer callback_data)
{
	NautilusWindow *window;
	char *location;

	window = NAUTILUS_WINDOW (callback_data);

	location = window->details->location_to_change_to_at_idle;
	window->details->location_to_change_to_at_idle = NULL;
	window->details->location_change_at_idle_id = 0;

	nautilus_window_go_to (window, location);
	g_free (location);

	return FALSE;
}

/* handle bonobo events from the throbber -- since they can come in at
   any time right in the middle of things, defer until idle */
static void 
throbber_callback (BonoboListener *listener,
		   const char *event_name, 
		   const CORBA_any *arg,
		   CORBA_Environment *ev,
		   gpointer callback_data)
{
	NautilusWindow *window;

	window = NAUTILUS_WINDOW (callback_data);

	g_free (window->details->location_to_change_to_at_idle);
	window->details->location_to_change_to_at_idle = g_strdup (
		BONOBO_ARG_GET_STRING (arg));

	if (window->details->location_change_at_idle_id == 0) {
		window->details->location_change_at_idle_id =
			gtk_idle_add (location_change_at_idle_callback, window);
	}
}

static void
throbber_set_throbbing (NautilusWindow *window,
			gboolean        throbbing)
{
	CORBA_boolean b;
	CORBA_any     val;

	val._type = TC_CORBA_boolean;
	val._value = &b;
	b = throbbing;

	eel_bonobo_pbclient_set_value_async (
		window->details->throbber_property_bag,
		"throbbing", &val, NULL);
}

static void
throbber_created_callback (Bonobo_Unknown     throbber,
			   CORBA_Environment *ev,
			   gpointer           user_data)
{
	char *exception_as_text;
	NautilusWindow *window;

	if (BONOBO_EX (ev)) {
		exception_as_text = bonobo_exception_get_text (ev);
		g_warning ("Throbber activation exception '%s'", exception_as_text);
		g_free (exception_as_text);
		return;
	}

	g_return_if_fail (NAUTILUS_IS_WINDOW (user_data));

	window = NAUTILUS_WINDOW (user_data);

	window->details->throbber_activating = FALSE;

	bonobo_ui_component_object_set (window->details->shell_ui,
					"/Toolbar/ThrobberWrapper",
					throbber, ev);
	CORBA_exception_free (ev);

	window->details->throbber_property_bag =
		Bonobo_Control_getProperties (throbber, ev);

	if (BONOBO_EX (ev)) {
		window->details->throbber_property_bag = CORBA_OBJECT_NIL;
		CORBA_exception_free (ev);
	} else {
		throbber_set_throbbing (window, window->details->throbber_active);
	}

	window->details->throbber_listener =
		bonobo_event_source_client_add_listener_full
		(window->details->throbber_property_bag,
		 g_cclosure_new (G_CALLBACK (throbber_callback), window, NULL), 
		 "Bonobo/Property:change:location", ev);

	bonobo_object_release_unref (throbber, ev);

	g_object_unref (window);
}

void
nautilus_window_allow_stop (NautilusWindow *window, gboolean allow)
{
	if (( window->details->throbber_active &&  allow) ||
	    (!window->details->throbber_active && !allow)) {
		return;
	}

	if (allow)
		access ("nautilus-throbber: start", 0);
	else
		access ("nautilus-throbber: stop", 0);

	window->details->throbber_active = allow;

	nautilus_window_ui_freeze (window);

	nautilus_bonobo_set_sensitive (window->details->shell_ui,
				       NAUTILUS_COMMAND_STOP, allow);

	if (window->details->throbber_property_bag != CORBA_OBJECT_NIL) {
		throbber_set_throbbing (window, allow);
	}

	nautilus_window_ui_thaw (window);
}


void
nautilus_window_activate_throbber (NautilusWindow *window)
{
	CORBA_Environment ev;
	char *exception_as_text;

	if (window->details->throbber_activating ||
	    window->details->throbber_property_bag != CORBA_OBJECT_NIL) {
		return;
	}

	/* FIXME bugzilla.gnome.org 41243: 
	 * We should use inheritance instead of these special cases
	 * for the desktop window.
	 */
	if (!NAUTILUS_IS_DESKTOP_WINDOW (window)) {
		CORBA_exception_init (&ev);

		g_object_ref (window);
		bonobo_get_object_async ("OAFIID:Nautilus_Throbber",
					 "IDL:Bonobo/Control:1.0",
					 &ev,
					 throbber_created_callback,
					 window);

		if (BONOBO_EX (&ev)) {
			exception_as_text = bonobo_exception_get_text (&ev);
			g_warning ("Throbber activation exception '%s'", exception_as_text);
			g_free (exception_as_text);
		}
		CORBA_exception_free (&ev);
		window->details->throbber_activating = TRUE;		
	}
}

void
nautilus_window_initialize_toolbars (NautilusWindow *window)
{
	nautilus_window_ui_freeze (window);

	if (eel_preferences_get_boolean (NAUTILUS_PREFERENCES_START_WITH_TOOLBAR)) {
		nautilus_window_activate_throbber (window);
	}

	window->details->back_button_item = create_back_or_forward_toolbar_item 
		(window, _("Go back a few pages"),
		 "/Toolbar/BackMenu");
	window->details->forward_button_item = create_back_or_forward_toolbar_item 
		(window, _("Go forward a number of pages"),
		 "/Toolbar/ForwardMenu");

	nautilus_window_ui_thaw (window);
}
