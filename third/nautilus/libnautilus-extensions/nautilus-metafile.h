/* -*- Mode: IDL; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/* nautilus-metafile.h - server side of Nautilus::Metafile
 *
 * Copyright (C) 2001 Eazel, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef NAUTILUS_METAFILE_H
#define NAUTILUS_METAFILE_H

#include "nautilus-metafile-server.h"

#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-xobject.h>
#include <gnome-xml/tree.h>

#include "nautilus-directory.h"

#define NAUTILUS_TYPE_METAFILE	          (nautilus_metafile_get_type ())
#define NAUTILUS_METAFILE(obj)	          (GTK_CHECK_CAST ((obj), NAUTILUS_TYPE_METAFILE, NautilusMetafile))
#define NAUTILUS_METAFILE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_METAFILE, NautilusMetafileClass))
#define NAUTILUS_IS_METAFILE(obj)         (GTK_CHECK_TYPE ((obj), NAUTILUS_TYPE_METAFILE))
#define NAUTILUS_IS_METAFILE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_METAFILE))

typedef struct NautilusMetafileDetails NautilusMetafileDetails;

typedef struct {
	BonoboXObject parent_slot;
	NautilusMetafileDetails *details;
} NautilusMetafile;

/* FIXME: move this back to nautilus-metafile.c after changes to 
 * nautilus-directory-metafile.c mean that it doesn't need to access it.
 */
struct NautilusMetafileDetails {
	NautilusDirectory *directory;

	/* FIXME: remove this field when gratuitous ORBit hacks are removed */
	gboolean directory_ref_is_gone;
};

typedef struct {
	BonoboXObjectClass parent_slot;
	POA_Nautilus_Metafile__epv epv;
} NautilusMetafileClass;

GtkType nautilus_metafile_get_type (void);

NautilusMetafile *nautilus_metafile_new (const char *directory_uri);

void call_metafile_changed_for_all_files_mentioned_in_metafile (NautilusDirectory *directory);

void nautilus_metafile_notify_metafile_ready (NautilusDirectory *directory);

void nautilus_metafile_apply_pending_changes (NautilusDirectory *directory);

void nautilus_metafile_destroy (NautilusDirectory *directory);

void nautilus_metafile_set_metafile_contents (NautilusDirectory *directory,
					  xmlDocPtr metafile_contents);


#endif /* NAUTILUS_METAFILE_H */
