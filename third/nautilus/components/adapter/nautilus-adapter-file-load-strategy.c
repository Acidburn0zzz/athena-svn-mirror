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


/* nautilus-adapter-file-load-strategy.c - 
 */


#include <config.h>

#include <string.h>
#include "nautilus-adapter-file-load-strategy.h"

#include <gtk/gtkobject.h>
#include <eel/eel-gtk-macros.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libnautilus/nautilus-view.h>


struct NautilusAdapterFileLoadStrategyDetails {
	Bonobo_PersistFile  persist_file;
};


static void nautilus_adapter_file_load_strategy_class_init (NautilusAdapterFileLoadStrategyClass *klass);
static void nautilus_adapter_file_load_strategy_init       (NautilusAdapterFileLoadStrategy      *strategy);
static void nautilus_adapter_file_load_strategy_destroy          (GtkObject                              *object);

static void nautilus_adapter_file_load_strategy_load_location (NautilusAdapterLoadStrategy *strategy,
								 const char                  *uri);

static void nautilus_adapter_file_load_strategy_stop_loading  (NautilusAdapterLoadStrategy *strategy);


EEL_CLASS_BOILERPLATE (NautilusAdapterFileLoadStrategy, nautilus_adapter_file_load_strategy, NAUTILUS_TYPE_ADAPTER_LOAD_STRATEGY)


static void
nautilus_adapter_file_load_strategy_class_init (NautilusAdapterFileLoadStrategyClass *klass)
{
	GtkObjectClass                   *object_class;
	NautilusAdapterLoadStrategyClass *adapter_load_strategy_class;

	object_class = GTK_OBJECT_CLASS (klass);

	object_class->destroy = nautilus_adapter_file_load_strategy_destroy;

	adapter_load_strategy_class = NAUTILUS_ADAPTER_LOAD_STRATEGY_CLASS (klass);

	adapter_load_strategy_class->load_location = nautilus_adapter_file_load_strategy_load_location;
	adapter_load_strategy_class->stop_loading = nautilus_adapter_file_load_strategy_stop_loading;
}

static void
nautilus_adapter_file_load_strategy_init (NautilusAdapterFileLoadStrategy *strategy)
{
	strategy->details = g_new0 (NautilusAdapterFileLoadStrategyDetails, 1);
}

static void
nautilus_adapter_file_load_strategy_destroy (GtkObject *object)
{
	NautilusAdapterFileLoadStrategy *strategy;
	CORBA_Environment ev;

	strategy = NAUTILUS_ADAPTER_FILE_LOAD_STRATEGY (object);

	if (strategy->details->persist_file != CORBA_OBJECT_NIL) {
		CORBA_exception_init (&ev);
		bonobo_object_release_unref (strategy->details->persist_file, &ev);
		CORBA_exception_free (&ev);
	}

	g_free (strategy->details);

	EEL_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}




NautilusAdapterLoadStrategy *
nautilus_adapter_file_load_strategy_new (Bonobo_PersistFile  persist_file)
{
	NautilusAdapterFileLoadStrategy *strategy;

	strategy = NAUTILUS_ADAPTER_FILE_LOAD_STRATEGY (g_object_new (NAUTILUS_TYPE_ADAPTER_FILE_LOAD_STRATEGY, NULL));
	g_object_ref (strategy);
	gtk_object_sink (GTK_OBJECT (strategy));

	strategy->details->persist_file = persist_file;
	
	return NAUTILUS_ADAPTER_LOAD_STRATEGY (strategy);
}

static ORBit_IMethod *
get_file_load_method (void)
{
	guint i;
	ORBit_IInterface *iface;
	static ORBit_IMethod *method = NULL;
	
	iface = &Bonobo_PersistFile__iinterface;
	if (!method) {
		for (i = 0; i < iface->methods._length; i++) {
			if (!strcmp ("load", iface->methods._buffer [i].name)) {
				method = &iface->methods._buffer [i];
			}
		}
	}
	g_assert (method);

	return method;
}

static void
nautilus_adapter_file_load_strategy_load_location (NautilusAdapterLoadStrategy *abstract_strategy,
						   const char                  *uri)

{
	char *local_path;
	gpointer args[1];
	NautilusAdapterFileLoadStrategy *strategy;

	strategy = NAUTILUS_ADAPTER_FILE_LOAD_STRATEGY (abstract_strategy);

	g_object_ref (strategy);

	local_path = gnome_vfs_get_local_path_from_uri (uri);

	if (local_path == NULL) {
		nautilus_adapter_load_strategy_report_load_failed (abstract_strategy);
		return;
	}

	nautilus_adapter_load_strategy_report_load_underway (abstract_strategy);

	args [0] = &local_path;
	
	nautilus_adapter_load_strategy_load_async (
		abstract_strategy,
		strategy->details->persist_file,
		get_file_load_method (),
		args,
		NULL, NULL);

	g_object_unref (strategy);

	g_free (local_path);
}

static void
nautilus_adapter_file_load_strategy_stop_loading  (NautilusAdapterLoadStrategy *abstract_strategy)
{

	g_return_if_fail (NAUTILUS_IS_ADAPTER_FILE_LOAD_STRATEGY (abstract_strategy));

}

