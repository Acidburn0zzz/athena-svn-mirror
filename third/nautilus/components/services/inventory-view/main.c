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

/* main.c - Main function and object activation function for inventory
 * view component.
 */

#include <config.h>
#include "nautilus-inventory-view.h"
#include <libnautilus/nautilus-view-standard-main.h>

#define FACTORY_IID        "OAFIID:nautilus_inventory_view_factory:abf7f064-bcd0-4ac5-93a2-35d5a676e7cc"
#define VIEW_IID           "OAFIID:nautilus_inventory_view:c6f2baf8-f563-4b84-8b02-81fb355d3fb5"
#define EXECUTABLE_NAME    "nautilus-inventory-view"
#define GET_TYPE_FUNCTION  nautilus_inventory_view_get_type

int
main (int argc, char *argv[])
{
	return nautilus_view_standard_main (EXECUTABLE_NAME, VERSION,
					    argc, argv,
					    FACTORY_IID, VIEW_IID,
					    nautilus_view_create_from_get_type_function,
					    GET_TYPE_FUNCTION);
}

