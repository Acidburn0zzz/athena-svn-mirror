/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * Copyright (C) 2000 Eazel, Inc
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
 * Author: Andy Hertzfeld
 */

/* main.c - main function and object activation function for the throbber component. */

#include <config.h>
#include "nautilus-throbber.h"

#include <bonobo.h>
#include <gnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libnautilus-extensions/nautilus-debug.h>
#include <libnautilus-extensions/nautilus-global-preferences.h>
#include <liboaf/liboaf.h>

static int object_count = 0;

static void
throbber_object_destroyed(GtkObject *obj)
{
	object_count--;
	if (object_count <= 0) {
		gtk_main_quit ();
	}
}

static BonoboObject *
throbber_make_object (BonoboGenericFactory *factory, 
			const char *iid, 
			void *closure)
{
	NautilusThrobber *throbber;
	BonoboObject *bonobo_control;

	if (strcmp (iid, "OAFIID:nautilus_throbber")) {
		return NULL;
	}
	
	throbber = NAUTILUS_THROBBER (gtk_object_new (NAUTILUS_TYPE_THROBBER, NULL));

	object_count++;

	bonobo_control = nautilus_throbber_get_control (throbber);

	gtk_signal_connect (GTK_OBJECT (bonobo_control), "destroy", throbber_object_destroyed, NULL);
	return bonobo_control;
}

int
main (int argc, char *argv[])
{
	BonoboGenericFactory *factory;
	CORBA_ORB orb;
	char *registration_id;

	/* Make criticals and warnings stop in the debugger if
	 * NAUTILUS_DEBUG is set. Unfortunately, this has to be done
	 * explicitly for each domain.
	 */
	if (g_getenv ("NAUTILUS_DEBUG") != NULL) {
		nautilus_make_warnings_and_criticals_stop_in_debugger
			(G_LOG_DOMAIN, g_log_domain_glib,
			 "Bonobo",
			 "Gdk",
			 "GnomeUI",
			 "GnomeVFS",
			 "GnomeVFS-CORBA",
			 "GnomeVFS-pthread",
			 "Gtk",
			 "Gdk-Pixbuf",
			 "Nautilus",
			 "Nautilus-Authenticate",
			 "Nautilus-Tree",
			 "ORBit",
			 NULL);
	}

	/* Disable session manager connection */
	gnome_client_disable_master_connection ();

	gnomelib_register_popt_table (oaf_popt_options, oaf_get_popt_table_name ());
	orb = oaf_init (argc, argv);

	gnome_init ("nautilus-throbber", VERSION, 
		    argc, argv); 

	gdk_rgb_init ();

	bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL);

	/* initialize gnome-vfs, etc */
	g_thread_init (NULL);
	gnome_vfs_init ();
	nautilus_global_preferences_initialize ();   
	
	registration_id = oaf_make_registration_id ("OAFIID:nautilus_throbber_factory", getenv ("DISPLAY"));
	factory = bonobo_generic_factory_new_multi (registration_id, 
						    throbber_make_object,
						    NULL);
	g_free (registration_id);

	
	do {
		bonobo_main ();
	} while (object_count > 0);

	gnome_vfs_shutdown ();	
	return 0;
}
