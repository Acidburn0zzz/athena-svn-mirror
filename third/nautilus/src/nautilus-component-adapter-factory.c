/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

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
 */

/* nautilus-component-adapter-factory.c - client wrapper for the
 * special adapter component, which wraps Bonobo components as
 * Nautilus Views and in the process keeps evil synchronous I/O out of
 * the Nautilus process itself.
 */

#include <config.h>
#include "nautilus-component-adapter-factory.h"

#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-object.h>
#include <eel/eel-debug.h>
#include <libgnome/gnome-macros.h>
#include <libnautilus-adapter/nautilus-adapter-factory.h>

#define NAUTILUS_COMPONENT_ADAPTER_FACTORY_IID "OAFIID:Nautilus_Adapter_Factory"

struct NautilusComponentAdapterFactoryDetails {
	Nautilus_ComponentAdapterFactory corba_factory;
};

static NautilusComponentAdapterFactory *global_component_adapter_factory = NULL;

GNOME_CLASS_BOILERPLATE (NautilusComponentAdapterFactory, nautilus_component_adapter_factory,
			 GtkObject, GTK_TYPE_OBJECT)

static void
activate_factory (NautilusComponentAdapterFactory *factory)
{
	factory->details->corba_factory = bonobo_get_object
		(NAUTILUS_COMPONENT_ADAPTER_FACTORY_IID,
		 "IDL:Nautilus/ComponentAdapterFactory:1.0", NULL);
}

static void
unref_factory (NautilusComponentAdapterFactory *factory)
{
	CORBA_Environment ev;
	
	CORBA_exception_init (&ev);
	Bonobo_Unknown_unref (factory->details->corba_factory, &ev);
	CORBA_exception_free (&ev);
}

static void
release_factory (NautilusComponentAdapterFactory *factory)
{
	CORBA_Object_release (factory->details->corba_factory, NULL);
	factory->details->corba_factory = CORBA_OBJECT_NIL;
}

static Nautilus_ComponentAdapterFactory
get_corba_factory (NautilusComponentAdapterFactory *factory)
{
	CORBA_Environment ev;
	Nautilus_ComponentAdapterFactory result;
	gboolean need_unref;

	CORBA_exception_init (&ev);

	need_unref = FALSE;
	if (CORBA_Object_is_nil (factory->details->corba_factory, &ev)
	    || BONOBO_EX (&ev)
	    || CORBA_Object_non_existent (factory->details->corba_factory, &ev)
	    || BONOBO_EX (&ev)) {
		release_factory (factory);
		activate_factory (factory);
		need_unref = TRUE;
	}
	
	CORBA_exception_free (&ev);

	result = bonobo_object_dup_ref (factory->details->corba_factory, NULL);

	if (need_unref) {
		unref_factory (factory);
	}

	return result;
}

static void
nautilus_component_adapter_factory_instance_init (NautilusComponentAdapterFactory *factory)
{
	factory->details = g_new0 (NautilusComponentAdapterFactoryDetails, 1);
}

static void
nautilus_component_adapter_factory_destroy (GtkObject *object)
{
	NautilusComponentAdapterFactory *factory;

	factory = NAUTILUS_COMPONENT_ADAPTER_FACTORY (object);

	release_factory (factory);
	g_free (factory->details);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
nautilus_component_adapter_factory_class_init  (NautilusComponentAdapterFactoryClass *klass)
{
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass *) klass;
	object_class->destroy = nautilus_component_adapter_factory_destroy;
}

static void
component_adapter_factory_at_exit_destructor (void)
{
	if (global_component_adapter_factory != NULL) {
		g_object_unref (global_component_adapter_factory);
	}
}

NautilusComponentAdapterFactory *
nautilus_component_adapter_factory_get (void)
{
	NautilusComponentAdapterFactory *factory;

	if (global_component_adapter_factory == NULL) {
		factory = NAUTILUS_COMPONENT_ADAPTER_FACTORY
			(g_object_new (NAUTILUS_TYPE_COMPONENT_ADAPTER_FACTORY, NULL));
		
		g_object_ref (factory);
		gtk_object_sink (GTK_OBJECT (factory));
		
		global_component_adapter_factory = factory;
		eel_debug_call_at_shutdown (component_adapter_factory_at_exit_destructor);
	}

	return global_component_adapter_factory;
}

Nautilus_View
nautilus_component_adapter_factory_create_adapter (NautilusComponentAdapterFactory *factory,
						   Bonobo_Unknown component)
{
	Nautilus_View nautilus_view;
	Bonobo_Control bonobo_control;
	Nautilus_ComponentAdapterFactory corba_factory;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	nautilus_view = Bonobo_Unknown_queryInterface
		(component, "IDL:Nautilus/View:1.0", &ev);
	if (BONOBO_EX (&ev)) {
		nautilus_view = CORBA_OBJECT_NIL;
	}

	if (nautilus_view != CORBA_OBJECT_NIL) {
		/* Object has the View interface, great! We might not
		 * need to adapt it.
		 */
		bonobo_control = Bonobo_Unknown_queryInterface
			(component, "IDL:Bonobo/Control:1.0", &ev);
		if (BONOBO_EX (&ev)) {
			bonobo_control = CORBA_OBJECT_NIL;
		}
		if (bonobo_control != CORBA_OBJECT_NIL) {
			/* It has the control interface too, so all is peachy. */
			bonobo_object_release_unref (bonobo_control, NULL);
		} else {
			/* No control interface; we have no way to
			 * support a View that doesn't also support
			 * the Control interface, so fail.
			 */
			bonobo_object_release_unref (nautilus_view, NULL);
			nautilus_view = CORBA_OBJECT_NIL;
		}
	} else {
		/* No View interface, we must adapt the object */
		corba_factory = get_corba_factory (factory);
		nautilus_view = Nautilus_ComponentAdapterFactory_create_adapter 
			(corba_factory, component, &ev);
		if (BONOBO_EX (&ev)) {
			nautilus_view = CORBA_OBJECT_NIL;
		}
		bonobo_object_release_unref (corba_factory, NULL);
	}

	CORBA_exception_free (&ev);

	return nautilus_view;
}
