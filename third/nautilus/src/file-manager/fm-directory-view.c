/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-directory-view.c
 *
 * Copyright (C) 1999, 2000  Free Software Foundation
 * Copyright (C) 2000, 2001  Eazel, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Ettore Perazzoli,
 *          John Sullivan <sullivan@eazel.com>,
 *          Darin Adler <darin@bentspoon.com>,
 *          Pavel Cisler <pavel@eazel.com>,
 *          David Emory Watson <dwatson@cs.ucr.edu>
 */

#include <config.h>
#include "fm-directory-view.h"

#include "fm-error-reporting.h"
#include "fm-properties-window.h"
#include <libgnome/gnome-url.h>
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-window.h>
#include <bonobo/bonobo-zoomable.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-exception.h>
#include <eel/eel-background.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-gnome-extensions.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-gtk-macros.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>
#include <eel/eel-vfs-extensions.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkclipboard.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkselection.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkmessagedialog.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libnautilus-private/nautilus-recent.h>
#include <libegg/egg-screen-exec.h>
#include <libnautilus-private/nautilus-bonobo-extensions.h>
#include <libnautilus-private/nautilus-directory-background.h>
#include <libnautilus-private/nautilus-directory.h>
#include <libnautilus-private/nautilus-file-attributes.h>
#include <libnautilus-private/nautilus-file-dnd.h>
#include <libnautilus-private/nautilus-file-operations.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-icon-factory.h>
#include <libnautilus-private/nautilus-link.h>
#include <libnautilus-private/nautilus-metadata.h>
#include <libnautilus-private/nautilus-mime-actions.h>
#include <libnautilus-private/nautilus-program-choosing.h>
#include <libnautilus-private/nautilus-trash-directory.h>
#include <libnautilus-private/nautilus-trash-monitor.h>
#include <libnautilus-private/nautilus-view-identifier.h>
#include <libnautilus/nautilus-bonobo-ui.h>
#include <math.h>
#include <unistd.h>

/* The list view receives files from the directory model in chunks, to
 * improve responsiveness during loading. This is the number of files
 * we add to the list or change at once.
 */
#define FILES_TO_PROCESS_AT_ONCE 300

#define DISPLAY_TIMEOUT_INTERVAL_MSECS 1000

#define SILENT_WINDOW_OPEN_LIMIT 5

#define DUPLICATE_HORIZONTAL_ICON_OFFSET 70
#define DUPLICATE_VERTICAL_ICON_OFFSET   30

#define RESPONSE_RUN 1000
#define RESPONSE_DISPLAY 1001
#define RESPONSE_RUN_IN_TERMINAL 1002

/* MOD2 is num lock -- I would include MOD3-5 if I was sure they were not lock keys */
#define ALL_NON_LOCK_MODIFIER_KEYS (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)

/* Paths to use when referring to bonobo menu items. Paths used by
 * subclasses are in fm-directory-view.h 
 */
#define FM_DIRECTORY_VIEW_COMMAND_RENAME                                "/commands/Rename"
#define FM_DIRECTORY_VIEW_COMMAND_OPEN                      		"/commands/Open"
#define FM_DIRECTORY_VIEW_COMMAND_OPEN_ALTERNATE        		"/commands/OpenAlternate"
#define FM_DIRECTORY_VIEW_COMMAND_OPEN_WITH				"/commands/Open With"
#define FM_DIRECTORY_VIEW_COMMAND_NEW_FOLDER				"/commands/New Folder"
#define FM_DIRECTORY_VIEW_COMMAND_NEW_LAUNCHER				"/commands/New Launcher"
#define FM_DIRECTORY_VIEW_COMMAND_EDIT_LAUNCHER				"/commands/Edit Launcher"
#define FM_DIRECTORY_VIEW_COMMAND_DELETE                    		"/commands/Delete"
#define FM_DIRECTORY_VIEW_COMMAND_TRASH                    		"/commands/Trash"
#define FM_DIRECTORY_VIEW_COMMAND_EMPTY_TRASH                   	"/commands/Empty Trash"
#define FM_DIRECTORY_VIEW_COMMAND_DUPLICATE                		"/commands/Duplicate"
#define FM_DIRECTORY_VIEW_COMMAND_CREATE_LINK                		"/commands/Create Link"
#define FM_DIRECTORY_VIEW_COMMAND_PROPERTIES         			"/commands/Properties"
#define FM_DIRECTORY_VIEW_COMMAND_OTHER_APPLICATION    			"/commands/OtherApplication"
#define FM_DIRECTORY_VIEW_COMMAND_OTHER_VIEWER	   			"/commands/OtherViewer"
#define FM_DIRECTORY_VIEW_COMMAND_CUT_FILES	    			"/commands/Cut Files"
#define FM_DIRECTORY_VIEW_COMMAND_COPY_FILES				"/commands/Copy Files"
#define FM_DIRECTORY_VIEW_COMMAND_PASTE_FILES	   			"/commands/Paste Files"

#define FM_DIRECTORY_VIEW_MENU_PATH_OPEN_ALTERNATE        		"/menu/File/Open Placeholder/OpenAlternate"
#define FM_DIRECTORY_VIEW_MENU_PATH_OPEN_WITH				"/menu/File/Open Placeholder/Open With"
#define FM_DIRECTORY_VIEW_MENU_PATH_SCRIPTS				"/menu/File/Open Placeholder/Scripts"
#define FM_DIRECTORY_VIEW_MENU_PATH_TRASH                    		"/menu/Edit/Dangerous File Items Placeholder/Trash"
#define FM_DIRECTORY_VIEW_MENU_PATH_DELETE                    		"/menu/Edit/Dangerous File Items Placeholder/Delete"
#define FM_DIRECTORY_VIEW_MENU_PATH_EMPTY_TRASH                    	"/menu/File/Global File Items Placeholder/Empty Trash"
#define FM_DIRECTORY_VIEW_MENU_PATH_CREATE_LINK                	 	"/menu/Edit/File Items Placeholder/Create Link"
#define FM_DIRECTORY_VIEW_MENU_PATH_APPLICATIONS_PLACEHOLDER    	"/menu/File/Open Placeholder/Open With/Applications Placeholder"
#define FM_DIRECTORY_VIEW_MENU_PATH_OTHER_APPLICATION		    	"/menu/File/Open Placeholder/Open With/OtherApplication"
#define FM_DIRECTORY_VIEW_MENU_PATH_VIEWERS_PLACEHOLDER    		"/menu/File/Open Placeholder/Open With/Viewers Placeholder"
#define FM_DIRECTORY_VIEW_MENU_PATH_OTHER_VIEWER		    	"/menu/File/Open Placeholder/Open With/OtherViewer"
#define FM_DIRECTORY_VIEW_MENU_PATH_SCRIPTS_PLACEHOLDER    		"/menu/File/Open Placeholder/Scripts/Scripts Placeholder"
#define FM_DIRECTORY_VIEW_MENU_PATH_SCRIPTS_SEPARATOR    		"/menu/File/Open Placeholder/Scripts/After Scripts"
#define FM_DIRECTORY_VIEW_MENU_PATH_CUT_FILES    			"/menu/Edit/Cut"
#define FM_DIRECTORY_VIEW_MENU_PATH_COPY_FILES		    		"/menu/Edit/Copy"
#define FM_DIRECTORY_VIEW_MENU_PATH_PASTE_FILES		    		"/menu/File/Paste"

#define FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND				"/popups/background"
#define FM_DIRECTORY_VIEW_POPUP_PATH_SELECTION				"/popups/selection"

#define FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND_SCRIPTS_PLACEHOLDER	"/popups/background/Before Zoom Items/Scripts/Scripts Placeholder"
#define FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND_SCRIPTS_SEPARATOR	"/popups/background/Before Zoom Items/Scripts/After Scripts"

#define FM_DIRECTORY_VIEW_POPUP_PATH_APPLICATIONS_PLACEHOLDER    	"/popups/selection/Open Placeholder/Open With/Applications Placeholder"
#define FM_DIRECTORY_VIEW_POPUP_PATH_VIEWERS_PLACEHOLDER    		"/popups/selection/Open Placeholder/Open With/Viewers Placeholder"
#define FM_DIRECTORY_VIEW_POPUP_PATH_SCRIPTS_PLACEHOLDER    		"/popups/selection/Open Placeholder/Scripts/Scripts Placeholder"
#define FM_DIRECTORY_VIEW_POPUP_PATH_SCRIPTS_SEPARATOR    		"/popups/selection/Open Placeholder/Scripts/After Scripts"
#define FM_DIRECTORY_VIEW_POPUP_PATH_OPEN_WITH				"/popups/selection/Open Placeholder/Open With"
#define FM_DIRECTORY_VIEW_POPUP_PATH_SCRIPTS				"/popups/selection/Open Placeholder/Scripts"
#define FM_DIRECTORY_VIEW_POPUP_PATH_MIME_ACTIONS			"/popups/selection/Mime Actions"

#define MAX_MENU_LEVELS 5

enum {
	ADD_FILE,
	BEGIN_FILE_CHANGES,
	BEGIN_LOADING,
	CLEAR,
	END_FILE_CHANGES,
	END_LOADING,
	FILE_CHANGED,
	LOAD_ERROR,
	MOVE_COPY_ITEMS,
	REMOVE_FILE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static GdkAtom copied_files_atom;

static gboolean show_delete_command_auto_value;
static gboolean confirm_trash_auto_value;
static gboolean use_new_window_auto_value;

static char *scripts_directory_uri;
static int scripts_directory_uri_length;

struct FMDirectoryViewDetails
{
	NautilusView *nautilus_view;
	BonoboZoomable *zoomable;

	NautilusDirectory *model;
	NautilusFile *directory_as_file;
	BonoboUIComponent *ui;

	GList *scripts_directory_list;

	guint display_selection_idle_id;
	guint update_menus_timeout_id;
	
	guint display_pending_timeout_id;
	guint display_pending_idle_id;
	
	guint files_added_handler_id;
	guint files_changed_handler_id;
	guint load_error_handler_id;
	guint done_loading_handler_id;
	guint file_changed_handler_id;

	GList *new_added_files;
	GList *new_changed_files;

	GHashTable *non_ready_files;

	GList *old_added_files;
	GList *old_changed_files;

	GList *pending_uris_selected;

	gboolean loading;
	gboolean menus_merged;
	gboolean menu_states_untrustworthy;
	gboolean scripts_invalid;
	gboolean reported_load_error;

	gboolean sort_directories_first;

	gboolean show_hidden_files;
	gboolean show_backup_files;
	gboolean ignore_hidden_file_preferences;

	gboolean batching_selection_level;
	gboolean selection_changed_while_batched;

	gboolean metadata_for_directory_as_file_pending;
	gboolean metadata_for_files_in_directory_pending;

	gboolean selection_change_is_due_to_shell;
	gboolean send_selection_change_to_shell;

	NautilusFile *file_monitored_for_open_with;
};

typedef enum {
	RESPECT_PREFERENCE,
	PREFER_EXISTING_WINDOW,
	FORCE_NEW_WINDOW
} WindowChoice;

typedef enum {
	ACTIVATION_ACTION_LAUNCH,
	ACTIVATION_ACTION_LAUNCH_IN_TERMINAL,
	ACTIVATION_ACTION_DISPLAY,
	ACTIVATION_ACTION_DO_NOTHING
} ActivationAction;

typedef struct {
	FMDirectoryView *view;
	NautilusFile *file;
	WindowChoice choice;
} ActivateParameters;

typedef struct {
	char *id;
	char *verb;
	CORBA_sequence_CORBA_string *uri_list;
} BonoboMimeActionData;

enum {
	GNOME_COPIED_FILES
};

static const GtkTargetEntry clipboard_targets[] = {
	{ "x-special/gnome-copied-files", 0, GNOME_COPIED_FILES },
};

/* forward declarations */

static void     cancel_activate_callback                       (gpointer              callback_data);
static gboolean display_selection_info_idle_callback           (gpointer              data);
static gboolean file_is_launchable                             (NautilusFile         *file);
static void     fm_directory_view_class_init                   (FMDirectoryViewClass *klass);
static void     fm_directory_view_init                         (FMDirectoryView      *view);
static void     fm_directory_view_duplicate_selection          (FMDirectoryView      *view,
								GList                *files,
								GArray               *item_locations);
static gboolean fm_directory_view_confirm_deletion             (FMDirectoryView      *view,
								GList                *uris,
								gboolean              all);
static void     fm_directory_view_create_links_for_files       (FMDirectoryView      *view,
								GList                *files,
								GArray               *item_locations);
static void     trash_or_delete_files                          (FMDirectoryView      *view,
								const GList          *files);
static void     fm_directory_view_activate_file                (FMDirectoryView      *view,
								NautilusFile         *file,
								WindowChoice          choice);
static void     load_directory                                 (FMDirectoryView      *view,
								NautilusDirectory    *directory);
static void     fm_directory_view_merge_menus                  (FMDirectoryView      *view);
static char *   file_name_from_uri                             (const char           *uri);
static void     stop_loading_callback                          (NautilusView         *nautilus_view,
								FMDirectoryView      *directory_view);
static void     load_location_callback                         (NautilusView         *nautilus_view,
								const char           *location,
								FMDirectoryView      *directory_view);
static void     selection_changed_callback                     (NautilusView         *nautilus_view,
								GList                *selection,
								FMDirectoryView      *directory_view);
static void     open_one_in_new_window                         (gpointer              data,
								gpointer              callback_data);
static void     open_one_properties_window                     (gpointer              data,
								gpointer              callback_data);
static void     zoomable_set_zoom_level_callback               (BonoboZoomable       *zoomable,
								float                 level,
								FMDirectoryView      *view);
static void     zoomable_zoom_in_callback                      (BonoboZoomable       *zoomable,
								FMDirectoryView      *directory_view);
static void     zoomable_zoom_out_callback                     (BonoboZoomable       *zoomable,
								FMDirectoryView      *directory_view);
static void     zoomable_zoom_to_fit_callback                  (BonoboZoomable       *zoomable,
								FMDirectoryView      *directory_view);
static void     schedule_update_menus                          (FMDirectoryView      *view);
static void     schedule_update_menus_callback                 (gpointer              callback_data);
static void     remove_update_menus_timeout_callback           (FMDirectoryView      *view);
static void     schedule_idle_display_of_pending_files         (FMDirectoryView      *view);
static void     unschedule_idle_display_of_pending_files       (FMDirectoryView      *view);
static void     schedule_timeout_display_of_pending_files      (FMDirectoryView      *view);
static void     unschedule_timeout_display_of_pending_files    (FMDirectoryView      *view);
static void     unschedule_display_of_pending_files            (FMDirectoryView      *view);
static void     disconnect_model_handlers                      (FMDirectoryView      *view);
static void     filtering_changed_callback                     (gpointer              callback_data);
static void     metadata_for_directory_as_file_ready_callback  (NautilusFile         *file,
								gpointer              callback_data);
static void     metadata_for_files_in_directory_ready_callback (NautilusDirectory    *directory,
								GList                *files,
								gpointer              callback_data);
static void     fm_directory_view_trash_state_changed_callback (NautilusTrashMonitor *trash,
								gboolean              state,
								gpointer              callback_data);
static void     fm_directory_view_select_file                  (FMDirectoryView      *view,
								NautilusFile         *file);
static void     monitor_file_for_open_with                     (FMDirectoryView      *view,
								NautilusFile         *file);

EEL_CLASS_BOILERPLATE (FMDirectoryView, fm_directory_view, GTK_TYPE_SCROLLED_WINDOW)

EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, add_file)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, bump_zoom_level)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, can_zoom_in)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, can_zoom_out)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, clear)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, file_changed)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, get_background_widget)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, get_selection)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, is_empty)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, reset_to_defaults)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, restore_default_zoom_level)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, select_all)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, set_selection)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (fm_directory_view, zoom_to_level)

typedef struct {
	GnomeVFSMimeApplication *application;
	NautilusFile *file;
	FMDirectoryView *directory_view;
} ApplicationLaunchParameters;

typedef struct {
	NautilusViewIdentifier *identifier;
	char *uri;
	FMDirectoryView *directory_view;
} ViewerLaunchParameters;

typedef struct {
	NautilusFile *file;
	FMDirectoryView *directory_view;
} ScriptLaunchParameters;

static ApplicationLaunchParameters *
application_launch_parameters_new (GnomeVFSMimeApplication *application,
			      	   NautilusFile *file,
			           FMDirectoryView *directory_view)
{
	ApplicationLaunchParameters *result;

	result = g_new0 (ApplicationLaunchParameters, 1);
	result->application = gnome_vfs_mime_application_copy (application);
	g_object_ref (directory_view);
	result->directory_view = directory_view;
	nautilus_file_ref (file);
	result->file = file;

	return result;
}

static void
application_launch_parameters_free (ApplicationLaunchParameters *parameters)
{
	gnome_vfs_mime_application_free (parameters->application);
	g_object_unref (parameters->directory_view);
	nautilus_file_unref (parameters->file);
	g_free (parameters);
}			      

static ViewerLaunchParameters *
viewer_launch_parameters_new (NautilusViewIdentifier *identifier,
			      const char *uri,
			      FMDirectoryView *directory_view)
{
	ViewerLaunchParameters *result;

	result = g_new0 (ViewerLaunchParameters, 1);
	result->identifier = nautilus_view_identifier_copy (identifier);
	g_object_ref (directory_view);
	result->directory_view = directory_view;
	result->uri = g_strdup (uri);

	return result;
}

static void
viewer_launch_parameters_free (ViewerLaunchParameters *parameters)
{
	nautilus_view_identifier_free (parameters->identifier);
	g_object_unref (parameters->directory_view);
	g_free (parameters->uri);
	g_free (parameters);
}			      

static ScriptLaunchParameters *
script_launch_parameters_new (NautilusFile *file,
			      FMDirectoryView *directory_view)
{
	ScriptLaunchParameters *result;

	result = g_new0 (ScriptLaunchParameters, 1);
	g_object_ref (directory_view);
	result->directory_view = directory_view;
	nautilus_file_ref (file);
	result->file = file;

	return result;
}

static void
script_launch_parameters_free (ScriptLaunchParameters *parameters)
{
	g_object_unref (parameters->directory_view);
	nautilus_file_unref (parameters->file);
	g_free (parameters);
}			      

/* Returns the GtkWindow that this directory view occupies, or NULL
 * if at the moment this directory view is not in a GtkWindow or the
 * GtkWindow cannot be determined. Primarily used for parenting dialogs.
 */
GtkWindow *
fm_directory_view_get_containing_window (FMDirectoryView *view)
{
	GtkWidget *window;

	g_assert (FM_IS_DIRECTORY_VIEW (view));
	
	window = gtk_widget_get_ancestor (GTK_WIDGET (view), GTK_TYPE_WINDOW);
	if (window == NULL) {
		return NULL;
	}

	return GTK_WINDOW (window);
}

gboolean
fm_directory_view_confirm_multiple_windows (FMDirectoryView *view, int count)
{
	GtkDialog *dialog;
	char *prompt;
	char *title;
	int response;

	if (count <= SILENT_WINDOW_OPEN_LIMIT) {
		return TRUE;
	}

	prompt = g_strdup_printf (_("This will open %d separate windows. "
				    "Are you sure you want to do this?"), count);
	title = g_strdup_printf (_("Open %d Windows?"), count);
	dialog = eel_show_yes_no_dialog (prompt, title, 
					 GTK_STOCK_OK, GTK_STOCK_CANCEL,
					 fm_directory_view_get_containing_window (view));
	g_free (prompt);
	g_free (title);

	response = gtk_dialog_run (dialog);
	gtk_object_destroy (GTK_OBJECT (dialog));

	return response == GTK_RESPONSE_YES;
}

static gboolean
selection_contains_one_item_in_menu_callback (FMDirectoryView *view, GList *selection)
{
	if (eel_g_list_exactly_one_item (selection)) {
		return TRUE;
	}

	/* If we've requested a menu update that hasn't yet occurred, then
	 * the mismatch here doesn't surprise us, and we won't complain.
	 * Otherwise, we will complain.
	 */
	if (!view->details->menu_states_untrustworthy) {
		g_warning ("Expected one selected item, found %d. No action will be performed.", 	
			   g_list_length (selection));
	}

	return FALSE;
}

static gboolean
selection_not_empty_in_menu_callback (FMDirectoryView *view, GList *selection)
{
	if (selection != NULL) {
		return TRUE;
	}

	/* If we've requested a menu update that hasn't yet occurred, then
	 * the mismatch here doesn't surprise us, and we won't complain.
	 * Otherwise, we will complain.
	 */
	if (!view->details->menu_states_untrustworthy) {
		g_warning ("Empty selection found when selection was expected. No action will be performed.");
	}

	return FALSE;
}

static void
open_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	GList *selection;
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	selection = fm_directory_view_get_selection (view);
	fm_directory_view_activate_files (view, selection);
	nautilus_file_list_free (selection);
}

static void
open_alternate_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	FMDirectoryView *view;
	GList *selection;
	char *uri;

	view = FM_DIRECTORY_VIEW (callback_data);
	selection = fm_directory_view_get_selection (view);

	if (use_new_window_auto_value) {
	        /* UI should have prevented this from being called unless exactly
	         * one item is selected.
	         */
	        if (selection_contains_one_item_in_menu_callback (view, selection)) {
	        	uri = nautilus_file_get_uri (NAUTILUS_FILE (selection->data));
			nautilus_view_open_location_in_this_window
				(view->details->nautilus_view, uri);
			g_free (uri);
	        }        
	} else {
		if (fm_directory_view_confirm_multiple_windows (view, g_list_length (selection))) {
			g_list_foreach (selection, open_one_in_new_window, view);
		}
	}

	nautilus_file_list_free (selection);
}

static void
fm_directory_view_launch_application (GnomeVFSMimeApplication *application,
				      NautilusFile *file,
				      FMDirectoryView *directory_view)
{
	char *uri;

	g_assert (application != NULL);
	g_assert (NAUTILUS_IS_FILE (file));
	g_assert (FM_IS_DIRECTORY_VIEW (directory_view));

	nautilus_launch_application
		(application, file, fm_directory_view_get_containing_window (directory_view));
	
	uri = nautilus_file_get_uri (file);
	egg_recent_model_add (nautilus_recent_get_model (), uri);

	g_free (uri);
}				      

static void
fm_directory_view_chose_application_callback (GnomeVFSMimeApplication *application, 
					      gpointer callback_data)
{
	ApplicationLaunchParameters *launch_parameters;

	g_assert (callback_data != NULL);

	launch_parameters = (ApplicationLaunchParameters *)callback_data;
	g_assert (launch_parameters->application == NULL);

	if (application != NULL) {
		fm_directory_view_launch_application 
			(application, /* NOT the (empty) application in launch_parameters */
			 launch_parameters->file,
			 launch_parameters->directory_view);
	}

	application_launch_parameters_free (launch_parameters);
}

static void
open_location (FMDirectoryView *directory_view, 
	       const char *new_uri, 
	       WindowChoice choice)
{
	g_assert (FM_IS_DIRECTORY_VIEW (directory_view));
	g_assert (new_uri != NULL);

	switch (choice) {
	case RESPECT_PREFERENCE:
		if (use_new_window_auto_value) {
			nautilus_view_open_location_prefer_existing_window
				(directory_view->details->nautilus_view, new_uri);
		} else {
			nautilus_view_open_location_in_this_window
				(directory_view->details->nautilus_view, new_uri);
		}
		break;
	case PREFER_EXISTING_WINDOW:
		nautilus_view_open_location_prefer_existing_window
			(directory_view->details->nautilus_view, new_uri);
		break;
	case FORCE_NEW_WINDOW:
		nautilus_view_open_location_force_new_window
			(directory_view->details->nautilus_view, new_uri, NULL);
		break;
	}
}

static void
switch_location_and_view (NautilusViewIdentifier *identifier, 
			  const char *new_uri, 
			  FMDirectoryView *directory_view)
{
	NautilusFile *file;

	g_assert (FM_IS_DIRECTORY_VIEW (directory_view));
	g_assert (identifier != NULL);
	g_assert (new_uri != NULL);

	/* User has explicitly chosen a viewer other than the default, so
	 * make it the default and then switch locations.
	 */
	/* FIXME bugzilla.gnome.org 41053: We might want an atomic operation
	 * for switching location and viewer together, so we don't have to
	 * rely on metadata for holding the default location.
	 */
	file = nautilus_file_get (new_uri);
	nautilus_mime_set_default_component_for_file (file, identifier->iid);
	nautilus_file_unref (file);

	open_location (directory_view, new_uri, RESPECT_PREFERENCE);
}

static void
fm_directory_view_chose_component_callback (NautilusViewIdentifier *identifier, 
					    gpointer callback_data)
{
	ViewerLaunchParameters *launch_parameters;

	g_assert (callback_data != NULL);

	launch_parameters = (ViewerLaunchParameters *)callback_data;
	g_assert (launch_parameters->identifier == NULL);

	if (identifier != NULL) {
		switch_location_and_view (identifier, /* NOT the (empty) identifier in launch_parameters */
					  launch_parameters->uri, 
					  launch_parameters->directory_view);
	}

	viewer_launch_parameters_free (launch_parameters);
}

static void
choose_program (FMDirectoryView *view,
		NautilusFile *file,
		GnomeVFSMimeActionType type)
{
	char *uri;

	g_assert (FM_IS_DIRECTORY_VIEW (view));
	g_assert (NAUTILUS_IS_FILE (file));
	g_assert (type == GNOME_VFS_MIME_ACTION_TYPE_COMPONENT ||
		  type == GNOME_VFS_MIME_ACTION_TYPE_APPLICATION);

	nautilus_file_ref (file);
	uri = nautilus_file_get_uri (file);

	if (type == GNOME_VFS_MIME_ACTION_TYPE_COMPONENT) {
		nautilus_choose_component_for_file 
			(file,
			 fm_directory_view_get_containing_window (view),
			 fm_directory_view_chose_component_callback,
			 viewer_launch_parameters_new
			 	(NULL, uri, view));
	} else {
		nautilus_choose_application_for_file 
			(file,
			 fm_directory_view_get_containing_window (view),
			 fm_directory_view_chose_application_callback,
			 application_launch_parameters_new
			 	(NULL, file, view));
	}

	g_free (uri);
	nautilus_file_unref (file);	
}

static void
open_with_other_program (FMDirectoryView *view, GnomeVFSMimeActionType action_type)
{
        GList *selection;

	g_assert (FM_IS_DIRECTORY_VIEW (view));
	g_assert (action_type == GNOME_VFS_MIME_ACTION_TYPE_APPLICATION ||
		  action_type == GNOME_VFS_MIME_ACTION_TYPE_COMPONENT);

       	selection = fm_directory_view_get_selection (view);

	if (selection_contains_one_item_in_menu_callback (view, selection)) {
		choose_program (view, NAUTILUS_FILE (selection->data), action_type);
	}

	nautilus_file_list_free (selection);
}

static void
other_application_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	open_with_other_program (FM_DIRECTORY_VIEW (callback_data), 
				 GNOME_VFS_MIME_ACTION_TYPE_APPLICATION);
}

static void
other_viewer_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	open_with_other_program (FM_DIRECTORY_VIEW (callback_data), 
				 GNOME_VFS_MIME_ACTION_TYPE_COMPONENT);
}

static void
edit_launcher (FMDirectoryView *view,
	       NautilusFile    *file)
{
	char *uri;

	uri = nautilus_file_get_uri (file);

	nautilus_launch_application_from_command (gtk_widget_get_screen (GTK_WIDGET (view)),
						  "gnome-desktop-item-edit", 
						  "gnome-desktop-item-edit",
						  uri, 
						  FALSE);
	g_free (uri);
}

static void
edit_launcher_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	GList *selection;
	FMDirectoryView *view;

	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	view = FM_DIRECTORY_VIEW (callback_data);

       	selection = fm_directory_view_get_selection (view);

	if (selection_contains_one_item_in_menu_callback (view, selection)) {
		edit_launcher (view, NAUTILUS_FILE (selection->data));
	}

	nautilus_file_list_free (selection);
}

static void
trash_or_delete_selected_files (FMDirectoryView *view)
{
        GList *selection;
        
	selection = fm_directory_view_get_selection (view);
	trash_or_delete_files (view, selection);					 
        nautilus_file_list_free (selection);
}

static void
trash_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
        trash_or_delete_selected_files (FM_DIRECTORY_VIEW (callback_data));
}

static gboolean
confirm_delete_directly (FMDirectoryView *view, 
			 GList *uris)
{
	GtkDialog *dialog;
	char *prompt;
	char *file_name;
	int uri_count;
	int response;

	g_assert (FM_IS_DIRECTORY_VIEW (view));

	/* Just Say Yes if the preference says not to confirm. */
	if (!confirm_trash_auto_value) {
		return TRUE;
	}

	uri_count = g_list_length (uris);
	g_assert (uri_count > 0);

	if (uri_count == 1) {
		file_name = file_name_from_uri ((char *) uris->data);
		prompt = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\"?"), 
					  file_name);
		g_free (file_name);
	} else {
		prompt = g_strdup_printf (_("Are you sure you want to permanently delete "
		  			    "the %d selected items?"), uri_count);
	}

	dialog = eel_show_yes_no_dialog
		(prompt,
		 _("Delete?"), GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
		 fm_directory_view_get_containing_window (view));

	g_free (prompt);

	response = gtk_dialog_run (dialog);
	gtk_object_destroy (GTK_OBJECT (dialog));

	return response == GTK_RESPONSE_YES;
}

static void
delete_selected_files (FMDirectoryView *view)
{
        GList *selection;
	GList *node;
	GList *file_uris;

	selection = fm_directory_view_get_selection (view);
	if (selection == NULL) {
		return;
	}

	file_uris = NULL;
	for (node = selection; node != NULL; node = node->next) {
		file_uris = g_list_prepend (file_uris,
					    nautilus_file_get_uri ((NautilusFile *) node->data));
	}
	
	if (confirm_delete_directly (view, 
				     file_uris)) {
		nautilus_file_operations_delete (file_uris, GTK_WIDGET (view));
	}
	
	eel_g_list_free_deep (file_uris);
        nautilus_file_list_free (selection);
}

static void
delete_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	if (!show_delete_command_auto_value) {
		return;
	}
        delete_selected_files (FM_DIRECTORY_VIEW (callback_data));
}

static void
duplicate_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
        FMDirectoryView *view;
        GList *selection;
        GArray *selected_item_locations;
 
        view = FM_DIRECTORY_VIEW (callback_data);
	selection = fm_directory_view_get_selection (view);
	if (selection_not_empty_in_menu_callback (view, selection)) {
		/* FIXME bugzilla.gnome.org 45061:
		 * should change things here so that we use a get_icon_locations (view, selection).
		 * Not a problem in this case but in other places the selection may change by
		 * the time we go and retrieve the icon positions, relying on the selection
		 * staying intact to ensure the right sequence and count of positions is fragile.
		 */
		selected_item_locations = fm_directory_view_get_selected_icon_locations (view);
	        fm_directory_view_duplicate_selection (view, selection, selected_item_locations);
	        g_array_free (selected_item_locations, TRUE);
	}

        nautilus_file_list_free (selection);
}

static void
create_link_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
        FMDirectoryView *view;
        GList *selection;
        GArray *selected_item_locations;
        
        g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

        view = FM_DIRECTORY_VIEW (callback_data);
	selection = fm_directory_view_get_selection (view);
	if (selection_not_empty_in_menu_callback (view, selection)) {
		selected_item_locations = fm_directory_view_get_selected_icon_locations (view);
	        fm_directory_view_create_links_for_files (view, selection, selected_item_locations);
	        g_array_free (selected_item_locations, TRUE);
	}

        nautilus_file_list_free (selection);
}

static void
bonobo_menu_select_all_callback (BonoboUIComponent *component, 
				 gpointer callback_data, 
				 const char *verb)
{
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	fm_directory_view_select_all (callback_data);
}

static void
reset_to_defaults_callback (BonoboUIComponent *component, 
			    gpointer callback_data, 
			    const char *verb)
{
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	fm_directory_view_reset_to_defaults (callback_data);
}

static void
bonobo_menu_empty_trash_callback (BonoboUIComponent *component, 
				  gpointer callback_data, 
				  const char *verb)
{                
        g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	nautilus_file_operations_empty_trash (GTK_WIDGET (callback_data));
}

static void
new_folder_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{                
        g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	fm_directory_view_new_folder (FM_DIRECTORY_VIEW (callback_data));
}

static void
new_launcher_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	char *parent_uri;
	FMDirectoryView *view;

	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	view = FM_DIRECTORY_VIEW (callback_data);

	parent_uri = fm_directory_view_get_uri (view);

	nautilus_launch_application_from_command (gtk_widget_get_screen (GTK_WIDGET (view)),
						  "gnome-desktop-item-edit", 
						  "gnome-desktop-item-edit --create-new",
						  parent_uri, 
						  FALSE);

	g_free (parent_uri);
}

static void
open_properties_window_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
        FMDirectoryView *view;
        GList *selection;
        
        g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

        view = FM_DIRECTORY_VIEW (callback_data);
	selection = fm_directory_view_get_selection (view);
	if (selection_not_empty_in_menu_callback (view, selection)) {
		if (fm_directory_view_confirm_multiple_windows (view, g_list_length (selection))) {
			g_list_foreach (selection, open_one_properties_window, view);
		}
	}

        nautilus_file_list_free (selection);
}

static gboolean
all_files_in_trash (GList *files)
{
	GList *node;

	/* Result is ambiguous if called on NULL, so disallow. */
	g_return_val_if_fail (files != NULL, FALSE);

	for (node = files; node != NULL; node = node->next) {
		if (!nautilus_file_is_in_trash (NAUTILUS_FILE (node->data))) {
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean
all_selected_items_in_trash (FMDirectoryView *view)
{
	GList *selection;
	gboolean result;

	/* If the contents share a parent directory, we need only
	 * check that parent directory. Otherwise we have to inspect
	 * each selected item.
	 */
	selection = fm_directory_view_get_selection (view);
	result = (selection == NULL) ? FALSE : all_files_in_trash (selection);
	nautilus_file_list_free (selection);

	return result;
}

static gboolean
we_are_in_vfolder_desktop_dir (FMDirectoryView *view)
{
	NautilusFile *file;
	char *mime_type;

	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	if (view->details->model == NULL) {
		return FALSE;
	}

	file = nautilus_directory_get_corresponding_file (view->details->model);
	mime_type = nautilus_file_get_mime_type (file);
	nautilus_file_unref (file);

	if (mime_type != NULL
	    && strcmp (mime_type, "x-directory/vfolder-desktop") == 0) {
		g_free (mime_type);
		return TRUE;
	} else {
		g_free (mime_type);
		return FALSE;
	}
}

static void
bonobo_control_activate_callback (BonoboObject *control, gboolean state, gpointer callback_data)
{
        FMDirectoryView *view;

        g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

        view = FM_DIRECTORY_VIEW (callback_data);

        if (state) {
                /* Add new menu items and perhaps whole menus */
                fm_directory_view_merge_menus (view);

	        /* Set initial sensitivity, wording, toggle state, etc. */       
                fm_directory_view_update_menus (view);
        }

        /* 
         * Nothing to do on deactivate case, which never happens because
         * of the way Nautilus content views are handled.
         */
}

/* Preferences changed callbacks */
static void
text_attribute_names_changed_callback (gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 text_attribute_names_changed, (view));
}

static void
image_display_policy_changed_callback (gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 image_display_policy_changed, (view));
}

static void
click_policy_changed_callback (gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 click_policy_changed, (view));
}

gboolean
fm_directory_view_should_sort_directories_first (FMDirectoryView *view)
{
	return view->details->sort_directories_first;
}

static void
sort_directories_first_changed_callback (gpointer callback_data)
{
	FMDirectoryView *view;
	gboolean preference_value;

	view = FM_DIRECTORY_VIEW (callback_data);

	preference_value = 
		eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST);

	if (preference_value != view->details->sort_directories_first) {
		view->details->sort_directories_first = preference_value;
		EEL_CALL_METHOD
			(FM_DIRECTORY_VIEW_CLASS, view,
			 sort_directories_first_changed, (view));
	}
}

static float fm_directory_view_preferred_zoom_levels[] = {
	(float) NAUTILUS_ICON_SIZE_SMALLEST	/ NAUTILUS_ICON_SIZE_STANDARD,
	(float) NAUTILUS_ICON_SIZE_SMALLER	/ NAUTILUS_ICON_SIZE_STANDARD,
	(float) NAUTILUS_ICON_SIZE_SMALL	/ NAUTILUS_ICON_SIZE_STANDARD,
	(float) NAUTILUS_ICON_SIZE_STANDARD	/ NAUTILUS_ICON_SIZE_STANDARD,
	(float) NAUTILUS_ICON_SIZE_LARGE	/ NAUTILUS_ICON_SIZE_STANDARD,
	(float) NAUTILUS_ICON_SIZE_LARGER	/ NAUTILUS_ICON_SIZE_STANDARD,
	(float) NAUTILUS_ICON_SIZE_LARGEST	/ NAUTILUS_ICON_SIZE_STANDARD
};

static void
set_up_scripts_directory_global (void)
{
	char *scripts_directory_path;

	if (scripts_directory_uri != NULL) {
		return;
	}

	scripts_directory_path = gnome_util_home_file ("nautilus-scripts");

	scripts_directory_uri = gnome_vfs_get_uri_from_local_path (scripts_directory_path);
	scripts_directory_uri_length = strlen (scripts_directory_uri);
	
	g_free (scripts_directory_path);
}

static void
create_scripts_directory (void)
{
	char *path;

	set_up_scripts_directory_global ();
	path = gnome_vfs_get_local_path_from_uri (scripts_directory_uri);
	mkdir (path, 
	       GNOME_VFS_PERM_USER_ALL | GNOME_VFS_PERM_GROUP_ALL | GNOME_VFS_PERM_OTHER_READ);
	g_free (path);
}

static void
scripts_added_or_changed_callback (NautilusDirectory *directory,
				   GList *files,
				   gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	view->details->scripts_invalid = TRUE;
	schedule_update_menus (view);
}

static void
icons_changed_callback (gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	view->details->scripts_invalid = TRUE;
	schedule_update_menus (view);
}

static void
add_directory_to_scripts_directory_list (FMDirectoryView *view,
					 NautilusDirectory *directory)
{
	GList *attributes;

	if (g_list_find (view->details->scripts_directory_list, directory) == NULL) {
		nautilus_directory_ref (directory);

		attributes = nautilus_icon_factory_get_required_file_attributes ();
		attributes = g_list_prepend (attributes, NAUTILUS_FILE_ATTRIBUTE_CAPABILITIES);
		attributes = g_list_prepend (attributes, NAUTILUS_FILE_ATTRIBUTE_DIRECTORY_ITEM_COUNT);
 
		nautilus_directory_file_monitor_add (directory, &view->details->scripts_directory_list,
						     FALSE, FALSE, attributes,
						     scripts_added_or_changed_callback, view);

		g_list_free (attributes);

		g_signal_connect_object (directory, "files_added",
					 G_CALLBACK (scripts_added_or_changed_callback), view, 0);
		g_signal_connect_object (directory, "files_changed",
					 G_CALLBACK (scripts_added_or_changed_callback), view, 0);

		view->details->scripts_directory_list = g_list_append
			(view->details->scripts_directory_list, directory);
	}
}

static void
remove_directory_from_scripts_directory_list (FMDirectoryView *view,
					      NautilusDirectory *directory)
{
	view->details->scripts_directory_list = g_list_remove
		(view->details->scripts_directory_list, directory);

	g_signal_handlers_disconnect_by_func (directory,
					      G_CALLBACK (scripts_added_or_changed_callback),
					      view);

	nautilus_directory_file_monitor_remove (directory, &view->details->scripts_directory_list);

	nautilus_directory_unref (directory);
}

static void
fm_directory_view_init (FMDirectoryView *view)
{
	static gboolean setup_autos = FALSE;
	NautilusDirectory *scripts_directory;

	if (!setup_autos) {
		setup_autos = TRUE;
		eel_preferences_add_auto_boolean (NAUTILUS_PREFERENCES_CONFIRM_TRASH,
						  &confirm_trash_auto_value);
		eel_preferences_add_auto_boolean (NAUTILUS_PREFERENCES_ENABLE_DELETE,
						  &show_delete_command_auto_value);
		eel_preferences_add_auto_boolean (NAUTILUS_PREFERENCES_WINDOW_ALWAYS_NEW,
						  &use_new_window_auto_value);
	}

	view->details = g_new0 (FMDirectoryViewDetails, 1);

	view->details->non_ready_files = g_hash_table_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (view), NULL);
	gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (view), NULL);

	view->details->nautilus_view = nautilus_view_new (GTK_WIDGET (view));

	set_up_scripts_directory_global ();

	scripts_directory = nautilus_directory_get (scripts_directory_uri);
	add_directory_to_scripts_directory_list (view, scripts_directory);
	nautilus_directory_unref (scripts_directory);

	view->details->zoomable = bonobo_zoomable_new ();
	bonobo_zoomable_set_parameters_full (view->details->zoomable,
					     0.0, .25, 4.0, TRUE, TRUE, FALSE,
					     fm_directory_view_preferred_zoom_levels, NULL,
					     G_N_ELEMENTS (fm_directory_view_preferred_zoom_levels));
	bonobo_object_add_interface (BONOBO_OBJECT (view->details->nautilus_view),
				     BONOBO_OBJECT (view->details->zoomable));

	view->details->sort_directories_first = 
		eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST);

	g_signal_connect_object (view->details->nautilus_view,  "stop_loading",
				 G_CALLBACK (stop_loading_callback), view, 0);
	g_signal_connect_object (view->details->nautilus_view, "load_location",
			  G_CALLBACK (load_location_callback), view, 0);

	nautilus_view_set_listener_mask
		(NAUTILUS_VIEW (view->details->nautilus_view),
		 NAUTILUS_VIEW_LISTEN_SELECTION);

	g_signal_connect_object (view->details->nautilus_view, "selection_changed",
				 G_CALLBACK (selection_changed_callback), view, 0);
        g_signal_connect_object (fm_directory_view_get_bonobo_control (view), "activate",
				 G_CALLBACK (bonobo_control_activate_callback), view, 0);
	g_signal_connect_object (view->details->zoomable, "zoom_in",
				 G_CALLBACK (zoomable_zoom_in_callback), view, 0);
	g_signal_connect_object (view->details->zoomable, "zoom_out", 
				 G_CALLBACK (zoomable_zoom_out_callback), view, 0);
	g_signal_connect_object (view->details->zoomable, "set_zoom_level", 
				 G_CALLBACK (zoomable_set_zoom_level_callback), view, 0);
	g_signal_connect_object (view->details->zoomable, "zoom_to_fit",
				 G_CALLBACK (zoomable_zoom_to_fit_callback), view, 0);
	g_signal_connect_object (nautilus_trash_monitor_get (), "trash_state_changed",
				 G_CALLBACK (fm_directory_view_trash_state_changed_callback), view, 0);

	/* React to icon theme changes. */
	g_signal_connect_object (nautilus_icon_factory_get (), "icons_changed",
				 G_CALLBACK (icons_changed_callback),
				 view, G_CONNECT_SWAPPED);

	gtk_widget_show (GTK_WIDGET (view));

	filtering_changed_callback (view);
	
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_WINDOW_ALWAYS_NEW,
				      schedule_update_menus_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
				      filtering_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
				      filtering_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_CONFIRM_TRASH,
				      schedule_update_menus_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_ENABLE_DELETE,
				      schedule_update_menus_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS,
				      text_attribute_names_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
				      image_display_policy_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_CLICK_POLICY,
				      click_policy_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST, 
				      sort_directories_first_changed_callback, view);

}

static void
fm_directory_view_destroy (GtkObject *object)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (object);

	disconnect_model_handlers (view);

	/* Since we are owned by the NautilusView, if we're going it's
	 * gone. It would be even better to NULL this out when the
	 * NautilusView goes away, but this is good enough for our
	 * purposes.
	 */
	view->details->zoomable = NULL;
	view->details->nautilus_view = NULL;
	
	monitor_file_for_open_with (view, NULL);

	fm_directory_view_stop (view);
	fm_directory_view_clear (view);

	EEL_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
fm_directory_view_finalize (GObject *object)
{
	FMDirectoryView *view;
	GList *node, *next;

	view = FM_DIRECTORY_VIEW (object);

	for (node = view->details->scripts_directory_list; node != NULL; node = next) {
		next = node->next;
		remove_directory_from_scripts_directory_list (view, node->data);
	}

	nautilus_directory_unref (view->details->model);
	view->details->model = NULL;
	nautilus_file_unref (view->details->directory_as_file);

	if (view->details->display_selection_idle_id != 0) {
		gtk_idle_remove (view->details->display_selection_idle_id);
	}

	remove_update_menus_timeout_callback (view);

	fm_directory_view_ignore_hidden_file_preferences (view);

	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_WINDOW_ALWAYS_NEW,
					 schedule_update_menus_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
					 filtering_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
					 filtering_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_CONFIRM_TRASH,
					 schedule_update_menus_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_ENABLE_DELETE,
					 schedule_update_menus_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS,
					 text_attribute_names_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
					 image_display_policy_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_CLICK_POLICY,
					 click_policy_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST,
					 sort_directories_first_changed_callback, view);

	g_hash_table_destroy (view->details->non_ready_files);

	eel_remove_weak_pointer (&view->details->ui);

	g_free (view->details);

	EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/**
 * fm_directory_view_display_selection_info:
 *
 * Display information about the current selection, and notify the view frame of the changed selection.
 * @view: FMDirectoryView for which to display selection info.
 * 
 **/
void
fm_directory_view_display_selection_info (FMDirectoryView *view)
{
	GList *selection;
	GnomeVFSFileSize non_folder_size;
	guint non_folder_count, folder_count, folder_item_count;
	gboolean folder_item_count_known;
	guint item_count;
	GList *p;
	char *first_item_name;
	char *non_folder_str;
	char *folder_count_str;
	char *folder_item_count_str;
	char *status_string;
	NautilusFile *file;

	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	selection = fm_directory_view_get_selection (view);
	
	folder_item_count_known = TRUE;
	folder_count = 0;
	folder_item_count = 0;
	non_folder_count = 0;
	non_folder_size = 0;
	first_item_name = NULL;
	folder_count_str = NULL;
	non_folder_str = NULL;
	folder_item_count_str = NULL;
	
	for (p = selection; p != NULL; p = p->next) {
		file = p->data;
		if (nautilus_file_is_directory (file)) {
			folder_count++;
			if (nautilus_file_get_directory_item_count (file, &item_count, NULL)) {
				folder_item_count += item_count;
			} else {
				folder_item_count_known = FALSE;
			}
		} else {
			non_folder_count++;
			non_folder_size += nautilus_file_get_size (file);
		}

		if (first_item_name == NULL) {
			first_item_name = nautilus_file_get_display_name (file);
		}
	}
	
	nautilus_file_list_free (selection);
	
	/* Break out cases for localization's sake. But note that there are still pieces
	 * being assembled in a particular order, which may be a problem for some localizers.
	 */

	if (folder_count != 0) {
		if (folder_count == 1) {
			if (non_folder_count == 0) {
				folder_count_str = g_strdup_printf (_("\"%s\" selected"), first_item_name);
			} else {
				folder_count_str = g_strdup (_("1 folder selected"));
			}
		} else {
			folder_count_str = g_strdup_printf (_("%d folders selected"), folder_count);
		}

		if (folder_count == 1) {
			if (!folder_item_count_known) {
				folder_item_count_str = g_strdup ("");
			} else if (folder_item_count == 0) {
				folder_item_count_str = g_strdup (_(" (containing 0 items)"));
			} else if (folder_item_count == 1) {
				folder_item_count_str = g_strdup (_(" (containing 1 item)"));
			} else {
				folder_item_count_str = g_strdup_printf (_(" (containing %d items)"), folder_item_count);
			}
		}
		else {
			if (!folder_item_count_known) {
				folder_item_count_str = g_strdup ("");
			} else if (folder_item_count == 0) {
				folder_item_count_str = g_strdup (_(" (containing a total of 0 items)"));
			} else if (folder_item_count == 1) {
				folder_item_count_str = g_strdup (_(" (containing a total of 1 item)"));
			} else {
				folder_item_count_str = g_strdup_printf (_(" (containing a total of %d items)"), folder_item_count);
			}
			
		}
	}

	if (non_folder_count != 0) {
		char *size_string;

		size_string = gnome_vfs_format_file_size_for_display (non_folder_size);

		if (folder_count == 0) {
			if (non_folder_count == 1) {
				non_folder_str = g_strdup_printf (_("\"%s\" selected (%s)"), 
								  first_item_name,
								  size_string);
			} else {
				non_folder_str = g_strdup_printf (_("%d items selected (%s)"), 
								  non_folder_count, 
								  size_string);
			}
		} else {
			/* Folders selected also, use "other" terminology */
			if (non_folder_count == 1) {
				non_folder_str = g_strdup_printf (_("1 other item selected (%s)"), 
								  size_string);
			} else {
				non_folder_str = g_strdup_printf (_("%d other items selected (%s)"), 
								  non_folder_count, 
								  size_string);
			}
		}

		g_free (size_string);
	}

	if (folder_count == 0 && non_folder_count == 0)	{
		status_string = g_strdup ("");
	} else if (folder_count == 0) {
		status_string = g_strdup (non_folder_str);
	} else if (non_folder_count == 0) {
		/* No use marking this for translation, since you
		 * can't reorder the strings, which is the main thing
		 * you'd want to do.
		 */
		status_string = g_strdup_printf ("%s%s",
						 folder_count_str, 
						 folder_item_count_str);
	} else {
		/* This is marked for translation in case a localizer
		 * needs to change ", " to something else. The comma
		 * is between the message about the number of folders
		 * and the number of items in those folders and the
		 * message about the number of other items and the
		 * total size of those items.
		 */
		status_string = g_strdup_printf (_("%s%s, %s"), 
						 folder_count_str, 
						 folder_item_count_str,
						 non_folder_str);
	}

	g_free (first_item_name);
	g_free (folder_count_str);
	g_free (folder_item_count_str);
	g_free (non_folder_str);

	nautilus_view_report_status (view->details->nautilus_view,
				     status_string);
	g_free (status_string);
}

void
fm_directory_view_send_selection_change (FMDirectoryView *view)
{
	GList *selection, *uris, *p;

	/* Collect a list of URIs. */
	selection = fm_directory_view_get_selection (view);
	uris = NULL;
	for (p = selection; p != NULL; p = p->next) {
		uris = g_list_prepend (uris, nautilus_file_get_uri (p->data));
	}
	nautilus_file_list_free (selection);

	/* Send the selection change. */
	nautilus_view_report_selection_change (view->details->nautilus_view,
					       uris);

	/* Free the URIs. */
	eel_g_list_free_deep (uris);

	view->details->send_selection_change_to_shell = FALSE;
}

static void
load_location_callback (NautilusView *nautilus_view,
			const char *location,
			FMDirectoryView *directory_view)
{
	NautilusDirectory *directory;

	directory = nautilus_directory_get (location);
	load_directory (directory_view, directory);
	nautilus_directory_unref (directory);
}

static GList *
file_list_from_uri_list (GList *uri_list)
{
	GList *file_list, *node;

	file_list = NULL;
	for (node = uri_list; node != NULL; node = node->next) {
		file_list = g_list_prepend
			(file_list,
			 nautilus_file_get (node->data));
	}
	return g_list_reverse (file_list);
}

static void
selection_changed_callback (NautilusView *nautilus_view,
			    GList *selection_uris,
			    FMDirectoryView *view)
{
	GList *selection;

	if (view->details->loading) {
		eel_g_list_free_deep (view->details->pending_uris_selected);
		view->details->pending_uris_selected = NULL;
	}

	if (!view->details->loading) {
		/* If we aren't still loading, set the selection right now. */
		selection = file_list_from_uri_list (selection_uris);
		view->details->selection_change_is_due_to_shell = TRUE;
		fm_directory_view_set_selection (view, selection);
		view->details->selection_change_is_due_to_shell = FALSE;
		nautilus_file_list_free (selection);
	} else {
		/* If we are still loading, add to the list of pending URIs instead. */
		view->details->pending_uris_selected =
			g_list_concat (view->details->pending_uris_selected,
				       eel_g_str_list_copy (selection_uris));
	}
}

static void
stop_loading_callback (NautilusView *nautilus_view,
		       FMDirectoryView *view)
{
	fm_directory_view_stop (view);
}

static void
fm_directory_view_file_limit_reached (FMDirectoryView *view)
{
	g_assert (FM_IS_DIRECTORY_VIEW (view));

	EEL_CALL_METHOD (FM_DIRECTORY_VIEW_CLASS, view,
		 	       file_limit_reached, (view));
}

static void
real_file_limit_reached (FMDirectoryView *view)
{
	NautilusFile *file;
	GtkDialog *dialog;
	char *directory_name;
	char *message;

	g_assert (FM_IS_DIRECTORY_VIEW (view));

	file = fm_directory_view_get_directory_as_file (view);
	directory_name = nautilus_file_get_display_name (file);

	/* Note that the number of items actually displayed varies somewhat due
	 * to the way files are collected in batches. So you can't assume that
	 * no more than the constant limit are displayed.
	 */
	message = g_strdup_printf (_("The folder \"%s\" contains more files than "
			             "Nautilus can handle. Some files will not be "
			             "displayed."), 
			           directory_name);
	g_free (directory_name);

	dialog = eel_show_warning_dialog (message,
					       _("Too Many Files"),
					       fm_directory_view_get_containing_window (view));
	g_free (message);
}

static void
check_for_directory_hard_limit (FMDirectoryView *view)
{
	if (nautilus_directory_file_list_length_reached (view->details->model)) {
		fm_directory_view_file_limit_reached (view);
	}
}


static void
done_loading (FMDirectoryView *view)
{
	GList *uris_selected, *selection;

	if (!view->details->loading) {
		return;
	}

	/* This can be called during destruction, in which case there
	 * is no NautilusView any more.
	 */
	if (view->details->nautilus_view != NULL) {
		nautilus_view_report_load_complete (view->details->nautilus_view);
		schedule_update_menus (view);
		check_for_directory_hard_limit (view);

		uris_selected = view->details->pending_uris_selected;
		if (uris_selected != NULL) {
			view->details->pending_uris_selected = NULL;
			
			selection = file_list_from_uri_list (uris_selected);
			eel_g_list_free_deep (uris_selected);
			

			view->details->selection_change_is_due_to_shell = TRUE;
			fm_directory_view_set_selection (view, selection);
			view->details->selection_change_is_due_to_shell = FALSE;
			fm_directory_view_reveal_selection (view);
			
			nautilus_file_list_free (selection);
		}
	}

	fm_directory_view_end_loading (view);

	view->details->loading = FALSE;
}

static void
reset_background_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	eel_background_reset 
		(fm_directory_view_get_background 
			(FM_DIRECTORY_VIEW (callback_data)));
}

static void
zoomable_zoom_in_callback (BonoboZoomable *zoomable, FMDirectoryView *directory_view)
{
	fm_directory_view_bump_zoom_level (directory_view, 1);
}

static void
zoomable_zoom_out_callback (BonoboZoomable *zoomable, FMDirectoryView *directory_view)
{
	fm_directory_view_bump_zoom_level (directory_view, -1);
}

static NautilusZoomLevel
nautilus_zoom_level_from_float(float zoom_level)
{
	int icon_size = floor(zoom_level * NAUTILUS_ICON_SIZE_STANDARD + 0.5);
	
	if (icon_size <= NAUTILUS_ICON_SIZE_SMALLEST) {
		return NAUTILUS_ZOOM_LEVEL_SMALLEST;
	} else if (icon_size <= NAUTILUS_ICON_SIZE_SMALLER) {
		return NAUTILUS_ZOOM_LEVEL_SMALLER;
	} else if (icon_size <= NAUTILUS_ICON_SIZE_SMALL) {
		return NAUTILUS_ZOOM_LEVEL_SMALL;
	} else if (icon_size <= NAUTILUS_ICON_SIZE_STANDARD) {
		return NAUTILUS_ZOOM_LEVEL_STANDARD;
	} else if (icon_size <= NAUTILUS_ICON_SIZE_LARGE) {
		return NAUTILUS_ZOOM_LEVEL_LARGE;
	} else if (icon_size <= NAUTILUS_ICON_SIZE_LARGER) {
		return NAUTILUS_ZOOM_LEVEL_LARGER;
	} else {
		return NAUTILUS_ZOOM_LEVEL_LARGEST;
	}
}

static void
zoomable_set_zoom_level_callback (BonoboZoomable *zoomable, float level, FMDirectoryView *view)
{
	fm_directory_view_zoom_to_level (view, nautilus_zoom_level_from_float (level));
}

static void
zoomable_zoom_to_fit_callback (BonoboZoomable *zoomable, FMDirectoryView *view)
{
	/* FIXME bugzilla.gnome.org 42388:
	 * Need to really implement "zoom to fit"
	 */
	fm_directory_view_restore_default_zoom_level (view);
}

typedef struct {
	GHashTable *debuting_uris;
	GList	   *added_files;
} DebutingUriData;

static void
debuting_uri_data_free (DebutingUriData *data)
{
	g_hash_table_destroy (data->debuting_uris);
	nautilus_file_list_free (data->added_files);
	g_free (data);
}
 
/* This signal handler watch for the arrival of the icons created
 * as the result of a file operation. Once the last one is detected
 * it selects and reveals them all.
 */
static void
debuting_uri_add_file_callback (FMDirectoryView *view,
				NautilusFile *new_file,
				DebutingUriData *data)
{
	char *uri;

	uri = nautilus_file_get_uri (new_file);

	if (g_hash_table_remove (data->debuting_uris, uri)) {
		g_object_ref (new_file);
		data->added_files = g_list_prepend (data->added_files, new_file);

		if (g_hash_table_size (data->debuting_uris) == 0) {
			fm_directory_view_set_selection (view, data->added_files);
			fm_directory_view_reveal_selection (view);
			g_signal_handlers_disconnect_by_func (view,
							      G_CALLBACK (debuting_uri_add_file_callback),
							      data);
		}
	}
	
	g_free (uri);
}

typedef struct {
	GList		*added_files;
	FMDirectoryView *directory_view;
} CopyMoveDoneData;

static void
copy_move_done_data_free (CopyMoveDoneData *data)
{
	g_assert (data != NULL);
	
	eel_remove_weak_pointer (&data->directory_view);
	nautilus_file_list_free (data->added_files);
	g_free (data);
}

static void
pre_copy_move_add_file_callback (FMDirectoryView *view, NautilusFile *new_file, CopyMoveDoneData *data)
{
	g_object_ref (new_file);
	data->added_files = g_list_prepend (data->added_files, new_file);
}

/* This needs to be called prior to nautilus_file_operations_copy_move.
 * It hooks up a signal handler to catch any icons that get added before
 * the copy_done_callback is invoked. The return value should  be passed
 * as the data for copy_move_done_callback.
 */
static CopyMoveDoneData *
pre_copy_move (FMDirectoryView *directory_view)
{
	CopyMoveDoneData *copy_move_done_data;

	copy_move_done_data = g_new0 (CopyMoveDoneData, 1);
	copy_move_done_data->directory_view = directory_view;

	eel_add_weak_pointer (&copy_move_done_data->directory_view);

	/* We need to run after the default handler adds the folder we want to
	 * operate on. The ADD_FILE signal is registered as G_SIGNAL_RUN_LAST, so we
	 * must use connect_after.
	 */
	g_signal_connect (directory_view, "add_file",
			  G_CALLBACK (pre_copy_move_add_file_callback), copy_move_done_data);

	return copy_move_done_data;
}

/* This function is used to pull out any debuting uris that were added
 * and (as a side effect) remove them from the debuting uri hash table.
 */
static gboolean
copy_move_done_partition_func (gpointer data, gpointer callback_data)
{
 	char *uri;
 	gboolean result;
 	
	uri = nautilus_file_get_uri (NAUTILUS_FILE (data));
	result = g_hash_table_remove ((GHashTable *) callback_data, uri);
	g_free (uri);

	return result;
}

static gboolean
remove_not_really_moved_files (gpointer key,
			       gpointer value,
			       gpointer callback_data)
{
	GList **added_files;

	if (GPOINTER_TO_INT (value)) {
		return FALSE;
	}
	
	added_files = callback_data;
	*added_files = g_list_prepend (*added_files,
				       nautilus_file_get (key));
	return TRUE;
}


/* When this function is invoked, the file operation is over, but all
 * the icons may not have been added to the directory view yet, so
 * we can't select them yet.
 * 
 * We're passed a hash table of the uri's to look out for, we hook
 * up a signal handler to await their arrival.
 */
static void
copy_move_done_callback (GHashTable *debuting_uris, gpointer data)
{
	FMDirectoryView  *directory_view;
	CopyMoveDoneData *copy_move_done_data;
	DebutingUriData  *debuting_uri_data;

	copy_move_done_data = (CopyMoveDoneData *) data;
	directory_view = copy_move_done_data->directory_view;

	if (directory_view != NULL) {
		g_assert (FM_IS_DIRECTORY_VIEW (directory_view));
	
		debuting_uri_data = g_new (DebutingUriData, 1);
		debuting_uri_data->debuting_uris = debuting_uris;
		debuting_uri_data->added_files = eel_g_list_partition
			(copy_move_done_data->added_files,
			 copy_move_done_partition_func,
			 debuting_uris,
			 &copy_move_done_data->added_files);

		/* We're passed the same data used by pre_copy_move_add_file_callback, so disconnecting
		 * it will free data. We've already siphoned off the added_files we need, and stashed the
		 * directory_view pointer.
		 */
		g_signal_handlers_disconnect_by_func (directory_view,
						      G_CALLBACK (pre_copy_move_add_file_callback),
						      data);
	
		/* Any items in the debuting_uris hash table that have
		 * "FALSE" as their value aren't really being copied
		 * or moved, so we can't wait for an add_file signal
		 * to come in for those.
		 */
		g_hash_table_foreach_remove (debuting_uris,
					     remove_not_really_moved_files,
					     &debuting_uri_data->added_files);
		
		if (g_hash_table_size (debuting_uris) == 0) {
			/* on the off-chance that all the icons have already been added */
			if (debuting_uri_data->added_files != NULL) {
				fm_directory_view_set_selection (directory_view,
								 debuting_uri_data->added_files);
				fm_directory_view_reveal_selection (directory_view);
			}
			debuting_uri_data_free (debuting_uri_data);
		} else {
			/* We need to run after the default handler adds the folder we want to
			 * operate on. The ADD_FILE signal is registered as G_SIGNAL_RUN_LAST, so we
			 * must use connect_after.
			 */
			g_signal_connect_data (GTK_OBJECT (directory_view),
					       "add_file",
					       G_CALLBACK (debuting_uri_add_file_callback),
					       debuting_uri_data,
					       (GClosureNotify) debuting_uri_data_free,
					       G_CONNECT_AFTER);
		}
	}

	copy_move_done_data_free (copy_move_done_data);
}

static gboolean
real_file_still_belongs (FMDirectoryView *view, NautilusFile *file)
{
	return nautilus_directory_contains_file (view->details->model, file);
}

static gboolean
still_should_show_file (FMDirectoryView *view, NautilusFile *file)
{
	return fm_directory_view_should_show_file (view, file)
		&& EEL_INVOKE_METHOD (FM_DIRECTORY_VIEW_CLASS, view, file_still_belongs, (view, file));
}

static gboolean
ready_to_load (NautilusFile *file)
{
	return nautilus_icon_factory_is_basic_icon_ready_for_file (file);
}

/* Go through all the new added and changed files.
 * Put any that are not ready to load in the non_ready_files hash table.
 * Add all the rest to the old_added_files and old_changed_files lists.
 * Sort the old_added_files list if anything is added to it.
 */
static void
process_new_files (FMDirectoryView *view)
{
	GList *new_added_files, *new_changed_files, *old_added_files, *old_changed_files;
	GHashTable *non_ready_files;
	GList *node;
	NautilusFile *file;
	gboolean in_non_ready;

	new_added_files = view->details->new_added_files;
	view->details->new_added_files = NULL;
	new_changed_files = view->details->new_changed_files;
	view->details->new_changed_files = NULL;

	non_ready_files = view->details->non_ready_files;

	old_added_files = view->details->old_added_files;
	old_changed_files = view->details->old_changed_files;

	/* Newly added files go into the old_added_files list if they're
	 * ready, and into the hash table if they're not.
	 */
	for (node = new_added_files; node != NULL; node = node->next) {
		file = NAUTILUS_FILE (node->data);
		in_non_ready = g_hash_table_lookup (non_ready_files, file) != NULL;
		if (fm_directory_view_should_show_file (view, file)) {
			if (ready_to_load (file)) {
				if (in_non_ready) {
					g_hash_table_remove (non_ready_files, file);
					nautilus_file_unref (file);
				}
				nautilus_file_ref (file);
				old_added_files = g_list_prepend (old_added_files, file);
			} else {
				if (!in_non_ready) {
					nautilus_file_ref (file);
					g_hash_table_insert (non_ready_files, file, file);
				}
			}
		}
	}
	nautilus_file_list_free (new_added_files);

	/* Newly changed files go into the old_added_files list if they're ready
	 * and were seen non-ready in the past, into the old_changed_files list
	 * if they are read and were not seen non-ready in the past, and into
	 * the hash table if they're not ready.
	 */
	for (node = new_changed_files; node != NULL; node = node->next) {
		file = NAUTILUS_FILE (node->data);
		if (ready_to_load (file) || !still_should_show_file (view, file)) {
			if (g_hash_table_lookup (non_ready_files, file) != NULL) {
				g_hash_table_remove (non_ready_files, file);
				nautilus_file_unref (file);
				if (still_should_show_file (view, file)) {
					nautilus_file_ref (file);
					old_added_files = g_list_prepend (old_added_files, file);
				}
			} else if (fm_directory_view_should_show_file(view, file)) {
				nautilus_file_ref (file);
				old_changed_files = g_list_prepend 
					(old_changed_files, file);
			}
		}
	}
	nautilus_file_list_free (new_changed_files);

	/* If any files were added to old_added_files, then resort it. */
	if (old_added_files != view->details->old_added_files) {
		view->details->old_added_files = old_added_files;
		EEL_INVOKE_METHOD (FM_DIRECTORY_VIEW_CLASS, view, sort_files,
				   (view, &view->details->old_added_files));
	}
	view->details->old_changed_files = old_changed_files;
}

static GList *
split_off_first_n (GList **list, int removed_count)
{
	GList *first_n_items, *nth_item;

	nth_item = g_list_nth (*list, removed_count);
	first_n_items = *list;

	if (nth_item == NULL) {
		*list = NULL;
	} else {
		nth_item->prev->next = NULL;
		nth_item->prev = NULL;
		*list = nth_item;
	}

	return first_n_items;
}

static void
process_old_files (FMDirectoryView *view)
{
	GList *files_added, *files_changed, *node;
	NautilusFile *file;
	GList *selection;
	gboolean send_selection_change;

	files_added = split_off_first_n (&view->details->old_added_files, FILES_TO_PROCESS_AT_ONCE);
	files_changed = split_off_first_n (&view->details->old_changed_files, FILES_TO_PROCESS_AT_ONCE);
	
	send_selection_change = FALSE;

	if (files_added != NULL || files_changed != NULL) {
		g_signal_emit (view, signals[BEGIN_FILE_CHANGES], 0);

		for (node = files_added; node != NULL; node = node->next) {
			file = NAUTILUS_FILE (node->data);
			g_signal_emit (view,
				       signals[ADD_FILE], 0, file);
		}

		for (node = files_changed; node != NULL; node = node->next) {
			file = NAUTILUS_FILE (node->data);
			
			g_signal_emit (view,
				       signals[still_should_show_file (view, file)
					       ? FILE_CHANGED : REMOVE_FILE], 0,
				       file);
		}

		g_signal_emit (view, signals[END_FILE_CHANGES], 0);

		if (files_changed != NULL) {
			selection = fm_directory_view_get_selection (view);
			send_selection_change = eel_g_lists_sort_and_check_for_intersection
				(&files_changed, &selection);
			nautilus_file_list_free (selection);
		}

		nautilus_file_list_free (files_added);
		nautilus_file_list_free (files_changed);
	}

	if (send_selection_change) {
		/* Send a selection change since some file names could
		 * have changed.
		 */
		fm_directory_view_send_selection_change (view);
	}
}

/* Return FALSE if there is no work remaining. */
static gboolean
display_pending_files (FMDirectoryView *view)
{
	process_new_files (view);
	process_old_files (view);

	if (view->details->old_added_files != NULL
	    || view->details->old_changed_files != NULL) {
		return TRUE;
	}

	if (view->details->model != NULL
	    && nautilus_directory_are_all_files_seen (view->details->model)
	    && g_hash_table_size (view->details->non_ready_files) == 0) {
		done_loading (view);
	}

	return FALSE;
}

static gboolean
display_selection_info_idle_callback (gpointer data)
{
	FMDirectoryView *view;
	
	view = FM_DIRECTORY_VIEW (data);

	g_object_ref (G_OBJECT (view));

	view->details->display_selection_idle_id = 0;
	fm_directory_view_display_selection_info (view);
	if (view->details->send_selection_change_to_shell) {
		fm_directory_view_send_selection_change (view);
	}

	g_object_unref (G_OBJECT (view));

	return FALSE;
}

static void
remove_update_menus_timeout_callback (FMDirectoryView *view) 
{
	if (view->details->update_menus_timeout_id != 0) {
		gtk_timeout_remove (view->details->update_menus_timeout_id);
		view->details->update_menus_timeout_id = 0;
	}
}

static void
update_menus_if_pending (FMDirectoryView *view)
{
	if (!view->details->menu_states_untrustworthy) {
		return;
	}

	remove_update_menus_timeout_callback (view);
	fm_directory_view_update_menus (view);
}

static gboolean
update_menus_timeout_callback (gpointer data)
{
	FMDirectoryView *view;
	
	view = FM_DIRECTORY_VIEW (data);

	g_object_ref (G_OBJECT (view));

	view->details->update_menus_timeout_id = 0;
	fm_directory_view_update_menus (view);

	g_object_unref (G_OBJECT (view));

	return FALSE;
}

static gboolean
display_pending_idle_callback (gpointer data)
{
	gboolean ret;
	FMDirectoryView *view;
		
	view = FM_DIRECTORY_VIEW (data);

	g_object_ref (G_OBJECT (view));

	if (display_pending_files (view)) {
		ret = TRUE;
	} else {
		ret = FALSE;
		view->details->display_pending_idle_id = 0;
	}

	g_object_unref (G_OBJECT (view));

	return ret;
}

static gboolean
display_pending_timeout_callback (gpointer data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (data);

	g_object_ref (G_OBJECT (view));

	view->details->display_pending_timeout_id = 0;

	/* If we have more files to do, use an idle, not another timeout. */
	if (display_pending_files (view)) {
		schedule_idle_display_of_pending_files (view);
	}

	g_object_unref (G_OBJECT (view));

	return FALSE;
}

static void
schedule_idle_display_of_pending_files (FMDirectoryView *view)
{
	/* No need to schedule an idle if there's already one pending. */
	if (view->details->display_pending_idle_id != 0) {
		return;
	}

	/* An idle takes precedence over a timeout. */
	unschedule_timeout_display_of_pending_files (view);

	view->details->display_pending_idle_id =
		gtk_idle_add_priority (G_PRIORITY_LOW, display_pending_idle_callback, view);
}

static void
schedule_timeout_display_of_pending_files (FMDirectoryView *view)
{
	/* No need to schedule a timeout if there's already one pending. */
	if (view->details->display_pending_timeout_id != 0) {
		return;
	}

	/* An idle takes precedence over a timeout. */
	if (view->details->display_pending_idle_id != 0) {
		return;
	}

	view->details->display_pending_timeout_id =
		gtk_timeout_add (DISPLAY_TIMEOUT_INTERVAL_MSECS,
				 display_pending_timeout_callback, view);
}

static void
unschedule_idle_display_of_pending_files (FMDirectoryView *view)
{
	/* Get rid of idle if it's active. */
	if (view->details->display_pending_idle_id != 0) {
		g_assert (view->details->display_pending_timeout_id == 0);
		gtk_idle_remove (view->details->display_pending_idle_id);
		view->details->display_pending_idle_id = 0;
	}
}

static void
unschedule_timeout_display_of_pending_files (FMDirectoryView *view)
{
	/* Get rid of timeout if it's active. */
	if (view->details->display_pending_timeout_id != 0) {
		g_assert (view->details->display_pending_idle_id == 0);
		gtk_timeout_remove (view->details->display_pending_timeout_id);
		view->details->display_pending_timeout_id = 0;
	}
}

static void
unschedule_display_of_pending_files (FMDirectoryView *view)
{
	unschedule_idle_display_of_pending_files (view);
	unschedule_timeout_display_of_pending_files (view);
}

static void
queue_pending_files (FMDirectoryView *view,
		     GList *files,
		     GList **pending_list)
{
	if (files == NULL) {
		return;
	}

	*pending_list = g_list_concat (*pending_list,
				       nautilus_file_list_copy (files));

	if (view->details->loading)
		schedule_timeout_display_of_pending_files (view);
	else
		schedule_idle_display_of_pending_files (view);
}

static void
files_added_callback (NautilusDirectory *directory,
		      GList *files,
		      gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);
	queue_pending_files (view, files, &view->details->new_added_files);
}

static void
files_changed_callback (NautilusDirectory *directory,
			GList *files,
			gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);
	queue_pending_files (view, files, &view->details->new_changed_files);
	
	/* A change in MIME type could affect the Open with menu, for
	 * one thing, so we need to update menus when files change.
	 */
	schedule_update_menus (view);
}

static void
done_loading_callback (NautilusDirectory *directory,
		       gpointer callback_data)
{
	FMDirectoryView *view;
	
	view = FM_DIRECTORY_VIEW (callback_data);
	
	process_new_files (view);
	if (g_hash_table_size (view->details->non_ready_files) == 0) {
		schedule_idle_display_of_pending_files (view);
	}
}

static void
load_error_callback (NautilusDirectory *directory,
		     GnomeVFSResult load_error_code,
		     gpointer callback_data)
{
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	/* FIXME: By doing a stop, we discard some pending files. Is
	 * that OK?
	 */
	fm_directory_view_stop (view);

	/* Emit a signal to tell subclasses that a load error has
	 * occurred, so they can handle it in the UI.
	 */
	g_signal_emit (view,
			 signals[LOAD_ERROR], 0, load_error_code);
}

static void
real_load_error (FMDirectoryView *view, GnomeVFSResult result)
{
	g_assert (result != GNOME_VFS_OK);

	/* Report only one error per failed directory load (from the UI
	 * point of view, not from the NautilusDirectory point of view).
	 * Otherwise you can get multiple identical errors caused by 
	 * unrelated code that just happens to try to iterate this
	 * directory.
	 */
	if (!view->details->reported_load_error) {
		fm_report_error_loading_directory 
			(fm_directory_view_get_directory_as_file (view), 
			 result,
			 fm_directory_view_get_containing_window (view));
	}
	view->details->reported_load_error = TRUE;
}

/**
 * fm_directory_queue_notice_file_change
 * 
 * Called by a subclass to put a file into the queue of files to update.
 * This is only necessary when the subclass is monitoring files other than
 * the ones in the directory for this location.
 */
void
fm_directory_view_queue_file_change (FMDirectoryView *view, NautilusFile *file)
{
	GList singleton_list;

	singleton_list.data = file;
	singleton_list.next = NULL;
	singleton_list.prev = NULL;
	queue_pending_files (view, &singleton_list, &view->details->new_changed_files);
}

/**
 * fm_directory_view_clear:
 *
 * Emit the signal to clear the contents of the view. Subclasses must
 * override the signal handler for this signal. This is normally called
 * only by FMDirectoryView.
 * @view: FMDirectoryView to empty.
 * 
 **/
void
fm_directory_view_clear (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	g_signal_emit (view, signals[CLEAR], 0);
}

/**
 * fm_directory_view_begin_loading:
 *
 * Emit the signal to prepare for loading the contents of a new location. 
 * Subclasses might want to override the signal handler for this signal. 
 * This is normally called only by FMDirectoryView.
 * @view: FMDirectoryView that is switching to view a new location.
 * 
 **/
void
fm_directory_view_begin_loading (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	g_signal_emit (view, signals[BEGIN_LOADING], 0);
}

/**
 * fm_directory_view_end_loading:
 *
 * Emit the signal after loading the contents of a new location. 
 * Subclasses might want to override the signal handler for this signal. 
 * This is normally called only by FMDirectoryView.
 * @view: FMDirectoryView that is switching to view a new location.
 * 
 **/
void
fm_directory_view_end_loading (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	g_signal_emit (view, signals[END_LOADING], 0);
}

/**
 * fm_directory_view_bump_zoom_level:
 *
 * bump the current zoom level by invoking the relevant subclass through the slot
 * 
 **/
void
fm_directory_view_bump_zoom_level (FMDirectoryView *view, int zoom_increment)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	if (!fm_directory_view_supports_zooming (view)) {
		return;
	}

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 bump_zoom_level, (view, zoom_increment));
}

/**
 * fm_directory_view_zoom_to_level:
 *
 * Set the current zoom level by invoking the relevant subclass through the slot
 * 
 **/
void
fm_directory_view_zoom_to_level (FMDirectoryView *view, int zoom_level)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	if (!fm_directory_view_supports_zooming (view)) {
		return;
	}

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 zoom_to_level, (view, zoom_level));
}


void
fm_directory_view_set_zoom_level (FMDirectoryView *view, int zoom_level)
{
	float new_zoom_level;

	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	if (!fm_directory_view_supports_zooming (view)) {
		return;
	}

	new_zoom_level = (float) nautilus_get_icon_size_for_zoom_level (zoom_level)
		/ NAUTILUS_ICON_SIZE_STANDARD;

	bonobo_zoomable_report_zoom_level_changed (
		view->details->zoomable, new_zoom_level, NULL);
}

/**
 * fm_directory_view_restore_default_zoom_level:
 *
 * restore to the default zoom level by invoking the relevant subclass through the slot
 * 
 **/
void
fm_directory_view_restore_default_zoom_level (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	if (!fm_directory_view_supports_zooming (view)) {
		return;
	}

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 restore_default_zoom_level, (view));
}

/**
 * fm_directory_view_can_zoom_in:
 *
 * Determine whether the view can be zoomed any closer.
 * @view: The zoomable FMDirectoryView.
 * 
 * Return value: TRUE if @view can be zoomed any closer, FALSE otherwise.
 * 
 **/
gboolean
fm_directory_view_can_zoom_in (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	if (!fm_directory_view_supports_zooming (view)) {
		return FALSE;
	}

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 can_zoom_in, (view));
}

/**
 * fm_directory_view_can_rename_file
 *
 * Determine whether a file can be renamed.
 * @file: A NautilusFile
 * 
 * Return value: TRUE if @file can be renamed, FALSE otherwise.
 * 
 **/
static gboolean
fm_directory_view_can_rename_file (FMDirectoryView *view, NautilusFile *file)
{
	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 can_rename_file, (view, file));
}

/**
 * fm_directory_view_can_zoom_out:
 *
 * Determine whether the view can be zoomed any further away.
 * @view: The zoomable FMDirectoryView.
 * 
 * Return value: TRUE if @view can be zoomed any further away, FALSE otherwise.
 * 
 **/
gboolean
fm_directory_view_can_zoom_out (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	if (!fm_directory_view_supports_zooming (view)) {
		return FALSE;
	}

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 can_zoom_out, (view));
}

GtkWidget *
fm_directory_view_get_background_widget (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 get_background_widget, (view));
}

EelBackground *
fm_directory_view_get_background (FMDirectoryView *view)
{
	return eel_get_widget_background (fm_directory_view_get_background_widget (view));
}

/**
 * fm_directory_view_get_selection:
 *
 * Get a list of NautilusFile pointers that represents the
 * currently-selected items in this view. Subclasses must override
 * the signal handler for the 'get_selection' signal. Callers are
 * responsible for g_free-ing the list (but not its data).
 * @view: FMDirectoryView whose selected items are of interest.
 * 
 * Return value: GList of NautilusFile pointers representing the selection.
 * 
 **/
GList *
fm_directory_view_get_selection (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 get_selection, (view));
}

/**
 * fm_directory_view_get_bonobo_ui_container:
 *
 * Get the BonoboUIContainer for this FMDirectoryView.
 * This is normally called only by subclasses in order to
 * install and modify bonobo menus and such.
 * @view: FMDirectoryView of interest.
 * 
 * Return value: BonoboUIContainer for this view.
 * 
 **/
Bonobo_UIContainer
fm_directory_view_get_bonobo_ui_container (FMDirectoryView *view)
{
        g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);
        
        return bonobo_control_get_remote_ui_container
		(fm_directory_view_get_bonobo_control (view), NULL);
}

/**
 * fm_directory_view_get_bonobo_control:
 *
 * Get the BonoboControl for this FMDirectoryView.
 * This is normally called only by subclasses in order to
 * help editables interact with the clipboard ui component
 * @view: FMDirectoryView of interest.
 * 
 * Return value: BonoboUIContainer for this view.
 * 
 **/
BonoboControl *
fm_directory_view_get_bonobo_control (FMDirectoryView *view)
{
	NautilusView *nautilus_view;

	nautilus_view = fm_directory_view_get_nautilus_view (view);
	return nautilus_view_get_bonobo_control (nautilus_view);
}

/**
 * fm_directory_view_get_nautilus_view:
 *
 * Get the NautilusView for this FMDirectoryView.
 * This is normally called only by the embedding framework.
 * @view: FMDirectoryView of interest.
 * 
 * Return value: NautilusView for this view.
 * 
 **/
NautilusView *
fm_directory_view_get_nautilus_view (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);

	return view->details->nautilus_view;
}

/**
 * fm_directory_view_get_model:
 *
 * Get the model for this FMDirectoryView.
 * @view: FMDirectoryView of interest.
 * 
 * Return value: NautilusDirectory for this view.
 * 
 **/
NautilusDirectory *
fm_directory_view_get_model (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);

	return view->details->model;
}

static void
prepend_uri_one (gpointer data, gpointer callback_data)
{
	NautilusFile *file;
	GList **result;
	
	g_assert (NAUTILUS_IS_FILE (data));
	g_assert (callback_data != NULL);

	result = (GList **) callback_data;
	file = (NautilusFile *) data;
	*result = g_list_prepend (*result, nautilus_file_get_uri (file));
}

static void
offset_drop_points (GArray *relative_item_points,
		    int x_offset, int y_offset)
{
	guint index;

	if (relative_item_points == NULL) {
		return;
	}

	for (index = 0; index < relative_item_points->len; index++) {
		g_array_index (relative_item_points, GdkPoint, index).x += x_offset;
		g_array_index (relative_item_points, GdkPoint, index).y += y_offset;
	}
}

static void
fm_directory_view_create_links_for_files (FMDirectoryView *view, GList *files,
					  GArray *relative_item_points)
{
	GList *uris;
	CopyMoveDoneData *copy_move_done_data;
	g_assert (relative_item_points->len == 0
		  || g_list_length (files) == relative_item_points->len);
	
        g_assert (FM_IS_DIRECTORY_VIEW (view));
        g_assert (files != NULL);

	/* create a list of URIs */
	uris = NULL;
	g_list_foreach (files, prepend_uri_one, &uris);
	uris = g_list_reverse (uris);

        g_assert (g_list_length (uris) == g_list_length (files));

	/* offset the drop locations a bit so that we don't pile
	 * up the icons on top of each other
	 */
	offset_drop_points (relative_item_points,
			    DUPLICATE_HORIZONTAL_ICON_OFFSET,
			    DUPLICATE_VERTICAL_ICON_OFFSET);

        copy_move_done_data = pre_copy_move (view);
	nautilus_file_operations_copy_move (uris, relative_item_points, NULL, GDK_ACTION_LINK, 
		GTK_WIDGET (view), copy_move_done_callback, copy_move_done_data);
	eel_g_list_free_deep (uris);
}

static void
fm_directory_view_duplicate_selection (FMDirectoryView *view, GList *files,
				       GArray *relative_item_points)
{
	GList *uris;
	CopyMoveDoneData *copy_move_done_data;

        g_assert (FM_IS_DIRECTORY_VIEW (view));
        g_assert (files != NULL);
	g_assert (g_list_length (files) == relative_item_points->len
		|| relative_item_points->len == 0);

	/* create a list of URIs */
	uris = NULL;
	g_list_foreach (files, prepend_uri_one, &uris);
	uris = g_list_reverse (uris);

        g_assert (g_list_length (uris) == g_list_length (files));
        
	/* offset the drop locations a bit so that we don't pile
	 * up the icons on top of each other
	 */
	offset_drop_points (relative_item_points,
			    DUPLICATE_HORIZONTAL_ICON_OFFSET,
			    DUPLICATE_VERTICAL_ICON_OFFSET);

        copy_move_done_data = pre_copy_move (view);
	nautilus_file_operations_copy_move (uris, relative_item_points, NULL, GDK_ACTION_COPY,
		GTK_WIDGET (view), copy_move_done_callback, copy_move_done_data);
	eel_g_list_free_deep (uris);
}

/* special_link_in_selection
 * 
 * Return TRUE if one of our special links is in the selection.
 * Special links include the following: 
 *	 NAUTILUS_LINK_TRASH, NAUTILUS_LINK_HOME, NAUTILUS_LINK_MOUNT
 */
 
static gboolean
special_link_in_selection (FMDirectoryView *view)
{
	gboolean saw_link;
	GList *selection, *node;
	NautilusFile *file;
	char *uri;

	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	saw_link = FALSE;

	selection = fm_directory_view_get_selection (FM_DIRECTORY_VIEW (view));

	for (node = selection; node != NULL; node = node->next) {
		file = NAUTILUS_FILE (node->data);

		uri = nautilus_file_get_uri (file);

		/* FIXME: This reads the link file every single time. */
		saw_link = nautilus_file_is_local (file)
			&& nautilus_file_is_nautilus_link (file)
			&& nautilus_link_local_is_special_link (uri);
		
		g_free (uri);
		
		if (saw_link) {
			break;
		}
	}
	
	nautilus_file_list_free (selection);
	
	return saw_link;
}

static gboolean
can_move_uri_to_trash (FMDirectoryView *view, const char *file_uri_string)
{
	/* Return TRUE if we can get a trash directory on the same volume as this file. */
	GnomeVFSURI *file_uri;
	GnomeVFSURI *directory_uri;
	GnomeVFSURI *trash_dir_uri;
	gboolean result;

	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);
	g_return_val_if_fail (file_uri_string != NULL, FALSE);

	file_uri = gnome_vfs_uri_new (file_uri_string);

	g_return_val_if_fail (file_uri != NULL, FALSE);

	/* FIXME: Why can't we just pass file_uri to gnome_vfs_find_directory? */
	directory_uri = gnome_vfs_uri_get_parent (file_uri);
	gnome_vfs_uri_unref (file_uri);

	if (directory_uri == NULL) {
		return FALSE;
	}

	/*
	 * Create a new trash if needed but don't go looking for an old Trash.
	 * Passing 0 permissions as gnome-vfs would override the permissions 
	 * passed with 700 while creating .Trash directory
	 */
	result = gnome_vfs_find_directory (directory_uri, GNOME_VFS_DIRECTORY_KIND_TRASH,
					   &trash_dir_uri, TRUE, FALSE, 0) == GNOME_VFS_OK;
	if (result) {
		gnome_vfs_uri_unref (trash_dir_uri);
	}
	gnome_vfs_uri_unref (directory_uri);

	return result;
}

static char *
file_name_from_uri (const char *uri)
{
	NautilusFile *file;
	char *file_name;
	
	file = nautilus_file_get (uri);
	file_name = nautilus_file_get_display_name (file);
	nautilus_file_unref (file);

	return file_name;	
}

static gboolean
fm_directory_view_confirm_deletion (FMDirectoryView *view, GList *uris, gboolean all)
{
	GtkDialog *dialog;
	char *prompt;
	int uri_count;
	char *file_name;
	int response;

	g_assert (FM_IS_DIRECTORY_VIEW (view));

	uri_count = g_list_length (uris);
	g_assert (uri_count > 0);
	
	if (uri_count == 1) {
		file_name = file_name_from_uri ((char *) uris->data);

		prompt = g_strdup_printf (_("\"%s\" cannot be moved to the Trash. Do "
					    "you want to delete it immediately?"), file_name);
		g_free (file_name);
	} else {
		if (all) {
			prompt = g_strdup_printf (_("The %d selected items cannot be moved "
						    "to the Trash. Do you want to delete them "
						    "immediately?"), uri_count);
		} else {
			prompt = g_strdup_printf (_("%d of the selected items cannot be moved "
						    "to the Trash. Do you want to delete those "
						    "%d items immediately?"), uri_count, uri_count);
		}
	}

	dialog = eel_show_yes_no_dialog
		(prompt,
		 _("Delete Immediately?"), GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
		 fm_directory_view_get_containing_window (view));
	
	g_free (prompt);

	response = gtk_dialog_run (dialog);
	gtk_object_destroy (GTK_OBJECT (dialog));

	return response == GTK_RESPONSE_YES;
}

static gboolean
confirm_delete_from_trash (FMDirectoryView *view, GList *uris)
{
	GtkDialog *dialog;
	char *prompt;
	char *file_name;
	int uri_count;
	int response;

	g_assert (FM_IS_DIRECTORY_VIEW (view));

	/* Just Say Yes if the preference says not to confirm. */
	if (!confirm_trash_auto_value) {
		return TRUE;
	}

	uri_count = g_list_length (uris);
	g_assert (uri_count > 0);

	if (uri_count == 1) {
		file_name = file_name_from_uri ((char *) uris->data);
		prompt = g_strdup_printf (_("Are you sure you want to permanently delete \"%s\" "
					    "from the Trash?"), file_name);
		g_free (file_name);
	} else {
		prompt = g_strdup_printf (_("Are you sure you want to permanently delete "
		  			    "the %d selected items from the Trash?"), uri_count);
	}

	dialog = eel_show_yes_no_dialog (
		prompt,
		_("Delete From Trash?"), GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
		fm_directory_view_get_containing_window (view));

	g_free (prompt);
	
	response = gtk_dialog_run (dialog);
	gtk_object_destroy (GTK_OBJECT (dialog));

	return response == GTK_RESPONSE_YES;
}

static void
trash_or_delete_files_common (FMDirectoryView *view,
			      const GList *file_uris,
			      GArray *relative_item_points,
			      gboolean delete_if_all_already_in_trash)
{
	const GList *file_node;
	char *file_uri;
	GList *moveable_uris;
	GList *unmoveable_uris;
	GList *in_trash_uris;

	g_assert (FM_IS_DIRECTORY_VIEW (view));

	/* Collect three lists: (1) items that can be moved to trash,
	 * (2) items that can only be deleted in place, and (3) items that
	 * are already in trash. 
	 * 
	 * Always move (1) to trash if non-empty.
	 * Delete (3) only if (1) and (2) are non-empty, otherwise ignore (3).
	 * Ask before deleting (2) if non-empty.
	 * Ask before deleting (3) if non-empty.
	 */

	moveable_uris = NULL;
	unmoveable_uris = NULL;
	in_trash_uris = NULL;
	
	for (file_node = file_uris; file_node != NULL; file_node = file_node->next) {
		file_uri = (char *) file_node->data;
		
		if (delete_if_all_already_in_trash && eel_uri_is_in_trash (file_uri)) {
			in_trash_uris = g_list_prepend (in_trash_uris, g_strdup (file_uri));
		} else if (can_move_uri_to_trash (view, file_uri)) {
			moveable_uris = g_list_prepend (moveable_uris, g_strdup (file_uri));
		} else {
			unmoveable_uris = g_list_prepend (unmoveable_uris, g_strdup (file_uri));
		}
	}

	if (in_trash_uris != NULL && moveable_uris == NULL && unmoveable_uris == NULL) {
		if (confirm_delete_from_trash (view, in_trash_uris)) {
			nautilus_file_operations_delete (in_trash_uris, GTK_WIDGET (view));
		}		
	} else {
		if (moveable_uris != NULL) {
			nautilus_file_operations_copy_move (moveable_uris, relative_item_points, 
							    EEL_TRASH_URI, GDK_ACTION_MOVE, GTK_WIDGET (view),
							    copy_move_done_callback, pre_copy_move (view));
		}
		if (unmoveable_uris != NULL) {
			if (fm_directory_view_confirm_deletion (view, 
								unmoveable_uris,
								moveable_uris == NULL)) {
				nautilus_file_operations_delete (unmoveable_uris, GTK_WIDGET (view));
			}
		}
	}
	
	eel_g_list_free_deep (in_trash_uris);
	eel_g_list_free_deep (moveable_uris);
	eel_g_list_free_deep (unmoveable_uris);
}

static void
trash_or_delete_files (FMDirectoryView *view,
		       const GList *files)
{
	GList *file_uris;
	const GList *node;
	
	file_uris = NULL;
	for (node = files; node != NULL; node = node->next) {
		file_uris = g_list_prepend (file_uris,
					    nautilus_file_get_uri ((NautilusFile *) node->data));
	}
	
	file_uris = g_list_reverse (file_uris);
	trash_or_delete_files_common (view, file_uris, NULL, TRUE);
	eel_g_list_free_deep (file_uris);
}

static gboolean
can_rename_file (FMDirectoryView *view, NautilusFile *file)
{
	return nautilus_file_can_rename (file);
}

static void
start_renaming_file (FMDirectoryView *view, NautilusFile *file)
{
	if (file !=  NULL) {
		fm_directory_view_select_file (view, file);
	}
}

static void
reveal_newly_added_folder (FMDirectoryView *view, NautilusFile *new_file, const char *target_uri)
{
	if (nautilus_file_matches_uri (new_file, target_uri)) {
		g_signal_handlers_disconnect_by_func (view,
						      G_CALLBACK (reveal_newly_added_folder),
						      (void *) target_uri);
		/* no need to select because start_renaming_file selects
		 * fm_directory_view_select_file (view, new_file);
		 */
		EEL_CALL_METHOD (FM_DIRECTORY_VIEW_CLASS, view, start_renaming_file, (view, new_file));
		fm_directory_view_reveal_selection (view);
	}
}

static void
new_folder_done (const char *new_folder_uri, gpointer data)
{
	FMDirectoryView *directory_view;
	NautilusFile *file;
	char *screen_string;
	GdkScreen *screen;

	directory_view = (FMDirectoryView *) data;
	g_assert (FM_IS_DIRECTORY_VIEW (directory_view));

	screen = gtk_widget_get_screen (GTK_WIDGET (directory_view));
	screen_string = g_strdup_printf ("%d", gdk_screen_get_number (screen));

	file = nautilus_file_get (new_folder_uri);
	nautilus_file_set_metadata
		(file, NAUTILUS_METADATA_KEY_SCREEN,
		 NULL,
		 screen_string);
	g_free (screen_string);

	/* We need to run after the default handler adds the folder we want to
	 * operate on. The ADD_FILE signal is registered as G_SIGNAL_RUN_LAST, so we
	 * must use connect_after.
	 */
	g_signal_connect_data (directory_view,
			       "add_file",
			       G_CALLBACK (reveal_newly_added_folder),
			       g_strdup (new_folder_uri),
			       (GClosureNotify)g_free,
			       G_CONNECT_AFTER);
}

void
fm_directory_view_new_folder (FMDirectoryView *directory_view)
{
	char *parent_uri;

	parent_uri = fm_directory_view_get_uri (directory_view);
	nautilus_file_operations_new_folder (GTK_WIDGET (directory_view),
					     parent_uri,
					     new_folder_done, directory_view);

	g_free (parent_uri);
}

/* handle the open command */

static void
open_one_in_new_window (gpointer data, gpointer callback_data)
{
	g_assert (NAUTILUS_IS_FILE (data));
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	fm_directory_view_activate_file (FM_DIRECTORY_VIEW (callback_data),
					 NAUTILUS_FILE (data),
					 FORCE_NEW_WINDOW);
}

static void
open_one_properties_window (gpointer data, gpointer callback_data)
{
	g_assert (NAUTILUS_IS_FILE (data));
	g_assert (FM_IS_DIRECTORY_VIEW (callback_data));

	fm_properties_window_present (data, callback_data);
}

NautilusFile *
fm_directory_view_get_directory_as_file (FMDirectoryView *view)
{
	g_assert (FM_IS_DIRECTORY_VIEW (view));

	return view->details->directory_as_file; 
}

static void
bonobo_launch_application_callback (BonoboUIComponent *component, gpointer callback_data, const char *path)
{
	ApplicationLaunchParameters *launch_parameters;
	
	launch_parameters = (ApplicationLaunchParameters *) callback_data;
	fm_directory_view_launch_application 
		(launch_parameters->application,
		 launch_parameters->file,
		 launch_parameters->directory_view);
}				    

static void
bonobo_open_location_with_viewer_callback (BonoboUIComponent *component, gpointer callback_data, const char *path)
{
	ViewerLaunchParameters *launch_parameters;

	launch_parameters = (ViewerLaunchParameters *) callback_data;

	switch_location_and_view (launch_parameters->identifier,
				  launch_parameters->uri,
				  launch_parameters->directory_view);
}

static void
add_numbered_menu_item (BonoboUIComponent *ui,
			const char *parent_path,
			const char *label,
			const char *tip,
			int index,
			GdkPixbuf *pixbuf,
			gpointer callback,
			gpointer callback_data,
			GDestroyNotify destroy_notify)
{
	char *escaped_parent_path, *escaped_label, *verb_name, *item_path;
	
	escaped_parent_path = eel_str_double_underscores (parent_path);

	escaped_label = eel_str_double_underscores (label);
	nautilus_bonobo_add_numbered_menu_item 
		(ui, 
		 escaped_parent_path,
		 index,
		 escaped_label, 
		 pixbuf);
	g_free (escaped_label);

	item_path = nautilus_bonobo_get_numbered_menu_item_path
		(ui, escaped_parent_path, index);
	nautilus_bonobo_set_tip (ui, item_path, tip);
	g_free (item_path);

	verb_name = nautilus_bonobo_get_numbered_menu_item_command 
		(ui, escaped_parent_path, index);	
	bonobo_ui_component_add_verb_full (ui, verb_name,
					   g_cclosure_new (callback, callback_data,
							   (GClosureNotify) destroy_notify));	   
	g_free (verb_name);

	g_free (escaped_parent_path);
}

static void
add_submenu (BonoboUIComponent *ui,
	     const char *parent_path,
	     const char *label,
	     GdkPixbuf *pixbuf)
{
	char *escaped_parent_path, *escaped_label;

	escaped_parent_path = eel_str_double_underscores (parent_path);
	escaped_label = eel_str_double_underscores (label);
	nautilus_bonobo_add_submenu (ui, escaped_parent_path, escaped_label, pixbuf);
	g_free (escaped_label);
	g_free (escaped_parent_path);
}

static void
add_application_to_bonobo_menu (FMDirectoryView *directory_view,
				GnomeVFSMimeApplication *application, 
				NautilusFile *file,
				int index)
{
	ApplicationLaunchParameters *launch_parameters;
	char *tip;

	launch_parameters = application_launch_parameters_new 
		(application, file, directory_view);
	tip = g_strdup_printf (_("Use \"%s\" to open the selected item"), application->name);

	add_numbered_menu_item (directory_view->details->ui, 
				FM_DIRECTORY_VIEW_MENU_PATH_APPLICATIONS_PLACEHOLDER,
				application->name,
				tip,
				index,
				NULL,
				bonobo_launch_application_callback,
				launch_parameters,
				(GDestroyNotify) application_launch_parameters_free);
	/* Use same launch parameters and no DestroyNotify for popup item, which has same
	 * lifetime as the item in the File menu in the menu bar.
	 */
	add_numbered_menu_item (directory_view->details->ui, 
				FM_DIRECTORY_VIEW_POPUP_PATH_APPLICATIONS_PLACEHOLDER,
				application->name,
				tip,
				index,
				NULL,
				bonobo_launch_application_callback,
				launch_parameters,
				NULL);
	g_free (tip);
}

static void
add_component_to_bonobo_menu (FMDirectoryView *directory_view,
			      Bonobo_ServerInfo *content_view, 
			      const char *uri,
			      int index)
{
	NautilusViewIdentifier *identifier;
	ViewerLaunchParameters *launch_parameters;
	char *tip;
	char *label;
	
	identifier = nautilus_view_identifier_new_from_content_view (content_view);
	launch_parameters = viewer_launch_parameters_new (identifier, uri, directory_view);
	nautilus_view_identifier_free (identifier);

	label = g_strdup (launch_parameters->identifier->viewer_label);
	tip = g_strdup_printf (_("Use \"%s\" to open the selected item"), label);

	add_numbered_menu_item (directory_view->details->ui, 
				FM_DIRECTORY_VIEW_MENU_PATH_VIEWERS_PLACEHOLDER,
				label,
				tip,
				index,
				NULL,
				bonobo_open_location_with_viewer_callback,
				launch_parameters,
				(GDestroyNotify) viewer_launch_parameters_free);
	/* Use same launch parameters and no DestroyNotify for popup item, which has same
	 * lifetime as the item in the File menu in the menu bar.
	 */
 	add_numbered_menu_item (directory_view->details->ui, 
				FM_DIRECTORY_VIEW_POPUP_PATH_VIEWERS_PLACEHOLDER,
				label,
				tip,
				index,
				NULL,
				bonobo_open_location_with_viewer_callback,
				launch_parameters,
				NULL);
	g_free (tip);
	g_free (label);
}


static void
reset_bonobo_open_with_menu (FMDirectoryView *view, GList *selection)
{
	GList *applications, *components, *node;
	NautilusFile *file;
	gboolean sensitive;
	gboolean any_applications;
	gboolean any_viewers;
	char *uri;
	int index;
	
	/* Clear any previous inserted items in the applications and viewers placeholders */
	nautilus_bonobo_remove_menu_items_and_commands
		(view->details->ui, FM_DIRECTORY_VIEW_MENU_PATH_APPLICATIONS_PLACEHOLDER);
	nautilus_bonobo_remove_menu_items_and_commands 
		(view->details->ui, FM_DIRECTORY_VIEW_MENU_PATH_VIEWERS_PLACEHOLDER);
	nautilus_bonobo_remove_menu_items_and_commands 
		(view->details->ui, FM_DIRECTORY_VIEW_POPUP_PATH_APPLICATIONS_PLACEHOLDER);
	nautilus_bonobo_remove_menu_items_and_commands 
		(view->details->ui, FM_DIRECTORY_VIEW_POPUP_PATH_VIEWERS_PLACEHOLDER);
	
	/* This menu is only displayed when there's one selected item. */
	if (!eel_g_list_exactly_one_item (selection)) {
		sensitive = FALSE;
		monitor_file_for_open_with (view, NULL);
	} else {
		sensitive = TRUE;
		any_applications = FALSE;
		any_viewers = FALSE;
		
		file = NAUTILUS_FILE (selection->data);
		
		monitor_file_for_open_with (view, file);

		uri = nautilus_file_get_uri (file);
		
		applications = nautilus_mime_get_short_list_applications_for_file (NAUTILUS_FILE (selection->data));
		for (node = applications, index = 0; node != NULL; node = node->next, index++) {
			any_applications = TRUE;
			add_application_to_bonobo_menu (view, node->data, file, index);
		}
		gnome_vfs_mime_application_list_free (applications); 
		
		components = nautilus_mime_get_short_list_components_for_file (NAUTILUS_FILE (selection->data));
		for (node = components, index = 0; node != NULL; node = node->next, index++) {
			any_viewers = TRUE;
			add_component_to_bonobo_menu (view, node->data, uri, index);
		}
		gnome_vfs_mime_component_list_free (components); 

		nautilus_bonobo_set_label 
			(view->details->ui,
			 FM_DIRECTORY_VIEW_COMMAND_OTHER_APPLICATION,
			 any_applications ? _("Other _Application...") : _("An _Application..."));

		nautilus_bonobo_set_label 
			(view->details->ui,
			 FM_DIRECTORY_VIEW_COMMAND_OTHER_VIEWER,
			 any_applications ? _("Other _Viewer...") : _("A _Viewer..."));

		g_free (uri);
	}

	/* It's OK to set the sensitivity of the menu items (rather than the verbs)
	 * here because these are submenu titles, not items with verbs.
	 */
	nautilus_bonobo_set_sensitive (view->details->ui,
				       FM_DIRECTORY_VIEW_MENU_PATH_OPEN_WITH,
				       sensitive);
	nautilus_bonobo_set_sensitive (view->details->ui,
				       FM_DIRECTORY_VIEW_POPUP_PATH_OPEN_WITH,
				       sensitive);
}

static BonoboMimeActionData *
bonobo_mime_action_data_new (const char *id, const char *verb, GList *files)
{
	BonoboMimeActionData *data;
	CORBA_sequence_CORBA_string *uri_list;
	int i;

	data = g_new (BonoboMimeActionData, 1);
	data->id = g_strdup (id);
	data->verb = g_strdup (verb);


	/* convert the GList of files into a CORBA sequence */

	uri_list = CORBA_sequence_CORBA_string__alloc ();
	uri_list->_maximum = g_list_length (files);
	uri_list->_length = uri_list->_maximum;
	uri_list->_buffer = CORBA_sequence_CORBA_string_allocbuf (uri_list->_length);

	for (i=0; files; files = files->next, i++)
	{
		NautilusFile *file;
		char *uri;

		file = files->data;
		uri = nautilus_file_get_uri (file);

		uri_list->_buffer[i] = CORBA_string_dup ((char*)uri);

		g_free (uri);
	}

	CORBA_sequence_set_release (uri_list, CORBA_TRUE);
	data->uri_list = uri_list;


	return data;
}

static void
bonobo_mime_action_data_free (BonoboMimeActionData *data)
{
	g_free (data->id);
	g_free (data->verb);
	g_free (data);
}


static void
bonobo_mime_action_activate_callback (CORBA_Object obj,
				      const char *error_reason,
				      gpointer user_data)
{
	Bonobo_Listener listener;
	CORBA_Environment ev;
	BonoboMimeActionData *data;
	CORBA_any any;

	data = user_data;

	if (obj == CORBA_OBJECT_NIL) {
		GtkWidget *dialog;

		/* FIXME: make an error message that is not so lame */
		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Could not complete specified action:  %s"), error_reason);
		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (dialog);
		return;
	}

	CORBA_exception_init (&ev);

	listener = Bonobo_Unknown_queryInterface (obj,
						  "IDL:Bonobo/Listener:1.0",
						  &ev);

	if (!BONOBO_EX (&ev)) {
		any._type = TC_CORBA_sequence_CORBA_string;
		any._value = data->uri_list;
		Bonobo_Listener_event (listener, data->verb, &any, &ev);
		bonobo_object_release_unref (listener, &ev);
	} else {
		GtkWidget *dialog;

		/* FIXME: make an error message that is not so lame */
		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Could not complete specified action."));
		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (dialog);
	}

}


static void
bonobo_mime_action_callback (BonoboUIComponent *component,
			     gpointer callback_data, const char *path)
{
	BonoboMimeActionData *data;

	data = callback_data;

	bonobo_activation_activate_from_id_async (data->id, 0,
				bonobo_mime_action_activate_callback,
				data, NULL);
	
}

static void
bonobo_mime_action_menu_data_destroy_callback (gpointer data, GClosure *closure)
{
	bonobo_mime_action_data_free ((BonoboMimeActionData *)data);
}

static gboolean
can_handle_multiple_files (Bonobo_ServerInfo *info)
{
	Bonobo_ActivationProperty *prop;

	prop = bonobo_server_info_prop_find (info, "nautilus:can_handle_multiple_files");
	return prop->v._u.value_boolean;
}

static void
add_bonobo_menu_ui_and_verbs (FMDirectoryView *view, GList *files,
			      Bonobo_ServerInfo *info, GList *verb_names)
{
	GList *l;
	GString *ui_xml;
	const GList *langs;
	GSList *langs_cpy;
	char *ui_xml_str;

	g_return_if_fail (verb_names != NULL);

	langs = gnome_i18n_get_language_list ("LANG");
	langs_cpy = NULL;
	/* copy it to a singly linked list since bonobo wants that...sigh */
	for (; langs; langs = langs->next) {
		langs_cpy = g_slist_append (langs_cpy, langs->data);
	}

	ui_xml = g_string_new ("<Root><commands>");

	/* build the commands */
	for (l = verb_names; l; l = l->next) {
		const char *label;
		char *prop_name;
		char *verb;

		verb = l->data;

		prop_name = g_strdup_printf ("nautilusverb:%s", verb);
		label = bonobo_server_info_prop_lookup (info, prop_name,
							langs_cpy);
		g_free (prop_name);

		g_string_append_printf (ui_xml, "<cmd name=\"%s\" label=\"%s\"/>", verb, label);
	}

	g_slist_free (langs_cpy);

	ui_xml = g_string_append (ui_xml, "</commands><popups><popup name=\"selection\"><placeholder name=\"Mime Actions\"><separator/>");

	/* build the UI */
	for (l = verb_names; l; l = l->next) {
		char *verb = l->data;

		g_string_append_printf (ui_xml, "<menuitem name=\"%s\" verb=\"%s\"/>", verb, verb);
	}
	
	ui_xml = g_string_append (ui_xml, "</placeholder></popup></popups></Root>");
	ui_xml_str = g_string_free (ui_xml, FALSE);

	bonobo_ui_component_set (view->details->ui, "/",
				 ui_xml_str, NULL);
	g_free (ui_xml_str);

	/* if it doesn't handle multiple files, disable the menu items */
	if ((g_list_length (files) > 1) &&
	    (can_handle_multiple_files (info) == FALSE)) {

		for (; verb_names; verb_names = verb_names->next) {
			char *path = g_strdup_printf ("/commands/%s",
						      (char *)verb_names->data);
			bonobo_ui_component_set_prop (view->details->ui,
						      path,
						      "sensitive",
						      "0", NULL);
			g_free (path);
		}

		/* no reason to continue */
		return;
	}

	/* add the verbs */
	for (l = verb_names; l; l = l->next) {
		const char *verb;
		BonoboMimeActionData *data;
		GClosure *closure;

		verb = l->data;
		
		data = bonobo_mime_action_data_new (info->iid,
						    verb, files);
		closure = g_cclosure_new
				(G_CALLBACK (bonobo_mime_action_callback),
				 data,
				 bonobo_mime_action_menu_data_destroy_callback);	
		bonobo_ui_component_add_verb_full
					(view->details->ui,
					 data->verb,
					 closure); 
	}

}

static GList *
get_bonobo_menu_verb_names (Bonobo_ServerInfo *info)
{
	GList *l;
	unsigned int i;
	int offset;

	offset = strlen ("nautilusverb:");

	l = NULL;
	for (i = 0; i < info->props._length; i++) {

		/* look for properties that start with "nautilusverb:".  The
		 * part following the colon is the verb name
		 */
		if (strstr (info->props._buffer[i].name, "nautilusverb:")) {
			l = g_list_prepend (l,
			      g_strdup (&info->props._buffer[i].name[offset]));	
		}
	}

	return l;
}

static gboolean
has_file_in_list (GList *list, NautilusFile *file)
{
	gboolean ret = FALSE;
	char *mime;
       
	mime = nautilus_file_get_mime_type (file);
	
	for (; list; list = list->next) {
		NautilusFile *tmp_file = list->data;
		char *tmp_mime = nautilus_file_get_mime_type (tmp_file);

		if (strcmp (tmp_mime, mime) == 0) {
			ret = TRUE;
			g_free (tmp_mime);
			break;
		}

		g_free (tmp_mime);
	}
	
	g_free (mime);
	return ret;
}

static GList *
get_unique_files (GList *selection)
{
	GList *result;

	result = NULL;
	for (; selection; selection = selection->next) {
		if (!has_file_in_list (result,
				       NAUTILUS_FILE (selection->data))) {
			result = g_list_prepend (result, selection->data);
		}
	}	

	return g_list_reverse (result);
}

static void
reset_bonobo_mime_actions_menu (FMDirectoryView *view, GList *selection)
{
	gboolean sensitive;
	GList *components, *l, *unique_selection;

	/* Clear any previous inserted items in the mime actions placeholder */
	nautilus_bonobo_remove_menu_items_and_commands
		(view->details->ui, FM_DIRECTORY_VIEW_POPUP_PATH_MIME_ACTIONS);

	sensitive = TRUE;
		
	/* only query for the unique files so we can reduce oaf traffic */
	unique_selection = get_unique_files (selection);

	components = nautilus_mime_get_popup_components_for_files (unique_selection);
	g_list_free (unique_selection);

	for (l = components; l; l = l->next) {
		Bonobo_ServerInfo *info;
		GList *verb_names;

		info = l->data;
		verb_names = get_bonobo_menu_verb_names (info);
		
		add_bonobo_menu_ui_and_verbs (view, selection, info,
					      verb_names);
		eel_g_list_free_deep (verb_names);
		
	}

	if (components != NULL) {
		gnome_vfs_mime_component_list_free (components);
	} else {
		sensitive = FALSE;
	}

	nautilus_bonobo_set_sensitive (view->details->ui,
				       FM_DIRECTORY_VIEW_POPUP_PATH_MIME_ACTIONS,
				       sensitive);
}

static char *
change_to_view_directory (FMDirectoryView *view)
{
	char *uri;
	char *path;
	char *old_path;

	old_path = g_get_current_dir ();

	uri = nautilus_directory_get_uri (view->details->model);
	path = gnome_vfs_get_local_path_from_uri (uri);

	/* FIXME: What to do about non-local directories? */
	if (path != NULL) {
		chdir (path);
	}

	g_free (uri);
	g_free (path);

	return old_path;
}

static char *
get_file_names_as_parameter_string (GList *selection)
{
	char *name, *quoted_name;
	char *result;
	GString *parameter_string;
	GList *node;

	parameter_string = g_string_new ("");
	for (node = selection; node != NULL; node = node->next) {
		name = nautilus_file_get_name (NAUTILUS_FILE (node->data));
		quoted_name = g_shell_quote (name);
		g_string_append (parameter_string, quoted_name);
		g_string_append (parameter_string, " ");
		g_free (name);
		g_free (quoted_name);
	}

	result = parameter_string->str;
	g_string_free (parameter_string, FALSE);

	return result;
}

static char *
get_file_paths_or_uris_as_newline_delimited_string (GList *selection, gboolean get_paths)
{
	char *path;
	char *uri;
	char *result;
	GString *expanding_string;
	GList *node;

	expanding_string = g_string_new ("");
	for (node = selection; node != NULL; node = node->next) {
		uri = nautilus_file_get_uri (NAUTILUS_FILE (node->data));

		if (get_paths) {
			path = gnome_vfs_get_local_path_from_uri (uri);
			g_string_append (expanding_string, path);
			g_free (path);
		} else {
			g_string_append (expanding_string, uri);
		}
		g_string_append (expanding_string, "\n");
		g_free (uri);
	}

	result = expanding_string->str;
	g_string_free (expanding_string, FALSE);

	return result;
}

static char *
get_file_paths_as_newline_delimited_string (GList *selection)
{
	return get_file_paths_or_uris_as_newline_delimited_string (selection, TRUE);
}

static char *
get_file_uris_as_newline_delimited_string (GList *selection)
{
	return get_file_paths_or_uris_as_newline_delimited_string (selection, FALSE);
}

/*
 * Set up some environment variables that scripts can use
 * to take advantage of the current Nautilus state.
 */
static void
set_script_environment_variables (FMDirectoryView *view, GList *selected_files)
{
	char *file_paths;
	char *uris;
	char *uri;
	char *geometry_string;

	if (nautilus_directory_is_local (view->details->model)) {
		file_paths = get_file_paths_as_newline_delimited_string (selected_files);
	} else {
		file_paths = g_strdup ("");
	}
	eel_setenv ("NAUTILUS_SCRIPT_SELECTED_FILE_PATHS", file_paths, TRUE);
	g_free (file_paths);

	uris = get_file_uris_as_newline_delimited_string (selected_files);
	eel_setenv ("NAUTILUS_SCRIPT_SELECTED_URIS", uris, TRUE);
	g_free (uris);

	uri = nautilus_directory_get_uri (view->details->model);
	eel_setenv ("NAUTILUS_SCRIPT_CURRENT_URI", uri, TRUE);
	g_free (uri);

	geometry_string = eel_gtk_window_get_geometry_string 
		(GTK_WINDOW (fm_directory_view_get_containing_window (view)));
	eel_setenv ("NAUTILUS_SCRIPT_WINDOW_GEOMETRY", geometry_string, TRUE);
	g_free (geometry_string);
}

/* Unset all the special script environment variables. */
static void
unset_script_environment_variables (void)
{
	eel_unsetenv ("NAUTILUS_SCRIPT_SELECTED_FILE_PATHS");
	eel_unsetenv ("NAUTILUS_SCRIPT_SELECTED_URIS");
	eel_unsetenv ("NAUTILUS_SCRIPT_CURRENT_URI");
	eel_unsetenv ("NAUTILUS_SCRIPT_WINDOW_GEOMETRY");
}

static void
run_script_callback (BonoboUIComponent *component, gpointer callback_data, const char *path)
{
	ScriptLaunchParameters *launch_parameters;
	GdkScreen *screen;
	GList *selected_files;
	char *file_uri;
	char *local_file_path;
	char *quoted_path;
	char *old_working_dir;
	char *parameters, *command, *name;
	
	launch_parameters = (ScriptLaunchParameters *) callback_data;

	file_uri = nautilus_file_get_uri (launch_parameters->file);
	local_file_path = gnome_vfs_get_local_path_from_uri (file_uri);
	g_assert (local_file_path != NULL);
	g_free (file_uri);

	quoted_path = g_shell_quote (local_file_path);
	g_free (local_file_path);

	old_working_dir = change_to_view_directory (launch_parameters->directory_view);

	selected_files = fm_directory_view_get_selection (launch_parameters->directory_view);
	set_script_environment_variables (launch_parameters->directory_view, selected_files);
	 
	if (nautilus_directory_is_local (launch_parameters->directory_view->details->model)) {
		parameters = get_file_names_as_parameter_string (selected_files);

		/* FIXME: must append command and parameters here, because nautilus_launch_application_from_command
		 * quotes all parameters as if they are a single parameter. Should add or change API in
		 * nautilus-program-choosing.c to support multiple parameters.
		 */
		command = g_strconcat (quoted_path, " ", parameters, NULL);
		g_free (parameters);
	} else {
		/* We pass no parameters in the remote case. It's up to scripts to be smart
		 * and check the environment variables. 
		 */
		command = g_strdup (quoted_path);
	}

	screen = gtk_widget_get_screen (GTK_WIDGET (launch_parameters->directory_view));

	name = nautilus_file_get_name (launch_parameters->file);
	/* FIXME: handle errors with dialog? Or leave up to each script? */
	nautilus_launch_application_from_command (screen, name, command, NULL, FALSE);
	g_free (name);
	g_free (command);

	nautilus_file_list_free (selected_files);
	unset_script_environment_variables ();
	chdir (old_working_dir);		
	g_free (old_working_dir);
	g_free (quoted_path);
}

static void
add_script_to_script_menus (FMDirectoryView *directory_view,
			    NautilusFile *file,
			    int index,
			    const char *menu_path,
			    const char *popup_path, 
			    const char *popup_bg_path)
{
	ScriptLaunchParameters *launch_parameters;
	char *tip;
	char *name;
	GdkPixbuf *pixbuf;

	name = nautilus_file_get_display_name (file);
	tip = g_strdup_printf (_("Run \"%s\" on any selected items"), name);

	launch_parameters = script_launch_parameters_new (file, directory_view);
	pixbuf = nautilus_icon_factory_get_pixbuf_for_file 
		(file, NULL, NAUTILUS_ICON_SIZE_FOR_MENUS);

	add_numbered_menu_item (directory_view->details->ui, 
				menu_path,
				name,
				tip,
				index,
				pixbuf,
				run_script_callback,
				launch_parameters,
				(GDestroyNotify) script_launch_parameters_free);

	/* Use same launch parameters and no DestroyNotify for popup item, which has same
	 * lifetime as the item in the File menu in the menu bar.
	 */
	add_numbered_menu_item (directory_view->details->ui,
				popup_path,
				name,
				tip,
				index,
				pixbuf,
				run_script_callback,
				launch_parameters,
				NULL);
	add_numbered_menu_item (directory_view->details->ui,
				popup_bg_path,
				name,
				tip,
				index,
				pixbuf,
				run_script_callback,
				launch_parameters,
				NULL);

	g_object_unref (pixbuf);
	g_free (name);
	g_free (tip);
}

static void
add_submenu_to_script_menus (FMDirectoryView *directory_view,
			     NautilusFile *file,
			     const char *menu_path,
			     const char *popup_path,
			     const char *popup_bg_path)
{
	char *name;
	GdkPixbuf *pixbuf;

	name = nautilus_file_get_display_name (file);
	pixbuf = nautilus_icon_factory_get_pixbuf_for_file 
		(file, NULL, NAUTILUS_ICON_SIZE_FOR_MENUS);
	add_submenu (directory_view->details->ui, menu_path, name, pixbuf);
	add_submenu (directory_view->details->ui, popup_path, name, pixbuf);
	add_submenu (directory_view->details->ui, popup_bg_path, name, pixbuf);
	g_object_unref (pixbuf);
	g_free (name);
}

static gboolean
directory_belongs_in_scripts_menu (const char *uri)
{
	int num_levels;
	int i;

	if (!eel_str_has_prefix (uri, scripts_directory_uri)) {
		return FALSE;
	}

	num_levels = 0;
	for (i = scripts_directory_uri_length; uri[i] != '\0'; i++) {
		if (uri[i] == '/') {
			num_levels++;
		}
	}

	if (num_levels > MAX_MENU_LEVELS) {
		return FALSE;
	}

	return TRUE;
}

static gboolean
update_directory_in_scripts_menu (FMDirectoryView *view, NautilusDirectory *directory)
{
	char *menu_path, *popup_path, *popup_bg_path;
	GList *file_list, *filtered, *node;
	gboolean any_scripts;
	NautilusFile *file;
	NautilusDirectory *dir;
	char *uri;
	int i;
	
	uri = nautilus_directory_get_uri (directory);
	menu_path = g_strconcat (FM_DIRECTORY_VIEW_MENU_PATH_SCRIPTS_PLACEHOLDER,
				 uri + scripts_directory_uri_length,
				 NULL);
	popup_path = g_strconcat (FM_DIRECTORY_VIEW_POPUP_PATH_SCRIPTS_PLACEHOLDER,
				  uri + scripts_directory_uri_length,
				  NULL);
	popup_bg_path = g_strconcat (FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND_SCRIPTS_PLACEHOLDER,
				  uri + scripts_directory_uri_length,
				  NULL);
	g_free (uri);

	file_list = nautilus_directory_get_file_list (directory);
	filtered = nautilus_file_list_filter_hidden_and_backup (file_list, FALSE, FALSE);
	nautilus_file_list_free (file_list);

	file_list = nautilus_file_list_sort_by_display_name (filtered);

	any_scripts = FALSE;
	i = 0;
	for (node = file_list; node != NULL; node = node->next) {
		file = node->data;

		if (file_is_launchable (file)) {
			add_script_to_script_menus (view, file, i++, menu_path, popup_path, popup_bg_path);
			any_scripts = TRUE;
		} else if (nautilus_file_is_directory (file)) {
			uri = nautilus_file_get_uri (file);
			if (directory_belongs_in_scripts_menu (uri)) {
				dir = nautilus_directory_get (uri);
				add_directory_to_scripts_directory_list (view, dir);
				nautilus_directory_unref (dir);

				add_submenu_to_script_menus (view, file, menu_path, popup_path, popup_bg_path);

				any_scripts = TRUE;
			}
			g_free (uri);
		}
	}

	nautilus_file_list_free (file_list);

	g_free (popup_path);
	g_free (popup_bg_path);
	g_free (menu_path);

	return any_scripts;
}

static void
update_scripts_menu (FMDirectoryView *view)
{
	gboolean any_scripts;
	GList *sorted_copy, *node;
	NautilusDirectory *directory;
	char *uri;

	/* There is a race condition here.  If we don't mark the scripts menu as
	   valid before we begin our task then we can lose script menu updates that
	   occur before we finish. */
	view->details->scripts_invalid = FALSE;

	nautilus_bonobo_remove_menu_items_and_commands
		(view->details->ui, FM_DIRECTORY_VIEW_MENU_PATH_SCRIPTS_PLACEHOLDER);
	nautilus_bonobo_remove_menu_items_and_commands
		(view->details->ui, FM_DIRECTORY_VIEW_POPUP_PATH_SCRIPTS_PLACEHOLDER);
	nautilus_bonobo_remove_menu_items_and_commands
		(view->details->ui, FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND_SCRIPTS_PLACEHOLDER);

	/* As we walk through the directories, remove any that no longer belong. */
	any_scripts = FALSE;
	sorted_copy = nautilus_directory_list_sort_by_uri
		(nautilus_directory_list_copy (view->details->scripts_directory_list));
	for (node = sorted_copy; node != NULL; node = node->next) {
		directory = node->data;

		uri = nautilus_directory_get_uri (directory);
		if (!directory_belongs_in_scripts_menu (uri)) {
			remove_directory_from_scripts_directory_list (view, directory);
		} else if (update_directory_in_scripts_menu (view, directory)) {
			any_scripts = TRUE;
		}
		g_free (uri);
	}
	nautilus_directory_list_free (sorted_copy);

	nautilus_bonobo_set_hidden (view->details->ui, 
				    FM_DIRECTORY_VIEW_MENU_PATH_SCRIPTS_SEPARATOR, 
				    !any_scripts);
	nautilus_bonobo_set_hidden (view->details->ui, 
				    FM_DIRECTORY_VIEW_POPUP_PATH_SCRIPTS_SEPARATOR, 
				    !any_scripts);
	nautilus_bonobo_set_hidden (view->details->ui, 
				    FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND_SCRIPTS_SEPARATOR, 
				    !any_scripts);
}

static void
open_scripts_folder_callback (BonoboUIComponent *component, 
		     	      gpointer callback_data, 
		     	      const char *verb)
{      
	FMDirectoryView *view;

	view = FM_DIRECTORY_VIEW (callback_data);

	create_scripts_directory ();

	open_location (view, scripts_directory_uri, RESPECT_PREFERENCE);
	
	eel_show_info_dialog_with_details 
		(_("All executable files in this folder will appear in the "
		   "Scripts menu. Choosing a script from the menu will run "
		   "that script with any selected items as input."), 
		 _("About Scripts"),
		 _("All executable files in this folder will appear in the "
		   "Scripts menu. Choosing a script from the menu will run "
		   "that script.\n\n"
		   "When executed from a local folder, scripts will be passed "
		   "the selected file names. When executed from a remote folder "
		   "(e.g. a folder showing web or ftp content), scripts will "
		   "be passed no parameters.\n\n"
		   "In all cases, the following environment variables will be "
		   "set by Nautilus, which the scripts may use:\n\n"
		   "NAUTILUS_SCRIPT_SELECTED_FILE_PATHS: newline-delimited paths for selected files (only if local)\n\n"
		   "NAUTILUS_SCRIPT_SELECTED_URIS: newline-delimited URIs for selected files\n\n"
		   "NAUTILUS_SCRIPT_CURRENT_URI: URI for current location\n\n"
		   "NAUTILUS_SCRIPT_WINDOW_GEOMETRY: position and size of current window"),
		 fm_directory_view_get_containing_window (view));
}

static BonoboWindow *
get_bonobo_window (FMDirectoryView *view)
{
	GtkWidget *window;
	
	/* Note: This works only because we are in the same process
	 * as the Nautilus shell. Bonobo components in their own
	 * processes can't do this.
	 */
	window = gtk_widget_get_ancestor (GTK_WIDGET (view), BONOBO_TYPE_WINDOW);
	g_assert (window != NULL);

	return BONOBO_WINDOW (window);
}

static GtkMenu *
create_popup_menu (FMDirectoryView *view, const char *popup_path)
{
	GtkMenu *menu;
	
	menu = GTK_MENU (gtk_menu_new ());
	gtk_menu_set_screen (menu, gtk_widget_get_screen (GTK_WIDGET (view)));
	gtk_widget_show (GTK_WIDGET (menu));

	bonobo_window_add_popup (get_bonobo_window (view), menu, popup_path);

	return menu;
}

static char *
convert_file_list_to_string (GList *files,
			     gboolean cut)
{
	GString *uris;
	GList *node;
	char *uri, *result;

	uris = g_string_new (cut ? "cut" : "copy");
	
	for (node = files; node != NULL; node = node->next) {
		uri = nautilus_file_get_uri (node->data);
		g_string_append_c (uris, '\n');
		g_string_append (uris, uri);
		g_free (uri);
	}

	result = uris->str;
	g_string_free (uris, FALSE);

	return result;
}

static void
get_clipboard_callback (GtkClipboard     *clipboard,
			GtkSelectionData *selection_data,
			guint             info,
			gpointer          user_data_or_owner)
{
	char *str = user_data_or_owner;

	gtk_selection_data_set (selection_data,
				copied_files_atom,
				8,
				str,
				strlen (str));
}

static void
clear_clipboard_callback (GtkClipboard *clipboard,
			  gpointer      user_data_or_owner)
{
	g_free (user_data_or_owner);
}

static void
copy_or_cut_files (FMDirectoryView *view,
		   gboolean cut)
{
	int count;
	char *status_string, *name;
	GList *clipboard_contents;
	char *clipboard_string;
	
	clipboard_contents = fm_directory_view_get_selection (view);

	clipboard_string = convert_file_list_to_string (clipboard_contents, cut);
	
	gtk_clipboard_set_with_data (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
				     clipboard_targets, G_N_ELEMENTS (clipboard_targets),
				     get_clipboard_callback, clear_clipboard_callback,
				     clipboard_string);
	
	
	count = g_list_length (clipboard_contents);
	if (count == 1) {
		name = nautilus_file_get_display_name (clipboard_contents->data);
		if (cut) {
			status_string = g_strdup_printf (_("\"%s\" will be moved "
							   "if you select the Paste Files command"),
							 name);
		} else {
			status_string = g_strdup_printf (_("\"%s\" will be copied "
							   "if you select the Paste Files command"),
							 name);
		}
		g_free (name);
	} else {
		if (cut) {
			status_string = g_strdup_printf (_("The %d selected items will be moved "
							   "if you select the Paste Files command"),
							 count);
		} else {
			status_string = g_strdup_printf (_("The %d selected items will be copied "
							   "if you select the Paste Files command"),
							 count);
		}
	}
	nautilus_file_list_free (clipboard_contents);
	
	nautilus_view_report_status (view->details->nautilus_view,
				     status_string);
	g_free (status_string);
}

static void
copy_files_callback (BonoboUIComponent *component,
		     gpointer callback_data,
		     const char *verb)
{
	copy_or_cut_files (callback_data, FALSE);
}

static void
cut_files_callback (BonoboUIComponent *component,
		    gpointer callback_data,
		    const char *verb)
{
	copy_or_cut_files (callback_data, TRUE);
}

static GList *
convert_lines_to_str_list (char **lines, gboolean *cut)
{
	int i;
	GList *result;

	if (lines[0] == NULL) {
		return NULL;
	}

	if (strcmp (lines[0], "cut") == 0) {
		*cut = TRUE;
	} else if (strcmp (lines[0], "copy") == 0) {
		*cut = FALSE;
	} else {
		return NULL;
	}

	result = NULL;
	for (i = 1; lines[i] != NULL; i++) {
		result = g_list_prepend (result, g_strdup (lines[i]));
	}
	return g_list_reverse (result);
}

static void
clipboard_received_callback (GtkClipboard     *clipboard,
			     GtkSelectionData *selection_data,
			     gpointer          data)
{
	FMDirectoryView *view;
	char **lines;
	gboolean cut;
	GList *item_uris;
	char *view_uri;

	view = FM_DIRECTORY_VIEW (data);
	
	if (selection_data->type != copied_files_atom
	    || selection_data->length <= 0) {
		item_uris = NULL;
	} else {
		/* Not sure why it's legal to assume there's an extra byte
		 * past the end of the selection data that it's safe to write
		 * to. But gtk_editable_selection_received does this, so I
		 * think it is OK.
		 */
		selection_data->data[selection_data->length] = '\0';
		lines = g_strsplit (selection_data->data, "\n", 0);
		item_uris = convert_lines_to_str_list (lines, &cut);
		g_strfreev (lines);
	}

	view_uri = fm_directory_view_get_uri (view);

	if (item_uris == NULL|| view_uri == NULL) {
		nautilus_view_report_status (view->details->nautilus_view,
					     _("There is nothing on the clipboard to paste."));
	} else {
		fm_directory_view_move_copy_items (item_uris, NULL, view_uri,
						   cut ? GDK_ACTION_MOVE : GDK_ACTION_COPY,
						   0, 0,
						   view);
	}
}

static void
paste_files_callback (BonoboUIComponent *component,
		      gpointer callback_data,
		      const char *verb)
{
	gtk_clipboard_request_contents (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
					copied_files_atom,
					clipboard_received_callback,
					callback_data);
}

static void
rename_file_callback (BonoboUIComponent *component, gpointer callback_data, const char *verb)
{
	FMDirectoryView *view;
	NautilusFile *file;
	GList *selection;
	
	view = FM_DIRECTORY_VIEW (callback_data);
	selection = fm_directory_view_get_selection (view);

	file = NAUTILUS_FILE (selection->data);

	EEL_CALL_METHOD (FM_DIRECTORY_VIEW_CLASS, view, start_renaming_file, (view, file));
	
	nautilus_file_list_free (selection);
}

static void
real_merge_menus (FMDirectoryView *view)
{
	BonoboUIVerb verbs [] = {
		BONOBO_UI_VERB ("Rename", rename_file_callback),
		BONOBO_UI_VERB ("Copy Files", copy_files_callback),
		BONOBO_UI_VERB ("Create Link", create_link_callback),
		BONOBO_UI_VERB ("Cut Files", cut_files_callback),
		BONOBO_UI_VERB ("Delete", delete_callback),
		BONOBO_UI_VERB ("Duplicate", duplicate_callback),
		BONOBO_UI_VERB ("Empty Trash", bonobo_menu_empty_trash_callback),
		BONOBO_UI_VERB ("New Folder", new_folder_callback),
		BONOBO_UI_VERB ("New Launcher", new_launcher_callback),
		BONOBO_UI_VERB ("Open Scripts Folder", open_scripts_folder_callback),
		BONOBO_UI_VERB ("Open", open_callback),
		BONOBO_UI_VERB ("OpenAlternate", open_alternate_callback),
		BONOBO_UI_VERB ("OtherApplication", other_application_callback),
		BONOBO_UI_VERB ("OtherViewer", other_viewer_callback),
		BONOBO_UI_VERB ("Edit Launcher", edit_launcher_callback),
		BONOBO_UI_VERB ("Paste Files", paste_files_callback),
		BONOBO_UI_VERB ("Reset Background", reset_background_callback),
		BONOBO_UI_VERB ("Reset to Defaults", reset_to_defaults_callback),
		BONOBO_UI_VERB ("Select All", bonobo_menu_select_all_callback),
		BONOBO_UI_VERB ("Properties", open_properties_window_callback),
		BONOBO_UI_VERB ("Trash", trash_callback),
		BONOBO_UI_VERB_END
	};

	view->details->ui = nautilus_view_set_up_ui (view->details->nautilus_view,
						     DATADIR,
						     "nautilus-directory-view-ui.xml",
						     "nautilus");
	eel_add_weak_pointer (&view->details->ui);

	bonobo_ui_component_add_verb_list_with_data (view->details->ui, verbs, view);

	g_signal_connect_object (fm_directory_view_get_background (view), "settings_changed",
				 G_CALLBACK (schedule_update_menus), G_OBJECT (view),
				 G_CONNECT_SWAPPED);

	/* Do one-time state changes here; context-dependent ones go in update_menus */
	if (!fm_directory_view_supports_zooming (view)) {
		nautilus_bonobo_set_hidden 
			(view->details->ui, NAUTILUS_POPUP_PATH_ZOOM_ITEMS_PLACEHOLDER, TRUE);
	}

	view->details->scripts_invalid = TRUE;
}

static void
clipboard_targets_received (GtkClipboard     *clipboard,
			    GtkSelectionData *selection_data,
			    gpointer          user_data)
{
	FMDirectoryView *view;
	gboolean can_paste;
	GdkAtom *targets;
	int n_targets;
	int i;

	view = FM_DIRECTORY_VIEW (user_data);
	can_paste = FALSE;

	if (gtk_selection_data_get_targets (selection_data, &targets, &n_targets)) {
		for (i=0; i < n_targets; i++) {
			if (targets[i] == copied_files_atom) {
				can_paste = TRUE;
			}
		}

		g_free (targets);
	}

	if (view->details->ui != NULL)
		nautilus_bonobo_set_sensitive (view->details->ui,
					       FM_DIRECTORY_VIEW_COMMAND_PASTE_FILES,
					       can_paste);
	g_object_unref (view);
}

static void
real_update_menus (FMDirectoryView *view)
{
	GList *selection;
	gint selection_count;
	const char *tip, *accelerator, *label;
	char *label_with_underscore;
	gboolean selection_contains_special_link;
	gboolean is_read_only;
	gboolean can_create_files;
	gboolean can_delete_files;
	gboolean can_copy_files;
	gboolean can_link_files;
	gboolean can_duplicate_files;
	gboolean show_separate_delete_command;
	gboolean vfolder_directory;
	EelBackground *background;

	if (view->details->ui == NULL) {
		return;
	}
	
	selection = fm_directory_view_get_selection (view);
	selection_count = g_list_length (selection);

	selection_contains_special_link = special_link_in_selection (view);
	is_read_only = fm_directory_view_is_read_only (view);

	can_create_files = fm_directory_view_supports_creating_files (view);
	can_delete_files = !is_read_only
		&& selection_count != 0
		&& !selection_contains_special_link;
	can_copy_files = selection_count != 0
		&& !selection_contains_special_link;
	can_duplicate_files = can_create_files && can_copy_files;
	can_link_files = can_create_files && can_copy_files;
	
	vfolder_directory = we_are_in_vfolder_desktop_dir (view);

	bonobo_ui_component_freeze (view->details->ui, NULL);

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_RENAME,
				       selection_count == 1 &&
				       fm_directory_view_can_rename_file (view, selection->data));

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_NEW_FOLDER,
				       can_create_files);

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_OPEN,
				       selection_count != 0);

	if (use_new_window_auto_value) {
		nautilus_bonobo_set_sensitive (view->details->ui, 
					       FM_DIRECTORY_VIEW_COMMAND_OPEN_ALTERNATE,
					       selection_count == 1);

		nautilus_bonobo_set_label
			(view->details->ui,
			 FM_DIRECTORY_VIEW_COMMAND_OPEN_ALTERNATE,
			 _("Open _in This Window"));
	} else {
		if (selection_count <= 1) {
			label_with_underscore = g_strdup (_("Open _in New Window"));
		} else {
			label_with_underscore = g_strdup_printf (_("Open _in %d New Windows"), selection_count);
		}
		nautilus_bonobo_set_label
			(view->details->ui,
			 FM_DIRECTORY_VIEW_COMMAND_OPEN_ALTERNATE,
			 label_with_underscore);
		g_free (label_with_underscore);
					   
		nautilus_bonobo_set_sensitive (view->details->ui, 
					       FM_DIRECTORY_VIEW_COMMAND_OPEN_ALTERNATE,
					       selection_count != 0);
	}	

	/* Broken into its own function just for convenience */
	reset_bonobo_open_with_menu (view, selection);
	reset_bonobo_mime_actions_menu (view, selection);

	if (all_selected_items_in_trash (view)) {
		label = _("_Delete from Trash");
		accelerator = "*ctl*t";
		tip = _("Delete all selected items permanently");
		show_separate_delete_command = FALSE;
	} else {
		label = _("Mo_ve to Trash");
		accelerator = "*ctl*t";
		tip = _("Move each selected item to the Trash");
		show_separate_delete_command = show_delete_command_auto_value;
	}
	
	nautilus_bonobo_set_label
		(view->details->ui,
		 FM_DIRECTORY_VIEW_COMMAND_TRASH,
		 label);
	nautilus_bonobo_set_accelerator (view->details->ui, 
					 FM_DIRECTORY_VIEW_MENU_PATH_TRASH, 
					 accelerator);
	nautilus_bonobo_set_tip (view->details->ui, 
				 FM_DIRECTORY_VIEW_COMMAND_TRASH, 
				 tip);
	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_TRASH,
				       can_delete_files);
	
	nautilus_bonobo_set_hidden (view->details->ui, 
				    FM_DIRECTORY_VIEW_COMMAND_DELETE,
				    !show_separate_delete_command);
	if (show_separate_delete_command) {
		nautilus_bonobo_set_label
			(view->details->ui,
			 FM_DIRECTORY_VIEW_COMMAND_DELETE, _("_Delete"));
		nautilus_bonobo_set_sensitive (view->details->ui, 
					       FM_DIRECTORY_VIEW_COMMAND_DELETE,
					       can_delete_files);
	}

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_DUPLICATE,
				       can_duplicate_files);

	background = fm_directory_view_get_background (view);
	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_RESET_BACKGROUND,
				       background != NULL
				       && nautilus_file_background_is_set (background));

	nautilus_bonobo_set_label
		(view->details->ui,
		 FM_DIRECTORY_VIEW_COMMAND_CREATE_LINK,
		 selection_count > 1
			? _("Ma_ke Links")
			: _("Ma_ke Link"));
	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_CREATE_LINK,
				       can_link_files);

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_PROPERTIES,
				       selection_count != 0
			      		&& fm_directory_view_supports_properties (view));

	nautilus_bonobo_set_label
		(view->details->ui,
		 FM_DIRECTORY_VIEW_COMMAND_EMPTY_TRASH,
		 _("_Empty Trash"));
	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_EMPTY_TRASH,
				       !nautilus_trash_monitor_is_empty ());

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       NAUTILUS_COMMAND_SELECT_ALL,
				       !fm_directory_view_is_empty (view));

	nautilus_bonobo_set_label
		(view->details->ui,
		 FM_DIRECTORY_VIEW_COMMAND_CUT_FILES,
		 selection_count == 1
		 ? _("Cu_t File")
		 : _("Cu_t Files"));
	nautilus_bonobo_set_sensitive (view->details->ui,
				       FM_DIRECTORY_VIEW_COMMAND_CUT_FILES,
				       can_delete_files);

	nautilus_bonobo_set_label
		(view->details->ui,
		 FM_DIRECTORY_VIEW_COMMAND_COPY_FILES,
		 selection_count == 1
		 ? _("_Copy File")
		 : _("_Copy Files"));
	nautilus_bonobo_set_sensitive (view->details->ui,
				       FM_DIRECTORY_VIEW_COMMAND_COPY_FILES,
				       can_copy_files);

	if (is_read_only) {
		nautilus_bonobo_set_sensitive (view->details->ui,
					       FM_DIRECTORY_VIEW_COMMAND_PASTE_FILES,
					       FALSE);
	} else {
		/* Ask the clipboard */
		g_object_ref (view); /* Need to keep the object alive until we get the reply */
		gtk_clipboard_request_contents (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
						gdk_atom_intern ("TARGETS", FALSE),
						clipboard_targets_received,
						view);
	}

	nautilus_bonobo_set_hidden (view->details->ui, 
				    FM_DIRECTORY_VIEW_COMMAND_NEW_LAUNCHER,
				    ! vfolder_directory);

	nautilus_bonobo_set_hidden (view->details->ui, 
				    FM_DIRECTORY_VIEW_COMMAND_EDIT_LAUNCHER,
				    ! vfolder_directory);

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_NEW_LAUNCHER,
				       can_create_files);

	nautilus_bonobo_set_sensitive (view->details->ui, 
				       FM_DIRECTORY_VIEW_COMMAND_EDIT_LAUNCHER,
				       selection_count == 1);

	bonobo_ui_component_thaw (view->details->ui, NULL);

	nautilus_file_list_free (selection);

	if (view->details->scripts_invalid) {
		update_scripts_menu (view);
	}
}

/**
 * fm_directory_view_pop_up_selection_context_menu
 *
 * Pop up a context menu appropriate to the selected items.
 * @view: FMDirectoryView of interest.
 * @event: The event that triggered this context menu.
 * 
 * Return value: NautilusDirectory for this view.
 * 
 **/
void 
fm_directory_view_pop_up_selection_context_menu  (FMDirectoryView *view, 
						  GdkEventButton *event)
{
	g_assert (FM_IS_DIRECTORY_VIEW (view));

	/* Make the context menu items not flash as they update to proper disabled,
	 * etc. states by forcing menus to update now.
	 */
	update_menus_if_pending (view);

	eel_pop_up_context_menu (create_popup_menu 
				      	(view, FM_DIRECTORY_VIEW_POPUP_PATH_SELECTION),
				      EEL_DEFAULT_POPUP_MENU_DISPLACEMENT,
				      EEL_DEFAULT_POPUP_MENU_DISPLACEMENT,
				      event);
}

/**
 * fm_directory_view_pop_up_background_context_menu
 *
 * Pop up a context menu appropriate to the view globally at the last right click location.
 * @view: FMDirectoryView of interest.
 * 
 * Return value: NautilusDirectory for this view.
 * 
 **/
void 
fm_directory_view_pop_up_background_context_menu (FMDirectoryView *view, 
						  GdkEventButton *event)
{
	g_assert (FM_IS_DIRECTORY_VIEW (view));

	/* Make the context menu items not flash as they update to proper disabled,
	 * etc. states by forcing menus to update now.
	 */
	update_menus_if_pending (view);

	eel_pop_up_context_menu (create_popup_menu 
				      (view, FM_DIRECTORY_VIEW_POPUP_PATH_BACKGROUND),
				      EEL_DEFAULT_POPUP_MENU_DISPLACEMENT,
				      EEL_DEFAULT_POPUP_MENU_DISPLACEMENT,
				      event);
}

static void
schedule_update_menus (FMDirectoryView *view) 
{
	g_assert (FM_IS_DIRECTORY_VIEW (view));

	/* Make sure we haven't already destroyed it */
	g_assert (view->details->nautilus_view != NULL);

	view->details->menu_states_untrustworthy = TRUE;

	if (view->details->menus_merged
	    && view->details->update_menus_timeout_id == 0) {
		view->details->update_menus_timeout_id
			= gtk_timeout_add (300, update_menus_timeout_callback, view);
	}
}

/**
 * fm_directory_view_notify_selection_changed:
 * 
 * Notify this view that the selection has changed. This is normally
 * called only by subclasses.
 * @view: FMDirectoryView whose selection has changed.
 * 
 **/
void
fm_directory_view_notify_selection_changed (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	if (!view->details->selection_change_is_due_to_shell) {
		view->details->send_selection_change_to_shell = TRUE;
	}

	/* Schedule a display of the new selection. */
	if (view->details->display_selection_idle_id == 0) {
		view->details->display_selection_idle_id
			= gtk_idle_add (display_selection_info_idle_callback,
					view);
	}

	if (view->details->batching_selection_level != 0) {
		view->details->selection_changed_while_batched = TRUE;
	} else {
		/* Here is the work we do only when we're not
		 * batching selection changes. In other words, it's the slower
		 * stuff that we don't want to slow down selection techniques
		 * such as rubberband-selecting in icon view.
		 */

		/* Schedule an update of menu item states to match selection */
		schedule_update_menus (view);
	}
}

static gboolean
file_is_launchable (NautilusFile *file)
{
	char *mime_type;
	gboolean type_can_be_executable;

	mime_type = nautilus_file_get_mime_type (file);
	type_can_be_executable = gnome_vfs_mime_can_be_executable (mime_type);
	g_free (mime_type);

	return type_can_be_executable 
		&& nautilus_file_can_get_permissions (file)
		&& nautilus_file_can_execute (file)
		&& nautilus_file_is_executable (file);
}

static void
report_broken_symbolic_link (FMDirectoryView *view, NautilusFile *file)
{
	char *target_path;
	char *prompt;
	GtkDialog *dialog;
	GList file_as_list;
	int response;
	
	g_assert (nautilus_file_is_broken_symbolic_link (file));

	target_path = nautilus_file_get_symbolic_link_target_path (file);
	if (target_path == NULL) {
		prompt = g_strdup_printf (_("This link can't be used, because it has no target. "
					    "Do you want to move this link to the Trash?"));
	} else {
		prompt = g_strdup_printf (_("This link can't be used, because its target \"%s\" doesn't exist. "
				 	    "Do you want to move this link to the Trash?"),
					  target_path);
	}

	dialog = eel_show_yes_no_dialog (prompt,
					 _("Broken Link"), _("Mo_ve to Trash"), GTK_STOCK_CANCEL,
					 fm_directory_view_get_containing_window (view));

	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_YES);

	/* Make this modal to avoid problems with reffing the view & file
	 * to keep them around in case the view changes, which would then
	 * cause the old view not to be destroyed, which would cause its
	 * merged Bonobo items not to be un-merged. Maybe we need to unmerge
	 * explicitly when disconnecting views instead of relying on the
	 * unmerge in Destroy. But since BonoboUIHandler is probably going
	 * to change wildly, I don't want to mess with this now.
	 */

	response = gtk_dialog_run (dialog);
	gtk_object_destroy (GTK_OBJECT (dialog));

	if (response == GTK_RESPONSE_YES) {
		file_as_list.data = file;
		file_as_list.next = NULL;
		file_as_list.prev = NULL;
	        trash_or_delete_files (view, &file_as_list);					 
	}

	g_free (target_path);
	g_free (prompt);
}

static ActivationAction
get_executable_text_file_action (FMDirectoryView *view, NautilusFile *file)
{
	GtkDialog *dialog;
	char *file_name;
	char *prompt;
	int preferences_value;
	int response;

	g_assert (nautilus_file_contains_text (file));

	preferences_value = eel_preferences_get_enum 
		(NAUTILUS_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION);
	switch (preferences_value) {
	case NAUTILUS_EXECUTABLE_TEXT_LAUNCH:
		return ACTIVATION_ACTION_LAUNCH;
	case NAUTILUS_EXECUTABLE_TEXT_DISPLAY:
		return ACTIVATION_ACTION_DISPLAY;
	case NAUTILUS_EXECUTABLE_TEXT_ASK:
		break;
	default:
		/* Complain non-fatally, since preference data can't be trusted */
		g_warning ("Unknown value %d for NAUTILUS_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION",
			   preferences_value);
		
	}


	file_name = nautilus_file_get_display_name (file);
	prompt = g_strdup_printf (_("\"%s\" is an executable text file. "
				    "Do you want to run it, or display its contents?"),
				  file_name);
	g_free (file_name);

	dialog = eel_create_question_dialog (prompt,
					     _("Run or Display?"),
					     _("_Display"), RESPONSE_DISPLAY,
					     _("Run in _Terminal"), RESPONSE_RUN_IN_TERMINAL,
					     fm_directory_view_get_containing_window (view));
	gtk_dialog_add_button (dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (dialog, _("_Run"), RESPONSE_RUN);
	gtk_widget_show (GTK_WIDGET (dialog));
	
	g_free (prompt);

	response = gtk_dialog_run (dialog);
	gtk_object_destroy (GTK_OBJECT (dialog));
	
	switch (response) {
	case RESPONSE_RUN:
		return ACTIVATION_ACTION_LAUNCH;
	case RESPONSE_RUN_IN_TERMINAL:
		return ACTIVATION_ACTION_LAUNCH_IN_TERMINAL;
	case RESPONSE_DISPLAY:
		return ACTIVATION_ACTION_DISPLAY;
	default:
		return ACTIVATION_ACTION_DO_NOTHING;
	}
}

static void
activate_callback (NautilusFile *file, gpointer callback_data)
{
	ActivateParameters *parameters;
	FMDirectoryView *view;
	char *uri, *file_uri;
	char *executable_path, *quoted_path, *name;
	GnomeVFSMimeApplication *application;
	ActivationAction action;
	GdkScreen *screen;
	
	parameters = callback_data;

	eel_timed_wait_stop (cancel_activate_callback, parameters);

	view = FM_DIRECTORY_VIEW (parameters->view);

	uri = nautilus_file_get_activation_uri (file);

	action = ACTIVATION_ACTION_DISPLAY;

	screen = gtk_widget_get_screen (GTK_WIDGET (view));

	/* Note that we check for FILE_TYPE_SYMBOLIC_LINK only here,
	 * not specifically for broken-ness, because the file type
	 * will be the target's file type in the non-broken case.
	 */
	if (nautilus_file_is_broken_symbolic_link (file)) {
		report_broken_symbolic_link (view, file);
		action = ACTIVATION_ACTION_DO_NOTHING;
	} else if (eel_str_has_prefix (uri, NAUTILUS_DESKTOP_COMMAND_SPECIFIER)) {
		file_uri = nautilus_file_get_uri (file);
		nautilus_launch_desktop_file (
				screen, file_uri, NULL,
				fm_directory_view_get_containing_window (view));
		g_free (file_uri);		 
		action = ACTIVATION_ACTION_DO_NOTHING;
	} else if (eel_str_has_prefix (uri, NAUTILUS_COMMAND_SPECIFIER)) {
		uri += strlen (NAUTILUS_COMMAND_SPECIFIER);
		nautilus_launch_application_from_command (screen, NULL, uri, NULL, FALSE);
		action = ACTIVATION_ACTION_DO_NOTHING;
	}

	if (action != ACTIVATION_ACTION_DO_NOTHING && file_is_launchable (file)) {

		action = ACTIVATION_ACTION_LAUNCH;
		
		/* FIXME bugzilla.gnome.org 42391: This should check if
		 * the activation URI points to something launchable,
		 * not the original file. Also, for symbolic links we
		 * need to check the X bit on the target file, not on
		 * the original.
		 */

		/* Launch executables to activate them. */
		executable_path = gnome_vfs_get_local_path_from_uri (uri);

		/* Non-local executables don't get launched. They act like non-executables. */
		if (executable_path == NULL) {
			action = ACTIVATION_ACTION_DISPLAY;
		} else if (nautilus_file_contains_text (file)) {
			/* Special case for executable text files, since it might be
			 * dangerous & unexpected to launch these.
			 */
			action = get_executable_text_file_action (view, file);
		}

		if (action == ACTIVATION_ACTION_LAUNCH ||
		    action == ACTIVATION_ACTION_LAUNCH_IN_TERMINAL) {
			quoted_path = g_shell_quote (executable_path);
			name = nautilus_file_get_name (file);
			nautilus_launch_application_from_command 
						(screen, name, quoted_path, NULL,
						 (action == ACTIVATION_ACTION_LAUNCH_IN_TERMINAL) /* use terminal */ );
			g_free (name);
			g_free (quoted_path);
		}

		g_free (executable_path);
	}

	if (action == ACTIVATION_ACTION_DISPLAY) {
		/* BADHACK(tm) to make desktop web links work */
		if (nautilus_file_is_nautilus_link (file) &&
		    uri != NULL &&
		    (eel_str_has_prefix (uri, "http:") ||
		     eel_str_has_prefix (uri, "https:"))) {
			gnome_url_show (uri, NULL);
		} else {
			if (nautilus_mime_get_default_action_type_for_file (file)
			    == GNOME_VFS_MIME_ACTION_TYPE_APPLICATION) {
				application = nautilus_mime_get_default_application_for_file (file);
			} else {
				/* If the action type is unspecified, treat it like
				 * the component case. This is most likely to happen
				 * (only happens?) when there are no registered
				 * viewers or apps, or there are errors in the
				 * mime.keys files.
				 */
				application = NULL;
			}
			
			if (application != NULL) {
				fm_directory_view_launch_application (application, file, view);
				gnome_vfs_mime_application_free (application);
			} else {
				open_location (view, uri, parameters->choice);
			}
		}
	}

	g_free (uri);
	g_free (parameters);
}

static void
cancel_activate_callback (gpointer callback_data)
{
	ActivateParameters *parameters;

	parameters = (ActivateParameters *) callback_data;

	nautilus_file_cancel_call_when_ready (parameters->file, 
					      activate_callback, 
					      parameters);

	g_free (parameters);
}

/**
 * fm_directory_view_activate_file:
 * 
 * Activate a file in this view. This might involve switching the displayed
 * location for the current window, or launching an application.
 * @view: FMDirectoryView in question.
 * @file: A NautilusFile representing the file in this view to activate.
 * @use_new_window: Should this item be opened in a new window?
 * 
 **/
static void
fm_directory_view_activate_file (FMDirectoryView *view, 
				 NautilusFile *file,
				 WindowChoice choice)
{
	ActivateParameters *parameters;
	GList *attributes;
	char *file_name;
	char *timed_wait_prompt;

	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));
	g_return_if_fail (NAUTILUS_IS_FILE (file));

	/* Might have to read some of the file to activate it. */
	attributes = nautilus_mime_actions_get_minimum_file_attributes ();
	attributes = g_list_prepend (attributes, NAUTILUS_FILE_ATTRIBUTE_ACTIVATION_URI);
	attributes = g_list_prepend (attributes, NAUTILUS_FILE_ATTRIBUTE_FILE_TYPE);
	parameters = g_new (ActivateParameters, 1);
	parameters->view = view;
	parameters->file = file;
	parameters->choice = choice;

	file_name = nautilus_file_get_display_name (file);
	timed_wait_prompt = g_strdup_printf (_("Opening \"%s\""), file_name);
	g_free (file_name);
	
	eel_timed_wait_start
		(cancel_activate_callback,
		 parameters,
		 _("Cancel Open?"),
		 timed_wait_prompt,
		 fm_directory_view_get_containing_window (view));
	g_free (timed_wait_prompt);
	nautilus_file_call_when_ready
		(file, attributes, activate_callback, parameters);

	g_list_free (attributes);
}


/**
 * fm_directory_view_activate_files:
 * 
 * Activate a list of files. Each one might launch with an application or
 * with a component. This is normally called only by subclasses.
 * @view: FMDirectoryView in question.
 * @files: A GList of NautilusFiles to activate.
 * 
 **/
void
fm_directory_view_activate_files (FMDirectoryView *view, 
				  GList *files)
{
	GList *node;
	int file_count;
	gboolean use_new_window;

	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	/* If there's a single file to activate, check user's preference whether
	 * to open it in this window or a new window. If there is more than one
	 * file to activate, open each one in a new window. Don't try to choose
	 * one special one to replace the current window's contents; we tried this
	 * but it proved mysterious in practice.
	 */
	file_count = g_list_length (files);
	use_new_window = file_count > 1	|| use_new_window_auto_value;
	
	if (!use_new_window || fm_directory_view_confirm_multiple_windows (view, file_count)) {
		for (node = files; node != NULL; node = node->next) {  	
			fm_directory_view_activate_file 
				(view, node->data,
				 file_count == 1
				 ? RESPECT_PREFERENCE
				 : PREFER_EXISTING_WINDOW);
		}
	}
}

static void
file_changed_callback (NautilusFile *file, gpointer callback_data)
{
	FMDirectoryView *view = FM_DIRECTORY_VIEW (callback_data);

	schedule_update_menus (view);

	/* We might have different capabilities, so we need to update
	   relative icon emblems . (Writeable etc) */
	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view, emblems_changed, (view));
}

/**
 * load_directory:
 * 
 * Switch the displayed location to a new uri. If the uri is not valid,
 * the location will not be switched; user feedback will be provided instead.
 * @view: FMDirectoryView whose location will be changed.
 * @uri: A string representing the uri to switch to.
 * 
 **/
static void
load_directory (FMDirectoryView *view,
		NautilusDirectory *directory)
{
	NautilusDirectory *old_directory;
	NautilusFile *old_file;
	GList *attributes;

	g_assert (FM_IS_DIRECTORY_VIEW (view));
	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	fm_directory_view_stop (view);
	fm_directory_view_clear (view);

	/* Update menus when directory is empty, before going to new
	 * location, so they won't have any false lingering knowledge
	 * of old selection.
	 */
	schedule_update_menus (view);

	disconnect_model_handlers (view);

	old_directory = view->details->model;
	nautilus_directory_ref (directory);
	view->details->model = directory;
	nautilus_directory_unref (old_directory);

	old_file = view->details->directory_as_file;
	view->details->directory_as_file =
		nautilus_directory_get_corresponding_file (directory);
	nautilus_file_unref (old_file);

	view->details->reported_load_error = FALSE;

	/* FIXME bugzilla.gnome.org 45062: In theory, we also need to monitor metadata here (as
         * well as doing a call when ready), in case external forces
         * change the directory's file metadata.
	 */
	attributes = g_list_prepend (NULL, NAUTILUS_FILE_ATTRIBUTE_METADATA);
	view->details->metadata_for_directory_as_file_pending = TRUE;
	view->details->metadata_for_files_in_directory_pending = TRUE;
	nautilus_file_call_when_ready
		(view->details->directory_as_file,
		 attributes,
		 metadata_for_directory_as_file_ready_callback, view);
	nautilus_directory_call_when_ready
		(view->details->model,
		 attributes,
		 FALSE,
		 metadata_for_files_in_directory_ready_callback, view);
	g_list_free (attributes);

	/* If capabilities change, then we need to update the menus
	 * because of New Folder, and relative emblems.
	 */
	attributes = g_list_prepend (NULL, NAUTILUS_FILE_ATTRIBUTE_CAPABILITIES);
	nautilus_file_monitor_add (view->details->directory_as_file,
				   &view->details->directory_as_file,
				   attributes);
	g_list_free (attributes);

	view->details->file_changed_handler_id = g_signal_connect
		(view->details->directory_as_file, "changed",
		 G_CALLBACK (file_changed_callback), view);
}

static void
finish_loading (FMDirectoryView *view)
{
	GList *attributes;

	nautilus_view_report_load_underway (view->details->nautilus_view);

	/* Tell interested parties that we've begun loading this directory now.
	 * Subclasses use this to know that the new metadata is now available.
	 */
	fm_directory_view_begin_loading (view);

	if (nautilus_directory_are_all_files_seen (view->details->model)) {
		schedule_idle_display_of_pending_files (view);		
	} else {
		schedule_timeout_display_of_pending_files (view);
	}
	
	view->details->loading = TRUE;

	/* Start loading. */

	/* Connect handlers to learn about loading progress. */
	view->details->done_loading_handler_id = g_signal_connect
		(view->details->model, "done_loading",
		 G_CALLBACK (done_loading_callback), view);
	view->details->load_error_handler_id = g_signal_connect
		(view->details->model, "load_error",
		 G_CALLBACK (load_error_callback), view);

	/* Monitor the things needed to get the right icon. Also
	 * monitor a directory's item count because the "size"
	 * attribute is based on that, and the file's metadata
	 * and possible custom name.
	 */
	attributes = nautilus_icon_factory_get_required_file_attributes ();
	attributes = g_list_prepend (attributes,
				     NAUTILUS_FILE_ATTRIBUTE_DIRECTORY_ITEM_COUNT);
	attributes = g_list_prepend (attributes, 
				     NAUTILUS_FILE_ATTRIBUTE_METADATA);
	attributes = g_list_prepend (attributes, 
				     NAUTILUS_FILE_ATTRIBUTE_MIME_TYPE);
	attributes = g_list_prepend (attributes, 
				     NAUTILUS_FILE_ATTRIBUTE_DISPLAY_NAME);

	nautilus_directory_file_monitor_add (view->details->model,
					     &view->details->model,
					     view->details->show_hidden_files,
					     view->details->show_backup_files,
					     attributes,
					     files_added_callback, view);

	g_list_free (attributes);

    	view->details->files_added_handler_id = g_signal_connect
		(view->details->model, "files_added",
		 G_CALLBACK (files_added_callback), view);
	view->details->files_changed_handler_id = g_signal_connect
		(view->details->model, "files_changed",
		 G_CALLBACK (files_changed_callback), view);
}

static void
finish_loading_if_all_metadata_loaded (FMDirectoryView *view)
{
	if (!view->details->metadata_for_directory_as_file_pending &&
	    !view->details->metadata_for_files_in_directory_pending) {
		finish_loading (view);
	}
}

static void
metadata_for_directory_as_file_ready_callback (NautilusFile *file,
			      		       gpointer callback_data)
{
	FMDirectoryView *view;

	view = callback_data;

	g_assert (FM_IS_DIRECTORY_VIEW (view));
	g_assert (view->details->directory_as_file == file);
	g_assert (view->details->metadata_for_directory_as_file_pending);

	view->details->metadata_for_directory_as_file_pending = FALSE;
	
	finish_loading_if_all_metadata_loaded (view);
}

static void
metadata_for_files_in_directory_ready_callback (NautilusDirectory *directory,
				   		GList *files,
			           		gpointer callback_data)
{
	FMDirectoryView *view;

	view = callback_data;

	g_assert (FM_IS_DIRECTORY_VIEW (view));
	g_assert (view->details->model == directory);
	g_assert (view->details->metadata_for_files_in_directory_pending);

	view->details->metadata_for_files_in_directory_pending = FALSE;
	
	finish_loading_if_all_metadata_loaded (view);
}

EelStringList *
fm_directory_view_get_emblem_names_to_exclude (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 get_emblem_names_to_exclude, (view));
}

static void
fm_directory_view_add_relative_emblems_to_exclude (FMDirectoryView *view,
						   EelStringList *list)
{
	if (!nautilus_file_can_write (view->details->directory_as_file)) {
		eel_string_list_prepend (list, NAUTILUS_FILE_EMBLEM_NAME_CANT_WRITE);
		eel_string_list_remove_duplicates (list);
	}
}

static EelStringList *
real_get_emblem_names_to_exclude (FMDirectoryView *view)
{
	EelStringList *list;
	
	g_assert (FM_IS_DIRECTORY_VIEW (view));

	list = eel_string_list_new_from_string (NAUTILUS_FILE_EMBLEM_NAME_TRASH, TRUE);

	fm_directory_view_add_relative_emblems_to_exclude (view, list);

	return list;
}

/**
 * fm_directory_view_merge_menus:
 * 
 * Add this view's menus to the window's menu bar.
 * @view: FMDirectoryView in question.
 */
static void
fm_directory_view_merge_menus (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	/* Remember that the menus have been merged so that we
	 * won't try to update them before merging them.
	 */
	view->details->menus_merged = TRUE;

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 merge_menus, (view));
}

static void
disconnect_handler (GtkObject *object, int *id)
{
	if (*id != 0) {
		g_signal_handler_disconnect (object, *id);
		*id = 0;
	}
}

static void
disconnect_directory_handler (FMDirectoryView *view, int *id)
{
	disconnect_handler (GTK_OBJECT (view->details->model), id);
}

static void
disconnect_directory_as_file_handler (FMDirectoryView *view, int *id)
{
	disconnect_handler (GTK_OBJECT (view->details->directory_as_file), id);
}

static void
disconnect_model_handlers (FMDirectoryView *view)
{
	if (view->details->model == NULL) {
		return;
	}
	disconnect_directory_handler (view, &view->details->files_added_handler_id);
	disconnect_directory_handler (view, &view->details->files_changed_handler_id);
	disconnect_directory_handler (view, &view->details->done_loading_handler_id);
	disconnect_directory_handler (view, &view->details->load_error_handler_id);
	disconnect_directory_as_file_handler (view, &view->details->file_changed_handler_id);
	nautilus_directory_file_monitor_remove (view->details->model,
						&view->details->model);
	nautilus_file_cancel_call_when_ready (view->details->directory_as_file,
					      metadata_for_directory_as_file_ready_callback,
					      view);
	nautilus_directory_cancel_callback (view->details->model,
					    metadata_for_files_in_directory_ready_callback,
					    view);
	nautilus_file_monitor_remove (view->details->directory_as_file,
				      &view->details->directory_as_file);
}

/**
 * fm_directory_view_reset_to_defaults:
 *
 * set sorting order, zoom level, etc. to match defaults
 * 
 **/
void
fm_directory_view_reset_to_defaults (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 reset_to_defaults, (view));
}

/**
 * fm_directory_view_select_all:
 *
 * select all the items in the view
 * 
 **/
void
fm_directory_view_select_all (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 select_all, (view));
}

/**
 * fm_directory_view_set_selection:
 *
 * set the selection to the items identified in @selection. @selection
 * should be a list of NautilusFiles
 * 
 **/
void
fm_directory_view_set_selection (FMDirectoryView *view, GList *selection)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 set_selection, (view, selection));
}

static void
fm_directory_view_select_file (FMDirectoryView *view, NautilusFile *file)
{
	GList file_list;

	file_list.data = file;
	file_list.next = NULL;
	file_list.prev = NULL;
	fm_directory_view_set_selection (view, &file_list);
}

/**
 * fm_directory_view_get_selected_icon_locations:
 *
 * return an array of locations of selected icons if available
 * Return value: GArray of GdkPoints
 * 
 **/
GArray *
fm_directory_view_get_selected_icon_locations (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 get_selected_icon_locations, (view));
}

/**
 * fm_directory_view_reveal_selection:
 *
 * Scroll as necessary to reveal the selected items.
 **/
void
fm_directory_view_reveal_selection (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 reveal_selection, (view));
}

static gboolean
unref_key_and_remove (gpointer key, gpointer value, gpointer callback_data)
{
	nautilus_file_unref (key);
	return TRUE;
}

/**
 * fm_directory_view_stop:
 * 
 * Stop the current ongoing process, such as switching to a new uri.
 * @view: FMDirectoryView in question.
 * 
 **/
void
fm_directory_view_stop (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	unschedule_display_of_pending_files (view);

	/* Free extra undisplayed files */
	nautilus_file_list_free (view->details->new_added_files);
	view->details->new_added_files = NULL;
	nautilus_file_list_free (view->details->new_changed_files);
	view->details->new_changed_files = NULL;
	g_hash_table_foreach_remove (view->details->non_ready_files, unref_key_and_remove, NULL);
	nautilus_file_list_free (view->details->old_added_files);
	view->details->old_added_files = NULL;
	nautilus_file_list_free (view->details->old_changed_files);
	view->details->old_changed_files = NULL;
	eel_g_list_free_deep (view->details->pending_uris_selected);
	view->details->pending_uris_selected = NULL;

	if (view->details->model != NULL) {
		nautilus_directory_file_monitor_remove (view->details->model, view);
	}
	done_loading (view);
}

gboolean
fm_directory_view_is_read_only (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 is_read_only, (view));
}

gboolean
fm_directory_view_is_empty (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 is_empty, (view));
}

static gboolean
real_is_read_only (FMDirectoryView *view)
{
	return !nautilus_file_can_write (fm_directory_view_get_directory_as_file (view));
}

gboolean
fm_directory_view_supports_creating_files (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 supports_creating_files, (view));
}

gboolean
fm_directory_view_accepts_dragged_files (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 accepts_dragged_files, (view));
}

static gboolean
showing_trash_directory (FMDirectoryView *view)
{
	return nautilus_file_is_in_trash (fm_directory_view_get_directory_as_file (view));
}

/**
 * fm_directory_view_should_show_file
 * 
 * Returns whether or not this file should be displayed based on
 * current filtering options.
 */
gboolean
fm_directory_view_should_show_file (FMDirectoryView *view, NautilusFile *file)
{
	return nautilus_file_should_show (file, 
					  view->details->show_hidden_files, 
					  view->details->show_backup_files);
}

static gboolean
real_supports_creating_files (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return !fm_directory_view_is_read_only (view) && !showing_trash_directory (view);
}

static gboolean
real_accepts_dragged_files (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return TRUE;
}

gboolean
fm_directory_view_supports_properties (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 supports_properties, (view));
}

static gboolean
real_supports_properties (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return TRUE;
}

gboolean
fm_directory_view_supports_zooming (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	if (!view->details->zoomable) {
		return FALSE;
	}

	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(FM_DIRECTORY_VIEW_CLASS, view,
		 supports_zooming, (view));
}

static gboolean
real_supports_zooming (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return TRUE;
}

/**
 * fm_directory_view_update_menus:
 * 
 * Update the sensitivity and wording of dynamic menu items.
 * @view: FMDirectoryView in question.
 */
void
fm_directory_view_update_menus (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	EEL_CALL_METHOD
		(FM_DIRECTORY_VIEW_CLASS, view,
		 update_menus, (view));

	view->details->menu_states_untrustworthy = FALSE;
}

static void
schedule_update_menus_callback (gpointer callback_data)
{
	schedule_update_menus (FM_DIRECTORY_VIEW (callback_data));
}

static void
filtering_changed_callback (gpointer callback_data)
{
	FMDirectoryView	*directory_view;
	gboolean new_show_hidden, new_show_backup;
	gboolean filtering_actually_changed;

	directory_view = FM_DIRECTORY_VIEW (callback_data);
	filtering_actually_changed = FALSE;

	new_show_hidden = eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES);
	if (new_show_hidden != directory_view->details->show_hidden_files) {
		filtering_actually_changed = TRUE;
		directory_view->details->show_hidden_files = new_show_hidden ;
	}

	new_show_backup = eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES);
	if (new_show_backup != directory_view->details->show_backup_files) {
		filtering_actually_changed = TRUE;
		directory_view->details->show_backup_files = new_show_backup;
	}

	/* Reload the current uri so that the filtering changes take place. */
	if (filtering_actually_changed && directory_view->details->model != NULL) {
		load_directory (directory_view,
				directory_view->details->model);
	}
}

void
fm_directory_view_ignore_hidden_file_preferences (FMDirectoryView *view)
{
	g_return_if_fail (view->details->model == NULL);

	if (view->details->ignore_hidden_file_preferences) {
		return;
	}

	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
					      filtering_changed_callback,
					      view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
					      filtering_changed_callback,
					      view);

	view->details->show_hidden_files = FALSE;
	view->details->show_backup_files = FALSE;
	view->details->ignore_hidden_file_preferences = TRUE;
}

char *
fm_directory_view_get_uri (FMDirectoryView *view)
{
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), NULL);
	if (view->details->model == NULL) {
		return NULL;
	}
	return nautilus_directory_get_uri (view->details->model);
}

void
fm_directory_view_move_copy_items (const GList *item_uris,
				   GArray *relative_item_points,
				   const char *target_uri,
				   int copy_action,
				   int x, int y,
				   FMDirectoryView *view)
{
	char *parameters, *temp;
	GList *p;
	
	g_assert (relative_item_points == NULL
		  || relative_item_points->len == 0 
		  || g_list_length ((GList *)item_uris) == relative_item_points->len);

	/* add the drop location to the icon offsets */
	offset_drop_points (relative_item_points, x, y);

	/* special-case "command:" here instead of starting a move/copy */
	if (eel_str_has_prefix (target_uri, NAUTILUS_DESKTOP_COMMAND_SPECIFIER)) {
		nautilus_launch_desktop_file (
				gtk_widget_get_screen (GTK_WIDGET (view)),
				target_uri, item_uris,
				fm_directory_view_get_containing_window (view));
		return;
	} else if (eel_str_has_prefix (target_uri, NAUTILUS_COMMAND_SPECIFIER)) {
		parameters = NULL;
		for (p = (GList *) item_uris; p != NULL; p = p->next) {
			temp = g_strconcat ((char *) p->data, " ", parameters, NULL);
			if (parameters != NULL) {
				g_free (parameters);
			}
			parameters = temp;
		}

		target_uri += strlen (NAUTILUS_COMMAND_SPECIFIER);

		nautilus_launch_application_from_command (
				gtk_widget_get_screen (GTK_WIDGET (view)),
				NULL, target_uri, parameters, FALSE);
		g_free (parameters);
		
		return;
	}
	
	if (eel_uri_is_trash (target_uri) && copy_action == GDK_ACTION_MOVE) {
		trash_or_delete_files_common (view, item_uris, relative_item_points, FALSE);
	} else {
		nautilus_file_operations_copy_move
			(item_uris, relative_item_points, 
			 target_uri, copy_action, GTK_WIDGET (view),
			 copy_move_done_callback, pre_copy_move (view));
	}
}

gboolean
fm_directory_view_can_accept_item (NautilusFile *target_item,
				   const char *item_uri,
				   FMDirectoryView *view)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE (target_item), FALSE);
	g_return_val_if_fail (item_uri != NULL, FALSE);
	g_return_val_if_fail (FM_IS_DIRECTORY_VIEW (view), FALSE);

	return nautilus_drag_can_accept_item (target_item, item_uri);
}

static void
fm_directory_view_trash_state_changed_callback (NautilusTrashMonitor *trash_monitor,
						gboolean state, gpointer callback_data)
{
	FMDirectoryView *view;

	view = (FMDirectoryView *) callback_data;
	g_assert (FM_IS_DIRECTORY_VIEW (view));
	
	schedule_update_menus (view);
}

void
fm_directory_view_start_batching_selection_changes (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));

	++view->details->batching_selection_level;
	view->details->selection_changed_while_batched = FALSE;
}

void
fm_directory_view_stop_batching_selection_changes (FMDirectoryView *view)
{
	g_return_if_fail (FM_IS_DIRECTORY_VIEW (view));
	g_return_if_fail (view->details->batching_selection_level > 0);

	if (--view->details->batching_selection_level == 0) {
		if (view->details->selection_changed_while_batched) {
			fm_directory_view_notify_selection_changed (view);
		}
	}
}

static void
monitor_file_for_open_with (FMDirectoryView *view, NautilusFile *file)
{
	NautilusFile **file_spot;
	NautilusFile *old_file;
	GList *attributes;

	/* Quick out when not changing. */
	file_spot = &view->details->file_monitored_for_open_with;
	old_file = *file_spot;
	if (old_file == file) {
		return;
	}

	/* Point at the new file. */
	nautilus_file_ref (file);
	*file_spot = file;

	/* Stop monitoring the old file. */
	if (old_file != NULL) {
		nautilus_file_monitor_remove (old_file, file_spot);
		nautilus_file_unref (old_file);
	}

	/* Start monitoring the new file. */
	if (file != NULL) {
		attributes = nautilus_mime_actions_get_full_file_attributes ();
		nautilus_file_monitor_add (file, file_spot, attributes);
		g_list_free (attributes);
	}
}


static void
real_sort_files (FMDirectoryView *view, GList **files)
{
}

static GArray *
real_get_selected_icon_locations (FMDirectoryView *view)
{
        /* By default, just return an empty list. */
        return g_array_new (FALSE, TRUE, sizeof (GdkPoint));
}

static void
fm_directory_view_class_init (FMDirectoryViewClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkScrolledWindowClass *scrolled_window_class;

	widget_class = GTK_WIDGET_CLASS (klass);
	scrolled_window_class = GTK_SCROLLED_WINDOW_CLASS (klass);

	G_OBJECT_CLASS (klass)->finalize = fm_directory_view_finalize;
	GTK_OBJECT_CLASS (klass)->destroy = fm_directory_view_destroy;

	/* Get rid of the strange 3-pixel gap that GtkScrolledWindow
	 * uses by default. It does us no good.
	 */
	scrolled_window_class->scrollbar_spacing = 0;

	signals[ADD_FILE] =
		g_signal_new ("add_file",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, add_file),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1, NAUTILUS_TYPE_FILE);
	signals[BEGIN_FILE_CHANGES] =
		g_signal_new ("begin_file_changes",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, begin_file_changes),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[BEGIN_LOADING] =
		g_signal_new ("begin_loading",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, begin_loading),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[CLEAR] =
		g_signal_new ("clear",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, clear),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[END_FILE_CHANGES] =
		g_signal_new ("end_file_changes",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, end_file_changes),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[END_LOADING] =
		g_signal_new ("end_loading",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, end_loading),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[FILE_CHANGED] =
		g_signal_new ("file_changed",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, file_changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1, NAUTILUS_TYPE_FILE);
	signals[LOAD_ERROR] =
		g_signal_new ("load_error",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, load_error),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__INT,
		              G_TYPE_NONE, 1, G_TYPE_INT);
	signals[REMOVE_FILE] =
		g_signal_new ("remove_file",
		              G_TYPE_FROM_CLASS (klass),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (FMDirectoryViewClass, remove_file),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE, 1, NAUTILUS_TYPE_FILE);

	klass->accepts_dragged_files = real_accepts_dragged_files;
	klass->file_limit_reached = real_file_limit_reached;
	klass->file_still_belongs = real_file_still_belongs;
	klass->get_emblem_names_to_exclude = real_get_emblem_names_to_exclude;
	klass->get_selected_icon_locations = real_get_selected_icon_locations;
	klass->is_read_only = real_is_read_only;
	klass->load_error = real_load_error;
	klass->sort_files = real_sort_files;
	klass->can_rename_file = can_rename_file;
	klass->start_renaming_file = start_renaming_file;
	klass->supports_creating_files = real_supports_creating_files;
	klass->supports_properties = real_supports_properties;
	klass->supports_zooming = real_supports_zooming;
        klass->merge_menus = real_merge_menus;
        klass->update_menus = real_update_menus;

	/* Function pointers that subclasses must override */
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, add_file);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, bump_zoom_level);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, can_zoom_in);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, can_zoom_out);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, clear);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, file_changed);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, get_background_widget);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, get_selection);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, is_empty);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, reset_to_defaults);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, restore_default_zoom_level);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, select_all);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, set_selection);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, fm_directory_view, zoom_to_level);

	copied_files_atom = gdk_atom_intern ("x-special/gnome-copied-files", FALSE);
}
