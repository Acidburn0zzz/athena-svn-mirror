/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 2000 Eazel, Inc.
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
 * Authors: Darin Adler <darin@bentspoon.com>
 */

#include <config.h>
#include "nautilus-desktop-window.h"

#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gtk/gtklayout.h>
#include <libgnome/gnome-macros.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libnautilus-private/nautilus-file-utilities.h>

struct NautilusDesktopWindowDetails {
	int dummy;
};

static void set_wmspec_desktop_hint (GdkWindow *window);

GNOME_CLASS_BOILERPLATE (NautilusDesktopWindow, nautilus_desktop_window,
			 NautilusWindow, NAUTILUS_TYPE_WINDOW)

static void
nautilus_desktop_window_instance_init (NautilusDesktopWindow *window)
{
	window->details = g_new0 (NautilusDesktopWindowDetails, 1);

	gtk_widget_set_size_request (GTK_WIDGET (window),
				     gdk_screen_width (),
				     gdk_screen_height ());

	gtk_window_move (GTK_WINDOW (window), 0, 0);

	/* shouldn't really be needed given our semantic type
	 * of _NET_WM_TYPE_DESKTOP, but why not
	 */
	gtk_window_set_resizable (GTK_WINDOW (window),
				  FALSE);
}

static void
nautilus_desktop_window_realized (NautilusDesktopWindow *window)
{
	/* Tuck the desktop windows xid in the root to indicate we own the desktop.
	 */
	Window window_xid;
	window_xid = GDK_WINDOW_XWINDOW (GTK_WIDGET (window)->window);
	gdk_property_change (NULL, gdk_atom_intern ("NAUTILUS_DESKTOP_WINDOW_ID", FALSE),
			     gdk_x11_xatom_to_atom (XA_WINDOW), 32,
			     PropModeReplace, (guchar *) &window_xid, 1);
}

static gint
nautilus_desktop_window_delete_event (NautilusDesktopWindow *window)
{
	/* Returning true tells GTK+ not to delete the window. */
	return TRUE;
}

void
nautilus_desktop_window_update_directory (NautilusDesktopWindow *window)
{
	char *desktop_directory_path;
	char *desktop_directory_uri;

	g_assert (NAUTILUS_IS_DESKTOP_WINDOW (window));

	desktop_directory_path = nautilus_get_desktop_directory ();
	
	desktop_directory_uri = gnome_vfs_get_uri_from_local_path (desktop_directory_path);
	g_free (desktop_directory_path);
	window->affect_desktop_on_next_location_change = TRUE;
	nautilus_window_go_to (NAUTILUS_WINDOW (window), desktop_directory_uri);
	g_free (desktop_directory_uri);
}

NautilusDesktopWindow *
nautilus_desktop_window_new (NautilusApplication *application)
{
	NautilusDesktopWindow *window;

	window = NAUTILUS_DESKTOP_WINDOW
		(gtk_widget_new (nautilus_desktop_window_get_type(),
				 "app", application,
				 "app_id", "nautilus",
				 NULL));

	/* Special sawmill setting*/
	gtk_window_set_wmclass (GTK_WINDOW (window), "desktop_window", "Nautilus");

	g_signal_connect (window, "realize", G_CALLBACK (nautilus_desktop_window_realized), NULL);
	g_signal_connect (window, "delete_event", G_CALLBACK (nautilus_desktop_window_delete_event), NULL);

	/* Point window at the desktop folder.
	 * Note that nautilus_desktop_window_init is too early to do this.
	 */
	nautilus_desktop_window_update_directory (window);

	return window;
}

static void
finalize (GObject *object)
{
	NautilusDesktopWindow *window;

	window = NAUTILUS_DESKTOP_WINDOW (object);

	gdk_property_delete (NULL, gdk_atom_intern ("NAUTILUS_DESKTOP_WINDOW_ID", TRUE));
	g_free (window->details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Disable this code for the time being, since its:
 * a) not working
 * b) crashes nautilus if the root window has a different
 *    depth than the nautilus window
 *
 * The idea behind the code is to grab the old background pixmap
 * and temporarily set it as the background for the nautilus window
 * to avoid flashing before the correct background has been set.
 */
#if 0
static void
set_gdk_window_background (GdkWindow *window,
			   gboolean   have_pixel,
			   Pixmap     pixmap,
			   gulong     pixel)
{
	Window w;

	w = GDK_WINDOW_XWINDOW (window);

	if (pixmap != None) {
		XSetWindowBackgroundPixmap (GDK_DISPLAY (), w,
					    pixmap);
	} else if (have_pixel) {
		XSetWindowBackground (GDK_DISPLAY (), w,
				      pixel);
	} else {
		XSetWindowBackgroundPixmap (GDK_DISPLAY (), w,
					    None);
	}
}

static void
set_window_background (GtkWidget *widget,
		       gboolean   already_have_root_bg,
		       gboolean   have_pixel,
		       Pixmap     pixmap,
		       gulong     pixel)
{
	Atom type;
	gulong nitems, bytes_after;
	gint format;
	guchar *data;
	
	/* Set the background to show the root window to avoid a flash that
	 * would otherwise occur.
	 */

	if (GTK_IS_WINDOW (widget)) {
		gtk_widget_set_app_paintable (widget, TRUE);
	}
	
	if (!already_have_root_bg) {
		have_pixel = FALSE;
		already_have_root_bg = TRUE;
		
		/* We want to do this round-trip-to-server work only
		 * for the first invocation, not on recursions
		 */
		
		XGetWindowProperty (GDK_DISPLAY (), GDK_ROOT_WINDOW (),
				    gdk_x11_get_xatom_by_name ("_XROOTPMAP_ID"),
				    0L, 1L, False, XA_PIXMAP,
				    &type, &format, &nitems, &bytes_after,
				    &data);
  
		if (type == XA_PIXMAP) {
			if (format == 32 && nitems == 1 && bytes_after == 0) {
				pixmap = *(Pixmap *) data;
			}
  
			XFree (data);
		}

		if (pixmap == None) {
			XGetWindowProperty (GDK_DISPLAY (), GDK_ROOT_WINDOW (),
					    gdk_x11_get_xatom_by_name ("_XROOTCOLOR_PIXEL"),
					    0L, 1L, False, AnyPropertyType,
					    &type, &format, &nitems, &bytes_after,
					    &data);
			
			if (type != None) {
				if (format == 32 && nitems == 1 && bytes_after == 0) {
					pixel = *(gulong *) data;
					have_pixel = TRUE;
				}
				
				XFree (data);
			}
		}
	}
	
	set_gdk_window_background (widget->window,
				   have_pixel, pixmap, pixel);
	
	if (GTK_IS_BIN (widget) &&
	    GTK_BIN (widget)->child) {
		/* Ensure we're realized */
		gtk_widget_realize (GTK_BIN (widget)->child);
		
		set_window_background (GTK_BIN (widget)->child,
				       already_have_root_bg, have_pixel,
				       pixmap, pixel);
	}

	/* For both parent and child, if it's a layout then set on the
	 * bin window as well.
	 */
	if (GTK_IS_LAYOUT (widget)) {
		set_gdk_window_background (GTK_LAYOUT (widget)->bin_window,
					   have_pixel,
					   pixmap, pixel);
	}
}
#endif
static void
map (GtkWidget *widget)
{
	/* Disable for now, see above for comments */
#if 0
	set_window_background (widget, FALSE, FALSE, None, 0);
#endif	
	/* Chain up to realize our children */
	GTK_WIDGET_CLASS (parent_class)->map (widget);
	gdk_window_lower (widget->window);
}


static void
set_wmspec_desktop_hint (GdkWindow *window)
{
        Atom atom;
        
        atom = XInternAtom (gdk_display,
                            "_NET_WM_WINDOW_TYPE_DESKTOP",
                            False);

        XChangeProperty (GDK_WINDOW_XDISPLAY (window),
                         GDK_WINDOW_XWINDOW (window),
                         XInternAtom (gdk_display,
                                      "_NET_WM_WINDOW_TYPE",
                                      False),
                         XA_ATOM, 32, PropModeReplace,
                         (guchar *) &atom, 1);
}


static void
realize (GtkWidget *widget)
{
	NautilusDesktopWindow *window;

	window = NAUTILUS_DESKTOP_WINDOW (widget);

	/* Make sure we get keyboard events */
	gtk_widget_set_events (widget, gtk_widget_get_events (widget) 
			      | GDK_KEY_PRESS_MASK | GDK_KEY_PRESS_MASK);
			      
	/* Do the work of realizing. */
	GTK_WIDGET_CLASS (parent_class)->realize (widget);

	/* This is the new way to set up the desktop window */
	set_wmspec_desktop_hint (widget->window);
}

static void
real_add_current_location_to_history_list (NautilusWindow *window)
{
	/* Do nothing. The desktop window's location should not
	 * show up in the history list.
	 */
}

static void
nautilus_desktop_window_class_init (NautilusDesktopWindowClass *class)
{
	G_OBJECT_CLASS (class)->finalize = finalize;
	GTK_WIDGET_CLASS (class)->realize = realize;


	GTK_WIDGET_CLASS (class)->map = map;

	NAUTILUS_WINDOW_CLASS (class)->add_current_location_to_history_list 
		= real_add_current_location_to_history_list;
}
