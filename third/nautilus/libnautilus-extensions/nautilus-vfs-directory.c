/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-vfs-directory.c: Subclass of NautilusDirectory to help implement the
   virtual trash directory.
 
   Copyright (C) 1999, 2000 Eazel, Inc.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Author: Darin Adler <darin@eazel.com>
*/

#include <config.h>
#include "nautilus-vfs-directory.h"

#include "nautilus-directory-private.h"
#include "nautilus-gtk-macros.h"
#include "nautilus-file-private.h"

struct NautilusVFSDirectoryDetails {
};

static void nautilus_vfs_directory_initialize       (gpointer   object,
						     gpointer   klass);
static void nautilus_vfs_directory_initialize_class (gpointer   klass);

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusVFSDirectory,
				   nautilus_vfs_directory,
				   NAUTILUS_TYPE_DIRECTORY)

static void
nautilus_vfs_directory_initialize (gpointer object, gpointer klass)
{
	NautilusVFSDirectory *directory;

	directory = NAUTILUS_VFS_DIRECTORY (object);

	directory->details = g_new0 (NautilusVFSDirectoryDetails, 1);
}

static void
vfs_destroy (GtkObject *object)
{
	NAUTILUS_CALL_PARENT_CLASS (GTK_OBJECT_CLASS, destroy, (object));
}

static gboolean
vfs_contains_file (NautilusDirectory *directory,
		   NautilusFile *file)
{
	g_assert (NAUTILUS_IS_VFS_DIRECTORY (directory));
	g_return_val_if_fail (NAUTILUS_IS_FILE (file), FALSE);

	return file->details->directory == directory;
}

static void
vfs_call_when_ready (NautilusDirectory *directory,
		     GList *file_attributes,
		     NautilusDirectoryCallback callback,
		     gpointer callback_data)
{
	g_assert (NAUTILUS_IS_VFS_DIRECTORY (directory));

	nautilus_directory_call_when_ready_internal
		(directory,
		 NULL,
		 file_attributes,
		 callback,
		 NULL,
		 callback_data);
}

static void
vfs_cancel_callback (NautilusDirectory *directory,
		     NautilusDirectoryCallback callback,
		     gpointer callback_data)
{
	g_assert (NAUTILUS_IS_VFS_DIRECTORY (directory));

	nautilus_directory_cancel_callback_internal
		(directory,
		 NULL,
		 callback,
		 NULL,
		 callback_data);
}

static void
vfs_file_monitor_add (NautilusDirectory *directory,
		      gconstpointer client,
		      gboolean monitor_hidden_files,
		      gboolean monitor_backup_files,
		      GList *file_attributes,
		      gboolean force_reload)
{
	g_assert (NAUTILUS_IS_VFS_DIRECTORY (directory));
	g_assert (client != NULL);

	if (force_reload) {
		nautilus_directory_force_reload (directory,
						 file_attributes);
	}

	nautilus_directory_monitor_add_internal
		(directory, NULL,
		 client,
		 monitor_hidden_files,
		 monitor_backup_files,
		 file_attributes);
}

static void
vfs_file_monitor_remove (NautilusDirectory *directory,
			 gconstpointer client)
{
	g_assert (NAUTILUS_IS_VFS_DIRECTORY (directory));
	g_assert (client != NULL);
	
	nautilus_directory_monitor_remove_internal (directory, NULL, client);
}

static gboolean
vfs_are_all_files_seen (NautilusDirectory *directory)
{
	g_assert (NAUTILUS_IS_VFS_DIRECTORY (directory));
	
	return directory->details->directory_loaded;
}

static int
any_non_metafile_item (gconstpointer item, gconstpointer callback_data)
{
	/* A metafile is exactly what we are not looking for, anything else is a match. */
	return nautilus_file_matches_uri
		(NAUTILUS_FILE (item), (const char *) callback_data)
		? 1 : 0;
}

static gboolean
vfs_is_not_empty (NautilusDirectory *directory)
{
	char *public_metafile_uri;
	gboolean not_empty;
	
	g_return_val_if_fail (NAUTILUS_IS_VFS_DIRECTORY (directory), FALSE);
	g_return_val_if_fail (nautilus_directory_is_anyone_monitoring_file_list (directory), FALSE);
	
	if (directory->details->public_metafile_vfs_uri == NULL) {
		not_empty = directory->details->file_list != NULL;
	} else {
		public_metafile_uri = gnome_vfs_uri_to_string
			(directory->details->public_metafile_vfs_uri,
			 GNOME_VFS_URI_HIDE_NONE);
		
		/* Return TRUE if the directory contains anything besides a metafile. */
		not_empty = g_list_find_custom (directory->details->file_list,
						public_metafile_uri,
						any_non_metafile_item) != NULL;
		
		g_free (public_metafile_uri);
	}
	
	return not_empty;
}

static void
nautilus_vfs_directory_initialize_class (gpointer klass)
{
	GtkObjectClass *object_class;
	NautilusDirectoryClass *directory_class;

	object_class = GTK_OBJECT_CLASS (klass);
	directory_class = NAUTILUS_DIRECTORY_CLASS (klass);
	
	object_class->destroy = vfs_destroy;

	directory_class->contains_file = vfs_contains_file;
	directory_class->call_when_ready = vfs_call_when_ready;
	directory_class->cancel_callback = vfs_cancel_callback;
	directory_class->file_monitor_add = vfs_file_monitor_add;
	directory_class->file_monitor_remove = vfs_file_monitor_remove;
	directory_class->are_all_files_seen = vfs_are_all_files_seen;
	directory_class->is_not_empty = vfs_is_not_empty;
}
