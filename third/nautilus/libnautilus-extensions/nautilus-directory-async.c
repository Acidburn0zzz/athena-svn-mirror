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

#include "nautilus-directory-metafile.h"
#include "nautilus-directory-notify.h"
#include "nautilus-directory-private.h"
#include "nautilus-file-attributes.h"
#include "nautilus-file-private.h"
#include "nautilus-glib-extensions.h"
#include "nautilus-global-preferences.h"
#include "nautilus-link.h"
#include "nautilus-search-uri.h"
#include "nautilus-string.h"
#include <ctype.h>
#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>
#include <gtk/gtkmain.h>
#include <stdlib.h>
#include <stdio.h>

/* turn this on to see messages about each load_directory call: */
#if 0
#define DEBUG_LOAD_DIRECTORY
#endif

/* turn this on to check if async. job calls are balanced */
#if 0
#define DEBUG_ASYNC_JOBS
#endif

#define METAFILE_PERMISSIONS (GNOME_VFS_PERM_USER_READ | GNOME_VFS_PERM_USER_WRITE \
			      | GNOME_VFS_PERM_GROUP_READ | GNOME_VFS_PERM_GROUP_WRITE \
			      | GNOME_VFS_PERM_OTHER_READ | GNOME_VFS_PERM_OTHER_WRITE)

#define DIRECTORY_LOAD_ITEMS_PER_CALLBACK 32

/* Keep async. jobs down to this number for all directories. */
#define MAX_ASYNC_JOBS 10

struct MetafileReadState {
	gboolean use_public_metafile;
	NautilusReadFileHandle *handle;
	GnomeVFSAsyncHandle *get_file_info_handle;
};

struct MetafileWriteState {
	gboolean use_public_metafile;
	GnomeVFSAsyncHandle *handle;
	xmlChar *buffer;
	GnomeVFSFileSize size;
	gboolean write_again;
};

struct TopLeftTextReadState {
	NautilusFile *file;
	NautilusReadFileHandle *handle;
};

struct ActivationURIReadState {
	NautilusFile *file;
	NautilusReadFileHandle *handle;
};

typedef struct {
	NautilusFile *file; /* Which file, NULL means all. */
	union {
		NautilusDirectoryCallback directory;
		NautilusFileCallback file;
	} callback;
	gpointer callback_data;
	Request request;
} ReadyCallback;

typedef struct {
	NautilusFile *file; /* Which file, NULL means all. */
	gboolean monitor_hidden_files; /* defines whether "all" includes hidden files */
	gboolean monitor_backup_files; /* defines whether "all" includes backup files */
	gconstpointer client;
	Request request;
} Monitor;

typedef gboolean (* RequestCheck) (const Request *);
typedef gboolean (* FileCheck) (NautilusFile *);

/* Current number of async. jobs. */
static int async_job_count;
static GHashTable *waiting_directories;
#ifdef DEBUG_ASYNC_JOBS
static GHashTable *async_jobs;
#endif

/* Forward declarations for functions that need them. */
static void     deep_count_load           (NautilusDirectory *directory,
					   const char        *uri);
static void     metafile_read_restart     (NautilusDirectory *directory);
static gboolean request_is_satisfied      (NautilusDirectory *directory,
					   NautilusFile      *file,
					   Request           *request);
static void     cancel_loading_attributes (NautilusDirectory *directory,
					   GList             *file_attributes);

/* Some helpers for case-insensitive strings.
 * Move to nautilus-glib-extensions?
 */

static gboolean
istr_equal (gconstpointer v, gconstpointer v2)
{
	return g_strcasecmp (v, v2) == 0;
}

static guint
istr_hash (gconstpointer key)
{
	const char *p;
	guint h;

	h = 0;
	for (p = key; *p != '\0'; p++) {
		h = (h << 5) - h + tolower ((guchar) *p);
	}
	
	return h;
}

static GHashTable *
istr_set_new (void)
{
	return g_hash_table_new (istr_hash, istr_equal);
}

static void
istr_set_insert (GHashTable *table, const char *istr)
{
	char *key;

	if (g_hash_table_lookup (table, istr) == NULL) {
		key = g_strdup (istr);
		g_hash_table_insert (table, key, key);
	}
}

static void
add_istr_to_list (gpointer key, gpointer value, gpointer callback_data)
{
	GList **list;

	list = callback_data;
	*list = g_list_prepend (*list, g_strdup (key));
}

static GList *
istr_set_get_as_list (GHashTable *table)
{
	GList *list;

	list = NULL;
	g_hash_table_foreach (table, add_istr_to_list, &list);
	return list;
}

static void
istr_set_destroy (GHashTable *table)
{
	nautilus_g_hash_table_destroy_deep (table);
}

/* Start a job. This is really just a way of limiting the number of
 * async. requests that we issue at any given time. Without this, the
 * number of requests is unbounded.
 */
static gboolean
async_job_start (NautilusDirectory *directory,
		 const char *job)
{
#ifdef DEBUG_ASYNC_JOBS
	char *key;
#endif

	g_assert (async_job_count >= 0);
	g_assert (async_job_count <= MAX_ASYNC_JOBS);

	if (async_job_count >= MAX_ASYNC_JOBS) {
		if (waiting_directories == NULL) {
			waiting_directories = nautilus_g_hash_table_new_free_at_exit
				(NULL, NULL,
				 "nautilus-directory-async.c: waiting_directories");
		}

		g_hash_table_insert (waiting_directories,
				     directory,
				     directory);
		
		return FALSE;
	}

#ifdef DEBUG_ASYNC_JOBS
	if (async_jobs == NULL) {
		async_jobs = nautilus_g_hash_table_new_free_at_exit
			(g_str_hash, g_str_equal,
			 "nautilus-directory-async.c: async_jobs");
	}
	key = g_strconcat (directory->details->uri, ": ", job, NULL);
	if (g_hash_table_lookup (async_jobs, key) != NULL) {
		g_warning ("same job twice: %s in %s",
			   job,
			   directory->details->uri);
	}
	g_hash_table_insert (async_jobs, key, directory);
#endif	

	async_job_count += 1;
	return TRUE;
}

/* End a job. */
static void
async_job_end (NautilusDirectory *directory,
	       const char *job)
{
#ifdef DEBUG_ASYNC_JOBS
	char *key;
	gpointer table_key, value;
#endif

	g_assert (async_job_count > 0);

#ifdef DEBUG_ASYNC_JOBS
	g_assert (async_jobs != NULL);
	key = g_strconcat (directory->details->uri, ": ", job, NULL);
	if (!g_hash_table_lookup_extended (async_jobs, key, &table_key, &value)) {
		g_warning ("ending job we didn't start: %s in %s",
			   job,
			   directory->details->uri);
	} else {
		g_hash_table_remove (async_jobs, key);
		g_free (table_key);
	}
	g_free (key);
#endif

	async_job_count -= 1;
}

/* Helper to get one value from a hash table. */
static void
get_one_value_callback (gpointer key, gpointer value, gpointer callback_data)
{
	gpointer *returned_value;

	returned_value = callback_data;
	*returned_value = value;
}

/* return a single value from a hash table. */
static gpointer
get_one_value (GHashTable *table)
{
	gpointer value;

	value = NULL;
	if (table != NULL) {
		g_hash_table_foreach (table, get_one_value_callback, &value);
	}
	return value;
}

/* Wake up directories that are "blocked" as long as there are job
 * slots available.
 */
static void
async_job_wake_up (void)
{
	static gboolean already_waking_up = FALSE;
	gpointer value;

	g_assert (async_job_count >= 0);
	g_assert (async_job_count <= MAX_ASYNC_JOBS);

	if (already_waking_up) {
		return;
	}
	
	already_waking_up = TRUE;
	while (async_job_count < MAX_ASYNC_JOBS) {
		value = get_one_value (waiting_directories);
		if (value == NULL) {
			break;
		}
		g_hash_table_remove (waiting_directories, value);
		nautilus_directory_async_state_changed
			(NAUTILUS_DIRECTORY (value));
	}
	already_waking_up = FALSE;
}

static void
directory_count_cancel (NautilusDirectory *directory)
{
	if (directory->details->count_in_progress != NULL) {
		gnome_vfs_async_cancel (directory->details->count_in_progress);
		directory->details->count_file = NULL;
		directory->details->count_in_progress = NULL;

		async_job_end (directory, "directory count");
	}
}

static void
deep_count_cancel (NautilusDirectory *directory)
{
	if (directory->details->deep_count_in_progress != NULL) {
		g_assert (NAUTILUS_IS_FILE (directory->details->deep_count_file));

		gnome_vfs_async_cancel (directory->details->deep_count_in_progress);

		directory->details->deep_count_file->details->deep_counts_status = NAUTILUS_REQUEST_NOT_STARTED;

		directory->details->deep_count_file = NULL;
		directory->details->deep_count_in_progress = NULL;
		g_free (directory->details->deep_count_uri);
		directory->details->deep_count_uri = NULL;
		nautilus_g_list_free_deep (directory->details->deep_count_subdirectories);
		directory->details->deep_count_subdirectories = NULL;

		async_job_end (directory, "deep count");
	}
}

static void
mime_list_cancel (NautilusDirectory *directory)
{
	if (directory->details->mime_list_in_progress != NULL) {
		g_assert (NAUTILUS_IS_FILE (directory->details->mime_list_file));

		gnome_vfs_async_cancel (directory->details->mime_list_in_progress);
		istr_set_destroy (directory->details->mime_list_hash);

		directory->details->mime_list_file = NULL;
		directory->details->mime_list_in_progress = NULL;
		directory->details->mime_list_hash = NULL;

		async_job_end (directory, "MIME list");
	}
}

static void
top_left_cancel (NautilusDirectory *directory)
{
	if (directory->details->top_left_read_state != NULL) {
		nautilus_read_file_cancel (directory->details->top_left_read_state->handle);
		g_free (directory->details->top_left_read_state);
		directory->details->top_left_read_state = NULL;

		async_job_end (directory, "top left");
	}
}

static void
activation_uri_cancel (NautilusDirectory *directory)
{
	if (directory->details->activation_uri_read_state != NULL) {
		nautilus_read_file_cancel (directory->details->activation_uri_read_state->handle);
		g_free (directory->details->activation_uri_read_state);
		directory->details->activation_uri_read_state = NULL;

		async_job_end (directory, "activation URI");
	}
}

static void
file_info_cancel (NautilusDirectory *directory)
{
	if (directory->details->get_info_in_progress != NULL) {
		gnome_vfs_async_cancel (directory->details->get_info_in_progress);
		directory->details->get_info_file = NULL;
		directory->details->get_info_in_progress = NULL;

		async_job_end (directory, "file info");
	}
}

static void
metafile_read_cancel (NautilusDirectory *directory)
{
	if (directory->details->metafile_read_state != NULL) {
		if (directory->details->metafile_read_state->handle != NULL) {
			nautilus_read_file_cancel (directory->details->metafile_read_state->handle);
		}
		if (directory->details->metafile_read_state->get_file_info_handle != NULL) {
			gnome_vfs_async_cancel (directory->details->metafile_read_state->get_file_info_handle);
		}
		g_free (directory->details->metafile_read_state);
		directory->details->metafile_read_state = NULL;

		async_job_end (directory, "metafile read");
	}
}

static gboolean
can_use_public_metafile (NautilusDirectory *directory)
{
	NautilusSpeedTradeoffValue preference_value;
	
	g_return_val_if_fail (NAUTILUS_IS_DIRECTORY (directory), FALSE);

	if (directory->details->public_metafile_vfs_uri == NULL) {
		return FALSE;
	}

	preference_value = nautilus_preferences_get_integer (NAUTILUS_PREFERENCES_USE_PUBLIC_METADATA);

	if (preference_value == NAUTILUS_SPEED_TRADEOFF_ALWAYS) {
		return TRUE;
	}
	
	if (preference_value == NAUTILUS_SPEED_TRADEOFF_NEVER) {
		return FALSE;
	}

	g_assert (preference_value == NAUTILUS_SPEED_TRADEOFF_LOCAL_ONLY);
	return nautilus_directory_is_local (directory);
}

static void
metafile_read_mark_done (NautilusDirectory *directory)
{
	g_free (directory->details->metafile_read_state);
	directory->details->metafile_read_state = NULL;	

	directory->details->metafile_read = TRUE;

	/* Move over the changes to the metafile that were in the hash table. */
	nautilus_directory_metafile_apply_pending_changes (directory);

	/* Tell change-watchers that we have update information. */
	nautilus_directory_emit_metadata_changed (directory);

	/* Let the callers that were waiting for the metafile know. */
	nautilus_directory_async_state_changed (directory);
}

static void
metafile_read_done (NautilusDirectory *directory)
{
	async_job_end (directory, "metafile read");
	metafile_read_mark_done (directory);
}

static void
metafile_read_try_public_metafile (NautilusDirectory *directory)
{
	directory->details->metafile_read_state->use_public_metafile = TRUE;
	metafile_read_restart (directory);
}

static void
metafile_read_check_for_directory_callback (GnomeVFSAsyncHandle *handle,
					    GList *results,
					    gpointer callback_data)
{
	NautilusDirectory *directory;
	GnomeVFSGetFileInfoResult *result;

	directory = NAUTILUS_DIRECTORY (callback_data);

	g_assert (directory->details->metafile_read_state->get_file_info_handle == handle);
	g_assert (nautilus_g_list_exactly_one_item (results));

	directory->details->metafile_read_state->get_file_info_handle = NULL;

	result = results->data;

	if (result->result == GNOME_VFS_OK
	    && ((result->file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) != 0)
	    && result->file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		/* Is a directory. */
		metafile_read_try_public_metafile (directory);
	} else {
		/* Not a directory. */
		metafile_read_done (directory);
	}
}

static void
metafile_read_check_for_directory (NautilusDirectory *directory)
{
	GList fake_list;
	
	/* We only get here if the public metafile is in question,
	 * which in turn only happens if the URI is one that gnome-vfs
	 * can handle.
	 */
	g_assert (directory->details->vfs_uri != NULL);

	/* We have to do a get_info call to check if this a directory. */
	fake_list.data = directory->details->vfs_uri;
	fake_list.next = NULL;
	fake_list.prev = NULL;
	gnome_vfs_async_get_file_info
		(&directory->details->metafile_read_state->get_file_info_handle,
		 &fake_list,
		 GNOME_VFS_FILE_INFO_DEFAULT,
		 metafile_read_check_for_directory_callback,
		 directory);
}

static void
metafile_read_failed (NautilusDirectory *directory)
{
	NautilusFile *file;
	gboolean need_directory_check, is_directory;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (directory->details->metafile == NULL);

	directory->details->metafile_read_state->handle = NULL;

	if (!directory->details->metafile_read_state->use_public_metafile
	    && can_use_public_metafile (directory)) {
		/* The goal here is to read the real metafile, but
		 * only if the directory is actually a directory.
		 */

		/* First, check if we already know if it a directory. */
		file = nautilus_file_get (directory->details->uri);
		if (file == NULL || file->details->is_gone) {
			need_directory_check = FALSE;
			is_directory = FALSE;
		} else if (file->details->info == NULL) {
			need_directory_check = TRUE;
			is_directory = TRUE;
		} else {
			need_directory_check = FALSE;
			is_directory = nautilus_file_is_directory (file);
		}
		nautilus_file_unref (file);

		/* Do the directory check if we don't know. */
		if (need_directory_check) {
			metafile_read_check_for_directory (directory);
			return;
		}

		/* Try for the public metafile if it is a directory. */
		if (is_directory) {
			metafile_read_try_public_metafile (directory);
			return;
		}
	}

	metafile_read_done (directory);
}

static void
metafile_read_done_callback (GnomeVFSResult result,
			     GnomeVFSFileSize file_size,
			     char *file_contents,
			     gpointer callback_data)
{
	NautilusDirectory *directory;
	int size;
	char *buffer;

	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (directory->details->metafile == NULL);

	if (result != GNOME_VFS_OK) {
		g_assert (file_contents == NULL);
		metafile_read_failed (directory);
		return;
	}
	
	size = file_size;
	if ((GnomeVFSFileSize) size != file_size) {
		g_free (file_contents);
		metafile_read_failed (directory);
		return;
	}

	/* The gnome-xml parser requires a zero-terminated array. */
	buffer = g_realloc (file_contents, size + 1);
	buffer[size] = '\0';
	nautilus_directory_set_metafile_contents (directory,
						  xmlParseMemory (buffer, size));
	g_free (buffer);

	metafile_read_done (directory);
}

static void
metafile_read_restart (NautilusDirectory *directory)
{
	char *text_uri;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	text_uri = gnome_vfs_uri_to_string
		(directory->details->metafile_read_state->use_public_metafile
		 ? directory->details->public_metafile_vfs_uri
		 : directory->details->private_metafile_vfs_uri,
		 GNOME_VFS_URI_HIDE_NONE);

	directory->details->metafile_read_state->handle = nautilus_read_entire_file_async
		(text_uri, metafile_read_done_callback, directory);

	g_free (text_uri);
}

static gboolean
allow_metafile (NautilusDirectory *directory)
{
	const char *uri;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	/* Note that this inhibits both reading and writing metadata
	 * completely. In the future we may want to inhibit writing to
	 * the real directory while allowing parallel-directory
	 * metadata.
	 */

	/* For now, hard-code these schemes. Perhaps we should
	 * hardcode the schemes that are good for metadata instead of
	 * the schemes that are bad for it.
	 */
	/* FIXME bugzilla.eazel.com 2434: 
	 * We need to handle this in a better way. Perhaps a
	 * better way can wait until we have support for metadata
	 * access inside gnome-vfs.
	 */
	uri = directory->details->uri;
	if (nautilus_is_search_uri (uri)
	    || nautilus_istr_has_prefix (uri, "ghelp:")
	    || nautilus_istr_has_prefix (uri, "gnome-help:")
	    || nautilus_istr_has_prefix (uri, "help:")
	    || nautilus_istr_has_prefix (uri, "info:")
	    || nautilus_istr_has_prefix (uri, "man:")
	    || nautilus_istr_has_prefix (uri, "pipe:")
	    ) {
		return FALSE;
	}
	
	return TRUE;
}

/* This checks if there's a request for the metafile contents. */
static gboolean
is_anyone_waiting_for_metafile (NautilusDirectory *directory)
{
	GList *node;
	ReadyCallback *callback;
	Monitor *monitor;	

	for (node = directory->details->call_when_ready_list; node != NULL; node = node->next) {
		callback = node->data;
		if (callback->request.metafile) {
			return TRUE;
		}
	}

	for (node = directory->details->monitor_list; node != NULL; node = node->next) {
		monitor = node->data;
		if (monitor->request.metafile) {
			return TRUE;
		}
	}	

	return FALSE;
}

static void
metafile_read_start (NautilusDirectory *directory)
{
	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	if (directory->details->metafile_read
	    || directory->details->metafile_read_state != NULL) {
		return;
	}

	g_assert (directory->details->metafile == NULL);

	if (!is_anyone_waiting_for_metafile (directory)) {
		return;
	}

	if (!allow_metafile (directory)) {
		metafile_read_mark_done (directory);
	} else {
		if (!async_job_start (directory, "metafile read")) {
			return;
		}
		directory->details->metafile_read_state = g_new0 (MetafileReadState, 1);
		metafile_read_restart (directory);
	}
}

static void
metafile_write_done (NautilusDirectory *directory)
{
	if (directory->details->metafile_write_state->write_again) {
		nautilus_metafile_write_start (directory);
		return;
	}

	xmlFree (directory->details->metafile_write_state->buffer);
	g_free (directory->details->metafile_write_state);
	directory->details->metafile_write_state = NULL;
	nautilus_directory_unref (directory);
}

static void
metafile_write_failed (NautilusDirectory *directory)
{
	if (directory->details->metafile_write_state->use_public_metafile) {
		directory->details->metafile_write_state->use_public_metafile = FALSE;
		nautilus_metafile_write_start (directory);
		return;
	}

	metafile_write_done (directory);
}

static void
metafile_write_failure_close_callback (GnomeVFSAsyncHandle *handle,
				       GnomeVFSResult result,
				       gpointer callback_data)
{
	NautilusDirectory *directory;

	directory = NAUTILUS_DIRECTORY (callback_data);

	metafile_write_failed (directory);
}

static void
metafile_write_success_close_callback (GnomeVFSAsyncHandle *handle,
				       GnomeVFSResult result,
				       gpointer callback_data)
{
	NautilusDirectory *directory;

	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (directory->details->metafile_write_state->handle == NULL);

	if (result != GNOME_VFS_OK) {
		metafile_write_failed (directory);
		return;
	}

	/* Now that we have finished writing, it is time to delete the
	 * private file if we wrote the public one.
	 */
	if (directory->details->metafile_write_state->use_public_metafile) {
		/* A synchronous unlink is OK here because the private
		 * metafiles are local, so an unlink is very fast.
		 */
		gnome_vfs_unlink_from_uri (directory->details->private_metafile_vfs_uri);
	}

	metafile_write_done (directory);
}

static void
metafile_write_callback (GnomeVFSAsyncHandle *handle,
			 GnomeVFSResult result,
			 gconstpointer buffer,
			 GnomeVFSFileSize bytes_requested,
			 GnomeVFSFileSize bytes_read,
			 gpointer callback_data)
{
	NautilusDirectory *directory;

	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (directory->details->metafile_write_state->handle == handle);
	g_assert (directory->details->metafile_write_state->buffer == buffer);
	g_assert (directory->details->metafile_write_state->size == bytes_requested);

	g_assert (directory->details->metafile_write_state->handle != NULL);
	gnome_vfs_async_close (directory->details->metafile_write_state->handle,
			       result == GNOME_VFS_OK
			       ? metafile_write_success_close_callback
			       : metafile_write_failure_close_callback,
			       directory);
	directory->details->metafile_write_state->handle = NULL;
}

static void
metafile_write_create_callback (GnomeVFSAsyncHandle *handle,
				GnomeVFSResult result,
				gpointer callback_data)
{
	NautilusDirectory *directory;
	
	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (directory->details->metafile_write_state->handle == handle);
	
	if (result != GNOME_VFS_OK) {
		metafile_write_failed (directory);
		return;
	}

	gnome_vfs_async_write (directory->details->metafile_write_state->handle,
			       directory->details->metafile_write_state->buffer,
			       directory->details->metafile_write_state->size,
			       metafile_write_callback,
			       directory);
}

void
nautilus_metafile_write_start (NautilusDirectory *directory)
{
	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	directory->details->metafile_write_state->write_again = FALSE;

	/* Open the file. */
	gnome_vfs_async_create_uri
		(&directory->details->metafile_write_state->handle,
		 directory->details->metafile_write_state->use_public_metafile
		 ? directory->details->public_metafile_vfs_uri
		 : directory->details->private_metafile_vfs_uri,
		 GNOME_VFS_OPEN_WRITE, FALSE, METAFILE_PERMISSIONS,
		 metafile_write_create_callback, directory);
}

static void
metafile_write (NautilusDirectory *directory)
{
	int xml_doc_size;
	
	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	nautilus_directory_ref (directory);

	/* If we are already writing, then just remember to do it again. */
	if (directory->details->metafile_write_state != NULL) {
		nautilus_directory_unref (directory);
		directory->details->metafile_write_state->write_again = TRUE;
		return;
	}

	/* Don't write anything if there's nothing to write.
	 * At some point, we might want to change this to actually delete
	 * the metafile in this case.
	 */
	if (directory->details->metafile == NULL) {
		nautilus_directory_unref (directory);
		return;
	}

	/* Create the write state. */
	directory->details->metafile_write_state = g_new0 (MetafileWriteState, 1);
	directory->details->metafile_write_state->use_public_metafile
		= can_use_public_metafile (directory);
	xmlDocDumpMemory (directory->details->metafile,
			  &directory->details->metafile_write_state->buffer,
			  &xml_doc_size);
	directory->details->metafile_write_state->size = xml_doc_size;
	nautilus_metafile_write_start (directory);
}

static gboolean
metafile_write_idle_callback (gpointer callback_data)
{
	NautilusDirectory *directory;

	directory = NAUTILUS_DIRECTORY (callback_data);

	directory->details->write_metafile_idle_id = 0;
	metafile_write (directory);

	nautilus_directory_unref (directory);

	return FALSE;
}

void
nautilus_directory_request_write_metafile (NautilusDirectory *directory)
{
	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	if (!allow_metafile (directory)) {
		return;
	}

	/* Set up an idle task that will write the metafile. */
	if (directory->details->write_metafile_idle_id == 0) {
		nautilus_directory_ref (directory);
		directory->details->write_metafile_idle_id =
			gtk_idle_add (metafile_write_idle_callback,
				      directory);
	}
}

static int
monitor_key_compare (gconstpointer a,
		     gconstpointer data)
{
	const Monitor *monitor;
	const Monitor *compare_monitor;

	monitor = a;
	compare_monitor = data;
	
	if (monitor->client < compare_monitor->client) {
		return -1;
	}
	if (monitor->client > compare_monitor->client) {
		return +1;
	}

	if (monitor->file < compare_monitor->file) {
		return -1;
	}
	if (monitor->file > compare_monitor->file) {
		return +1;
	}
	
	return 0;
}

static GList *
find_monitor (NautilusDirectory *directory,
	      NautilusFile *file,
	      gconstpointer client)
{
	GList *result;
	Monitor *monitor;

	monitor = g_new0 (Monitor, 1);
	monitor->client = client;
	monitor->file = file;

	result = g_list_find_custom (directory->details->monitor_list,
				     monitor,
				     monitor_key_compare);

	g_free (monitor);
	
	return result;
}

static void
remove_monitor_link (NautilusDirectory *directory,
		     GList *link)
{
	if (link != NULL) {
		directory->details->monitor_list =
			g_list_remove_link (directory->details->monitor_list, link);
		g_free (link->data);
		g_list_free_1 (link);
	}
}

static void
remove_monitor (NautilusDirectory *directory,
		NautilusFile *file,
		gconstpointer client)
{
	remove_monitor_link (directory, find_monitor (directory, file, client));
}

void
nautilus_directory_set_up_request (Request *request,
				   GList *file_attributes)
{
	memset (request, 0, sizeof (*request));

	request->directory_count = g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_DIRECTORY_ITEM_COUNT,
		 nautilus_strcmp_compare_func) != NULL;
	request->deep_count = g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_DEEP_COUNTS,
		 nautilus_strcmp_compare_func) != NULL;
	request->mime_list = g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_DIRECTORY_ITEM_MIME_TYPES,
		 nautilus_strcmp_compare_func) != NULL;
	request->file_info = g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_MIME_TYPE,
		 nautilus_strcmp_compare_func) != NULL;
	request->file_info |= g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_IS_DIRECTORY,
		 nautilus_strcmp_compare_func) != NULL;
	request->file_info |= g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_CAPABILITIES,
		 nautilus_strcmp_compare_func) != NULL;
	request->file_info |= g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_FILE_TYPE,
		 nautilus_strcmp_compare_func) != NULL;
	
	if (g_list_find_custom (file_attributes,
				NAUTILUS_FILE_ATTRIBUTE_TOP_LEFT_TEXT,
				nautilus_strcmp_compare_func) != NULL) {
		request->top_left_text = TRUE;
		request->file_info = TRUE;
	}
	
	if (g_list_find_custom (file_attributes,
				NAUTILUS_FILE_ATTRIBUTE_ACTIVATION_URI,
				nautilus_strcmp_compare_func) != NULL) {
		request->file_info = TRUE;
		request->activation_uri = TRUE;
	}
	
	request->metafile |= g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_METADATA,
		 nautilus_strcmp_compare_func) != NULL;

	/* FIXME bugzilla.eazel.com 2435:
	 * Some file attributes are really pieces of metadata.
	 * This is a confusing/broken design, since other metadata
	 * pieces are handled separately from file attributes. There
	 * are many ways this could be changed, ranging from making
	 * all metadata pieces have corresponding file attributes, to
	 * making a single file attribute that means "get all metadata",
	 * to making metadata keys be acceptable as file attributes
	 * directly (would need some funky char trick to prevent
	 * namespace collisions).
	 */
	request->metafile |= g_list_find_custom
		(file_attributes,
		 NAUTILUS_FILE_ATTRIBUTE_CUSTOM_ICON,
		 nautilus_strcmp_compare_func) != NULL;
}

static gboolean
is_tentative (gpointer data, gpointer callback_data)
{
	NautilusFile *file;

	g_assert (callback_data == NULL);

	file = NAUTILUS_FILE (data);
	return file->details->info == NULL;
}

static GList *
get_non_tentative_file_list (NautilusDirectory *directory)
{
	GList *tentative_files, *non_tentative_files;

	tentative_files = nautilus_g_list_partition
		(g_list_copy (directory->details->file_list),
		 is_tentative, NULL, &non_tentative_files);
	g_list_free (tentative_files);

	nautilus_file_list_ref (non_tentative_files);
	return non_tentative_files;
}

void
nautilus_directory_monitor_add_internal (NautilusDirectory *directory,
					 NautilusFile *file,
					 gconstpointer client,
					 gboolean monitor_hidden_files,
					 gboolean monitor_backup_files,
					 GList *file_attributes)
{
	Monitor *monitor;
	GList *file_list;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	/* Replace any current monitor for this client/file pair. */
	remove_monitor (directory, file, client);

	/* Add the new monitor. */
	monitor = g_new (Monitor, 1);
	monitor->file = file;
	monitor->monitor_hidden_files = monitor_hidden_files;
	monitor->monitor_backup_files = monitor_backup_files;
	monitor->client = client;
	nautilus_directory_set_up_request (&monitor->request, file_attributes);

	monitor->request.file_list = file == NULL;
	directory->details->monitor_list =
		g_list_prepend (directory->details->monitor_list, monitor);

	/* Re-send the "files_added" signal for this set of files.
	 * Old monitorers already know about them, but it's harmless
	 * to hear about the same files again.
	 */
	if (file == NULL) {
		file_list = get_non_tentative_file_list (directory);
		if (file_list != NULL) {
			nautilus_directory_emit_files_added
				(directory, file_list);
			nautilus_file_list_free (file_list);
		}
	}

	/* Kick off I/O. */
	nautilus_directory_async_state_changed (directory);
}

static void
set_file_unconfirmed (NautilusFile *file, gboolean unconfirmed)
{
	NautilusDirectory *directory;

	g_assert (NAUTILUS_IS_FILE (file));
	g_assert (unconfirmed == FALSE || unconfirmed == TRUE);

	if (file->details->unconfirmed == unconfirmed) {
		return;
	}
	file->details->unconfirmed = unconfirmed;

	directory = file->details->directory;
	if (unconfirmed) {
		directory->details->confirmed_file_count--;
	} else {
		directory->details->confirmed_file_count++;
	}
}

static gboolean show_hidden_files = TRUE;
static gboolean show_backup_files = TRUE;

static void
show_hidden_files_changed_callback (gpointer callback_data)
{
	show_hidden_files = nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES);
}

static void
show_backup_files_changed_callback (gpointer callback_data)
{
	show_backup_files = nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES);
}

static GnomeVFSDirectoryFilterOptions
get_filter_options_for_directory_count (void)
{
	static gboolean show_hidden_files_changed_callback_installed = FALSE;
	static gboolean show_backup_files_changed_callback_installed = FALSE;
	GnomeVFSDirectoryFilterOptions filter_options;
	
	filter_options = GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR
		| GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR;

	/* Add the callback once for the life of our process */
	if (show_hidden_files_changed_callback_installed == FALSE) {
		nautilus_preferences_add_callback_while_process_is_running (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
									    show_hidden_files_changed_callback,
									    NULL);
		show_hidden_files_changed_callback_installed = TRUE;
		
		/* Peek for the first time */
		show_hidden_files_changed_callback (NULL);
	}

	/* Add the callback once for the life of our process */
	if (show_backup_files_changed_callback_installed == FALSE) {
		nautilus_preferences_add_callback_while_process_is_running (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
									    show_backup_files_changed_callback,
									    NULL);
		show_backup_files_changed_callback_installed = TRUE;
		
		/* Peek for the first time */
		show_backup_files_changed_callback (NULL);
	}
	
	if (!show_hidden_files) {
		filter_options |= GNOME_VFS_DIRECTORY_FILTER_NODOTFILES;
	}
	if (!show_backup_files) {
		filter_options |= GNOME_VFS_DIRECTORY_FILTER_NOBACKUPFILES;
	}

	return filter_options;
}

static void
load_directory_done (NautilusDirectory *directory)
{
	NautilusFile *file;

	if (directory->details->load_mime_list_hash != NULL) {
		istr_set_destroy (directory->details->load_mime_list_hash);
		directory->details->load_mime_list_hash = NULL;
	}

	file = directory->details->load_directory_file;
	if (file != NULL) {
		directory->details->load_directory_file = NULL;

		file->details->loading_directory = FALSE;
		if (file->details->directory != directory) {
			nautilus_directory_async_state_changed (file->details->directory);
		}
		
		nautilus_file_unref (file);
	}

	gnome_vfs_directory_filter_destroy (directory->details->load_file_count_filter);
	directory->details->load_file_count_filter = NULL;
	
	nautilus_directory_async_state_changed (directory);
}

static gboolean
dequeue_pending_idle_callback (gpointer callback_data)
{
	NautilusDirectory *directory;
	GList *pending_file_info;
	GList *node, *next;
	NautilusFile *file;
	GList *changed_files, *added_files;
	GnomeVFSFileInfo *file_info;

	directory = NAUTILUS_DIRECTORY (callback_data);

	directory->details->dequeue_pending_idle_id = 0;

	/* Handle the files in the order we saw them. */
	pending_file_info = g_list_reverse (directory->details->pending_file_info);
	directory->details->pending_file_info = NULL;

	/* If we are no longer monitoring, then throw away these. */
	if (!nautilus_directory_is_file_list_monitored (directory)) {
		gnome_vfs_file_info_list_free (pending_file_info);
		load_directory_done (directory);
		return FALSE;
	}

	added_files = NULL;
	changed_files = NULL;

	/* Build a list of NautilusFile objects. */
	for (node = pending_file_info; node != NULL; node = node->next) {
		file_info = node->data;

		/* Update the file count. */
		/* FIXME bugzilla.eazel.com 5063: This could count a file twice if we get it
		 * from both load_directory and from
		 * new_files_callback. Not too hard to fix by moving
		 * this into the actual callback instead of waiting
		 * for the idle function.
		 */
		if (gnome_vfs_directory_filter_apply (directory->details->load_file_count_filter,
						      file_info)) {
			directory->details->load_file_count += 1;
		}

		/* Add the MIME type to the set. */
		if (directory->details->load_mime_list_hash != NULL) {
			istr_set_insert (directory->details->load_mime_list_hash,
					 file_info->mime_type);
		}
		
		/* check if the file already exists */
		file = nautilus_directory_find_file_by_name (directory, file_info->name);
		if (file != NULL) {
			/* file already exists, check if it changed */
			set_file_unconfirmed (file, FALSE);
			if (nautilus_file_update_info (file, file_info)) {
				/* File changed, notify about the change. */
				nautilus_file_ref (file);
				changed_files = g_list_prepend (changed_files, file);
			}
			nautilus_file_ref (file);			
		} else {
			/* new file, create a nautilus file object and add it to the list */
			file = nautilus_file_new_from_info (directory, file_info);
			nautilus_directory_add_file (directory, file);			
		}
		added_files = g_list_prepend (added_files, file);
	}
	gnome_vfs_file_info_list_free (pending_file_info);

	/* If we are done loading, then we assume that any unconfirmed
         * files are gone.
	 */
	if (directory->details->directory_loaded) {
		for (node = directory->details->file_list;
		     node != NULL; node = next) {
			file = NAUTILUS_FILE (node->data);
			next = node->next;

			if (file->details->unconfirmed) {
				nautilus_file_ref (file);
				changed_files = g_list_prepend (changed_files, file);

				file->details->is_gone = TRUE;
				nautilus_directory_remove_file (directory, file);
			}
		}
	}

	/* Send the changed and added signals. */
	nautilus_directory_emit_change_signals_deep (directory, changed_files);
	nautilus_file_list_free (changed_files);
	nautilus_directory_emit_files_added (directory, added_files);
	nautilus_file_list_free (added_files);

	if (directory->details->directory_loaded
	    && !directory->details->directory_loaded_sent_notification) {
		/* Send the done_loading signal. */
		nautilus_directory_emit_done_loading (directory);

		file = directory->details->load_directory_file;

		if (file != NULL) {
			file->details->directory_count_is_up_to_date = TRUE;
			file->details->got_directory_count = TRUE;
			file->details->directory_count = directory->details->load_file_count;

			file->details->got_mime_list = TRUE;
			file->details->mime_list_is_up_to_date = TRUE;
			file->details->mime_list = istr_set_get_as_list
				(directory->details->load_mime_list_hash);

			nautilus_file_changed (file);
		}
		
		load_directory_done (directory);

		directory->details->directory_loaded_sent_notification = TRUE;
	}

	/* Get the state machine running again. */
	nautilus_directory_async_state_changed (directory);
	return FALSE;
}

void
nautilus_directory_schedule_dequeue_pending (NautilusDirectory *directory)
{
	if (directory->details->dequeue_pending_idle_id == 0) {
		directory->details->dequeue_pending_idle_id
			= gtk_idle_add (dequeue_pending_idle_callback, directory);
	}
}

static void
directory_load_one (NautilusDirectory *directory,
		    GnomeVFSFileInfo *info)
{
	if (info == NULL) {
		return;
	}

	/* Arrange for the "loading" part of the work. */
	gnome_vfs_file_info_ref (info);
	directory->details->pending_file_info
		= g_list_prepend (directory->details->pending_file_info, info);
	nautilus_directory_schedule_dequeue_pending (directory);
}

static void
file_list_cancel (NautilusDirectory *directory)
{
	if (directory->details->directory_load_in_progress != NULL) {
		gnome_vfs_async_cancel (directory->details->directory_load_in_progress);
		directory->details->directory_load_in_progress = NULL;
		async_job_end (directory, "file list");
	}
}

static void
directory_load_done (NautilusDirectory *directory,
		     GnomeVFSResult result)
{
	GList *node;

	file_list_cancel (directory);
	directory->details->directory_loaded = TRUE;
	directory->details->directory_loaded_sent_notification = FALSE;

	/* Note that GNOME_VFS_OK can make it this far when the file-list
	 * length limit has been reached. In that case, don't treat it as
	 * an error.
	 */
	if (result != GNOME_VFS_ERROR_EOF && result != GNOME_VFS_OK) {
		/* The load did not complete successfully. This means
		 * we don't know the status of the files in this directory.
		 * We clear the unconfirmed bit on each file here so that
		 * they won't be marked "gone" later -- we don't know enough
		 * about them to know whether they are really gone.
		 */
		for (node = directory->details->file_list;
		     node != NULL; node = node->next) {
			set_file_unconfirmed (NAUTILUS_FILE (node->data), FALSE);
		}

		nautilus_directory_emit_load_error (directory,
						    result);
	}

	/* Call the idle function right away. */
	if (directory->details->dequeue_pending_idle_id != 0) {
		gtk_idle_remove (directory->details->dequeue_pending_idle_id);
	}
	dequeue_pending_idle_callback (directory);
}

static GnomeVFSDirectoryListPosition
directory_list_get_next_position (GnomeVFSDirectoryList *list,
				  GnomeVFSDirectoryListPosition position)
{
	if (position != GNOME_VFS_DIRECTORY_LIST_POSITION_NONE) {
		return gnome_vfs_directory_list_position_next (position);
	}
	if (list == NULL) {
		return GNOME_VFS_DIRECTORY_LIST_POSITION_NONE;
	}
	return gnome_vfs_directory_list_get_first_position (list);
}

static void
directory_load_callback (GnomeVFSAsyncHandle *handle,
			 GnomeVFSResult result,
			 GnomeVFSDirectoryList *list,
			 guint entries_read,
			 gpointer callback_data)
{
	NautilusDirectory *directory;
	GnomeVFSDirectoryListPosition last_handled, p;

	directory = NAUTILUS_DIRECTORY (callback_data);

	g_assert (directory->details->directory_load_in_progress != NULL);
	g_assert (directory->details->directory_load_in_progress == handle);

	/* Move items from the list onto our pending queue.
	 * We can't do this in the most straightforward way, becuse the position
	 * for a gnome_vfs_directory_list does not have a way of representing one
	 * past the end. So we must keep a position to the last item we handled
	 * rather than keeping a position past the last item we handled.
	 */
	last_handled = directory->details->directory_load_list_last_handled;
        p = last_handled;
	while ((p = directory_list_get_next_position (list, p))
	       != GNOME_VFS_DIRECTORY_LIST_POSITION_NONE) {
		directory_load_one
			(directory, gnome_vfs_directory_list_get (list, p));
		last_handled = p;
	}
	directory->details->directory_load_list_last_handled = last_handled;

	if (nautilus_directory_file_list_length_reached (directory) ||
	    result != GNOME_VFS_OK) {
		directory_load_done (directory, 
				     result);
	}
}

void
nautilus_directory_monitor_remove_internal (NautilusDirectory *directory,
					    NautilusFile *file,
					    gconstpointer client)
{
	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (file == NULL || NAUTILUS_IS_FILE (file));
	g_assert (client != NULL);

	remove_monitor (directory, file, client);

	nautilus_directory_async_state_changed (directory);
}

static int
ready_callback_key_compare (gconstpointer a, gconstpointer b)
{
	const ReadyCallback *callback_a, *callback_b;

	callback_a = a;
	callback_b = b;
	if (callback_a->file < callback_b->file) {
		return -1;
	}
	if (callback_a->file > callback_b->file) {
		return 1;
	}
	if (callback_a->file == NULL) {
		if (callback_a->callback.directory < callback_b->callback.directory) {
			return -1;
		}
		if (callback_a->callback.directory > callback_b->callback.directory) {
			return 1;
		}
	} else {
		if (callback_a->callback.file < callback_b->callback.file) {
			return -1;
		}
		if (callback_a->callback.file > callback_b->callback.file) {
			return 1;
		}
	}
	if (callback_a->callback_data < callback_b->callback_data) {
		return -1;
	}
	if (callback_a->callback_data > callback_b->callback_data) {
		return 1;
	}
	return 0;
}

static void
ready_callback_call (NautilusDirectory *directory,
		     const ReadyCallback *callback)
{
	GList *file_list;

	/* Call the callback. */
	if (callback->file != NULL) {
		(* callback->callback.file) (callback->file,
					     callback->callback_data);
	} else {
		if (directory == NULL || !callback->request.file_list) {
			file_list = NULL;
		} else {
			file_list = get_non_tentative_file_list (directory);
		}

		/* Pass back the file list if the user was waiting for it. */
		(* callback->callback.directory) (directory,
						  file_list,
						  callback->callback_data);

		nautilus_file_list_free (file_list);
	}
}

void
nautilus_directory_call_when_ready_internal (NautilusDirectory *directory,
					     NautilusFile *file,
					     GList *file_attributes,
					     NautilusDirectoryCallback directory_callback,
					     NautilusFileCallback file_callback,
					     gpointer callback_data)
{
	ReadyCallback callback;

	g_assert (directory == NULL || NAUTILUS_IS_DIRECTORY (directory));
	g_assert (file == NULL || NAUTILUS_IS_FILE (file));
	g_assert (file != NULL || directory_callback != NULL);
	g_assert (file == NULL || file_callback != NULL);
	
	/* Construct a callback object. */
	callback.file = file;
	if (file == NULL) {
		callback.callback.directory = directory_callback;
	} else {
		callback.callback.file = file_callback;
	}
	callback.callback_data = callback_data;
	nautilus_directory_set_up_request (&callback.request, file_attributes);
	callback.request.file_list = file == NULL && file_attributes != NULL;
	
	/* Handle the NULL case. */
	if (directory == NULL) {
		ready_callback_call (NULL, &callback);
		return;
	}

	/* Check if the callback is already there. */
	if (g_list_find_custom (directory->details->call_when_ready_list,
				&callback,
				ready_callback_key_compare) != NULL) {
		g_warning ("tried to add a new callback while an old one was pending");
		return;
	}

	/* Add the new callback to the list. */
	directory->details->call_when_ready_list = g_list_prepend
		(directory->details->call_when_ready_list,
		 g_memdup (&callback, sizeof (callback)));

	nautilus_directory_async_state_changed (directory);
}

gboolean      
nautilus_directory_check_if_ready_internal (NautilusDirectory *directory,
					    NautilusFile *file,
					    GList *file_attributes)
{
	Request request;

	g_assert (NAUTILUS_IS_DIRECTORY (directory));

	nautilus_directory_set_up_request (&request, file_attributes);
	return request_is_satisfied (directory, file, &request);
}					    

static void
remove_callback_link_keep_data (NautilusDirectory *directory,
				GList *link)
{
	directory->details->call_when_ready_list = g_list_remove_link
		(directory->details->call_when_ready_list, link);
	g_list_free_1 (link);
}

static void
remove_callback_link (NautilusDirectory *directory,
		      GList *link)
{
	g_free (link->data);
	remove_callback_link_keep_data (directory, link);
}

void
nautilus_directory_cancel_callback_internal (NautilusDirectory *directory,
					     NautilusFile *file,
					     NautilusDirectoryCallback directory_callback,
					     NautilusFileCallback file_callback,
					     gpointer callback_data)
{
	ReadyCallback callback;
	GList *node;

	if (directory == NULL) {
		return;
	}

	g_assert (NAUTILUS_IS_DIRECTORY (directory));
	g_assert (file == NULL || NAUTILUS_IS_FILE (file));
	g_assert (file != NULL || directory_callback != NULL);
	g_assert (file == NULL || file_callback != NULL);

	/* Construct a callback object. */
	callback.file = file;
	if (file == NULL) {
		callback.callback.directory = directory_callback;
	} else {
		callback.callback.file = file_callback;
	}
	callback.callback_data = callback_data;

	/* Remove queued callback from the list. */
	node = g_list_find_custom (directory->details->call_when_ready_list,
				&callback,
				ready_callback_key_compare);
	if (node != NULL) {
		remove_callback_link (directory, node);
		nautilus_directory_async_state_changed (directory);
	}
}

static void
directory_count_callback (GnomeVFSAsyncHandle *handle,
			  GnomeVFSResult result,
			  GnomeVFSDirectoryList *list,
			  guint entries_read,
			  gpointer callback_data)
{
	NautilusDirectory *directory;
	NautilusFile *count_file;

	directory = NAUTILUS_DIRECTORY (callback_data);

	g_assert (directory->details->count_in_progress == handle);
	count_file = directory->details->count_file;
	g_assert (NAUTILUS_IS_FILE (count_file));

	if (result == GNOME_VFS_OK) {
		return;
	}

	count_file->details->directory_count_is_up_to_date = TRUE;

	/* Record either a failure or success. */
	if (result != GNOME_VFS_ERROR_EOF) {
		count_file->details->directory_count_failed = TRUE;
	} else {
		count_file->details->got_directory_count = TRUE;
		count_file->details->directory_count = entries_read;
	}
	directory->details->count_file = NULL;
	directory->details->count_in_progress = NULL;

	/* Send file-changed even if count failed, so interested parties can
	 * distinguish between unknowable and not-yet-known cases.
	 */
	nautilus_file_changed (count_file);

	/* Start up the next one. */
	async_job_end (directory, "directory count");
	nautilus_directory_async_state_changed (directory);
}

static void
new_files_callback (GnomeVFSAsyncHandle *handle,
		    GList *results,
		    gpointer callback_data)
{
	GList **handles, *node;
	NautilusDirectory *directory;
	GnomeVFSGetFileInfoResult *result;

	directory = NAUTILUS_DIRECTORY (callback_data);
	handles = &directory->details->get_file_infos_in_progress;
	g_assert (handle == NULL || g_list_find (*handles, handle) != NULL);
	
	/* Note that this call is done. */
	*handles = g_list_remove (*handles, handle);

	/* Queue up the new files. */
	for (node = results; node != NULL; node = node->next) {
		result = node->data;

		if (result->result == GNOME_VFS_OK) {
			directory_load_one (directory, result->file_info);
		}
	}
}

void
nautilus_directory_get_info_for_new_files (NautilusDirectory *directory,
					   GList *vfs_uri_list)
{
	GnomeVFSAsyncHandle *handle;
	gnome_vfs_async_get_file_info
		(&handle,
		 vfs_uri_list,
		 (GNOME_VFS_FILE_INFO_GET_MIME_TYPE
		  | GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
		 new_files_callback,
		 directory);

	directory->details->get_file_infos_in_progress
		= g_list_prepend (directory->details->get_file_infos_in_progress,
				  handle);
}

void
nautilus_async_destroying_file (NautilusFile *file)
{
	NautilusDirectory *directory;
	gboolean changed;
	GList *node, *next;
	ReadyCallback *callback;
	Monitor *monitor;

	directory = file->details->directory;
	changed = FALSE;

	/* Check for callbacks. */
	for (node = directory->details->call_when_ready_list; node != NULL; node = next) {
		next = node->next;
		callback = node->data;

		if (callback->file == file) {
			/* Client should have cancelled callback. */
			g_warning ("destroyed file has call_when_ready pending");
			remove_callback_link (directory, node);
			changed = TRUE;
		}
	}

	/* Check for monitors. */
	for (node = directory->details->monitor_list; node != NULL; node = next) {
		next = node->next;
		monitor = node->data;

		if (monitor->file == file) {
			/* Client should have removed monitor earlier. */
			g_warning ("destroyed file still being monitored");
			remove_monitor_link (directory, node);
			changed = TRUE;
		}
	}

	/* Check if it's a file that's currently being worked on.
	 * If so, make that NULL so it gets canceled right away.
	 */
	if (directory->details->count_file == file) {
		directory->details->count_file = NULL;
		changed = TRUE;
	}
	if (directory->details->deep_count_file == file) {
		directory->details->deep_count_file = NULL;
		changed = TRUE;
	}
	if (directory->details->mime_list_file == file) {
		directory->details->mime_list_file = NULL;
		changed = TRUE;
	}
	if (directory->details->get_info_file == file) {
		directory->details->get_info_file = NULL;
		changed = TRUE;
	}
	if (directory->details->top_left_read_state != NULL
	    && directory->details->top_left_read_state->file == file) {
		directory->details->top_left_read_state->file = NULL;
		changed = TRUE;
	}
	if (directory->details->activation_uri_read_state != NULL
	    && directory->details->activation_uri_read_state->file == file) {
		directory->details->activation_uri_read_state->file = NULL;
		changed = TRUE;
	}

	/* Let the directory take care of the rest. */
	if (changed) {
		nautilus_directory_async_state_changed (directory);
	}
}

static gboolean
lacks_directory_count (NautilusFile *file)
{
	return nautilus_file_is_directory (file)
		&& !file->details->directory_count_is_up_to_date;
}

static gboolean
should_get_directory_count (NautilusFile *file)
{
	return lacks_directory_count (file)
		&& !file->details->loading_directory;
}

static gboolean
wants_directory_count (const Request *request)
{
	return request->directory_count;
}

static gboolean
lacks_top_left (NautilusFile *file)
{
	return nautilus_file_should_get_top_left_text (file)
		&& nautilus_file_contains_text (file)
		&& !file->details->top_left_text_is_up_to_date;
}

static gboolean
wants_top_left (const Request *request)
{
	return request->top_left_text;
}

static gboolean
lacks_info (NautilusFile *file)
{
	return !file->details->file_info_is_up_to_date
		&& !file->details->is_gone;
}

static gboolean
wants_info (const Request *request)
{
	return request->file_info;
}

static gboolean
lacks_deep_count (NautilusFile *file)
{
	return nautilus_file_is_directory (file)
		&& file->details->deep_counts_status != NAUTILUS_REQUEST_DONE;
}

static gboolean
wants_deep_count (const Request *request)
{
	return request->deep_count;
}

static gboolean
lacks_mime_list (NautilusFile *file)
{
	return nautilus_file_is_directory (file)
		&& !file->details->mime_list_is_up_to_date;
}

static gboolean
should_get_mime_list (NautilusFile *file)
{
	return lacks_mime_list (file)
		&& !file->details->loading_directory;
}

static gboolean
wants_mime_list (const Request *request)
{
	return request->mime_list;
}

static gboolean
lacks_activation_uri (NautilusFile *file)
{
	return file->details->info != NULL
		&& !file->details->activation_uri_is_up_to_date;
}

static gboolean
wants_activation_uri (const Request *request)
{
	return request->activation_uri;
}


static gboolean
has_problem (NautilusDirectory *directory, NautilusFile *file, FileCheck problem)
{
	GList *node;

	if (file != NULL) {
		return (* problem) (file);
	}

	for (node = directory->details->file_list; node != NULL; node = node->next) {
		if ((* problem) (node->data)) {
			return TRUE;
		}
	}

	return FALSE;
	
}

static gboolean
request_is_satisfied (NautilusDirectory *directory,
		      NautilusFile *file,
		      Request *request)
{
	if (request->metafile && !directory->details->metafile_read) {
		return FALSE;
	}

	if (request->file_list && !(directory->details->directory_loaded &&
				    directory->details->directory_loaded_sent_notification)) {
		return FALSE;
	}

	if (request->directory_count) {
		if (has_problem (directory, file, lacks_directory_count)) {
			return FALSE;
		}
	}

	if (request->file_info) {
		if (has_problem (directory, file, lacks_info)) {
			return FALSE;
		}
	}

	if (request->top_left_text) {
		if (has_problem (directory, file, lacks_top_left)) {
			return FALSE;
		}
	}

	if (request->deep_count) {
		if (has_problem (directory, file, lacks_deep_count)) {
			return FALSE;
		}
	}

	if (request->mime_list) {
		if (has_problem (directory, file, lacks_mime_list)) {
			return FALSE;
		}
	}

	if (request->activation_uri) {
		if (has_problem (directory, file, lacks_activation_uri)) {
			return FALSE;
		}
	}

	return TRUE;
}		      

static gboolean
call_ready_callbacks (NautilusDirectory *directory)
{
	gboolean called_any;
	GList *node, *next;
	ReadyCallback *callback;

	callback = NULL;
	called_any = FALSE;
	while (1) {
		/* Check if any callbacks are satisifed and call them if they are. */
		for (node = directory->details->call_when_ready_list;
		     node != NULL; node = next) {
			next = node->next;
			callback = node->data;

			if (request_is_satisfied (directory, callback->file, &callback->request)) {
				break;
			}
		}
		if (node == NULL) {
			return called_any;
		}
		
		/* Callbacks are one-shots, so remove it now. */
		remove_callback_link_keep_data (directory, node);
		
		/* Call the callback. */
		ready_callback_call (directory, callback);
		g_free (callback);
		called_any = TRUE;
	}
}

/* This checks if there's a request for monitoring the file list. */
gboolean
nautilus_directory_is_anyone_monitoring_file_list (NautilusDirectory *directory)
{
	GList *node;
	ReadyCallback *callback;
	Monitor *monitor;

	for (node = directory->details->call_when_ready_list;
	     node != NULL; node = node->next) {
		callback = node->data;
		if (callback->request.file_list) {
			return TRUE;
		}
	}

	for (node = directory->details->monitor_list;
	     node != NULL; node = node->next) {
		monitor = node->data;
		if (monitor->request.file_list) {
			return TRUE;
		}
	}

	return FALSE;
}

/* This checks if the file list being monitored. */
gboolean
nautilus_directory_is_file_list_monitored (NautilusDirectory *directory) 
{
	return directory->details->file_list_monitored;
}

static void
mark_all_files_unconfirmed (NautilusDirectory *directory)
{
	GList *node;
	NautilusFile *file;

	for (node = directory->details->file_list; node != NULL; node = node->next) {
		file = node->data;
		set_file_unconfirmed (file, TRUE);
	}
}

static gboolean
should_display_file_name (const char *name,
			  GnomeVFSDirectoryFilterOptions options)
{
	/* Note that the name is URI-encoded, but this should not
	 * affect the . or the ~.
	 */

	if ((options & GNOME_VFS_DIRECTORY_FILTER_NODOTFILES) != 0
	    && nautilus_file_name_matches_hidden_pattern (name)) {
		return FALSE;
	}

	if ((options & GNOME_VFS_DIRECTORY_FILTER_NOBACKUPFILES) != 0
	    && nautilus_file_name_matches_backup_pattern (name)) {
		return FALSE;
	}
	
	/* Note that we don't bother to check for "." or ".." here, because
	 * this function is used only for search results, which never include
	 * those special files. If we later use this function more generally,
	 * we might have to change this.
	 */
	return TRUE;
}

/* Filter search results based on user preferences. This must be done
 * differently than filtering other files because the search results
 * are encoded: the entire file path is encoded and stored as the file
 * name.
 */
static gboolean
filter_search_uri (const GnomeVFSFileInfo *info, gpointer data)
{
	GnomeVFSDirectoryFilterOptions options;
	char *real_file_uri;
	gboolean result;

	options = GPOINTER_TO_INT (data);
	
	real_file_uri = nautilus_get_target_uri_from_search_result_name (info->name);
	result = should_display_file_name (g_basename (real_file_uri), options);	
	g_free (real_file_uri);

	return result;
}

static GnomeVFSDirectoryFilter *
get_file_count_filter (NautilusDirectory *directory)
{
	if (nautilus_is_search_uri (directory->details->uri)) {
		return gnome_vfs_directory_filter_new_custom
			(filter_search_uri,
			 GNOME_VFS_DIRECTORY_FILTER_NEEDS_NAME,
			 GINT_TO_POINTER (get_filter_options_for_directory_count ()));
	}

	return gnome_vfs_directory_filter_new
		(GNOME_VFS_DIRECTORY_FILTER_NONE,
		 get_filter_options_for_directory_count (),
		 NULL);
}

/* Start monitoring the file list if it isn't already. */
static void
start_monitoring_file_list (NautilusDirectory *directory)
{
	if (!directory->details->file_list_monitored) {
		g_assert (directory->details->directory_load_in_progress == NULL);
		directory->details->file_list_monitored = TRUE;
		nautilus_file_list_ref (directory->details->file_list);
	}

	if (directory->details->directory_loaded
	    || directory->details->directory_load_in_progress != NULL) {
		return;
	}

	if (!async_job_start (directory, "file list")) {
		return;
	}

	mark_all_files_unconfirmed (directory);

	g_assert (directory->details->uri != NULL);
	directory->details->directory_load_list_last_handled
		= GNOME_VFS_DIRECTORY_LIST_POSITION_NONE;
        directory->details->load_directory_file =
		nautilus_directory_get_corresponding_file (directory);
	directory->details->load_directory_file->details->loading_directory = TRUE;
	directory->details->load_file_count = 0;
	directory->details->load_file_count_filter = get_file_count_filter (directory);
	directory->details->load_mime_list_hash = istr_set_new ();
#ifdef DEBUG_LOAD_DIRECTORY
	g_message ("load_directory called to monitor file list of %s", directory->details->uri);
#endif	
	gnome_vfs_async_load_directory
		(&directory->details->directory_load_in_progress, /* handle */
		 directory->details->uri,                         /* uri */
		 (GNOME_VFS_FILE_INFO_GET_MIME_TYPE	          /* options */
		  | GNOME_VFS_FILE_INFO_FOLLOW_LINKS),
		 NULL, 					          /* sort_rules */
		 FALSE, 				          /* reverse_order */
		 GNOME_VFS_DIRECTORY_FILTER_NONE,                 /* filter_type */
		 (GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR            /* filter_options */
		  | GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR),
		 NULL,                                            /* filter_pattern */
		 DIRECTORY_LOAD_ITEMS_PER_CALLBACK,               /* items_per_notification */
		 directory_load_callback,                         /* callback */
		 directory);
}

/* Stop monitoring the file list if it is being monitored. */
void
nautilus_directory_stop_monitoring_file_list (NautilusDirectory *directory)
{
	if (!directory->details->file_list_monitored) {
		g_assert (directory->details->directory_load_in_progress == NULL);
		return;
	}

	directory->details->file_list_monitored = FALSE;
	file_list_cancel (directory);
	nautilus_file_list_unref (directory->details->file_list);
	directory->details->directory_loaded = FALSE;
}

static void
file_list_start (NautilusDirectory *directory)
{
	if (nautilus_directory_is_anyone_monitoring_file_list (directory)) {
		start_monitoring_file_list (directory);
	} else {
		nautilus_directory_stop_monitoring_file_list (directory);
	}
}

void
nautilus_directory_invalidate_counts (NautilusDirectory *directory)
{
	NautilusFile *file;
	NautilusDirectory *parent_directory;

	file = nautilus_directory_get_existing_corresponding_file (directory);
	if (file != NULL) {
		parent_directory = file->details->directory;

		if (parent_directory->details->count_file == file) {
			directory_count_cancel (parent_directory);
		}
		if (parent_directory->details->deep_count_file == file) {
			deep_count_cancel (parent_directory);
		}
		if (parent_directory->details->mime_list_file == file) {
			mime_list_cancel (parent_directory);
		}

		file->details->directory_count_is_up_to_date = FALSE;
		file->details->deep_counts_status = NAUTILUS_REQUEST_NOT_STARTED;
		file->details->mime_list_is_up_to_date = FALSE;

		nautilus_file_unref (file);

		nautilus_directory_async_state_changed (parent_directory);
	}
}

static void
nautilus_directory_invalidate_file_attributes (NautilusDirectory *directory,
					       GList             *file_attributes)
{
	GList *node;

	cancel_loading_attributes (directory, file_attributes);

	for (node = directory->details->file_list; node != NULL; node = node->next) {
		nautilus_file_invalidate_attributes_internal (NAUTILUS_FILE (node->data),
							      file_attributes);
	}

	if (directory->details->as_file != NULL) {
		nautilus_file_invalidate_attributes_internal (directory->details->as_file,
							      file_attributes);
	}
}

void
nautilus_directory_force_reload (NautilusDirectory *directory,
				 GList             *file_attributes)
{
	/* invalidate attributes that are getting reloaded for all files */
	nautilus_directory_invalidate_file_attributes (directory, file_attributes);

	/* Start a new directory load. */
	file_list_cancel (directory);
	directory->details->directory_loaded = FALSE;

	/* Start a new directory count. */
	nautilus_directory_invalidate_counts (directory);

	nautilus_directory_async_state_changed (directory);
}

static gboolean
monitor_includes_file (const Monitor *monitor,
		       NautilusFile *file)
{
	if (monitor->file == file) {
		return TRUE;
	}
	if (monitor->file != NULL) {
		return FALSE;
	}
	if (file == file->details->directory->details->as_file) {
		return FALSE;
	}
	return nautilus_file_should_show (file,
					  monitor->monitor_hidden_files,
					  monitor->monitor_backup_files);
}

static gboolean
is_needy (NautilusFile *file,
	  FileCheck check_missing,
	  RequestCheck check_wanted)
{
	NautilusDirectory *directory;
	GList *node;
	ReadyCallback *callback;
	Monitor *monitor;

	g_assert (NAUTILUS_IS_FILE (file));

	if (!(* check_missing) (file)) {
		return FALSE;
	}

	directory = file->details->directory;
	for (node = directory->details->call_when_ready_list;
	     node != NULL; node = node->next) {
		callback = node->data;
		if ((* check_wanted) (&callback->request)) {
			if (callback->file == file) {
				return TRUE;
			}
			if (callback->file == NULL
			    && file != directory->details->as_file) {
				return TRUE;
			}
		}
	}
	for (node = directory->details->monitor_list;
	     node != NULL; node = node->next) {
		monitor = node->data;
		if ((* check_wanted) (&monitor->request)) {
			if (monitor_includes_file (monitor, file)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static NautilusFile *
select_needy_file (NautilusDirectory *directory,
		   FileCheck check_missing,
		   RequestCheck check_wanted)
{
	GList *node, *node_2;
	ReadyCallback *callback;
	Monitor *monitor;
	NautilusFile *file;

	/* Quick out if no one is interested. */
	for (node = directory->details->call_when_ready_list;
	     node != NULL; node = node->next) {
		callback = node->data;
		if ((* check_wanted) (&callback->request)) {
			break;
		}
	}
	if (node == NULL) {
		for (node = directory->details->monitor_list;
		     node != NULL; node = node->next) {
			monitor = node->data;
			if ((* check_wanted) (&monitor->request)) {
				break;
			}
		}
		if (node == NULL) {
			return NULL;
		}
	}

	/* Search for a file that has an unfulfilled request. */
	for (node = directory->details->file_list;
	     node != NULL; node = node->next) {
		file = node->data;
		if ((* check_missing) (file)) {
		    	for (node_2 = directory->details->call_when_ready_list;
			     node_2 != NULL; node_2 = node_2->next) {
				callback = node_2->data;
				if ((callback->file == NULL || callback->file == file)
				    && (* check_wanted) (&callback->request)) {
					break;
				}
		    	}
			if (node_2 != NULL) {
				return file;
			}
			for (node_2 = directory->details->monitor_list;
			     node_2 != NULL; node_2 = node_2->next) {
				monitor = node_2->data;
				if (monitor_includes_file (monitor, file)
				    && (* check_wanted) (&monitor->request)) {
					break;
				}
			}
			if (node_2 != NULL) {
				return file;
			}
		}
	}

	/* Finally, check the file for the directory itself. */
	file = directory->details->as_file;
	if (file != NULL) {
		if ((* check_missing) (file)) {
		    	for (node_2 = directory->details->call_when_ready_list;
			     node_2 != NULL; node_2 = node_2->next) {
				callback = node_2->data;
				if (callback->file == file
				    && (* check_wanted) (&callback->request)) {
					break;
				}
		    	}
			if (node_2 != NULL) {
				return file;
			}
			for (node_2 = directory->details->monitor_list;
			     node_2 != NULL; node_2 = node_2->next) {
				monitor = node_2->data;
				if (monitor->file == file
				    && (* check_wanted) (&monitor->request)) {
					break;
				}
			}
			if (node_2 != NULL) {
				return file;
			}
		}
	}

	return NULL;
}

static void
directory_count_start (NautilusDirectory *directory)
{
	NautilusFile *file;
	char *uri;

	/* If there's already a count in progress, check to be sure
	 * it's still wanted.
	 */
	if (directory->details->count_in_progress != NULL) {
		file = directory->details->count_file;
		if (file != NULL) {
			g_assert (NAUTILUS_IS_FILE (file));
			g_assert (file->details->directory == directory);
			if (is_needy (file,
				      should_get_directory_count,
				      wants_directory_count)) {
				return;
			}
		}

		/* The count is not wanted, so stop it. */
		directory_count_cancel (directory);
	}

	/* Figure out which file to get a count for. */
	file = select_needy_file (directory,
				  should_get_directory_count,
				  wants_directory_count);
	if (file == NULL) {
		return;
	}

	if (!async_job_start (directory, "directory count")) {
		return;
	}

	/* Start counting. */
	directory->details->count_file = file;
	uri = nautilus_file_get_uri (file);
#ifdef DEBUG_LOAD_DIRECTORY		
	g_message ("load_directory called to get shallow file count for %s", uri);
#endif	
	gnome_vfs_async_load_directory
		(&directory->details->count_in_progress,
		 uri,
		 GNOME_VFS_FILE_INFO_DEFAULT,
		 NULL,
		 FALSE,
		 GNOME_VFS_DIRECTORY_FILTER_NONE,
		 get_filter_options_for_directory_count (),
		 NULL,
		 G_MAXINT,
		 directory_count_callback,
		 directory);
	g_free (uri);
}

static void
deep_count_one (NautilusDirectory *directory,
		GnomeVFSFileInfo *info)
{
	NautilusFile *file;
	char *escaped_name, *uri;

	file = directory->details->deep_count_file;

	if ((info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) != 0
	    && info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		/* Count the directory. */
		file->details->deep_directory_count += 1;

		/* Record the fact that we have to descend into this directory. */
		escaped_name = gnome_vfs_escape_string (info->name);
		uri = nautilus_make_path (directory->details->deep_count_uri, escaped_name);
		g_free (escaped_name);
		directory->details->deep_count_subdirectories = g_list_prepend
			(directory->details->deep_count_subdirectories, uri);
	} else {
		/* Even non-regular files count as files. */
		file->details->deep_file_count += 1;
	}

	/* Count the size. */
	if ((info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE) != 0) {
		file->details->deep_size += info->size;
	}
}

static void
deep_count_callback (GnomeVFSAsyncHandle *handle,
		     GnomeVFSResult result,
		     GnomeVFSDirectoryList *list,
		     guint entries_read,
		     gpointer callback_data)
{
	NautilusDirectory *directory;
	NautilusFile *file;
	GnomeVFSDirectoryListPosition last_handled, p;
	char *uri;
	gboolean done;

	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (directory->details->deep_count_in_progress == handle);
	file = directory->details->deep_count_file;
	g_assert (NAUTILUS_IS_FILE (file));

	/* We can't do this in the most straightforward way, becuse the position
	 * for a gnome_vfs_directory_list does not have a way of representing one
	 * past the end. So we must keep a position to the last item we handled
	 * rather than keeping a position past the last item we handled.
	 */
	last_handled = directory->details->deep_count_last_handled;
        p = last_handled;
	while ((p = directory_list_get_next_position (list, p))
	       != GNOME_VFS_DIRECTORY_LIST_POSITION_NONE) {
		deep_count_one (directory, gnome_vfs_directory_list_get (list, p));
		last_handled = p;
	}
	directory->details->deep_count_last_handled = last_handled;

	done = FALSE;
	if (result != GNOME_VFS_OK) {
		if (result != GNOME_VFS_ERROR_EOF) {
			file->details->deep_unreadable_count += 1;
		}
		
		directory->details->deep_count_in_progress = NULL;
		g_free (directory->details->deep_count_uri);
		directory->details->deep_count_uri = NULL;

		if (directory->details->deep_count_subdirectories != NULL) {
			/* Work on a new directory. */
			uri = directory->details->deep_count_subdirectories->data;
			directory->details->deep_count_subdirectories = g_list_remove
				(directory->details->deep_count_subdirectories, uri);
			deep_count_load (directory, uri);
			g_free (uri);
		} else {
			file->details->deep_counts_status = NAUTILUS_REQUEST_DONE;
			directory->details->deep_count_file = NULL;
			done = TRUE;
		}
	}

	nautilus_file_changed (file);

	if (done) {
		async_job_end (directory, "deep count");
		nautilus_directory_async_state_changed (directory);
	}
}

static void
deep_count_load (NautilusDirectory *directory, const char *uri)
{
	g_assert (directory->details->deep_count_uri == NULL);
	directory->details->deep_count_uri = g_strdup (uri);
	directory->details->deep_count_last_handled
		= GNOME_VFS_DIRECTORY_LIST_POSITION_NONE;
#ifdef DEBUG_LOAD_DIRECTORY		
	g_message ("load_directory called to get deep file count for %s", uri);
#endif	
	gnome_vfs_async_load_directory
		(&directory->details->deep_count_in_progress,
		 uri,
		 GNOME_VFS_FILE_INFO_DEFAULT,
		 NULL,
		 FALSE,
		 GNOME_VFS_DIRECTORY_FILTER_NONE,
		 (GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR
		  | GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR),
		 NULL,
		 G_MAXINT,
		 deep_count_callback,
		 directory);
}

static void
deep_count_start (NautilusDirectory *directory)
{
	NautilusFile *file;
	char *uri;

	/* If there's already a count in progress, check to be sure
	 * it's still wanted.
	 */
	if (directory->details->deep_count_in_progress != NULL) {
		file = directory->details->deep_count_file;
		if (file != NULL) {
			g_assert (NAUTILUS_IS_FILE (file));
			g_assert (file->details->directory == directory);
			if (is_needy (file,
				      lacks_deep_count,
				      wants_deep_count)) {
				return;
			}
		}

		/* The count is not wanted, so stop it. */
		deep_count_cancel (directory);
	}

	/* Figure out which file to get a count for. */
	file = select_needy_file (directory,
				  lacks_deep_count,
				  wants_deep_count);
	if (file == NULL) {
		return;
	}

	if (!async_job_start (directory, "deep count")) {
		return;
	}

	/* Start counting. */
	file->details->deep_counts_status = NAUTILUS_REQUEST_IN_PROGRESS;
	file->details->deep_directory_count = 0;
	file->details->deep_file_count = 0;
	file->details->deep_unreadable_count = 0;
	file->details->deep_size = 0;
	directory->details->deep_count_file = file;
	uri = nautilus_file_get_uri (file);
	deep_count_load (directory, uri);
	g_free (uri);
}

static void
mime_list_one (NautilusDirectory *directory,
	       GnomeVFSFileInfo *info)
{
	istr_set_insert (directory->details->mime_list_hash, info->mime_type);
}

static void
mime_list_callback (GnomeVFSAsyncHandle *handle,
		    GnomeVFSResult result,
		    GnomeVFSDirectoryList *list,
		    guint entries_read,
		    gpointer callback_data)
{
	NautilusDirectory *directory;
	NautilusFile *file;
	GnomeVFSDirectoryListPosition last_handled, p;

	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (directory->details->mime_list_in_progress == handle);
	file = directory->details->mime_list_file;
	g_assert (NAUTILUS_IS_FILE (file));

	/* We can't do this in the most straightforward way, becuse the position
	 * for a gnome_vfs_directory_list does not have a way of representing one
	 * past the end. So we must keep a position to the last item we handled
	 * rather than keeping a position past the last item we handled.
	 */
	last_handled = directory->details->mime_list_last_handled;
        p = last_handled;
	while ((p = directory_list_get_next_position (list, p))
	       != GNOME_VFS_DIRECTORY_LIST_POSITION_NONE) {
		mime_list_one (directory, gnome_vfs_directory_list_get (list, p));
		last_handled = p;
	}
	directory->details->mime_list_last_handled = last_handled;

	if (result == GNOME_VFS_OK) {
		return;
	}

	file->details->mime_list_is_up_to_date = TRUE;

	/* Record either a failure or success. */
	nautilus_g_list_free_deep (file->details->mime_list);
	if (result != GNOME_VFS_ERROR_EOF) {
		file->details->mime_list_failed = TRUE;
		file->details->mime_list = NULL;
	} else {
		file->details->got_mime_list = TRUE;
		file->details->mime_list = istr_set_get_as_list
			(directory->details->mime_list_hash);
	}
	istr_set_destroy (directory->details->mime_list_hash);

	directory->details->mime_list_in_progress = NULL;
	directory->details->mime_list_file = NULL;
	directory->details->mime_list_hash = NULL;

	/* Send file-changed even if getting the item type list
	 * failed, so interested parties can distinguish between
	 * unknowable and not-yet-known cases.
	 */
	nautilus_file_changed (file);

	/* Start up the next one. */
	async_job_end (directory, "MIME list");
	nautilus_directory_async_state_changed (directory);
}

static void
mime_list_load (NautilusDirectory *directory, const char *uri)
{
	directory->details->mime_list_last_handled
		= GNOME_VFS_DIRECTORY_LIST_POSITION_NONE;
	directory->details->mime_list_hash = istr_set_new ();
#ifdef DEBUG_LOAD_DIRECTORY		
	g_message ("load_directory called to get MIME list of %s", uri);
#endif	
	gnome_vfs_async_load_directory
		(&directory->details->mime_list_in_progress,
		 uri,
		 GNOME_VFS_FILE_INFO_GET_MIME_TYPE,
		 NULL,
		 FALSE,
		 GNOME_VFS_DIRECTORY_FILTER_NONE,
		 (GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR
		  | GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR),
		 NULL,
		 DIRECTORY_LOAD_ITEMS_PER_CALLBACK,
		 mime_list_callback,
		 directory);
}

static void
mime_list_start (NautilusDirectory *directory)
{
	NautilusFile *file;
	char *uri;

	/* If there's already a count in progress, check to be sure
	 * it's still wanted.
	 */
	if (directory->details->mime_list_in_progress != NULL) {
		file = directory->details->mime_list_file;
		if (file != NULL) {
			g_assert (NAUTILUS_IS_FILE (file));
			g_assert (file->details->directory == directory);
			if (is_needy (file,
				      should_get_mime_list,
				      wants_mime_list)) {
				return;
			}
		}

		/* The count is not wanted, so stop it. */
		mime_list_cancel (directory);
	}

	/* Figure out which file to get a mime list for. */
	file = select_needy_file (directory,
				  should_get_mime_list,
				  wants_mime_list);
	if (file == NULL) {
		return;
	}

	if (!async_job_start (directory, "MIME list")) {
		return;
	}

	directory->details->mime_list_file = file;
	uri = nautilus_file_get_uri (file);
	mime_list_load (directory, uri);
	g_free (uri);
}

static int
count_lines (const char *text, int length)
{
	int count, i;

	count = 0;
	for (i = 0; i < length; i++) {
		count += *text++ == '\n';
	}
	return count;
}

static void
top_left_read_done (NautilusDirectory *directory)
{
	g_assert (directory->details->top_left_read_state->handle == NULL);
	g_assert (NAUTILUS_IS_FILE (directory->details->top_left_read_state->file));

	directory->details->top_left_read_state->file->details->got_top_left_text = TRUE;

	g_free (directory->details->top_left_read_state);
	directory->details->top_left_read_state = NULL;

	async_job_end (directory, "top left");
	nautilus_directory_async_state_changed (directory);
}

static void
top_left_read_callback (GnomeVFSResult result,
			GnomeVFSFileSize bytes_read,
			char *file_contents,
			gpointer callback_data)
{
	NautilusDirectory *directory;
	NautilusFile *changed_file;

	directory = NAUTILUS_DIRECTORY (callback_data);

	directory->details->top_left_read_state->handle = NULL;

	directory->details->top_left_read_state->file->details->top_left_text_is_up_to_date = TRUE;

	changed_file = NULL;
	if (result == GNOME_VFS_OK) {
		g_free (directory->details->top_left_read_state->file->details->top_left_text);
		directory->details->top_left_read_state->file->details->top_left_text =
			nautilus_extract_top_left_text (file_contents, bytes_read);
		
		directory->details->top_left_read_state->file->details->got_top_left_text = TRUE;

		changed_file = directory->details->top_left_read_state->file;
		nautilus_file_ref (changed_file);
		
		g_free (file_contents);
	}
	
	top_left_read_done (directory);

	if (changed_file != NULL) {
		nautilus_file_changed (changed_file);
		nautilus_file_unref (changed_file);
	}
}

static gboolean
top_left_read_more_callback (GnomeVFSFileSize bytes_read,
			     const char *file_contents,
			     gpointer callback_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (callback_data));

	/* Stop reading when we have enough. */
	return bytes_read < NAUTILUS_FILE_TOP_LEFT_TEXT_MAXIMUM_BYTES
		&& count_lines (file_contents, bytes_read) <= NAUTILUS_FILE_TOP_LEFT_TEXT_MAXIMUM_LINES;
}

static void
top_left_start (NautilusDirectory *directory)
{
	NautilusFile *file;
	char *uri;

	/* If there's already a read in progress, check to be sure
	 * it's still wanted.
	 */
	if (directory->details->top_left_read_state != NULL) {
		file = directory->details->top_left_read_state->file;
		if (file != NULL) {
			g_assert (NAUTILUS_IS_FILE (file));
			g_assert (file->details->directory == directory);
			if (is_needy (file,
				      lacks_top_left,
				      wants_top_left)) {
				return;
			}
		}

		/* The top left is not wanted, so stop it. */
		top_left_cancel (directory);
	}

	/* Figure out which file to read the top left for. */
	file = select_needy_file (directory,
				  lacks_top_left,
				  wants_top_left);
	if (file == NULL) {
		return;
	}

	if (!async_job_start (directory, "top left")) {
		return;
	}

	/* Start reading. */
	directory->details->top_left_read_state = g_new0 (TopLeftTextReadState, 1);
	directory->details->top_left_read_state->file = file;
	uri = nautilus_file_get_uri (file);
	directory->details->top_left_read_state->handle = nautilus_read_file_async
		(uri,
		 top_left_read_callback,
		 top_left_read_more_callback,
		 directory);
	g_free (uri);
}

static void
get_info_callback (GnomeVFSAsyncHandle *handle,
		   GList *results,
		   gpointer callback_data)
{
	NautilusDirectory *directory;
	NautilusFile *get_info_file;
	GnomeVFSGetFileInfoResult *result;

	directory = NAUTILUS_DIRECTORY (callback_data);
	g_assert (handle == NULL || handle == directory->details->get_info_in_progress);
	g_assert (nautilus_g_list_exactly_one_item (results));
	get_info_file = directory->details->get_info_file;
	g_assert (NAUTILUS_IS_FILE (get_info_file));
	
	directory->details->get_info_file = NULL;
	directory->details->get_info_in_progress = NULL;

	/* ref here because we might be removing the last ref when we
	 * mark the file gone below, but we need to keep a ref at
	 * least long enough to send the change notification. 
	 */
	nautilus_file_ref (get_info_file);

	result = results->data;

	if (result->result != GNOME_VFS_OK) {
		get_info_file->details->file_info_is_up_to_date = TRUE;
		if (get_info_file->details->info != NULL) {
			gnome_vfs_file_info_unref (get_info_file->details->info);
			get_info_file->details->info = NULL;
		}
		get_info_file->details->get_info_failed = TRUE;
		get_info_file->details->get_info_error = result->result;
		if (result->result == GNOME_VFS_ERROR_NOT_FOUND) {
			/* mark file as gone */

			get_info_file->details->is_gone = TRUE;
			if (get_info_file != directory->details->as_file) {
				nautilus_directory_remove_file (directory, get_info_file);
			}
		}
	} else {
		nautilus_file_update_info (get_info_file, result->file_info);
	}

	nautilus_file_changed (get_info_file);
	nautilus_file_unref (get_info_file);

	async_job_end (directory, "file info");
	nautilus_directory_async_state_changed (directory);
}

static void
file_info_start (NautilusDirectory *directory)
{
	NautilusFile *file;
	char *uri;
	GnomeVFSURI *vfs_uri;
	GList fake_list;

	/* If there's already a file info fetch in progress, check to
	 * be sure it's still wanted.
	 */
	if (directory->details->get_info_in_progress != NULL) {
		file = directory->details->get_info_file;
		if (file != NULL) {
			g_assert (NAUTILUS_IS_FILE (file));
			g_assert (file->details->directory == directory);
			if (is_needy (file, lacks_info, wants_info)) {
				return;
			}
		}

		/* The info is not wanted, so stop it. */
		file_info_cancel (directory);
	}

	/* Figure out which file to get file info for. */
	do {
		file = select_needy_file (directory, lacks_info, wants_info);
		if (file == NULL) {
			return;
		}
		
		uri = nautilus_file_get_uri (file);
		vfs_uri = gnome_vfs_uri_new (uri);
		g_free (uri);
		
		if (vfs_uri == NULL) {
			file->details->file_info_is_up_to_date = TRUE;
			file->details->get_info_failed = TRUE;
			file->details->get_info_error = GNOME_VFS_ERROR_INVALID_URI;
			nautilus_file_changed (file);
		}
	} while (vfs_uri == NULL);

	/* Found one we need to get the info for. */
	if (!async_job_start (directory, "file info")) {
		return;
	}
	directory->details->get_info_file = file;
	fake_list.data = vfs_uri;
	fake_list.prev = NULL;
	fake_list.next = NULL;
	gnome_vfs_async_get_file_info
		(&directory->details->get_info_in_progress,
		 &fake_list,
		 GNOME_VFS_FILE_INFO_GET_MIME_TYPE
		 | GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
		 get_info_callback,
		 directory);
	gnome_vfs_uri_unref (vfs_uri);
}

static void
activation_uri_done (NautilusDirectory *directory,
		     NautilusFile *file,
		     const char *uri)
{
	file->details->activation_uri_is_up_to_date = TRUE;

	file->details->got_activation_uri = TRUE;
	g_free (file->details->activation_uri);
	file->details->activation_uri = g_strdup (uri);

	async_job_end (directory, "activation URI");
	nautilus_directory_async_state_changed (directory);
}

static void
activation_uri_read_done (NautilusDirectory *directory,
			  const char *uri)
{
	NautilusFile *file;

	file = directory->details->activation_uri_read_state->file;
	g_free (directory->details->activation_uri_read_state);
	directory->details->activation_uri_read_state = NULL;

	activation_uri_done (directory, file, uri);
}

static void
activation_uri_nautilus_link_read_callback (GnomeVFSResult result,
					    GnomeVFSFileSize bytes_read,
					    char *file_contents,
					    gpointer callback_data)
{
	NautilusDirectory *directory;
	char *buffer, *uri;

	directory = NAUTILUS_DIRECTORY (callback_data);

	/* Handle the case where we read the Nautilus link. */
	if (result != GNOME_VFS_OK) {
		/* FIXME bugzilla.eazel.com 2433: We should report this error to the user. */
		g_free (file_contents);
		uri = NULL;
	} else {
		/* The gnome-xml parser requires a zero-terminated array. */
		buffer = g_realloc (file_contents, bytes_read + 1);
		buffer[bytes_read] = '\0';
		uri = nautilus_link_get_link_uri_given_file_contents (buffer, bytes_read);
		g_free (buffer);
	}

	activation_uri_read_done (directory, uri);
	g_free (uri);
}

static void
activation_uri_gmc_link_read_callback (GnomeVFSResult result,
				       GnomeVFSFileSize bytes_read,
				       char *file_contents,
				       gpointer callback_data)
{
	NautilusDirectory *directory;
	char *end_of_line, *uri;

	directory = NAUTILUS_DIRECTORY (callback_data);

	/* Handle the case where we read the GMC link. */
	if (result != GNOME_VFS_OK || !nautilus_str_has_prefix (file_contents, "URL: ")) {
		/* FIXME bugzilla.eazel.com 2433: We should report this error to the user. */
		uri = NULL;
	} else {
		/* Make sure we don't run off the end of the buffer. */
		end_of_line = memchr (file_contents, '\n', bytes_read);
		if (end_of_line != NULL) {
			uri = g_strndup (file_contents, end_of_line - file_contents);
		} else {
			uri = g_strndup (file_contents, bytes_read);
		}
	}

	g_free (file_contents);
	activation_uri_read_done (directory, uri);
	g_free (uri);
}

static gboolean
activation_uri_gmc_link_read_more_callback (GnomeVFSFileSize bytes_read,
					    const char *file_contents,
					    gpointer callback_data)
{
	g_assert (NAUTILUS_IS_DIRECTORY (callback_data));

	/* We need the first 512 bytes to see if something is a gmc link. */
	return bytes_read < 512;
}

static void
activation_uri_start (NautilusDirectory *directory)
{
	NautilusFile *file;
	char *mime_type, *uri;
	gboolean gmc_style_link, nautilus_style_link;

	/* If there's already a activation URI read in progress, check
	 * to be sure it's still wanted.
	 */
	if (directory->details->activation_uri_read_state != NULL) {
		file = directory->details->activation_uri_read_state->file;
		if (file != NULL) {
			g_assert (NAUTILUS_IS_FILE (file));
			g_assert (file->details->directory == directory);
			if (is_needy (file, lacks_info, wants_info)) {
				return;
			}
		}

		/* The count is not wanted, so stop it. */
		activation_uri_cancel (directory);
	}

	/* Figure out which file to get activation_uri for. */
	file = select_needy_file (directory,
				  lacks_activation_uri,
				  wants_activation_uri);
	if (file == NULL) {
		return;
	}

	if (!async_job_start (directory, "activation URI")) {
		return;
	}

	/* Figure out if it is a link. */
	mime_type = nautilus_file_get_mime_type (file);
	gmc_style_link = nautilus_strcasecmp (mime_type, "application/x-gmc-link") == 0;
	g_free (mime_type);
	nautilus_style_link = nautilus_file_is_nautilus_link (file);
	
	/* If it's not a link we are done. If it is, we need to read it. */
	if (!(gmc_style_link || nautilus_style_link)) {
		activation_uri_done (directory, file, NULL);
	} else {
		directory->details->activation_uri_read_state = g_new0 (ActivationURIReadState, 1);
		directory->details->activation_uri_read_state->file = file;
		uri = nautilus_file_get_uri (file);
		if (gmc_style_link) {
			directory->details->activation_uri_read_state->handle = nautilus_read_file_async
				(uri,
				 activation_uri_gmc_link_read_callback,
				 activation_uri_gmc_link_read_more_callback,
				 directory);
		} else {
			directory->details->activation_uri_read_state->handle = nautilus_read_entire_file_async
				(uri,
				 activation_uri_nautilus_link_read_callback,
				 directory);
		}
		g_free (uri);
	}
}

static void
start_or_stop_io (NautilusDirectory *directory)
{
	/* Start or stop getting file info. */
	file_info_start (directory);

	/* Start or stop reading the metafile. */
	metafile_read_start (directory);

	/* Start or stop reading files. */
	file_list_start (directory);

	/* Start or stop getting directory counts. */
	directory_count_start (directory);
	deep_count_start (directory);

	/* Start or stop getting mime lists. */
	mime_list_start (directory);

	/* Start or stop getting top left pieces of files. */
	top_left_start (directory);

	/* Start or stop getting activation URIs, which includes
	 * reading the contents of Nautilus and GMC link files.
	 */
	activation_uri_start (directory);
}

/* Call this when the monitor or call when ready list changes,
 * or when some I/O is completed.
 */
void
nautilus_directory_async_state_changed (NautilusDirectory *directory)
{
	/* Check if any callbacks are satisfied and call them if they
	 * are. Do this last so that any changes done in start or stop
	 * I/O functions immediately (not in callbacks) are taken into
	 * consideration. If any callbacks are called, consider the
	 * I/O state again so that we can release or cancel I/O that
	 * is not longer needed once the callbacks are satisfied.
	 */
	nautilus_directory_ref (directory);
	do {
		start_or_stop_io (directory);
	} while (call_ready_callbacks (directory));
	nautilus_directory_unref (directory);

	/* Check if any directories should wake up. */
	async_job_wake_up ();
}

void
nautilus_directory_cancel (NautilusDirectory *directory)
{
	/* Arbitrary order (kept alphabetical). */
	activation_uri_cancel (directory);
	deep_count_cancel (directory);
	directory_count_cancel (directory);
	file_info_cancel (directory);
	file_list_cancel (directory);
	metafile_read_cancel (directory);
	mime_list_cancel (directory);
	top_left_cancel (directory);

	/* We aren't waiting for anything any more. */
	if (waiting_directories != NULL) {
		g_hash_table_remove (waiting_directories, directory);
	}

	/* Check if any directories should wake up. */
	async_job_wake_up ();
}


static void
cancel_directory_count_for_file (NautilusDirectory *directory,
				 NautilusFile      *file)
{
	if (directory->details->count_file == file) {
		directory_count_cancel (directory);
	}
}


static void
cancel_deep_counts_for_file (NautilusDirectory *directory,
			     NautilusFile      *file)
{
	if (directory->details->deep_count_file == file) {
		deep_count_cancel (directory);
	}
}


static void
cancel_mime_list_for_file (NautilusDirectory *directory,
			   NautilusFile      *file)
{
	if (directory->details->mime_list_file == file) {
		mime_list_cancel (directory);
	}
}

static void
cancel_top_left_text_for_file (NautilusDirectory *directory,
			       NautilusFile      *file)
{
	if (directory->details->top_left_read_state != NULL &&
	    directory->details->top_left_read_state->file == file) {
		top_left_cancel (directory);
	}
}

static void
cancel_file_info_for_file (NautilusDirectory *directory,
			   NautilusFile      *file)
{
	if (directory->details->get_info_file == file) {
		file_info_cancel (directory);
	}
}

static void
cancel_activation_uri_for_file (NautilusDirectory *directory,
				NautilusFile      *file)
{
	if (directory->details->activation_uri_read_state != NULL &&
	    directory->details->activation_uri_read_state->file == file) {
		activation_uri_cancel (directory);
	}
}


static void
cancel_loading_attributes (NautilusDirectory *directory,
			   GList *file_attributes)
{
	Request request;
	
	nautilus_directory_set_up_request (&request,
					   file_attributes);

	if (request.directory_count) {
		directory_count_cancel (directory);
	}
	if (request.deep_count) {
		deep_count_cancel (directory);
	}
	if (request.mime_list) {
		mime_list_cancel (directory);
	}
	if (request.top_left_text) {
		top_left_cancel (directory);
	}
	if (request.file_info) {
		file_info_cancel (directory);
	}
	if (request.activation_uri) {
		file_info_cancel (directory);
	}
	
	/* FIXME bugzilla.eazel.com 5064: implement cancelling metadata when we
	   implement invalidating metadata */
}

void
nautilus_directory_cancel_loading_file_attributes (NautilusDirectory *directory,
						   NautilusFile      *file,
						   GList             *file_attributes)
{
	Request request;
	
	nautilus_directory_set_up_request (&request,
					   file_attributes);

	if (request.directory_count) {
		cancel_directory_count_for_file (directory, file);
	}
	if (request.deep_count) {
		cancel_deep_counts_for_file (directory, file);
	}
	if (request.mime_list) {
		cancel_mime_list_for_file (directory, file);
	}
	if (request.top_left_text) {
		cancel_top_left_text_for_file (directory, file);
	}
	if (request.file_info) {
		cancel_file_info_for_file (directory, file);
	}
	if (request.activation_uri) {
		cancel_activation_uri_for_file (directory, file);
	}

	/* FIXME bugzilla.eazel.com 5064: implement cancelling metadata when we
	   implement invalidating metadata */
}
