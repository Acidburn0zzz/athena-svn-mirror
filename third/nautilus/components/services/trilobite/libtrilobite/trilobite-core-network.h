/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 * Copyright (C) 2000 Eazel, Inc
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * libtrilobite - Useful functions shared between all services.  This
 * includes things like xml parsing, logging, error control, and others.
 */

#ifndef TRILOBITE_CORE_NETWORK_H
#define TRILOBITE_CORE_NETWORK_H

#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>

char *trilobite_xml_get_string (xmlNode *node, const char *name);
gboolean trilobite_fetch_uri (const char *uri_text, char **body, int *length);
gboolean trilobite_fetch_uri_to_file (const char *uri_text, const char *filename);

#endif /* TRILOBITE_CORE_NETWORK_H */
