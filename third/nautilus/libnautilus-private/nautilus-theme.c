/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-theme.c: theme framework with xml-based theme definition files
  
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
  
   Author: Andy Hertzfeld <andy@eazel.com>
*/

#include <config.h>
#include "nautilus-theme.h"

#include "nautilus-file-utilities.h"
#include "nautilus-file.h"
#include "nautilus-global-preferences.h"
#include "nautilus-metadata.h"
#include <eel/eel-debug.h>
#include <eel/eel-gdk-pixbuf-extensions.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-string-list.h>
#include <eel/eel-string.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-xml-extensions.h>
#include <libxml/parser.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-util.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <librsvg/rsvg.h>

/* static globals to hold the last accessed theme files */
static char	 *last_theme_name = NULL;
static xmlDocPtr last_theme_document = NULL;

static char *theme_from_preferences = NULL;

#define THEME_PREVIEW_ICON_WIDTH 70
#define THEME_PREVIEW_ICON_HEIGHT 48

static void
theme_changed_callback (gpointer callback_data)
{
	g_free (theme_from_preferences);
	theme_from_preferences = eel_preferences_get (NAUTILUS_PREFERENCES_THEME);
}

/* return the current theme by asking the preferences machinery */
char *
nautilus_theme_get_theme (void)
{
	static gboolean theme_changed_callback_installed = FALSE;

	/* Add the callback once for the life of our process */
	if (!theme_changed_callback_installed) {
		eel_preferences_add_callback (NAUTILUS_PREFERENCES_THEME,
						   theme_changed_callback,
						   NULL);
		theme_changed_callback_installed = TRUE;
		
		/* Peek for the first time */
		theme_changed_callback (NULL);
	}
	
	return g_strdup (theme_from_preferences);
}

/* set the current theme */
void
nautilus_theme_set_theme (const char *new_theme)
{
	char *old_theme;
	
	old_theme = nautilus_theme_get_theme ();
	if (eel_strcmp (old_theme, new_theme)) {
		eel_preferences_set (NAUTILUS_PREFERENCES_THEME, new_theme);
	}
	g_free (old_theme);
}

/* load and parse a theme file */
static xmlDocPtr
load_theme_document (const char *theme_name)
{
	xmlDocPtr theme_document;
	char *theme_path, *temp_str;
	char *user_themes_directory;
	
	temp_str = g_strdup_printf("%s/%s.xml", theme_name, theme_name);
	theme_path = nautilus_pixmap_file (temp_str);
	g_free(temp_str);

	/* if we can't find the theme document in the global area, try in the user's home */
	if (theme_path == NULL) {
		user_themes_directory = nautilus_theme_get_user_themes_directory ();
		temp_str = g_strdup_printf("%s/%s.xml", theme_name, theme_name);
		theme_path = g_build_filename (user_themes_directory, temp_str, NULL);
		
		g_free (user_themes_directory);
		g_free (temp_str);
	
		if (!g_file_test (theme_path, G_FILE_TEST_EXISTS)) {
			g_free (theme_path);
			theme_path = NULL;
		}
	}
	
	/* if the file cannot be found, return NULL for no document */
	if (theme_path == NULL) {
		return NULL;
	}
	
	/* load and parse the theme file */
	theme_document = xmlParseFile (theme_path);
	g_free (theme_path);

	return theme_document;
}

static void
free_last_theme (void)
{
	if (last_theme_document != NULL) {
		xmlFreeDoc (last_theme_document);
	}
	g_free (last_theme_name);
}

/* Fetch data from the specified theme.  Cache the last theme file as a parsed xml document
 */
char *
nautilus_theme_get_theme_data_from_theme (const char *resource_name, const char *property_name, const char *theme_name)
{
	char *temp_str;
	char *theme_data;
	xmlDocPtr theme_document;
	xmlNodePtr resource_node;
	static gboolean did_set_up_free_last_theme = FALSE;
	
	/* fetch the current theme name */
	theme_data = NULL;
	
	if (eel_strcmp (theme_name, last_theme_name) == 0) {
		theme_document = last_theme_document;
	} else {
		/* release the old saved data, since the theme changed */
		if (!did_set_up_free_last_theme) {
			eel_debug_call_at_shutdown (free_last_theme);
			did_set_up_free_last_theme = TRUE;
		}
		free_last_theme ();
		
		last_theme_name = g_strdup (theme_name);
		last_theme_document = load_theme_document (theme_name);
		theme_document = last_theme_document;
	}
	
	if (theme_document != NULL) {
		/* fetch the resource node */			
		resource_node = eel_xml_get_child_by_name (xmlDocGetRootElement (theme_document), resource_name);
		if (resource_node != NULL) {
			temp_str = xmlGetProp (resource_node, property_name);
			if (temp_str != NULL) {
				theme_data = g_strdup (temp_str);
				xmlFree (temp_str);
			}
		}
	}
	
	return theme_data;
}

/* Fetch data from the current theme.
 */
char *
nautilus_theme_get_theme_data (const char *resource_name, const char *property_name)
{
	char *result;
	char *theme_name;
	theme_name = nautilus_theme_get_theme ();
	result = nautilus_theme_get_theme_data_from_theme (resource_name, property_name, theme_name);
	g_free (theme_name);
	return result;
}

/* utility routine to return the full path to a themed image that
   searches the local themes if it can't find it in the shared space */
static char *
nautilus_pixmap_file_may_be_local (const char *themed_image)
{
	char *image_path, *user_themes_directory;
	
	image_path = nautilus_pixmap_file (themed_image);
	if (image_path == NULL) {
		user_themes_directory = nautilus_theme_get_user_themes_directory ();
		
		image_path = g_build_filename (user_themes_directory, themed_image, NULL);
		if (!g_file_test (image_path, G_FILE_TEST_EXISTS)) {
			g_free (image_path);
			image_path = NULL;
		}
		
		g_free (user_themes_directory);
	}
	return image_path;
}

/* given a theme, fetch the full path name of an image with the passed-in name  */
/* return NULL if there isn't a corresponding image.  Optionally, add a .png suffix if we */
/* cant otherwise find one. 								  */

char *
nautilus_theme_get_image_path_from_theme (const char *image_name, const char* theme_name)
{
	char *image_path, *png_string, *temp_str;
	
	temp_str = g_strdup_printf ("%s/%s", theme_name, image_name);
	image_path = nautilus_pixmap_file_may_be_local (temp_str);
	
	/* see if a theme-specific image exists; if so, return it */
	if (image_path != NULL) {
		g_free (temp_str);	
		return image_path;
	}
	
	/* try if with a .png extension if it doesn't already have one */
	if (!eel_istr_has_suffix (image_name, ".png")) {
		png_string = g_strconcat (temp_str, ".png", NULL);
		image_path = nautilus_pixmap_file_may_be_local (png_string);
		g_free (png_string);
		
		if (image_path) {
			g_free (temp_str);	
			return image_path;
		}
	}
	g_free (temp_str);


	/* we couldn't find a theme specific one, so look for a general image */
	image_path = nautilus_pixmap_file (image_name);
	if (image_path != NULL) {
		return image_path;
	}
	
	/* if it doesn't have a .png extension, try it with that */
	if (!eel_istr_has_suffix (image_name, ".png")) {
		png_string = g_strconcat (image_name, ".png", NULL);
		image_path = nautilus_pixmap_file (png_string);
		g_free (png_string);
		
		if (image_path) {
			return image_path;
		}
	}
		
	/* we couldn't find anything, so return NULL */
	g_free (image_path);
	return NULL;
}

/* commonly used cover to get_image_path_from_theme to return an image path using the current theme */
char *
nautilus_theme_get_image_path (const char *image_name)
{
	char *theme_name, *image_path;
	
	theme_name = nautilus_theme_get_theme ();
	image_path = nautilus_theme_get_image_path_from_theme (image_name, theme_name);	
	g_free (theme_name);
	
	return image_path;
}

/* create a pixbuf that represents the passed in theme name */
GdkPixbuf *
nautilus_theme_make_preview_pixbuf (const char *theme_name)
{
	char *pixbuf_file, *theme_preview_name;
	char *user_themes_directory;
	GdkPixbuf *pixbuf;
	
	/* first, see if we can find an explicit preview */
	theme_preview_name = g_strdup_printf ("%s/%s", theme_name, "theme_preview.png");
	pixbuf_file = nautilus_pixmap_file (theme_preview_name);
	if (pixbuf_file != NULL) {
		pixbuf = gdk_pixbuf_new_from_file (pixbuf_file, NULL);
		g_free (theme_preview_name);
		g_free (pixbuf_file);
		return pixbuf;
	} else {
		/* try the user directory */
		user_themes_directory = nautilus_theme_get_user_themes_directory ();
		pixbuf_file = g_build_filename (user_themes_directory, theme_preview_name, NULL);
		g_free (user_themes_directory);
		
		if (g_file_test (pixbuf_file, G_FILE_TEST_EXISTS)) {
			pixbuf = gdk_pixbuf_new_from_file (pixbuf_file, NULL);
			g_free (pixbuf_file);
			g_free (theme_preview_name);
			return pixbuf;
		}  else {
			g_free (pixbuf_file);
		}
	}
	
	/* couldn't find a custom one, so try for a directory */	
	g_free (theme_preview_name);
	theme_preview_name = g_strdup_printf ("%s/%s", theme_name, "i-directory.png");
	pixbuf_file = nautilus_pixmap_file (theme_preview_name);
	g_free (theme_preview_name);
	
	if (pixbuf_file == NULL) {
		theme_preview_name = g_strdup_printf ("%s/%s", theme_name, "i-directory.svg");
		pixbuf_file = nautilus_pixmap_file (theme_preview_name);
		g_free (theme_preview_name);
	}
	
	/* try the user directory if necessary */
	if (pixbuf_file == NULL) {
		user_themes_directory = nautilus_theme_get_user_themes_directory ();
		theme_preview_name = g_strdup_printf ("%s/i-directory.png", theme_name);
		pixbuf_file = g_build_filename (user_themes_directory, theme_preview_name, NULL);
		g_free (theme_preview_name);
		
		if (!g_file_test (pixbuf_file, G_FILE_TEST_EXISTS)) {
			g_free (pixbuf_file);
			theme_preview_name = g_strdup_printf ("%s/i-directory.svg", theme_name);
			pixbuf_file = g_build_filename (user_themes_directory, theme_preview_name, NULL);
			g_free (theme_preview_name);
		
			if (!g_file_test (pixbuf_file, G_FILE_TEST_EXISTS)) {
				g_free (pixbuf_file);
				pixbuf_file = NULL;
			}
		}
		
		g_free (user_themes_directory);		
	}
	
	/* if we can't find anything, return NULL */
	if (pixbuf_file == NULL) {
		return NULL;
	}
	
	pixbuf = NULL;
	
	/* load the icon that we found and return it */
	if (eel_istr_has_suffix (pixbuf_file, ".svg")) {
		pixbuf = rsvg_pixbuf_from_file (pixbuf_file, NULL);
	} else {
		pixbuf = gdk_pixbuf_new_from_file (pixbuf_file, NULL);
	}
	
	g_free (pixbuf_file);
	return pixbuf;
}

typedef struct 
{
	char *name;
	char *path;
	char *display_name;
	char *description;
	GdkPixbuf *preview_pixbuf;
	gboolean builtin;
} ThemeAttibutes;

/* Test for the presence of an icon file */
static gboolean
vfs_file_exists (const char *file_uri)
{
	GnomeVFSResult result;
	GnomeVFSFileInfo *file_info;
	
	file_info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (file_uri, file_info, 0);
	gnome_vfs_file_info_unref (file_info);

	return result == GNOME_VFS_OK;
}

static gboolean
has_image_file (const char *path_uri,
		const char *dir_name,
		const char *image_file)
{
	char* image_uri;
	gboolean exists;

	image_uri = g_strdup_printf ("%s/%s/%s.png", path_uri, dir_name, image_file);

	exists = vfs_file_exists (image_uri);
	g_free (image_uri);

	if (exists) {
		return TRUE;
	}

	image_uri = g_strdup_printf ("%s/%s/%s.svg", path_uri, dir_name, image_file);

	exists = vfs_file_exists (image_uri);
	g_free (image_uri);

	return exists;
}

static char*
theme_get_property (const char *themes_location_uri,
		    const char *theme_name,
		    const char *property)
{
	char *theme_file_uri;
	char *theme_file_name;
 	xmlDocPtr theme_document;
	xmlChar *xml_result;
	char *result;

	g_return_val_if_fail (themes_location_uri != NULL, NULL);
	g_return_val_if_fail (theme_name != NULL, NULL);
	g_return_val_if_fail (property != NULL, NULL);

	xml_result = NULL;
	result = NULL;

	theme_file_uri = g_strdup_printf ("%s/%s/%s.xml",
					  themes_location_uri,
					  theme_name,
					  theme_name);

	theme_file_name = gnome_vfs_get_local_path_from_uri (theme_file_uri);
	g_free (theme_file_uri);

	g_return_val_if_fail (g_file_test (theme_file_name,
					   G_FILE_TEST_EXISTS), NULL);
	
	/* read the xml document */
	theme_document = xmlParseFile (theme_file_name);
	g_free (theme_file_name);
	g_return_val_if_fail (theme_document != NULL, NULL);
		
	/* fetch the property, if any */		
	xml_result = eel_xml_get_property_translated (xmlDocGetRootElement (theme_document),
						      property);
	xmlFreeDoc (theme_document);
	
	/* Convert the xml char* to a regular char* to get allocators matched */
	if (xml_result != NULL) {
		result = g_strdup (xml_result);
		xmlFree (xml_result);
	}

	return result;
}

static char*
theme_get_name_property (const char *themes_location_uri,
			 const char *theme_name)
{
	char *name;

	g_return_val_if_fail (theme_name != NULL, NULL);
	g_return_val_if_fail (themes_location_uri != NULL, NULL);

	name = theme_get_property (themes_location_uri, theme_name, "name");
	
	if (name == NULL) {
		name = g_strdup (theme_name);
	}

	return name;
}

static char*
theme_get_description_property (const char *themes_location_uri,
				const char *theme_name)
{
	char *description;

	g_return_val_if_fail (themes_location_uri != NULL, NULL);
	g_return_val_if_fail (theme_name != NULL, NULL);

	description = theme_get_property (themes_location_uri, theme_name, "description");
	
	if (description == NULL) {
		description = g_strdup_printf (_("No description available for the \"%s\" theme"), theme_name);
	}

	return description;
}

static GList *
theme_list_prepend (GList *theme_list,
		    const char *themes_location_uri,
		    const char *theme_name,
		    gboolean builtin)
{
	ThemeAttibutes *attributes;
	GdkPixbuf *unscaled_preview_pixbuf;

	g_return_val_if_fail (theme_name != NULL, NULL);
	g_return_val_if_fail (themes_location_uri != NULL, NULL);
	
	attributes = g_new0 (ThemeAttibutes, 1);
	attributes->name = g_strdup (theme_name);
	attributes->path = g_build_filename (themes_location_uri, theme_name, NULL);

	unscaled_preview_pixbuf = nautilus_theme_make_preview_pixbuf (theme_name);
	attributes->preview_pixbuf = eel_gdk_pixbuf_scale_down_to_fit (unscaled_preview_pixbuf,
								       THEME_PREVIEW_ICON_WIDTH,
								       THEME_PREVIEW_ICON_HEIGHT);
	g_object_unref (unscaled_preview_pixbuf);

	attributes->builtin = builtin;

	attributes->display_name = theme_get_name_property (themes_location_uri, theme_name);
	attributes->description = theme_get_description_property (themes_location_uri, theme_name);

	return g_list_prepend (theme_list, attributes);
}

static GList *
theme_get_themes_for_location (const char *themes_location_uri,
			       gboolean builtin)
{
	GnomeVFSResult result;
	GnomeVFSFileInfo *file_info;
	GList *possible_theme_directories;
	GList *node;
	GList *themes;
	
	g_return_val_if_fail (themes_location_uri != NULL, NULL);

	possible_theme_directories = NULL;
	result = gnome_vfs_directory_list_load (&possible_theme_directories,
						themes_location_uri,
						GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	
	if (result != GNOME_VFS_OK) {
		return NULL;
	}

	themes = NULL;
	for (node = possible_theme_directories; node != NULL; node = node->next) {
		g_assert (node->data != NULL);

		file_info = node->data;

		if ((file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		    && (file_info->name[0] != '.')) {
			if (has_image_file (themes_location_uri, file_info->name, "i-directory" )) {
				themes = theme_list_prepend (themes,
							     themes_location_uri,
							     file_info->name,
							     builtin);
			}
		}
	}

	return g_list_reverse (themes);
}

static GList *
theme_get_builtin_themes (void)
{
	char *pixmap_directory;
	char *builtin_themes_location_uri;
	GList *builtin_themes;
	
	pixmap_directory = nautilus_get_pixmap_directory ();
	builtin_themes_location_uri = gnome_vfs_get_uri_from_local_path (pixmap_directory);
	builtin_themes = theme_get_themes_for_location (builtin_themes_location_uri, TRUE);
	g_free (pixmap_directory);
	g_free (builtin_themes_location_uri);

	return builtin_themes;
}

static GList *
theme_get_user_themes (void)
{
	char *user_themes_location;
	char *user_themes_location_uri;
	GList *user_themes;

	user_themes_location = nautilus_theme_get_user_themes_directory ();
	user_themes_location_uri = gnome_vfs_get_uri_from_local_path (user_themes_location);
	user_themes = theme_get_themes_for_location (user_themes_location_uri, FALSE);
	g_free (user_themes_location);
	g_free (user_themes_location_uri);

	return user_themes;
}

static void
theme_list_invoke_callback (GList *theme_list,
			    NautilusThemeCallback callback,
			    gpointer callback_data)
{
	GList *node;
	const ThemeAttibutes *attributes;

	g_return_if_fail (callback != NULL);

	for (node = theme_list; node != NULL; node = node->next) {
		g_assert (node->data != NULL);
		attributes = node->data;

		g_assert (attributes->name != NULL);
		g_assert (attributes->path != NULL);
		g_assert (attributes->display_name != NULL);
		g_assert (attributes->description != NULL);
		g_assert (attributes->preview_pixbuf != NULL);

		(* callback) (attributes->name,
			      attributes->path,
			      attributes->display_name,
			      attributes->description,
			      attributes->preview_pixbuf,
			      attributes->builtin,
			      callback_data);
	}
}

static void
attributes_free (gpointer data,
		 gpointer user_data)
{
	ThemeAttibutes *attributes;

	g_return_if_fail (data != NULL);

	attributes = data;

	g_free (attributes->name);
	g_free (attributes->path);
	g_free (attributes->display_name);
	g_free (attributes->description);
	if (attributes->preview_pixbuf != NULL) {
		g_object_unref (attributes->preview_pixbuf);
	}

	g_free (attributes);
}

void
nautilus_theme_for_each_theme (NautilusThemeCallback callback,
			       gpointer callback_data)
{
	GList *builtin_themes;
	GList *user_themes;

	g_return_if_fail (callback != NULL);

	builtin_themes = theme_get_builtin_themes ();
 	user_themes = theme_get_user_themes ();

	theme_list_invoke_callback (builtin_themes, callback, callback_data);
	theme_list_invoke_callback (user_themes, callback, callback_data);

	eel_g_list_free_deep_custom (builtin_themes, attributes_free, NULL);
	eel_g_list_free_deep_custom (user_themes, attributes_free, NULL);
}

char *
nautilus_theme_get_user_themes_directory (void)
{
	char *user_directory;
	char *user_themes_directory;

	user_directory = nautilus_get_user_directory ();
	user_themes_directory = g_build_filename (user_directory, "themes", NULL);
	g_free (user_directory);

	return user_themes_directory;
}

/* Remove the given theme name from from Nautilus. */
GnomeVFSResult
nautilus_theme_remove_user_theme (const char *theme_to_remove_name)
{
	char *user_themes_directory;
	char *theme_to_remove_path;
	GnomeVFSResult result;
	GList *uri_list;

	g_return_val_if_fail (theme_to_remove_name != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	user_themes_directory = nautilus_theme_get_user_themes_directory ();
	theme_to_remove_path = g_build_filename (user_themes_directory, theme_to_remove_name, NULL);
	g_free (user_themes_directory);
	
	uri_list = g_list_prepend (NULL, gnome_vfs_uri_new (theme_to_remove_path));			
	g_free (theme_to_remove_path);

	result = gnome_vfs_xfer_delete_list (uri_list, GNOME_VFS_XFER_RECURSIVE,
					     GNOME_VFS_XFER_ERROR_MODE_ABORT,
					     NULL, NULL);
	gnome_vfs_uri_list_free (uri_list);

	return result;
}

/* Install the theme found at the given path (if valid). */
NautilusThemeInstallResult
nautilus_theme_install_user_theme (const char *theme_to_install_path)
{
	GnomeVFSHandle *handle;
	GnomeVFSResult result;
	char *theme_name;
	char *theme_xml_path;
	char *user_themes_directory;
	char *theme_destination_path;
	char *command;
	char *mime_type;
	char *quoted_user_path;
	char *quoted_theme_path;
	int status;

	user_themes_directory = nautilus_theme_get_user_themes_directory ();
	
	/* Create the user themes directory if it doesn't exist */
	if (!g_file_test (user_themes_directory, G_FILE_TEST_EXISTS)) {
		result = gnome_vfs_make_directory (user_themes_directory,
						   GNOME_VFS_PERM_USER_ALL
						   | GNOME_VFS_PERM_GROUP_ALL
						   | GNOME_VFS_PERM_OTHER_READ);

		if (result != GNOME_VFS_OK) {
			g_free (user_themes_directory);
			return NAUTILUS_THEME_INSTALL_FAILED_USER_THEMES_DIRECTORY_CREATION;
		}
	}

	if (theme_to_install_path != NULL && g_file_test (theme_to_install_path, G_FILE_TEST_IS_REGULAR)) {
		result = gnome_vfs_open (&handle, theme_to_install_path, GNOME_VFS_OPEN_READ);
		gnome_vfs_close (handle);
		/* Did we manage to read the file? */
		if (result != GNOME_VFS_OK) {
			g_free (user_themes_directory);
			return NAUTILUS_THEME_INSTALL_FAILED;
		}
		mime_type = gnome_vfs_get_mime_type (theme_to_install_path);
		if (mime_type != NULL) {
			quoted_theme_path = g_shell_quote (theme_to_install_path);
			quoted_user_path = g_shell_quote (user_themes_directory);
			
			if (strcmp (mime_type, "application/x-compressed-tar") == 0) {
				/* gzipped tarball */
				command = g_strdup_printf ("gzip -d -c < %s | (cd %s ; tar -xf -)", 
							   quoted_theme_path,
							   quoted_user_path
							   );
			} else if (strcmp (mime_type, "application/x-tar") == 0) {
				/* vanilla tarball */
				command = g_strdup_printf ("cd %s && tar -xf %s", 
							   quoted_user_path, 
							   quoted_theme_path
							   );
					   
			} else if (strcmp (mime_type, "application/x-bzip") == 0) {
				/* bzipped tarball */
				command = g_strdup_printf ("bzip2 -d -c < %s | (cd %s ; tar -xf -)", 
							   quoted_theme_path, 
							   quoted_user_path
							   );
			} else {
				/* unsupported mime-type */
				command = NULL;
			}
			g_free (user_themes_directory);
			g_free (quoted_theme_path);
			g_free (quoted_user_path);
			g_free (mime_type);
			if (command != NULL) {
				status = system (command);
				g_free (command);
				if (status != 0) {
					return NAUTILUS_THEME_INSTALL_FAILED;
				} else {
					return NAUTILUS_THEME_INSTALL_OK; 
				}
			} else {
				return NAUTILUS_THEME_INSTALL_NOT_A_THEME_FILE;
			}
		} else {
			g_free (user_themes_directory);
			return NAUTILUS_THEME_INSTALL_FAILED;
		}
	}

	if (theme_to_install_path == NULL
	    || !g_file_test (theme_to_install_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		return NAUTILUS_THEME_INSTALL_NOT_A_THEME_DIRECTORY;
	}
	
	theme_name = eel_uri_get_basename (theme_to_install_path);
	g_return_val_if_fail (theme_name != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	theme_destination_path = g_build_filename (user_themes_directory, theme_name, NULL);
	theme_xml_path = g_strdup_printf ("%s/%s.xml",
					  theme_to_install_path,
					  theme_name);
	g_free (user_themes_directory);
	g_free (theme_name);
	if (!g_file_test (theme_xml_path, G_FILE_TEST_EXISTS)) {
		g_free (theme_destination_path);
		g_free (theme_xml_path);
		return NAUTILUS_THEME_INSTALL_NOT_A_THEME_DIRECTORY;
	}
	g_free (theme_xml_path);
	result = eel_copy_uri_simple (theme_to_install_path, theme_destination_path);
	if (result != GNOME_VFS_OK) {
		g_free (theme_destination_path);
		return NAUTILUS_THEME_INSTALL_FAILED;
	}

	g_free (theme_destination_path);
	
	return NAUTILUS_THEME_INSTALL_OK;
}
