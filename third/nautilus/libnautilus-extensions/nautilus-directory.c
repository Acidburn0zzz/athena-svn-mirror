/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-directory.c: Nautilus directory model.
 
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
#include "nautilus-directory-private.h"

#include "nautilus-directory-metafile.h"
#include "nautilus-directory-notify.h"
#include "nautilus-file-private.h"
#include "nautilus-file-utilities.h"
#include "nautilus-glib-extensions.h"
#include "nautilus-global-preferences.h"
#include "nautilus-gtk-macros.h"
#include "nautilus-lib-self-check-functions.h"
#include "nautilus-metadata.h"
#include "nautilus-string.h"
#include "nautilus-trash-directory.h"
#include "nautilus-vfs-directory.h"
#include <ctype.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

enum {
	FILES_ADDED,
	FILES_CHANGED,
	DONE_LOADING,
	LOAD_ERROR,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* Specifications for in-directory metafile. */
#define METAFILE_NAME ".nautilus-metafile.xml"

/* Specifications for parallel-directory metafile. */
#define METAFILES_DIRECTORY_NAME "metafiles"
#define METAFILE_SUFFIX ".xml"
#define METAFILES_DIRECTORY_PERMISSIONS \
	(GNOME_VFS_PERM_USER_ALL \
         | GNOME_VFS_PERM_GROUP_ALL \
	 | GNOME_VFS_PERM_OTHER_ALL)

static GHashTable *directories;

static void               nautilus_directory_destroy          (GtkObject              *object);
static void               nautilus_directory_initialize       (gpointer                object,
							       gpointer                klass);
static void               nautilus_directory_initialize_class (NautilusDirectoryClass *klass);
static NautilusDirectory *nautilus_directory_new              (const char             *uri);
static char *             real_get_name_for_self_as_new_file  (NautilusDirectory      *directory);
static void               set_directory_uri                   (NautilusDirectory      *directory,
							       const char             *new_uri);

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusDirectory,
				   nautilus_directory,
				   GTK_TYPE_OBJECT)

static void
nautilus_directory_initialize_class (NautilusDirectoryClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	
	object_class->destroy = nautilus_directory_destroy;

	signals[FILES_ADDED] =
		gtk_signal_new ("files_added",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusDirectoryClass, files_added),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[FILES_CHANGED] =
		gtk_signal_new ("files_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusDirectoryClass, files_changed),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
	signals[DONE_LOADING] =
		gtk_signal_new ("done_loading",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusDirectoryClass, done_loading),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	signals[LOAD_ERROR] =
		gtk_signal_new ("load_error",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusDirectoryClass, load_error),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1, GTK_TYPE_INT);
	
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	klass->get_name_for_self_as_new_file = real_get_name_for_self_as_new_file;
}

static void
nautilus_directory_initialize (gpointer object, gpointer klass)
{
	NautilusDirectory *directory;

	directory = NAUTILUS_DIRECTORY(object);

	directory->details = g_new0 (NautilusDirectoryDetails, 1);
	directory->details->file_hash = g_hash_table_new (g_str_hash, g_str_equal);
	directory->details->metafile_node_hash = g_hash_table_new (g_str_hash, g_str_equal);
}

void
nautilus_directory_ref (NautilusDirectory *directory)
{
	if (directory == NULL) {
		return;
	}

	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));

	gtk_object_ref (GTK_OBJECT (directory));
}

void
nautilus_directory_unref (NautilusDirectory *directory)
{
	if (directory == NULL) {
		return;
	}

	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));

	gtk_object_unref (GTK_OBJECT (directory));
}

static void
nautilus_directory_destroy (GtkObject *object)
{
	NautilusDirectory *directory;

	directory = NAUTILUS_DIRECTORY (object);

	g_assert (directory->details->metafile_write_state == NULL);
	nautilus_directory_cancel (directory);
	g_assert (directory->details->metafile_read_state == NULL);
	g_assert (directory->details->count_in_progress == NULL);
	g_assert (directory->details->top_left_read_state == NULL);

	if (directory->details->monitor_list != NULL) {
		g_warning ("destroying a NautilusDirectory while it's being monitored");
		nautilus_g_list_free_deep (directory->details->monitor_list);
	}

	g_hash_table_remove (directories, directory->details->uri);

	if (directory->details->dequeue_pending_idle_id != 0) {
		gtk_idle_remove (directory->details->dequeue_pending_idle_id);
	}
 
	nautilus_directory_metafile_destroy (directory);

	g_free (directory->details->uri);
	if (directory->details->vfs_uri != NULL) {
		gnome_vfs_uri_unref (directory->details->vfs_uri);
	}
	if (directory->details->public_metafile_vfs_uri != NULL) {
		gnome_vfs_uri_unref (directory->details->public_metafile_vfs_uri);
	}
	if (directory->details->private_metafile_vfs_uri != NULL) {
		gnome_vfs_uri_unref (directory->details->private_metafile_vfs_uri);
	}
	g_assert (directory->details->file_list == NULL);
	g_hash_table_destroy (directory->details->file_hash);
	g_hash_table_destroy (directory->details->metafile_node_hash);
	g_assert (directory->details->directory_load_in_progress == NULL);
	g_assert (directory->details->count_in_progress == NULL);
	g_assert (directory->details->dequeue_pending_idle_id == 0);
	gnome_vfs_file_info_list_unref (directory->details->pending_file_info);
	g_assert (directory->details->write_metafile_idle_id == 0);

	g_free (directory->details);

	NAUTILUS_CALL_PARENT_CLASS (GTK_OBJECT_CLASS, destroy, (object));
}

static void
invalidate_one_count (gpointer key, gpointer value, gpointer user_data)
{
	NautilusDirectory *directory;

	g_assert (key != NULL);
	g_assert (NAUTILUS_IS_DIRECTORY (value));
	g_assert (user_data == NULL);

	directory = NAUTILUS_DIRECTORY (value);
	
	nautilus_directory_invalidate_counts (directory);
}

static void
filtering_changed_callback (gpointer callback_data)
{
	g_assert (callback_data == NULL);

	/* Preference about which items to show has changed, so we
	 * can't trust any of our precomputed directory counts.
	 */
	g_hash_table_foreach (directories, invalidate_one_count, NULL);
}

static void
add_filtering_callbacks (void)
{
	nautilus_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
					   filtering_changed_callback,
					   NULL);
	nautilus_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
					   filtering_changed_callback,
					   NULL);
}

static void
remove_filtering_callbacks (void)
{
	nautilus_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
					      filtering_changed_callback,
					      NULL);
	nautilus_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
					      filtering_changed_callback,
					      NULL);
}

static char *
nautilus_directory_make_uri_canonical (const char *uri)
{
	char *canonical_maybe_trailing_slash;
	char *canonical;
	char *with_slashes;
	size_t length;

	canonical_maybe_trailing_slash = nautilus_make_uri_canonical (uri);

	/* To NautilusDirectory, a uri with or without a trailing
	 * / is equivalent. This is necessary to prevent separate
	 * NautilusDirectories for the same location from being
	 * created. (See bugzilla.eazel.com 3322 for an example.)
	 */
	canonical = nautilus_str_strip_trailing_chr (canonical_maybe_trailing_slash, '/');
	if (strcmp (canonical, canonical_maybe_trailing_slash) != 0) {
		/* If some trailing '/' were stripped, there's the possibility,
		 * that we stripped away all the '/' from a uri that has only
		 * '/' characters. If you change this code, check to make sure
		 * that "file:///" still works as a URI.
		 */
		length = strlen (canonical);
		if (length == 0 || canonical[length - 1] == ':') {
			with_slashes = g_strconcat (canonical, "///", NULL);
			g_free (canonical);
			canonical = with_slashes;
		}
	}

	g_free (canonical_maybe_trailing_slash);
	
	return canonical;
}


/**
 * nautilus_directory_get:
 * @uri: URI of directory to get.
 *
 * Get a directory given a uri.
 * Creates the appropriate subclass given the uri mappings.
 * Returns a referenced object, not a floating one. Unref when finished.
 * If two windows are viewing the same uri, the directory object is shared.
 */
NautilusDirectory *
nautilus_directory_get_internal (const char *uri, gboolean create)
{
	char *canonical_uri;
	NautilusDirectory *directory;

	if (uri == NULL) {
    		return NULL;
	}

	canonical_uri = nautilus_directory_make_uri_canonical (uri);

	/* Create the hash table first time through. */
	if (directories == NULL) {
		directories = nautilus_g_hash_table_new_free_at_exit
			(g_str_hash, g_str_equal, "nautilus-directory.c: directories");

		add_filtering_callbacks ();
		g_atexit (remove_filtering_callbacks);
		
	}

	/* If the object is already in the hash table, look it up. */

	directory = g_hash_table_lookup (directories,
					 canonical_uri);
	if (directory != NULL) {
		nautilus_directory_ref (directory);
	} else if (create) {
		/* Create a new directory object instead. */
		directory = nautilus_directory_new (canonical_uri);
		if (directory == NULL) {
			return NULL;
		}

		g_assert (strcmp (directory->details->uri, canonical_uri) == 0);

		/* Put it in the hash table. */
		g_hash_table_insert (directories,
				     directory->details->uri,
				     directory);
	}

	g_free (canonical_uri);

	return directory;
}

NautilusDirectory *
nautilus_directory_get (const char *uri)
{
	return nautilus_directory_get_internal (uri, TRUE);
}

NautilusDirectory *
nautilus_directory_get_existing (const char *uri)
{
	return nautilus_directory_get_internal (uri, FALSE);
}

/* Returns a reffed NautilusFile object for this directory.
 */
NautilusFile *
nautilus_directory_get_corresponding_file (NautilusDirectory *directory)
{
	NautilusFile *file;

	file = nautilus_directory_get_existing_corresponding_file (directory);
	if (file == NULL) {
		file = nautilus_file_get (directory->details->uri);
	}

	return file;
}

/* Returns a reffed NautilusFile object for this directory, but only if the
 * NautilusFile object has already been created.
 */
NautilusFile *
nautilus_directory_get_existing_corresponding_file (NautilusDirectory *directory)
{
	NautilusFile *file;
	
	file = directory->details->as_file;
	if (file != NULL) {
		nautilus_file_ref (file);
		return file;
	}

	return nautilus_file_get_existing (directory->details->uri);
}

/* nautilus_directory_get_name_for_self_as_new_file:
 * 
 * Get a name to display for the file representing this
 * directory. This is called only when there's no VFS
 * directory for this NautilusDirectory.
 */
char *
nautilus_directory_get_name_for_self_as_new_file (NautilusDirectory *directory)
{
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), NULL);
	
	return NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 get_name_for_self_as_new_file, (directory));
}

static char *
real_get_name_for_self_as_new_file (NautilusDirectory *directory)
{
	const char *directory_uri;
	char *name, *colon;
	
	directory_uri = directory->details->uri;

	colon = strchr (directory_uri, ':');
	if (colon == NULL || colon == directory_uri) {
		name = g_strdup (directory_uri);
	} else {
		name = g_strndup (directory_uri, colon - directory_uri);
	}

	return name;
}

char *
nautilus_directory_get_uri (NautilusDirectory *directory)
{
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), NULL);

	return g_strdup (directory->details->uri);
}

static GnomeVFSURI *
construct_private_metafile_vfs_uri (const char *uri)
{
	GnomeVFSResult result;
	char *user_directory;
	GnomeVFSURI *user_directory_uri, *metafiles_directory_uri, *alternate_uri;
	char *escaped_uri, *file_name;

	/* Ensure that the metafiles directory exists. */
	user_directory = nautilus_get_user_directory ();
	user_directory_uri = gnome_vfs_uri_new (user_directory);
	g_free (user_directory);

	metafiles_directory_uri = gnome_vfs_uri_append_file_name (user_directory_uri,
								  METAFILES_DIRECTORY_NAME);
	gnome_vfs_uri_unref (user_directory_uri);
	result = nautilus_make_directory_and_parents (metafiles_directory_uri,
						      METAFILES_DIRECTORY_PERMISSIONS);
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_FILE_EXISTS) {
		gnome_vfs_uri_unref (metafiles_directory_uri);
		return NULL;
	}

	/* Construct a file name from the URI. */
	escaped_uri = gnome_vfs_escape_slashes (uri);
	file_name = g_strconcat (escaped_uri, ".xml", NULL);
	g_free (escaped_uri);

	/* Construct a URI for something in the "metafiles" directory. */
	alternate_uri = gnome_vfs_uri_append_file_name (metafiles_directory_uri, file_name);
	gnome_vfs_uri_unref (metafiles_directory_uri);
	g_free (file_name);

	return alternate_uri;
}

static NautilusDirectory *
nautilus_directory_new (const char *uri)
{
	NautilusDirectory *directory;

	g_assert (uri != NULL);

	if (nautilus_uri_is_trash (uri)) {
		directory = NAUTILUS_DIRECTORY (gtk_object_new (NAUTILUS_TYPE_TRASH_DIRECTORY, NULL));
	} else {
		directory = NAUTILUS_DIRECTORY (gtk_object_new (NAUTILUS_TYPE_VFS_DIRECTORY, NULL));
	}
	gtk_object_ref (GTK_OBJECT (directory));
	gtk_object_sink (GTK_OBJECT (directory));

	set_directory_uri (directory, uri);

	return directory;
}

gboolean
nautilus_directory_is_local (NautilusDirectory *directory)
{
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), FALSE);
	
	if (directory->details->vfs_uri == NULL) {
		return TRUE;
	}
	return gnome_vfs_uri_is_local (directory->details->vfs_uri);
}

gboolean
nautilus_directory_are_all_files_seen (NautilusDirectory *directory)
{
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), FALSE);
	
	return NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 are_all_files_seen, (directory));
}

static void
add_to_hash_table (NautilusDirectory *directory, NautilusFile *file, GList *node)
{
	g_assert (node != NULL);
	g_assert (g_hash_table_lookup (directory->details->file_hash,
				       file->details->relative_uri) == NULL);
	g_hash_table_insert (directory->details->file_hash,
			     file->details->relative_uri, node);
}

static GList *
extract_from_hash_table (NautilusDirectory *directory, NautilusFile *file)
{
	char *relative_uri;
	GList *node;

	relative_uri = file->details->relative_uri;
	if (relative_uri == NULL) {
		return NULL;
	}

	/* Find the list node in the hash table. */
	node = g_hash_table_lookup (directory->details->file_hash, relative_uri);
	g_hash_table_remove (directory->details->file_hash, relative_uri);

	return node;
}

void
nautilus_directory_add_file (NautilusDirectory *directory, NautilusFile *file)
{
	GList *node;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (NAUTILUS_IS_FILE (file));
	g_assert (file->details->relative_uri != NULL);

	/* Add to list. */
	node = g_list_prepend (directory->details->file_list, file);
	directory->details->file_list = node;

	/* Add to hash table. */
	add_to_hash_table (directory, file, node);

	directory->details->confirmed_file_count++;

	/* Ref if we are monitoring. */
	if (nautilus_directory_is_file_list_monitored (directory)) {
		nautilus_file_ref (file);
	}
}

void
nautilus_directory_remove_file (NautilusDirectory *directory, NautilusFile *file)
{
	GList *node;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (NAUTILUS_IS_FILE (file));
	g_assert (file->details->relative_uri != NULL);

	/* Find the list node in the hash table. */
	node = extract_from_hash_table (directory, file);
	g_assert (node != NULL);
	g_assert (node->data == file);

	/* Remove the item from the list. */
	directory->details->file_list = g_list_remove_link
		(directory->details->file_list, node);
	g_list_free_1 (node);

	if (!file->details->unconfirmed) {
		directory->details->confirmed_file_count--;
	}

	/* Unref if we are monitoring. */
	if (nautilus_directory_is_file_list_monitored (directory)) {
		nautilus_file_unref (file);
	}
}

gboolean
nautilus_directory_file_list_length_reached (NautilusDirectory *directory)
{
	return directory->details->confirmed_file_count >= NAUTILUS_DIRECTORY_FILE_LIST_HARD_LIMIT;
}

GList *
nautilus_directory_begin_file_name_change (NautilusDirectory *directory,
					   NautilusFile *file)
{
	/* Find the list node in the hash table. */
	return extract_from_hash_table (directory, file);
}

void
nautilus_directory_end_file_name_change (NautilusDirectory *directory,
					 NautilusFile *file,
					 GList *node)
{
	/* Add the list node to the hash table. */
	if (node != NULL) {
		add_to_hash_table (directory, file, node);
	}
}

NautilusFile *
nautilus_directory_find_file_by_name (NautilusDirectory *directory,
				      const char *name)
{
	char *relative_uri;
	NautilusFile *file;

	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	relative_uri = gnome_vfs_escape_string (name);
	file = nautilus_directory_find_file_by_relative_uri
		(directory, relative_uri);
	g_free (relative_uri);
	return file;
}

NautilusFile *
nautilus_directory_find_file_by_relative_uri (NautilusDirectory *directory,
					      const char *relative_uri)
{
	GList *node;

	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), NULL);
	g_return_val_if_fail (relative_uri != NULL, NULL);

	node = g_hash_table_lookup (directory->details->file_hash,
				    relative_uri);
	return node == NULL ? NULL : NAUTILUS_FILE (node->data);
}

void
nautilus_directory_emit_files_added (NautilusDirectory *directory,
				     GList *added_files)
{
	if (added_files != NULL) {
		gtk_signal_emit (GTK_OBJECT (directory),
				 signals[FILES_ADDED],
				 added_files);
	}
}

void
nautilus_directory_emit_files_changed (NautilusDirectory *directory,
				       GList *changed_files)
{
	if (changed_files != NULL) {
		gtk_signal_emit (GTK_OBJECT (directory),
				 signals[FILES_CHANGED],
				 changed_files);
	}
}

void
nautilus_directory_emit_change_signals_deep (NautilusDirectory *directory,
					     GList *changed_files)
{
	GList *p;

	for (p = changed_files; p != NULL; p = p->next) {
		nautilus_file_emit_changed (p->data);
	}
	nautilus_directory_emit_files_changed (directory, changed_files);
}

void
nautilus_directory_emit_metadata_changed (NautilusDirectory *directory)
{
	/* Say that all the files have changed.
	 * We could optimize this to only mention files that
	 * have metadata, but this is a fine rough cut for now.
	 */
	nautilus_directory_emit_change_signals_deep
		(directory, directory->details->file_list);
}

void
nautilus_directory_emit_done_loading (NautilusDirectory *directory)
{
	gtk_signal_emit (GTK_OBJECT (directory),
			 signals[DONE_LOADING]);
}

void
nautilus_directory_emit_load_error (NautilusDirectory *directory,
				    GnomeVFSResult error_result)
{
	gtk_signal_emit (GTK_OBJECT (directory),
			 signals[LOAD_ERROR],
			 error_result);
}


static char *
uri_get_directory_part (const char *uri)
{
	GnomeVFSURI *vfs_uri, *directory_vfs_uri;
	char *directory_uri;

	/* Make VFS version of URI. */
	vfs_uri = gnome_vfs_uri_new (uri);
	if (vfs_uri == NULL) {
		return NULL;
	}

	/* Make VFS version of directory URI. */
	directory_vfs_uri = gnome_vfs_uri_get_parent (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);
	if (directory_vfs_uri == NULL) {
		return NULL;
	}

	/* Make text version of directory URI. */
	directory_uri = gnome_vfs_uri_to_string (directory_vfs_uri,
						 GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (directory_vfs_uri);
	
	return directory_uri;
}

/* Return a directory object for this one's parent. */
static NautilusDirectory *
get_parent_directory (const char *uri)
{
	char *directory_uri;
	NautilusDirectory *directory;

	directory_uri = uri_get_directory_part (uri);
	directory = nautilus_directory_get (directory_uri);
	g_free (directory_uri);
	return directory;
}

/* If a directory object exists for this one's parent, then
 * return it, otherwise return NULL.
 */
static NautilusDirectory *
get_parent_directory_if_exists (const char *uri)
{
	char *directory_uri;
	NautilusDirectory *directory;

	/* Make text version of directory URI. */
	directory_uri = uri_get_directory_part (uri);
	directory = nautilus_directory_get_existing (directory_uri);
	g_free (directory_uri);
	return directory;
}

static void
hash_table_list_prepend (GHashTable *table, gconstpointer key, gpointer data)
{
	GList *list;

	list = g_hash_table_lookup (table, key);
	list = g_list_prepend (list, data);
	g_hash_table_insert (table, (gpointer) key, list);
}

static void
call_files_added_free_list (gpointer key, gpointer value, gpointer user_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (key));
	g_assert (value != NULL);
	g_assert (user_data == NULL);

	gtk_signal_emit (GTK_OBJECT (key),
			 signals[FILES_ADDED],
			 value);
	g_list_free (value);
}

static void
call_files_changed_free_list (gpointer key, gpointer value, gpointer user_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (key));
	g_assert (value != NULL);
	g_assert (user_data == NULL);

	nautilus_directory_emit_change_signals_deep (key, value);
	g_list_free (value);
}

static void
call_files_changed_unref_free_list (gpointer key, gpointer value, gpointer user_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (key));
	g_assert (value != NULL);
	g_assert (user_data == NULL);

	nautilus_directory_emit_change_signals_deep (key, value);
	nautilus_file_list_free (value);
}

static void
call_get_file_info_free_list (gpointer key, gpointer value, gpointer user_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (key));
	g_assert (value != NULL);
	g_assert (user_data == NULL);

	nautilus_directory_get_info_for_new_files (key, value);
	gnome_vfs_uri_list_free (value);
}

static void
invalidate_count_and_unref (gpointer key, gpointer value, gpointer user_data)
{
	NautilusDirectory *directory;

	g_assert (NAUTILUS_IS_DIRECTORY (key));
	g_assert (value == key);
	g_assert (user_data == NULL);

	directory = NAUTILUS_DIRECTORY (key);
	
	nautilus_directory_invalidate_counts (directory);
	nautilus_directory_unref (directory);
}

static void
collect_parent_directories (GHashTable *hash_table, NautilusDirectory *directory)
{
	g_assert (hash_table != NULL);
	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	if (g_hash_table_lookup (hash_table, directory) == NULL) {
		nautilus_directory_ref (directory);
		g_hash_table_insert  (hash_table, directory, directory);
	}
}

void
nautilus_directory_notify_files_added (GList *uris)
{
	GHashTable *added_lists;
	GList *p;
	NautilusDirectory *directory;
	GHashTable *parent_directories;
	const char *uri;
	GnomeVFSURI *vfs_uri;

	/* Make a list of added files in each directory. */
	added_lists = g_hash_table_new (NULL, NULL);

	/* Make a list of parent directories that will need their counts updated. */
	parent_directories = g_hash_table_new (NULL, NULL);

	for (p = uris; p != NULL; p = p->next) {
		uri = (const char *) p->data;

		/* See if the directory is already known. */
		directory = get_parent_directory_if_exists (uri);
		if (directory == NULL) {
			continue;
		}

		collect_parent_directories (parent_directories, directory);

		/* If no one is monitoring files in the directory, nothing to do. */
		if (!nautilus_directory_is_file_list_monitored (directory)) {
			nautilus_directory_unref (directory);
			continue;
		}

		/* Collect the URIs to use. */
		vfs_uri = gnome_vfs_uri_new (uri);
		if (vfs_uri == NULL) {
			nautilus_directory_unref (directory);
			g_warning ("bad uri %s", uri);
			continue;
		}
		hash_table_list_prepend (added_lists, directory, vfs_uri);
		nautilus_directory_unref (directory);
	}


	/* Now get file info for the new files. This creates NautilusFile
	 * objects for the new files, and sends out a files_added signal. 
	 */
	g_hash_table_foreach (added_lists, call_get_file_info_free_list, NULL);
	g_hash_table_destroy (added_lists);

	/* Invalidate count for each parent directory. */
	g_hash_table_foreach (parent_directories, invalidate_count_and_unref, NULL);
	g_hash_table_destroy (parent_directories);
}

void
nautilus_directory_notify_files_removed (GList *uris)
{
	GHashTable *changed_lists;
	GList *p;
	NautilusDirectory *directory;
	GHashTable *parent_directories;
	const char *uri;
	NautilusFile *file;

	/* Make a list of changed files in each directory. */
	changed_lists = g_hash_table_new (NULL, NULL);

	/* Make a list of parent directories that will need their counts updated. */
	parent_directories = g_hash_table_new (NULL, NULL);

	/* Go through all the notifications. */
	for (p = uris; p != NULL; p = p->next) {
		uri = (const char *) p->data;

		/* Update file count for parent directory if anyone might care. */
		directory = get_parent_directory_if_exists (uri);
		if (directory != NULL) {
			collect_parent_directories (parent_directories, directory);
			nautilus_directory_unref (directory);
		}

		/* Find the file. */
		file = nautilus_file_get_existing (uri);
		if (file != NULL) {
			/* Mark it gone and prepare to send the changed signal. */
			nautilus_file_mark_gone (file);
			hash_table_list_prepend (changed_lists,
						 file->details->directory, file);
		}
	}

	/* Now send out the changed signals. */
	g_hash_table_foreach (changed_lists, call_files_changed_unref_free_list, NULL);
	g_hash_table_destroy (changed_lists);

	/* Invalidate count for each parent directory. */
	g_hash_table_foreach (parent_directories, invalidate_count_and_unref, NULL);
	g_hash_table_destroy (parent_directories);
}

static void
set_directory_uri (NautilusDirectory *directory,
		   const char *new_uri)
{
	GnomeVFSURI *new_vfs_uri;
	GnomeVFSURI *new_public_metafile_vfs_uri;
	GnomeVFSURI *new_private_metafile_vfs_uri;

	new_vfs_uri = gnome_vfs_uri_new (new_uri);
	new_public_metafile_vfs_uri = new_vfs_uri == NULL ? NULL
		: gnome_vfs_uri_append_file_name (new_vfs_uri, METAFILE_NAME);
	new_private_metafile_vfs_uri = construct_private_metafile_vfs_uri (new_uri);

	g_free (directory->details->uri);
	directory->details->uri = g_strdup (new_uri);
	
	if (directory->details->vfs_uri != NULL) {
		gnome_vfs_uri_unref (directory->details->vfs_uri);
	}
	directory->details->vfs_uri = new_vfs_uri;

	if (directory->details->public_metafile_vfs_uri != NULL) {
		gnome_vfs_uri_unref (directory->details->public_metafile_vfs_uri);
	}
	directory->details->public_metafile_vfs_uri =
		new_public_metafile_vfs_uri;

	if (directory->details->private_metafile_vfs_uri != NULL) {
		gnome_vfs_uri_unref (directory->details->private_metafile_vfs_uri);
	}
	directory->details->private_metafile_vfs_uri
		= new_private_metafile_vfs_uri;
}

static char *
get_path_from_vfs_uri (GnomeVFSURI *vfs_uri)
{
	char *uri, *path;

	if (vfs_uri == NULL) {
		return NULL;
	}

	uri = gnome_vfs_uri_to_string (vfs_uri, GNOME_VFS_URI_HIDE_NONE);
	if (uri == NULL) {
		return NULL;
	}
	path = gnome_vfs_get_local_path_from_uri (uri);
	g_free (uri);
	return path;
}

static char *
get_private_metafile_path (NautilusDirectory *directory)
{
	return get_path_from_vfs_uri (directory->details->private_metafile_vfs_uri);
}

static void
change_directory_uri (NautilusDirectory *directory,
		      const char *new_uri)
{
	char *old_metafile_path, *new_metafile_path;

	/* I believe it's impossible for a self-owned file/directory
	 * to be moved. But if that did somehow happen, this function
	 * wouldn't do enough to handle it.
	 */
	g_return_if_fail (directory->details->as_file == NULL);

	old_metafile_path = get_private_metafile_path (directory);

	g_hash_table_remove (directories,
			     directory->details->uri);

	set_directory_uri (directory, new_uri);

	g_hash_table_insert (directories,
			     directory->details->uri,
			     directory);

	new_metafile_path = get_private_metafile_path (directory);

	if (old_metafile_path != NULL && new_metafile_path != NULL) {
		rename (old_metafile_path, new_metafile_path);
	}

	g_free (old_metafile_path);
	g_free (new_metafile_path);
}

typedef struct {
	char *uri_prefix;
	GList *directories;
} CollectData;

static void
collect_directories_by_prefix (gpointer key, gpointer value, gpointer callback_data)
{
	const char *uri, *uri_suffix;
	NautilusDirectory *directory;
	CollectData *collect_data;

	uri = (const char *) key;
	directory = NAUTILUS_DIRECTORY (value);
	collect_data = (CollectData *) callback_data;
	
	if (nautilus_str_has_prefix (uri, collect_data->uri_prefix)) {
		uri_suffix = &uri[strlen (collect_data->uri_prefix)];
		switch (uri_suffix[0]) {
		case '\0':
		case '/':
			nautilus_directory_ref (directory);
			collect_data->directories =
				g_list_prepend (collect_data->directories,
						directory);
			break;
		}
	}
}

static char *
str_replace_prefix (const char *str,
		    const char *old_prefix,
		    const char *new_prefix)
{
	const char *old_suffix;

	g_return_val_if_fail (nautilus_str_has_prefix (str, old_prefix),
			      g_strdup (str));

	old_suffix = &str [strlen (old_prefix)];
	return g_strconcat (new_prefix, old_suffix, NULL);
}

void
nautilus_directory_moved (const char *old_uri,
			  const char *new_uri)
{
	char *canonical_old_uri, *canonical_new_uri;
	CollectData collection;
	NautilusDirectory *directory;
	char *new_directory_uri;
	GList *node;

	canonical_old_uri = nautilus_directory_make_uri_canonical (old_uri);
	canonical_new_uri = nautilus_directory_make_uri_canonical (new_uri);

	collection.uri_prefix = canonical_old_uri;
	collection.directories = NULL;

	g_hash_table_foreach (directories, collect_directories_by_prefix, &collection);

	for (node = collection.directories; node != NULL; node = node->next) {
		directory = NAUTILUS_DIRECTORY (node->data);
		new_directory_uri = str_replace_prefix (directory->details->uri,
							canonical_old_uri,
							canonical_new_uri);
		change_directory_uri (directory,
				      new_directory_uri);
		g_free (new_directory_uri);
		nautilus_directory_unref (directory);
	}

	g_list_free (collection.directories);

	g_free (canonical_old_uri);
	g_free (canonical_new_uri);
}

void
nautilus_directory_notify_files_moved (GList *uri_pairs)
{
	GList *p;
	URIPair *pair;
	NautilusFile *file;
	NautilusDirectory *old_directory, *new_directory;
	GHashTable *parent_directories;
	GList *new_files_list, *unref_list;
	GHashTable *added_lists, *changed_lists;
	char *name;

	/* Make a list of added and changed files in each directory. */
	new_files_list = NULL;
	added_lists = g_hash_table_new (NULL, NULL);
	changed_lists = g_hash_table_new (NULL, NULL);
	unref_list = NULL;

	/* Make a list of parent directories that will need their counts updated. */
	parent_directories = g_hash_table_new (NULL, NULL);

	for (p = uri_pairs; p != NULL; p = p->next) {
		pair = p->data;

		/* Handle overwriting a file. */
		file = nautilus_file_get_existing (pair->to_uri);
		if (file != NULL) {
			/* Mark it gone and prepare to send the changed signal. */
			nautilus_file_mark_gone (file);
			new_directory = file->details->directory;
			hash_table_list_prepend (changed_lists,
						 new_directory,
						 file);
			collect_parent_directories (parent_directories,
						    new_directory);
		}

		/* Update any directory objects that are affected. */
		nautilus_directory_moved (pair->from_uri, pair->to_uri);

		/* Move an existing file. */
		file = nautilus_file_get_existing (pair->from_uri);
		if (file == NULL) {
			/* Handle this as if it was a new file. */
			new_files_list = g_list_prepend (new_files_list,
							 pair->to_uri);
		} else {
			/* Handle notification in the old directory. */
			old_directory = file->details->directory;
			collect_parent_directories (parent_directories, old_directory);

			/* Locate the new directory. */
			new_directory = get_parent_directory (pair->to_uri);
			collect_parent_directories (parent_directories, new_directory);
			/* We can unref now -- new_directory is in the
			 * parent directories list so it will be
			 * around until the end of this function
			 * anyway.
			 */
			nautilus_directory_unref (new_directory);

			/* Update the file's name. */
			name = nautilus_uri_get_basename (pair->to_uri);
			nautilus_file_update_name (file, name);
			g_free (name);

			/* Update the file's directory. */
			nautilus_file_set_directory (file, new_directory);
			
			hash_table_list_prepend
				(changed_lists, old_directory, file);
			if (old_directory != new_directory) {
				hash_table_list_prepend
					(added_lists, new_directory, file);
			}

			/* Unref each file once to balance out nautilus_file_get. */
			unref_list = g_list_prepend (unref_list, file);
		}
	}

	/* Now send out the changed and added signals for existing file objects. */
	g_hash_table_foreach (changed_lists, call_files_changed_free_list, NULL);
	g_hash_table_destroy (changed_lists);
	g_hash_table_foreach (added_lists, call_files_added_free_list, NULL);
	g_hash_table_destroy (added_lists);

	/* Let the file objects go. */
	nautilus_file_list_free (unref_list);

	/* Invalidate count for each parent directory. */
	g_hash_table_foreach (parent_directories, invalidate_count_and_unref, NULL);
	g_hash_table_destroy (parent_directories);

	/* Separate handling for brand new file objects. */
	nautilus_directory_notify_files_added (new_files_list);
	g_list_free (new_files_list);
}

void 
nautilus_directory_schedule_metadata_copy (GList *uri_pairs)
{
	GList *p;
	URIPair *pair;
	NautilusDirectory *source_directory, *destination_directory;
	const char *source_relative_uri, *destination_relative_uri;

	for (p = uri_pairs; p != NULL; p = p->next) {
		pair = (URIPair *) p->data;

		source_directory = get_parent_directory (pair->from_uri);
		destination_directory = get_parent_directory (pair->to_uri);
		
		source_relative_uri = g_basename (pair->from_uri);
		destination_relative_uri = g_basename (pair->to_uri);
		
		nautilus_directory_copy_file_metadata (source_directory,
						       source_relative_uri,
						       destination_directory,
						       destination_relative_uri);
		
		nautilus_directory_unref (source_directory);
		nautilus_directory_unref (destination_directory);
	}
}

void 
nautilus_directory_schedule_metadata_move (GList *uri_pairs)
{
	GList *p;
	URIPair *pair;
	NautilusDirectory *source_directory, *destination_directory;
	const char *source_relative_uri, *destination_relative_uri;

	for (p = uri_pairs; p != NULL; p = p->next) {
		pair = (URIPair *) p->data;

		source_directory = get_parent_directory (pair->from_uri);
		destination_directory = get_parent_directory (pair->to_uri);
		
		source_relative_uri = g_basename (pair->from_uri);
		destination_relative_uri = g_basename (pair->to_uri);
		
		nautilus_directory_copy_file_metadata (source_directory,
						       source_relative_uri,
						       destination_directory,
						       destination_relative_uri);
		nautilus_directory_remove_file_metadata (source_directory,
							 source_relative_uri);
		
		nautilus_directory_unref (source_directory);
		nautilus_directory_unref (destination_directory);
	}
}

void 
nautilus_directory_schedule_metadata_remove (GList *uris)
{
	GList *p;
	const char *uri;
	NautilusDirectory *directory;
	const char *relative_uri;

	for (p = uris; p != NULL; p = p->next) {
		uri = (const char *) p->data;

		directory = get_parent_directory (uri);
		relative_uri = g_basename (uri);
		
		nautilus_directory_remove_file_metadata (directory,
							 relative_uri);
		
		nautilus_directory_unref (directory);
	}
}

void
nautilus_directory_schedule_position_setting (GList *position_setting_list)
{
	GList *p;
	const NautilusFileChangesQueuePositionSetting *item;
	NautilusFile *file;
	char *position_string;

	for (p = position_setting_list; p != NULL; p = p->next) {
		item = (const NautilusFileChangesQueuePositionSetting *) p->data;

		file = nautilus_file_get (item->uri);
		
		position_string = g_strdup_printf ("%d,%d", item->point.x, item->point.y);
		nautilus_file_set_metadata
			(file,
			 NAUTILUS_METADATA_KEY_ICON_POSITION,
			 NULL,
			 position_string);
		g_free (position_string);
		
		nautilus_file_unref (file);
	}
}


gboolean
nautilus_directory_contains_file (NautilusDirectory *directory,
				  NautilusFile *file)
{
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), FALSE);
	g_return_val_if_fail (NAUTILUS_IS_FILE (file), FALSE);

	if (nautilus_file_is_gone (file)) {
		return FALSE;
	}

	return NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 contains_file, (directory, file));
}

char *
nautilus_directory_get_file_uri (NautilusDirectory *directory,
				 const char *file_name)
{
	GnomeVFSURI *directory_uri, *file_uri;
	char *result;

	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), NULL);
	g_return_val_if_fail (file_name != NULL, NULL);

	result = NULL;

	directory_uri = gnome_vfs_uri_new (directory->details->uri);

	g_assert (directory_uri != NULL);

	file_uri = gnome_vfs_uri_append_string (directory_uri, file_name);
	gnome_vfs_uri_unref (directory_uri);

	if (file_uri != NULL) {
		result = gnome_vfs_uri_to_string (file_uri, GNOME_VFS_URI_HIDE_NONE);
		gnome_vfs_uri_unref (file_uri);
	}

	return result;
}

void
nautilus_directory_call_when_ready (NautilusDirectory *directory,
				    GList *file_attributes,
				    NautilusDirectoryCallback callback,
				    gpointer callback_data)
{
	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));
	g_return_if_fail (callback != NULL);

	NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 call_when_ready, (directory, file_attributes,
				   callback, callback_data));
}


void
nautilus_directory_cancel_callback (NautilusDirectory *directory,
				    NautilusDirectoryCallback callback,
				    gpointer callback_data)
{
	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));
	g_return_if_fail (callback != NULL);

	NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 cancel_callback, (directory, callback, callback_data));
}

void
nautilus_directory_file_monitor_add (NautilusDirectory *directory,
				     gconstpointer client,
				     gboolean monitor_hidden_files,
				     gboolean monitor_backup_files,
				     GList *file_attributes,
				     gboolean force_reload)
{
	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));
	g_return_if_fail (client != NULL);

	NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 file_monitor_add, (directory, client,
				    monitor_hidden_files,
				    monitor_backup_files,
				    file_attributes,
				    force_reload));
}

void
nautilus_directory_file_monitor_remove (NautilusDirectory *directory,
					gconstpointer client)
{
	g_return_if_fail (NAUTILUS_IS_DIRECTORY (directory));
	g_return_if_fail (client != NULL);

	NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 file_monitor_remove, (directory, client));
}

gboolean
nautilus_directory_is_not_empty (NautilusDirectory *directory)
{
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), FALSE);

	return NAUTILUS_CALL_VIRTUAL
		(NAUTILUS_DIRECTORY_CLASS, directory,
		 is_not_empty, (directory));
}

#if !defined (NAUTILUS_OMIT_SELF_CHECK)

#include "nautilus-debug.h"
#include "nautilus-file-attributes.h"

static int data_dummy;
static gboolean got_metadata_flag;
static gboolean got_files_flag;

static void
got_metadata_callback (NautilusDirectory *directory, GList *files, gpointer callback_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (callback_data == &data_dummy);

	got_metadata_flag = TRUE;
}

static void
got_files_callback (NautilusDirectory *directory, GList *files, gpointer callback_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (g_list_length (files) > 10);
	g_assert (callback_data == &data_dummy);

	got_files_flag = TRUE;
}

/* Return the number of extant NautilusDirectories */
int
nautilus_directory_number_outstanding (void)
{
        return directories ? g_hash_table_size (directories) : 0;
}

void
nautilus_self_check_directory (void)
{
	NautilusDirectory *directory;
	NautilusFile *file;
	GList *attributes;

	directory = nautilus_directory_get ("file:///etc");
	file = nautilus_file_get ("file:///etc/passwd");

	NAUTILUS_CHECK_INTEGER_RESULT (g_hash_table_size (directories), 1);

	nautilus_directory_file_monitor_add
		(directory, &data_dummy,
		 TRUE, TRUE,
		 NULL, FALSE);

	got_metadata_flag = FALSE;

	attributes = g_list_append (NULL, NAUTILUS_FILE_ATTRIBUTE_METADATA);
	nautilus_directory_call_when_ready (directory, attributes,
					    got_metadata_callback, &data_dummy);
	g_list_free (attributes);

	while (!got_metadata_flag) {
		gtk_main_iteration ();
	}

	nautilus_file_set_metadata (file, "TEST", "default", "value");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_file_get_metadata (file, "TEST", "default"), "value");

	nautilus_file_set_boolean_metadata (file, "TEST_BOOLEAN", TRUE, TRUE);
	NAUTILUS_CHECK_BOOLEAN_RESULT (nautilus_file_get_boolean_metadata (file, "TEST_BOOLEAN", TRUE), TRUE);
	nautilus_file_set_boolean_metadata (file, "TEST_BOOLEAN", TRUE, FALSE);
	NAUTILUS_CHECK_BOOLEAN_RESULT (nautilus_file_get_boolean_metadata (file, "TEST_BOOLEAN", TRUE), FALSE);
	NAUTILUS_CHECK_BOOLEAN_RESULT (nautilus_file_get_boolean_metadata (NULL, "TEST_BOOLEAN", TRUE), TRUE);

	nautilus_file_set_integer_metadata (file, "TEST_INTEGER", 0, 17);
	NAUTILUS_CHECK_INTEGER_RESULT (nautilus_file_get_integer_metadata (file, "TEST_INTEGER", 0), 17);
	nautilus_file_set_integer_metadata (file, "TEST_INTEGER", 0, -1);
	NAUTILUS_CHECK_INTEGER_RESULT (nautilus_file_get_integer_metadata (file, "TEST_INTEGER", 0), -1);
	nautilus_file_set_integer_metadata (file, "TEST_INTEGER", 42, 42);
	NAUTILUS_CHECK_INTEGER_RESULT (nautilus_file_get_integer_metadata (file, "TEST_INTEGER", 42), 42);
	NAUTILUS_CHECK_INTEGER_RESULT (nautilus_file_get_integer_metadata (NULL, "TEST_INTEGER", 42), 42);
	NAUTILUS_CHECK_INTEGER_RESULT (nautilus_file_get_integer_metadata (file, "NONEXISTENT_KEY", 42), 42);

	NAUTILUS_CHECK_BOOLEAN_RESULT (nautilus_directory_get ("file:///etc") == directory, TRUE);
	nautilus_directory_unref (directory);

	NAUTILUS_CHECK_BOOLEAN_RESULT (nautilus_directory_get ("file:///etc/") == directory, TRUE);
	nautilus_directory_unref (directory);

	NAUTILUS_CHECK_BOOLEAN_RESULT (nautilus_directory_get ("file:///etc////") == directory, TRUE);
	nautilus_directory_unref (directory);

	nautilus_file_unref (file);

	nautilus_directory_file_monitor_remove (directory, &data_dummy);

	nautilus_directory_unref (directory);

	while (g_hash_table_size (directories) != 0) {
		gtk_main_iteration ();
	}

	NAUTILUS_CHECK_INTEGER_RESULT (g_hash_table_size (directories), 0);

	directory = nautilus_directory_get ("file:///etc");

	got_metadata_flag = FALSE;
	attributes = g_list_append (NULL, NAUTILUS_FILE_ATTRIBUTE_METADATA);
	nautilus_directory_call_when_ready (directory, attributes,
					    got_metadata_callback, &data_dummy);
	g_list_free (attributes);

	while (!got_metadata_flag) {
		gtk_main_iteration ();
	}

	NAUTILUS_CHECK_BOOLEAN_RESULT (directory->details->metafile != NULL, TRUE);

	got_files_flag = FALSE;

	attributes = g_list_prepend (NULL, NAUTILUS_FILE_ATTRIBUTE_MIME_TYPE);
	attributes = g_list_prepend (attributes, NAUTILUS_FILE_ATTRIBUTE_DEEP_COUNTS);
	nautilus_directory_call_when_ready (directory, attributes,
					    got_files_callback, &data_dummy);
	g_list_free (attributes);

	while (!got_files_flag) {
		gtk_main_iteration ();
	}

	NAUTILUS_CHECK_BOOLEAN_RESULT (directory->details->file_list == NULL, TRUE);

	NAUTILUS_CHECK_INTEGER_RESULT (g_hash_table_size (directories), 1);

	file = nautilus_file_get ("file:///etc/passwd");

	NAUTILUS_CHECK_STRING_RESULT (nautilus_file_get_metadata (file, "TEST", "default"), "value");
	
	nautilus_file_unref (file);

	nautilus_directory_unref (directory);

	NAUTILUS_CHECK_INTEGER_RESULT (g_hash_table_size (directories), 0);

	/* escape_slashes: code is now in gnome-vfs, but lets keep the tests here for now */
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes (""), "");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("a"), "a");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("/"), "%2F");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("%"), "%25");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("a/a"), "a%2Fa");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("a%a"), "a%25a");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("%25"), "%2525");
	NAUTILUS_CHECK_STRING_RESULT (gnome_vfs_escape_slashes ("%2F"), "%252F");

	/* nautilus_directory_make_uri_canonical */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical (""), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("file:/"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("file:///"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("TRASH:XXX"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("trash:xxx"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("GNOME-TRASH:XXX"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("gnome-trash:xxx"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("file:///home/mathieu/"), "file:///home/mathieu");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("file:///home/mathieu"), "file:///home/mathieu");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("ftp://mathieu:password@le-hackeur.org"), "ftp://mathieu:password@le-hackeur.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("ftp://mathieu:password@le-hackeur.org/"), "ftp://mathieu:password@le-hackeur.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("http://le-hackeur.org"), "http://le-hackeur.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("http://le-hackeur.org/"), "http://le-hackeur.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("http://le-hackeur.org/dir"), "http://le-hackeur.org/dir");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("http://le-hackeur.org/dir/"), "http://le-hackeur.org/dir");
	/* FIXME bugzilla.eazel.com 5068: the "nested" URI loses some characters here. Maybe that's OK because we escape them in practice? */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("search://[file://]file_name contains stuff"), "search://[file/]file_name contains stuff");
#ifdef EAZEL_SERVICES
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("eazel-services:/~turtle"), "eazel-services:///~turtle");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_directory_make_uri_canonical ("eazel-services:///~turtle"), "eazel-services:///~turtle");
#endif	
}

#endif /* !NAUTILUS_OMIT_SELF_CHECK */
