/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-link-historical.c: xml-based link files.
 
   Copyright (C) 1999, 2000, 2001 Eazel, Inc.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the historicalied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Author: Andy Hertzfeld <andy@eazel.com>
*/

#include <config.h>
#include "nautilus-link-historical.h"

#include "nautilus-directory-notify.h"
#include "nautilus-directory.h"
#include "nautilus-file-attributes.h"
#include "nautilus-file.h"
#include "nautilus-file-utilities.h"
#include "nautilus-global-preferences.h"
#include "nautilus-metadata.h"
#include "nautilus-program-choosing.h"
#include <eel/eel-glib-extensions.h>
#include <eel/eel-gnome-extensions.h>
#include <eel/eel-preferences.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-xml-extensions.h>
#include <libxml/parser.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnomevfs/gnome-vfs.h>
#include <stdlib.h>


#define NAUTILUS_LINK_GENERIC_TAG	"Generic Link"
#define NAUTILUS_LINK_TRASH_TAG 	"Trash Link"
#define NAUTILUS_LINK_MOUNT_TAG 	"Mount Link"
#define NAUTILUS_LINK_HOME_TAG 		"Home Link"

#define REMOTE_ICON_DIR_PERMISSIONS (GNOME_VFS_PERM_USER_ALL \
				     | GNOME_VFS_PERM_GROUP_ALL \
				     | GNOME_VFS_PERM_OTHER_ALL)

typedef void (* NautilusFileFunction) (NautilusFile *file);

static const char *
get_tag (NautilusLinkType type)
{
	switch (type) {
	default:
		g_assert_not_reached ();
		/* fall through */
	case NAUTILUS_LINK_GENERIC:
		return NAUTILUS_LINK_GENERIC_TAG;
	case NAUTILUS_LINK_TRASH:
		return NAUTILUS_LINK_TRASH_TAG;
	case NAUTILUS_LINK_MOUNT:
		return NAUTILUS_LINK_MOUNT_TAG;
	case NAUTILUS_LINK_HOME:
		return NAUTILUS_LINK_HOME_TAG;
	}
}

static NautilusLinkType
get_link_type (const char *tag)
{
	if (tag != NULL) {
		if (strcmp (tag, NAUTILUS_LINK_TRASH_TAG) == 0) {
			return NAUTILUS_LINK_TRASH;
		}
		if (strcmp (tag, NAUTILUS_LINK_MOUNT_TAG) == 0) {
			return NAUTILUS_LINK_MOUNT;
		}
		if (strcmp (tag, NAUTILUS_LINK_HOME_TAG) == 0) {
			return NAUTILUS_LINK_HOME;
		}
	}
	return NAUTILUS_LINK_GENERIC;
}

gboolean
nautilus_link_historical_local_create (const char       *directory_uri,
				       const char       *name,
				       const char       *image,
				       const char       *target_uri,
				       const GdkPoint   *point,
				       NautilusLinkType  type)
{
	xmlDocPtr output_document;
	xmlNodePtr root_node;
	char *directory_path, *path;
	int result;
	char *uri;
	GList dummy_list;
	NautilusFileChangesQueuePosition item;

	
	g_return_val_if_fail (directory_uri != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (image != NULL, FALSE);
	g_return_val_if_fail (target_uri != NULL, FALSE);
	
	/* create a new xml document */
	output_document = xmlNewDoc ("1.0");
	
	/* add the root node to the output document */
	root_node = xmlNewDocNode (output_document, NULL, "nautilus_object", NULL);
	xmlDocSetRootElement (output_document, root_node);

	/* Add mime magic string so that the mime sniffer can recognize us.
	 * Note: The value of the tag identfies what type of link this.  */
	xmlSetProp (root_node, "nautilus_link", get_tag (type));
	
	/* Add link and custom icon tags */
	xmlSetProp (root_node, "custom_icon", image);
	xmlSetProp (root_node, "link", target_uri);
	
	/* all done, so save the xml document as a link file */
	directory_path = gnome_vfs_get_local_path_from_uri (directory_uri);
	if (directory_uri == NULL) {
		xmlFreeDoc (output_document);
		return FALSE;
	}

	path = g_build_filename (directory_path, name, NULL);
	g_free (directory_path);

	result = xmlSaveFile (path, output_document);
	
	xmlFreeDoc (output_document);

	if (result <= 0) {
		g_free (path);
		return FALSE;
	}
	
	/* Notify that this new file has been created. */
	uri = gnome_vfs_get_uri_from_local_path (path);
	dummy_list.data = uri;
	dummy_list.next = NULL;
	dummy_list.prev = NULL;
	nautilus_directory_notify_files_added (&dummy_list);
	nautilus_directory_schedule_metadata_remove (&dummy_list);

	if (point != NULL) {
		item.uri = uri;
		item.set = TRUE;
		item.point.x = point->x;
		item.point.y = point->y;
		
		dummy_list.data = &item;
		dummy_list.next = NULL;
		dummy_list.prev = NULL;
	
		nautilus_directory_schedule_position_set (&dummy_list);
	}

	g_free (uri);

	g_free (path);

	return TRUE;
}

static char *
xml_get_root_property (xmlDoc *doc,
		       const char *key)
{
	char *property, *duplicate;
	
	/* Need to g_strdup so we can free with g_free instead of xmlFree. */
	property = xmlGetProp (xmlDocGetRootElement (doc), key);
	duplicate = g_strdup (property);
	xmlFree (property);
	return duplicate;
}

static char *
local_get_root_property (const char *uri,
			 const char *key)
{
	GnomeVFSFileInfo *info;
	char *path;
	GnomeVFSResult result;
	gboolean is_link;
	xmlDoc *document;
	char *property;
	
	/* Check mime type. Exit if it is not a nautilus link */

	info = gnome_vfs_file_info_new ();

	result = gnome_vfs_get_file_info (uri, info,
					  GNOME_VFS_FILE_INFO_GET_MIME_TYPE
					  | GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	is_link = result == GNOME_VFS_OK
		&& (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE) != 0
		&& g_ascii_strcasecmp (info->mime_type, "application/x-nautilus-link") == 0;

	gnome_vfs_file_info_unref (info);

	if (!is_link) {
		return NULL;
	}
	
	path = gnome_vfs_get_local_path_from_uri (uri);
	if (path == NULL) {
		return NULL;
	}
		
	document = xmlParseFile (path);
	g_free (path);

	if (document == NULL) {
		return NULL;
	}

	property = xml_get_root_property (document, key);
	xmlFreeDoc (document);
	return property;
}

static gboolean
local_set_root_property (const char *uri,
			 const char *key,
			 const char *value,
			 NautilusFileFunction extra_notify)
{
	xmlDocPtr document;
	xmlNodePtr root;
	xmlChar *old_value;
	char *path;
	NautilusFile *file;

	path = gnome_vfs_get_local_path_from_uri (uri);
	if (path == NULL) {
		return FALSE;
	}
	document = xmlParseFile (path);
	if (document == NULL) {
		g_free (path);
		return FALSE;
	}
	root = xmlDocGetRootElement (document);
	if (root == NULL) {
		xmlFreeDoc (document);
		g_free (path);
		return FALSE;
	}

	/* Check if the property value is already correct. */
	old_value = xmlGetProp (root, key);
	if (old_value != NULL && strcmp (old_value, value) == 0) {
		xmlFree (old_value);
		xmlFreeDoc (document);
		g_free (path);
		return TRUE;
	}

	xmlFree (old_value);

	/* Change and write the property. */
	xmlSetProp (root, key, value);
	xmlSaveFile (path, document);
	xmlFreeDoc (document);

	/* Notify about the change. */
	file = nautilus_file_get (uri);
	if (file != NULL) {
		if (extra_notify != NULL) {
			(* extra_notify) (file);
		}
		nautilus_file_changed (file);
		nautilus_file_unref (file);
	}
	g_free (path);
		
	return TRUE;
}


/* Set the icon for a link file. This can only be called on local
 * paths, and only on files known to be link files.
 */
gboolean
nautilus_link_historical_local_set_icon (const char *uri, const char *icon_name)
{
	return local_set_root_property (uri,
					NAUTILUS_METADATA_KEY_CUSTOM_ICON,
					icon_name,
					NULL);
}


/* Set the link uri for a link file. This can only be called on local
 * paths, and only on files known to be link files.
 */
gboolean
nautilus_link_historical_local_set_link_uri (const char *uri, const char *link_uri)
{
	return local_set_root_property (uri,
					"link",
					link_uri,
					NULL);
}

gboolean
nautilus_link_historical_local_set_type (const char *uri, NautilusLinkType type)
{
	return local_set_root_property (uri,
					"nautilus_link",
					get_tag (type),
					NULL);
}

/* returns additional text to display under the name, NULL if none */
char *
nautilus_link_historical_local_get_additional_text (const char *uri)
{
	return local_get_root_property
		(uri, NAUTILUS_METADATA_KEY_EXTRA_TEXT);
}


/* Returns the link uri associated with a link file. */
char *
nautilus_link_historical_local_get_link_uri (const char *uri)
{
	return local_get_root_property (uri, "link");
}

/* Returns the link type of the link file. */
NautilusLinkType
nautilus_link_historical_local_get_link_type (const char *uri)
{
	char *property;
	NautilusLinkType type;
	
	property = local_get_root_property (uri, "nautilus_link");
	type = get_link_type (property);
	g_free (property);

	return type;
}

/* FIXME bugzilla.eazel.com 2495: 
 * Caller has to know to pass in a file with a NUL character at the end.
 */
char *
nautilus_link_historical_get_link_uri_given_file_contents (const char *file_contents,
							   int file_size)
{
	xmlDoc *doc;
	char *property;
	
	doc = xmlParseMemory ((char *) file_contents, file_size);
	property = xml_get_root_property (doc, "link");
	xmlFreeDoc (doc);
	return property;
}


char *
nautilus_link_historical_get_link_icon_given_file_contents (const char *file_contents,
							    int         file_size)
{
	xmlDoc *doc;
	char *property;
	
	doc = xmlParseMemory ((char *) file_contents, file_size);
	property = xml_get_root_property (doc, NAUTILUS_METADATA_KEY_CUSTOM_ICON);
	xmlFreeDoc (doc);
	return property;
}

void
nautilus_link_historical_local_create_from_gnome_entry (GnomeDesktopItem *entry, const char *dest_uri, const GdkPoint *position)
{
	char *icon_name, *icon;
	char *launch_string, *terminal_command;
	const char *name, *arguments;

	if (entry == NULL || dest_uri == NULL) {
		return;
	}
	
	name      = gnome_desktop_item_get_string (entry, GNOME_DESKTOP_ITEM_NAME);
	arguments = gnome_desktop_item_get_string (entry, GNOME_DESKTOP_ITEM_EXEC);

	switch (gnome_desktop_item_get_entry_type (entry)) {
	case GNOME_DESKTOP_ITEM_TYPE_APPLICATION:
		if (gnome_desktop_item_get_boolean (entry, GNOME_DESKTOP_ITEM_TERMINAL)) {
			terminal_command = eel_gnome_make_terminal_command (arguments);
			launch_string = g_strconcat (NAUTILUS_COMMAND_SPECIFIER, terminal_command, NULL);
			g_free (terminal_command);
		} else {
			launch_string = g_strconcat (NAUTILUS_COMMAND_SPECIFIER, arguments, NULL);
		}
		break;
	case GNOME_DESKTOP_ITEM_TYPE_LINK:
		launch_string = g_strdup (arguments);
		break;
	default:
		/* Unknown .desktop file type */
		launch_string = NULL;
	}
	

	icon = gnome_desktop_item_get_icon (entry, NULL);
	if (icon != NULL) {
		icon_name = eel_make_uri_from_half_baked_uri (icon);
		g_free (icon);
	} else {
		icon_name = g_strdup ("gnome-unknown.png");
	}
	
	if (launch_string != NULL) {
		nautilus_link_historical_local_create (dest_uri, name, icon_name,
						       launch_string, position, NAUTILUS_LINK_GENERIC);
	}
	
	g_free (icon_name);
	g_free (launch_string);
}
