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
 * Nautilus is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* nautilus-shell.h: Server side of the Nautilus:Shell CORBA object
 * that represents the shell across processes.
 */

#include <config.h>
#include "nautilus-shell.h"

#include "nautilus-desktop-window.h"
#include "nautilus-main.h"
#include "nautilus-shell-interface.h"
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libnautilus-extensions/nautilus-file-utilities.h>
#include <libnautilus-extensions/nautilus-glib-extensions.h>
#include <libnautilus-extensions/nautilus-gtk-extensions.h>
#include <libnautilus-extensions/nautilus-gtk-macros.h>
#include <libnautilus-extensions/nautilus-label.h>
#include <libnautilus-extensions/nautilus-preferences.h>
#include <libnautilus-extensions/nautilus-stock-dialogs.h>
#include <libnautilus-extensions/nautilus-string.h>
#include <libnautilus/nautilus-bonobo-workarounds.h>
#include <stdlib.h>

/* turn this on to enable the caveat dialog */
#if 0
#define SHOW_CAVEAT
#endif

/* Keep window from shrinking down ridiculously small; numbers are somewhat arbitrary */
#define APPLICATION_WINDOW_MIN_WIDTH	300
#define APPLICATION_WINDOW_MIN_HEIGHT	100

#define START_STATE_CONFIG "start-state"

struct NautilusShellDetails {
	NautilusApplication *application;
};

static void     nautilus_shell_initialize       (NautilusShell          *shell);
static void     nautilus_shell_initialize_class (NautilusShellClass     *klass);
static void     destroy                         (GtkObject              *shell);
static void     corba_open_windows              (PortableServer_Servant  servant,
						 const Nautilus_URIList *list,
						 const CORBA_char       *geometry,
						 CORBA_Environment      *ev);
static void     corba_open_default_window       (PortableServer_Servant  servant,
						 const CORBA_char       *geometry,
						 CORBA_Environment      *ev);
static void     corba_start_desktop             (PortableServer_Servant  servant,
						 CORBA_Environment      *ev);
static void     corba_stop_desktop              (PortableServer_Servant  servant,
						 CORBA_Environment      *ev);
static void     corba_quit                      (PortableServer_Servant  servant,
						 CORBA_Environment      *ev);
static void     corba_restart                   (PortableServer_Servant  servant,
						 CORBA_Environment      *ev);
static gboolean restore_window_states           (NautilusShell          *shell);

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusShell,
				   nautilus_shell,
				   BONOBO_OBJECT_TYPE)

static void
nautilus_shell_initialize_class (NautilusShellClass *klass)
{
	GTK_OBJECT_CLASS (klass)->destroy = destroy;
}

static POA_Nautilus_Shell__epv *
nautilus_shell_get_epv (void)
{
	static POA_Nautilus_Shell__epv epv;

	epv.open_windows = corba_open_windows;
	epv.open_default_window = corba_open_default_window;
	epv.start_desktop = corba_start_desktop;
	epv.stop_desktop = corba_stop_desktop;
	epv.quit = corba_quit;
	epv.restart = corba_restart;

	return &epv;
}

static POA_Nautilus_Shell__vepv *
nautilus_shell_get_vepv (void)
{
	static POA_Nautilus_Shell__vepv vepv;

	vepv.Bonobo_Unknown_epv = nautilus_bonobo_object_get_epv ();
	vepv.Nautilus_Shell_epv = nautilus_shell_get_epv ();

	return &vepv;
}

static POA_Nautilus_Shell *
nautilus_shell_create_servant (void)
{
	POA_Nautilus_Shell *servant;
	CORBA_Environment ev;

	servant = (POA_Nautilus_Shell *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = nautilus_shell_get_vepv ();
	CORBA_exception_init (&ev);
	POA_Nautilus_Shell__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_error ("can't initialize Nautilus shell");
	}
	CORBA_exception_free (&ev);

	return servant;
}

static void
nautilus_shell_initialize (NautilusShell *shell)
{
	Nautilus_Shell corba_shell;

	shell->details = g_new0 (NautilusShellDetails, 1);

	corba_shell = bonobo_object_activate_servant
		(BONOBO_OBJECT (shell), nautilus_shell_create_servant ());
	bonobo_object_construct (BONOBO_OBJECT (shell), corba_shell);
}

static void
destroy (GtkObject *object)
{
	NautilusShell *shell;

	shell = NAUTILUS_SHELL (object);
	g_free (shell->details);

	NAUTILUS_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

NautilusShell *
nautilus_shell_new (NautilusApplication *application)
{
	NautilusShell *shell;

	shell = NAUTILUS_SHELL (gtk_object_new (NAUTILUS_TYPE_SHELL, NULL));
	shell->details->application = application;
	return shell;
}

#ifdef SHOW_CAVEAT

static void
display_caveat (GtkWindow *parent_window)
{
	GtkWidget *dialog;
	GtkWidget *frame;
	GtkWidget *pixmap;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *text;
	char *file_name;

	dialog = gnome_dialog_new (_("Caveat"),
				   GNOME_STOCK_BUTTON_OK,
				   NULL);
  	gtk_container_set_border_width (GTK_CONTAINER (dialog), GNOME_PAD);
  	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "caveat", "Nautilus");

  	hbox = gtk_hbox_new (FALSE, GNOME_PAD);
  	gtk_container_set_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
  	gtk_widget_show (hbox);
  	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), 
  			    hbox,
  			    FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	file_name = nautilus_pixmap_file ("About_Image.png");
	if (file_name != NULL) {
		pixmap = gnome_pixmap_new_from_file (file_name);
		g_free (file_name);

		if (pixmap != NULL) {
			frame = gtk_frame_new (NULL);
			gtk_widget_show (frame);
			gtk_frame_set_shadow_type (GTK_FRAME (frame),
						   GTK_SHADOW_IN);
			gtk_box_pack_start (GTK_BOX (vbox), frame,
					    FALSE, FALSE, 0);

			gtk_widget_show (pixmap);
			gtk_container_add (GTK_CONTAINER (frame), pixmap);
		}
	}

  	text = nautilus_label_new
		(_("Thank you for your interest in Nautilus.\n "
		   "\n"
		   "As with any software under development, you should exercise caution when "
		   "using Nautilus.  Eazel does not provide any guarantee that it will work "
		   "properly, or assume any liability for your use of it.  Please use it at your "
		   "own risk.\n"
		   "\n"
		   "Please visit http://www.eazel.com/feedback.html to provide feedback, "
		   "comments, and suggestions."));
	nautilus_label_make_larger (NAUTILUS_LABEL (text), 1);
	nautilus_label_set_justify (NAUTILUS_LABEL (text), GTK_JUSTIFY_LEFT);
	nautilus_label_set_wrap (NAUTILUS_LABEL (text), TRUE);
	gtk_widget_show (text);
  	gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);

  	gnome_dialog_set_close (GNOME_DIALOG (dialog), TRUE);
	gnome_dialog_set_parent (GNOME_DIALOG (dialog), parent_window);

	gtk_widget_show (GTK_WIDGET (dialog));
}

#endif /* SHOW_CAVEAT */

static void
display_caveat_first_time (NautilusShell *shell, NautilusWindow *window)
{
#ifdef SHOW_CAVEAT
	static gboolean showed_caveat;

	/* Show the "not ready for prime time" dialog after the first
	 * window appears, so it's on top.
	 */
	/* FIXME bugzilla.eazel.com 1256: It's not on top of the
         * windows other than the first one.
	 */
	if (!showed_caveat
	    && g_getenv ("NAUTILUS_NO_CAVEAT_DIALOG") == NULL) {
		gtk_signal_connect (GTK_OBJECT (window), "show",
				    display_caveat, window);
	}
	showed_caveat = TRUE;
#endif /* SHOW_CAVEAT */
}

static void
open_window (NautilusShell *shell, const char *uri, const char *geometry)
{
	NautilusWindow *window;

	window = nautilus_application_create_window (shell->details->application);

	if (geometry != NULL) {
		nautilus_gtk_window_set_initial_geometry_from_string (GTK_WINDOW (window),
								      geometry,
								      APPLICATION_WINDOW_MIN_WIDTH,
								      APPLICATION_WINDOW_MIN_HEIGHT);
	}

	if (uri == NULL) {
		nautilus_window_go_home (window);
	} else {
		nautilus_window_go_to (window, uri);
	}
	display_caveat_first_time (shell, window);
}

static void
corba_open_windows (PortableServer_Servant servant,
		    const Nautilus_URIList *list,
		    const CORBA_char *geometry,
		    CORBA_Environment *ev)
{
	NautilusShell *shell;
	guint i;

	shell = NAUTILUS_SHELL (((BonoboObjectServant *) servant)->bonobo_object);

	/* Open windows at each requested location. */
	for (i = 0; i < list->_length; i++) {
		g_assert (list->_buffer[i] != NULL);
		open_window (shell, list->_buffer[i], geometry);
	}
}

static void
corba_open_default_window (PortableServer_Servant servant,
			   const CORBA_char *geometry,
			   CORBA_Environment *ev)
{
	NautilusShell *shell;

	shell = NAUTILUS_SHELL (((BonoboObjectServant *) servant)->bonobo_object);

	if (!restore_window_states (shell)) {
		/* Open a window pointing at the default location. */
		open_window (shell, NULL, geometry);
	}
}

static void
corba_start_desktop (PortableServer_Servant servant,
		      CORBA_Environment *ev)
{
	NautilusShell	      *shell;
	NautilusApplication   *application;

	shell	    = NAUTILUS_SHELL (((BonoboObjectServant *) servant)->bonobo_object);
	application = NAUTILUS_APPLICATION (shell->details->application);
	
	nautilus_application_open_desktop (application);
}

static void
corba_stop_desktop (PortableServer_Servant servant,
		    CORBA_Environment *ev)
{	
	nautilus_application_close_desktop ();
}

static void
corba_quit (PortableServer_Servant servant,
	    CORBA_Environment *ev)
{
	nautilus_main_event_loop_quit ();
}

/*
 * code for saving the state of nautilus windows across a restart
 *
 * for now, only the window geometry & uri is saved, into "start-state",
 * in a list of strings like:
 *     "90x90+1+1 uri"
 */

static void
save_window_states (void)
{
	GList *windows, *iter;
	GList *out;
	NautilusWindow *window;
	GdkWindow *gdk_window;
	char *window_info;
	int x, y, width, height;
	char *location;

	out = NULL;
	windows = nautilus_application_get_window_list ();
	for (iter = windows; iter; iter = g_list_next (iter)) {
		window = (NautilusWindow *) (iter->data);

		width = GTK_WIDGET (window)->allocation.width;
		height = GTK_WIDGET (window)->allocation.height;

		/* need root origin (origin of all the window dressing) */
		gdk_window = GTK_WIDGET (window)->window;
		gdk_window_get_root_origin (gdk_window, &x, &y);

		location = nautilus_window_get_location (window);

		/* FIXME bugzilla.eazel.com 4375
		   This hardcoded subst should be parameterized
		   at some point. This ensures that when eazel-install:nautilus
		   restarts nautilus, it doesn't go to eazel-install:nautilus but
		   to eazel: instead */
		if (nautilus_istr_has_prefix (location, "eazel-install:")) {
			g_free (location);
			location = g_strdup ("eazel:");
		}

		window_info = g_strdup_printf ("%dx%d+%d+%d %s", 
					       width, height, 
					       x, y, 
					       location);

		g_free (location);

		out = g_list_prepend (out, window_info);
	}

	nautilus_preferences_set_string_list (START_STATE_CONFIG, out);
	nautilus_g_list_free_deep (out);
}

/* returns TRUE if there was state info which has been used to create new windows */
static gboolean
restore_window_states (NautilusShell *shell)
{
	GList *start_state, *iter;
	NautilusWindow *window;
	char *window_info, *p, *uri;
	int x, y, width, height;

	start_state = nautilus_preferences_get_string_list (START_STATE_CONFIG);
	if (! start_state) {
		return FALSE;
	}

	for (iter = start_state; iter; iter = iter->next) {
		p = window_info = (char *) (iter->data);

		width = strtol (p, &p, 10);
		if (*p == 'x') {
			p++;
		}
		height = strtol (p, &p, 10);
		if (*p == '+') {
			p++;
		}
		x = strtol (p, &p, 10);
		if (*p == '+') {
			p++;
		}
		y = strtol (p, &p, 10);
		p = strchr (p, ' ');
		if (p) {
			uri = p+1;
		} else {
			uri = NULL;
		}

		window = nautilus_application_create_window (shell->details->application);
		if (uri == NULL) {
			nautilus_window_go_home (window);
		} else {
			nautilus_window_go_to (window, uri);
		}

		gtk_widget_set_uposition (GTK_WIDGET (window), x, y);
		gtk_widget_set_usize (GTK_WIDGET (window), width, height);
		display_caveat_first_time (shell, window);
	}
	nautilus_g_list_free_deep (start_state);
	nautilus_preferences_set_string_list (START_STATE_CONFIG, NULL);
	return TRUE;
}

static void
corba_restart (PortableServer_Servant servant,
	       CORBA_Environment *ev)
{
	save_window_states ();

	nautilus_main_event_loop_quit ();
	nautilus_setenv ("_NAUTILUS_RESTART", "yes", 1);
}
