/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <gnome.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include "file-data.h"
#include "file-list.h"
#include "file-utils.h"
#include "fr-archive.h"
#include "fr-command.h"
#include "fr-command-arj.h"
#include "fr-command-cfile.h"
#include "fr-command-iso.h"
#include "fr-command-lha.h"
#include "fr-command-rar.h"
#include "fr-command-rpm.h"
#include "fr-command-tar.h"
#include "fr-command-unstuff.h"
#include "fr-command-zip.h"
#include "fr-command-zoo.h"
#include "fr-error.h"
#include "fr-marshal.h"
#include "fr-process.h"
#include "utf8-fnmatch.h"


#define MAX_CHUNK_LEN 16000 /* FIXME : what is the max length of a command 
			     * line ? */
#define UNKNOWN_TYPE "application/octet-stream"
#define SAME_FS (FALSE)
#define NO_BACKUP_FILES (TRUE)
#define NO_DOT_FILES (TRUE)
#define IGNORE_CASE (TRUE)


enum {
	START,
	DONE,
	PROGRESS,
	MESSAGE,
	STOPPABLE,
	LAST_SIGNAL
};

static GObjectClass *parent_class;
static guint fr_archive_signals[LAST_SIGNAL] = { 0 };

static void fr_archive_class_init (FRArchiveClass *class);
static void fr_archive_init       (FRArchive *archive);
static void fr_archive_finalize   (GObject *object);


GType
fr_archive_get_type (void)
{
        static GType type = 0;

        if (! type) {
                static const GTypeInfo type_info = {
			sizeof (FRArchiveClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_archive_class_init,
			NULL,
			NULL,
			sizeof (FRArchive),
			0,
			(GInstanceInitFunc) fr_archive_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "FRArchive",
					       &type_info,
					       0);
        }

        return type;
}


static void
fr_archive_class_init (FRArchiveClass *class)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (class);

        parent_class = g_type_class_peek_parent (class);

	fr_archive_signals[START] =
                g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FRArchiveClass, start),
			      NULL, NULL,
			      fr_marshal_VOID__INT,
			      G_TYPE_NONE, 
			      1, G_TYPE_INT);
	fr_archive_signals[DONE] =
                g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FRArchiveClass, done),
			      NULL, NULL,
			      fr_marshal_VOID__INT_POINTER,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      G_TYPE_POINTER);
	fr_archive_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FRArchiveClass, progress),
			      NULL, NULL,
			      fr_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	fr_archive_signals[MESSAGE] =
		g_signal_new ("message",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FRArchiveClass, message),
			      NULL, NULL,
			      fr_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
	fr_archive_signals[STOPPABLE] =
                g_signal_new ("stoppable",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FRArchiveClass, stoppable),
			      NULL, NULL,
			      fr_marshal_VOID__BOOL,
			      G_TYPE_NONE, 
			      1, G_TYPE_BOOLEAN);
	
	gobject_class->finalize = fr_archive_finalize;
	class->start = NULL;
	class->done = NULL;
	class->progress = NULL;
	class->message = NULL;
}


void
fr_archive_stoppable (FRArchive *archive,
		      gboolean   stoppable)
{
	g_signal_emit (G_OBJECT (archive), 
		       fr_archive_signals[STOPPABLE],
		       0,
		       stoppable);
}


static gboolean
archive_sticky_only_cb (FRProcess *process,
			FRArchive *archive)
{
	fr_archive_stoppable (archive, FALSE);
	return TRUE;
}


static void
fr_archive_init (FRArchive *archive)
{
	archive->filename = NULL;
	archive->command = NULL;
	archive->is_compressed_file = FALSE;

	archive->fake_load_func = NULL;
	archive->fake_load_data = NULL;

	archive->process = fr_process_new ();
	g_signal_connect (G_OBJECT (archive->process), 
			  "sticky_only",
			  G_CALLBACK (archive_sticky_only_cb),
			  archive);
}


FRArchive *
fr_archive_new (void)
{
	return FR_ARCHIVE (g_object_new (FR_TYPE_ARCHIVE, NULL));
}


static void
fr_archive_finalize (GObject *object)
{
	FRArchive *archive;

	g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_ARCHIVE (object));
  
	archive = FR_ARCHIVE (object);

	if (archive->filename != NULL)
		g_free (archive->filename);

	if (archive->command != NULL) 
		g_object_unref (archive->command);

	g_object_unref (archive->process);

	/* Chain up */

        if (G_OBJECT_CLASS (parent_class)->finalize)
                G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* filename must not be escaped. */
static gboolean
create_command_from_mime_type (FRArchive  *archive, 
			       const char *filename,
			       const char *mime_type)
{
	archive->is_compressed_file = FALSE;

	if (is_mime_type (mime_type, "application/x-tar")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_NONE);
	} else if (is_mime_type (mime_type, "application/x-compressed-tar")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_GZIP);
	} else if (is_mime_type (mime_type, "application/x-bzip-compressed-tar")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_BZIP2);
	} else if (is_mime_type (mime_type, "application/zip") ||
		   is_mime_type (mime_type, "application/x-zip")) {
		archive->command = fr_command_zip_new (archive->process, 
						       filename);
	} else if (is_mime_type (mime_type, "application/x-zoo")) {
		archive->command = fr_command_zoo_new (archive->process,
						       filename);
	} else if (is_mime_type (mime_type, "application/x-rar")) {
		archive->command = fr_command_rar_new (archive->process, 
						       filename);
	} else if (is_mime_type (mime_type, "application/x-arj")) {
		archive->command = fr_command_arj_new (archive->process, 
						       filename);
	} else if (is_mime_type (mime_type, "application/x-stuffit")) {
		archive->command = fr_command_unstuff_new (archive->process,
							   filename);
	} else if (is_mime_type (mime_type, "application/x-rpm")) {
		archive->command = fr_command_rpm_new (archive->process,
						       filename);
	} else if (is_mime_type (mime_type, "application/x-cd-image")) {
		archive->command = fr_command_iso_new (archive->process,
						       filename);
	} else 
		return FALSE;

	return TRUE;
}


/* filename must not be escaped. */
static const char *
get_mime_type_from_content (const char *filename) 
{
	const char *mime_type;
	
	mime_type = gnome_vfs_get_file_mime_type (filename, NULL, FALSE);
	
	if (strcmp (mime_type, UNKNOWN_TYPE) == 0)
		return NULL;
	
	return mime_type;
}


static gboolean
hexcmp (const guchar *first_bytes,
	const guchar *buffer,
	int           len)
{
	int i;
	
	for (i = 0; i < len; i++)
		if (first_bytes[i] != buffer[i])
			return FALSE;
	
	return TRUE;
}


/* filename must not be escaped. */
static const char *
get_mime_type_from_sniffer (const char *filename) 
{
	static struct {
		const char *mime_type;
		const char *first_bytes;
		int         len;
	}            sniffer_data [] = { 
		{"application/zip",                   "\x50\x4B\x03\x04", 4},
		/* FIXME
		 *{"application/x-compressed-tar",      "\x1F\x8B\x08\x08", 4},
		 *{"application/x-bzip-compressed-tar", "\x42\x5A\x68\x39", 4},
		 */
		{ NULL, NULL, 0 } 
	};
	FILE        *file;
	char         buffer[5];
	int          n, i;
	
	file = fopen (filename, "rb");
	
	if (file == NULL) 
                return NULL;
	
	n = fread (buffer, sizeof (char), sizeof (buffer) - 1, file);
	buffer[n] = 0;
	
	fclose (file);
	
	for (i = 0; sniffer_data[i].mime_type != NULL; i++) {
		const char *first_bytes = sniffer_data[i].first_bytes;
		int          len        = sniffer_data[i].len;
		
		if (hexcmp (first_bytes, buffer, len)) 
			return sniffer_data[i].mime_type;
	}
	
	return NULL;
}

	
/* filename must not be escaped. */
static gboolean
create_command_from_filename (FRArchive  *archive, 
			      const char *filename,
			      gboolean    loading)
{
	archive->is_compressed_file = FALSE;

	if (file_extension_is (filename, ".tar.gz")
	    || file_extension_is (filename, ".tgz")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_GZIP);
	} else if (file_extension_is (filename, ".tar.bz2")
		   || file_extension_is (filename, ".tbz2")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_BZIP2);
	} else if (file_extension_is (filename, ".tar.bz")
		   || file_extension_is (filename, ".tbz")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_BZIP);
	} else if (file_extension_is (filename, ".tar.Z")
		   || file_extension_is (filename, ".taz")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_COMPRESS);
	} else if (file_extension_is (filename, ".tar.lzo")
		   || file_extension_is (filename, ".tzo")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_LZOP);
	} else if (file_extension_is (filename, ".tar")) {
		archive->command = fr_command_tar_new (archive->process, 
						       filename, 
						       FR_COMPRESS_PROGRAM_NONE);
	} else if (file_extension_is (filename, ".zip")
		   || file_extension_is (filename, ".ear")
		   || file_extension_is (filename, ".jar")
		   || file_extension_is (filename, ".war")) {
		archive->command = fr_command_zip_new (archive->process, 
						       filename);
	} else if (file_extension_is (filename, ".zoo")) {
		archive->command = fr_command_zoo_new (archive->process,
						       filename);
	} else if (file_extension_is (filename, ".lzh")) {
		archive->command = fr_command_lha_new (archive->process, 
						       filename);
	} else if (file_extension_is (filename, ".rar")) {
		archive->command = fr_command_rar_new (archive->process, 
						       filename);
	} else if (file_extension_is (filename, ".arj")) {
		archive->command = fr_command_arj_new (archive->process, 
						       filename);
	} else if (loading) {
		if (file_extension_is (filename, ".gz")
		    || file_extension_is (filename, ".z")
		    || file_extension_is (filename, ".Z")) {
			archive->command = fr_command_cfile_new (archive->process, filename, FR_COMPRESS_PROGRAM_GZIP);
			archive->is_compressed_file = TRUE;
		} else if (file_extension_is (filename, ".bz")) {
			archive->command = fr_command_cfile_new (archive->process, filename, FR_COMPRESS_PROGRAM_BZIP);
			archive->is_compressed_file = TRUE;
		} else if (file_extension_is (filename, ".bz2")) {
			archive->command = fr_command_cfile_new (archive->process, filename, FR_COMPRESS_PROGRAM_BZIP2);
			archive->is_compressed_file = TRUE;
		} else if (file_extension_is (filename, ".lzo")) {
			archive->command = fr_command_cfile_new (archive->process, filename, FR_COMPRESS_PROGRAM_LZOP);
			archive->is_compressed_file = TRUE;
		} else if (file_extension_is (filename, ".bin")
			   || file_extension_is (filename, ".sit")) {
			archive->command = fr_command_unstuff_new (archive->process,
								   filename);
		} else if (file_extension_is (filename, ".rpm")) {
			archive->command = fr_command_rpm_new (archive->process,
							       filename);
		} else if (file_extension_is (filename, ".iso")) {
			archive->command = fr_command_iso_new (archive->process,
							       filename);
		} else
			return FALSE;
	} else
		return FALSE;
	
	return TRUE;
}


static void
action_started (FRCommand *command, 
		FRAction   action,
		FRArchive *archive)
{
	g_signal_emit (G_OBJECT (archive), 
		       fr_archive_signals[START],
		       0,
		       action);
}


static void
action_performed (FRCommand   *command, 
		  FRAction     action,
		  FRProcError *error,
		  FRArchive   *archive)
{
#ifdef DEBUG
	char *s_action = NULL;

	switch (action) {
	case FR_ACTION_LIST:
		s_action = "List";
		break;
	case FR_ACTION_ADD:
		s_action = "Add";
		break;
	case FR_ACTION_DELETE:
		s_action = "Delete";
		break;
	case FR_ACTION_EXTRACT:
		s_action = "Extract";
		break;
	case FR_ACTION_TEST:
		s_action = "Test";
		break;
	case FR_ACTION_GET_LIST:
		s_action = "Get list";
		break;
	}
	g_print ("%s [DONE]\n", s_action);
#endif

	g_signal_emit (G_OBJECT (archive), 
		       fr_archive_signals[DONE],
		       0,
		       action,
		       error);
}


static gboolean
archive_progress_cb (FRCommand  *command,
		     double      fraction,
		     FRArchive  *archive)
{
	g_signal_emit (G_OBJECT (archive), 
		       fr_archive_signals[PROGRESS], 
		       0,
		       fraction);
	return TRUE;
}


static gboolean
archive_message_cb  (FRCommand  *command,
		     const char *msg,
		     FRArchive  *archive)		     
{
	g_signal_emit (G_OBJECT (archive), 
		       fr_archive_signals[MESSAGE], 
		       0,
		       msg);
	return TRUE;
}


/* filename must not be escaped. */
gboolean
fr_archive_new_file (FRArchive  *archive, 
		     const char *filename)
{
	FRCommand *tmp_command;

	if (filename == NULL)
		return FALSE;

	tmp_command = archive->command;
	if (! create_command_from_filename (archive, filename, FALSE)) {
		archive->command = tmp_command;
		return FALSE;
	}

	if (tmp_command != NULL) 
		g_object_unref (G_OBJECT (tmp_command));

	archive->read_only = FALSE;

	if (archive->filename != NULL)
		g_free (archive->filename);	
	archive->filename = g_strdup (filename);

	g_signal_connect (G_OBJECT (archive->command), 
			  "start",
			  G_CALLBACK (action_started),
			  archive);
	g_signal_connect (G_OBJECT (archive->command), 
			  "done",
			  G_CALLBACK (action_performed),
			  archive);
	g_signal_connect (G_OBJECT (archive->command), 
			  "progress",
			  G_CALLBACK (archive_progress_cb),
			  archive);
	g_signal_connect (G_OBJECT (archive->command), 
			  "message",
			  G_CALLBACK (archive_message_cb),
			  archive);

	return TRUE;
}


void
fr_archive_set_fake_load_func (FRArchive    *archive,
			       FakeLoadFunc  func,
			       gpointer      data)
{
	archive->fake_load_func = func;
	archive->fake_load_data = data;
}


gboolean
fr_archive_fake_load (FRArchive *archive)
{
	if (archive->fake_load_func != NULL)
		return (*archive->fake_load_func) (archive, archive->fake_load_data);
	else
		return FALSE;
}


/* filename must not be escaped. */
gboolean
fr_archive_load (FRArchive   *archive, 
		 const char  *filename,
		 GError     **gerror)
{
	FRCommand  *tmp_command;
	const char *mime_type;

	g_return_val_if_fail (archive != NULL, FALSE);

	if (access (filename, F_OK) != 0) { /* file must exists. */
		if (gerror != NULL) 
			*gerror = g_error_new (fr_error_quark (),
					       0,
					       _("The file does not exist."));
		return FALSE;
	}

	archive->read_only = access (filename, W_OK) != 0;

	if (archive->filename != NULL)
		g_free (archive->filename);
	archive->filename = g_strdup (filename);

	tmp_command = archive->command;

	/* prefer mime-magic */

	mime_type = get_mime_type_from_sniffer (filename);
	if (mime_type == NULL)
		mime_type = get_mime_type_from_content (filename);
	
	if ((mime_type == NULL)
	    || ! create_command_from_mime_type (archive, filename, mime_type))
		if (! create_command_from_filename (archive, filename, TRUE)) {
			archive->command = tmp_command;
			if (gerror != NULL) 
				*gerror = g_error_new (fr_error_quark (),
						       0,
						       _("Archive type not supported."));

                        return FALSE;
		}
	
	if (tmp_command != NULL) 
		g_object_unref (G_OBJECT (tmp_command));

	g_signal_connect (G_OBJECT (archive->command), 
			  "start",
			  G_CALLBACK (action_started),
			  archive);
	g_signal_connect (G_OBJECT (archive->command), 
			  "done",
			  G_CALLBACK (action_performed),
			  archive);
	g_signal_connect (G_OBJECT (archive->command), 
			  "progress",
			  G_CALLBACK (archive_progress_cb),
			  archive);
	g_signal_connect (G_OBJECT (archive->command), 
			  "message",
			  G_CALLBACK (archive_message_cb),
			  archive);

	fr_archive_stoppable (archive, TRUE);
	archive->command->fake_load = fr_archive_fake_load (archive);

	fr_process_clear (archive->process);
	fr_command_list (archive->command);
	fr_process_start (archive->process);

	return TRUE;
}


void
fr_archive_reload (FRArchive *archive)
{
	g_return_if_fail (archive != NULL);
	g_return_if_fail (archive->filename != NULL);

	fr_archive_stoppable (archive, TRUE);
	archive->command->fake_load = fr_archive_fake_load (archive);

	fr_process_clear (archive->process);
	fr_command_list (archive->command);
	fr_process_start (archive->process);
}


/* filename must not be escaped. */
void
fr_archive_rename (FRArchive  *archive,
		   const char *filename)
{
	g_return_if_fail (archive != NULL);

	if (archive->is_compressed_file) 
		/* If the archive is a compressed file we have to reload it,
		 * because in this case the 'content' of the archive changes 
		 * too. */
		fr_archive_load (archive, filename, NULL);

	else {
		if (archive->filename != NULL)
			g_free (archive->filename);
		archive->filename = g_strdup (filename);
		
		fr_command_set_filename (archive->command, filename);
	}
}


/* -- add -- */


static char *
create_tmp_base_dir (const char *base_dir,
		     const char *dest_path) 
{
	char *dest_dir;
	char *temp_dir;
	char *tmp;
	char *parent_dir, *dir;

	if ((dest_path == NULL) 
	    || (*dest_path == '\0') 
	    || (strcmp (dest_path, "/") == 0)) 
		return g_strdup (base_dir);

	dest_dir = g_strdup (dest_path);
	if (dest_dir[strlen (dest_dir) - 1] == G_DIR_SEPARATOR)
		dest_dir[strlen (dest_dir) - 1] = 0;

#ifdef DEBUG
	g_print ("base_dir: %s\n", base_dir);
	g_print ("dest_dir: %s\n", dest_dir);
#endif

	temp_dir = get_temp_work_dir ();
	tmp = remove_level_from_path (dest_dir);
	parent_dir =  g_build_filename (temp_dir, tmp, NULL);
	g_free (tmp);

	ensure_dir_exists (parent_dir, 0700);

#ifdef DEBUG
	g_print ("mkdir %s\n", parent_dir);
#endif

	g_free (parent_dir);

	dir = g_build_filename (temp_dir, "/", dest_dir, NULL);
	symlink (base_dir, dir);

#ifdef DEBUG
	g_print ("symlink %s --> %s\n", dir, base_dir);
#endif

	g_free (dir);
	g_free (dest_dir);

	return temp_dir;
}


/* Note: all paths unescaped. */
static FileData *
find_file_in_archive (FRArchive *archive, 
		      char      *path)
{
	GList *scan;

	g_return_val_if_fail (path != NULL, NULL);

	for (scan = archive->command->file_list; scan; scan = scan->next) {
		FileData *fdata = scan->data;
		if (strcmp (path, fdata->original_path) == 0)
			return fdata;
	}

	return NULL;
}


static void _archive_remove (FRArchive *archive, GList *file_list);


static GList *
escape_file_list (GList *file_list) 
{
	GList *e_file_list = NULL;
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		char *filename = scan->data;
		e_file_list = g_list_prepend (e_file_list, shell_escape (filename));
	}

	return e_file_list;
}


/* Note: all paths unescaped. */
static GList *
newer_files_only (FRArchive  *archive,
		  GList      *file_list,
		  const char *base_dir)
{
	GList *newer_files = NULL;
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		char     *filename = scan->data;
		char     *fullpath;
		FileData *fdata;

		fdata = find_file_in_archive (archive, filename);

		if (fdata == NULL) {
			newer_files = g_list_prepend (newer_files, scan->data);
			continue;
		}

		fullpath = g_strconcat (base_dir, "/", filename, NULL);

		if (path_is_file (fullpath)
		    && (fdata->modified >= get_file_mtime (fullpath))) {
			g_free (fullpath);
			continue;
		}

		newer_files = g_list_prepend (newer_files, scan->data);
		g_free (fullpath);
	}
	
	return newer_files;
}


/* Note: all paths unescaped. */
void
fr_archive_add (FRArchive     *archive, 
		GList         *file_list, 
		const char    *base_dir,
		const char    *dest_dir,
		gboolean       update,
		const char    *password,
		FRCompression  compression)
{
	GList    *new_file_list = NULL;
	gboolean  base_dir_created = FALSE;
	gboolean  new_file_list_created = FALSE;
	GList    *e_file_list;
	GList    *scan;
	char     *tmp_base_dir = NULL;

	if (file_list == NULL)
		return;

	if (archive->read_only)
		return;

	tmp_base_dir = g_strdup (base_dir);

	if ((dest_dir != NULL) && (*dest_dir != '\0') && (strcmp (dest_dir, "/") != 0)) {
		const char *rel_dest_dir = dest_dir;

		tmp_base_dir = create_tmp_base_dir (base_dir, dest_dir);
		base_dir_created = TRUE;

		if (dest_dir[0] == G_DIR_SEPARATOR)
			rel_dest_dir = dest_dir + 1;

		new_file_list_created = TRUE;
		new_file_list = NULL;
		for (scan = file_list; scan != NULL; scan = scan->next) {
			char *filename = scan->data;
			new_file_list = g_list_prepend (new_file_list, g_build_filename (rel_dest_dir, filename, NULL));
		}
	} else
		new_file_list = file_list;

	fr_archive_stoppable (archive, FALSE);

	/* if the command cannot update,  get the list of files that are 
	 * newer than the ones in the archive. */

	if (update && ! archive->command->propAddCanUpdate) {
		GList *tmp_file_list = new_file_list;
		new_file_list = newer_files_only (archive, tmp_file_list, tmp_base_dir);
		if (new_file_list_created)
			path_list_free (tmp_file_list);
		new_file_list_created = TRUE;
	} 

	if (new_file_list == NULL) {
#ifdef DEBUG
		g_print ("nothing to update.\n");
#endif

		if (base_dir_created) 
			rmdir_recursive (tmp_base_dir);
		g_free (tmp_base_dir);

		archive->process->error.type = FR_PROC_ERROR_NONE;
		g_signal_emit_by_name (G_OBJECT (archive->process), 
				       "done",
				       FR_ACTION_ADD,
				       &archive->process->error);
		return;
	}

	fr_command_uncompress (archive->command);

	/* when files are already present in a tar archive and are added
	 * again, they are not replaced, so we have to delete them first. */

	/* if we are adding (== ! update) and 'add' cannot replace or
	 * if we are updating and 'add' cannot update, 
	 * delete the files first. */

	if ((! update && ! archive->command->propAddCanReplace)
	    || (update && ! archive->command->propAddCanUpdate)) {
		GList *del_list = NULL;

		for (scan = new_file_list; scan != NULL; scan = scan->next) {
			char *filename = scan->data;
			if (find_file_in_archive (archive, filename)) 
				del_list = g_list_prepend (del_list, filename);
		}

		/* delete */

		if (del_list != NULL) {
			_archive_remove (archive, del_list);
			fr_process_set_ignore_error (archive->process, TRUE);
			g_list_free (del_list);
		}
	}

	/* add now. */

	e_file_list = escape_file_list (new_file_list);
	fr_command_set_n_files (archive->command, g_list_length (e_file_list));

	for (scan = e_file_list; scan != NULL; ) {
		GList *prev = scan->prev;
		GList *chunk_list;
		int    l;
		
		chunk_list = scan;
		l = 0;
		while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
			if (l == 0)
				l = strlen (scan->data);
			prev = scan;
			scan = scan->next;
			if (scan != NULL)
				l += strlen (scan->data);
		}
		
		prev->next = NULL;
		fr_command_add (archive->command, 
				chunk_list, 
				tmp_base_dir, 
				update,
				password,
				compression);
		prev->next = scan;
	}

	path_list_free (e_file_list);
	if (new_file_list_created)
		g_list_free (new_file_list);

	fr_command_recompress (archive->command, compression);

	if (base_dir_created) { /* remove the temp dir */
		fr_process_begin_command (archive->process, "rm");
		fr_process_set_working_dir (archive->process, g_get_tmp_dir());
		fr_process_set_sticky (archive->process, TRUE);
		fr_process_add_arg (archive->process, "-rf");
		fr_process_add_arg (archive->process, tmp_base_dir);
		fr_process_end_command (archive->process);

	}
	g_free (tmp_base_dir);
}


static void
file_list_remove_from_pattern (GList      **list, 
			       const char  *pattern)
{
	char  **patterns;
	GList  *scan;
	
	if (pattern == NULL)
		return;
	
	patterns = search_util_get_patterns (pattern);
	
	for (scan = *list; scan;) {
		char *path = scan->data;
		char *utf8_name;

		utf8_name = g_filename_to_utf8 (file_name_from_path (path), 
						-1, NULL, NULL, NULL);

		if (strcmp ("lt-gthumb", utf8_name) == 0)
			g_print ("%s <--> %s\n", pattern, utf8_name);

		if (match_patterns (patterns, utf8_name, FNM_CASEFOLD)) {
			*list = g_list_remove_link (*list, scan);
			g_free (scan->data);
			g_list_free (scan);
			scan = *list;
			if (strcmp ("lt-gthumb", utf8_name) == 0)
				g_print ("Y\n");
		} else {
			scan = scan->next;
			if (strcmp ("lt-gthumb", utf8_name) == 0)
				g_print ("N\n");
		}

		g_free (utf8_name);
	}
	
	g_strfreev (patterns);
}


/* -- add with wildcard -- */


typedef struct {
	FRArchive     *archive;
        char          *exclude_files;
	char          *base_dir;
	char          *dest_dir;
	gboolean       update;
	char          *password;
	FRCompression  compression;
	DoneFunc       done_func;
	gpointer       done_data;
} AddWithWildcardData;


static void
add_with_wildcard__step2 (GList *file_list, gpointer data)
{
	AddWithWildcardData *aww_data = data;

	file_list_remove_from_pattern (&file_list, aww_data->exclude_files);

	fr_archive_add (aww_data->archive,
			file_list,
			aww_data->base_dir,
			aww_data->dest_dir,
			aww_data->update,
			aww_data->password,
			aww_data->compression);
	path_list_free (file_list);

	if (aww_data->done_func) 
		aww_data->done_func (aww_data->done_data);

	g_free (aww_data->base_dir);
	g_free (aww_data->password);
	g_free (aww_data->exclude_files);
}


/* Note: all paths unescaped. */
VisitDirHandle *
fr_archive_add_with_wildcard (FRArchive     *archive, 
			      const char    *include_files,
			      const char    *exclude_files,
			      const char    *base_dir,
			      const char    *dest_dir,
			      gboolean       update,
			      gboolean       recursive,
			      gboolean       follow_links,
			      /*gboolean       same_fs,
			      gboolean       no_backup_files,
			      gboolean       no_dot_files,
			      gboolean       ignore_case,
			      */
			      const char    *password,
			      FRCompression  compression,
			      DoneFunc       done_func,
			      gpointer       done_data)
{
	AddWithWildcardData *aww_data;

	if (archive->read_only)
		return NULL;

	fr_archive_stoppable (archive, TRUE);

	aww_data = g_new0 (AddWithWildcardData, 1);
	aww_data->archive = archive;
	aww_data->base_dir = g_strdup (base_dir);
	aww_data->dest_dir = g_strdup (dest_dir);
	aww_data->update = update;
	aww_data->password = g_strdup (password);
	aww_data->compression = compression;
	aww_data->exclude_files = g_strdup (exclude_files);
	aww_data->done_func = done_func;
	aww_data->done_data = done_data;

	return get_wildcard_file_list_async (base_dir, 
					     include_files, 
					     recursive, 
					     follow_links, 
					     SAME_FS,
					     NO_BACKUP_FILES,
					     NO_DOT_FILES,
					     IGNORE_CASE,
					     add_with_wildcard__step2, 
					     aww_data);
}


/* -- fr_archive_add_directory -- */


typedef struct {
	FRArchive     *archive;
	char          *directory;
	char          *base_dir;
	char          *dest_dir;
	gboolean       update;
	char          *password;
	FRCompression  compression;
	DoneFunc       done_func;
	gpointer       done_data;
} AddDirectoryData;


static void
add_directory__step2 (GList *file_list, gpointer data)
{
	AddDirectoryData *ad_data = data;

	if (file_list != NULL) {
		fr_archive_add (ad_data->archive,
				file_list,
				ad_data->base_dir,
				ad_data->dest_dir,
				ad_data->update,
				ad_data->password,
				ad_data->compression);
		path_list_free (file_list);
	}

	if (ad_data->done_func) 
		ad_data->done_func (ad_data->done_data);

	/**/

	g_free (ad_data->directory);
	g_free (ad_data->base_dir);
	g_free (ad_data->password);
	g_free (ad_data);
}


/* Note: all paths unescaped. */
VisitDirHandle *
fr_archive_add_directory (FRArchive     *archive, 
			  const char    *directory,
			  const char    *base_dir,
			  const char    *dest_dir,
			  gboolean       update,
			  const char    *password,
			  FRCompression  compression,
			  DoneFunc       done_func,
			  gpointer       done_data)

{
	AddDirectoryData *ad_data;

	if (archive->read_only)
		return NULL;

	fr_archive_stoppable (archive, TRUE);

	ad_data = g_new0 (AddDirectoryData, 1);
	ad_data->archive = archive;
	ad_data->directory = g_strdup (directory);
	ad_data->base_dir = g_strdup (base_dir);
	ad_data->dest_dir = g_strdup (dest_dir);
	ad_data->update = update;
	ad_data->password = g_strdup (password);
	ad_data->compression = compression;
	ad_data->done_func = done_func;
	ad_data->done_data = done_data;

	return get_directory_file_list_async (directory, 
					      base_dir, 
					      add_directory__step2, 
					      ad_data);
}


/* -- remove -- */


/* Note: all paths unescaped. */
static void
_archive_remove (FRArchive *archive,
		 GList     *file_list)
{
	gboolean  file_list_created = FALSE;
	GList    *e_file_list;
	GList    *scan;

	/* file_list == NULL means delete all files in archive. */

	if (file_list == NULL) {
		for (scan = archive->command->file_list; scan != NULL; scan = scan->next) {
			FileData *fdata = (FileData*) scan->data;
			file_list = g_list_prepend (file_list, fdata->original_path);
		}
		file_list_created = TRUE;
	}

	e_file_list = escape_file_list (file_list);
	fr_command_set_n_files (archive->command, g_list_length (e_file_list));

	for (scan = e_file_list; scan != NULL; ) {
		GList *prev = scan->prev;
		GList *chunk_list;
		int    l;
		
		chunk_list = scan;
		l = 0;
		while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
			if (l == 0)
				l = strlen (scan->data);
			prev = scan;
			scan = scan->next;
			if (scan != NULL)
				l += strlen (scan->data);
		}
		
		prev->next = NULL;
		fr_command_delete (archive->command, chunk_list);
		prev->next = scan;
	}

	if (file_list_created)
		g_list_free (file_list);
	path_list_free (e_file_list);
}


/* Note: all paths unescaped. */
void
fr_archive_remove (FRArchive     *archive,
		   GList         *file_list,
		   FRCompression  compression)
{
	g_return_if_fail (archive != NULL);

	if (archive->read_only)
		return;

	fr_archive_stoppable (archive, FALSE);

	fr_command_uncompress (archive->command);
	_archive_remove (archive, file_list);
	fr_command_recompress (archive->command, compression);
}


/* -- extract -- */


/* Note: all paths escaped, source_dir and dest_dir escaped. */
static void
move_files_to_dir (FRArchive  *archive,
		   GList      *file_list,
		   const char *source_dir,
		   const char *dest_dir)
{
	GList *scan;

	fr_process_begin_command (archive->process, "mv");
	fr_process_add_arg (archive->process, "-f");
	for (scan = file_list; scan; scan = scan->next) {
		char  path[4096]; /* FIXME : 4096 ? */
		char *filename = scan->data;
		
		if (filename[0] == '/')
			sprintf (path, "%s%s", source_dir, filename);
		else
			sprintf (path, "%s/%s", source_dir, filename);
		
		fr_process_add_arg (archive->process, path);
	}
	fr_process_add_arg (archive->process, dest_dir);
	fr_process_end_command (archive->process);
}


/* Note: all paths escaped, temp_dir and dest_dir unescaped. */
static void
move_files_in_chunks (FRArchive  *archive,
		      GList      *file_list,
		      const char *temp_dir,
		      const char *dest_dir)
{
	GList *scan;
	int    e_temp_dir_l;
	char  *e_temp_dir;
	char  *e_dest_dir;

	e_temp_dir = shell_escape (temp_dir);
	e_dest_dir = shell_escape (dest_dir);
	e_temp_dir_l = strlen (e_temp_dir);

	for (scan = file_list; scan != NULL; ) {
		GList *prev = scan->prev;
		GList *chunk_list;
		int    l;

		chunk_list = scan;
		l = 0;
		while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
			if (l == 0)
				l = e_temp_dir_l + 1 + strlen (scan->data);
			prev = scan;
			scan = scan->next;
			if (scan != NULL)
				l += e_temp_dir_l + 1 + strlen (scan->data);
		}

		prev->next = NULL;
		move_files_to_dir (archive,
				   chunk_list,
				   e_temp_dir, 
				   e_dest_dir);
		prev->next = scan;
	}	
	
	g_free (e_temp_dir);
	g_free (e_dest_dir);
}


/* Note: all paths escaped, dest_dir unescaped. */
static void
extract_in_chunks (FRCommand  *command,
		   GList      *file_list,
		   const char *dest_dir,
		   gboolean    overwrite,
		   gboolean    skip_older,
		   gboolean    junk_paths,
		   const char *password)
{
	GList *scan;

	fr_command_set_n_files (command, g_list_length (file_list));

	if (file_list == NULL) {
		fr_command_extract (command,
				    file_list,
				    dest_dir,
				    overwrite,
				    skip_older,
				    junk_paths,
				    password);
		return;
	}

	for (scan = file_list; scan != NULL; ) {
		GList *prev = scan->prev;
		GList *chunk_list;
		int    l;

		chunk_list = scan;
		l = 0;
		while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
			if (l == 0)
				l = strlen (scan->data);
			prev = scan;
			scan = scan->next;
			if (scan != NULL)
				l += strlen (scan->data);
		}

		prev->next = NULL;
		fr_command_extract (command,
				    chunk_list,
				    dest_dir,
				    overwrite,
				    skip_older,
				    junk_paths,
				    password);
		prev->next = scan;
	}
}


static char*
compute_base_path (const char *e_base_dir,
		   const char *path,
		   gboolean    junk_paths,
		   gboolean    can_junk_paths)
{
	int         e_base_dir_len = strlen (e_base_dir);
	int         path_len = strlen (path);
	const char *base_path;
	char       *name_end;
	char       *new_path;

	if (junk_paths) {
		if (can_junk_paths)
			new_path = g_strdup (file_name_from_path (path));
		else
			new_path = g_strdup (path);
#ifdef DEBUG
		g_print ("%s, %s --> %s\n", e_base_dir, path, new_path);
#endif
		return new_path;
	}

	if (path_len <= e_base_dir_len)
		return NULL;
	base_path = path + e_base_dir_len;
	if (path[0] != '/')
		base_path -= 1;
	name_end = strchr (base_path, '/');

	if (name_end == NULL)
		new_path = g_strdup (path);
	else {
		int name_len = name_end - path;
		new_path = g_strndup (path, name_len);
	}

#ifdef DEBUG
	g_print ("%s, %s --> %s\n", e_base_dir, path, new_path);
#endif

	return new_path;
}


static GList*
compute_list_base_path (const char *base_dir,
			GList      *e_filtered,
			gboolean    junk_paths,
			gboolean    can_junk_paths)
{
	char  *e_base_dir = shell_escape (base_dir);
	GList *scan;
	GList *list = NULL, *list_unique = NULL;
	GList *last_inserted;

	if (e_filtered == NULL)
		return NULL;

	for (scan = e_filtered; scan; scan = scan->next) {
		const char *path = scan->data;
		char       *new_path;
		new_path = compute_base_path (e_base_dir, path, junk_paths, can_junk_paths);
		if (new_path != NULL)
			list = g_list_prepend (list, new_path);
	}

	/* The above operation can create duplicates, we remove them here. */
	list = g_list_sort (list, (GCompareFunc)strcmp);
	
	last_inserted = NULL;
	for (scan = list; scan; scan = scan->next) {
		const char *path = scan->data;

		if (last_inserted != NULL) {
			const char *last_path = (const char*)last_inserted->data;
			if (strcmp (last_path, path) == 0) {
				g_free (scan->data);
				continue;
			}
		}

		last_inserted = scan;
		list_unique = g_list_prepend (list_unique, scan->data);
	}

	g_list_free (list);
	g_free (e_base_dir);

	return list_unique;
}


/* Note : All paths unescaped.  
 * Note2: Do not escape dest_dir it will escaped in fr_command_extract if 
 *        needed. */
void
fr_archive_extract (FRArchive  *archive,
		    GList      *file_list,
		    const char *dest_dir,
		    const char *base_dir,
		    gboolean    skip_older,
		    gboolean    overwrite,
		    gboolean    junk_paths,
		    const char *password)
{
	GList    *filtered, *e_filtered;
	GList    *scan;
	gboolean  extract_all;
	gboolean  use_base_dir;
	gboolean  move_to_dest_dir;
	gboolean  file_list_created = FALSE;

	g_return_if_fail (archive != NULL);

	fr_archive_stoppable (archive, TRUE);

	/* if a command supports all the requested options use 
	 * fr_command_extract directly. */

	extract_all = (file_list == NULL);
	if (extract_all && ! archive->command->propCanExtractAll) {
		GList *scan;

		scan = archive->command->file_list;
		for (; scan; scan = scan->next) {
			FileData *fdata = scan->data;
			file_list = g_list_prepend (file_list, g_strdup (fdata->original_path));
		}

		file_list_created = TRUE;
	}

	use_base_dir = ! ((base_dir == NULL) 
			  || (strcmp (base_dir, "") == 0)
			  || (strcmp (base_dir, "/") == 0));
	
	if (! use_base_dir
	    && ! (! overwrite && ! archive->command->propExtractCanAvoidOverwrite)
	    && ! (skip_older && ! archive->command->propExtractCanSkipOlder)
	    && ! (junk_paths && ! archive->command->propExtractCanJunkPaths)) {
		GList *e_file_list;

		e_file_list = escape_file_list (file_list);
		extract_in_chunks (archive->command,
				   e_file_list,
				   dest_dir,
				   overwrite,
				   skip_older,
				   junk_paths,
				   password);
		path_list_free (e_file_list);

		if (file_list_created) 
			path_list_free (file_list);

		return;
	}

	/* .. else we have to implement the unsupported options. */
	
	move_to_dest_dir = (use_base_dir
			    || ((junk_paths 
				 && ! archive->command->propExtractCanJunkPaths)));

	if (extract_all && ! file_list_created) {
		GList *scan;

		scan = archive->command->file_list;
		for (; scan; scan = scan->next) {
			FileData *fdata = scan->data;
			file_list = g_list_prepend (file_list, g_strdup (fdata->original_path));
		}

		file_list_created = TRUE;
	}

	filtered = NULL;
	for (scan = file_list; scan; scan = scan->next) {
		FileData   *fdata;
		char       *arch_filename = scan->data;
		char        dest_filename[4096];
		const char *filename;

		fdata = find_file_in_archive (archive, arch_filename);

		if (fdata == NULL)
			continue;

		/* get the destination file path. */

		if (! junk_paths)
			filename = arch_filename;
		else
			filename = file_name_from_path (arch_filename);

		if ((dest_dir[strlen (dest_dir) - 1] == '/')
		    || (filename[0] == '/'))
			sprintf (dest_filename, "%s%s", dest_dir, filename);
		else
			sprintf (dest_filename, "%s/%s", dest_dir, filename);
		

#ifdef DEBUG		
		g_print ("-> %s\n", dest_filename);
#endif

		/**/
		
		if (! archive->command->propExtractCanSkipOlder
		    && skip_older 
		    && path_is_file (dest_filename)
		    && (fdata->modified < get_file_mtime (dest_filename)))
			continue;

		if (! archive->command->propExtractCanAvoidOverwrite
		    && ! overwrite 
		    && path_is_file (dest_filename))
			continue;

		filtered = g_list_prepend (filtered, fdata->original_path);
	}

	if (filtered == NULL) {
		/* all files got filtered, do nothing. */

#ifdef DEBUG
		g_print ("All files got filtered, do nothing.\n");
#endif

		if (extract_all) 
			path_list_free (file_list);
		return;
	} 

	e_filtered = escape_file_list (filtered);	

	if (move_to_dest_dir) {
		char *temp_dir;
		char *e_temp_dir;

		temp_dir = get_temp_work_dir ();
		extract_in_chunks (archive->command,
				   e_filtered,
				   temp_dir,
				   overwrite,
				   skip_older,
				   junk_paths,
				   password);

		if (use_base_dir) {
			GList *tmp = compute_list_base_path (base_dir, e_filtered, junk_paths, archive->command->propExtractCanJunkPaths);
			path_list_free (e_filtered);
			e_filtered = tmp;
		}

		move_files_in_chunks (archive, 
				      e_filtered, 
				      temp_dir, 
				      dest_dir);

		/* remove the temp dir. */

		e_temp_dir = shell_escape (temp_dir);
		fr_process_begin_command (archive->process, "rm");
		fr_process_add_arg (archive->process, "-rf");
		fr_process_add_arg (archive->process, e_temp_dir);
		fr_process_end_command (archive->process);
		g_free (e_temp_dir);

		g_free (temp_dir);
	} else
		extract_in_chunks (archive->command,
				   e_filtered,
				   dest_dir,
				   overwrite,
				   skip_older,
				   junk_paths,
				   password);

	path_list_free (e_filtered);
	if (filtered != NULL)
		g_list_free (filtered);

	if (file_list_created) 
		/* the list has been created in this function. */
		path_list_free (file_list);
}


void
fr_archive_test (FRArchive  *archive,
		 const char *password)
{
	fr_archive_stoppable (archive, TRUE);

	fr_process_clear (archive->process);
	fr_command_set_n_files (archive->command, 0);
	fr_command_test (archive->command, password);
	fr_process_start (archive->process);
}


/*
 * Remember to keep the ext array in alphanumeric order and to scan the array
 * in reverse order, this is because the file 'foo.tar.gz' must return the 
 * '.tar.gz' and not the '.gz' extension.
 */
G_CONST_RETURN char *
fr_archive_utils__get_file_name_ext (const char *filename)
{
	static char * ext[] = {
		".arj",
		".bin",
		".bz", 
		".bz2", 
		".ear",
		".gz", 
		".iso",
		".jar",
		".lzh",
		".lzo",
		".rar",
		".rpm",
		".sit",
		".tar", 
		".tar.bz", 
		".tar.bz2", 
		".tar.gz", 
		".tar.lzo",
		".tar.Z", 
		".taz", 
		".tbz",
		".tbz2",
		".tgz", 
		".tzo",
		".war",
		".z", 
		".zip",
		".zoo",
		".Z" 
	};
	int n = sizeof (ext) / sizeof (char*);
	int i;

	for (i = n - 1; i >= 0; i--) 
		if (file_extension_is (filename, ext[i]))
			return ext[i];

	return NULL;
}


gboolean
fr_archive_utils__file_is_archive (const char *filename)
{
	const char *mime_type;

	mime_type = get_mime_type_from_content (filename);
	
	if (mime_type == NULL)
		return FALSE;
	
	mime_type = get_mime_type_from_sniffer (filename);
	
	if (mime_type != NULL)
		return TRUE;
	
	return fr_archive_utils__get_file_name_ext (filename) != NULL;
}
