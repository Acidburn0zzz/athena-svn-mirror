/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-storage-fs.c: Sample file-system based Storage implementation
 *
 * This is just a sample file-system based Storage implementation.
 * it is only used for debugging purposes
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 */
#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <storage-modules/bonobo-storage-fs.h>
#include <storage-modules/bonobo-stream-fs.h>
#include <bonobo/bonobo-storage-plugin.h>

static BonoboStorageClass *bonobo_storage_fs_parent_class;

static void
bonobo_storage_fs_destroy (GtkObject *object)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (object);

	g_free (storage_fs->path);
}

static Bonobo_StorageInfo*
fs_get_info (BonoboStorage *storage,
	     const CORBA_char *path,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	Bonobo_StorageInfo *si;
	struct stat st;
	char *full = NULL;
	
	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}

	full = g_concat_dir_and_file (storage_fs->path, path);

	if (stat (full, &st) == -1)
		goto get_info_except;

	si = Bonobo_StorageInfo__alloc ();
	
	si->size = st.st_size;
	si->name = CORBA_string_dup (path);

	if (S_ISDIR (st.st_mode)) {
		si->type = Bonobo_STORAGE_TYPE_DIRECTORY;
		si->content_type = CORBA_string_dup ("x-directory/normal");
	} else {
		si->type = Bonobo_STORAGE_TYPE_REGULAR;
		si->content_type = 
			CORBA_string_dup ("application/octet-stream");
	}

	return si;

 get_info_except:

	if (full)
		g_free (full);
	
	if (errno == EACCES) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NoPermission, 
				     NULL);
	else if (errno == ENOENT) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotFound, 
				     NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_IOError, NULL);
	
	return CORBA_OBJECT_NIL;
}

static void
fs_set_info (BonoboStorage *storage,
	     const CORBA_char *path,
	     const Bonobo_StorageInfo *info,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Storage_NotSupported, 
			     NULL);
}

static BonoboStream *
fs_open_stream (BonoboStorage *storage, 
		const CORBA_char *path, 
		Bonobo_Storage_OpenMode mode, 
		CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	BonoboStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	stream = bonobo_stream_fs_open (full, mode, 0644, ev);
	g_free (full);

	return stream;
}

static BonoboStorage *
fs_open_storage (BonoboStorage *storage, const CORBA_char *path, 
		 Bonobo_Storage_OpenMode mode, CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	BonoboStorage *new_storage;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	new_storage = bonobo_storage_fs_open (full, mode, 0644, ev);
	g_free (full);

	return new_storage;
}


static void
fs_rename (BonoboStorage *storage, const CORBA_char *path, 
	   const CORBA_char *new_path, CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	char *full_old, *full_new;

	full_old = g_concat_dir_and_file (storage_fs->path, path);
	full_new = g_concat_dir_and_file (storage_fs->path, new_path);

	if (rename (full_old, full_new) == -1) {

		if ((errno == EACCES) || (errno == EPERM) || (errno == EROFS)) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else if (errno == ENOENT) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else if ((errno == EEXIST) || (errno == ENOTEMPTY)) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NameExists, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, 
					     NULL);
	}

	g_free (full_old);
	g_free (full_new);
}

static void
fs_commit (BonoboStorage *storage, CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_revert (BonoboStorage *storage, CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static Bonobo_Storage_DirectoryList *
fs_list_contents (BonoboStorage *storage, const CORBA_char *path, 
		  Bonobo_StorageInfoFields mask, CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	Bonobo_Storage_DirectoryList *list = NULL;
	Bonobo_StorageInfo *buf;
	struct dirent *de;
	struct stat st;
	DIR *dir = NULL;
	gint i, max, v, num_entries = 0;
	gchar *full;

	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}

	if (!(dir = opendir (storage_fs->path)))
			goto list_contents_except;
	
	for (max = 0; readdir (dir); max++)
		/* do nothing */;

	rewinddir (dir);

	buf = CORBA_sequence_Bonobo_StorageInfo_allocbuf (max);
	list = Bonobo_Storage_DirectoryList__alloc();
	list->_buffer = buf;
	CORBA_sequence_set_release (list, TRUE); 
	
	for (i = 0; (de = readdir (dir)) && (i < max); i++) {
		
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
		    (de->d_name[0] == '.' && de->d_name[1] == '.' 
		     && de->d_name[2] == '\0'))
			continue; /* Ignore . and .. */

		buf [i].name = CORBA_string_dup (de->d_name);
		buf [i].size = 0;
		buf [i].content_type = NULL;

		full = g_concat_dir_and_file (storage_fs->path, de->d_name);
		v = stat (full, &st);
		g_free (full);

		if (v == -1) 
			goto list_contents_except;

		buf [i].size = st.st_size;
	
		if (S_ISDIR (st.st_mode)) { 
			buf [i].type = Bonobo_STORAGE_TYPE_DIRECTORY;
			buf [i].content_type = 
				CORBA_string_dup ("x-directory/normal");
		} else { 
			buf [i].type = Bonobo_STORAGE_TYPE_REGULAR;
			buf [i].content_type = 
				CORBA_string_dup ("application/octet-stream");
		}

		num_entries++;
	}

	list->_length = num_entries; 

	closedir (dir);
	
	return list; 

 list_contents_except:

	if (dir)
		closedir (dir);

	if (list) 
		CORBA_free (list);
	
	if (errno == ENOENT) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotFound, 
				     NULL);
	else if (errno == ENOTDIR) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotStorage, 
				     NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_IOError, NULL);
	
	return CORBA_OBJECT_NIL;
}

static void
fs_erase (BonoboStorage *storage,
	  const CORBA_char *path,
	  CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);

	if (remove (full) == -1) {

		if (errno == ENOENT) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else if (errno == ENOTEMPTY) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotEmpty, 
					     NULL);
		else if ((errno == EACCES) || (errno = EPERM)) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, NULL);
	}

	g_free (full);
}

static void
bonobo_storage_fs_class_init (BonoboStorageFSClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	BonoboStorageClass *sclass = BONOBO_STORAGE_CLASS (class);
	
	bonobo_storage_fs_parent_class = 
		gtk_type_class (bonobo_storage_get_type ());

	sclass->get_info       = fs_get_info;
	sclass->set_info       = fs_set_info;
	sclass->open_stream    = fs_open_stream;
	sclass->open_storage   = fs_open_storage;
	sclass->copy_to        = NULL; /* use the generic method */
	sclass->rename         = fs_rename;
	sclass->commit         = fs_commit;
	sclass->revert         = fs_revert;
	sclass->list_contents  = fs_list_contents;
	sclass->erase          = fs_erase;
	
	object_class->destroy = bonobo_storage_fs_destroy;
}

static void
bonobo_storage_init (BonoboObject *object)
{
}

GtkType
bonobo_storage_fs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StorageFS:1.0",
			sizeof (BonoboStorageFS),
			sizeof (BonoboStorageFSClass),
			(GtkClassInitFunc) bonobo_storage_fs_class_init,
			(GtkObjectInitFunc) bonobo_storage_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_storage_get_type (), &info);
	}

	return type;
}

BonoboStorage *
bonobo_storage_fs_construct (BonoboStorageFS *storage,
			    Bonobo_Storage corba_storage,
			    const char *path, const char *open_mode)
{
	g_return_val_if_fail (storage != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_STORAGE (storage), NULL);
	g_return_val_if_fail (corba_storage != CORBA_OBJECT_NIL, NULL);

	bonobo_storage_construct (
		BONOBO_STORAGE (storage), corba_storage);

	return BONOBO_STORAGE (storage);
}

/*
 * Creates the Gtk object and the corba server bound to it
 */
static BonoboStorage *
do_bonobo_storage_fs_create (const char *path)
{
	BonoboStorageFS *storage_fs;
	Bonobo_Storage corba_storage;

	storage_fs = gtk_type_new (bonobo_storage_fs_get_type ());
	storage_fs->path = g_strdup (path);
	
	corba_storage = bonobo_storage_corba_object_create (
		BONOBO_OBJECT (storage_fs));
	if (corba_storage == CORBA_OBJECT_NIL){
		bonobo_object_unref (BONOBO_OBJECT (storage_fs));
		return NULL;
	}

	return bonobo_storage_construct
		(BONOBO_STORAGE (storage_fs), corba_storage);
}

/** 
 * bonobo_storage_fs_open:
 * @path: path to existing directory that represents the storage
 * @flags: open flags.
 * @mode: mode used if @flags containst Bonobo_Storage_CREATE for the storage.
 *
 * Returns a BonoboStorage object that represents the storage at @path
 */
BonoboStorage *
bonobo_storage_fs_open (const char *path, gint flags, gint mode,
			CORBA_Environment *ev)
{
	struct stat st;
	
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (ev != NULL, NULL);

	/* Most storages are files */
	mode = mode | 010101;

	if ((flags & Bonobo_Storage_CREATE) &&
	    (mkdir (path, mode) == -1) && (errno != EEXIST)) {

		if (errno == EACCES) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, NULL);
		return NULL;
	}

	if (stat (path, &st) == -1) {
		
		if (errno == ENOENT) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError,
					     NULL);
		return NULL;
	}

	if (!S_ISDIR (st.st_mode)) { 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotStorage, NULL);
		return NULL;
	}

	return do_bonobo_storage_fs_create (path);
}

gint 
init_storage_plugin (StoragePlugin *plugin)
{
	g_return_val_if_fail (plugin != NULL, -1);

	plugin->name = "fs";
	plugin->description = "Native Filesystem Driver";
	plugin->version = BONOBO_STORAGE_VERSION;
	
	plugin->storage_open = bonobo_storage_fs_open; 
	plugin->stream_open = bonobo_stream_fs_open; 

	return 0;
}

