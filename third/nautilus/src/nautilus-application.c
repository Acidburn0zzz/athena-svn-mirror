/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *  Nautilus
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000, 2001 Eazel, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>,
 *           Darin Adler <darin@eazel.com>
 *
 */

#include <config.h>
#include "nautilus-application.h"

#include "file-manager/fm-desktop-icon-view.h"
#include "file-manager/fm-icon-view.h"
#include "file-manager/fm-list-view.h"
#include "file-manager/fm-search-list-view.h"
#include "nautilus-desktop-window.h"
#include "nautilus-first-time-druid.h"
#include "nautilus-main.h"
#include "nautilus-shell-interface.h"
#include "nautilus-shell.h"
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-object.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-config.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-client.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libnautilus-extensions/nautilus-file-utilities.h>
#include <libnautilus-extensions/nautilus-global-preferences.h>
#include <libnautilus-extensions/nautilus-gtk-macros.h>
#include <libnautilus-extensions/nautilus-icon-factory.h>
#include <libnautilus-extensions/nautilus-sound.h>
#include <libnautilus-extensions/nautilus-stock-dialogs.h>
#include <libnautilus-extensions/nautilus-string.h>
#include <libnautilus-extensions/nautilus-string-list.h>
#include <libnautilus-extensions/nautilus-undo-manager.h>
#include <libnautilus-extensions/nautilus-volume-monitor.h>
#include <libnautilus-extensions/nautilus-metafile-factory.h>
#include <liboaf/liboaf.h>

#define FACTORY_IID	     "OAFIID:nautilus_factory:bd1e1862-92d7-4391-963e-37583f0daef3"
#define ICON_VIEW_IID	     "OAFIID:nautilus_file_manager_icon_view:42681b21-d5ca-4837-87d2-394d88ecc058"
#define LIST_VIEW_IID	     "OAFIID:nautilus_file_manager_list_view:521e489d-0662-4ad7-ac3a-832deabe111c"
#define SEARCH_LIST_VIEW_IID "OAFIID:nautilus_file_manager_search_list_view:b186e381-198e-43cf-9c46-60b6bb35db0b"
#define SHELL_IID	     "OAFIID:nautilus_shell:cd5183b2-3913-4b74-9b8e-10528b0de08d"

static CORBA_boolean manufactures                          (PortableServer_Servant    servant,
							    const CORBA_char         *iid,
							    CORBA_Environment        *ev);
static CORBA_Object  create_object                         (PortableServer_Servant    servant,
							    const CORBA_char         *iid,
							    const GNOME_stringlist   *params,
							    CORBA_Environment        *ev);
static void          nautilus_application_initialize       (NautilusApplication      *application);
static void          nautilus_application_initialize_class (NautilusApplicationClass *klass);
static void          nautilus_application_destroy          (GtkObject                *object);
static gboolean      confirm_ok_to_run_as_root             (void);
static gboolean      need_to_show_first_time_druid         (void);
static void          desktop_changed_callback              (gpointer                  user_data);
static void          volume_mounted_callback               (NautilusVolumeMonitor    *monitor,
							    NautilusVolume           *volume,
							    NautilusApplication      *application);
static void          volume_unmounted_callback             (NautilusVolumeMonitor    *monitor,
							    NautilusVolume           *volume,
							    NautilusApplication      *application);
static void	     init_session 			    (void);

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusApplication, nautilus_application, BONOBO_OBJECT_TYPE)

static POA_GNOME_ObjectFactory__epv factory_epv = {
	NULL,
	&manufactures,
	&create_object
};
static PortableServer_ServantBase__epv base_epv;
static POA_GNOME_ObjectFactory__vepv vepv = {
	&base_epv,
	&factory_epv
};

static CORBA_boolean
manufactures (PortableServer_Servant servant,
	      const CORBA_char *iid,
	      CORBA_Environment *ev)
{
	return strcmp (iid, ICON_VIEW_IID) == 0
		|| strcmp (iid, NAUTILUS_DESKTOP_ICON_VIEW_IID) == 0
		|| strcmp (iid, LIST_VIEW_IID) == 0
		|| strcmp (iid, SEARCH_LIST_VIEW_IID) == 0
		|| strcmp (iid, SHELL_IID) == 0
		|| strcmp (iid, METAFILE_FACTORY_IID) == 0;
}

static CORBA_Object
create_object (PortableServer_Servant servant,
	       const CORBA_char *iid,
	       const GNOME_stringlist *params,
	       CORBA_Environment *ev)
{
	BonoboObject *object;
	FMDirectoryView *directory_view;
	NautilusApplication *application;

	if (strcmp (iid, ICON_VIEW_IID) == 0) {
		directory_view = FM_DIRECTORY_VIEW (gtk_object_new (fm_icon_view_get_type (), NULL));
		object = BONOBO_OBJECT (fm_directory_view_get_nautilus_view (directory_view));
	} else if (strcmp (iid, NAUTILUS_DESKTOP_ICON_VIEW_IID) == 0) {
		directory_view = FM_DIRECTORY_VIEW (gtk_object_new (fm_desktop_icon_view_get_type (), NULL));
		object = BONOBO_OBJECT (fm_directory_view_get_nautilus_view (directory_view));
	} else if (strcmp (iid, LIST_VIEW_IID) == 0) {
		directory_view = FM_DIRECTORY_VIEW (gtk_object_new (fm_list_view_get_type (), NULL));
		object = BONOBO_OBJECT (fm_directory_view_get_nautilus_view (directory_view));
	} else if (strcmp (iid, SEARCH_LIST_VIEW_IID) == 0) {
		directory_view = FM_DIRECTORY_VIEW (gtk_object_new (fm_search_list_view_get_type (), NULL));
		object = BONOBO_OBJECT (fm_directory_view_get_nautilus_view (directory_view));
	} else if (strcmp (iid, SHELL_IID) == 0) {
		application = NAUTILUS_APPLICATION (((BonoboObjectServant *) servant)->bonobo_object);
		object = BONOBO_OBJECT (nautilus_shell_new (application));
	} else if (strcmp (iid, METAFILE_FACTORY_IID) == 0) {
		object = BONOBO_OBJECT (nautilus_metafile_factory_get_instance ());
	} else {
		return CORBA_OBJECT_NIL;
	}

	return CORBA_Object_duplicate (bonobo_object_corba_objref (object), ev);
}

static CORBA_Object
create_factory (PortableServer_POA poa,
		NautilusApplication *bonobo_object,
		CORBA_Environment *ev)
{
	BonoboObjectServant *servant;

	servant = g_new0 (BonoboObjectServant, 1);
	((POA_GNOME_ObjectFactory *) servant)->vepv = &vepv;
	POA_GNOME_ObjectFactory__init ((PortableServer_Servant) servant, ev);
	return bonobo_object_activate_servant (BONOBO_OBJECT (bonobo_object), servant);
}

/* Keeps track of the one and only desktop window. */
static NautilusDesktopWindow *nautilus_application_desktop_window;

/* Keeps track of all the nautilus windows. */
static GList *nautilus_application_window_list;

GList *
nautilus_application_get_window_list (void)
{
	return nautilus_application_window_list;
}

static void
nautilus_application_initialize_class (NautilusApplicationClass *klass)
{
	GTK_OBJECT_CLASS (klass)->destroy = nautilus_application_destroy;
}

static void
nautilus_application_initialize (NautilusApplication *application)
{
	CORBA_Environment ev;
	CORBA_Object corba_object;

	CORBA_exception_init (&ev);
	corba_object = create_factory (bonobo_poa (), application, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_error ("could not create factory");
	}
	CORBA_exception_free (&ev);
	
	bonobo_object_construct (BONOBO_OBJECT (application), corba_object);
	
	/* Create an undo manager */
	application->undo_manager = nautilus_undo_manager_new ();

	/* Watch for volume mounts so we can restore open windows */
	gtk_signal_connect (GTK_OBJECT (nautilus_volume_monitor_get ()),
			    "volume_mounted",
			    volume_mounted_callback,
			    application);

	/* Watch for volume unmounts so we can close open windows */
	gtk_signal_connect (GTK_OBJECT (nautilus_volume_monitor_get ()),
			    "volume_unmounted",
			    volume_unmounted_callback,
			    application);
}

NautilusApplication *
nautilus_application_new (void)
{
	return NAUTILUS_APPLICATION (gtk_object_new (nautilus_application_get_type (), NULL));
}

static void
nautilus_application_destroy (GtkObject *object)
{
	NautilusApplication *application;

	application = NAUTILUS_APPLICATION (object);

	nautilus_bookmarks_exiting ();
	
	bonobo_object_unref (BONOBO_OBJECT (application->undo_manager));

	NAUTILUS_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static gboolean
check_required_directories (NautilusApplication *application)
{
	char			*user_directory;
	char			*user_main_directory;
	char			*desktop_directory;
	NautilusStringList	*dir_list;
	char 			*dir_list_concatenated;
	char 			*error_string;
	char 			*dialog_title;
	GnomeDialog 		*dialog;
	int			 failed_count;
	
	g_assert (NAUTILUS_IS_APPLICATION (application));

	user_directory = nautilus_get_user_directory ();
	user_main_directory = nautilus_get_user_main_directory ();
	desktop_directory = nautilus_get_desktop_directory ();

	dir_list = nautilus_string_list_new (TRUE);
	
	if (!g_file_test (user_directory, G_FILE_TEST_ISDIR)) {
		nautilus_string_list_insert (dir_list, user_directory);
	}
	g_free (user_directory);
	    
	if (!g_file_test (user_main_directory, G_FILE_TEST_ISDIR)) {
		nautilus_string_list_insert (dir_list, user_main_directory);
	}
	g_free (user_main_directory);
	    
	if (!g_file_test (desktop_directory, G_FILE_TEST_ISDIR)) {
		nautilus_string_list_insert (dir_list, desktop_directory);
	}
	g_free (desktop_directory);

	failed_count = nautilus_string_list_get_length (dir_list);

	if (failed_count != 0) {
		dir_list_concatenated = nautilus_string_list_as_concatenated_string (dir_list, "\n");

		if (failed_count == 1) {
			dialog_title = g_strdup (_("Couldn't Create Required Folder"));
			error_string = g_strdup_printf (_("Nautilus could not create the required folder \"%s\". "
							  "Before running Nautilus, please create this folder, or "
							  "set permissions such that Nautilus can create it."),
							dir_list_concatenated);
		} else {
			dialog_title = g_strdup (_("Couldn't Create Required Folders"));
			error_string = g_strdup_printf (_("Nautilus could not create the following required folders:\n\n"
							  "%s\n\n"
							  "Before running Nautilus, please create these folders, or "
							  "set permissions such that Nautilus can create them."),
							dir_list_concatenated);
		}
		
		dialog = nautilus_show_error_dialog (error_string, dialog_title, NULL);
		/* We need the main event loop so the user has a chance to see the dialog. */
		nautilus_main_event_loop_register (GTK_OBJECT (dialog));

		g_free (dir_list_concatenated);
		g_free (error_string);
		g_free (dialog_title);
	}

	nautilus_string_list_free (dir_list);

	return failed_count == 0;
}

static int
nautilus_strv_length (const char * const *strv)
{
	const char * const *p;

	for (p = strv; *p != NULL; p++) { }
	return p - strv;
}

static Nautilus_URIList *
nautilus_make_uri_list_from_shell_strv (const char * const *strv)
{
	int length, i;
	Nautilus_URIList *uri_list;
	char *translated_uri;

	length = nautilus_strv_length (strv);

	uri_list = Nautilus_URIList__alloc ();
	uri_list->_maximum = length;
	uri_list->_length = length;
	uri_list->_buffer = CORBA_sequence_Nautilus_URI_allocbuf (length);
	for (i = 0; i < length; i++) {
		translated_uri = nautilus_make_uri_from_shell_arg (strv[i]);
		uri_list->_buffer[i] = CORBA_string_dup (translated_uri);
		g_free (translated_uri);
		translated_uri = NULL;
	}
	CORBA_sequence_set_release (uri_list, CORBA_TRUE);

	return uri_list;
}

void
nautilus_application_startup (NautilusApplication *application,
			      gboolean kill_shell,
			      gboolean restart_shell,
			      gboolean start_desktop,
			      gboolean no_default_window,
			      gboolean do_first_time_druid_check,
			      const char *geometry,
			      const char *urls[])
{
	CORBA_Environment ev;
	Nautilus_Shell shell;
	OAF_RegistrationResult result;
	const char *message, *detailed_message;
	GnomeDialog *dialog;
	Nautilus_URIList *url_list;
	const CORBA_char *corba_geometry;
	int num_failures;

	num_failures = 0;

	/* Perform check for nautilus being run as super user */
	if (!(kill_shell || restart_shell)) {
		if (!confirm_ok_to_run_as_root ()) {
			return;
		}
	}

	/* Check the user's ~/.nautilus directories and post warnings
	 * if there are problems.
	 */
	if (!check_required_directories (application)) {
		return;
	}

	/* Run the first time startup druid if needed. */
	if (do_first_time_druid_check && need_to_show_first_time_druid ()) {
		nautilus_first_time_druid_show (application, start_desktop, urls);
		return;
	}
	
	/* initialize the sound machinery */
	nautilus_sound_initialize ();
	
	CORBA_exception_init (&ev);

	/* Start up the factory. */
	while (TRUE) {
		/* Try to register the file manager view factory with OAF. */
		result = oaf_active_server_register
			(FACTORY_IID,
			 bonobo_object_corba_objref (BONOBO_OBJECT (application)));
		switch (result) {
		case OAF_REG_SUCCESS:
			/* We are registered with OAF and all is right with the world. */
		case OAF_REG_ALREADY_ACTIVE:
			/* Another copy of nautilus already is running and registered. */
			message = NULL;
			detailed_message = NULL;
			break;
		case OAF_REG_NOT_LISTED:
			/* Can't register myself due to trouble locating the
			 * Nautilus_Shell.oaf file. This has happened when you
			 * launch Nautilus with an LD_LIBRARY_PATH that
			 * doesn't include the directory containg the oaf
			 * library. It could also happen if the
			 * Nautilus_Shell.oaf file was not present for some
			 * reason. Sometimes killing oafd and gconfd fixes
			 * this problem but we don't exactly understand why,
			 * since neither of the above causes explain it.
			 */
			message = _("Nautilus can't be used now. "
				    "Running the command \"nautilus-clean.sh -x\""
				    " from the console may fix the problem. If not,"
				    " you can try rebooting the computer or"
				    " installing Nautilus again.");
			/* FIXME bugzilla.eazel.com 2536: The guesses and stuff here are lame. */
			detailed_message = _("Nautilus can't be used now. "
					     "Running the command \"nautilus-clean.sh -x\""
					     " from the console may fix the problem. If not,"
					     " you can try rebooting the computer or"
					     " installing Nautilus again.\n\n"
					     "OAF couldn't locate the Nautilus_shell.oaf file. "
					     "One cause of this seems to be an LD_LIBRARY_PATH "
					     "that does not include the oaf library's directory. "
					     "Another possible cause would be bad install "
					     "with a missing Nautilus_Shell.oaf file.\n\n"
					     "Running \"nautilus-clean.sh -x\" will kill all "
					     "OAF and GConf processes, which may be needed by "
					     "other applications.\n\n"
					     "Sometimes killing oafd and gconfd fixes "
					     "the problem, but we don't know why.\n\n"
					     "We have also seen this error when a faulty "
					     "version of oaf was installed.");
			break;
		default:
			/* This should never happen. */
			g_warning ("bad error code from oaf_active_server_register");
		case OAF_REG_ERROR:
			/* Some misc. error (can never happen with current
			 * version of OAF). Show dialog and terminate the
			 * program.
			 */
			/* FIXME bugzilla.eazel.com 2537: Looks like this does happen with the
			 * current OAF. I guess I read the code
			 * wrong. Need to figure out when and make a
			 * good message.
			 */
			message = _("Nautilus can't be used now, due to an unexpected error.");
			detailed_message = _("Nautilus can't be used now, due to an unexpected error "
					     "from OAF when attempting to register the file manager view server.");
			break;
		}

		/* Get the shell object. */
		if (message == NULL) {
			shell = oaf_activate_from_id (SHELL_IID, 0, NULL, NULL);
			if (!CORBA_Object_is_nil (shell, &ev)) {
				break;
			}

			/* If we couldn't find ourselves it's a bad problem so
			 * we better stop looping.
			 */
			if (result == OAF_REG_SUCCESS) {
				/* FIXME bugzilla.eazel.com 2538: When can this happen? */
				message = _("Nautilus can't be used now, due to an unexpected error.");
				detailed_message = _("Nautilus can't be used now, due to an unexpected error "
						     "from OAF when attempting to locate the factory."
						     "Killing oafd and restarting Nautilus may help fix the problem.");
			} else {
				num_failures++;
				if (num_failures > 20) {
					message = _("Nautilus can't be used now, due to an unexpected error.");
					detailed_message = _("Nautilus can't be used now, due to an unexpected error "
							     "from OAF when attempting to locate the shell object. "
							     "Killing oafd and restarting Nautilus may help fix the problem.");
					
				}
			}
		}

		if (message != NULL) {
			dialog = nautilus_show_error_dialog_with_details (message, NULL, detailed_message, NULL);
			/* We need the main event loop so the user has a chance to see the dialog. */
			nautilus_main_event_loop_register (GTK_OBJECT (dialog));
			goto out;
		}
	}

	if (kill_shell) {
		Nautilus_Shell_quit (shell, &ev);
	} else if (restart_shell) {
		Nautilus_Shell_restart (shell, &ev);
	} else {
		if (start_desktop) {
			Nautilus_Shell_start_desktop (shell, &ev);
		}
		
		/* Monitor the preference to show or hide the desktop */
		nautilus_preferences_add_callback_while_alive (NAUTILUS_PREFERENCES_SHOW_DESKTOP,
							       desktop_changed_callback,
							       application,
							       GTK_OBJECT (application));

		/* CORBA C mapping doesn't allow NULL to be passed
		   for string parameters */
		corba_geometry = (geometry != NULL) ? geometry : "";

	  	/* Create the other windows. */
		if (urls != NULL) {
			url_list = nautilus_make_uri_list_from_shell_strv (urls);
			Nautilus_Shell_open_windows (shell, url_list, corba_geometry, &ev);
			CORBA_free (url_list);
		} else if (!no_default_window) {
			Nautilus_Shell_open_default_window (shell, corba_geometry, &ev);
		}
		
		/* Add ourselves to the session */
		init_session ();
	}

	/* We're done with the shell now, so let it go. */
	/* HACK: Don't bother releasing the shell in the case where we
	 * just told it to quit -- that just leads to hangs and does
	 * no good. We could probably fix this in some fancier way if
	 * we could figure out a better lifetime rule.
	 */
	if (!(kill_shell || restart_shell)) {
		bonobo_object_release_unref (shell, NULL);
	}

 out:
	CORBA_exception_free (&ev);
}

static void
nautilus_application_create_desktop_window (NautilusApplication *application)
{
	g_return_if_fail (nautilus_application_desktop_window == NULL);
	g_return_if_fail (NAUTILUS_IS_APPLICATION (application));

	nautilus_application_desktop_window = nautilus_desktop_window_new (application);
	gtk_widget_show (GTK_WIDGET (nautilus_application_desktop_window));
}

void
nautilus_application_open_desktop (NautilusApplication *application)
{
	if (nautilus_application_desktop_window == NULL) {
		nautilus_application_create_desktop_window (application);
	}
}

void
nautilus_application_close_desktop (void)
{
	if (nautilus_application_desktop_window != NULL) {
		gtk_widget_destroy (GTK_WIDGET (nautilus_application_desktop_window));
		nautilus_application_desktop_window = NULL;
	}	
}

void
nautilus_application_close_all_windows (void)
{
	while (nautilus_application_window_list != NULL) {
		nautilus_window_close (NAUTILUS_WINDOW (nautilus_application_window_list->data));
	}
}

static void
nautilus_application_destroyed_window (GtkObject *object, NautilusApplication *application)
{
	nautilus_application_window_list = g_list_remove (nautilus_application_window_list, object);
}

static gboolean
nautilus_window_delete_event_callback (GtkWidget *widget,
				       GdkEvent *event,
				       gpointer user_data)
{
	NautilusWindow *window;

	window = NAUTILUS_WINDOW (widget);
	nautilus_window_close (window);

	return TRUE;
}				       

NautilusWindow *
nautilus_application_create_window (NautilusApplication *application)
{
	NautilusWindow *window;

	g_return_val_if_fail (NAUTILUS_IS_APPLICATION (application), NULL);
	
	window = NAUTILUS_WINDOW (gtk_widget_new (nautilus_window_get_type (),
						  "app", GTK_OBJECT (application),
						  "app_id", "nautilus", NULL));
	
	gtk_signal_connect (GTK_OBJECT (window), 
			    "delete_event", GTK_SIGNAL_FUNC (nautilus_window_delete_event_callback),
                    	    NULL);

	gtk_signal_connect (GTK_OBJECT (window),
			    "destroy", nautilus_application_destroyed_window,
			    application);

	nautilus_application_window_list = g_list_prepend (nautilus_application_window_list, window);

	/* Do not yet show the window. It will be shown later on if it can
	 * successfully display its initial URI. Otherwise it will be destroyed
	 * without ever having seen the light of day.
	 */

	return window;
}

/*
 * confirm_ok_to_run_as_root:
 *
 * Puts out a warning if the user is running nautilus as root.
 */
static gboolean
confirm_ok_to_run_as_root (void)
{
	GtkWidget *dialog;
	int result;

	if (geteuid () != 0) {
		return TRUE;
	}

	if (g_getenv ("NAUTILUS_OK_TO_RUN_AS_ROOT") != NULL) {
		return TRUE;
	}

	dialog = gnome_message_box_new
		(_("You are about to run Nautilus as root.\n\n"
		   "As root, you can damage your system if you are not careful, and\n"
		   "Nautilus will not stop you from doing it."),
		 GNOME_MESSAGE_BOX_WARNING,
		 GNOME_STOCK_BUTTON_OK, _("Quit"), NULL);
	result = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	
	return result == 0;
}

/* callback for showing or hiding the desktop based on the user's preference */
static void
desktop_changed_callback (gpointer user_data)
{
	NautilusApplication *application;
	
	application = NAUTILUS_APPLICATION (user_data);
	if ( nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_DESKTOP)) {
		nautilus_application_open_desktop (application);
	} else {
		nautilus_application_close_desktop ();
	}
}

/*
 * need_to_show_first_time_druid
 *
 * Determine whether Nautilus needs to show the first time druid.
 * 
 * Note that the flag file indicating whether the druid has been
 * presented is: ~/.nautilus/first-time-flag.
 *
 * Another alternative could be to use preferences to store this flag
 * However, there because of bug 1229 this is not yet possible.
 *
 * Also, for debugging purposes, it is convenient to have just one file
 * to kill in order to test the startup druid:
 *
 * rm -f ~/.nautilus/first-time-flag
 *
 * In order to accomplish the same thing with preferences, you would have
 * to either kill ALL your preferences or spend time digging in ~/.gconf
 * xml files finding the right one.
 */
static gboolean
need_to_show_first_time_druid (void)
{
	gboolean result;
	char *user_directory;
	char *druid_flag_file_name;
	
	user_directory = nautilus_get_user_directory ();

	druid_flag_file_name = g_strdup_printf ("%s/%s",
						user_directory,
						"first-time-flag");

	result = !g_file_exists (druid_flag_file_name);	
	g_free (druid_flag_file_name);

	/* we changed the name of the flag for version 1.0, so we should
	 * check for and delete the old one, if the new one didn't exist 
	 */
	if (result) {
		druid_flag_file_name = g_strdup_printf ("file://%s/%s",
						user_directory,
						"first-time-wizard-flag");
		gnome_vfs_unlink (druid_flag_file_name);
		g_free (druid_flag_file_name);
	}
	g_free (user_directory); 
	return result;
}

/* Called whenever a volume is mounted.
 * It would also be cool to restore open windows and
 * position info saved when the volume was unmounted.
 */
static void
volume_mounted_callback (NautilusVolumeMonitor *monitor, NautilusVolume *volume,
			 NautilusApplication *application)
{
}

static gboolean
window_can_be_closed (NautilusWindow *window)
{
	if (!NAUTILUS_IS_DESKTOP_WINDOW (window)) {
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
is_last_closable_window (NautilusWindow *window)
{
	GList *node, *window_list;
	
	window_list = nautilus_application_get_window_list ();
	
	for (node = window_list; node != NULL; node = node->next) {
		if (window != NAUTILUS_WINDOW (node->data) && window_can_be_closed (NAUTILUS_WINDOW (node->data))) {
			return FALSE;
		}
	}
	
	return TRUE;
}


/* Called whenever a volume is unmounted. Check and see if there are any windows open
 * displaying contents on the volume. If there are, close them.
 * It would also be cool to save open window and position info.
 */
static void
volume_unmounted_callback (NautilusVolumeMonitor *monitor, NautilusVolume *volume,
			   NautilusApplication *application)
{
	GList *window_list, *node, *close_list;
	NautilusWindow *window;
	char *uri;
	char *path;
		
	close_list = NULL;
	
	/* Check and see if any of the open windows are displaying contents from the unmounted volume */
	window_list = nautilus_application_get_window_list ();
	
	/* Construct a list of windows to be closed. Do not add the non-closable windows to the list. */
	for (node = window_list; node != NULL; node = node->next) {
		window = NAUTILUS_WINDOW (node->data);
		if (window != NULL && window_can_be_closed (window)) {
			uri = nautilus_window_get_location (window);
			path = gnome_vfs_get_local_path_from_uri (uri);
			if (nautilus_str_has_prefix (path, volume->mount_path)) {
				close_list = g_list_prepend (close_list, window);
			}
			g_free (path);
			g_free (uri);
		}
	}
		
	/* Handle the windows in the close list. */
	for (node = close_list; node != NULL; node = node->next) {
		window = NAUTILUS_WINDOW (node->data);
		if (is_last_closable_window (window)) {
			/* Don't close the last or only window. Try to redirect to the default home directory. */		 	
			nautilus_window_go_home (window);
		} else {
			nautilus_window_close (window);
		}
	}
		
	g_list_free (close_list);
}


static void
removed_from_session (GnomeClient *client, gpointer data)
{
	nautilus_main_event_loop_quit ();
}

static gint
save_session (GnomeClient *client, gint phase, GnomeSaveStyle save_style, gint shutdown,
	      GnomeInteractStyle interact_style, gint fast, gpointer data)
{
	return TRUE;
}

static void
set_session_restart (GnomeClient *client, gboolean restart)
{
	static char *restart_argv[] = { "nautilus", "--no-default-window", 0 };

	gnome_client_set_restart_command (client, 2, restart_argv);

	gnome_client_set_priority (client, 40);
	
	if (g_getenv ("NAUTILUS_DEBUG") != NULL) {
		/* Don't respawn in debug mode */
		gnome_client_set_restart_style (client, GNOME_RESTART_NEVER);
	} else {
		gnome_client_set_restart_style (client, (restart ? GNOME_RESTART_IMMEDIATELY : GNOME_RESTART_NEVER));
	}
}

static void
init_session (void)
{
	GnomeClient *client;

	client = gnome_master_client ();

	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
			   (GtkSignalFunc) save_session,
			    NULL);

	gtk_signal_connect (GTK_OBJECT (client), "die",
			    (GtkSignalFunc) removed_from_session,
			    NULL);
			    			    			    	
	set_session_restart (client, nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_ADD_TO_SESSION));
}

