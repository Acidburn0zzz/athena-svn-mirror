/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Nautilus
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *           Maciej Stachowiak <mjs@eazel.com>
 *
 */


/* #define DEBUG_MJS 1 */

/* nautilus-applicable-views.c: Implementation of routines for mapping a location
   change request to a set of views and actual URL to be loaded. */

#include <config.h>
#include "nautilus-applicable-views.h"

#include <ctype.h>
#include <dirent.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libnautilus-extensions/nautilus-directory.h>
#include <libnautilus-extensions/nautilus-file-attributes.h>
#include <libnautilus-extensions/nautilus-file.h>
#include <libnautilus-extensions/nautilus-glib-extensions.h>
#include <libnautilus-extensions/nautilus-global-preferences.h>
#include <libnautilus-extensions/nautilus-metadata.h>
#include <libnautilus-extensions/nautilus-mime-actions.h>
#include <libnautilus-extensions/nautilus-string.h>
#include <libnautilus-extensions/nautilus-view-identifier.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>

struct NautilusNavigationInfo {
	NautilusNavigationCallback callback;
	gpointer callback_data;
        char *location;
        NautilusFile *file;
        NautilusDirectory *directory;
	NautilusViewIdentifier *initial_content_id;
	GnomeVFSAsyncHandle *handle;
};

static NautilusNavigationResult
get_nautilus_navigation_result_from_gnome_vfs_result (GnomeVFSResult gnome_vfs_result)
{
        switch (gnome_vfs_result) {
        case GNOME_VFS_OK:
                return NAUTILUS_NAVIGATION_RESULT_OK;
        case GNOME_VFS_ERROR_NOT_FOUND:
                return NAUTILUS_NAVIGATION_RESULT_NOT_FOUND;
        case GNOME_VFS_ERROR_INVALID_URI:
                return NAUTILUS_NAVIGATION_RESULT_INVALID_URI;
        case GNOME_VFS_ERROR_NOT_SUPPORTED:
                return NAUTILUS_NAVIGATION_RESULT_UNSUPPORTED_SCHEME;
	case GNOME_VFS_ERROR_LOGIN_FAILED:
		return NAUTILUS_NAVIGATION_RESULT_LOGIN_FAILED;
	case GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE:	
		return NAUTILUS_NAVIGATION_RESULT_SERVICE_NOT_AVAILABLE;
	case GNOME_VFS_ERROR_ACCESS_DENIED:	
		return NAUTILUS_NAVIGATION_RESULT_ACCESS_DENIED;
        case GNOME_VFS_ERROR_HOST_NOT_FOUND:
                return NAUTILUS_NAVIGATION_RESULT_HOST_NOT_FOUND;
	case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
		return NAUTILUS_NAVIGATION_RESULT_HOST_HAS_NO_ADDRESS;
        case GNOME_VFS_ERROR_GENERIC:
        case GNOME_VFS_ERROR_INTERNAL:
                /* These two have occurred at least once in the web browser component */
                return NAUTILUS_NAVIGATION_RESULT_UNSPECIFIC_ERROR;
        default:
                /* Whenever this message fires, we should consider adding a specific case
                 * to make the error as comprehensible as possible to the user. Please
                 * bug me (sullivan@eazel.com) if you see this fire and don't have the
                 * inclination to immediately make a good message yourself (tell me
                 * what GnomeVFSResult code the message reported, and what caused it to
                 * fire).
                 */
                g_warning ("in nautilus-applicable-views.c, got unhandled GnomeVFSResult %d (%s). If this is a "
                	   "legitimate get_file_info result, please tell sullivan@eazel.com so he can "
                	   "write a decent user-level error message for it.", 
                	   gnome_vfs_result,
                	   gnome_vfs_result_to_string (gnome_vfs_result));
                return NAUTILUS_NAVIGATION_RESULT_UNSPECIFIC_ERROR;
        }
}




static gboolean
got_file_info_callback_common (NautilusFile *file,
                               gpointer data,
                               gboolean final)
{
        GnomeVFSResult vfs_result_code;
        NautilusNavigationInfo *info;
        NautilusNavigationResult result_code;
        NautilusViewIdentifier *default_id;
        OAF_ServerInfo *default_component;
        
        info = (NautilusNavigationInfo *) data;
        
        g_assert (info->file == file);
        result_code = NAUTILUS_NAVIGATION_RESULT_UNDEFINED;
	default_id = NULL;
        info->handle = NULL;
        
        /* Get the result. */
        vfs_result_code = nautilus_file_get_file_info_result (file);
        
        if (vfs_result_code != GNOME_VFS_OK
            && vfs_result_code != GNOME_VFS_ERROR_NOT_SUPPORTED
            && vfs_result_code != GNOME_VFS_ERROR_INVALID_URI) {
                goto out;
        }
        
        default_component = nautilus_mime_get_default_component_for_file (info->file);
        if (default_component != NULL) {
        	default_id = nautilus_view_identifier_new_from_content_view (default_component);
                CORBA_free (default_component);
        }
        
#ifdef DEBUG_MJS
        printf ("XXXXXX - default_id: %s (%s)\n", default_id->iid, default_id->name);
#endif
        
        /* If no components found at all - if there are any, there will be a default. */
        if (default_id != NULL) {
                vfs_result_code = GNOME_VFS_OK;
                result_code = get_nautilus_navigation_result_from_gnome_vfs_result (vfs_result_code);
        } else {
                /* Map GnomeVFSResult to one of the types that Nautilus knows how to handle. */
                if (vfs_result_code == GNOME_VFS_OK && default_id == NULL) {
                	/* If the complete list is non-empty, the default shouldn't have been NULL */
                    	g_assert (!nautilus_mime_has_any_components_for_file (info->file));
                        result_code = NAUTILUS_NAVIGATION_RESULT_NO_HANDLER_FOR_TYPE;
                }

		/* As long as we have any usable id, we can view this location. */
                if (default_id == NULL) {
	                goto out;
                }
        }
               
        g_assert (default_id != NULL);
	info->initial_content_id = nautilus_view_identifier_copy (default_id);
        
 out:
 	if (result_code == NAUTILUS_NAVIGATION_RESULT_UNDEFINED) {
                result_code = get_nautilus_navigation_result_from_gnome_vfs_result (vfs_result_code);
 	}

        (* info->callback) (result_code, info, 
                            final || result_code != NAUTILUS_NAVIGATION_RESULT_OK, 
                            info->callback_data);

        return (result_code == NAUTILUS_NAVIGATION_RESULT_OK);
}


static void
got_full_file_info_callback (NautilusFile *file,
                             gpointer data)
{
        got_file_info_callback_common (file, data, TRUE);
}

static void
got_minimum_file_info_callback (NautilusFile *file,
                                gpointer data)
{
        GList *attributes;
        NautilusNavigationInfo *info;
        
        info = (NautilusNavigationInfo *) data;

        /* We start monitoring files here so we get a single load of
         * the directory instead of multiple ones. The concept is that
         * our load of the directory is shared both with the
         * possible call_when_ready below and with other stuff needed by
         * components.
         */
        nautilus_directory_file_monitor_add (info->directory, info,
                                             TRUE, TRUE, NULL, FALSE);
        
        if (nautilus_mime_actions_file_needs_full_file_attributes (file)) {
                if (got_file_info_callback_common (file, data, FALSE)) {
                        attributes = nautilus_mime_actions_get_full_file_attributes ();
                        nautilus_file_call_when_ready (file, attributes,
                                                       got_full_file_info_callback, data);
                        g_list_free (attributes);

                }
        } else {
                got_file_info_callback_common (file, data, TRUE);
        }
}

        

/* NautilusNavigationInfo */

NautilusNavigationInfo *
nautilus_navigation_info_new (const char *location,
                              NautilusNavigationCallback notify_when_ready,
                              gpointer notify_data)
{
        NautilusNavigationInfo *info;
        GList *attributes;

        info = g_new0 (NautilusNavigationInfo, 1);
        
        info->callback = notify_when_ready;
        info->callback_data = notify_data;
        
        /* Remember the location separately, since nautilus_file_get
         * currently munges locations that have unescaped characters
         * in the "file name" part.
         */
        info->location = g_strdup (location);
        info->file = nautilus_file_get (location);
        info->directory = nautilus_directory_get (location);

        /* Arrange for all the file attributes we will need. */
        attributes = nautilus_mime_actions_get_minimum_file_attributes ();
        nautilus_file_call_when_ready (info->file, attributes,
                                       got_minimum_file_info_callback, info);
        g_list_free (attributes);
        
        return info;
}

void
nautilus_navigation_info_cancel (NautilusNavigationInfo *info)
{
        g_return_if_fail (info != NULL);

        if (info->handle != NULL) {
                gnome_vfs_async_cancel (info->handle);
                info->handle = NULL;
        }

        nautilus_file_cancel_call_when_ready
                (info->file, got_minimum_file_info_callback, info);

        nautilus_file_cancel_call_when_ready
                (info->file, got_full_file_info_callback, info);

        nautilus_directory_file_monitor_remove
                (info->directory, info);
}

void
nautilus_navigation_info_free (NautilusNavigationInfo *info)
{
        g_return_if_fail (info != NULL);
        
        nautilus_navigation_info_cancel (info);

        g_free (info->location);
        nautilus_file_unref (info->file);
        nautilus_directory_unref (info->directory);
        nautilus_view_identifier_free (info->initial_content_id);

        g_free (info);
}

char *
nautilus_navigation_info_get_location (NautilusNavigationInfo *info)
{
        return g_strdup (info->location);
}

NautilusViewIdentifier *
nautilus_navigation_info_get_initial_content_id (NautilusNavigationInfo *info)
{
        return nautilus_view_identifier_copy (info->initial_content_id);
}
