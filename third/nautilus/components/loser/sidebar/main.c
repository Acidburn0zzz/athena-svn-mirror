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
 * Author: Maciej Stachowiak
 */

/* main.c - main function and object activation function for loser
   sidebar view component that fails on command. */

#include <config.h>

#include "nautilus-sidebar-loser.h"

#include <gnome.h>
#include <bonobo-activation/bonobo-activation.h>
#include <eel/eel-gnome-extensions.h>
#include <bonobo.h>

#define FACTORY_IID "OAFIID:Nautilus_Sidebar_Loser_Factory"

static int object_count = 0;

static void
loser_object_destroyed(GtkObject *obj)
{
	object_count--;
	if (object_count <= 0) {
		gtk_main_quit ();
	}
}

static BonoboObject *
loser_make_object (BonoboGenericFactory *factory, 
		    const char *iid, 
		    void *closure)
{
	NautilusSidebarLoser *view;
	NautilusView *nautilus_view;

	nautilus_sidebar_loser_maybe_fail ("pre-make-object");

	if (strcmp (iid, "OAFIID:Nautilus_Sidebar_Loser")) {
		return NULL;
	}

	view = NAUTILUS_SIDEBAR_LOSER (g_object_new (NAUTILUS_TYPE_SIDEBAR_LOSER, NULL));

	object_count++;

	nautilus_view = nautilus_sidebar_loser_get_nautilus_view (view);

	g_signal_connect (nautilus_view, "destroy", G_CALLBACK (loser_object_destroyed), NULL);

	nautilus_sidebar_loser_maybe_fail ("post-make-object");

	return BONOBO_OBJECT (nautilus_view);
}

int main(int argc, char *argv[])
{
	BonoboGenericFactory *factory;
	CORBA_Environment ev;
	char *registration_id;

	CORBA_exception_init (&ev);

	nautilus_sidebar_loser_maybe_fail ("pre-init");

        bonobo_ui_init ("nautilus-sidebar-loser", VERSION, &argc, argv); 
	
	nautilus_sidebar_loser_maybe_fail ("post-init");

        registration_id = eel_bonobo_make_registration_id (FACTORY_IID);
	factory = bonobo_generic_factory_new (FACTORY_IID,
					      loser_make_object,
					      NULL);
	g_free (registration_id);

	nautilus_sidebar_loser_maybe_fail ("post-factory-init");

	if (factory != NULL) {
		do {
			bonobo_main ();
		} while (object_count > 0);
	}
	
	return 0;
}
