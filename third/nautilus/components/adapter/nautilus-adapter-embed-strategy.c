/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 2000, 2001 Eazel, Inc.
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
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Maciej Stachowiak <mjs@eazel.com>
 */


/* nautilus-adapter-embed-strategy.h
 */


#include <config.h>
#include "nautilus-adapter-embed-strategy.h"

#include "nautilus-adapter-control-embed-strategy.h"
#include "nautilus-adapter-embed-strategy-private.h"
#include "nautilus-adapter-control-factory-embed-strategy.h"

#include <gtk/gtkobject.h>
#include <gtk/gtksignal.h>
#include <eel/eel-gtk-macros.h>
#include <stdio.h>

enum {
	ACTIVATE,
	DEACTIVATE,
	OPEN_LOCATION,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void nautilus_adapter_embed_strategy_class_init (NautilusAdapterEmbedStrategyClass *klass);
static void nautilus_adapter_embed_strategy_init       (NautilusAdapterEmbedStrategy      *strategy);

EEL_CLASS_BOILERPLATE (NautilusAdapterEmbedStrategy,
				   nautilus_adapter_embed_strategy,
				   GTK_TYPE_OBJECT)

EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (nautilus_adapter_embed_strategy, get_widget)
EEL_IMPLEMENT_MUST_OVERRIDE_SIGNAL (nautilus_adapter_embed_strategy, get_zoomable)

static void
nautilus_adapter_embed_strategy_class_init (NautilusAdapterEmbedStrategyClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;

	signals[ACTIVATE] =
		g_signal_new ("activate",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NautilusAdapterEmbedStrategyClass, activate),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[DEACTIVATE] =
		g_signal_new ("deactivate",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NautilusAdapterEmbedStrategyClass, deactivate),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[OPEN_LOCATION] =
		g_signal_new ("open_location",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NautilusAdapterEmbedStrategyClass, open_location),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__STRING,
		              G_TYPE_NONE, 1, G_TYPE_STRING);
	
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, nautilus_adapter_embed_strategy, get_widget);
	EEL_ASSIGN_MUST_OVERRIDE_SIGNAL (klass, nautilus_adapter_embed_strategy, get_zoomable);
}

static void
nautilus_adapter_embed_strategy_init (NautilusAdapterEmbedStrategy *strategy)
{
}

NautilusAdapterEmbedStrategy *
nautilus_adapter_embed_strategy_get (Bonobo_Unknown component)
{
	NautilusAdapterEmbedStrategy *strategy;
	Bonobo_ControlFactory	control_factory;
	Bonobo_Control		control;
	CORBA_Environment	ev;

	CORBA_exception_init (&ev);

	strategy = NULL;

	control = Bonobo_Unknown_queryInterface
		(component, "IDL:Bonobo/Control:1.0", &ev);
	if (ev._major == CORBA_NO_EXCEPTION && !CORBA_Object_is_nil (control, &ev)) {
		strategy = nautilus_adapter_control_embed_strategy_new
			(control, CORBA_OBJECT_NIL);
		bonobo_object_release_unref (control, NULL);
	}
	
	if (strategy != NULL) {
		control_factory = Bonobo_Unknown_queryInterface
			(component, "IDL:Bonobo/ControlFactory:1.0", &ev);
		if (ev._major == CORBA_NO_EXCEPTION && !CORBA_Object_is_nil (control_factory, &ev)) {
			strategy = nautilus_adapter_control_factory_embed_strategy_new
				(control_factory, CORBA_OBJECT_NIL);
			bonobo_object_release_unref (control_factory, NULL);
		}
	}

	CORBA_exception_free (&ev);

	return strategy;
}

GtkWidget *
nautilus_adapter_embed_strategy_get_widget (NautilusAdapterEmbedStrategy *strategy)
{
	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(NAUTILUS_ADAPTER_EMBED_STRATEGY_CLASS, strategy,
		 get_widget, (strategy));
}

BonoboObject *
nautilus_adapter_embed_strategy_get_zoomable (NautilusAdapterEmbedStrategy *strategy)
{
	return EEL_CALL_METHOD_WITH_RETURN_VALUE
		(NAUTILUS_ADAPTER_EMBED_STRATEGY_CLASS, strategy,
		 get_zoomable, (strategy));
}


void 
nautilus_adapter_embed_strategy_activate (NautilusAdapterEmbedStrategy *strategy,
					  Bonobo_UIContainer            ui_container)
{
	g_signal_emit (strategy,
			 signals[ACTIVATE], 0,
			 ui_container);
}

void 
nautilus_adapter_embed_strategy_deactivate (NautilusAdapterEmbedStrategy *strategy)
{
	g_signal_emit (strategy,
			 signals[DEACTIVATE], 0);
}

void 
nautilus_adapter_embed_strategy_emit_open_location (NautilusAdapterEmbedStrategy *strategy,
						    const char                   *uri)
{
	g_signal_emit (strategy,
			 signals[OPEN_LOCATION], 0,
			 uri);
}
