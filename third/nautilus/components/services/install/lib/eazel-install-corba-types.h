/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * Copyright (C) 2000 Eazel, Inc
 * Copyright (C) 2000 Helix Code, Inc
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
 * Authors: Eskil Heyn Olsen <eskil@eazel.com>
 */

#ifndef EAZEL_INSTALL_SERVICES_CORBA_TYPES_H
#define EAZEL_INSTALL_SERVICES_CORBA_TYPES_H

#include <eazel-package-system-types.h>
#include <trilobite-eazel-install.h>
#include <glib.h>

GNOME_Trilobite_Eazel_PackageDataStruct* corba_packagedatastruct_from_packagedata (const PackageData *pack);

GNOME_Trilobite_Eazel_PackageDataStructList* corba_packagedatastructlist_from_packagedata_list (GList *packages);

GNOME_Trilobite_Eazel_PackageDataStructList* corba_packagedatastructlist_from_packagedata_tree (GList *packlist);

GNOME_Trilobite_Eazel_CategoryStructList* corba_category_list_from_categorydata_list (GList *categories);

PackageData* packagedata_from_corba_packagedatastruct (const GNOME_Trilobite_Eazel_PackageDataStruct *corbapack);

GList* packagedata_list_from_corba_packagedatastructlist (const GNOME_Trilobite_Eazel_PackageDataStructList *corbapack);

GList* packagedata_tree_from_corba_packagedatastructlist (const GNOME_Trilobite_Eazel_PackageDataStructList *corbalist);

GList* categorydata_list_from_corba_categorystructlist (const GNOME_Trilobite_Eazel_CategoryStructList *corbacategories);

#endif /* EAZEL_INSTALL_SERVICES_TYPES_H */
