 /* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 1999, 2000, 2001 Eazel, Inc.
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
 */

/* This is the sidebar widget, which displays overview information
 * hosts individual panels for various views.
 */

#include <config.h>
#include "nautilus-sidebar.h"

#include "nautilus-sidebar-tabs.h"
#include "nautilus-sidebar-title.h"

#include <bonobo/bonobo-property-bag-client.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-exception.h>

#include <eel/eel-background.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-gtk-macros.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>
#include <eel/eel-vfs-extensions.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libxml/parser.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkpaned.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomevfs/gnome-vfs-application-registry.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libnautilus-private/nautilus-directory.h>
#include <libnautilus-private/nautilus-file-dnd.h>
#include <libnautilus-private/nautilus-file-operations.h>
#include <libnautilus-private/nautilus-file.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-keep-last-vertical-box.h>
#include <libnautilus-private/nautilus-metadata.h>
#include <libnautilus-private/nautilus-mime-actions.h>
#include <libnautilus-private/nautilus-program-choosing.h>
#include <libnautilus-private/nautilus-sidebar-functions.h>
#include <libnautilus-private/nautilus-theme.h>
#include <libnautilus-private/nautilus-trash-monitor.h>
#include <math.h>

struct NautilusSidebarDetails {
	GtkVBox *container;
	NautilusSidebarTitle *title;
	GtkNotebook *notebook;
	NautilusSidebarTabs *sidebar_tabs;
	NautilusSidebarTabs *title_tab;
	GtkHBox *button_box_centerer;
	GtkVBox *button_box;
	gboolean has_buttons;
	char *uri;
	NautilusFile *file;
	guint file_changed_connection;
	int selected_index;
	gboolean background_connected;
	int old_width;

	char *default_background_color;
	char *default_background_image;
	char *current_background_color;
	char *current_background_image;
	gboolean is_default_background;
};

/* button assignments */
#define CONTEXTUAL_MENU_BUTTON 3

static void     nautilus_sidebar_class_init      (GtkObjectClass   *object_klass);
static void     nautilus_sidebar_init            (GtkObject        *object);
static void     nautilus_sidebar_deactivate_panel      (NautilusSidebar  *sidebar);
static gboolean nautilus_sidebar_press_event           (GtkWidget        *widget,
							GdkEventButton   *event);
static gboolean nautilus_sidebar_release_event         (GtkWidget        *widget,
						        GdkEventButton   *event);
static gboolean nautilus_sidebar_leave_event           (GtkWidget        *widget,
							GdkEventCrossing *event);
static gboolean nautilus_sidebar_motion_event          (GtkWidget        *widget,
							GdkEventMotion   *event);
static void     nautilus_sidebar_destroy               (GtkObject        *object);
static void     nautilus_sidebar_finalize              (GObject          *object);
static void     nautilus_sidebar_drag_data_received    (GtkWidget        *widget,
							GdkDragContext   *context,
							int               x,
							int               y,
							GtkSelectionData *selection_data,
							guint             info,
							guint             time);
static void     nautilus_sidebar_read_theme            (NautilusSidebar  *sidebar);
static void     nautilus_sidebar_size_allocate         (GtkWidget        *widget,
							GtkAllocation    *allocation);
static void     nautilus_sidebar_style_set             (GtkWidget        *widget,
							GtkStyle         *previous_style);
static void     nautilus_sidebar_theme_changed         (gpointer          user_data);
static void     nautilus_sidebar_confirm_trash_changed (gpointer          user_data);
static void     nautilus_sidebar_update_appearance     (NautilusSidebar  *sidebar);
static void     nautilus_sidebar_update_buttons        (NautilusSidebar  *sidebar);
static void     add_command_buttons                    (NautilusSidebar  *sidebar,
							GList            *application_list);
static void     background_metadata_changed_callback   (NautilusSidebar  *sidebar);

#define DEFAULT_TAB_COLOR "#999999"

/* FIXME bugzilla.gnome.org 41245: hardwired sizes */
#define SIDEBAR_MINIMUM_WIDTH 1
#define SIDEBAR_MINIMUM_HEIGHT 400

/* Some auto-updated values */
static int      sidebar_width_auto_value = SIDEBAR_MINIMUM_WIDTH;
static gboolean confirm_trash_auto_value = TRUE;

enum {
	LOCATION_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* drag and drop definitions */

enum {
	TARGET_URI_LIST,
	TARGET_COLOR,
	TARGET_BGIMAGE,
	TARGET_KEYWORD,
	TARGET_BACKGROUND_RESET,
	TARGET_GNOME_URI_LIST
};

static GtkTargetEntry target_table[] = {
	{ "text/uri-list",  0, TARGET_URI_LIST },
	{ "application/x-color", 0, TARGET_COLOR },
	{ "property/bgimage", 0, TARGET_BGIMAGE },
	{ "property/keyword", 0, TARGET_KEYWORD },
	{ "x-special/gnome-reset-background", 0, TARGET_BACKGROUND_RESET },	
	{ "x-special/gnome-icon-list",  0, TARGET_GNOME_URI_LIST }
};

typedef enum {
	NO_PART,
	BACKGROUND_PART,
	ICON_PART,
	TITLE_TAB_PART,
	TABS_PART
} SidebarPart;

EEL_CLASS_BOILERPLATE (NautilusSidebar, nautilus_sidebar, EEL_TYPE_BACKGROUND_BOX)

/* initializing the class object by installing the operations we override */
static void
nautilus_sidebar_class_init (GtkObjectClass *object_klass)
{
	GtkWidgetClass *widget_class;
	GObjectClass *gobject_class;
	
	NautilusSidebarClass *klass;

	widget_class = GTK_WIDGET_CLASS (object_klass);
	klass = NAUTILUS_SIDEBAR_CLASS (object_klass);
	gobject_class = G_OBJECT_CLASS (object_klass);
	
	gobject_class->finalize = nautilus_sidebar_finalize;

	object_klass->destroy = nautilus_sidebar_destroy;
	
	widget_class->drag_data_received  = nautilus_sidebar_drag_data_received;
	widget_class->motion_notify_event = nautilus_sidebar_motion_event;
	widget_class->leave_notify_event = nautilus_sidebar_leave_event;
	widget_class->button_press_event  = nautilus_sidebar_press_event;
	widget_class->button_release_event  = nautilus_sidebar_release_event;
	widget_class->size_allocate = nautilus_sidebar_size_allocate;
	widget_class->style_set = nautilus_sidebar_style_set;

	/* add the "location changed" signal */
	signals[LOCATION_CHANGED] = g_signal_new
		("location_changed",
		 G_TYPE_FROM_CLASS (object_klass),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET (NautilusSidebarClass,
				    location_changed),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__STRING,
		 G_TYPE_NONE, 1, G_TYPE_STRING);
}

/* utility routine to allocate the box the holds the command buttons */
static void
make_button_box (NautilusSidebar *sidebar)
{
	sidebar->details->button_box_centerer = GTK_HBOX (gtk_hbox_new (FALSE, 0));
	gtk_box_pack_start_defaults (GTK_BOX (sidebar->details->container),
			    	     GTK_WIDGET (sidebar->details->button_box_centerer));

	sidebar->details->button_box = GTK_VBOX (nautilus_keep_last_vertical_box_new (GNOME_PAD_SMALL));
	gtk_container_set_border_width (GTK_CONTAINER (sidebar->details->button_box), GNOME_PAD);				
	gtk_widget_show (GTK_WIDGET (sidebar->details->button_box));
	gtk_box_pack_start (GTK_BOX (sidebar->details->button_box_centerer),
			    GTK_WIDGET (sidebar->details->button_box),
			    TRUE, FALSE, 0);
	sidebar->details->has_buttons = FALSE;
}

/* initialize the instance's fields, create the necessary subviews, etc. */

static void
nautilus_sidebar_init (GtkObject *object)
{
	GtkWidget *widget;
	static gboolean setup_autos = FALSE;
	NautilusSidebar *sidebar;
	
	sidebar = NAUTILUS_SIDEBAR (object);
	widget = GTK_WIDGET (object);

	sidebar->details = g_new0 (NautilusSidebarDetails, 1);

	if (!setup_autos) {
		setup_autos = TRUE;
		eel_preferences_add_auto_integer (
			NAUTILUS_PREFERENCES_SIDEBAR_WIDTH,
			&sidebar_width_auto_value);

		eel_preferences_add_auto_boolean (
			NAUTILUS_PREFERENCES_CONFIRM_TRASH,
			&confirm_trash_auto_value);
	}
	
	/* set the requested size of the sidebar */
	gtk_widget_set_size_request (widget, sidebar_width_auto_value,
				     SIDEBAR_MINIMUM_HEIGHT);
	sidebar->details->old_width = sidebar_width_auto_value;

	/* load the default background from the current theme */
	nautilus_sidebar_read_theme (sidebar);

	/* enable mouse tracking */
	gtk_widget_add_events (GTK_WIDGET (sidebar), GDK_POINTER_MOTION_MASK);
	  	
	/* create the container box */
  	sidebar->details->container = GTK_VBOX (gtk_vbox_new (FALSE, 0));
	gtk_container_set_border_width (GTK_CONTAINER (sidebar->details->container), 0);				
	gtk_widget_show (GTK_WIDGET (sidebar->details->container));
	gtk_container_add (GTK_CONTAINER (sidebar),
			   GTK_WIDGET (sidebar->details->container));

	/* allocate and install the index title widget */ 
	sidebar->details->title = NAUTILUS_SIDEBAR_TITLE (nautilus_sidebar_title_new ());
	gtk_widget_show (GTK_WIDGET (sidebar->details->title));
	gtk_box_pack_start (GTK_BOX (sidebar->details->container),
			    GTK_WIDGET (sidebar->details->title),
			    FALSE, FALSE, GNOME_PAD);
	
	/* allocate the index tabs */
	sidebar->details->sidebar_tabs = NAUTILUS_SIDEBAR_TABS (nautilus_sidebar_tabs_new ());
	sidebar->details->selected_index = -1;

	/* also, allocate the title tab */
	sidebar->details->title_tab = NAUTILUS_SIDEBAR_TABS (nautilus_sidebar_tabs_new ());
	g_object_ref (sidebar->details->title_tab);
	gtk_object_sink (GTK_OBJECT (sidebar->details->title_tab));

	nautilus_sidebar_tabs_set_title_mode (sidebar->details->title_tab, TRUE);	
	
	gtk_widget_show (GTK_WIDGET (sidebar->details->sidebar_tabs));
	gtk_box_pack_end (GTK_BOX (sidebar->details->container),
			  GTK_WIDGET (sidebar->details->sidebar_tabs),
			  FALSE, FALSE, 0);
	
	/* allocate and install the panel tabs */
  	sidebar->details->notebook = GTK_NOTEBOOK (gtk_notebook_new ());
	g_object_ref (sidebar->details->notebook);
	gtk_object_sink (GTK_OBJECT (sidebar->details->notebook));
		
	gtk_notebook_set_show_tabs (sidebar->details->notebook, FALSE);
	
	/* allocate and install the command button container */
	make_button_box (sidebar);

	/* add a callback for when the theme changes */
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_THEME, nautilus_sidebar_theme_changed, sidebar);

	/* add a callback for when the preference whether to confirm trashing/deleting file changes */
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_CONFIRM_TRASH, nautilus_sidebar_confirm_trash_changed, sidebar);

	/* prepare ourselves to receive dropped objects */
	gtk_drag_dest_set (GTK_WIDGET (sidebar),
			   GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP, 
			   target_table, G_N_ELEMENTS (target_table),
			   GDK_ACTION_COPY | GDK_ACTION_MOVE);
}

static void
nautilus_sidebar_destroy (GtkObject *object)
{
	NautilusSidebar *sidebar;

	sidebar = NAUTILUS_SIDEBAR (object);

	if (sidebar->details->notebook != NULL) {
		g_object_unref (sidebar->details->notebook);
		sidebar->details->notebook = NULL;
	}
	if (sidebar->details->title_tab != NULL) {
		g_object_unref (sidebar->details->title_tab);
		sidebar->details->title_tab = NULL;
	}
	
	EEL_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
nautilus_sidebar_finalize (GObject *object)
{
	NautilusSidebar *sidebar;

	sidebar = NAUTILUS_SIDEBAR (object);

	if (sidebar->details->file != NULL) {
		nautilus_file_monitor_remove (sidebar->details->file, sidebar);
		nautilus_file_unref (sidebar->details->file);
	}
	
	g_free (sidebar->details->uri);
	g_free (sidebar->details->default_background_color);
	g_free (sidebar->details->default_background_image);
	g_free (sidebar->details->current_background_color);
	g_free (sidebar->details->current_background_image);
	g_free (sidebar->details);
		
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_THEME,
					 nautilus_sidebar_theme_changed,
					 sidebar);

	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_CONFIRM_TRASH,
					 nautilus_sidebar_confirm_trash_changed,
					 sidebar);


	EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/* callback to handle resetting the background */
static void
reset_background_callback (GtkWidget *menu_item, GtkWidget *sidebar)
{
	EelBackground *background;

	background = eel_get_widget_background (sidebar);
	if (background != NULL) { 
		eel_background_reset (background); 
	}
}

static const char *
get_page_iid (NautilusSidebar *sidebar,
	      int page_number)
{
	GtkWidget *page;
	
	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (sidebar->details->notebook),
					  page_number);
	if (page == NULL) {
		return NULL;
	}
	return nautilus_view_frame_get_view_iid (NAUTILUS_VIEW_FRAME (page));
}

/* utility routine that checks if the active panel matches the passed-in object id */
static gboolean
nautilus_sidebar_active_panel_matches_id_or_is_damaged (NautilusSidebar *sidebar, const char *id)
{
	const char *current_iid;
	
	if (sidebar->details->selected_index < 0) {
		return FALSE;
	}
	
	/* if we can't get the active one, say yes to removing it, to make sure to
	 * remove the tab
	 */
	current_iid = get_page_iid (sidebar, sidebar->details->selected_index);
	if (current_iid == NULL) {
		return TRUE;
	}
	return eel_strcmp (current_iid, id) == 0;	
}

/* if the active panel matches the passed in id, hide it. */
void
nautilus_sidebar_hide_active_panel_if_matches (NautilusSidebar *sidebar, const char *sidebar_id)
{
	if (nautilus_sidebar_active_panel_matches_id_or_is_damaged (sidebar, sidebar_id)) {
		nautilus_sidebar_deactivate_panel (sidebar);
	}
}

static gboolean
any_panel_matches_iid (NautilusSidebar *sidebar, const char *id)
{
	int i, count;
	const char *page_iid;

	count = g_list_length (GTK_NOTEBOOK (sidebar->details->notebook)->children);
	for (i = 0; i < count; i++) {
		page_iid = get_page_iid (sidebar, i);
		if (page_iid != NULL && strcmp (page_iid, id) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

/* callback for sidebar panel menu items to toggle their visibility */
static void
toggle_sidebar_panel (GtkWidget *widget,
		      const char *sidebar_iid)
{
 	NautilusSidebar *sidebar;
	const char *preference_key;
	gboolean already_on;

	g_return_if_fail (GTK_IS_CHECK_MENU_ITEM (widget));
	g_return_if_fail (NAUTILUS_IS_SIDEBAR (g_object_get_data (G_OBJECT (widget), "user_data")));
	g_return_if_fail (g_object_get_data (G_OBJECT (widget), "nautilus-sidebar/preference-key") != NULL);

	preference_key = g_object_get_data (G_OBJECT (widget), "nautilus-sidebar/preference-key");

	sidebar = NAUTILUS_SIDEBAR (g_object_get_data (G_OBJECT (widget), "user_data"));

	nautilus_sidebar_hide_active_panel_if_matches (sidebar, sidebar_iid);
	
	already_on = any_panel_matches_iid (sidebar, sidebar_iid);
	
	/* This little dance gets the preferences code to send a
	 * notification even though it thinks there's "no change".
	 *
	 * This is needed to deal with situations when the display
	 * become out of whack with the number of running sidebar
	 * panels, for example when a panel crashes.
	 */
	eel_preferences_set_boolean (preference_key, already_on);
	eel_preferences_set_boolean (preference_key, !already_on);
}

typedef struct
{
	NautilusSidebar *sidebar;
	GtkMenu *menu;
} ForEachPanelData;

static void
sidebar_for_each_sidebar_panel (const char *name,
				const char *iid,
				const char *preference_key,
				gpointer callback_data) 
{
	ForEachPanelData *data;
	GtkWidget *menu_item;
	gboolean panel_visible;

	g_return_if_fail (name != NULL);
	g_return_if_fail (iid != NULL);
	g_return_if_fail (preference_key != NULL);
	g_return_if_fail (callback_data != NULL);

	data = callback_data;

	g_return_if_fail (NAUTILUS_IS_SIDEBAR (data->sidebar));
	g_return_if_fail (GTK_IS_MENU (data->menu));

	/* If the panel is not visible in the current user level, then
	 * we dont need to create a menu item for it.
	 */
	panel_visible = any_panel_matches_iid (data->sidebar, iid);
	if (!panel_visible && !eel_preferences_is_visible (preference_key)) {
		return;
	}

	/* add a check menu item */
	menu_item = gtk_check_menu_item_new_with_label (name);

	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), panel_visible);
	gtk_widget_show (menu_item);
	g_object_set_data (G_OBJECT (menu_item), "user_data", data->sidebar);
	gtk_menu_shell_append (GTK_MENU_SHELL (data->menu), menu_item);
	g_signal_connect_data (menu_item, "activate",
			       G_CALLBACK (toggle_sidebar_panel),
			       g_strdup (iid), (GClosureNotify) g_free, 0);

	g_object_set_data_full (G_OBJECT (menu_item),
				"nautilus-sidebar/preference-key",
				g_strdup (preference_key), g_free);
}

/* utility routine to add a menu item for each potential sidebar panel */
static void
sidebar_add_panel_context_menu_items (NautilusSidebar *sidebar,
				      GtkWidget *menu)
{
	ForEachPanelData data;

	data.sidebar = sidebar;
	data.menu = GTK_MENU (menu);

	nautilus_sidebar_for_each_panel (sidebar_for_each_sidebar_panel, &data);
}

/* create the context menu */
GtkWidget *
nautilus_sidebar_create_context_menu (NautilusSidebar *sidebar)
{
	GtkWidget *menu, *menu_item;
	EelBackground *background;
	gboolean has_background;

	background = eel_get_widget_background (GTK_WIDGET(sidebar));
	has_background = background && !sidebar->details->is_default_background;

	menu = gtk_menu_new ();
	
	/* add the sidebar panels */
	sidebar_add_panel_context_menu_items (sidebar, menu);

	/* add a separator */
	menu_item = gtk_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	/* add the reset background item, possibly disabled */
	menu_item = gtk_menu_item_new_with_mnemonic (_("Use _Default Background"));
 	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
        gtk_widget_set_sensitive (menu_item, has_background);
	g_signal_connect_object (menu_item, "activate",
				 G_CALLBACK (reset_background_callback), sidebar, 0);

	return menu;
}

/* create a new instance */
NautilusSidebar *
nautilus_sidebar_new (void)
{
	return NAUTILUS_SIDEBAR (gtk_widget_new (nautilus_sidebar_get_type (), NULL));
}

/* utility routine to handle mapping local file names to a uri */
static char*
map_local_data_file (char *file_name)
{
	char *temp_str;
	if (file_name && !eel_istr_has_prefix (file_name, "file://")) {

		if (eel_str_has_prefix (file_name, "./")) {
			temp_str = nautilus_theme_get_image_path (file_name + 2);
		} else {
			temp_str = g_strdup_printf ("%s/%s", NAUTILUS_DATADIR, file_name);
		}
		
		g_free (file_name);
		file_name = gnome_vfs_get_uri_from_local_path (temp_str);
		g_free (temp_str);
	}
	return file_name;
}

/* read the theme file and set up the default backgrounds and images accordingly */
static void
nautilus_sidebar_read_theme (NautilusSidebar *sidebar)
{
	char *background_color, *background_image;
	
	background_color = nautilus_theme_get_theme_data ("sidebar", NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_COLOR);
	background_image = nautilus_theme_get_theme_data ("sidebar", NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_IMAGE);
	
	g_free (sidebar->details->default_background_color);
	sidebar->details->default_background_color = NULL;
	g_free (sidebar->details->default_background_image);
	sidebar->details->default_background_image = NULL;
			
	if (background_color && strlen (background_color)) {
		sidebar->details->default_background_color = g_strdup (background_color);
	}
			
	/* set up the default background image */
	
	background_image = map_local_data_file (background_image);
	if (background_image && strlen (background_image)) {
		sidebar->details->default_background_image = g_strdup (background_image);
	}

	g_free (background_color);
	g_free (background_image);
}

/* handler for handling theme changes */

static void
nautilus_sidebar_theme_changed (gpointer user_data)
{
	NautilusSidebar *sidebar;
	
	sidebar = NAUTILUS_SIDEBAR (user_data);
	nautilus_sidebar_read_theme (sidebar);
	nautilus_sidebar_update_appearance (sidebar);
	gtk_widget_queue_draw (GTK_WIDGET (sidebar)) ;	
}

/* handler for handling confirming trash preferences changes */

static void
nautilus_sidebar_confirm_trash_changed (gpointer user_data)
{
	NautilusSidebar *sidebar;
	
	sidebar = NAUTILUS_SIDEBAR (user_data);
	nautilus_sidebar_update_buttons (sidebar);
}

/* hit testing */

static SidebarPart
hit_test (NautilusSidebar *sidebar,
	  int x, int y)
{
	if (eel_point_in_widget (GTK_WIDGET (sidebar->details->sidebar_tabs), x, y)) {
		return TABS_PART;
	}
	
	if (eel_point_in_widget (GTK_WIDGET (sidebar->details->title_tab), x, y)) {
		return TITLE_TAB_PART;
	}
	
	if (nautilus_sidebar_title_hit_test_icon (sidebar->details->title, x, y)) {
		return ICON_PART;
	}
	
	if (eel_point_in_widget (GTK_WIDGET (sidebar), x, y)) {
		return BACKGROUND_PART;
	}

	return NO_PART;
}

/* utility to test if a uri refers to a local image */
static gboolean
uri_is_local_image (const char *uri)
{
	GdkPixbuf *pixbuf;
	char *image_path;
	
	image_path = gnome_vfs_get_local_path_from_uri (uri);
	if (image_path == NULL) {
		return FALSE;
	}

	pixbuf = gdk_pixbuf_new_from_file (image_path, NULL);
	g_free (image_path);
	
	if (pixbuf == NULL) {
		return FALSE;
	}
	g_object_unref (pixbuf);
	return TRUE;
}

static void
receive_dropped_uri_list (NautilusSidebar *sidebar,
			  int x, int y,
			  GtkSelectionData *selection_data)
{
	char **uris;
	gboolean exactly_one;
	GtkWindow *window;
	
	uris = g_strsplit (selection_data->data, "\r\n", 0);
	exactly_one = uris[0] != NULL && (uris[1] == NULL || uris[1][0] == '\0');
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (sidebar)));
	
	switch (hit_test (sidebar, x, y)) {
	case NO_PART:
	case BACKGROUND_PART:
		/* FIXME bugzilla.gnome.org 42507: Does this work for all images, or only background images?
		 * Other views handle background images differently from other URIs.
		 */
		if (exactly_one && uri_is_local_image (uris[0])) {
			eel_background_receive_dropped_background_image
				(eel_get_widget_background (GTK_WIDGET (sidebar)),
				 uris[0]);
		} else if (exactly_one) {
			g_signal_emit (sidebar,
					 signals[LOCATION_CHANGED], 0,
			 		 uris[0]);	
		}
		break;
	case TABS_PART:
	case TITLE_TAB_PART:
		break;
	case ICON_PART:
		/* handle images dropped on the logo specially */
		
		if (!exactly_one) {
			eel_show_error_dialog (
				_("You can't assign more than one custom icon at a time! "
				  "Please drag just one image to set a custom icon."), 
				_("More Than One Image"),
				window);
			break;
		}
		
		if (uri_is_local_image (uris[0])) {
			if (sidebar->details->file != NULL) {
				nautilus_file_set_metadata (sidebar->details->file,
							    NAUTILUS_METADATA_KEY_CUSTOM_ICON,
							    NULL,
							    uris[0]);
				nautilus_file_set_metadata (sidebar->details->file,
							    NAUTILUS_METADATA_KEY_ICON_SCALE,
							    NULL,
							    NULL);
			}
		} else {	
			if (eel_is_remote_uri (uris[0])) {
				eel_show_error_dialog (
					_("The file that you dropped is not local.  "
					  "You can only use local images as custom icons."), 
					_("Local Images Only"),
					window);
			
			} else {
				eel_show_error_dialog (
					_("The file that you dropped is not an image.  "
					  "You can only use local images as custom icons."),
					_("Images Only"),
					window);
			}
		}	
		break;
	}

	g_strfreev (uris);
}

static void
receive_dropped_color (NautilusSidebar *sidebar,
		       int x, int y,
		       GtkSelectionData *selection_data)
{
	guint16 *channels;
	char *color_spec;

	if (selection_data->length != 8 || selection_data->format != 16) {
		g_warning ("received invalid color data");
		return;
	}
	
	channels = (guint16 *) selection_data->data;
	color_spec = g_strdup_printf ("#%02X%02X%02X", channels[0] >> 8, channels[1] >> 8, channels[2] >> 8);

	switch (hit_test (sidebar, x, y)) {
	case NO_PART:
		g_warning ("dropped color, but not on any part of sidebar");
		break;
	case TABS_PART:
		/* color dropped on main tabs */
		nautilus_sidebar_tabs_receive_dropped_color
			(sidebar->details->sidebar_tabs,
			 x, y, selection_data);

		/* Block so we don't respond to our own metadata changes.
		 */
		g_signal_handlers_block_by_func (sidebar->details->file,
						 G_CALLBACK (background_metadata_changed_callback),
						 sidebar);
						  
		nautilus_file_set_metadata
			(sidebar->details->file,
			 NAUTILUS_METADATA_KEY_SIDEBAR_TAB_COLOR,
			 DEFAULT_TAB_COLOR,
			 color_spec);

		g_signal_handlers_unblock_by_func (sidebar->details->file,
						   G_CALLBACK (background_metadata_changed_callback),
						   sidebar);
		break;
	case TITLE_TAB_PART:
		/* color dropped on title tab */
		nautilus_sidebar_tabs_receive_dropped_color
			(sidebar->details->title_tab,
			 x, y, selection_data);
		
		/* Block so we don't respond to our own metadata changes.
		 */
		g_signal_handlers_block_by_func (sidebar->details->file,
						 G_CALLBACK (background_metadata_changed_callback),
						 sidebar);

		nautilus_file_set_metadata
			(sidebar->details->file,
			 NAUTILUS_METADATA_KEY_SIDEBAR_TITLE_TAB_COLOR,
			 DEFAULT_TAB_COLOR,
			 color_spec);

		g_signal_handlers_unblock_by_func (sidebar->details->file,
						   G_CALLBACK (background_metadata_changed_callback),
						   sidebar);
		break;
	case ICON_PART:
	case BACKGROUND_PART:
		/* Let the background change based on the dropped color. */
		eel_background_receive_dropped_color
			(eel_get_widget_background (GTK_WIDGET (sidebar)),
			 GTK_WIDGET (sidebar), x, y, selection_data);
		break;
	}
	g_free(color_spec);
}

/* handle receiving a dropped keyword */

static void
receive_dropped_keyword (NautilusSidebar *sidebar,
			 int x, int y,
			 GtkSelectionData *selection_data)
{
	nautilus_drag_file_receive_dropped_keyword (sidebar->details->file, selection_data->data);
	
	/* regenerate the display */
	nautilus_sidebar_update_appearance (sidebar);  	
}

static void  
nautilus_sidebar_drag_data_received (GtkWidget *widget, GdkDragContext *context,
					 int x, int y,
					 GtkSelectionData *selection_data,
					 guint info, guint time)
{
	NautilusSidebar *sidebar;
	EelBackground *background;

	g_return_if_fail (NAUTILUS_IS_SIDEBAR (widget));

	sidebar = NAUTILUS_SIDEBAR (widget);

	switch (info) {
	case TARGET_GNOME_URI_LIST:
	case TARGET_URI_LIST:
		receive_dropped_uri_list (sidebar, x, y, selection_data);
		break;
	case TARGET_COLOR:
		receive_dropped_color (sidebar, x, y, selection_data);
		break;
	case TARGET_BGIMAGE:
		if (hit_test (sidebar, x, y) == BACKGROUND_PART)
			receive_dropped_uri_list (sidebar, x, y, selection_data);
		break;	
	case TARGET_BACKGROUND_RESET:
		background = eel_get_widget_background ( GTK_WIDGET (sidebar));
		if (background != NULL) { 
			eel_background_reset (background); 
		}
		break;
	case TARGET_KEYWORD:
		receive_dropped_keyword (sidebar, x, y, selection_data);
		break;
	default:
		g_warning ("unknown drop type");
	}
}

static void
view_loaded_callback (NautilusViewFrame *view_frame, gpointer user_data)
{
	NautilusSidebar *sidebar;
	
	sidebar = NAUTILUS_SIDEBAR (user_data);	
	nautilus_sidebar_tabs_connect_view (sidebar->details->sidebar_tabs, GTK_WIDGET (view_frame));
}

/* add a new panel to the sidebar */
void
nautilus_sidebar_add_panel (NautilusSidebar *sidebar, NautilusViewFrame *panel)
{
	GtkWidget *label;
	char *description;
	int page_num;
	
	g_return_if_fail (NAUTILUS_IS_SIDEBAR (sidebar));
	g_return_if_fail (NAUTILUS_IS_VIEW_FRAME (panel));
	
	description = nautilus_view_frame_get_label (panel);

	label = gtk_label_new (description);

	gtk_widget_show (label);
	
	g_signal_connect_object (panel, "view_loaded",
				 G_CALLBACK (view_loaded_callback), sidebar, 0);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (sidebar->details->notebook),
				  GTK_WIDGET (panel), label);
	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (sidebar->details->notebook),
					  GTK_WIDGET (panel));

	/* tell the index tabs about it */
	nautilus_sidebar_tabs_add_view (sidebar->details->sidebar_tabs,
					description, GTK_WIDGET (panel), page_num);
	
	g_free (description);

	gtk_widget_show (GTK_WIDGET (panel));
}

/* remove the passed-in panel from the sidebar */
void
nautilus_sidebar_remove_panel (NautilusSidebar *sidebar,
				       NautilusViewFrame *panel)
{
	int page_num;
	char *description;
	
	g_return_if_fail (NAUTILUS_IS_VIEW_FRAME (panel));

	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (sidebar->details->notebook),
					  GTK_WIDGET (panel));
	description = nautilus_view_frame_get_label (panel);

	if (page_num >= 0) {
		gtk_notebook_remove_page (GTK_NOTEBOOK (sidebar->details->notebook),
				  page_num);
	}
	
	/* Remove the tab associated with this panel */
	nautilus_sidebar_tabs_remove_view (sidebar->details->sidebar_tabs, description);
	if (page_num <= sidebar->details->selected_index) {
		sidebar->details->selected_index -= 1;
	}
	g_free (description);
}

static void
notify_current_sidebar_view (NautilusSidebar *sidebar, const char *property, gboolean value)
{
	CORBA_Environment ev;
	Bonobo_PropertyBag property_bag;
	Bonobo_Control control;
	GtkWidget *notebook_page;
	
	notebook_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (sidebar->details->notebook),
						   sidebar->details->selected_index);
	control = nautilus_view_frame_get_control (NAUTILUS_VIEW_FRAME (notebook_page));	
	if (control != CORBA_OBJECT_NIL) {
		CORBA_exception_init (&ev);
		property_bag = Bonobo_Control_getProperties (control, &ev);
		if (!BONOBO_EX (&ev) && property_bag != CORBA_OBJECT_NIL) {
			bonobo_property_bag_client_set_value_gboolean (property_bag, property, value, &ev);
			bonobo_object_release_unref (property_bag, NULL);
		}
		CORBA_exception_free (&ev);
	}
}

/* utility to activate the panel corresponding to the passed in index  */
static void
nautilus_sidebar_activate_panel (NautilusSidebar *sidebar, int which_view)
{
	char *title;
	GtkNotebook *notebook;

	/* nothing to do if it's already active */
	if (sidebar->details->selected_index == which_view) {
		return;
	}
	
	notebook = sidebar->details->notebook;
	if (sidebar->details->selected_index < 0) {
		gtk_widget_show (GTK_WIDGET (notebook));
		if (GTK_WIDGET (notebook)->parent == NULL) {
			gtk_box_pack_end (GTK_BOX (sidebar->details->container),
					  GTK_WIDGET (notebook),
					  TRUE, TRUE, 0);
		}
		
		gtk_widget_show (GTK_WIDGET (sidebar->details->title_tab));
		if (GTK_WIDGET (sidebar->details->title_tab)->parent == NULL) {
			gtk_box_pack_end (GTK_BOX (sidebar->details->container),
					  GTK_WIDGET (sidebar->details->title_tab),
					  FALSE, FALSE, 0);
		}
	} else {
		notify_current_sidebar_view (sidebar, "close", TRUE);
	}
	
	sidebar->details->selected_index = which_view;
	title = nautilus_sidebar_tabs_get_title_from_index (sidebar->details->sidebar_tabs,
							  which_view);
	nautilus_sidebar_tabs_set_title (sidebar->details->title_tab, title);
	nautilus_sidebar_tabs_prelight_tab (sidebar->details->title_tab, -1);
    
	g_free (title);
	
	/* hide the buttons, since they look confusing when partially overlapped */
	gtk_widget_hide (GTK_WIDGET (sidebar->details->button_box_centerer));
	gtk_widget_hide (GTK_WIDGET (sidebar->details->title));
	
	gtk_notebook_set_current_page (notebook, which_view);
	notify_current_sidebar_view (sidebar, "close", FALSE);
}

/* utility to deactivate the active panel */
static void
nautilus_sidebar_deactivate_panel (NautilusSidebar *sidebar)
{
	if (sidebar->details->selected_index >= 0) {
		gtk_widget_hide (GTK_WIDGET (sidebar->details->notebook));
		gtk_widget_hide (GTK_WIDGET (sidebar->details->title_tab));
		notify_current_sidebar_view (sidebar, "close", TRUE);
	}
	
	gtk_widget_show (GTK_WIDGET (sidebar->details->button_box_centerer));
	gtk_widget_show (GTK_WIDGET (sidebar->details->title));
	sidebar->details->selected_index = -1;
	nautilus_sidebar_tabs_select_tab (sidebar->details->sidebar_tabs, -1);
}

/* handle mouse motion events by passing it to the tabs if necessary for pre-lighting */
static gboolean
nautilus_sidebar_motion_event (GtkWidget *widget, GdkEventMotion *event)
{
	int x, y;
	int which_tab;
	int title_top, title_bottom;
	NautilusSidebar *sidebar;
	NautilusSidebarTabs *sidebar_tabs, *title_tab;

	sidebar = NAUTILUS_SIDEBAR (widget);

	gtk_widget_get_pointer(widget, &x, &y);
	
	/* if the motion is in the main tabs, tell them about it */
	sidebar_tabs = sidebar->details->sidebar_tabs;
	if (y >= GTK_WIDGET (sidebar_tabs)->allocation.y) {
		which_tab = nautilus_sidebar_tabs_hit_test (sidebar_tabs, x, y);
		nautilus_sidebar_tabs_prelight_tab (sidebar_tabs, which_tab);
	} else
		nautilus_sidebar_tabs_prelight_tab (sidebar_tabs, -1);
	

	/* also handle prelighting in the title tab if necessary */
	if (sidebar->details->selected_index >= 0) {
		title_tab = sidebar->details->title_tab;
		title_top = GTK_WIDGET (title_tab)->allocation.y;
		title_bottom = title_top + GTK_WIDGET (title_tab)->allocation.height;
		if (y >= title_top && y < title_bottom) {
			which_tab = nautilus_sidebar_tabs_hit_test (title_tab, x, y);
		} else {
			which_tab = -1;
		}
		nautilus_sidebar_tabs_prelight_tab (title_tab, which_tab);
	}

	return TRUE;
}

/* handle the leave event by turning off the preliting */

static gboolean
nautilus_sidebar_leave_event (GtkWidget *widget, GdkEventCrossing *event)
{
	NautilusSidebar *sidebar;
	NautilusSidebarTabs *sidebar_tabs;

	sidebar = NAUTILUS_SIDEBAR (widget);
	sidebar_tabs = sidebar->details->sidebar_tabs; 
	nautilus_sidebar_tabs_prelight_tab (sidebar_tabs, -1);

	return TRUE;
}

/* handle the context menu if necessary */
static gboolean
nautilus_sidebar_press_event (GtkWidget *widget, GdkEventButton *event)
{
	NautilusSidebar *sidebar;
	GtkWidget *menu;
		
	if (widget->window != event->window) {
		return FALSE;
	}

	sidebar = NAUTILUS_SIDEBAR (widget);

	/* handle the context menu */
	if (event->button == CONTEXTUAL_MENU_BUTTON) {
		menu = nautilus_sidebar_create_context_menu (sidebar);	
		eel_pop_up_context_menu (GTK_MENU(menu),
				      EEL_DEFAULT_POPUP_MENU_DISPLACEMENT,
				      EEL_DEFAULT_POPUP_MENU_DISPLACEMENT,
				      event);
	}	
	return TRUE;
}

/* handle the sidebar tabs on the upstroke */
static gboolean
nautilus_sidebar_release_event (GtkWidget *widget, GdkEventButton *event)
{
	int title_top, title_bottom;
	NautilusSidebar *sidebar;
	NautilusSidebarTabs *sidebar_tabs;
	NautilusSidebarTabs *title_tab;
	int rounded_y;
	int which_tab;
		
	if (widget->window != event->window) {
		return FALSE;
	}

	sidebar = NAUTILUS_SIDEBAR (widget);
	
	sidebar_tabs = sidebar->details->sidebar_tabs;
	title_tab = sidebar->details->title_tab;
	rounded_y = floor (event->y + .5);

	/* if the click is in the main tabs, tell them about it */
	if (rounded_y >= GTK_WIDGET (sidebar->details->sidebar_tabs)->allocation.y) {
		which_tab = nautilus_sidebar_tabs_hit_test (sidebar_tabs, event->x, event->y);
		if (which_tab >= 0) {
			if (which_tab == sidebar->details->selected_index) {
				nautilus_sidebar_deactivate_panel (sidebar);
			} else {			
				nautilus_sidebar_tabs_select_tab (sidebar_tabs, which_tab);
				nautilus_sidebar_activate_panel (sidebar, which_tab);
				gtk_widget_queue_draw (widget);	
			}
		}
	} 
	
	/* also handle clicks in the title tab if necessary */
	if (sidebar->details->selected_index >= 0) {
		title_top = GTK_WIDGET (sidebar->details->title_tab)->allocation.y;
		title_bottom = title_top + GTK_WIDGET (sidebar->details->title_tab)->allocation.height;
		if (rounded_y >= title_top && rounded_y <= title_bottom) {
			which_tab = nautilus_sidebar_tabs_hit_test (title_tab, event->x, event->y);
			if (which_tab >= 0) {
				/* the user clicked in the title tab, so deactivate the panel */
				nautilus_sidebar_deactivate_panel (sidebar);
			}
		}
	}
	return TRUE;
}

static gboolean
value_different (const char *a, const char *b)
{
	if (!a && !b)
		return FALSE;

	if (!a || !b)
		return TRUE;

	return strcmp (a, b);
}

/* Handle the background changed signal by writing out the settings to metadata.
 */
static void
background_settings_changed_callback (EelBackground *background, NautilusSidebar *sidebar)
{
	char *image;
	char *color;

	g_assert (EEL_IS_BACKGROUND (background));
	g_assert (NAUTILUS_IS_SIDEBAR (sidebar));

	if (sidebar->details->file == NULL) {
		return;
	}
	
	/* Block so we don't respond to our own metadata changes.
	 */
	g_signal_handlers_block_by_func (sidebar->details->file,
					 G_CALLBACK (background_metadata_changed_callback),
					 sidebar);

	color = eel_background_get_color (background);
	image = eel_background_get_image_uri (background);
	
	nautilus_file_set_metadata (sidebar->details->file,
				    NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_COLOR,
				    NULL,
				    color);

	nautilus_file_set_metadata (sidebar->details->file,
				    NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_IMAGE,
				    NULL,
				    image);

	if (value_different (sidebar->details->current_background_color, color)) {
		g_free (sidebar->details->current_background_color);
		sidebar->details->current_background_color = g_strdup (color);
	}
	
	if (value_different (sidebar->details->current_background_image, image)) {
		g_free (sidebar->details->current_background_image);
		sidebar->details->current_background_image = g_strdup (image);
	}

	sidebar->details->is_default_background = FALSE;

	g_free (color);
	g_free (image);

	g_signal_handlers_unblock_by_func (sidebar->details->file,
					   G_CALLBACK (background_metadata_changed_callback),
					   sidebar);
}

/* handle the background reset signal by writing out NULL to metadata and setting the backgrounds
   fields to their default values */
static void
background_reset_callback (EelBackground *background, NautilusSidebar *sidebar)
{
	g_assert (EEL_IS_BACKGROUND (background));
	g_assert (NAUTILUS_IS_SIDEBAR (sidebar));

	if (sidebar->details->file == NULL) {
		return;
	}

	/* Block so we don't respond to our own metadata changes.
	 */
	g_signal_handlers_block_by_func (sidebar->details->file,
					 G_CALLBACK (background_metadata_changed_callback),
					 sidebar);

	nautilus_file_set_metadata (sidebar->details->file,
				    NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_COLOR,
				    NULL,
				    NULL);

	nautilus_file_set_metadata (sidebar->details->file,
				    NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_IMAGE,
				    NULL,
				    NULL);

	g_signal_handlers_unblock_by_func (sidebar->details->file,
					   G_CALLBACK (background_metadata_changed_callback),
					   sidebar);

	/* Force a read from the metadata to set the defaults
	 */
	background_metadata_changed_callback (sidebar);
}

static GtkWindow *
nautilus_sidebar_get_window (NautilusSidebar *sidebar)
{
	GtkWidget *result;

	result = gtk_widget_get_ancestor (GTK_WIDGET (sidebar), GTK_TYPE_WINDOW);

	return result == NULL ? NULL : GTK_WINDOW (result);
}

static void
command_button_callback (GtkWidget *button, char *id_str)
{
	NautilusSidebar *sidebar;
	GnomeVFSMimeApplication *application;
	
	sidebar = NAUTILUS_SIDEBAR (g_object_get_data (G_OBJECT (button), "user_data"));

	application = gnome_vfs_application_registry_get_mime_application (id_str);

	if (application != NULL) {
		nautilus_launch_application (application, sidebar->details->file,
					     nautilus_sidebar_get_window (sidebar));	

		gnome_vfs_mime_application_free (application);
	}
}

/* interpret commands for buttons specified by metadata. Handle some built-in ones explicitly, or fork
   a shell to handle general ones */
/* for now, we don't have any of these */
static void
metadata_button_callback (GtkWidget *button, const char *command_str)
{
	NautilusSidebar *sidebar;
		
	sidebar = NAUTILUS_SIDEBAR (g_object_get_data (G_OBJECT (button), "user_data"));
}

static void
nautilus_sidebar_chose_application_callback (GnomeVFSMimeApplication *application,
					     gpointer callback_data)
{
	NautilusSidebar *sidebar;

	sidebar = NAUTILUS_SIDEBAR (callback_data);

	if (application != NULL) {
		nautilus_launch_application
			(application, 
			 sidebar->details->file,
			 nautilus_sidebar_get_window (sidebar));
	}
}

static void
open_with_callback (GtkWidget *button, gpointer ignored)
{
	NautilusSidebar *sidebar;
	
	sidebar = NAUTILUS_SIDEBAR (g_object_get_data (G_OBJECT (button), "user_data"));
	
	g_return_if_fail (sidebar->details->file != NULL);

	nautilus_choose_application_for_file
		(sidebar->details->file,
		 GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (sidebar))),
		 nautilus_sidebar_chose_application_callback,
		 sidebar);
}

/* utility routine that allocates the command buttons from the command list */

static void
add_command_buttons (NautilusSidebar *sidebar, GList *application_list)
{
	char *id_string, *temp_str, *file_path;
	GList *p;
	GtkWidget *temp_button;
	GnomeVFSMimeApplication *application;

	/* There's always at least the "Open with..." button */
	sidebar->details->has_buttons = TRUE;

	for (p = application_list; p != NULL; p = p->next) {
	        application = p->data;	        

		temp_str = g_strdup_printf (_("Open with %s"), application->name);
	        temp_button = gtk_button_new_with_label (temp_str);
		g_free (temp_str);
		gtk_box_pack_start (GTK_BOX (sidebar->details->button_box), 
				    temp_button, 
				    FALSE, FALSE, 
				    0);

		/* Get the local path, if there is one */
		file_path = gnome_vfs_get_local_path_from_uri (sidebar->details->uri);
		if (file_path == NULL) {
			file_path = g_strdup (sidebar->details->uri);
		} 

		temp_str = g_shell_quote (file_path);		
		id_string = eel_str_replace_substring (application->id, "%s", temp_str); 		
		g_free (file_path);
		g_free (temp_str);

		eel_gtk_signal_connect_free_data 
			(GTK_OBJECT (temp_button), "clicked",
			 G_CALLBACK (command_button_callback), id_string);

                g_object_set_data (G_OBJECT (temp_button), "user_data", sidebar);
		
		gtk_widget_show (temp_button);
	}

	/* Catch-all button after all the others. */
	temp_button = gtk_button_new_with_label (_("Open with..."));
	g_signal_connect (temp_button, "clicked",
			  G_CALLBACK (open_with_callback), NULL);
	g_object_set_data (G_OBJECT (temp_button), "user_data", sidebar);
	gtk_widget_show (temp_button);
	gtk_box_pack_start (GTK_BOX (sidebar->details->button_box),
			    temp_button, FALSE, FALSE, 0);
}

/* utility to construct command buttons for the sidebar from the passed in metadata string */

static void
add_buttons_from_metadata (NautilusSidebar *sidebar, const char *button_data)
{
	char **terms;
	char *current_term, *temp_str;
	char *button_name, *command_string;
	const char *term;
	int index;
	GtkWidget *temp_button;
	
	/* split the button specification into a set of terms */	
	button_name = NULL;
	terms = g_strsplit (button_data, ";", 0);	
	
	/* for each term, either create a button or attach a property to one */
	for (index = 0; (term = terms[index]) != NULL; index++) {
		current_term = g_strdup (term);
		temp_str = strchr (current_term, '=');
		if (temp_str) {
			*temp_str = '\0';
			if (!g_ascii_strcasecmp (current_term, "button")) {
				button_name = g_strdup (temp_str + 1);
			} else if (!g_ascii_strcasecmp (current_term, "script")) {
			        if (button_name != NULL) {
			        	temp_button = gtk_button_new_with_label (button_name);		    
					gtk_box_pack_start (GTK_BOX (sidebar->details->button_box), 
							    temp_button, 
							    FALSE, FALSE, 
							    0);
					sidebar->details->has_buttons = TRUE;
					command_string = g_strdup (temp_str + 1);
					g_free (button_name);
					
					eel_gtk_signal_connect_free_data 
						(GTK_OBJECT (temp_button), "clicked",
						 G_CALLBACK (metadata_button_callback), command_string);
		                	g_object_set_data (G_OBJECT (temp_button), "user_data", sidebar);
					
					gtk_widget_show (temp_button);			
				}
			}
		}
		g_free(current_term);
	}
	g_strfreev (terms);
}

/* handle the hacked-in empty trash command */
static void
empty_trash_callback (GtkWidget *button, gpointer data)
{
	GtkWidget *window;
	
	window = gtk_widget_get_toplevel (button);
	nautilus_file_operations_empty_trash (window);
}

static void
nautilus_sidebar_trash_state_changed_callback (NautilusTrashMonitor *trash_monitor,
						gboolean state, gpointer callback_data)
{
		gtk_widget_set_sensitive (GTK_WIDGET (callback_data), !nautilus_trash_monitor_is_empty ());
}

/*
 * nautilus_sidebar_update_buttons:
 * 
 * Update the list of program-launching buttons based on the current uri.
 */
static void
nautilus_sidebar_update_buttons (NautilusSidebar *sidebar)
{
	char *button_data;
	GtkWidget *temp_button;
	GList *short_application_list;
	
	/* dispose of any existing buttons */
	if (sidebar->details->has_buttons) {
		gtk_container_remove (GTK_CONTAINER (sidebar->details->container),
				      GTK_WIDGET (sidebar->details->button_box_centerer)); 
		make_button_box (sidebar);
	}

	/* create buttons from file metadata if necessary */
	button_data = nautilus_file_get_metadata (sidebar->details->file,
						  NAUTILUS_METADATA_KEY_SIDEBAR_BUTTONS,
						  NULL);
	if (button_data) {
		add_buttons_from_metadata (sidebar, button_data);
		g_free(button_data);
	}

	/* here is a hack to provide an "empty trash" button when displaying the trash.  Eventually, we
	 * need a framework to allow protocols to add commands buttons */
	if (eel_istr_has_prefix (sidebar->details->uri, "trash:")) {
		/* FIXME: We don't use spaces to pad labels! */
		temp_button = gtk_button_new_with_mnemonic (_("_Empty Trash"));

		gtk_box_pack_start (GTK_BOX (sidebar->details->button_box), 
					temp_button, FALSE, FALSE, 0);
		gtk_widget_set_sensitive (temp_button, !nautilus_trash_monitor_is_empty ());
		gtk_widget_show (temp_button);
		sidebar->details->has_buttons = TRUE;
					
		g_signal_connect (temp_button, "clicked",
				  G_CALLBACK (empty_trash_callback), NULL);
		
		g_signal_connect_object (nautilus_trash_monitor_get (), "trash_state_changed",
					 G_CALLBACK (nautilus_sidebar_trash_state_changed_callback), temp_button, 0);
	}
	
	/* Make buttons for each item in short list + "Open with..." catchall,
	 * unless there aren't any applications at all in complete list. 
	 */

	if (nautilus_mime_has_any_applications_for_file (sidebar->details->file)) {
		short_application_list = 
			nautilus_mime_get_short_list_applications_for_file (sidebar->details->file);
		add_command_buttons (sidebar, short_application_list);
		gnome_vfs_mime_application_list_free (short_application_list);
	}

	/* Hide button box if a sidebar panel is showing. Otherwise, show it! */
	if (sidebar->details->selected_index != -1) {
		gtk_widget_hide (GTK_WIDGET (sidebar->details->button_box_centerer));
		gtk_widget_hide (GTK_WIDGET (sidebar->details->title));
	} else {
		gtk_widget_show (GTK_WIDGET (sidebar->details->button_box_centerer));
	}
}

static void
nautilus_sidebar_update_appearance (NautilusSidebar *sidebar)
{
	EelBackground *background;
	char *color_spec;
	char *background_color;
	char *background_image;

	g_return_if_fail (NAUTILUS_IS_SIDEBAR (sidebar));
	
	/* Connect the background changed signal to code that writes the color. */
	background = eel_get_widget_background (GTK_WIDGET (sidebar));
	if (!sidebar->details->background_connected) {
		sidebar->details->background_connected = TRUE;
		g_signal_connect_object (background,"settings_changed",
					 G_CALLBACK (background_settings_changed_callback), sidebar, 0);
		g_signal_connect_object (background, "reset",
					 G_CALLBACK (background_reset_callback), sidebar, 0);
	}
	
	/* Set up the background color and image from the metadata. */
	background_color = nautilus_file_get_metadata (sidebar->details->file,
						       NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_COLOR,
						       NULL);
	background_image = nautilus_file_get_metadata (sidebar->details->file,
						       NAUTILUS_METADATA_KEY_SIDEBAR_BACKGROUND_IMAGE,
						       NULL);

	if (background_color == NULL && background_image == NULL) {
		background_color = g_strdup (sidebar->details->default_background_color);
		background_image = g_strdup (sidebar->details->default_background_image);
		sidebar->details->is_default_background = TRUE;
	} else {
		sidebar->details->is_default_background = FALSE;
	}
		
	/* Block so we don't write these settings out in response to our set calls below */
	g_signal_handlers_block_by_func (background,
					 G_CALLBACK (background_settings_changed_callback),
					 sidebar);

	if (value_different (sidebar->details->current_background_color, background_color) ||
	    value_different (sidebar->details->current_background_image, background_image)) {
		
		g_free (sidebar->details->current_background_color);
		sidebar->details->current_background_color = g_strdup (background_color);
		g_free (sidebar->details->current_background_image);
		sidebar->details->current_background_image = g_strdup (background_image);

		eel_background_set_image_uri (background, background_image);
		eel_background_set_color (background, background_color);

		nautilus_sidebar_title_select_text_color
			(sidebar->details->title, background,
			 sidebar->details->is_default_background);
	}

	g_free (background_color);
	g_free (background_image);
	
	color_spec = nautilus_file_get_metadata (sidebar->details->file,
						 NAUTILUS_METADATA_KEY_SIDEBAR_TAB_COLOR,
						 DEFAULT_TAB_COLOR);
	nautilus_sidebar_tabs_set_color(sidebar->details->sidebar_tabs, color_spec);
	g_free (color_spec);

	color_spec = nautilus_file_get_metadata (sidebar->details->file,
						 NAUTILUS_METADATA_KEY_SIDEBAR_TITLE_TAB_COLOR,
						 DEFAULT_TAB_COLOR);
	nautilus_sidebar_tabs_set_color(sidebar->details->title_tab, color_spec);
	g_free (color_spec);

	g_signal_handlers_unblock_by_func (background,
					   G_CALLBACK (background_settings_changed_callback),
					   sidebar);
}


static void
background_metadata_changed_callback (NautilusSidebar *sidebar)
{
	GList *attributes;
	gboolean ready;

	attributes = nautilus_mime_actions_get_minimum_file_attributes ();
	ready = nautilus_file_check_if_ready (sidebar->details->file, attributes);
	g_list_free (attributes);

	if (ready) {
		nautilus_sidebar_update_appearance (sidebar);
		
		/* set up the command buttons */
		nautilus_sidebar_update_buttons (sidebar);
	}
}

/* here is the key routine that populates the sidebar with the appropriate information when the uri changes */

void
nautilus_sidebar_set_uri (NautilusSidebar *sidebar, 
			  const char* new_uri,
			  const char* initial_title)
{       
	NautilusFile *file;
	GList *attributes;

	g_return_if_fail (NAUTILUS_IS_SIDEBAR (sidebar));
	g_return_if_fail (new_uri != NULL);
	g_return_if_fail (initial_title != NULL);

	/* there's nothing to do if the uri is the same as the current one */ 
	if (eel_strcmp (sidebar->details->uri, new_uri) == 0) {
		return;
	}
	
	g_free (sidebar->details->uri);
	sidebar->details->uri = g_strdup (new_uri);
		
	if (sidebar->details->file != NULL) {
		g_signal_handler_disconnect (sidebar->details->file, 
					     sidebar->details->file_changed_connection);
		nautilus_file_monitor_remove (sidebar->details->file, sidebar);
	}


	file = nautilus_file_get (sidebar->details->uri);

	nautilus_file_unref (sidebar->details->file);

	sidebar->details->file = file;
	
	sidebar->details->file_changed_connection =
		g_signal_connect_object (sidebar->details->file, "changed",
					 G_CALLBACK (background_metadata_changed_callback),
					 sidebar, G_CONNECT_SWAPPED);

	attributes = nautilus_mime_actions_get_minimum_file_attributes ();
	nautilus_file_monitor_add (sidebar->details->file, sidebar, attributes);
	g_list_free (attributes);

	background_metadata_changed_callback (sidebar);

	/* tell the title widget about it */
	nautilus_sidebar_title_set_file (sidebar->details->title,
					 sidebar->details->file,
					 initial_title);
}

void
nautilus_sidebar_set_title (NautilusSidebar *sidebar, const char* new_title)
{       
	nautilus_sidebar_title_set_text (sidebar->details->title,
					 new_title);
}

/* we override size allocate so we can remember our size when it changes, since the paned widget
   doesn't generate a signal */
   
static void
nautilus_sidebar_size_allocate (GtkWidget *widget,
				GtkAllocation *allocation)
{
	NautilusSidebar *sidebar = NAUTILUS_SIDEBAR(widget);
	
	EEL_CALL_PARENT (GTK_WIDGET_CLASS, size_allocate, (widget, allocation));
	
	/* remember the size if it changed */
	if (widget->allocation.width != sidebar->details->old_width) {
		sidebar->details->old_width = widget->allocation.width;
 		eel_preferences_set_integer (NAUTILUS_PREFERENCES_SIDEBAR_WIDTH,
					     widget->allocation.width);
	}	
}

/* ::style_set handler for the sidebar */
static void
nautilus_sidebar_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	NautilusSidebar *sidebar;

	sidebar = NAUTILUS_SIDEBAR (widget);

	nautilus_sidebar_theme_changed (sidebar);
}

void
nautilus_sidebar_setup_width (NautilusSidebar *sidebar)
{
	GtkPaned *paned;

	g_return_if_fail (NAUTILUS_IS_SIDEBAR (sidebar));
	g_return_if_fail (GTK_WIDGET (sidebar)->parent != NULL);

	paned = GTK_PANED (GTK_WIDGET (sidebar)->parent);

	/* FIXME bugzilla.gnome.org 41245: Saved in pixels instead of in %? */
	/* FIXME bugzilla.gnome.org 41245: No reality check on the value? */
	gtk_paned_set_position (paned, sidebar_width_auto_value);
	sidebar->details->old_width = sidebar_width_auto_value;
}
