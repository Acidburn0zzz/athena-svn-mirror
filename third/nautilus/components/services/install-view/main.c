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
 * Author: Maciej Stachowiak <mjs@eazel.com>
 *         J Shane Culpepper <pepper@eazel.com>
 */

/* main.c - main function and object activation function for services
   content view component. */

#include <config.h>
#include <gnome.h>
#include <liboaf/liboaf.h>
#include <bonobo.h>
#include "nautilus-service-install-view.h"
#include <libtrilobite/libammonite.h>
#include <gconf/gconf.h>

static int object_count = 0;

static gboolean
quit_timer (void *unused)
{
	gtk_main_quit ();
	return FALSE;
}

static void
service_install_object_destroyed (GtkObject *obj)
{
	object_count--;
	if (object_count <= 0) {
		/* timing issues: let the install view handle its own shutdown before
		 * pulling the plug...
		 */
		gtk_timeout_add (5, quit_timer, NULL);
	}
}

static BonoboObject *
service_install_make_object (BonoboGenericFactory	*factory, 
	  		     const char			*iid,
			     void			*closure)
{

	NautilusServiceInstallView	*view;
	NautilusView			*nautilus_view;

	if (strcmp (iid, "OAFIID:nautilus_service_install_view:886546ca-1115-4ea4-8d30-8cefa2f5070b")) {
		return NULL;
	}

	view = NAUTILUS_SERVICE_INSTALL_VIEW (gtk_object_new (NAUTILUS_TYPE_SERVICE_INSTALL_VIEW, NULL));

	object_count++;

	nautilus_view = nautilus_service_install_view_get_nautilus_view (view);
	
	gtk_signal_connect (GTK_OBJECT (nautilus_view), "destroy", service_install_object_destroyed, NULL);

	return BONOBO_OBJECT (nautilus_view);
}

int
main (int argc, char *argv[])
{
	BonoboGenericFactory *factory;
	CORBA_ORB orb;
	char *registration_id;
	
	/* Disable session manager connection */
	gnome_client_disable_master_connection ();

	gnomelib_register_popt_table (oaf_popt_options, oaf_get_popt_table_name ());
	orb = oaf_init (argc, argv);

	gnome_init ("nautilus-service-install-view", VERSION, 
		    argc, argv);
	gdk_rgb_init ();

	bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL);
	gconf_init (argc, argv, NULL);

	ammonite_init (bonobo_poa());

	/* log to stderr, for now (but leave debug mode off) */
	trilobite_set_log_handler (stderr, G_LOG_DOMAIN);

        registration_id = oaf_make_registration_id ("OAFIID:nautilus_service_install_view_factory:e59e53d1-e3d1-46fe-ae28-3ec5c56b7d32", getenv ("DISPLAY"));
	factory = bonobo_generic_factory_new_multi (registration_id, 
						    service_install_make_object,
						    NULL);
	g_free (registration_id);

	do {
		bonobo_main ();
	} while (object_count > 0);

	return 0;
}
