/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Nautilus
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *  	     John Sullivan <sullivan@eazel.com>
 *
 */

/* nautilus-window.c: Implementation of the main window object */

#include <config.h>
#include "nautilus-spatial-window.h"
#include "nautilus-window-private.h"

#include "nautilus-application.h"
#include "nautilus-desktop-window.h"
#include "nautilus-bookmarks-window.h"
#include "nautilus-location-dialog.h"
#include "nautilus-main.h"
#include "nautilus-signaller.h"
#include "nautilus-switchable-navigation-bar.h"
#include "nautilus-window-manage-views.h"
#include "nautilus-zoom-control.h"
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-property-bag-client.h>
#include <bonobo/bonobo-ui-util.h>
#include <eel/eel-debug.h>
#include <eel/eel-gdk-extensions.h>
#include <eel/eel-gdk-pixbuf-extensions.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-gtk-macros.h>
#include <eel/eel-string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkvbox.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-macros.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libnautilus-private/nautilus-bonobo-extensions.h>
#include <libnautilus-private/nautilus-drag-window.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-file-attributes.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-horizontal-splitter.h>
#include <libnautilus-private/nautilus-icon-factory.h>
#include <libnautilus-private/nautilus-metadata.h>
#include <libnautilus-private/nautilus-mime-actions.h>
#include <libnautilus-private/nautilus-program-choosing.h>
#include <libnautilus-private/nautilus-sidebar-functions.h>
#include <libnautilus/nautilus-bonobo-ui.h>
#include <libnautilus/nautilus-clipboard.h>
#include <libnautilus/nautilus-undo.h>
#include <math.h>
#include <sys/time.h>

#define MAX_TITLE_LENGTH 180

struct _NautilusSpatialWindowDetails {        
	char *last_geometry;	
        guint save_geometry_timeout_id;	  
	
	GtkWidget *content_box;
	GtkWidget *location_button;
	GtkWidget *location_label;
	GtkWidget *location_statusbar;

	GnomeVFSURI *location;
};

GNOME_CLASS_BOILERPLATE (NautilusSpatialWindow, nautilus_spatial_window,
			 NautilusWindow, NAUTILUS_TYPE_WINDOW)

static gboolean
save_window_geometry_timeout (gpointer callback_data)
{
	NautilusSpatialWindow *window;
	
	window = NAUTILUS_SPATIAL_WINDOW (callback_data);
	
	nautilus_spatial_window_save_geometry (window);

	window->details->save_geometry_timeout_id = 0;

	return FALSE;
}

static gboolean
nautilus_spatial_window_configure_event (GtkWidget *widget,
					GdkEventConfigure *event)
{
	NautilusSpatialWindow *window;
	char *geometry_string;
	
	window = NAUTILUS_SPATIAL_WINDOW (widget);

	GTK_WIDGET_CLASS (parent_class)->configure_event (widget, event);
	
	/* Only save the geometry if the user hasn't resized the window
	 * for a second. Otherwise delay the callback another second.
	 */
	if (window->details->save_geometry_timeout_id != 0) {
		g_source_remove (window->details->save_geometry_timeout_id);
	}
	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (window)) && !NAUTILUS_IS_DESKTOP_WINDOW (window)) {
		geometry_string = eel_gtk_window_get_geometry_string (GTK_WINDOW (window));
	
		/* If the last geometry is NULL the window must have just
		 * been shown. No need to save geometry to disk since it
		 * must be the same.
		 */
		if (window->details->last_geometry == NULL) {
			window->details->last_geometry = geometry_string;
			return FALSE;
		}
	
		/* Don't save geometry if it's the same as before. */
		if (!strcmp (window->details->last_geometry, 
			     geometry_string)) {
			g_free (geometry_string);
			return FALSE;
		}

		g_free (window->details->last_geometry);
		window->details->last_geometry = geometry_string;

		window->details->save_geometry_timeout_id = 
			g_timeout_add (1000, save_window_geometry_timeout, window);
	}
	
	return FALSE;
}

static void
nautilus_spatial_window_unrealize (GtkWidget *widget)
{
	NautilusSpatialWindow *window;
	
	window = NAUTILUS_SPATIAL_WINDOW (widget);

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);

	if (window->details->save_geometry_timeout_id != 0) {
		g_source_remove (window->details->save_geometry_timeout_id);
		window->details->save_geometry_timeout_id = 0;
		nautilus_spatial_window_save_geometry (window);
	}
}


static void
nautilus_spatial_window_realize (GtkWidget *widget)
{
	NautilusSpatialWindow *window;

	window = NAUTILUS_SPATIAL_WINDOW (widget);

	GTK_WIDGET_CLASS (parent_class)->realize (widget);

	if (window->loading) {
		GdkCursor *cursor;

		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (widget->window, cursor);
		gdk_cursor_unref (cursor);
	} else {
		gdk_window_set_cursor (widget->window, NULL);
	}
}


static void
nautilus_spatial_window_destroy (GtkObject *object)
{
	NautilusSpatialWindow *window;

	window = NAUTILUS_SPATIAL_WINDOW (object);

	window->details->content_box = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
nautilus_spatial_window_finalize (GObject *object)
{
	NautilusSpatialWindow *window;
	
	window = NAUTILUS_SPATIAL_WINDOW (object);

	if (window->details->location != NULL) {
		gnome_vfs_uri_unref (window->details->location);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
nautilus_spatial_window_save_geometry (NautilusSpatialWindow *window)
{
	char *geometry_string;

	g_assert (NAUTILUS_IS_WINDOW (window));

	if (NAUTILUS_WINDOW (window)->details->viewed_file == NULL) {
		/* We never showed a file */
		return;
	}
	
	if (GTK_WIDGET(window)->window &&
	    !(gdk_window_get_state (GTK_WIDGET(window)->window) & GDK_WINDOW_STATE_MAXIMIZED)) {
		geometry_string = eel_gtk_window_get_geometry_string (GTK_WINDOW (window));
		
		nautilus_file_set_metadata (NAUTILUS_WINDOW (window)->details->viewed_file,
					    NAUTILUS_METADATA_KEY_WINDOW_GEOMETRY,
					    NULL,
					    geometry_string);
		
		g_free (geometry_string);
	}
}

void
nautilus_spatial_window_save_scroll_position (NautilusSpatialWindow *window)
{
	NautilusWindow *parent;
	char *scroll_string;

	parent = NAUTILUS_WINDOW(window);
	scroll_string = nautilus_view_frame_get_first_visible_file (parent->content_view);
	nautilus_file_set_metadata (parent->details->viewed_file,
				    NAUTILUS_METADATA_KEY_WINDOW_SCROLL_POSITION,
				    NULL,
				    scroll_string);
	g_free (scroll_string);
}

void
nautilus_spatial_window_save_show_hidden_files_mode (NautilusSpatialWindow *window)
{
	char *show_hidden_file_setting;
	Nautilus_ShowHiddenFilesMode mode;

	mode = NAUTILUS_WINDOW (window)->details->show_hidden_files_mode;
	if (mode != Nautilus_SHOW_HIDDEN_FILES_DEFAULT) {
		if (mode == Nautilus_SHOW_HIDDEN_FILES_ENABLE) {
			show_hidden_file_setting = "1";
		} else {
			show_hidden_file_setting = "0";
		}
		nautilus_file_set_metadata (NAUTILUS_WINDOW (window)->details->viewed_file,
			 		    NAUTILUS_METADATA_KEY_WINDOW_SHOW_HIDDEN_FILES,
				    	    NULL,
				    	    show_hidden_file_setting);
	} 
}

static void
nautilus_spatial_window_show (GtkWidget *widget)
{	
	NautilusSpatialWindow *window;

	window = NAUTILUS_SPATIAL_WINDOW (widget);
	
	GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
file_menu_close_parent_windows_callback (BonoboUIComponent *component, 
					 gpointer user_data, 
					 const char *verb)
{
	nautilus_application_close_parent_windows (NAUTILUS_SPATIAL_WINDOW (user_data));
}

static void
file_menu_close_all_windows_callback (BonoboUIComponent *component, 
					 gpointer user_data, 
					 const char *verb)
{
	nautilus_application_close_all_spatial_windows ();
}

static void
go_up_close_current_window_callback (BonoboUIComponent *component, 
				     gpointer user_data, 
				     const char *verb)
{
	nautilus_window_go_up (NAUTILUS_WINDOW (user_data), TRUE);
}


static void
real_prompt_for_location (NautilusWindow *window)
{
	GtkWidget *dialog;
	
	dialog = nautilus_location_dialog_new (window);
	gtk_widget_show (dialog);
}

static void
real_set_title (NautilusWindow *window, const char *title)
{

	if (title[0] == '\0') {
		gtk_window_set_title (GTK_WINDOW (window), _("Nautilus"));
	} else {
		char *window_title;

		window_title = eel_str_middle_truncate (title, MAX_TITLE_LENGTH);
		gtk_window_set_title (GTK_WINDOW (window), window_title);
		g_free (window_title);
	}
}

static void
real_merge_menus (NautilusWindow *nautilus_window)
{
	NautilusSpatialWindow *window;
	BonoboControl *control;
	BonoboUIVerb verbs [] = {
		BONOBO_UI_VERB ("Close Parent Folders", file_menu_close_parent_windows_callback),
		BONOBO_UI_VERB ("Close All Folders", file_menu_close_all_windows_callback),
		BONOBO_UI_VERB ("UpCloseCurrent", go_up_close_current_window_callback),
		BONOBO_UI_VERB_END
	};
	
	EEL_CALL_PARENT (NAUTILUS_WINDOW_CLASS,
			 merge_menus, (nautilus_window));

	window = NAUTILUS_SPATIAL_WINDOW (nautilus_window);

	bonobo_ui_util_set_ui (NAUTILUS_WINDOW (window)->details->shell_ui,
			       DATADIR,
			       "nautilus-spatial-window-ui.xml",
			       "nautilus", NULL);

	bonobo_ui_component_add_verb_list_with_data (nautilus_window->details->shell_ui,
						     verbs, window);

	control = bonobo_control_new (window->details->location_statusbar);
	bonobo_ui_component_object_set (nautilus_window->details->shell_ui,
		       			"/status/StatusButton",
					BONOBO_OBJREF (control),
					NULL);
}

static void
real_set_content_view_widget (NautilusWindow *window,
			      NautilusViewFrame *new_view)
{
	EEL_CALL_PARENT (NAUTILUS_WINDOW_CLASS, set_content_view_widget,
			 (window, new_view));
	
	gtk_container_add (GTK_CONTAINER (NAUTILUS_SPATIAL_WINDOW (window)->details->content_box),
			   GTK_WIDGET (new_view));
}

static void
real_window_close (NautilusWindow *window)
{
	nautilus_spatial_window_save_geometry (NAUTILUS_SPATIAL_WINDOW (window));
	nautilus_spatial_window_save_scroll_position (NAUTILUS_SPATIAL_WINDOW (window));
	nautilus_spatial_window_save_show_hidden_files_mode (NAUTILUS_SPATIAL_WINDOW (window));
}

static void 
real_get_default_size(NautilusWindow *window, guint *default_width, guint *default_height)
{
   if(default_width) {
      *default_width = NAUTILUS_SPATIAL_WINDOW_DEFAULT_WIDTH;
   }
   if(default_height) {
      *default_height = NAUTILUS_SPATIAL_WINDOW_DEFAULT_HEIGHT;	
   }
}


static void
real_set_throbber_active (NautilusWindow *window, gboolean active)
{
	NautilusSpatialWindow *spatial;

	spatial = NAUTILUS_SPATIAL_WINDOW (window);
	spatial->loading = active;
	
	if (!GTK_WIDGET_REALIZED (GTK_WIDGET (window)))
		return;

	if (active) {
		GdkCursor *cursor;

		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (GTK_WIDGET (window)->window,
				       cursor);
		gdk_cursor_unref (cursor);
		
	} else {
		gdk_window_set_cursor (GTK_WIDGET (window)->window,
				       NULL);
	}
}


static void
location_menu_item_activated_callback (GtkWidget *menu_item,
				       NautilusSpatialWindow *window)
{
	GnomeVFSURI *uri;
	char *location;
	GdkEvent *event;

	uri = g_object_get_data (G_OBJECT (menu_item), "uri");
	location = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	event = gtk_get_current_event();

	if (!gnome_vfs_uri_equal (uri, window->details->location))
	{
		if (event != NULL && ((GdkEventAny *) event)->type == GDK_BUTTON_RELEASE &&
		   (((GdkEventButton *) event)->button == 2 ||
		   (((GdkEventButton *) event)->state & GDK_SHIFT_MASK) != 0))
		{
			nautilus_window_open_location (NAUTILUS_WINDOW (window), location, TRUE);
		} else {
			nautilus_window_open_location (NAUTILUS_WINDOW (window), location, FALSE);
		}
		
	}

	if (event != NULL) {
		gdk_event_free (event);
	}

	g_free (location);
	
}

static void
menu_deactivate_callback (GtkWidget *menu,
			  gpointer   data)
{
	GMainLoop *loop;

	loop = data;

	if (g_main_loop_is_running (loop)) {
		g_main_loop_quit (loop);
	}
}

static void
menu_popup_pos (GtkMenu   *menu,
		gint      *x,
		gint      *y,
		gboolean  *push_in,
		gpointer	user_data)
{
	GtkWidget *widget;
	GtkRequisition menu_requisition, button_requisition;

	widget = user_data;

	gtk_widget_size_request (GTK_WIDGET (menu), &menu_requisition);
	gtk_widget_size_request (widget, &button_requisition);

	gdk_window_get_origin (widget->window, x, y);

	*y -= menu_requisition.height - button_requisition.height;

	*push_in = TRUE;
}

static void
location_button_clicked_callback (GtkWidget *widget, NautilusSpatialWindow *window)
{
	GtkWidget *popup, *menu_item, *first_item;
	GnomeVFSURI *uri;
	char *name;
	GMainLoop *loop;

	
	g_return_if_fail (window->details->location != NULL);

	popup = gtk_menu_new ();
	first_item = NULL;
	uri = gnome_vfs_uri_ref (window->details->location);
	while (uri != NULL) {
		name = nautilus_get_uri_shortname_for_display (uri);
		menu_item = gtk_image_menu_item_new_with_label (name);
		if (first_item == NULL) {
			first_item = menu_item;
		}
		
		g_free (name);
		gtk_widget_show (menu_item);
		g_signal_connect (menu_item, "activate",
				  G_CALLBACK (location_menu_item_activated_callback),
				  window);
		g_object_set_data_full (G_OBJECT (menu_item), "uri", uri, (GDestroyNotify)gnome_vfs_uri_unref);

		gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), menu_item);

		uri = gnome_vfs_uri_get_parent (uri);
	}
	gtk_menu_set_screen (GTK_MENU (popup), gtk_widget_get_screen (widget));

	loop = g_main_loop_new (NULL, FALSE);

	g_signal_connect (popup, "deactivate",
			  G_CALLBACK (menu_deactivate_callback),
			  loop);

	gtk_grab_add (popup);
	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, menu_popup_pos, widget, 1, GDK_CURRENT_TIME);
	gtk_menu_shell_select_item (GTK_MENU_SHELL (popup), first_item);
	g_main_loop_run (loop);
	gtk_grab_remove (popup);
	g_main_loop_unref (loop);
 	gtk_object_sink (GTK_OBJECT (popup));
}

void
nautilus_spatial_window_set_location_button  (NautilusSpatialWindow *window,
					      const char            *location)
{
	GnomeVFSURI *uri;
	char *name;
	
	uri = NULL;
	if (location != NULL) {
		uri = gnome_vfs_uri_new (location);
	}
	if (uri != NULL) {
		name = nautilus_get_uri_shortname_for_display (uri);
		gtk_label_set_label (GTK_LABEL (window->details->location_label),
				     name);
		g_free (name);
		gtk_widget_set_sensitive (window->details->location_button, TRUE);
	} else {
		gtk_label_set_label (GTK_LABEL (window->details->location_label),
				     "");
		gtk_widget_set_sensitive (window->details->location_button, FALSE);
	}

	if (window->details->location != NULL) {
		gnome_vfs_uri_unref (window->details->location);
	}
	window->details->location = uri;
}

static void
nautilus_spatial_window_instance_init (NautilusSpatialWindow *window)
{
	GtkShadowType shadow_type;
	GtkWidget *frame;
	GtkRcStyle *rc_style;
	GtkWidget *arrow;
	GtkWidget *hbox;

	window->details = g_new0 (NautilusSpatialWindowDetails, 1);
	window->affect_spatial_window_on_next_location_change = TRUE;

	window->details->content_box = 
		gtk_hbox_new (FALSE, 0);
	gtk_widget_show (window->details->content_box);
	bonobo_window_set_contents (BONOBO_WINDOW (window), 
				    window->details->content_box);

	window->details->location_statusbar = gtk_statusbar_new ();
	gtk_widget_show (window->details->location_statusbar);
	gtk_widget_hide (GTK_STATUSBAR (window->details->location_statusbar)->frame);
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (window->details->location_statusbar), 
					   FALSE);

	window->details->location_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (window->details->location_button),
			       GTK_RELIEF_NONE);
	rc_style = gtk_widget_get_modifier_style (window->details->location_button);
	rc_style->xthickness = 0;
	rc_style->ythickness = 0;
	gtk_widget_modify_style (window->details->location_button, 
				 rc_style);

	gtk_widget_show (window->details->location_button);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (window->details->location_button), 
			   hbox);
	gtk_widget_show (hbox);
	
	window->details->location_label = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (hbox), window->details->location_label,
			    FALSE, FALSE, 0);
	gtk_widget_show (window->details->location_label);
	
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_show (arrow);

	frame = gtk_frame_new (NULL);
	gtk_widget_style_get (GTK_WIDGET (window->details->location_statusbar), 
			      "shadow_type", &shadow_type, NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), shadow_type);
	gtk_box_pack_start (GTK_BOX (window->details->location_statusbar), 
			    frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	gtk_container_add (GTK_CONTAINER (frame), 
			   window->details->location_button);
	
	gtk_widget_set_sensitive (window->details->location_button, FALSE);
	g_signal_connect (window->details->location_button, 
			  "clicked", 
			  G_CALLBACK (location_button_clicked_callback), window);
	gtk_widget_show (window->details->location_statusbar);
}

static void
nautilus_spatial_window_class_init (NautilusSpatialWindowClass *class)
{
	NAUTILUS_WINDOW_CLASS (class)->window_type = Nautilus_WINDOW_SPATIAL;

	G_OBJECT_CLASS (class)->finalize = nautilus_spatial_window_finalize;
	GTK_OBJECT_CLASS (class)->destroy = nautilus_spatial_window_destroy;
	GTK_WIDGET_CLASS (class)->show = nautilus_spatial_window_show;
	GTK_WIDGET_CLASS (class)->configure_event = nautilus_spatial_window_configure_event;
	GTK_WIDGET_CLASS (class)->unrealize = nautilus_spatial_window_unrealize;
	GTK_WIDGET_CLASS (class)->realize = nautilus_spatial_window_realize;

	NAUTILUS_WINDOW_CLASS (class)->prompt_for_location = 
		real_prompt_for_location;
	NAUTILUS_WINDOW_CLASS (class)->set_title = 
		real_set_title;
	NAUTILUS_WINDOW_CLASS (class)->merge_menus = 
		real_merge_menus;
	NAUTILUS_WINDOW_CLASS (class)->set_content_view_widget = 
		real_set_content_view_widget;
	NAUTILUS_WINDOW_CLASS (class)->close = 
		real_window_close;
	NAUTILUS_WINDOW_CLASS(class)->get_default_size = real_get_default_size;

	NAUTILUS_WINDOW_CLASS(class)->set_throbber_active =
		real_set_throbber_active;
}
