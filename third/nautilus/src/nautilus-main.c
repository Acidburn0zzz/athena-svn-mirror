/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * Copyright (C) 1999, 2000 Eazel, Inc.
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
 * Authors: Elliot Lee <sopwith@redhat.com>,
 *          Darin Adler <darin@bentspoon.com>,
 *          John Sullivan <sullivan@eazel.com>
 *
 */

/* nautilus-main.c: Implementation of the routines that drive program lifecycle and main window creation/destruction. */

#include <config.h>
#include "nautilus-main.h"

#include "nautilus-application.h"
#include "nautilus-self-check-functions.h"
#include "nautilus-window.h"
#include <bonobo-activation/bonobo-activation.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-ui-main.h>
#include <dlfcn.h>
#include <eel/eel-debug.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-self-checks.h>
#include <gdk/gdkx.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-init.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libnautilus-private/nautilus-directory-metafile.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-lib-self-check-functions.h>
#include <libxml/parser.h>
#include <popt.h>
#include <stdlib.h>
#include <unistd.h>

/* Keeps track of everyone who wants the main event loop kept active */
static GSList *event_loop_registrants;

static gboolean
is_event_loop_needed (void)
{
	return event_loop_registrants != NULL;
}

static int
quit_if_in_main_loop (gpointer callback_data)
{
	guint level;

	g_assert (callback_data == NULL);

	level = gtk_main_level ();

	/* We can be called even outside the main loop by gnome_vfs_shutdown,
	 * so check that we are in a loop before calling quit.
	 */
	if (level != 0) {
		gtk_main_quit ();
	}

	/* We need to be called again if we quit a nested loop. */
	return level > 1;
}

static void
eel_gtk_main_quit_all (void)
{
	/* Calling gtk_main_quit directly only kills the current/top event loop.
	 * This idler will be run by the current event loop, killing it, and then
	 * by the next event loop, ...
	 */
	gtk_idle_add (quit_if_in_main_loop, NULL);
}

static void
event_loop_unregister (GtkObject *object)
{
	event_loop_registrants = g_slist_remove (event_loop_registrants, object);
	if (!is_event_loop_needed ()) {
		eel_gtk_main_quit_all ();
	}
}

void
nautilus_main_event_loop_register (GtkObject *object)
{
	g_signal_connect (object, "destroy", G_CALLBACK (event_loop_unregister), NULL);
	event_loop_registrants = g_slist_prepend (event_loop_registrants, object);
}

gboolean
nautilus_main_is_event_loop_mainstay (GtkObject *object)
{
	return g_slist_length (event_loop_registrants) == 1
		&& event_loop_registrants->data == object;
}

void
nautilus_main_event_loop_quit (void)
{
	while (event_loop_registrants != NULL) {
		gtk_object_destroy (event_loop_registrants->data);
	}
}

int
main (int argc, char *argv[])
{
	gboolean kill_shell;
	gboolean restart_shell;
	gboolean no_default_window;
	gboolean no_desktop;
	char *geometry;
	gboolean perform_self_check;
	poptContext popt_context;
	const char **args;
	NautilusApplication *application;
	char **argv_copy;
	GnomeProgram *program;
	GValue context_as_value = { 0 };

	struct poptOption options[] = {
#ifndef NAUTILUS_OMIT_SELF_CHECK
		{ "check", 'c', POPT_ARG_NONE, &perform_self_check, 0,
		  N_("Perform a quick set of self-check tests."), NULL },
#endif
		{ "geometry", 'g', POPT_ARG_STRING, &geometry, 0,
		  N_("Create the initial window with the given geometry."), N_("GEOMETRY") },
		{ "no-default-window", 'n', POPT_ARG_NONE, &no_default_window, 0,
		  N_("Only create windows for explicitly specified URIs."), NULL },
		{ "no-desktop", '\0', POPT_ARG_NONE, &no_desktop, 0,
		  N_("Do not manage the desktop (ignore the preference set in the preferences dialog)."), NULL },
		{ "quit", 'q', POPT_ARG_NONE, &kill_shell, 0,
		  N_("Quit Nautilus."), NULL },
		{ "restart", '\0', POPT_ARG_NONE | POPT_ARGFLAG_DOC_HIDDEN, &restart_shell, 0,
		  N_("Restart Nautilus."), NULL },
		{ NULL, '\0', 0, NULL, 0, NULL, NULL }
	};

	if (g_getenv ("NAUTILUS_DEBUG") != NULL) {
		eel_make_warnings_and_criticals_stop_in_debugger ();
	}
	
	/* Initialize gettext support */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Get parameters. */
	geometry = NULL;
	kill_shell = FALSE;
	no_default_window = FALSE;
	no_desktop = FALSE;
	perform_self_check = FALSE;
	restart_shell = FALSE;

	program = gnome_program_init ("nautilus", VERSION,
				      LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PROGRAM_STANDARD_PROPERTIES,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_HUMAN_READABLE_NAME, _("Nautilus"),
				      NULL);

	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT,
			       g_value_init (&context_as_value, G_TYPE_POINTER));

	popt_context = g_value_get_pointer (&context_as_value);

	/* Check for argument consistency. */
	args = poptGetArgs (popt_context);
	if (perform_self_check && args != NULL) {
		fprintf (stderr, _("nautilus: --check cannot be used with URIs.\n"));
		return EXIT_FAILURE;
	}
	if (perform_self_check && (kill_shell || restart_shell)) {
		fprintf (stderr, _("nautilus: --check cannot be used with other options.\n"));
		return EXIT_FAILURE;
	}
	if (kill_shell && args != NULL) {
		fprintf (stderr, _("nautilus: --quit cannot be used with URIs.\n"));
		return EXIT_FAILURE;
	}
	if (restart_shell && args != NULL) {
		fprintf (stderr, _("nautilus: --restart cannot be used with URIs.\n"));
		return EXIT_FAILURE;
	}
	if (geometry != NULL && args != NULL && args[0] != NULL && args[1] != NULL) {
		fprintf (stderr, _("nautilus: --geometry cannot be used with more than one URI.\n"));
		return EXIT_FAILURE;
	}

	/* Initialize the services that we use. */
	LIBXML_TEST_VERSION

	if (g_getenv ("NAUTILUS_ENABLE_TEST_COMPONENTS") != NULL) {
		bonobo_activation_set_test_components_enabled (TRUE);
	}

	/* Initialize preferences. This is needed so that proper 
	 * defaults are available before any preference peeking 
	 * happens.
	 */
	nautilus_global_preferences_init_with_folder_browsing ();
	if (no_desktop) {
		eel_preferences_set_is_invisible
			(NAUTILUS_PREFERENCES_SHOW_DESKTOP, TRUE);
		eel_preferences_set_is_invisible
			(NAUTILUS_PREFERENCES_DESKTOP_IS_HOME_DIR, TRUE);
	}
	
	bonobo_activate (); /* do now since we need it before main loop */

	/* Do either the self-check or the real work. */
	if (perform_self_check) {
#ifndef NAUTILUS_OMIT_SELF_CHECK
		/* Run the checks (each twice) for nautilus and libnautilus-private. */

		nautilus_directory_use_self_contained_metafile_factory ();

		nautilus_run_self_checks ();
		nautilus_run_lib_self_checks ();
		eel_exit_if_self_checks_failed ();

		nautilus_run_self_checks ();
		nautilus_run_lib_self_checks ();
		eel_exit_if_self_checks_failed ();
#endif
	} else {
		/* Run the nautilus application. */
		application = nautilus_application_new ();
		nautilus_application_startup
			(application,
			 kill_shell, restart_shell, no_default_window, no_desktop,
			 !(kill_shell || restart_shell),
			 geometry,
			 args);
		if (is_event_loop_needed ()) {
			gtk_main ();
		}
		bonobo_object_unref (application);
	}

	poptFreeContext (popt_context);

	gnome_vfs_shutdown ();
	eel_debug_shut_down ();
	bonobo_ui_debug_shutdown ();

	/* If told to restart, exec() myself again. This is used when
	 * the program is told to restart with CORBA, for example when
	 * an update takes place.
	 */

	if (g_getenv ("_NAUTILUS_RESTART") != NULL) {
		eel_unsetenv ("_NAUTILUS_RESTART");
		
		/* Might eventually want to copy all the parameters
		 * from argv into the new exec. For now, though, that
		 * would just interfere with the re-creation of
		 * windows based on the window info stored in gconf,
		 * including whether the desktop was started.
		 */
		argv_copy = g_new0 (char *, 2);
		argv_copy[0] = argv[0];
		
		execvp (argv[0], argv_copy);
	}

	return EXIT_SUCCESS;
}
