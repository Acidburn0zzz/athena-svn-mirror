/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-file-utilities..c - implementation of file manipulation routines.

   Copyright (C) 1999, 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: John Sullivan <sullivan@eazel.com>
*/

#include <config.h>
#include "nautilus-file-utilities.h"

#include "nautilus-file.h"
#include "nautilus-glib-extensions.h"
#include "nautilus-lib-self-check-functions.h"
#include "nautilus-link-set.h"
#include "nautilus-metadata.h"
#include "nautilus-string.h"
#include <ctype.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-xfer.h>
#include <pthread.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define NAUTILUS_USER_DIRECTORY_NAME ".nautilus"
#define DEFAULT_NAUTILUS_DIRECTORY_MODE (0755)

#define DESKTOP_DIRECTORY_NAME "desktop"
#define DEFAULT_DESKTOP_DIRECTORY_MODE (0755)

#define NAUTILUS_USER_MAIN_DIRECTORY_NAME "Nautilus"

#define READ_CHUNK_SIZE 8192

struct NautilusReadFileHandle {
	GnomeVFSAsyncHandle *handle;
	NautilusReadFileCallback callback;
	NautilusReadMoreCallback read_more_callback;
	gpointer callback_data;
	gboolean is_open;
	char *buffer;
	GnomeVFSFileSize bytes_read;
};

#undef PTHREAD_ASYNC_READ

#ifndef PTHREAD_ASYNC_READ
static void read_file_read_chunk (NautilusReadFileHandle *handle);
#endif

/**
 * nautilus_format_uri_for_display:
 *
 * Filter, modify, unescape and change URIs to make them appropriate
 * to display to users.
 *
 * @uri: a URI
 *
 * returns a g_malloc'd string
 **/
char *
nautilus_format_uri_for_display (const char *uri) 
{
	char *path;

	g_return_val_if_fail (uri != NULL, g_strdup (""));

	path = gnome_vfs_get_local_path_from_uri (uri);
	if (path != NULL) {
		return path;
	}
	
	return gnome_vfs_unescape_string_for_display (uri);
}

static gboolean
is_valid_scheme_character (char c)
{
	return isalnum ((guchar) c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_valid_scheme (const char *uri)
{
	const char *p;

	p = uri;

	if (!is_valid_scheme_character (*p)) {
		return FALSE;
	}

	do {
		p++;
	} while (is_valid_scheme_character (*p));

	return *p == ':';
}

/**
 * nautilus_make_uri_from_input:
 *
 * Takes a user input path/URI and makes a valid URI
 * out of it
 *
 * @location: a possibly mangled "uri"
 *
 * returns a newly allocated uri
 *
 **/
char *
nautilus_make_uri_from_input (const char *location)
{
	char *stripped, *path, *uri;

	g_return_val_if_fail (location != NULL, g_strdup (""));

	/* Strip off leading and trailing spaces.
	 * This makes copy/paste of URIs less error-prone.
	 */
	stripped = g_strstrip (g_strdup (location));

	switch (stripped[0]) {
	case '\0':
		uri = g_strdup ("");
		break;
	case '/':
		uri = gnome_vfs_get_uri_from_local_path (stripped);
		break;
	case '~':
		path = gnome_vfs_expand_initial_tilde (stripped);
		uri = gnome_vfs_get_uri_from_local_path (path);
		g_free (path);
		break;
	default:
		if (has_valid_scheme (stripped)) {
			uri = g_strdup (stripped);
		} else {
			uri = g_strconcat ("http://", stripped, NULL);
		}
	}

	g_free (stripped);

	return uri;
}

char *
nautilus_uri_get_basename (const char *uri)
{
	GnomeVFSURI *vfs_uri;
	char *name;

	/* Make VFS version of URI. */
	vfs_uri = gnome_vfs_uri_new (uri);
	if (vfs_uri == NULL) {
		return NULL;
	}

	/* Extract name part. */
	name = gnome_vfs_uri_extract_short_name (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);

	return name;
}

gboolean
nautilus_uri_is_trash (const char *uri)
{
	return nautilus_istr_has_prefix (uri, "trash:")
		|| nautilus_istr_has_prefix (uri, "gnome-trash:");
}

static gboolean
nautilus_uri_is_local_scheme (const char *uri)
{
	gboolean is_local_scheme;
	char *temp_scheme;
	int i;
	char *local_schemes[] = {"file", "help", "ghelp", "gnome-help",
				 "trash", "man", "info", 
				 "hardware", "search", "pipe",
				 "gnome-trash", NULL};

	is_local_scheme = FALSE;
	for (temp_scheme = *local_schemes, i = 0; temp_scheme != NULL; i++, temp_scheme = local_schemes[i]) {
		is_local_scheme = nautilus_istr_has_prefix (uri, temp_scheme);
		if (is_local_scheme) {
			break;
		}
	}
	

	return is_local_scheme;
}

static char *
nautilus_handle_trailing_slashes (const char *uri)
{
	char *temp, *uri_copy;
	gboolean previous_char_is_column, previous_chars_are_slashes_without_column;
	gboolean previous_chars_are_slashes_with_column;
	gboolean is_local_scheme;

	g_assert (uri != NULL);

	uri_copy = g_strdup (uri);
	if (strlen (uri_copy) <= 2) {
		return uri_copy;
	}

	is_local_scheme = nautilus_uri_is_local_scheme (uri);

	previous_char_is_column = FALSE;
	previous_chars_are_slashes_without_column = FALSE;
	previous_chars_are_slashes_with_column = FALSE;

	/* remove multiple trailing slashes */
	for (temp = uri_copy; *temp != '\0'; temp++) {
		if (*temp == '/' && !previous_char_is_column) {
			previous_chars_are_slashes_without_column = TRUE;
		} else if (*temp == '/' && previous_char_is_column) {
			previous_chars_are_slashes_without_column = FALSE;
			previous_char_is_column = TRUE;
			previous_chars_are_slashes_with_column = TRUE;
		} else {
			previous_chars_are_slashes_without_column = FALSE;
			previous_char_is_column = FALSE;
			previous_chars_are_slashes_with_column = FALSE;
		}

		if (*temp == ':') {
			previous_char_is_column = TRUE;
		}
	}

	if (*temp == '\0' && previous_chars_are_slashes_without_column) {
		if (is_local_scheme) {
			/* go back till you remove them all. */
			for (temp--; *(temp) == '/'; temp--) {
				*temp = '\0';
			}
		} else {
			/* go back till you remove them all but one. */
			for (temp--; *(temp - 1) == '/'; temp--) {
				*temp = '\0';
			}			
		}
	}

	if (*temp == '\0' && previous_chars_are_slashes_with_column) {
		/* go back till you remove them all but three. */
		for (temp--; *(temp - 3) != ':' && *(temp - 2) != ':' && *(temp - 1) != ':'; temp--) {
			*temp = '\0';
		}
	}


	return uri_copy;
}

char *
nautilus_make_uri_canonical (const char *uri)
{
	char *canonical_uri, *old_uri, *p;
	gboolean relative_uri;

	relative_uri = FALSE;

	if (uri == NULL) {
		return NULL;
	}

	/* Convert "gnome-trash:<anything>" and "trash:<anything>" to
	 * "trash:".
	 */
	if (nautilus_uri_is_trash (uri)) {
		return g_strdup (NAUTILUS_TRASH_URI);
	}

	/* FIXME bugzilla.eazel.com 648: 
	 * This currently ignores the issue of two uris that are not identical but point
	 * to the same data except for the specific cases of trailing '/' characters,
	 * file:/ and file:///, and "lack of file:".
	 */

	canonical_uri = nautilus_handle_trailing_slashes (uri);

	/* Note: In some cases, a trailing slash means nothing, and can
	 * be considered equivalent to no trailing slash. But this is
	 * not true in every case; specifically not for web addresses passed
	 * to a web-browser. So we don't have the trailing-slash-equivalence
	 * logic here, but we do use that logic in NautilusDirectory where
	 * the rules are more strict.
	 */

	/* Add file: if there is no scheme. */
	if (strchr (canonical_uri, ':') == NULL) {
		old_uri = canonical_uri;

		if (old_uri[0] != '/') {
			/* FIXME bugzilla.eazel.com 5069: 
			 *  bandaid alert. Is this really the right thing to do?
			 * 
			 * We got what really is a relative path. We do a little bit of
			 * a stretch here and assume it was meant to be a cryptic absolute path,
			 * and convert it to one. Since we can't call gnome_vfs_uri_new and
			 * gnome_vfs_uri_to_string to do the right make-canonical conversion,
			 * we have to do it ourselves.
			 */
			relative_uri = TRUE;
			canonical_uri = gnome_vfs_make_path_name_canonical (old_uri);
			g_free (old_uri);
			old_uri = canonical_uri;
			canonical_uri = g_strconcat ("file:///", old_uri, NULL);
		} else {
			canonical_uri = g_strconcat ("file:", old_uri, NULL);
		}
		g_free (old_uri);
	}

	/* Lower-case the scheme. */
	for (p = canonical_uri; *p != ':'; p++) {
		g_assert (*p != '\0');
		if (isupper (*p)) {
			*p = tolower (*p);
		}
	}

	if (!relative_uri) {
		old_uri = canonical_uri;
		canonical_uri = gnome_vfs_make_uri_canonical (canonical_uri);
		if (canonical_uri != NULL) {
			g_free (old_uri);
		} else {
			canonical_uri = old_uri;
		}
	}
	
	/* FIXME bugzilla.eazel.com 2802:
	 * Work around gnome-vfs's desire to convert file:foo into file://foo
	 * by converting to file:///foo here. When you remove this, check that
	 * typing "foo" into location bar does not crash and returns an error
	 * rather than displaying the contents of /
	 */
	if (nautilus_str_has_prefix (canonical_uri, "file://")
	    && !nautilus_str_has_prefix (canonical_uri, "file:///")) {
		old_uri = canonical_uri;
		canonical_uri = g_strconcat ("file:/", old_uri + 5, NULL);
		g_free (old_uri);
	}

	return canonical_uri;
}

gboolean
nautilus_uris_match (const char *uri_1, const char *uri_2)
{
	char *canonical_1;
	char *canonical_2;
	gboolean result;

	canonical_1 = nautilus_make_uri_canonical (uri_1);
	canonical_2 = nautilus_make_uri_canonical (uri_2);

	result = nautilus_str_is_equal (canonical_1, canonical_2);

	g_free (canonical_1);
	g_free (canonical_2);
	
	return result;
}


/**
 * nautilus_make_path:
 * 
 * Make a path name from a base path and name. The base path
 * can end with or without a separator character.
 *
 * Return value: the combined path name.
 **/
char * 
nautilus_make_path (const char *path, const char* name)
{
    	gboolean insert_separator;
    	int path_length;
	char *result;
	
	path_length = strlen (path);
    	insert_separator = path_length > 0 && 
    			   name[0] != '\0' && 
    			   path[path_length - 1] != G_DIR_SEPARATOR;

    	if (insert_separator) {
    		result = g_strconcat (path, G_DIR_SEPARATOR_S, name, NULL);
    	} else {
    		result = g_strconcat (path, name, NULL);
    	}

	return result;
}

/**
 * nautilus_get_user_directory:
 * 
 * Get the path for the directory containing nautilus settings.
 *
 * Return value: the directory path.
 **/
char *
nautilus_get_user_directory (void)
{
	char *user_directory = NULL;

	user_directory = nautilus_make_path (g_get_home_dir (),
					     NAUTILUS_USER_DIRECTORY_NAME);

	if (!g_file_exists (user_directory)) {
		mkdir (user_directory, DEFAULT_NAUTILUS_DIRECTORY_MODE);
		/* FIXME bugzilla.eazel.com 1286: 
		 * How should we handle the case where this mkdir fails? 
		 * Note that nautilus_application_startup will refuse to launch if this 
		 * directory doesn't get created, so that case is OK. But the directory 
		 * could be deleted after Nautilus was launched, and perhaps
		 * there is some bad side-effect of not handling that case.
		 */
	}

	return user_directory;
}

/**
 * nautilus_get_desktop_directory:
 * 
 * Get the path for the directory containing files on the desktop.
 *
 * Return value: the directory path.
 **/
char *
nautilus_get_desktop_directory (void)
{
	char *desktop_directory, *user_directory;

	user_directory = nautilus_get_user_directory ();
	desktop_directory = nautilus_make_path (user_directory, DESKTOP_DIRECTORY_NAME);
	g_free (user_directory);

	if (!g_file_exists (desktop_directory)) {
		mkdir (desktop_directory, DEFAULT_DESKTOP_DIRECTORY_MODE);
		/* FIXME bugzilla.eazel.com 1286: 
		 * How should we handle the case where this mkdir fails? 
		 * Note that nautilus_application_startup will refuse to launch if this 
		 * directory doesn't get created, so that case is OK. But the directory 
		 * could be deleted after Nautilus was launched, and perhaps
		 * there is some bad side-effect of not handling that case.
		 */
	}

	return desktop_directory;
}

/**
  * nautilus_user_main_directory_exists:
  *
  * returns true if the user directory exists.  This must be called
  * before nautilus_get_user_main_directory, which creates it if necessary
  *
  **/
gboolean
nautilus_user_main_directory_exists(void)
{
	gboolean directory_exists;
	char *main_directory;
	
	main_directory = g_strdup_printf ("%s/%s",
					g_get_home_dir(),
					NAUTILUS_USER_MAIN_DIRECTORY_NAME);
	directory_exists = g_file_exists(main_directory);
	g_free(main_directory);
	return directory_exists;
}


/**
 * nautilus_get_user_main_directory:
 * 
 * Get the path for the user's main Nautilus directory.  
 * Usually ~/Nautilus
 *
 * Return value: the directory path.
 **/
char *
nautilus_get_user_main_directory (void)
{
	char *user_main_directory = NULL;
	GnomeVFSResult result;
	NautilusFile *file;
	char *file_uri, *image_uri, *temp_str;
	char *source_directory_uri_text, *destination_directory_uri_text;
	GnomeVFSURI *source_directory_uri, *destination_directory_uri;
	GnomeVFSURI *source_uri, *destination_uri;
	
	user_main_directory = g_strdup_printf ("%s/%s",
					       g_get_home_dir(),
					       NAUTILUS_USER_MAIN_DIRECTORY_NAME);
												
	if (!g_file_exists (user_main_directory)) {			
		source_directory_uri_text = gnome_vfs_get_uri_from_local_path (NAUTILUS_DATADIR);
		source_directory_uri = gnome_vfs_uri_new (source_directory_uri_text);
		g_free (source_directory_uri_text);
		source_uri = gnome_vfs_uri_append_file_name (source_directory_uri, "top");
		gnome_vfs_uri_unref (source_directory_uri);

		destination_directory_uri_text = gnome_vfs_get_uri_from_local_path (g_get_home_dir());
		destination_directory_uri = gnome_vfs_uri_new (destination_directory_uri_text);
		g_free (destination_directory_uri_text);
		destination_uri = gnome_vfs_uri_append_file_name (destination_directory_uri, 
								  NAUTILUS_USER_MAIN_DIRECTORY_NAME);
		gnome_vfs_uri_unref (destination_directory_uri);
		
		result = gnome_vfs_xfer_uri (source_uri, destination_uri,
					 GNOME_VFS_XFER_RECURSIVE, GNOME_VFS_XFER_ERROR_MODE_ABORT,
					 GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
					 NULL, NULL);
		
		/* FIXME bugzilla.eazel.com 1286: 
		 * How should we handle error codes returned from gnome_vfs_xfer_uri? 
		 * Note that nautilus_application_startup will refuse to launch if this 
		 * directory doesn't get created, so that case is OK. But the directory 
		 * could be deleted after Nautilus was launched, and perhaps
		 * there is some bad side-effect of not handling that case.
		 */

		gnome_vfs_uri_unref (source_uri);
		gnome_vfs_uri_unref (destination_uri);

		/* If this fails to create the directory, nautilus_application_startup will
		 * notice and refuse to launch.
		 */
					
		/* assign a custom image for the directory icon */
		file_uri = gnome_vfs_get_uri_from_local_path (user_main_directory);
		temp_str = nautilus_pixmap_file ("nautilus-logo.png");
		image_uri = gnome_vfs_get_uri_from_local_path (temp_str);
		g_free (temp_str);
		
		file = nautilus_file_get (file_uri);
		g_free (file_uri);
		if (file != NULL) {
			nautilus_file_set_metadata (file,
						    NAUTILUS_METADATA_KEY_CUSTOM_ICON,
						    NULL,
						    image_uri);
			nautilus_file_unref (file);
		}
		
		/* now do the same for the about file */
		temp_str = g_strdup_printf ("%s/About.html", user_main_directory);
		file_uri = gnome_vfs_get_uri_from_local_path (temp_str);
		g_free (temp_str);
		
		file = nautilus_file_get (file_uri);
		if (file != NULL) {
			nautilus_file_set_metadata (file,
						    NAUTILUS_METADATA_KEY_CUSTOM_ICON,
						    NULL,
						    image_uri);
			nautilus_file_unref (file);
		}
		g_free (file_uri);
		
		g_free (image_uri);
		
		/* install the default link set */
		nautilus_link_set_install (user_main_directory, "apps");
		/*
		  nautilus_link_set_install (user_main_directory, "search_engines");
		*/
	}

	return user_main_directory;
}

/**
 * nautilus_get_pixmap_directory
 * 
 * Get the path for the directory containing Nautilus pixmaps.
 *
 * Return value: the directory path.
 **/
char *
nautilus_get_pixmap_directory (void)
{
	char *pixmap_directory;

	pixmap_directory = g_strdup_printf ("%s/%s", DATADIR, "pixmaps/nautilus");

	return pixmap_directory;
}

/* convenience routine to use gnome-vfs to test if a string is a remote uri */
gboolean
nautilus_is_remote_uri (const char *uri)
{
	gboolean is_local;
	GnomeVFSURI *vfs_uri;
	
	vfs_uri = gnome_vfs_uri_new (uri);
	is_local = gnome_vfs_uri_is_local (vfs_uri);
	gnome_vfs_uri_unref(vfs_uri);
	return !is_local;
}


/* FIXME bugzilla.eazel.com 2423: 
 * Callers just use this and dereference so we core dump if
 * pixmaps are missing. That is lame.
 */
char *
nautilus_pixmap_file (const char *partial_path)
{
	char *path;

	path = nautilus_make_path (DATADIR "/pixmaps/nautilus", partial_path);
	if (g_file_exists (path)) {
		return path;
	} else {
		g_free (path);
		return NULL;
	}
}

GnomeVFSResult
nautilus_read_entire_file (const char *uri,
			   int *file_size,
			   char **file_contents)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	char *buffer;
	GnomeVFSFileSize total_bytes_read;
	GnomeVFSFileSize bytes_read;

	*file_size = 0;
	*file_contents = NULL;

	/* Open the file. */
	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		return result;
	}

	/* Read the whole thing. */
	buffer = NULL;
	total_bytes_read = 0;
	do {
		buffer = g_realloc (buffer, total_bytes_read + READ_CHUNK_SIZE);
		result = gnome_vfs_read (handle,
					 buffer + total_bytes_read,
					 READ_CHUNK_SIZE,
					 &bytes_read);
		if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
			g_free (buffer);
			gnome_vfs_close (handle);
			return result;
		}

		/* Check for overflow. */
		if (total_bytes_read + bytes_read < total_bytes_read) {
			g_free (buffer);
			gnome_vfs_close (handle);
			return GNOME_VFS_ERROR_TOO_BIG;
		}

		total_bytes_read += bytes_read;
	} while (result == GNOME_VFS_OK);

	/* Close the file. */
	result = gnome_vfs_close (handle);
	if (result != GNOME_VFS_OK) {
		g_free (buffer);
		return result;
	}

	/* Return the file. */
	*file_size = total_bytes_read;
	*file_contents = g_realloc (buffer, total_bytes_read);
	return GNOME_VFS_OK;
}

#ifndef PTHREAD_ASYNC_READ
/* When close is complete, there's no more work to do. */
static void
read_file_close_callback (GnomeVFSAsyncHandle *handle,
			  GnomeVFSResult result,
			  gpointer callback_data)
{
}

/* Do a close if it's needed.
 * Be sure to get this right, or we have extra threads hanging around.
 */
static void
read_file_close (NautilusReadFileHandle *read_handle)
{
	if (read_handle->is_open) {
		gnome_vfs_async_close (read_handle->handle,
				       read_file_close_callback,
				       NULL);
		read_handle->is_open = FALSE;
	}
}

/* Close the file and then tell the caller we succeeded, handing off
 * the buffer to the caller.
 */
static void
read_file_succeeded (NautilusReadFileHandle *read_handle)
{
	read_file_close (read_handle);
	
	/* Reallocate the buffer to the exact size since it might be
	 * around for a while.
	 */
	(* read_handle->callback) (GNOME_VFS_OK,
				   read_handle->bytes_read,
				   g_realloc (read_handle->buffer,
					      read_handle->bytes_read),
				   read_handle->callback_data);

	g_free (read_handle);
}

/* Tell the caller we failed. */
static void
read_file_failed (NautilusReadFileHandle *read_handle, GnomeVFSResult result)
{
	read_file_close (read_handle);
	g_free (read_handle->buffer);
	
	(* read_handle->callback) (result, 0, NULL, read_handle->callback_data);
	g_free (read_handle);
}

/* A read is complete, so we might or might not be done. */
static void
read_file_read_callback (GnomeVFSAsyncHandle *handle,
				GnomeVFSResult result,
				gpointer buffer,
				GnomeVFSFileSize bytes_requested,
				GnomeVFSFileSize bytes_read,
				gpointer callback_data)
{
	NautilusReadFileHandle *read_handle;
	gboolean read_more;

	/* Do a few reality checks. */
	g_assert (bytes_requested == READ_CHUNK_SIZE);
	read_handle = callback_data;
	g_assert (read_handle->handle == handle);
	g_assert (read_handle->buffer + read_handle->bytes_read == buffer);
	g_assert (bytes_read <= bytes_requested);

	/* Check for a failure. */
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
		read_file_failed (read_handle, result);
		return;
	}

	/* Check for the extremely unlikely case where the file size overflows. */
	if (read_handle->bytes_read + bytes_read < read_handle->bytes_read) {
		read_file_failed (read_handle, GNOME_VFS_ERROR_TOO_BIG);
		return;
	}

	/* Bump the size. */
	read_handle->bytes_read += bytes_read;

	/* Read more unless we are at the end of the file. */
	if (bytes_read == 0 || result != GNOME_VFS_OK) {
		read_more = FALSE;
	} else {
		if (read_handle->read_more_callback == NULL) {
			read_more = TRUE;
		} else {
			read_more = (* read_handle->read_more_callback)
				(read_handle->bytes_read,
				 read_handle->buffer,
				 read_handle->callback_data);
		}
	}
	if (read_more) {
		read_file_read_chunk (read_handle);
		return;
	}

	/* If at the end of the file, we win! */
	read_file_succeeded (read_handle);
}

/* Start reading a chunk. */
static void
read_file_read_chunk (NautilusReadFileHandle *handle)
{
	handle->buffer = g_realloc (handle->buffer, handle->bytes_read + READ_CHUNK_SIZE);
	gnome_vfs_async_read (handle->handle,
			      handle->buffer + handle->bytes_read,
			      READ_CHUNK_SIZE,
			      read_file_read_callback,
			      handle);
}

/* Once the open is finished, read a first chunk. */
static void
read_file_open_callback (GnomeVFSAsyncHandle *handle,
			 GnomeVFSResult result,
			 gpointer callback_data)
{
	NautilusReadFileHandle *read_handle;
	
	read_handle = callback_data;
	g_assert (read_handle->handle == handle);

	/* Handle the failure case. */
	if (result != GNOME_VFS_OK) {
		read_file_failed (read_handle, result);
		return;
	}

	/* Handle success by reading the first chunk. */
	read_handle->is_open = TRUE;
	read_file_read_chunk (read_handle);
}

#else

typedef struct {
	NautilusReadFileCallback callback;
	NautilusReadMoreCallback more_callback;
	gpointer callback_data;
	pthread_mutex_t *callback_result_ready_semaphore;
	gboolean synch_callback_result;

	GnomeVFSResult result;
	GnomeVFSFileSize file_size;
	char *buffer;
} NautilusAsyncReadFileCallbackData;

static int
pthread_nautilus_read_file_callback_idle_binder (void *cast_to_context)
{
	NautilusAsyncReadFileCallbackData *context;
	
	context = (NautilusAsyncReadFileCallbackData *)cast_to_context;

	if (context->more_callback) {
		g_assert (context->callback_result_ready_semaphore != NULL);
		/* Synchronous callback flavor, wait for the return value. */
		context->synch_callback_result = (* context->more_callback) (context->file_size, 
			context->buffer, context->callback_data);
		/* Got the result, release the master thread */
		pthread_mutex_unlock (context->callback_result_ready_semaphore);
	} else {
		/* Asynchronous callback flavor, don't wait for the result. */
		(* context->callback) (context->result, context->file_size, 
			context->buffer, context->callback_data);

		/* We assume ownership of data here in the async call and have to
		 * free it.
		 */
		g_free (context);
	}

	return FALSE;
}

static gboolean
pthread_nautilus_read_file_callback_common (NautilusReadFileCallback callback,
	NautilusReadMoreCallback more_callback, gpointer callback_data, 
	GnomeVFSResult error, GnomeVFSFileSize file_size,
	char *buffer, pthread_mutex_t *callback_result_ready_semaphore)
{
	NautilusAsyncReadFileCallbackData *data;
	gboolean result;

	g_assert ((callback == NULL) != (more_callback == NULL));
	g_assert ((more_callback != NULL) == (callback_result_ready_semaphore != NULL));

	result = FALSE;
	data = g_new0 (NautilusAsyncReadFileCallbackData, 1);
	data->callback = callback;
	data->more_callback = more_callback;
	data->callback_data = callback_data;
	data->callback_result_ready_semaphore = callback_result_ready_semaphore;
	data->result = error;
	data->file_size = file_size;
	data->buffer = buffer;
	
	/* Set up the callback to get called in the main thread. */
	g_idle_add (pthread_nautilus_read_file_callback_idle_binder, data);

	if (callback_result_ready_semaphore != NULL) {
		/* Block until callback deposits the return value. This is not optimal but we do it
		 * to emulate the nautilus_read_file_async call behavior.
		 */
		pthread_mutex_lock (callback_result_ready_semaphore);
		result = data->synch_callback_result;

		/* In the synch call we still own data here and need to free it. */
		g_free (data);

	}

	return result;
}

static gboolean
pthread_nautilus_read_file_synchronous_callback (NautilusReadMoreCallback callback,
	gpointer callback_data, GnomeVFSFileSize file_size,
	char *buffer, pthread_mutex_t *callback_result_ready_semaphore)
{
	return pthread_nautilus_read_file_callback_common(NULL, callback,
		callback_data, GNOME_VFS_OK, file_size, buffer, callback_result_ready_semaphore);
}

static void
pthread_nautilus_read_file_asynchronous_callback (NautilusReadFileCallback callback,
	gpointer callback_data, GnomeVFSResult result, GnomeVFSFileSize file_size,
	char *buffer)
{
	pthread_nautilus_read_file_callback_common(callback, NULL,
		callback_data, result, file_size, buffer, NULL);
}

typedef struct {
	NautilusReadFileHandle handle;
	char *uri;
	volatile gboolean cancel_requested;
	/* Expose the synch callback semaphore to allow the cancel call to unlock it. */
	pthread_mutex_t *callback_result_ready_semaphore;
} NautilusAsyncReadFileData;

static void *
pthread_nautilus_read_file_thread_entry (void *cast_to_data)
{
	NautilusAsyncReadFileData *data;
	GnomeVFSResult result;
	char *buffer;
	GnomeVFSFileSize total_bytes_read;
	GnomeVFSFileSize bytes_read;
	pthread_mutex_t callback_result_ready_semaphore;
	
	data = (NautilusAsyncReadFileData *)cast_to_data;
	buffer = NULL;
	total_bytes_read = 0;

	result = gnome_vfs_open ((GnomeVFSHandle **)&data->handle.handle, data->uri, GNOME_VFS_OPEN_READ);
	if (result == GNOME_VFS_OK) {
	
		if (data->handle.read_more_callback != NULL) {
			/* read_more_callback is a synchronous callback, allocate a semaphore
			 * to provide for synchoronization with the callback.
			 * We are using the default mutex attributes that give us a fast mutex
			 * that behaves like a semaphore.
			 */
			pthread_mutex_init (&callback_result_ready_semaphore, NULL);
			/* Grab the semaphore -- the next lock will block us and
			 * we will need the callback to unblock the semaphore.
			 */
			pthread_mutex_lock (&callback_result_ready_semaphore);
			data->callback_result_ready_semaphore = &callback_result_ready_semaphore;
		}
		for (;;) {
			if (data->cancel_requested) {
				/* Cancelled by the master. */
				result = GNOME_VFS_ERROR_INTERRUPTED;
				break;
			}

			buffer = g_realloc (buffer, total_bytes_read + READ_CHUNK_SIZE);
			/* FIXME bugzilla.eazel.com 5070:
			 * For a better cancellation granularity we should use gnome_vfs_read_cancellable
			 * here, adding a GnomeVFSContext to NautilusAsyncReadFileData.
			 */
			result = gnome_vfs_read ((GnomeVFSHandle *)data->handle.handle, buffer + total_bytes_read,
				READ_CHUNK_SIZE, &bytes_read);

			total_bytes_read += bytes_read;

			if (data->cancel_requested) {
				/* Cancelled by the master. */
				result = GNOME_VFS_ERROR_INTERRUPTED;
				break;
			}

			if (result != GNOME_VFS_OK) {
				if (result == GNOME_VFS_ERROR_EOF) {
					/* not really an error, just done reading */
					result = GNOME_VFS_OK;
				}
				break;
			}

			if (data->handle.read_more_callback != NULL
				&& !pthread_nautilus_read_file_synchronous_callback (data->handle.read_more_callback,
					data->handle.callback_data, total_bytes_read, buffer, 
					&callback_result_ready_semaphore)) {
				/* callback doesn't want any more data */
				break;
			}

		}
		gnome_vfs_close ((GnomeVFSHandle *)data->handle.handle);
	}

	if (result != GNOME_VFS_OK) {
		/* Because of the error or cancellation, nobody will take the data we read, 
		 * delete the buffer here instead.
		 */
		g_free (buffer);
		buffer = NULL;
		total_bytes_read = 0;
	}

	/* Call the final callback. 
	 * If everything is OK, pass in the data read. 
	 * We are handing off the read buffer -- trim it to the actual size we need first
	 * so that it doesn't take up more space than needed.
	 */
	pthread_nautilus_read_file_asynchronous_callback(data->handle.callback, 
		data->handle.callback_data, result, total_bytes_read, 
		g_realloc (buffer, total_bytes_read));

	if (data->handle.read_more_callback != NULL) {
		pthread_mutex_destroy (&callback_result_ready_semaphore);
	}

	g_free (data->uri);
	g_free (data);

	return NULL;
}

static NautilusReadFileHandle *
pthread_nautilus_read_file_async(const char *uri, NautilusReadFileCallback callback, 
	NautilusReadMoreCallback read_more_callback, gpointer callback_data)
{
	NautilusAsyncReadFileData *data;
	pthread_attr_t thread_attr;
	pthread_t thread;

	data = g_new0 (NautilusAsyncReadFileData, 1);

	data->handle.callback = callback;
	data->handle.read_more_callback = read_more_callback;
	data->handle.callback_data = callback_data;
	data->cancel_requested = FALSE;
	data->uri = g_strdup (uri);

	pthread_attr_init (&thread_attr);
	pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create (&thread, &thread_attr, pthread_nautilus_read_file_thread_entry, data) != 0) {
		/* FIXME bugzilla.eazel.com 5071:
		 * Would be cleaner to call through an idle callback here.
		 */
		(*callback) (GNOME_VFS_ERROR_INTERNAL, 0, NULL, NULL);
		g_free (data);
		return NULL;
	}

	return (NautilusReadFileHandle *)data;
}

static void
pthread_nautilus_read_file_async_cancel (NautilusReadFileHandle *handle)
{
	/* Must call this before the final callback kicks in. */
	NautilusAsyncReadFileData *data;

	data = (NautilusAsyncReadFileData *)handle;
	data->cancel_requested = TRUE;
	if (data->callback_result_ready_semaphore != NULL) {
		pthread_mutex_unlock (data->callback_result_ready_semaphore);
	}

	/* now the thread will die on it's own and clean up after itself */
}

#endif

/* Set up the read handle and start reading. */
NautilusReadFileHandle *
nautilus_read_file_async (const char *uri,
			  NautilusReadFileCallback callback,
			  NautilusReadMoreCallback read_more_callback,
			  gpointer callback_data)
{
#ifndef PTHREAD_ASYNC_READ
	NautilusReadFileHandle *handle;

	handle = g_new0 (NautilusReadFileHandle, 1);

	handle->callback = callback;
	handle->read_more_callback = read_more_callback;
	handle->callback_data = callback_data;

	gnome_vfs_async_open (&handle->handle,
			      uri,
			      GNOME_VFS_OPEN_READ,
			      read_file_open_callback,
			      handle);
	return handle;
#else
	return pthread_nautilus_read_file_async(uri, callback, 
		read_more_callback, callback_data);
#endif
}

/* Set up the read handle and start reading. */
NautilusReadFileHandle *
nautilus_read_entire_file_async (const char *uri,
				 NautilusReadFileCallback callback,
				 gpointer callback_data)
{
	return nautilus_read_file_async (uri, callback, NULL, callback_data);
}

/* Stop the presses! */
void
nautilus_read_file_cancel (NautilusReadFileHandle *handle)
{
#ifndef PTHREAD_ASYNC_READ
	gnome_vfs_async_cancel (handle->handle);
	read_file_close (handle);
	g_free (handle->buffer);
	g_free (handle);
#else

	pthread_nautilus_read_file_async_cancel (handle);
#endif
}

GnomeVFSResult
nautilus_make_directory_and_parents (GnomeVFSURI *uri, guint permissions)
{
	GnomeVFSResult result;
	GnomeVFSURI *parent_uri;

	/* Make the directory, and return right away unless there's
	   a possible problem with the parent.
	*/
	result = gnome_vfs_make_directory_for_uri (uri, permissions);
	if (result != GNOME_VFS_ERROR_NOT_FOUND) {
		return result;
	}

	/* If we can't get a parent, we are done. */
	parent_uri = gnome_vfs_uri_get_parent (uri);
	if (parent_uri == NULL) {
		return result;
	}

	/* If we can get a parent, use a recursive call to create
	   the parent and its parents.
	*/
	result = nautilus_make_directory_and_parents (parent_uri, permissions);
	gnome_vfs_uri_unref (parent_uri);
	if (result != GNOME_VFS_OK) {
		return result;
	}

	/* A second try at making the directory after the parents
	   have all been created.
	*/
	result = gnome_vfs_make_directory_for_uri (uri, permissions);
	return result;
}

GnomeVFSResult
nautilus_copy_uri_simple ( const char *source_uri, const char *dest_uri)
{
	GnomeVFSResult result;
	GnomeVFSURI *real_source_uri, *real_dest_uri;
	real_source_uri = gnome_vfs_uri_new (source_uri);
	real_dest_uri = gnome_vfs_uri_new (dest_uri);
		
	result = gnome_vfs_xfer_uri (real_source_uri, real_dest_uri,
					GNOME_VFS_XFER_RECURSIVE, GNOME_VFS_XFER_ERROR_MODE_ABORT,
					GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
					NULL, NULL);
		
	gnome_vfs_uri_unref (real_source_uri);
	gnome_vfs_uri_unref (real_dest_uri);
		
	return  result;
}

char *
nautilus_unique_temporary_file_name (void)
{
	const char *prefix = "/tmp/nautilus-temp-file";
	char *file_name;
	static guint count = 1;

	file_name = g_strdup_printf ("%sXXXXXX", prefix);

	if (mktemp (file_name) != file_name) {
		g_free (file_name);
		file_name = g_strdup_printf ("%s-%d-%d", prefix, count++, getpid ());
	}

	return file_name;
}

char *
nautilus_get_build_time_stamp (void)
{
#ifdef EAZEL_BUILD_TIMESTAMP
	return g_strdup (EAZEL_BUILD_TIMESTAMP);
#else
	return NULL;
#endif
}

#if !defined (NAUTILUS_OMIT_SELF_CHECK)

void
nautilus_self_check_file_utilities (void)
{
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (""), "");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (" "), "");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (" / "), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (" /"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (" /home\n\n"), "file:///home");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (" \n\t"), "");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("!"), "http://!");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("#"), "http://#");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/ "), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/!"), "file:///!");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/#"), "file:///%23");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/%20"), "file:///%2520");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/%25"), "file:///%2525");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/:"), "file:///%3A");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/home"), "file:///home");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("/home/darin"), "file:///home/darin");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (":"), "http://:");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("::"), "http://::");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input (":://:://:::::::::::::::::"), "http://:://:://:::::::::::::::::");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:"), "file:");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:///%20"), "file:///%20");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:///%3F"), "file:///%3F");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:///:"), "file:///:");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:///?"), "file:///?");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:///home/joe/some file"), "file:///home/joe/some file");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file://home/joe/some file"), "file://home/joe/some file");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("file:::::////"), "file:::::////");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("foo://foobar.txt"), "foo://foobar.txt");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("home"), "http://home");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("http://null.stanford.edu"), "http://null.stanford.edu");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("http://null.stanford.edu:80"), "http://null.stanford.edu:80");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("http://seth@null.stanford.edu:80"), "http://seth@null.stanford.edu:80");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("http:::::::::"), "http:::::::::");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("www.eazel.com"), "http://www.eazel.com");
        NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_from_input ("http://null.stanford.edu/some file"), "http://null.stanford.edu/some file");

	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("file:///////"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("file://foo/"), "file://foo");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("file://foo"), "file://foo");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("file://"), "file://");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("file:/"), "file:/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("http://le-hacker.org"), "http://le-hacker.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("http://le-hacker.org/dir//////"), "http://le-hacker.org/dir/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_handle_trailing_slashes ("http://le-hacker.org/////"), "http://le-hacker.org/");

	/* nautilus_make_uri_canonical */

	/* FIXME bugzilla.eazel.com 5072: this is a bizarre result from an empty string */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical (""), "file:///");
	
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:/"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///home/mathieu/"), "file:///home/mathieu");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///home/mathieu"), "file:///home/mathieu");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("ftp://mathieu:password@le-hackeur.org"), "ftp://mathieu:password@le-hackeur.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("ftp://mathieu:password@le-hackeur.org/"), "ftp://mathieu:password@le-hackeur.org/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://le-hackeur.org"), "http://le-hackeur.org");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://le-hackeur.org/"), "http://le-hackeur.org/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://le-hackeur.org/dir"), "http://le-hackeur.org/dir");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://le-hackeur.org/dir/"), "http://le-hackeur.org/dir/");

	/* FIXME bugzilla.eazel.com 5068: the "nested" URI loses some characters here. Maybe that's OK because we escape them in practice? */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("search://[file://]file_name contains stuff"), "search://[file/]file_name contains stuff");
#ifdef EAZEL_SERVICES
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("eazel-services:/~turtle"), "eazel-services:///~turtle");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("eazel-services:///~turtle"), "eazel-services:///~turtle");
#endif

	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/"), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/./."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/.//."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/.///."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a"), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/a/b/.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a///"), "file:///a/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a"), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("../a"), "file:///../a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("..//a"), "file:///../a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/a/."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/a/.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a//."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a/."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical (".//a/."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a//."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a//.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a/.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical (".//a/.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a//.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical (".//a//.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/b/.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a/b/.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/./a/b/.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/a/./b/.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/a/b/./.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/a/b/../."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/b/../.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("./a/b/../.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("././a/b/../.."), "file:///");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/b/c/../.."), "file:///a");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/b/c/../../d"), "file:///a/d");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/b/../../d"), "file:///d");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/../../d"), "file:///../d");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/b/.././.././c"), "file:///c");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("a/.././.././b/c"), "file:///../b/c");

	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://www.eazel.com"), "http://www.eazel.com");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://www.eazel.com/"), "http://www.eazel.com/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://www.eazel.com/dir"), "http://www.eazel.com/dir");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://www.eazel.com/dir/"), "http://www.eazel.com/dir/");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://yakk:womble@www.eazel.com:42/blah/"), "http://yakk:womble@www.eazel.com:42/blah/");

	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("FILE:///"), "file:///");

	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///trash"), "file:///trash");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///Users/mikef"), "file:///Users/mikef");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/trash"), "file:///trash");

	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("root"), "file:///root");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("/root"), "file:///root");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("//root"), "file:///root");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("///root"), "file:///root");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("////root"), "file:///root");

	/* Test cases related to escaping. */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///%3F"), "file:///%3F");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///%78"), "file:///x");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///?"), "file:///%3F");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:///x"), "file:///x");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("glorb:///%3F"), "glorb:///%3F");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("glorb:///%78"), "glorb:///x");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("glorb:///?"), "glorb:///%3F");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("glorb:///x"), "glorb:///x");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http:///%3F"), "http:///%3F");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http:///%78"), "http:///x");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http:///?"), "http:///?");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http:///x"), "http:///x");

	/* FIXME bugzilla.eazel.com 4101: Why append a slash in this case, but not in the http://www.eazel.com case? */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("http://www.eazel.com:80"), "http://www.eazel.com:80/");

	/* Note: these cases behave differently here than in
	 * gnome-vfs. In some cases because of bugs in gnome-vfs, but
	 * in other cases because we just want them handled
	 * differently.
	 */
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:trash"), "file:trash");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("//trash"), "file:///trash");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("file:"), "file:");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("trash"), "file:///trash");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("glorp:"), "glorp:");
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("TRASH:XXX"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("trash:xxx"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("GNOME-TRASH:XXX"), NAUTILUS_TRASH_URI);
	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("gnome-trash:xxx"), NAUTILUS_TRASH_URI);

	NAUTILUS_CHECK_STRING_RESULT (nautilus_make_uri_canonical ("pipe:gnome-info2html2 as"), "pipe:gnome-info2html2 as");
}

#endif /* !NAUTILUS_OMIT_SELF_CHECK */
