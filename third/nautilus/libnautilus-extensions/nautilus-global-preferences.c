/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-prefs-dialog.c - Implementation for preferences dialog.

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
#include "nautilus-global-preferences.h"

#include "nautilus-file-utilities.h"
#include "nautilus-glib-extensions.h"
#include "nautilus-gtk-extensions.h"
#include "nautilus-preferences-dialog.h"
#include "nautilus-preferences-group.h"
#include "nautilus-preferences-item.h"
#include "nautilus-string.h"
#include "nautilus-view-identifier.h"
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gtk/gtkbox.h>
#include <libgnome/gnome-i18n.h>
#include <liboaf/liboaf.h>
#include <libgnomevfs/gnome-vfs-utils.h>

/* Constants */
static const char untranslated_global_preferences_dialog_title[] = N_("Nautilus Preferences");
#define GLOBAL_PREFERENCES_DIALOG_TITLE _(untranslated_global_preferences_dialog_title)

static const char PROXY_HOST_KEY[] = "/system/gnome-vfs/http-proxy-host";
static const char PROXY_PORT_KEY[] = "/system/gnome-vfs/http-proxy-port";
static const char USE_PROXY_KEY[] = "/system/gnome-vfs/use-http-proxy";
static const char SYSTEM_GNOME_VFS_PATH[] = "/system/gnome-vfs";

/* Forward declarations */
static char *     global_preferences_get_sidebar_panel_key               (const char    *panel_iid);
static gboolean   global_preferences_is_sidebar_panel_enabled_cover      (gpointer       data,
									  gpointer       callback_data);
static GList *    global_preferences_get_sidebar_panel_view_identifiers  (void);
static gboolean   global_preferences_close_dialog_callback               (GtkWidget     *dialog,
									  gpointer       user_data);
static void       global_preferences_install_sidebar_panel_defaults      (void);
static void       global_preferences_install_sidebar_panel_descriptions  (void);
static void       global_preferences_install_defaults                    (void);
static void       global_preferences_install_visibility                  (void);
static void       global_preferences_install_speed_tradeoff_descriptions (const char    *name,
									  const char    *description);
static void       global_preferences_install_home_location_defaults      (void);
static void       global_preferences_install_descriptions                (void);
static int        compare_view_identifiers                               (gconstpointer  a,
									  gconstpointer  b);
static GtkWidget *global_preferences_create_dialog                       (void);

static GtkWidget *global_prefs_dialog = NULL;

/**
 * global_preferences_install_descriptions
 *
 * Install descriptions for some preferences.  A preference needs a description
 * only if it appears in the preferences dialog.
 */
static void
global_preferences_install_descriptions (void)
{
	static gboolean preferences_registered = FALSE;
	g_return_if_fail (preferences_registered == FALSE);
	preferences_registered = TRUE;

	/* Theme */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_THEME,
					      _("current theme"));

	/* Window create new */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_WINDOW_ALWAYS_NEW,
					      _("Open each item in a separate window"));
	
	/* Trash confirm */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_CONFIRM_TRASH,
					      _("Ask before deleting items from the trash"));
	
	/* Click activation type */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_CLICK_POLICY,
					      _("Click policy"));
	
	nautilus_preferences_enumeration_insert (NAUTILUS_PREFERENCES_CLICK_POLICY,
						 _("single"),
						 _("Activate items with a single click"),
						 NAUTILUS_CLICK_POLICY_SINGLE);
	
	nautilus_preferences_enumeration_insert (NAUTILUS_PREFERENCES_CLICK_POLICY,
						 _("double"),
						 _("Activate items with a double click"),
						 NAUTILUS_CLICK_POLICY_DOUBLE);
	
	/*
	 * Speed tradeoffs
	 */
	global_preferences_install_speed_tradeoff_descriptions (NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS,
								_("Display text in icons"));
	
	global_preferences_install_speed_tradeoff_descriptions (NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
								_("Show thumbnails for image files"));

	global_preferences_install_speed_tradeoff_descriptions (NAUTILUS_PREFERENCES_USE_PUBLIC_METADATA,
								_("Read and write metadata in each folder"));
	
	global_preferences_install_speed_tradeoff_descriptions (NAUTILUS_PREFERENCES_PREVIEW_SOUND,
								_("Play a sound file when the mouse is over it"));
	
	/* Appearance options */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SMOOTH_GRAPHICS_MODE,
					      _("Use smoother (but slower) graphics"));

	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_DIRECTORY_VIEW_FONT_FAMILY,
					      _("Font family used to display file names"));

	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_START_WITH_TOOL_BAR,
					      _("Display tool bar in new windows"));

	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_START_WITH_LOCATION_BAR,
					      _("Display location bar in new windows"));

	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_START_WITH_STATUS_BAR,
					      _("Display status bar in new windows"));
	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_START_WITH_SIDEBAR,
					      _("Display sidebar in new windows"));
	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SHOW_DESKTOP,
					      _("Use Nautilus to draw the desktop"));
								  
	/* search tradeoffs */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SEARCH_METHOD,
					      _("Always do slow, complete search"));

	/* search bar type */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
					      _("search type to do by default"));
	
	nautilus_preferences_enumeration_insert (NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
						 _("search by text"),
						 _("Search for files by text only"),
						 NAUTILUS_SIMPLE_SEARCH_BAR);
	
	nautilus_preferences_enumeration_insert (NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
						 _("search by text and properties"),
						 _("Search for files by text and by their properties"),
						 NAUTILUS_COMPLEX_SEARCH_BAR);
	
	/* web search uri  */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SEARCH_WEB_URI,
					      _("Search Web Location"));

	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
					      _("Show hidden files (starting with \".\")"));
	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
					      _("Show backup files (ending with \"~\")"));
	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_SHOW_SPECIAL_FLAGS,
					      _("Show special flags in Properties window"));
	
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
					      _("Show only folders in tree sidebar panel"));

	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_CAN_ADD_CONTENT,
					      _("Can add Content"));

	/* built-in bookmarks */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_HIDE_BUILT_IN_BOOKMARKS,
					      _("Don't include the built-in bookmarks"));
	

	/* Home location */
	nautilus_preferences_set_description (NAUTILUS_PREFERENCES_HOME_URI,
					      _("Home Location"));

	/* Sidebar panel descriptions */
	global_preferences_install_sidebar_panel_descriptions ();
	
	/* Proxy descriptions */
	nautilus_preferences_set_description (USE_PROXY_KEY, _("Use HTTP Proxy"));
	nautilus_preferences_set_description (PROXY_HOST_KEY, _("HTTP Proxy"));
	nautilus_preferences_set_description (PROXY_PORT_KEY, _("HTTP Proxy Port"));
}

/**
 * global_preferences_install_defaults
 *
 * Install defaults for some preferences.  Install defaults only for preferences
 * that need something other than 0 (integer) FALSE (boolean) or "" (string) as
 * their defaults.
 *
 * Its possible to have different defaults for different user levels  Its not 
 * required to have defaults for EACH user level.  If there is no default
 * installed for a high user level, the next lowest user level with a valid
 * default is used.
 */
static void
global_preferences_install_defaults (void)
{
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);

	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);

	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_CONFIRM_TRASH, 
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);

	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY);
	
	nautilus_preferences_default_set_string (NAUTILUS_PREFERENCES_DIRECTORY_VIEW_FONT_FAMILY,
						 NAUTILUS_USER_LEVEL_NOVICE,
						 "helvetica");

	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_CLICK_POLICY,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  NAUTILUS_CLICK_POLICY_DOUBLE);
	
	nautilus_preferences_default_set_string (NAUTILUS_PREFERENCES_THEME,
						 NAUTILUS_USER_LEVEL_NOVICE,
						 "default");
	
	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY);

	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_USE_PUBLIC_METADATA,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY);
	
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SMOOTH_GRAPHICS_MODE,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);
	
	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_PREVIEW_SOUND,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY);
	
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SHOW_SPECIAL_FLAGS,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SHOW_SPECIAL_FLAGS,
						  NAUTILUS_USER_LEVEL_HACKER,
						  TRUE);
	
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SHOW_DESKTOP,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_SEARCH_METHOD,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);

	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_CAN_ADD_CONTENT,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_CAN_ADD_CONTENT,
						  NAUTILUS_USER_LEVEL_INTERMEDIATE,
						  TRUE);

	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  NAUTILUS_SIMPLE_SEARCH_BAR);

	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
						  NAUTILUS_USER_LEVEL_INTERMEDIATE,
						  NAUTILUS_COMPLEX_SEARCH_BAR);
	
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_WINDOW_ALWAYS_NEW,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);

	nautilus_preferences_default_set_string (NAUTILUS_PREFERENCES_ICON_CAPTIONS,
						 NAUTILUS_USER_LEVEL_NOVICE,
						 "size|date_modified|type");
	
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_HIDE_BUILT_IN_BOOKMARKS,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);
	
	/* FIXME bugzilla.eazel.com 1245: Saved in pixels instead of in %? */
	nautilus_preferences_default_set_integer (NAUTILUS_PREFERENCES_SIDEBAR_WIDTH,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  148);
	
	nautilus_preferences_default_set_string (NAUTILUS_PREFERENCES_SEARCH_WEB_URI,
						 NAUTILUS_USER_LEVEL_NOVICE,
						 "http://www.eazel.com/websearch");

	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_START_WITH_TOOL_BAR,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_START_WITH_LOCATION_BAR,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_START_WITH_STATUS_BAR,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);
	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_START_WITH_SIDEBAR, 
						  NAUTILUS_USER_LEVEL_NOVICE,
						  TRUE);

	nautilus_preferences_default_set_boolean (NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);

	/* Add the gnome-vfs path to the list of monitored directories - for proxy settings */
	nautilus_preferences_monitor_directory (SYSTEM_GNOME_VFS_PATH);
	
	/* Proxy defaults */
	nautilus_preferences_default_set_boolean (USE_PROXY_KEY,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  FALSE);
	nautilus_preferences_default_set_integer (PROXY_PORT_KEY,
						  NAUTILUS_USER_LEVEL_NOVICE,
						  8080);

	/* Sidebar panel defaults */
	global_preferences_install_sidebar_panel_defaults ();

	/* Home location */
	global_preferences_install_home_location_defaults ();
}

/**
 * global_preferences_install_visibility
 *
 * Set the visibilities for restricted preferences.  The visible user level
 * is the first user level at which the preference is visible.  By default
 * all preferences have a visibility of 0.
 *
 * A preference with a value greater than 0, will be "visible" only at that
 * level or higher.  Any getters that ask for that preference at lower user
 * levels will always receive the default value.  Also, if the preference 
 * has an entry in the preferences dialog, it will not be shown unless the 
 * current user level is greater than or equal to the preference's visible
 * user level.
 * 
 */
static void
global_preferences_install_visibility (void)
{
	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_HOME_URI,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_CLICK_POLICY,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_CONFIRM_TRASH,
						     NAUTILUS_USER_LEVEL_HACKER);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SHOW_SPECIAL_FLAGS,
						     NAUTILUS_USER_LEVEL_HACKER);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SMOOTH_GRAPHICS_MODE,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_START_WITH_TOOL_BAR,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_START_WITH_LOCATION_BAR,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_START_WITH_STATUS_BAR,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_START_WITH_SIDEBAR,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SHOW_DESKTOP,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_PREVIEW_SOUND,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_USE_PUBLIC_METADATA,
						     NAUTILUS_USER_LEVEL_HACKER);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SEARCH_METHOD,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_SEARCH_WEB_URI,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_HOME_URI,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);

	nautilus_preferences_set_visible_user_level (NAUTILUS_PREFERENCES_HIDE_BUILT_IN_BOOKMARKS,
						     NAUTILUS_USER_LEVEL_INTERMEDIATE);
}

/*
 * Private stuff
 */
static int
compare_view_identifiers (gconstpointer a, gconstpointer b)
{
 	NautilusViewIdentifier *idenfifier_a;
 	NautilusViewIdentifier *idenfifier_b;
	
 	g_assert (a != NULL);
 	g_assert (b != NULL);

 	idenfifier_a = (NautilusViewIdentifier*) a;
 	idenfifier_b = (NautilusViewIdentifier*) b;
	
	return nautilus_strcmp (idenfifier_a->name, idenfifier_b->name);
}

static GtkWidget *
global_preferences_create_dialog (void)
{
	GtkWidget		*prefs_dialog;
	NautilusPreferencesBox	*preference_box;
	GtkWidget		*directory_views_pane;
	GtkWidget		*sidebar_panels_pane;
	GtkWidget		*appearance_pane;
	GtkWidget		*file_indexing_pane;
	GtkWidget		*tradeoffs_pane;
	GtkWidget		*navigation_pane;

	/*
	 * In the soon to come star trek future, the following widgetry
	 * might be either fetched from a glade file or generated from 
	 * an xml file.
	 */
	prefs_dialog = nautilus_preferences_dialog_new (GLOBAL_PREFERENCES_DIALOG_TITLE);

	gtk_window_set_wmclass (GTK_WINDOW (prefs_dialog), "global_preferences", "Nautilus");

	gtk_signal_connect (GTK_OBJECT (prefs_dialog),
			    "close",
			    GTK_SIGNAL_FUNC (global_preferences_close_dialog_callback),
			    NULL);

	/* Create a preference box */
	preference_box = NAUTILUS_PREFERENCES_BOX (nautilus_preferences_dialog_get_prefs_box
						   (NAUTILUS_PREFERENCES_DIALOG (prefs_dialog)));


	/*
	 * Appearance
	 */
	appearance_pane = nautilus_preferences_box_add_pane (preference_box,
							     _("Appearance"),
							     _("Appearance Settings"));
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (appearance_pane), _("Smoother Graphics"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 0,
							 NAUTILUS_PREFERENCES_SMOOTH_GRAPHICS_MODE,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (appearance_pane), _("Fonts"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 1,
							 NAUTILUS_PREFERENCES_DIRECTORY_VIEW_FONT_FAMILY,
							 NAUTILUS_PREFERENCE_ITEM_FONT_FAMILY);

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (appearance_pane), _("Views"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 2,
							 NAUTILUS_PREFERENCES_START_WITH_TOOL_BAR,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 2,
							 NAUTILUS_PREFERENCES_START_WITH_LOCATION_BAR,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 2,
							 NAUTILUS_PREFERENCES_START_WITH_STATUS_BAR,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 2,
							 NAUTILUS_PREFERENCES_START_WITH_SIDEBAR,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (appearance_pane), _("Desktop"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (appearance_pane),
							 3,
							 NAUTILUS_PREFERENCES_SHOW_DESKTOP,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);

	
	/*
	 * Folder Views pane
	 */
	directory_views_pane = nautilus_preferences_box_add_pane (preference_box,
								 _("Folder Views"),
								 _("Folder Views Settings"));
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane), _("Window Behavior"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 0,
							 NAUTILUS_PREFERENCES_WINDOW_ALWAYS_NEW,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane), _("Click Behavior"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 1,
							 NAUTILUS_PREFERENCES_CLICK_POLICY,
							 NAUTILUS_PREFERENCE_ITEM_ENUM);

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane), _("Trash Behavior"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 2,
							 NAUTILUS_PREFERENCES_CONFIRM_TRASH,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane), _("Display"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 3,
							 NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);

	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 3,
							 NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);

	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 3,
							 NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);

	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (directory_views_pane),
							 3,
							 NAUTILUS_PREFERENCES_SHOW_SPECIAL_FLAGS,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);

	/*
	 * Search Settings 
	 */
	file_indexing_pane = nautilus_preferences_box_add_pane (preference_box,
								_("Search"),
								_("Search Settings"));
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (file_indexing_pane),
					     _("Search Complexity Options"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (file_indexing_pane),
							 0,
							 NAUTILUS_PREFERENCES_SEARCH_BAR_TYPE,
							 NAUTILUS_PREFERENCE_ITEM_ENUM);
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (file_indexing_pane),
					     _("Search Tradeoffs"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (file_indexing_pane),
							 1,
							 NAUTILUS_PREFERENCES_SEARCH_METHOD,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (file_indexing_pane),
					     _("Search Locations"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (file_indexing_pane),
							 2,
							 NAUTILUS_PREFERENCES_SEARCH_WEB_URI,
							 NAUTILUS_PREFERENCE_ITEM_EDITABLE_STRING);
					
	/*
	 * Sidebar panels pane
	 */
	sidebar_panels_pane = nautilus_preferences_box_add_pane (preference_box,
								 _("Sidebar Panels"),
								 _("Sidebar Panels Description"));
	
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (sidebar_panels_pane), 
					     _("Choose which panels should appear in the sidebar"));

	{
		char *preference_key;
		GList *view_identifiers;
		GList *p;
		NautilusViewIdentifier *identifier;

		view_identifiers = global_preferences_get_sidebar_panel_view_identifiers ();

		view_identifiers = g_list_sort (view_identifiers, compare_view_identifiers);

		for (p = view_identifiers; p != NULL; p = p->next) {
			identifier = (NautilusViewIdentifier *) (p->data);
			
			preference_key = global_preferences_get_sidebar_panel_key (identifier->iid);

			g_assert (preference_key != NULL);

			nautilus_preferences_pane_add_item_to_nth_group 
				(NAUTILUS_PREFERENCES_PANE (sidebar_panels_pane),
				 0,
				 preference_key,
				 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
			g_free (preference_key);

		}
	
		nautilus_view_identifier_list_free (view_identifiers);
	}

	/*
	 * Navigation
	 */
	navigation_pane = nautilus_preferences_box_add_pane (preference_box,
							    _("Navigation"),
							    _("Navigation Settings"));

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (navigation_pane), _("Home Location"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (navigation_pane),
							 0,
							 NAUTILUS_PREFERENCES_HOME_URI,
							 NAUTILUS_PREFERENCE_ITEM_EDITABLE_STRING);

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (navigation_pane), _("Proxy Settings"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (navigation_pane),
							 1,
							 USE_PROXY_KEY,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (navigation_pane),
							 1,
							 PROXY_HOST_KEY,
							 NAUTILUS_PREFERENCE_ITEM_EDITABLE_STRING);

	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (navigation_pane),
							 1,
							 PROXY_PORT_KEY,
							 NAUTILUS_PREFERENCE_ITEM_INTEGER);

	/* built-in bookmarks */
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (navigation_pane),
					     _("Built-in Bookmarks"));
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (navigation_pane),
							 2,
							 NAUTILUS_PREFERENCES_HIDE_BUILT_IN_BOOKMARKS,
							 NAUTILUS_PREFERENCE_ITEM_BOOLEAN);


	/*
	 * Tradeoffs
	 */
	tradeoffs_pane = nautilus_preferences_box_add_pane (preference_box,
							    _("Speed Tradeoffs"),
							    _("Speed Tradeoffs Settings"));

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane), _("Show Text in Icons"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane),
							 0,
							 NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS,
							 NAUTILUS_PREFERENCE_ITEM_SHORT_ENUM);

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane), _("Show Thumbnails for Image Files"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane),
							 1,
							 NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
							 NAUTILUS_PREFERENCE_ITEM_SHORT_ENUM);

	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane), _("Previewing Sound Files"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane),
							 2,
							 NAUTILUS_PREFERENCES_PREVIEW_SOUND,
							 NAUTILUS_PREFERENCE_ITEM_SHORT_ENUM);

	
	/* FIXME bugzilla.eazel.com 2560: This title phrase needs improvement. */
	nautilus_preferences_pane_add_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane), _("Make Folder Appearance Details Public"));
	
	nautilus_preferences_pane_add_item_to_nth_group (NAUTILUS_PREFERENCES_PANE (tradeoffs_pane),
							 3,
							 NAUTILUS_PREFERENCES_USE_PUBLIC_METADATA,
							 NAUTILUS_PREFERENCE_ITEM_SHORT_ENUM);


	/* Update the dialog so that the right items show up based on the current user level */
	nautilus_preferences_dialog_update (NAUTILUS_PREFERENCES_DIALOG (prefs_dialog));

	return prefs_dialog;
}

/* Make a query to find out what sidebar panels are available. */
static GList *
global_preferences_get_sidebar_panel_view_identifiers (void)
{
	CORBA_Environment ev;
	const char *query;
        OAF_ServerInfoList *oaf_result;
	guint i;
	NautilusViewIdentifier *id;
	GList *view_identifiers;

	CORBA_exception_init (&ev);

	query = "nautilus:sidebar_panel_name.defined() AND repo_ids.has ('IDL:Bonobo/Control:1.0')";

	oaf_result = oaf_query (query, NULL, &ev);
		
	view_identifiers = NULL;

        if (ev._major == CORBA_NO_EXCEPTION && oaf_result != NULL) {
		for (i = 0; i < oaf_result->_length; i++) {
			id = nautilus_view_identifier_new_from_sidebar_panel
				(&oaf_result->_buffer[i]);
			view_identifiers = g_list_prepend (view_identifiers, id);
		}
		view_identifiers = g_list_reverse (view_identifiers);
	} 

	if (oaf_result != NULL) {
		CORBA_free (oaf_result);
	}
	
	CORBA_exception_free (&ev);

	return view_identifiers;
}

GList *
nautilus_global_preferences_get_enabled_sidebar_panel_view_identifiers (void)
{
	GList *enabled_view_identifiers;
 	GList *disabled_view_identifiers;
        
	enabled_view_identifiers = global_preferences_get_sidebar_panel_view_identifiers ();

	enabled_view_identifiers = nautilus_g_list_partition (enabled_view_identifiers,
							      global_preferences_is_sidebar_panel_enabled_cover,
							      NULL,
							      &disabled_view_identifiers);
	
	nautilus_view_identifier_list_free (disabled_view_identifiers);
	
        return enabled_view_identifiers;
}

static void
destroy_global_prefs_dialog (void)
{
	/* Free the dialog first, cause it has refs to preferences */
	if (global_prefs_dialog != NULL) {
		/* Since it's a top-level window, it's OK to destroy rather than unref'ing. */
		gtk_widget_destroy (global_prefs_dialog);
	}
}

static GtkWidget *
global_preferences_get_dialog (void)
{
	static gboolean set_up_exit = FALSE;

	nautilus_global_preferences_initialize ();

	if (global_prefs_dialog == NULL) {
		/* Install descriptions right before creating the dialog.
		 * The descriptions are only used within the preferences
		 * dialog.
		 */
		global_preferences_install_descriptions ();
		global_prefs_dialog = global_preferences_create_dialog ();
	}

	if (!set_up_exit) {
		g_atexit (destroy_global_prefs_dialog);
		set_up_exit = TRUE;
	}

	return global_prefs_dialog;
}

static struct 
{
	const char *name;
	gboolean novice_default;
	gboolean intermediate_default;
	gboolean hacker_default;
	int visible_user_level;
} known_sidebar_panels[] =
{
	{ "OAFIID:nautilus_notes_view:7f04c3cb-df79-4b9a-a577-38b19ccd4185",       TRUE,  TRUE,  TRUE,  NAUTILUS_USER_LEVEL_NOVICE},
	{ "OAFIID:hyperbola_navigation_tree:57542ce0-71ff-442d-a764-462c92514234", TRUE,  TRUE,  TRUE,  NAUTILUS_USER_LEVEL_NOVICE },
	{ "OAFIID:nautilus_history_view:a7a85bdd-2ecf-4bc1-be7c-ed328a29aacb",     TRUE,  TRUE,  TRUE,  NAUTILUS_USER_LEVEL_NOVICE },
	{ "OAFIID:nautilus_tree_view:2d826a6e-1669-4a45-94b8-23d65d22802d",        FALSE, FALSE, FALSE, NAUTILUS_USER_LEVEL_NOVICE },
};

static void
global_preferences_install_sidebar_panel_defaults (void)
{
	guint i;
	
	/* Install the user level on/off defaults for known sidebar panels */
	for (i = 0; i < NAUTILUS_N_ELEMENTS (known_sidebar_panels); i++) {
		char *key = global_preferences_get_sidebar_panel_key (known_sidebar_panels[i].name);
		
		nautilus_preferences_default_set_boolean (key,
							  NAUTILUS_USER_LEVEL_NOVICE,
							  known_sidebar_panels[i].novice_default);
		nautilus_preferences_default_set_boolean (key,
							  NAUTILUS_USER_LEVEL_INTERMEDIATE,
							  known_sidebar_panels[i].intermediate_default);
		nautilus_preferences_default_set_boolean (key,
							  NAUTILUS_USER_LEVEL_HACKER,
							  known_sidebar_panels[i].hacker_default);

		nautilus_preferences_set_visible_user_level (key,
							     known_sidebar_panels[i].visible_user_level);

		g_free (key);
	}
}

static void
global_preferences_install_sidebar_panel_descriptions (void)
{
 	GList *view_identifiers;
 	GList *iterator;
	
	/* Install the descriptions for the available sidebar panels */
 	view_identifiers = global_preferences_get_sidebar_panel_view_identifiers ();

 	for (iterator = view_identifiers; iterator != NULL; iterator = iterator->next) {
 		NautilusViewIdentifier *identifier;
 		char *key;

 		identifier = iterator->data;
 		g_return_if_fail (identifier != NULL);
		
 		key = global_preferences_get_sidebar_panel_key (identifier->iid);
 		g_return_if_fail (key != NULL);
		
 		nautilus_preferences_set_description (key, identifier->name);
 		g_free (key);
 	}

 	nautilus_view_identifier_list_free (view_identifiers);
}

static char *
global_preferences_get_sidebar_panel_key (const char *panel_iid)
{
	g_return_val_if_fail (panel_iid != NULL, NULL);

	return g_strdup_printf ("%s/%s", NAUTILUS_PREFERENCES_SIDEBAR_PANELS_NAMESPACE, panel_iid);
}

static gboolean
global_preferences_is_sidebar_panel_enabled (NautilusViewIdentifier *panel_identifier)
{
	gboolean enabled;
        gchar  *key;
	
	g_return_val_if_fail (panel_identifier != NULL, FALSE);
	g_return_val_if_fail (panel_identifier->iid != NULL, FALSE);
	
	key = global_preferences_get_sidebar_panel_key (panel_identifier->iid);
	g_return_val_if_fail (key != NULL, FALSE);
        enabled = nautilus_preferences_get_boolean (key);
        g_free (key);

        return enabled;
}

static gboolean
global_preferences_is_sidebar_panel_enabled_cover (gpointer data, gpointer callback_data)
{
	return global_preferences_is_sidebar_panel_enabled (data);
}

static void
global_preferences_install_speed_tradeoff_descriptions (const char *name,
							const char *description)
{							  
	nautilus_preferences_set_description (name, description);
	
 	nautilus_preferences_enumeration_insert (name,
						 _("always"),
						    _("Always"),
						 NAUTILUS_SPEED_TRADEOFF_ALWAYS);
 	nautilus_preferences_enumeration_insert (name,
						 _("local only"),
						 _("Local Files Only"),
						 NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY);
 	nautilus_preferences_enumeration_insert (name,
						 _("never"),
						 _("Never"),
						 NAUTILUS_SPEED_TRADEOFF_NEVER);
}							  

static void
global_preferences_install_home_location_defaults (void)
{
	char *default_novice_home_uri;
	char *default_intermediate_home_uri;
	char *user_main_directory;		
	
	user_main_directory = nautilus_get_user_main_directory ();
	
	default_novice_home_uri = gnome_vfs_get_uri_from_local_path (user_main_directory);
	default_intermediate_home_uri = gnome_vfs_get_uri_from_local_path (g_get_home_dir ());
	
	nautilus_preferences_default_set_string (NAUTILUS_PREFERENCES_HOME_URI,
						 NAUTILUS_USER_LEVEL_NOVICE,
						 default_novice_home_uri);
	
	nautilus_preferences_default_set_string (NAUTILUS_PREFERENCES_HOME_URI,
						 NAUTILUS_USER_LEVEL_INTERMEDIATE,
						 default_intermediate_home_uri);
	
	g_free (user_main_directory);
	g_free (default_novice_home_uri);
	g_free (default_intermediate_home_uri);
}

static gboolean
global_preferences_close_dialog_callback (GtkWidget   *dialog,
					  gpointer    user_data)
{
	nautilus_global_preferences_hide_dialog ();

	return TRUE;
}


/*
 * Public functions
 */
void
nautilus_global_preferences_show_dialog (void)
{
	GtkWidget *dialog = global_preferences_get_dialog ();

	nautilus_gtk_window_present (GTK_WINDOW (dialog));
}

void
nautilus_global_preferences_hide_dialog (void)
{
	GtkWidget *dialog = global_preferences_get_dialog ();

	gtk_widget_hide (dialog);
}

void
nautilus_global_preferences_set_dialog_title (const char *title)
{
	GtkWidget *dialog;
	g_return_if_fail (title != NULL);
	
	dialog = global_preferences_get_dialog ();

	gtk_window_set_title (GTK_WINDOW (dialog), title);
}

void
nautilus_global_preferences_initialize (void)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}

	initialized = TRUE;

	/* Install defaults */
	global_preferences_install_defaults ();

	/* Install visiblities */
	global_preferences_install_visibility ();
}
