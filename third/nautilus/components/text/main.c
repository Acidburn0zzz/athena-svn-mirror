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
 */

/* main.c - Main function and object activation function for text
 * view component.
 */

#include <config.h>
#include <gnome.h>
#include "nautilus-text-view.h"
#include <libnautilus/nautilus-view-standard-main.h>

#define FACTORY_IID       "OAFIID:nautilus_text_view_factory:124ae209-d356-418f-8757-54e071cb3a21"
#define VIEW_IID          "OAFIID:nautilus_text_view:fa466311-17c1-435c-8231-c9fc434b6437"
#define EXECUTABLE_NAME   "nautilus-text-view"
#define GET_TYPE_FUNCTION nautilus_text_view_get_type

int
main (int argc, char *argv[])
{
/* Initialize gettext support */
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
#endif

	return nautilus_view_standard_main (EXECUTABLE_NAME, VERSION,
					    argc, argv,
					    FACTORY_IID, VIEW_IID,
					    nautilus_view_create_from_get_type_function,
					    GET_TYPE_FUNCTION);
}
