/*
 *  nautilus-file-info.c - Information about a file 
 *
 *  Copyright (C) 2003 Novell, Inc.
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
 *
 */

#include <config.h>
#include "nautilus-file-info.h"


GList *
nautilus_file_info_list_copy (GList *files)
{
	GList *ret;
	GList *l;
	
	ret = g_list_copy (files);
	for (l = ret; l != NULL; l = l->next) {
		g_object_ref (G_OBJECT (l->data));
	}

	return ret;
}

void              
nautilus_file_info_list_free (GList *files)
{
	GList *l;
	
	for (l = files; l != NULL; l = l->next) {
		g_object_unref (G_OBJECT (l->data));
	}
	
	g_list_free (files);
}

static void
nautilus_file_info_base_init (gpointer g_class)
{
}

GType                   
nautilus_file_info_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (NautilusFileInfoIface),
			nautilus_file_info_base_init,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "NautilusFileInfo",
					       &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

gboolean
nautilus_file_info_is_gone (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), FALSE);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->is_gone != NULL, FALSE);
	
	return NAUTILUS_FILE_INFO_GET_IFACE (file)->is_gone (file);
}

char *
nautilus_file_info_get_name (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_name != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_name (file);
}

char *
nautilus_file_info_get_uri (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_uri != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_uri (file);
}

char *
nautilus_file_info_get_parent_uri (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_uri != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_parent_uri (file);
}

char *
nautilus_file_info_get_uri_scheme (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_uri_scheme != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_uri_scheme (file);
}

char *
nautilus_file_info_get_mime_type (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_mime_type != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_mime_type (file);
}

gboolean
nautilus_file_info_is_mime_type (NautilusFileInfo *file,
				 const char *mime_type)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), FALSE);
	g_return_val_if_fail (mime_type != NULL, FALSE);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->is_mime_type != NULL, FALSE);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->is_mime_type (file,
								  mime_type);
}

gboolean
nautilus_file_info_is_directory (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), FALSE);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->is_directory != NULL, FALSE);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->is_directory (file);
}

GnomeVFSFileInfo *
nautilus_file_info_get_vfs_file_info (NautilusFileInfo *file)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_vfs_file_info != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_vfs_file_info (file);
}

void
nautilus_file_info_add_emblem (NautilusFileInfo *file,
			       const char *emblem_name)
{
	g_return_if_fail (NAUTILUS_IS_FILE_INFO (file));
	g_return_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_vfs_file_info != NULL);

	NAUTILUS_FILE_INFO_GET_IFACE (file)->add_emblem (file, emblem_name);
}

char *
nautilus_file_info_get_string_attribute (NautilusFileInfo *file,
					 const char *attribute_name)
{
	g_return_val_if_fail (NAUTILUS_IS_FILE_INFO (file), NULL);
	g_return_val_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->get_string_attribute != NULL, NULL);
	g_return_val_if_fail (attribute_name != NULL, NULL);

	return NAUTILUS_FILE_INFO_GET_IFACE (file)->get_string_attribute 
		(file, attribute_name);
}

void
nautilus_file_info_add_string_attribute (NautilusFileInfo *file,
					 const char *attribute_name,
					 const char *value)
{
	g_return_if_fail (NAUTILUS_IS_FILE_INFO (file));
	g_return_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->add_string_attribute != NULL);
	g_return_if_fail (attribute_name != NULL);
	g_return_if_fail (value != NULL);
	
	NAUTILUS_FILE_INFO_GET_IFACE (file)->add_string_attribute 
		(file, attribute_name, value);
}

void
nautilus_file_info_invalidate_extension_info (NautilusFileInfo *file)
{
	g_return_if_fail (NAUTILUS_IS_FILE_INFO (file));
	g_return_if_fail (NAUTILUS_FILE_INFO_GET_IFACE (file)->invalidate_extension_info != NULL);
	
	NAUTILUS_FILE_INFO_GET_IFACE (file)->invalidate_extension_info (file);
}
