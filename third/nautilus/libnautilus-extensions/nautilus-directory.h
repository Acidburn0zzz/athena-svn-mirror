/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-directory.h: Nautilus directory model.
 
   Copyright (C) 1999, 2000, 2001 Eazel, Inc.
  
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

#ifndef NAUTILUS_DIRECTORY_H
#define NAUTILUS_DIRECTORY_H

#include <gtk/gtkobject.h>
#include <libgnomevfs/gnome-vfs-types.h>

/* NautilusDirectory is a class that manages the model for a directory,
   real or virtual, for Nautilus, mainly the file-manager component. The directory is
   responsible for managing both real data and cached metadata. On top of
   the file system independence provided by gnome-vfs, the directory
   object also provides:
  
       1) A synchronization framework, which notifies via signals as the
          set of known files changes.
       2) An abstract interface for getting attributes and performing
          operations on files.
*/

#define NAUTILUS_TYPE_DIRECTORY \
	(nautilus_directory_get_type ())
#define NAUTILUS_DIRECTORY(obj) \
	(GTK_CHECK_CAST ((obj), NAUTILUS_TYPE_DIRECTORY, NautilusDirectory))
#define NAUTILUS_DIRECTORY_CLASS(klass) \
	(GTK_CHECK_CLASS_CAST ((klass), NAUTILUS_TYPE_DIRECTORY, NautilusDirectoryClass))
#define NAUTILUS_IS_DIRECTORY(obj) \
	(GTK_CHECK_TYPE ((obj), NAUTILUS_TYPE_DIRECTORY))
#define NAUTILUS_IS_DIRECTORY_CLASS(klass) \
	(GTK_CHECK_CLASS_TYPE ((klass), NAUTILUS_TYPE_DIRECTORY))

/* NautilusFile is defined both here and in nautilus-file.h. */
#ifndef NAUTILUS_FILE_DEFINED
#define NAUTILUS_FILE_DEFINED
typedef struct NautilusFile NautilusFile;
#endif

/* FIXME bugzilla.eazel.com 5382:
 * Increase or remove this limit? 
 */
/* FIXME bugzilla.eazel.com 5603:
 * This limit is not actually "hard", which can lead to some minor UI problems.
 */
#define NAUTILUS_DIRECTORY_FILE_LIST_HARD_LIMIT     4000

typedef struct NautilusDirectoryDetails NautilusDirectoryDetails;

typedef struct
{
	GtkObject object;
	NautilusDirectoryDetails *details;
} NautilusDirectory;

typedef void (*NautilusDirectoryCallback) (NautilusDirectory *directory,
					   GList             *files,
					   gpointer           callback_data);

typedef struct
{
	GtkObjectClass parent_class;

	/*** Notification signals for clients to connect to. ***/

	/* The files_added signal is emitted as the directory model 
	 * discovers new files.
	 */
	void     (* files_added)         (NautilusDirectory          *directory,
					  GList                      *added_files);

	/* The files_changed signal is emitted as changes occur to
	 * existing files that are noticed by the synchronization framework,
	 * including when an old file has been deleted. When an old file
	 * has been deleted, this is the last chance to forget about these
	 * file objects, which are about to be unref'd. Use a call to
	 * nautilus_file_is_gone () to test for this case.
	 */
	void     (* files_changed)       (NautilusDirectory         *directory,
					  GList                     *changed_files);

	/* The done_loading signal is emitted when a directory load
	 * request completes. This is needed because, at least in the
	 * case where the directory is empty, the caller will receive
	 * no kind of notification at all when a directory load
	 * initiated by `nautilus_directory_file_monitor_add' completes.
	 */
	void     (* done_loading)        (NautilusDirectory         *directory);

	void     (* load_error)          (NautilusDirectory         *directory);

	/*** Virtual functions for subclasses to override. ***/
	gboolean (* contains_file)       (NautilusDirectory         *directory,
					  NautilusFile              *file);
	void     (* call_when_ready)     (NautilusDirectory         *directory,
					  GList                     *file_attributes,
					  NautilusDirectoryCallback  callback,
					  gpointer                   callback_data);
	void     (* cancel_callback)     (NautilusDirectory         *directory,
					  NautilusDirectoryCallback  callback,
					  gpointer                   callback_data);
	void     (* file_monitor_add)    (NautilusDirectory          *directory,
					  gconstpointer              client,
					  gboolean                   monitor_hidden_files,
					  gboolean                   monitor_backup_files,
					  GList                     *monitor_attributes);
	void     (* file_monitor_remove) (NautilusDirectory         *directory,
					  gconstpointer              client);
	void     (* force_reload)        (NautilusDirectory         *directory);
	gboolean (* are_all_files_seen)  (NautilusDirectory         *directory);
	gboolean (* is_not_empty)        (NautilusDirectory         *directory);
	char *	 (* get_name_for_self_as_new_file) (NautilusDirectory *directory);
} NautilusDirectoryClass;

/* Basic GtkObject requirements. */
GtkType            nautilus_directory_get_type                 (void);

/* Get a directory given a uri.
 * Creates the appropriate subclass given the uri mappings.
 * Returns a referenced object, not a floating one. Unref when finished.
 * If two windows are viewing the same uri, the directory object is shared.
 */
NautilusDirectory *nautilus_directory_get                      (const char                *uri);

/* Covers for gtk_object_ref and gtk_object_unref that provide two conveniences:
 * 1) You don't have to cast to GtkObject *, so using these is type safe.
 * 2) You are allowed to call these with NULL,
 */
void               nautilus_directory_ref                      (NautilusDirectory         *directory);
void               nautilus_directory_unref                    (NautilusDirectory         *directory);

/* Access to a URI. */
char *             nautilus_directory_get_uri                  (NautilusDirectory         *directory);

/* Is this file still alive and in this directory? */
gboolean           nautilus_directory_contains_file            (NautilusDirectory         *directory,
								NautilusFile              *file);

/* Get the uri of the file in the directory, NULL if not found */
char *		   nautilus_directory_get_file_uri	       (NautilusDirectory	  *directory,
								const char 		  *file_name);

/* Get (and ref) a NautilusFile object for this directory. */
NautilusFile *     nautilus_directory_get_corresponding_file   (NautilusDirectory         *directory);
							    

/* Waiting for data that's read asynchronously.
 * The file attribute and metadata keys are for files in the directory.
 * If any file attributes or metadata keys are passed, it won't call
 * until all the files are seen.
 */
void               nautilus_directory_call_when_ready          (NautilusDirectory         *directory,
								GList                     *file_attributes,
								NautilusDirectoryCallback  callback,
								gpointer                   callback_data);
void               nautilus_directory_cancel_callback          (NautilusDirectory         *directory,
								NautilusDirectoryCallback  callback,
								gpointer                   callback_data);


/* Monitor the files in a directory. */
void               nautilus_directory_file_monitor_add         (NautilusDirectory         *directory,
								gconstpointer              client,
								gboolean                   monitor_hidden_files,
								gboolean                   monitor_backup_files,
								GList                     *attributes);
void               nautilus_directory_file_monitor_remove      (NautilusDirectory         *directory,
								gconstpointer              client);
void               nautilus_directory_force_reload             (NautilusDirectory         *directory);

/* Return true if the directory has information about all the files.
 * This will be false until the directory has been read at least once.
 */
gboolean           nautilus_directory_are_all_files_seen       (NautilusDirectory         *directory);

/* Return true if the directory is local. */
gboolean           nautilus_directory_is_local                 (NautilusDirectory         *directory);

/* Return false if directory contains anything besides a Nautilus metafile.
 * Only valid if directory is monitored. Used by the Trash monitor.
 */
gboolean           nautilus_directory_is_not_empty             (NautilusDirectory         *directory);
gboolean           nautilus_directory_file_list_length_reached (NautilusDirectory         *directory);

#endif /* NAUTILUS_DIRECTORY_H */
