/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-file-operations.c - Nautilus file operations.

   Copyright (C) 1999, 2000 Free Software Foundation
   Copyright (C) 2000, 2001 Eazel, Inc.

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
   
   Authors: Ettore Perazzoli <ettore@gnu.org> 
            Pavel Cisler <pavel@eazel.com> 
 */

#include <config.h>
#include <string.h>
#include "nautilus-file-operations.h"

#include "nautilus-file-operations-progress.h"
#include "nautilus-lib-self-check-functions.h"

#include <eel/eel-glib-extensions.h>
#include <eel/eel-pango-extensions.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-vfs-extensions.h>

#include <gnome.h>
#include <gtk/gtklabel.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-find-directory.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "nautilus-file-changes-queue.h"
#include "nautilus-file-private.h"
#include "nautilus-desktop-icon-file.h"
#include "nautilus-desktop-link-monitor.h"
#include "nautilus-global-preferences.h"
#include "nautilus-link.h"
#include "nautilus-trash-monitor.h"

typedef enum {
	TRANSFER_MOVE,
	TRANSFER_COPY,
	TRANSFER_DUPLICATE,
	TRANSFER_MOVE_TO_TRASH,
	TRANSFER_EMPTY_TRASH,
	TRANSFER_DELETE,
	TRANSFER_LINK
} TransferKind;

/* Copy engine callback state */
typedef struct {
	GnomeVFSAsyncHandle *handle;
	NautilusFileOperationsProgress *progress_dialog;
	const char *operation_title;	/* "Copying files" */
	const char *action_label;	/* "Files copied:" */
	const char *progress_verb;	/* "Copying" */
	const char *preparation_name;	/* "Preparing To Copy..." */
	const char *cleanup_name;	/* "Finishing Move..." */
	GnomeVFSXferErrorMode error_mode;
	GnomeVFSXferOverwriteMode overwrite_mode;
	GtkWidget *parent_view;
	TransferKind kind;
	void (* done_callback) (GHashTable *debuting_uris, gpointer data);
	gpointer done_callback_data;
	GHashTable *debuting_uris;
	gboolean cancelled;	
} TransferInfo;

static TransferInfo *
transfer_info_new (GtkWidget *parent_view)
{
	TransferInfo *result;
	
	result = g_new0 (TransferInfo, 1);
	result->parent_view = parent_view;
	
	eel_add_weak_pointer (&result->parent_view);
	
	return result;
}

static void
transfer_info_destroy (TransferInfo *transfer_info)
{
	eel_remove_weak_pointer (&transfer_info->parent_view);
	
	if (transfer_info->progress_dialog != NULL) {
		nautilus_file_operations_progress_done (transfer_info->progress_dialog);
	}
	
	if (transfer_info->debuting_uris != NULL) {
		g_hash_table_destroy (transfer_info->debuting_uris);
	}
	
	g_free (transfer_info);
}

/* Struct used to control applying icon positions to 
 * top level items during a copy, drag, new folder creation and
 * link creation
 */
typedef struct {
	GdkPoint *icon_positions;
	int last_icon_position_index;
	GList *uris;
	const GList *last_uri;
	int screen;
} IconPositionIterator;

static IconPositionIterator *
icon_position_iterator_new (GArray *icon_positions, const GList *uris,
			    int screen)
{
	IconPositionIterator *result;
	guint index;

	g_assert (icon_positions->len == g_list_length ((GList *)uris));
	result = g_new (IconPositionIterator, 1);
	
	/* make our own copy of the icon locations */
	result->icon_positions = g_new (GdkPoint, icon_positions->len);
	for (index = 0; index < icon_positions->len; index++) {
		result->icon_positions[index] = g_array_index (icon_positions, GdkPoint, index);
	}
	result->last_icon_position_index = 0;

	result->uris = eel_g_str_list_copy ((GList *)uris);
	result->last_uri = result->uris;
	result->screen = screen;

	return result;
}

static void
icon_position_iterator_free (IconPositionIterator *position_iterator)
{
	if (position_iterator == NULL) {
		return;
	}
	
	g_free (position_iterator->icon_positions);
	eel_g_list_free_deep (position_iterator->uris);
	g_free (position_iterator);
}

static gboolean
icon_position_iterator_get_next (IconPositionIterator *position_iterator,
				 const char *next_uri,
				 GdkPoint *point)
{
	if (position_iterator == NULL) {
		return FALSE;
	}
		
	for (;;) {
		if (position_iterator->last_uri == NULL) {
			/* we are done, no more points left */
			return FALSE;
		}

		/* Scan for the next point that matches the source_name
		 * uri.
		 */
		if (strcmp ((const char *) position_iterator->last_uri->data, 
			    next_uri) == 0) {
			break;
		}
		
		/* Didn't match -- a uri must have been skipped by the copy 
		 * engine because of a name conflict. All we need to do is 
		 * skip ahead too.
		 */
		position_iterator->last_uri = position_iterator->last_uri->next;
		position_iterator->last_icon_position_index++; 
	}

	/* apply the location to the target file */
	*point = position_iterator->icon_positions
		[position_iterator->last_icon_position_index];

	/* advance to the next point for next time */
	position_iterator->last_uri = position_iterator->last_uri->next;
	position_iterator->last_icon_position_index++; 

	return TRUE;
}

static char *
ellipsize_string_for_dialog (PangoContext *context, const char *str)
{
	int maximum_width;
	char *result;
	PangoLayout *layout;
	PangoFontMetrics *metrics;

	layout = pango_layout_new (context);

	metrics = pango_context_get_metrics (
		context, pango_context_get_font_description (context), NULL);

	maximum_width = pango_font_metrics_get_approximate_char_width (metrics) * 25 / PANGO_SCALE;

	pango_font_metrics_unref (metrics);

	eel_pango_layout_set_text_ellipsized (
		layout, str, maximum_width, EEL_ELLIPSIZE_MIDDLE);

	result = g_strdup (pango_layout_get_text (layout));

	g_object_unref (layout);

	return result;
}

static char *
format_and_ellipsize_uri_for_dialog (GtkWidget *context, const char *uri)
{
	char *unescaped, *result;

	unescaped = eel_format_uri_for_display (uri);
	result = ellipsize_string_for_dialog (
		gtk_widget_get_pango_context (context), unescaped);
	g_free (unescaped);

	return result;
}

static char *
extract_and_ellipsize_file_name_for_dialog (GtkWidget *context, const char *uri)
{
	char *basename;
	char *unescaped, *result;
	
	basename = g_path_get_basename (uri);
	g_return_val_if_fail (basename != NULL, NULL);

	unescaped = gnome_vfs_unescape_string_for_display (basename);
	result = ellipsize_string_for_dialog (
		gtk_widget_get_pango_context (context), unescaped);
	g_free (unescaped);
	g_free (basename);

	return result;
}

static GtkWidget *
parent_for_error_dialog (TransferInfo *transfer_info)
{
	if (transfer_info->progress_dialog != NULL) {
		return GTK_WIDGET (transfer_info->progress_dialog);
	}

	return transfer_info->parent_view;
}

static void
handle_response_callback (GtkDialog *dialog, int response, TransferInfo *transfer_info)
{
	transfer_info->cancelled = TRUE;
}

static void
handle_close_callback (GtkDialog *dialog, TransferInfo *transfer_info)
{
	transfer_info->cancelled = TRUE;
}

static void
create_transfer_dialog (const GnomeVFSXferProgressInfo *progress_info,
			TransferInfo *transfer_info)
{
	g_return_if_fail (transfer_info->progress_dialog == NULL);

	transfer_info->progress_dialog = nautilus_file_operations_progress_new 
		(transfer_info->operation_title, "", "", "", 0, 0, TRUE);

	/* Treat clicking on the close box or use of the escape key
	 * the same as clicking cancel.
	 */
	g_signal_connect (transfer_info->progress_dialog,
			  "response",
			  G_CALLBACK (handle_response_callback),
			  transfer_info);
	g_signal_connect (transfer_info->progress_dialog,
			  "close",
			  G_CALLBACK (handle_close_callback),
			  transfer_info);

	/* Make the progress dialog show up over the window we are copying into */
	if (transfer_info->parent_view != NULL) {
		GtkWidget *toplevel;

		/* Transient-for-desktop are visible on all desktops, we don't want
		   that. */
		toplevel = gtk_widget_get_toplevel (transfer_info->parent_view);
		if (toplevel != NULL &&
		    g_object_get_data (G_OBJECT (toplevel), "is_desktop_window") == NULL) {
			gtk_window_set_transient_for (GTK_WINDOW (transfer_info->progress_dialog), 
						      GTK_WINDOW (toplevel));
		}
	}
}

static void
progress_dialog_set_to_from_item_text (NautilusFileOperationsProgress *dialog,
				       const char *progress_verb,
				       const char *from_uri, const char *to_uri, 
				       gulong index, gulong size)
{
	char *item;
	char *from_path;
	char *to_path;
	char *progress_label_text;
	const char *from_prefix;
	const char *to_prefix;
	GnomeVFSURI *uri;
	int length;

	item = NULL;
	from_path = NULL;
	to_path = NULL;
	from_prefix = "";
	to_prefix = "";
	progress_label_text = NULL;

	if (from_uri != NULL) {
		uri = gnome_vfs_uri_new (from_uri);
		item = gnome_vfs_uri_extract_short_name (uri);
		from_path = gnome_vfs_uri_extract_dirname (uri);

		/* remove the last '/' */
		length = strlen (from_path);
		if (from_path [length - 1] == '/') {
			from_path [length - 1] = '\0';
		}
		
		gnome_vfs_uri_unref (uri);
		g_assert (progress_verb);
		progress_label_text = g_strdup_printf ("%s:", progress_verb);
		/* "From" dialog label, source path gets placed next to it in the dialog */
		from_prefix = _("From:");
	}

	if (to_uri != NULL) {
		uri = gnome_vfs_uri_new (to_uri);
		to_path = gnome_vfs_uri_extract_dirname (uri);

		/* remove the last '/' */
		length = strlen (to_path);
		if (to_path [length - 1] == '/') {
			to_path [length - 1] = '\0';
		}

		gnome_vfs_uri_unref (uri);
		/* "To" dialog label, source path gets placed next to it in the dialog */
		to_prefix = _("To:");
	}

	nautilus_file_operations_progress_new_file
		(dialog,
		 progress_label_text != NULL ? progress_label_text : "",
		 item != NULL ? item : "",
		 from_path != NULL ? from_path : "",
		 to_path != NULL ? to_path : "",
		 from_prefix, to_prefix, index, size);

	g_free (progress_label_text);
	g_free (item);
	g_free (from_path);
	g_free (to_path);
}

static int
handle_transfer_ok (const GnomeVFSXferProgressInfo *progress_info,
		    TransferInfo *transfer_info)
{
	if (transfer_info->cancelled
		&& progress_info->phase != GNOME_VFS_XFER_PHASE_COMPLETED) {
		/* If cancelled, delete any partially copied files that are laying
		 * around and return. Don't delete the source though..
		 */
		if (progress_info->target_name != NULL
		    && progress_info->source_name != NULL
		    && strcmp (progress_info->source_name, progress_info->target_name) != 0
		    && progress_info->bytes_total != progress_info->bytes_copied) {
			GList *delete_me;

			delete_me = g_list_prepend (NULL, progress_info->target_name);
			nautilus_file_operations_delete (delete_me, transfer_info->parent_view);
			g_list_free (delete_me);
		}

		return 0;
	}
	
	switch (progress_info->phase) {
	case GNOME_VFS_XFER_PHASE_INITIAL:
		create_transfer_dialog (progress_info, transfer_info);
		return 1;

	case GNOME_VFS_XFER_PHASE_COLLECTING:
		if (transfer_info->progress_dialog != NULL) {
			nautilus_file_operations_progress_set_operation_string
				(transfer_info->progress_dialog,
				 transfer_info->preparation_name);
		}
		return 1;

	case GNOME_VFS_XFER_PHASE_READYTOGO:
		if (transfer_info->progress_dialog != NULL) {
			nautilus_file_operations_progress_set_operation_string
				(transfer_info->progress_dialog,
				 transfer_info->action_label);
			nautilus_file_operations_progress_set_total
				(transfer_info->progress_dialog,
				 progress_info->files_total,
				 progress_info->bytes_total);
		}
		return 1;
				 
	case GNOME_VFS_XFER_PHASE_DELETESOURCE:
		nautilus_file_changes_consume_changes (FALSE);
		if (transfer_info->progress_dialog != NULL) {
			progress_dialog_set_to_from_item_text
				(transfer_info->progress_dialog,
				 transfer_info->progress_verb,
				 progress_info->source_name,
				 NULL,
				 progress_info->file_index,
				 progress_info->file_size);

			nautilus_file_operations_progress_update_sizes
				(transfer_info->progress_dialog,
				 MIN (progress_info->bytes_copied, 
				      progress_info->bytes_total),
				 MIN (progress_info->total_bytes_copied,
				      progress_info->bytes_total));
		}
		return 1;

	case GNOME_VFS_XFER_PHASE_MOVING:
	case GNOME_VFS_XFER_PHASE_OPENSOURCE:
	case GNOME_VFS_XFER_PHASE_OPENTARGET:
		/* fall through */
	case GNOME_VFS_XFER_PHASE_COPYING:
		if (transfer_info->progress_dialog != NULL) {
			if (progress_info->bytes_copied == 0) {
				progress_dialog_set_to_from_item_text
					(transfer_info->progress_dialog,
					 transfer_info->progress_verb,
					 progress_info->source_name,
					 progress_info->target_name,
					 progress_info->file_index,
					 progress_info->file_size);
			} else {
				nautilus_file_operations_progress_update_sizes
					(transfer_info->progress_dialog,
					 MIN (progress_info->bytes_copied, 
					      progress_info->bytes_total),
					 MIN (progress_info->total_bytes_copied,
					      progress_info->bytes_total));
			}
		}
		return 1;

	case GNOME_VFS_XFER_PHASE_CLEANUP:
		if (transfer_info->progress_dialog != NULL) {
			nautilus_file_operations_progress_clear
				(transfer_info->progress_dialog);
			nautilus_file_operations_progress_set_operation_string
				(transfer_info->progress_dialog,
				 transfer_info->cleanup_name);
		}
		return 1;

	case GNOME_VFS_XFER_PHASE_COMPLETED:
		nautilus_file_changes_consume_changes (TRUE);
		if (transfer_info->done_callback != NULL) {
			transfer_info->done_callback (transfer_info->debuting_uris,
						      transfer_info->done_callback_data);
			/* done_callback now owns (will free) debuting_uris */
			transfer_info->debuting_uris = NULL;
		}

		transfer_info_destroy (transfer_info);
		return 1;

	default:
		return 1;
	}
}

typedef enum {
	ERROR_READ_ONLY,
	ERROR_NOT_READABLE,
	ERROR_NOT_WRITABLE,
	ERROR_NOT_ENOUGH_PERMISSIONS,
	ERROR_NO_SPACE,
	ERROR_SOURCE_IN_TARGET,
	ERROR_OTHER
} NautilusFileOperationsErrorKind;

typedef enum {
	ERROR_LOCATION_UNKNOWN,
	ERROR_LOCATION_SOURCE,
	ERROR_LOCATION_SOURCE_PARENT,
	ERROR_LOCATION_SOURCE_OR_PARENT,
	ERROR_LOCATION_TARGET
} NautilusFileOperationsErrorLocation;


static void
build_error_string (const char *source_name, const char *target_name,
		    TransferKind operation_kind,
		    NautilusFileOperationsErrorKind error_kind,
		    NautilusFileOperationsErrorLocation error_location,
		    GnomeVFSResult error,
		    char **error_string, char **detail_string)
{
	/* Avoid clever message composing here, just use brute force and
	 * duplicate the different flavors of error messages for all the
	 * possible permutations.
	 * That way localizers have an easier time and can even rearrange the
	 * order of the words in the messages easily.
	 */

	char *error_format;
	char *detail_format;
	
	error_format = NULL;
	detail_format = NULL;

	*error_string = NULL;
	*detail_string = NULL;

	if (error_location == ERROR_LOCATION_SOURCE_PARENT) {

		switch (operation_kind) {
		case TRANSFER_MOVE:
		case TRANSFER_MOVE_TO_TRASH:
			if (error_kind == ERROR_READ_ONLY) {
				*error_string = g_strdup (_("Error while moving."));
				detail_format = _("\"%s\" cannot be moved because it is on "
						  "a read-only disk.");
			}
			break;

		case TRANSFER_DELETE:
		case TRANSFER_EMPTY_TRASH:
			switch (error_kind) {
			case ERROR_NOT_ENOUGH_PERMISSIONS:
			case ERROR_NOT_WRITABLE:
				*error_string = g_strdup (_("Error while deleting."));
				detail_format = _("\"%s\" cannot be deleted because you do not have "
						  "permissions to modify its parent folder.");
				break;
			
			case ERROR_READ_ONLY:
				*error_string = g_strdup (_("Error while deleting."));
				detail_format = _("\"%s\" cannot be deleted because it is on "
						  "a read-only disk.");
				break;

			default:
				break;
			}
			break;

		default:
			g_assert_not_reached ();
			break;
		}
		
		if (detail_format != NULL && source_name != NULL) {
			*detail_string = g_strdup_printf (detail_format, source_name);
		}

	} else if (error_location == ERROR_LOCATION_SOURCE_OR_PARENT) {

		g_assert (source_name != NULL);

		/* FIXME: Would be better if we could distinguish source vs parent permissions
		 * better somehow. The GnomeVFS copy engine would have to do some snooping
		 * after the failure in this case.
		 */
		switch (operation_kind) {
		case TRANSFER_MOVE:
			switch (error_kind) {
			case ERROR_NOT_ENOUGH_PERMISSIONS:
				*error_string = g_strdup (_("Error while moving."));
				detail_format = _("\"%s\" cannot be moved because you do not have "
						  "permissions to change it or its parent folder.");
				break;
			case ERROR_SOURCE_IN_TARGET:
				*error_string = g_strdup (_("Error while moving."));
				detail_format = _("Cannot move \"%s\" because it or its parent folder "
						  "are contained in the destination.");
				break;
			default:
				break;
			}
			break;
		case TRANSFER_MOVE_TO_TRASH:
			if (error_kind == ERROR_NOT_ENOUGH_PERMISSIONS) {
				*error_string = g_strdup (_("Error while moving."));
				detail_format = _("Cannot move \"%s\" to the trash because you do not have "
						  "permissions to change it or its parent folder.");
			}
			break;

		default:
			g_assert_not_reached ();
			break;
		}

		if (detail_format != NULL && source_name != NULL) {
			*detail_string = g_strdup_printf (detail_format, source_name);
		}

	} else if (error_location == ERROR_LOCATION_SOURCE) {

		g_assert (source_name != NULL);

		switch (operation_kind) {
		case TRANSFER_COPY:
		case TRANSFER_DUPLICATE:
			if (error_kind == ERROR_NOT_READABLE) {
				*error_string = g_strdup (_("Error while copying."));
				detail_format = _("\"%s\" cannot be copied because you do not have "
						  "permissions to read it.");
			}
			break;

		default:
			g_assert_not_reached ();
			break;
		}

		if (detail_format != NULL && source_name != NULL) {
			*detail_string = g_strdup_printf (detail_format, source_name);
		}

	} else if (error_location == ERROR_LOCATION_TARGET) {

		if (error_kind == ERROR_NO_SPACE) {
			switch (operation_kind) {
			case TRANSFER_COPY:
			case TRANSFER_DUPLICATE:
				error_format = _("Error while copying to \"%s\".");
				*detail_string = g_strdup (_("There is not enough space on the destination."));
				break;
			case TRANSFER_MOVE_TO_TRASH:
			case TRANSFER_MOVE:
				error_format = _("Error while moving to \"%s\".");
				*detail_string = g_strdup (_("There is not enough space on the destination."));
				break;
			case TRANSFER_LINK:
				error_format = _("Error while creating link in \"%s\".");
				*detail_string = g_strdup (_("There is not enough space on the destination."));
				break;
			default:
				g_assert_not_reached ();
				break;
			}
		} else {
			switch (operation_kind) {
			case TRANSFER_COPY:
			case TRANSFER_DUPLICATE:
				if (error_kind == ERROR_NOT_ENOUGH_PERMISSIONS) {
					error_format = _("Error while copying to \"%s\".");
					*detail_string = g_strdup (_("You do not have permissions to write to "
					   		            "this folder."));
				} else if (error_kind == ERROR_NOT_WRITABLE) {
					error_format = _("Error while copying to \"%s\".");
					*detail_string = g_strdup (_("The destination disk is read-only."));
				} 
				break;
			case TRANSFER_MOVE:
			case TRANSFER_MOVE_TO_TRASH:
				if (error_kind == ERROR_NOT_ENOUGH_PERMISSIONS) {
					error_format = _("Error while moving items to \"%s\".");
					*detail_string = g_strdup (_("You do not have permissions to write to "
					   		            "this folder."));
				} else if (error_kind == ERROR_NOT_WRITABLE) {
					error_format = _("Error while moving items to \"%s\".");
					*detail_string = g_strdup (_("The destination disk is read-only."));
				} 

				break;
			case TRANSFER_LINK:
				if (error_kind == ERROR_NOT_ENOUGH_PERMISSIONS) {
					error_format = _("Error while creating links in \"%s\".");
					*detail_string = g_strdup (_("You do not have permissions to write to "
					   		             "this folder."));
				} else if (error_kind == ERROR_NOT_WRITABLE) {
					error_format = _("Error while creating links in \"%s\".");
					*detail_string = g_strdup (_("The destination disk is read-only."));
				} 
				break;
			default:
				g_assert_not_reached ();
				break;
			}
		}
		if (error_format != NULL && target_name != NULL) {
			*error_string = g_strdup_printf (error_format, target_name);
		}
	}
	
	if (*error_string == NULL) {
		/* None of the specific error messages apply, use a catch-all
		 * generic error
		 */
		g_message ("Hit unexpected error \"%s\" while doing a file operation.",
			   gnome_vfs_result_to_string (error));

		/* FIXMEs: we need to consider a single item
		 * move/copy and not offer to continue in that case
		 */
		if (source_name != NULL) {
			switch (operation_kind) {
			case TRANSFER_COPY:
			case TRANSFER_DUPLICATE:
				error_format = _("Error \"%s\" while copying \"%s\".");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			case TRANSFER_MOVE:
				error_format = _("Error \"%s\" while moving \"%s\".");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			case TRANSFER_LINK:
				error_format = _("Error \"%s\" while creating a link to \"%s\".");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			case TRANSFER_DELETE:
			case TRANSFER_EMPTY_TRASH:
			case TRANSFER_MOVE_TO_TRASH:
				error_format = _("Error \"%s\" while deleting \"%s\".");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			default:
				g_assert_not_reached ();
				break;
			}
	
			*error_string = g_strdup_printf (error_format, 
							 gnome_vfs_result_to_string (error),
							 source_name);
		} else {
			switch (operation_kind) {
			case TRANSFER_COPY:
			case TRANSFER_DUPLICATE:
				error_format = _("Error \"%s\" while copying.");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			case TRANSFER_MOVE:
				error_format = _("Error \"%s\" while moving.");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			case TRANSFER_LINK:
				error_format = _("Error \"%s\" while linking.");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			case TRANSFER_DELETE:
			case TRANSFER_EMPTY_TRASH:
			case TRANSFER_MOVE_TO_TRASH:
				error_format = _("Error \"%s\" while deleting.");
				*detail_string = g_strdup (_("Would you like to continue?"));
				break;
			default:
				g_assert_not_reached ();
				break;
			}
	
			*error_string = g_strdup_printf (error_format, 
						         gnome_vfs_result_to_string (error));
		}
	}
}

static int
handle_transfer_vfs_error (const GnomeVFSXferProgressInfo *progress_info,
			   TransferInfo *transfer_info)
{
	/* Notice that the error mode in `transfer_info' is the one we have been
         * requested, but the transfer is always performed in mode
         * `GNOME_VFS_XFER_ERROR_MODE_QUERY'.
         */

	int error_dialog_button_pressed;
	int error_dialog_result;
	char *text;
	char *detail;
	char *formatted_source_name;
	char *formatted_target_name;
	const char *dialog_title;
	NautilusFileOperationsErrorKind error_kind;
	NautilusFileOperationsErrorLocation error_location;
	
	switch (transfer_info->error_mode) {
	case GNOME_VFS_XFER_ERROR_MODE_QUERY:

		/* transfer error, prompt the user to continue or cancel */

		/* stop timeout while waiting for user */
		nautilus_file_operations_progress_pause_timeout (transfer_info->progress_dialog);

		formatted_source_name = NULL;
		formatted_target_name = NULL;

		if (progress_info->source_name != NULL) {
			formatted_source_name = format_and_ellipsize_uri_for_dialog
				(parent_for_error_dialog (transfer_info),
				 progress_info->source_name);
		}

		if (progress_info->target_name != NULL) {
			formatted_target_name = format_and_ellipsize_uri_for_dialog
				(parent_for_error_dialog (transfer_info),
				 progress_info->target_name);
		}

		error_kind = ERROR_OTHER;
		error_location = ERROR_LOCATION_UNKNOWN;
		
		/* Single out a few common error conditions for which we have
		 * custom-taylored error messages.
		 */
		if ((progress_info->vfs_status == GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM
				|| progress_info->vfs_status == GNOME_VFS_ERROR_READ_ONLY)
			&& (transfer_info->kind == TRANSFER_DELETE
				|| transfer_info->kind == TRANSFER_EMPTY_TRASH)) {
			error_location = ERROR_LOCATION_SOURCE_PARENT;
			error_kind = ERROR_READ_ONLY;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED
			&& (transfer_info->kind == TRANSFER_DELETE
				|| transfer_info->kind == TRANSFER_EMPTY_TRASH)) {
			error_location = ERROR_LOCATION_SOURCE_PARENT;
			error_kind = ERROR_NOT_ENOUGH_PERMISSIONS;
		} else if ((progress_info->vfs_status == GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM
				|| progress_info->vfs_status == GNOME_VFS_ERROR_READ_ONLY)
			&& (transfer_info->kind == TRANSFER_MOVE
				|| transfer_info->kind == TRANSFER_MOVE_TO_TRASH)
			&& progress_info->phase != GNOME_VFS_XFER_CHECKING_DESTINATION) {
			error_location = ERROR_LOCATION_SOURCE_PARENT;
			error_kind = ERROR_READ_ONLY;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED
			&& transfer_info->kind == TRANSFER_MOVE
			&& progress_info->phase == GNOME_VFS_XFER_PHASE_OPENTARGET) {
			error_location = ERROR_LOCATION_TARGET;
			error_kind = ERROR_NOT_ENOUGH_PERMISSIONS;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED
			&& (transfer_info->kind == TRANSFER_MOVE
				|| transfer_info->kind == TRANSFER_MOVE_TO_TRASH)
			&& progress_info->phase != GNOME_VFS_XFER_CHECKING_DESTINATION) {
			error_location = ERROR_LOCATION_SOURCE_OR_PARENT;
			error_kind = ERROR_NOT_ENOUGH_PERMISSIONS;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED
			&& (transfer_info->kind == TRANSFER_COPY
				|| transfer_info->kind == TRANSFER_DUPLICATE)
			&& (progress_info->phase == GNOME_VFS_XFER_PHASE_OPENSOURCE
				|| progress_info->phase == GNOME_VFS_XFER_PHASE_COLLECTING
				|| progress_info->phase == GNOME_VFS_XFER_PHASE_INITIAL)) {
			error_location = ERROR_LOCATION_SOURCE;
			error_kind = ERROR_NOT_READABLE;
		} else if ((progress_info->vfs_status == GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM
				|| progress_info->vfs_status == GNOME_VFS_ERROR_READ_ONLY)
			&& progress_info->phase == GNOME_VFS_XFER_CHECKING_DESTINATION) {
			error_location = ERROR_LOCATION_TARGET;
			error_kind = ERROR_NOT_WRITABLE;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED
			&& progress_info->phase == GNOME_VFS_XFER_CHECKING_DESTINATION) {
			error_location = ERROR_LOCATION_TARGET;
			error_kind = ERROR_NOT_ENOUGH_PERMISSIONS;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_NO_SPACE) {
			error_location = ERROR_LOCATION_TARGET;
			error_kind = ERROR_NO_SPACE;
		} else if (progress_info->vfs_status == GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY
			   && transfer_info->kind == TRANSFER_MOVE) {
			error_location = ERROR_LOCATION_SOURCE_OR_PARENT;
			error_kind = ERROR_SOURCE_IN_TARGET;
		}

		build_error_string (formatted_source_name, formatted_target_name,
				    transfer_info->kind,
				    error_kind, error_location,
				    progress_info->vfs_status,
				    &text, &detail);

		switch (transfer_info->kind) {
		case TRANSFER_COPY:
		case TRANSFER_DUPLICATE:
			dialog_title = _("Error While Copying");
			break;
		case TRANSFER_MOVE:
			dialog_title = _("Error While Moving");
			break;
		case TRANSFER_LINK:
			dialog_title = _("Error While Linking");
			break;
		case TRANSFER_DELETE:
		case TRANSFER_EMPTY_TRASH:
		case TRANSFER_MOVE_TO_TRASH:
			dialog_title = _("Error While Deleting");
			break;
		default:
			dialog_title = NULL;
			break;
		}

		if (error_location == ERROR_LOCATION_TARGET ||
		    error_kind == ERROR_SOURCE_IN_TARGET) {
			/* We can't continue, just tell the user. */
			eel_run_simple_dialog (parent_for_error_dialog (transfer_info),
				TRUE, GTK_MESSAGE_ERROR, text, detail, dialog_title, GTK_STOCK_OK, NULL);
			error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_ABORT;

		} else if ((error_location == ERROR_LOCATION_SOURCE
				|| error_location == ERROR_LOCATION_SOURCE_PARENT
				|| error_location == ERROR_LOCATION_SOURCE_OR_PARENT)
			&& (error_kind == ERROR_NOT_ENOUGH_PERMISSIONS
				|| error_kind == ERROR_NOT_READABLE)) {
			/* The error could have happened on any of the files
			 * in the moved/copied/deleted hierarchy, we can probably
			 * continue. Allow the user to skip.
			 */
			error_dialog_button_pressed = eel_run_simple_dialog
				(parent_for_error_dialog (transfer_info), TRUE, 
				 GTK_MESSAGE_ERROR, text, 
				 detail, dialog_title,
				 GTK_STOCK_CANCEL, _("_Skip"), NULL);
				 
			switch (error_dialog_button_pressed) {
			case 0:
			case GTK_RESPONSE_DELETE_EVENT:
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_ABORT;
				break;
			case 1:
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_SKIP;
				break;
			default:
				g_assert_not_reached ();
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_ABORT;
			}
								
		} else {
			/* Generic error, offer to retry and skip. */
			error_dialog_button_pressed = eel_run_simple_dialog
				(parent_for_error_dialog (transfer_info), TRUE, 
				 GTK_MESSAGE_ERROR, text, 
				 detail, dialog_title,
				 _("_Skip"), GTK_STOCK_CANCEL, _("_Retry"), NULL);

			switch (error_dialog_button_pressed) {
			case 0:
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_SKIP;
				break;
			case 1:
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_ABORT;
				break;
			case 2:
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_RETRY;
				break;
			default:
				g_assert_not_reached ();
				error_dialog_result = GNOME_VFS_XFER_ERROR_ACTION_ABORT;
			}
		}
			
		g_free (text);
		g_free (detail);
		g_free (formatted_source_name);
		g_free (formatted_target_name);

		nautilus_file_operations_progress_resume_timeout (transfer_info->progress_dialog);

		return error_dialog_result;

	case GNOME_VFS_XFER_ERROR_MODE_ABORT:
	default:
		if (transfer_info->progress_dialog != NULL) {
			nautilus_file_operations_progress_done
				(transfer_info->progress_dialog);
		}
		return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
	}
}

/* is_special_link
 *
 * Check and see if file is one of our special links.
 * A special link ould be one of the following:
 * 	trash, home, volume
 */
static gboolean
is_special_link (const char *uri)
{

	return eel_uri_is_desktop (uri);
}

static gboolean
is_directory (const char *uri)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	gboolean is_dir;

	is_dir = FALSE;
	
	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (uri, info, GNOME_VFS_FILE_INFO_DEFAULT);

	if (result == GNOME_VFS_OK &&
	    info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) {
		is_dir = (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);
	}
	
	gnome_vfs_file_info_unref (info);

	return is_dir;
}


static int
handle_transfer_overwrite (const GnomeVFSXferProgressInfo *progress_info,
		           TransferInfo *transfer_info)
{
	int result;
	char *text, *primary_text, *secondary_text, *formatted_name;
	gboolean is_merge, target_is_dir;

	nautilus_file_operations_progress_pause_timeout (transfer_info->progress_dialog);	

	/* Handle special case files such as Trash, mount links and home directory */	
	if (is_special_link (progress_info->target_name)) {
		formatted_name = extract_and_ellipsize_file_name_for_dialog
			(parent_for_error_dialog (transfer_info),
			 progress_info->target_name);
		
		if (transfer_info->kind == TRANSFER_MOVE) {
			primary_text = g_strdup_printf (_("Could not move \"%s\" to the new location."),
			                                formatted_name);
						
			secondary_text = _("The name is already used for a special item that "
					   "cannot be removed or replaced.  If you still want "
					   "to move the item, rename it and try again.");
		} else {						
			primary_text = g_strdup_printf (_("Could not copy \"%s\" to the new location."),
			                                formatted_name);
						
			secondary_text = _("The name is already used for a special item that "
					   "cannot be removed or replaced.  If you still want "
					   "to copy the item, rename it and try again.");
		}
		
		eel_run_simple_dialog (parent_for_error_dialog (transfer_info), TRUE, GTK_MESSAGE_ERROR, primary_text, secondary_text,
				       _("Unable to Replace File"), GTK_STOCK_OK, NULL, NULL);

		g_free (primary_text);
		g_free (formatted_name);

		nautilus_file_operations_progress_resume_timeout (transfer_info->progress_dialog);

		return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP;
	}
	
	/* transfer conflict, prompt the user to replace or skip */
	formatted_name = format_and_ellipsize_uri_for_dialog (
		parent_for_error_dialog (transfer_info), progress_info->target_name);

	target_is_dir = is_directory (progress_info->target_name);
	if (target_is_dir) {
		text = g_strdup_printf (_("The folder \"%s\" already exists.  Would you like to replace it?"), 
					formatted_name);
	} else {
		text = g_strdup_printf (_("The file \"%s\" already exists.  Would you like to replace it?"), 
					formatted_name);
	}
	g_free (formatted_name);

	is_merge =  target_is_dir && is_directory (progress_info->source_name);

	if (is_merge) {
		secondary_text = _("If you replace the existing folder, any files in it that conflicts with the files being copied will be overwritten.");
	} else {
		secondary_text = _("If you replace an existing file, its contents will be overwritten.");
	}
	
	if (progress_info->duplicate_count == 1) {
		/* we are going to only get one duplicate alert, don't offer
		 * Replace All
		 */
		result = eel_run_simple_dialog 
			(parent_for_error_dialog (transfer_info),
			 TRUE,
			 GTK_MESSAGE_QUESTION, 
			 text, 
			 secondary_text, 
			 _("Conflict While Copying"),
			 _("_Skip"), _("_Replace"), NULL);
		g_free (text);	 

		nautilus_file_operations_progress_resume_timeout (transfer_info->progress_dialog);
					 
		switch (result) {
		case 0:
			return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP;
		default:
			g_assert_not_reached ();
			/* fall through */
		case 1:
			return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE;
		}
	} else {
		result = eel_run_simple_dialog
			(parent_for_error_dialog (transfer_info), TRUE, GTK_MESSAGE_QUESTION, text, 
			 secondary_text, 
			 _("Conflict While Copying"),
			 _("Replace _All"), _("_Skip"), _("_Replace"), NULL);
		g_free (text);

		nautilus_file_operations_progress_resume_timeout (transfer_info->progress_dialog);

		switch (result) {
		case 0:
			return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL;
		case 1:
			return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP;
		default:
			g_assert_not_reached ();
			/* fall through */
		case 2:
			return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE;
		}
	}
}

/* Note that we have these two separate functions with separate format
 * strings for ease of localization.
 */

static char *
get_link_name (char *name, int count) 
{
	char *result;
	char *unescaped_name;
	char *unescaped_tmp_name;
	char *unescaped_result;
	char *new_file;

	const char *format;
	
	g_assert (name != NULL);

	unescaped_tmp_name = gnome_vfs_unescape_string (name, "/");
	g_free (name);

	unescaped_name = g_filename_to_utf8 (unescaped_tmp_name, -1,
					     NULL, NULL, NULL);
	g_free (unescaped_tmp_name);

	if (count < 1) {
		g_warning ("bad count in get_link_name");
		count = 1;
	}

	if (count <= 2) {
		/* Handle special cases for low numbers.
		 * Perhaps for some locales we will need to add more.
		 */
		switch (count) {
		default:
			g_assert_not_reached ();
			/* fall through */
		case 1:
			/* appended to new link file */
			format = _("link to %s");
			break;
		case 2:
			/* appended to new link file */
			format = _("another link to %s");
			break;
		}
		unescaped_result = g_strdup_printf (format, unescaped_name);

	} else {
		/* Handle special cases for the first few numbers of each ten.
		 * For locales where getting this exactly right is difficult,
		 * these can just be made all the same as the general case below.
		 */
		switch (count % 10) {
		case 1:
			/* Localizers: Feel free to leave out the "st" suffix
			 * if there's no way to do that nicely for a
			 * particular language.
			 */
			format = _("%dst link to %s");
			break;
		case 2:
			/* appended to new link file */
			format = _("%dnd link to %s");
			break;
		case 3:
			/* appended to new link file */
			format = _("%drd link to %s");
			break;
		default:
			/* appended to new link file */
			format = _("%dth link to %s");
			break;
		}
		unescaped_result = g_strdup_printf (format, count, unescaped_name);
	}
	new_file = g_filename_from_utf8 (unescaped_result, -1, NULL, NULL, NULL);
	result = gnome_vfs_escape_path_string (new_file);
	
	g_free (unescaped_name);
	g_free (unescaped_result);
	g_free (new_file);

	return result;
}

/* Localizers: 
 * Feel free to leave out the st, nd, rd and th suffix or
 * make some or all of them match.
 */

/* localizers: tag used to detect the first copy of a file */
static const char untranslated_copy_duplicate_tag[] = N_(" (copy)");
/* localizers: tag used to detect the second copy of a file */
static const char untranslated_another_copy_duplicate_tag[] = N_(" (another copy)");

/* localizers: tag used to detect the x11th copy of a file */
static const char untranslated_x11th_copy_duplicate_tag[] = N_("th copy)");
/* localizers: tag used to detect the x12th copy of a file */
static const char untranslated_x12th_copy_duplicate_tag[] = N_("th copy)");
/* localizers: tag used to detect the x13th copy of a file */
static const char untranslated_x13th_copy_duplicate_tag[] = N_("th copy)");

/* localizers: tag used to detect the x1st copy of a file */
static const char untranslated_st_copy_duplicate_tag[] = N_("st copy)");
/* localizers: tag used to detect the x2nd copy of a file */
static const char untranslated_nd_copy_duplicate_tag[] = N_("nd copy)");
/* localizers: tag used to detect the x3rd copy of a file */
static const char untranslated_rd_copy_duplicate_tag[] = N_("rd copy)");

/* localizers: tag used to detect the xxth copy of a file */
static const char untranslated_th_copy_duplicate_tag[] = N_("th copy)");

#define COPY_DUPLICATE_TAG _(untranslated_copy_duplicate_tag)
#define ANOTHER_COPY_DUPLICATE_TAG _(untranslated_another_copy_duplicate_tag)
#define X11TH_COPY_DUPLICATE_TAG _(untranslated_x11th_copy_duplicate_tag)
#define X12TH_COPY_DUPLICATE_TAG _(untranslated_x12th_copy_duplicate_tag)
#define X13TH_COPY_DUPLICATE_TAG _(untranslated_x13th_copy_duplicate_tag)

#define ST_COPY_DUPLICATE_TAG _(untranslated_st_copy_duplicate_tag)
#define ND_COPY_DUPLICATE_TAG _(untranslated_nd_copy_duplicate_tag)
#define RD_COPY_DUPLICATE_TAG _(untranslated_rd_copy_duplicate_tag)
#define TH_COPY_DUPLICATE_TAG _(untranslated_th_copy_duplicate_tag)

/* localizers: appended to first file copy */
static const char untranslated_first_copy_duplicate_format[] = N_("%s (copy)%s");
/* localizers: appended to second file copy */
static const char untranslated_second_copy_duplicate_format[] = N_("%s (another copy)%s");

/* localizers: appended to x11th file copy */
static const char untranslated_x11th_copy_duplicate_format[] = N_("%s (%dth copy)%s");
/* localizers: appended to x12th file copy */
static const char untranslated_x12th_copy_duplicate_format[] = N_("%s (%dth copy)%s");
/* localizers: appended to x13th file copy */
static const char untranslated_x13th_copy_duplicate_format[] = N_("%s (%dth copy)%s");

/* localizers: appended to x1st file copy */
static const char untranslated_st_copy_duplicate_format[] = N_("%s (%dst copy)%s");
/* localizers: appended to x2nd file copy */
static const char untranslated_nd_copy_duplicate_format[] = N_("%s (%dnd copy)%s");
/* localizers: appended to x3rd file copy */
static const char untranslated_rd_copy_duplicate_format[] = N_("%s (%drd copy)%s");
/* localizers: appended to xxth file copy */
static const char untranslated_th_copy_duplicate_format[] = N_("%s (%dth copy)%s");

#define FIRST_COPY_DUPLICATE_FORMAT _(untranslated_first_copy_duplicate_format)
#define SECOND_COPY_DUPLICATE_FORMAT _(untranslated_second_copy_duplicate_format)
#define X11TH_COPY_DUPLICATE_FORMAT _(untranslated_x11th_copy_duplicate_format)
#define X12TH_COPY_DUPLICATE_FORMAT _(untranslated_x12th_copy_duplicate_format)
#define X13TH_COPY_DUPLICATE_FORMAT _(untranslated_x13th_copy_duplicate_format)

#define ST_COPY_DUPLICATE_FORMAT _(untranslated_st_copy_duplicate_format)
#define ND_COPY_DUPLICATE_FORMAT _(untranslated_nd_copy_duplicate_format)
#define RD_COPY_DUPLICATE_FORMAT _(untranslated_rd_copy_duplicate_format)
#define TH_COPY_DUPLICATE_FORMAT _(untranslated_th_copy_duplicate_format)

static char *
extract_string_until (const char *original, const char *until_substring)
{
	char *result;
	
	g_assert ((int) strlen (original) >= until_substring - original);
	g_assert (until_substring - original >= 0);

	result = g_malloc (until_substring - original + 1);
	strncpy (result, original, until_substring - original);
	result[until_substring - original] = '\0';
	
	return result;
}

/* Dismantle a file name, separating the base name, the file suffix and removing any
 * (xxxcopy), etc. string. Figure out the count that corresponds to the given
 * (xxxcopy) substring.
 */
static void
parse_previous_duplicate_name (const char *name,
			       char **name_base,
			       const char **suffix,
			       int *count)
{
	const char *tag;

	g_assert (name[0] != '\0');
	
	*suffix = strrchr (name + 1, '.');
	if (*suffix == NULL || (*suffix)[1] == '\0') {
		/* no suffix */
		*suffix = "";
	}

	tag = strstr (name, COPY_DUPLICATE_TAG);
	if (tag != NULL) {
		if (tag > *suffix) {
			/* handle case "foo. (copy)" */
			*suffix = "";
		}
		*name_base = extract_string_until (name, tag);
		*count = 1;
		return;
	}


	tag = strstr (name, ANOTHER_COPY_DUPLICATE_TAG);
	if (tag != NULL) {
		if (tag > *suffix) {
			/* handle case "foo. (another copy)" */
			*suffix = "";
		}
		*name_base = extract_string_until (name, tag);
		*count = 2;
		return;
	}


	/* Check to see if we got one of st, nd, rd, th. */
	tag = strstr (name, X11TH_COPY_DUPLICATE_TAG);

	if (tag == NULL) {
		tag = strstr (name, X12TH_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, X13TH_COPY_DUPLICATE_TAG);
	}

	if (tag == NULL) {
		tag = strstr (name, ST_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, ND_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, RD_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, TH_COPY_DUPLICATE_TAG);
	}

	/* If we got one of st, nd, rd, th, fish out the duplicate number. */
	if (tag != NULL) {
		/* localizers: opening parentheses to match the "th copy)" string */
		tag = strstr (name, _(" ("));
		if (tag != NULL) {
			if (tag > *suffix) {
				/* handle case "foo. (22nd copy)" */
				*suffix = "";
			}
			*name_base = extract_string_until (name, tag);
			/* localizers: opening parentheses of the "th copy)" string */
			if (sscanf (tag, _(" (%d"), count) == 1) {
				if (*count < 1 || *count > 1000000) {
					/* keep the count within a reasonable range */
					*count = 0;
				}
				return;
			}
			*count = 0;
			return;
		}
	}

	
	*count = 0;
	if (**suffix != '\0') {
		*name_base = extract_string_until (name, *suffix);
	} else {
		*name_base = g_strdup (name);
	}
}

static char *
make_next_duplicate_name (const char *base, const char *suffix, int count)
{
	const char *format;
	char *result;


	if (count < 1) {
		g_warning ("bad count %d in get_duplicate_name", count);
		count = 1;
	}

	if (count <= 2) {

		/* Handle special cases for low numbers.
		 * Perhaps for some locales we will need to add more.
		 */
		switch (count) {
		default:
			g_assert_not_reached ();
			/* fall through */
		case 1:
			format = FIRST_COPY_DUPLICATE_FORMAT;
			break;
		case 2:
			format = SECOND_COPY_DUPLICATE_FORMAT;
			break;

		}
		result = g_strdup_printf (format, base, suffix);
	} else {

		/* Handle special cases for the first few numbers of each ten.
		 * For locales where getting this exactly right is difficult,
		 * these can just be made all the same as the general case below.
		 */

		/* Handle special cases for x11th - x20th.
		 */
		switch (count % 100) {
		case 11:
			format = X11TH_COPY_DUPLICATE_FORMAT;
			break;
		case 12:
			format = X12TH_COPY_DUPLICATE_FORMAT;
			break;
		case 13:
			format = X13TH_COPY_DUPLICATE_FORMAT;
			break;
		default:
			format = NULL;
			break;
		}

		if (format == NULL) {
			switch (count % 10) {
			case 1:
				format = ST_COPY_DUPLICATE_FORMAT;
				break;
			case 2:
				format = ND_COPY_DUPLICATE_FORMAT;
				break;
			case 3:
				format = RD_COPY_DUPLICATE_FORMAT;
				break;
			default:
				/* The general case. */
				format = TH_COPY_DUPLICATE_FORMAT;
				break;
			}
		}

		result = g_strdup_printf (format, base, count, suffix);
	}

	return result;
}

static char *
get_duplicate_name (const char *name, int count_increment)
{
	char *result;
	char *name_base;
	const char *suffix;
	int count;

	parse_previous_duplicate_name (name, &name_base, &suffix, &count);
	result = make_next_duplicate_name (name_base, suffix, count + count_increment);

	g_free (name_base);

	return result;
}

static char *
get_next_duplicate_name (char *name, int count_increment)
{
	char *unescaped_name;
	char *unescaped_tmp_name;
	char *unescaped_result;
	char *result;
	char *new_file;

	unescaped_tmp_name = gnome_vfs_unescape_string (name, "/");
	g_free (name);

	unescaped_name = g_filename_to_utf8 (unescaped_tmp_name, -1,
					     NULL, NULL, NULL);
	if (!unescaped_name) {
		/* Couldn't convert to utf8 - probably
		 * G_BROKEN_FILENAMES not set when it should be.
		 * Try converting from the locale */
		unescaped_name = g_locale_to_utf8 (unescaped_tmp_name, -1, NULL, NULL, NULL);	

		if (!unescaped_name) {
			unescaped_name = eel_make_valid_utf8 (unescaped_tmp_name);
		}
	}
		
	g_free (unescaped_tmp_name);
	
	unescaped_result = get_duplicate_name (unescaped_name, count_increment);
	g_free (unescaped_name);

	new_file = g_filename_from_utf8 (unescaped_result, -1, NULL, NULL, NULL);
	result = gnome_vfs_escape_path_string (new_file);
	g_free (unescaped_result);
	g_free (new_file);
	return result;
}

static int
handle_transfer_duplicate (GnomeVFSXferProgressInfo *progress_info,
			   TransferInfo *transfer_info)
{
	switch (transfer_info->kind) {
	case TRANSFER_LINK:
		progress_info->duplicate_name = get_link_name
			(progress_info->duplicate_name,
			 progress_info->duplicate_count);
		break;

	case TRANSFER_COPY:
	case TRANSFER_MOVE_TO_TRASH:
		progress_info->duplicate_name = get_next_duplicate_name
			(progress_info->duplicate_name,
			 progress_info->duplicate_count);
		break;
	default:
		break;
		/* For all other cases we use the name as-is. */
	}

	return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
}

static int
update_transfer_callback (GnomeVFSAsyncHandle *handle,
	       GnomeVFSXferProgressInfo *progress_info,
	       gpointer data)
{
	TransferInfo *transfer_info;

	transfer_info = (TransferInfo *) data;

	switch (progress_info->status) {
	case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
		return handle_transfer_ok (progress_info, transfer_info);
	case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		return handle_transfer_vfs_error (progress_info, transfer_info);
	case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
		return handle_transfer_overwrite (progress_info, transfer_info);
	case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
		return handle_transfer_duplicate (progress_info, transfer_info);
	default:
		g_warning (_("Unknown GnomeVFSXferProgressStatus %d"),
			   progress_info->status);
		return 0;
	}
}

static void
apply_one_position (IconPositionIterator *position_iterator, 
		    const char *source_name,
		    const char *target_name)
{
	GdkPoint point;

	if (icon_position_iterator_get_next (position_iterator, source_name, &point)) {
		nautilus_file_changes_queue_schedule_position_set (target_name, point, position_iterator->screen);
	} else {
		nautilus_file_changes_queue_schedule_position_remove (target_name);
	}
}

typedef struct {
	GHashTable		*debuting_uris;
	IconPositionIterator	*iterator;
} SyncTransferInfo;

/* Low-level callback, called for every copy engine operation.
 * Generates notifications about new, deleted and moved files.
 */
static int
sync_transfer_callback (GnomeVFSXferProgressInfo *progress_info, gpointer data)
{
	GHashTable	     *debuting_uris;
	IconPositionIterator *position_iterator;
	gboolean              really_moved;

	if (data != NULL) {
		debuting_uris	  = ((SyncTransferInfo *) data)->debuting_uris;
		position_iterator = ((SyncTransferInfo *) data)->iterator;
	} else {
		debuting_uris     = NULL;
		position_iterator = NULL;
	}

	if (progress_info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		switch (progress_info->phase) {
		case GNOME_VFS_XFER_PHASE_OPENTARGET:
			if (progress_info->top_level_item) {
				/* this is one of the selected copied or moved items -- we need
				 * to make sure it's metadata gets copied over
				 */
				if (progress_info->source_name == NULL) {
					/* remove any old metadata */
					nautilus_file_changes_queue_schedule_metadata_remove 
						(progress_info->target_name);
				} else {
					nautilus_file_changes_queue_schedule_metadata_copy 
						(progress_info->source_name, progress_info->target_name);

					apply_one_position (position_iterator,
							    progress_info->source_name,
							    progress_info->target_name);
				}
				if (debuting_uris != NULL) {
					g_hash_table_replace (debuting_uris,
							      g_strdup (progress_info->target_name),
							      GINT_TO_POINTER (TRUE));
				}
			}
			nautilus_file_changes_queue_file_added (progress_info->target_name);
			break;

		case GNOME_VFS_XFER_PHASE_MOVING:
			g_assert (progress_info->source_name != NULL);

			/* If the source and target are the same, that
			 * means we "moved" something in place. No
			 * actual change happened, so we really don't
			 * want to send out any change notification,
			 * but we do want to select the files as
			 * "newly moved here" so we put them into the
			 * debuting_uris set.
			 */
			really_moved = strcmp (progress_info->source_name,
					       progress_info->target_name) != 0;

			if (progress_info->top_level_item) {
				if (really_moved) {
					nautilus_file_changes_queue_schedule_metadata_move 
						(progress_info->source_name, progress_info->target_name);
					
					apply_one_position (position_iterator,
							    progress_info->source_name,
							    progress_info->target_name);
				}
				
				if (debuting_uris != NULL) {
					g_hash_table_replace (debuting_uris,
							      g_strdup (progress_info->target_name),
							      GINT_TO_POINTER (really_moved));
				}
			}
			if (really_moved) {
				nautilus_file_changes_queue_file_moved (progress_info->source_name,
									progress_info->target_name);
			}
			break;
			
		case GNOME_VFS_XFER_PHASE_DELETESOURCE:
			g_assert (progress_info->source_name != NULL);
			if (progress_info->top_level_item) {
				nautilus_file_changes_queue_schedule_metadata_remove 
					(progress_info->source_name);
			}
			nautilus_file_changes_queue_file_removed (progress_info->source_name);
			break;
			
		case GNOME_VFS_XFER_PHASE_COMPLETED:
			/* done, clean up */
			icon_position_iterator_free (position_iterator);
			/* SyncXferInfo doesn't own the debuting_uris hash table - don't free it here.
			 */
			g_free (data);
			break;

		default:
			break;
		}
	}
	return 1;
}

static gboolean
check_target_directory_is_or_in_trash (GnomeVFSURI *trash_dir_uri, GnomeVFSURI *target_dir_uri)
{
	g_assert (target_dir_uri != NULL);

	if (trash_dir_uri == NULL) {
		return FALSE;
	}

	return gnome_vfs_uri_equal (trash_dir_uri, target_dir_uri)
		|| gnome_vfs_uri_is_parent (trash_dir_uri, target_dir_uri, TRUE);
}


static GnomeVFSURI *
append_basename (const GnomeVFSURI *target_directory,
		 const GnomeVFSURI *source_directory)
{
	char *file_name;
	GnomeVFSURI *ret;

	file_name = gnome_vfs_uri_extract_short_name (source_directory);
	if (file_name != NULL) {
		ret = gnome_vfs_uri_append_file_name (target_directory, 
						      file_name);
		g_free (file_name);
		return ret;
	}
	 
	return gnome_vfs_uri_dup (target_directory);
}

void
nautilus_file_operations_copy_move (const GList *item_uris,
				    GArray *relative_item_points,
				    const char *target_dir,
				    GdkDragAction copy_action,
				    GtkWidget *parent_view,
				    void (*done_callback) (GHashTable *debuting_uris, gpointer data),
				    gpointer done_callback_data)
{
	const GList *p;
	GnomeVFSXferOptions move_options;
	GList *source_uri_list, *target_uri_list;
	GnomeVFSURI *source_uri, *target_uri;
	GnomeVFSURI *source_dir_uri, *target_dir_uri;
	GnomeVFSURI *trash_dir_uri;
	GnomeVFSURI *uri;

	TransferInfo *transfer_info;
	SyncTransferInfo *sync_transfer_info;
	GnomeVFSResult result;
	gboolean target_is_trash;
	gboolean duplicate;
	gboolean target_is_mapping;
	gboolean have_nonlocal_source;
	
	IconPositionIterator *icon_position_iterator;

	GdkScreen *screen;
	int screen_num;

	g_assert (item_uris != NULL);

	target_dir_uri = NULL;
	trash_dir_uri = NULL;
	result = GNOME_VFS_OK;

	target_is_trash = FALSE;
	target_is_mapping = FALSE;
	if (target_dir != NULL) {
		if (eel_uri_is_trash (target_dir)) {
			target_is_trash = TRUE;
		} else {
			target_dir_uri = gnome_vfs_uri_new (target_dir);
		}
		if (strncmp (target_dir, "burn:", 5) == 0) {
			target_is_mapping = TRUE;
		}
			
	}

	/* Build the source and target URI lists and figure out if all
	 * the files are on the same disk.
	 */
	source_uri_list = NULL;
	target_uri_list = NULL;
	have_nonlocal_source = FALSE;
	duplicate = copy_action != GDK_ACTION_MOVE;
	for (p = item_uris; p != NULL; p = p->next) {
		/* Filter out special Nautilus link files */
		/* FIXME bugzilla.gnome.org 45295: 
		 * This is surprising behavior -- the user drags the Trash icon (say)
		 * to a folder, releases it, and nothing whatsoever happens. Don't we want
		 * a dialog in this case?
		 */
		if (is_special_link ((const char *) p->data)) {
			continue;
		}

		source_uri = gnome_vfs_uri_new ((const char *) p->data);
		if (source_uri == NULL) {
			continue;
		}
		
		if (strcmp (source_uri->method_string, "file") != 0) {
			have_nonlocal_source = TRUE;
		}
			
		source_dir_uri = gnome_vfs_uri_get_parent (source_uri);
		target_uri = NULL;
		if (target_dir != NULL) {
			if (target_is_trash) {
				gnome_vfs_find_directory (source_uri, GNOME_VFS_DIRECTORY_KIND_TRASH,
							  &target_dir_uri, FALSE, FALSE, 0777);			
			}
			if (target_dir_uri != NULL) {
				target_uri = append_basename (target_dir_uri, source_uri);
			}
		} else {
			/* duplication */
			target_uri = gnome_vfs_uri_ref (source_uri);
			if (target_dir_uri == NULL) {
				target_dir_uri = gnome_vfs_uri_ref (source_dir_uri);
			}
		}
		
		if (target_uri != NULL) {
			g_assert (target_dir_uri != NULL);

			target_uri_list = g_list_prepend (target_uri_list, target_uri);
			source_uri_list = g_list_prepend (source_uri_list, source_uri);

			if (duplicate
			    && !gnome_vfs_uri_equal (source_dir_uri, target_dir_uri)) {
				duplicate = FALSE;
			}
		}
		gnome_vfs_uri_unref (source_dir_uri);
	}

	if (target_is_trash) {
		/* Make sure new trash directories that we don't show yet get integrated. */
		nautilus_trash_monitor_add_new_trash_directories ();
	}

	move_options = GNOME_VFS_XFER_RECURSIVE;
	if (duplicate) {
		/* Copy operation, parents match -> duplicate
		 * operation. Ask gnome-vfs to generate unique names
		 * for target files.
		 */
		move_options |= GNOME_VFS_XFER_USE_UNIQUE_NAMES;
	}

	/* List may be NULL if we filtered all items out */
	if (source_uri_list == NULL) {
		if (target_dir_uri != NULL) {
			gnome_vfs_uri_unref (target_dir_uri);
		}
		if (target_uri_list != NULL) {
			gnome_vfs_uri_list_free (target_uri_list);
		}
		return;
	}
	
	source_uri_list = g_list_reverse (source_uri_list);
	target_uri_list = g_list_reverse (target_uri_list);

	if (target_is_mapping && !have_nonlocal_source && (copy_action == GDK_ACTION_COPY || copy_action == GDK_ACTION_MOVE)) {
		copy_action = GDK_ACTION_LINK;
	}
	if (copy_action == GDK_ACTION_MOVE && !target_is_mapping) {
		move_options |= GNOME_VFS_XFER_REMOVESOURCE;
	} else if (copy_action == GDK_ACTION_LINK) {
		move_options |= GNOME_VFS_XFER_LINK_ITEMS;
	}
	
	/* set up the copy/move parameters */
	transfer_info = transfer_info_new (parent_view);
	if (relative_item_points != NULL && relative_item_points->len > 0) {
		screen = gtk_widget_get_screen (GTK_WIDGET (parent_view));
		screen_num = gdk_screen_get_number (screen);
		/* FIXME: we probably don't need an icon_position_iterator
		 * here at all.
		 */
		icon_position_iterator = icon_position_iterator_new
			(relative_item_points, item_uris, screen_num);
	} else {
		icon_position_iterator = NULL;
	}
	
	if (target_is_trash && (move_options & GNOME_VFS_XFER_REMOVESOURCE) != 0) {
		/* when moving to trash, handle name conflicts automatically */
		move_options |= GNOME_VFS_XFER_USE_UNIQUE_NAMES;
		/* localizers: progress dialog title */
		transfer_info->operation_title = _("Moving files to the Trash");
		/* localizers: label prepended to the progress count */
		transfer_info->action_label =_("Files thrown out:");
		/* localizers: label prepended to the name of the current file moved */
		transfer_info->progress_verb =_("Moving");
		transfer_info->preparation_name =_("Preparing to Move to Trash...");

		transfer_info->kind = TRANSFER_MOVE_TO_TRASH;

	} else if ((move_options & GNOME_VFS_XFER_REMOVESOURCE) != 0) {
		/* localizers: progress dialog title */
		transfer_info->operation_title = _("Moving files");
		/* localizers: label prepended to the progress count */
		transfer_info->action_label =_("Files moved:");
		/* localizers: label prepended to the name of the current file moved */
		transfer_info->progress_verb =_("Moving");
		transfer_info->preparation_name =_("Preparing To Move...");
		transfer_info->cleanup_name = _("Finishing Move...");

		transfer_info->kind = TRANSFER_MOVE;

	} else if ((move_options & GNOME_VFS_XFER_LINK_ITEMS) != 0) {
		/* when creating links, handle name conflicts automatically */
		move_options |= GNOME_VFS_XFER_USE_UNIQUE_NAMES;
		/* localizers: progress dialog title */
		transfer_info->operation_title = _("Creating links to files");
		/* localizers: label prepended to the progress count */
		transfer_info->action_label =_("Files linked:");
		/* localizers: label prepended to the name of the current file linked */
		transfer_info->progress_verb =_("Linking");
		transfer_info->preparation_name = _("Preparing to Create Links...");
		transfer_info->cleanup_name = _("Finishing Creating Links...");

		transfer_info->kind = TRANSFER_LINK;

	} else {
		/* localizers: progress dialog title */
		transfer_info->operation_title = _("Copying files");
		/* localizers: label prepended to the progress count */
		transfer_info->action_label =_("Files copied:");
		/* localizers: label prepended to the name of the current file copied */
		transfer_info->progress_verb =_("Copying");
		transfer_info->preparation_name =_("Preparing To Copy...");
		transfer_info->cleanup_name = "";

		transfer_info->kind = TRANSFER_COPY;
	}

	/* we'll need to check for copy into Trash and for moving/copying the Trash itself */
	gnome_vfs_find_directory (target_dir_uri, GNOME_VFS_DIRECTORY_KIND_TRASH,
				  &trash_dir_uri, FALSE, FALSE, 0777);

	if ((move_options & GNOME_VFS_XFER_REMOVESOURCE) == 0) {
		/* don't allow copying into Trash */
		if (check_target_directory_is_or_in_trash (trash_dir_uri, target_dir_uri)) {
			eel_run_simple_dialog
				(parent_view, 
				 FALSE,
				 GTK_MESSAGE_ERROR,
				 ((move_options & GNOME_VFS_XFER_LINK_ITEMS) == 0)
				 ? _("You cannot copy items into the trash.")
				 : _("You cannot create links inside the trash."),
				 _("Files and folders can only be moved into the trash."), 
				 NULL,
				 GTK_STOCK_OK, NULL);
			result = GNOME_VFS_ERROR_NOT_PERMITTED;
		}
	}

	if (result == GNOME_VFS_OK) {
		for (p = source_uri_list; p != NULL; p = p->next) {
			uri = (GnomeVFSURI *)p->data;

			/* Check that a trash folder is not being moved/copied (link is OK). */
			if (trash_dir_uri != NULL 
			    && ((move_options & GNOME_VFS_XFER_LINK_ITEMS) == 0) 
			    && gnome_vfs_uri_equal (uri, trash_dir_uri)) {
			    	/* Distinguish Trash file on desktop from other trash folders for
			    	 * message purposes.
			    	 */

				eel_run_simple_dialog
					(parent_view,
					 FALSE,
					 GTK_MESSAGE_ERROR,
					 ((move_options & GNOME_VFS_XFER_REMOVESOURCE) != 0)
						 ? _("You cannot move this trash folder.")
						 : _("You cannot copy this trash folder."),
					 _("A trash folder is used for storing items moved to the trash."),
					 ((move_options & GNOME_VFS_XFER_REMOVESOURCE) != 0)
						 ? _("Can't Change Trash Location")
						 : _("Can't Copy Trash"),
					 GTK_STOCK_OK, NULL, NULL);

				result = GNOME_VFS_ERROR_NOT_PERMITTED;
				break;
			}
			
			/* FIXME:
			 * We should not have the case where a folder containing trash is moved into
			 * the trash give a generic "cannot move into itself" message, rather,
			 * we should have a trash specific message here.
			 */

			/* Don't allow recursive move/copy into itself. 
			 * (We would get a file system error if we proceeded but it is nicer to
			 * detect and report it at this level) */
			if ((move_options & GNOME_VFS_XFER_LINK_ITEMS) == 0
				&& (gnome_vfs_uri_equal (uri, target_dir_uri)
					|| gnome_vfs_uri_is_parent (uri, target_dir_uri, TRUE))) {
				eel_run_simple_dialog
					(parent_view, 
					 FALSE,
					 GTK_MESSAGE_ERROR,
					 ((move_options & GNOME_VFS_XFER_REMOVESOURCE) != 0) 
					 ? _("You cannot move a folder into itself.")
					 : _("You cannot copy a folder into itself."), 
					 _("The destination folder is inside the source folder."), 
					 ((move_options & GNOME_VFS_XFER_REMOVESOURCE) != 0) 
					 ? _("Can't Move Into Self")
					 : _("Can't Copy Into Self"),
					 GTK_STOCK_OK, NULL, NULL);			

				result = GNOME_VFS_ERROR_NOT_PERMITTED;
				break;
			}
			if ((move_options & GNOME_VFS_XFER_REMOVESOURCE) == 0
			        && (move_options & GNOME_VFS_XFER_USE_UNIQUE_NAMES) == 0
				&& gnome_vfs_uri_is_parent (target_dir_uri, uri, FALSE)) {
				eel_run_simple_dialog
					(parent_view, 
					 FALSE,
					 GTK_MESSAGE_ERROR,
					 _("You cannot copy a file over itself."),
					 _("The destination and source are the same file."), 
					 _("Can't Copy Over Self"), 
					 GTK_STOCK_OK, NULL, NULL);			

				result = GNOME_VFS_ERROR_NOT_PERMITTED;
				break;
			}
		}
	}

	transfer_info->error_mode = GNOME_VFS_XFER_ERROR_MODE_QUERY;
	transfer_info->overwrite_mode = GNOME_VFS_XFER_OVERWRITE_MODE_QUERY;
	transfer_info->done_callback = done_callback;
	transfer_info->done_callback_data = done_callback_data;
	transfer_info->debuting_uris = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	sync_transfer_info = g_new (SyncTransferInfo, 1);
	sync_transfer_info->iterator = icon_position_iterator;
	sync_transfer_info->debuting_uris = transfer_info->debuting_uris;

	if (result == GNOME_VFS_OK) {
		gnome_vfs_async_xfer (&transfer_info->handle, source_uri_list, target_uri_list,
		      		      move_options, GNOME_VFS_XFER_ERROR_MODE_QUERY, 
		      		      GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
				      GNOME_VFS_PRIORITY_DEFAULT,
		      		      update_transfer_callback, transfer_info,
		      		      sync_transfer_callback, sync_transfer_info);
	}

	gnome_vfs_uri_list_free (source_uri_list);
	gnome_vfs_uri_list_free (target_uri_list);
	if (trash_dir_uri != NULL) {
		gnome_vfs_uri_unref (trash_dir_uri);
	}
	gnome_vfs_uri_unref (target_dir_uri);
}

typedef struct {
	GnomeVFSAsyncHandle *handle;
	NautilusNewFolderCallback done_callback;
	gpointer data;
	GtkWidget *parent_view;
} NewFolderTransferState;

static int
handle_new_folder_vfs_error (const GnomeVFSXferProgressInfo *progress_info, NewFolderTransferState *state)
{
	const char *error_string;
	char *error_string_to_free;

	error_string_to_free = NULL;

	if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED) {
		error_string = _("You do not have permissions to write to the destination.");
	} else if (progress_info->vfs_status == GNOME_VFS_ERROR_NO_SPACE) {
		error_string = _("There is no space on the destination.");
	} else {
		error_string = g_strdup_printf (_("Error \"%s\" creating new folder."), 
						gnome_vfs_result_to_string (progress_info->vfs_status));
		error_string_to_free = (char *)error_string;
	}
	
	eel_show_error_dialog (_("Error creating new folder."), error_string, _("Error Creating New Folder"),
				    GTK_WINDOW (gtk_widget_get_toplevel (state->parent_view)));
	
	g_free (error_string_to_free);
	
	return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
}

static int
new_folder_transfer_callback (GnomeVFSAsyncHandle *handle,
			      GnomeVFSXferProgressInfo *progress_info,
			      gpointer data)
{
	NewFolderTransferState *state;
	char *temp_string;
	char *new_uri;
	
	state = (NewFolderTransferState *) data;

	switch (progress_info->phase) {

	case GNOME_VFS_XFER_PHASE_COMPLETED:
		eel_remove_weak_pointer (&state->parent_view);
		g_free (state);
		return 0;

	default:
		switch (progress_info->status) {
		case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
			nautilus_file_changes_consume_changes (TRUE);
			new_uri = NULL;
			if (progress_info->vfs_status == GNOME_VFS_OK) {
				new_uri = progress_info->target_name;
			}
			(* state->done_callback) (new_uri,
						  state->data);
			return 1;
	
		case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
	
			temp_string = progress_info->duplicate_name;
	
			if (progress_info->vfs_status == GNOME_VFS_ERROR_NAME_TOO_LONG) {
				/* special case an 8.3 file system */
				progress_info->duplicate_name = g_strndup (temp_string, 8);
				progress_info->duplicate_name[8] = '\0';
				g_free (temp_string);
				temp_string = progress_info->duplicate_name;
				progress_info->duplicate_name = g_strdup_printf
					("%s.%d", 
					 progress_info->duplicate_name,
					 progress_info->duplicate_count);
			} else {
				progress_info->duplicate_name = g_strdup_printf
					("%s%%20%d", 
					 progress_info->duplicate_name,
					 progress_info->duplicate_count);
			}
			g_free (temp_string);
			return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
	
		case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
			return handle_new_folder_vfs_error (progress_info, state);
		
	

		default:
			g_warning (_("Unknown GnomeVFSXferProgressStatus %d"),
				   progress_info->status);
			return 0;
		}
	}
}

void 
nautilus_file_operations_new_folder (GtkWidget *parent_view, 
				     const char *parent_dir,
				     NautilusNewFolderCallback done_callback,
				     gpointer data)
{
	GList *target_uri_list;
	GnomeVFSURI *uri, *parent_uri;
	char *dirname;
	NewFolderTransferState *state;

	state = g_new (NewFolderTransferState, 1);
	state->done_callback = done_callback;
	state->data = data;
	state->parent_view = parent_view;
	eel_add_weak_pointer (&state->parent_view);

	/* pass in the target directory and the new folder name as a destination URI */
	parent_uri = gnome_vfs_uri_new (parent_dir);
	/* localizers: the initial name of a new folder  */

	dirname = g_filename_from_utf8 (_("untitled folder"), -1, NULL, NULL, NULL);
	uri = gnome_vfs_uri_append_file_name (parent_uri, dirname);
	g_free (dirname);
	target_uri_list = g_list_prepend (NULL, uri);
	
	gnome_vfs_async_xfer (&state->handle, NULL, target_uri_list,
	      		      GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY,
	      		      GNOME_VFS_XFER_ERROR_MODE_QUERY, 
	      		      GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
			      GNOME_VFS_PRIORITY_DEFAULT,
	      		      new_folder_transfer_callback, state,
	      		      sync_transfer_callback, NULL);

	gnome_vfs_uri_list_free (target_uri_list);
	gnome_vfs_uri_unref (parent_uri);
}

typedef struct {
	GnomeVFSAsyncHandle *handle;
	NautilusNewFileCallback done_callback;
	gpointer data;
	GtkWidget *parent_view;
	char *empty_file;
	GHashTable *debuting_uris;
} NewFileTransferState;


static int
handle_new_file_vfs_error (const GnomeVFSXferProgressInfo *progress_info, NewFileTransferState *state)
{
	const char *error_string;
	char *error_string_to_free;

	error_string_to_free = NULL;

	if (progress_info->vfs_status == GNOME_VFS_ERROR_ACCESS_DENIED) {
		error_string = _("You do not have permissions to write to the destination.");
	} else if (progress_info->vfs_status == GNOME_VFS_ERROR_NO_SPACE) {
		error_string = _("There is no space on the destination.");
	} else {
		error_string = g_strdup_printf (_("Error \"%s\" creating new document."), 
						gnome_vfs_result_to_string (progress_info->vfs_status));
		error_string_to_free = (char *)error_string;
	}
	
	eel_show_error_dialog (_("Error creating new document."), error_string, _("Error Creating New Document"),
			       GTK_WINDOW (gtk_widget_get_toplevel (state->parent_view)));
	
	g_free (error_string_to_free);
	
	return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
}

static void
get_new_file_uri (gpointer       key,
		  gpointer       value,
		  gpointer       user_data)
{
	char *uri;
	char **uri_out;

	uri = key;
	uri_out = user_data;

	*uri_out = uri;
}


static int
new_file_transfer_callback (GnomeVFSAsyncHandle *handle,
			      GnomeVFSXferProgressInfo *progress_info,
			      gpointer data)
{
	NewFileTransferState *state;
	char *temp_string;
	char *uri;
	
	state = (NewFileTransferState *) data;

	switch (progress_info->phase) {

	case GNOME_VFS_XFER_PHASE_COMPLETED:
		uri = NULL;
		
		g_hash_table_foreach (state->debuting_uris,
				      get_new_file_uri, &uri);

		(* state->done_callback) (uri, state->data);
		/* uri is owned by hashtable, don't free */

		if (state->empty_file != NULL) {
			unlink (state->empty_file);
			g_free (state->empty_file);
		}
		eel_remove_weak_pointer (&state->parent_view);
		g_hash_table_destroy (state->debuting_uris);
		g_free (state);
		return 0;

	default:
		switch (progress_info->status) {
		case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
			nautilus_file_changes_consume_changes (TRUE);
			return 1;
	
		case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
	
			temp_string = progress_info->duplicate_name;
	
			if (progress_info->vfs_status == GNOME_VFS_ERROR_NAME_TOO_LONG) {
				/* special case an 8.3 file system */
				progress_info->duplicate_name = g_strndup (temp_string, 8);
				progress_info->duplicate_name[8] = '\0';
				g_free (temp_string);
				temp_string = progress_info->duplicate_name;
				progress_info->duplicate_name = g_strdup_printf
					("%s.%d", 
					 progress_info->duplicate_name,
					 progress_info->duplicate_count);
			} else {
				progress_info->duplicate_name = g_strdup_printf
					("%s%%20%d", 
					 progress_info->duplicate_name,
					 progress_info->duplicate_count);
			}
			g_free (temp_string);
			return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
	
		case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
			return handle_new_file_vfs_error (progress_info, state);
		
	

		default:
			g_warning (_("Unknown GnomeVFSXferProgressStatus %d"),
				   progress_info->status);
			return 0;
		}
	}
}

void 
nautilus_file_operations_new_file (GtkWidget *parent_view, 
				   const char *parent_dir,
				   const char *source_uri_text,
				   NautilusNewFileCallback done_callback,
				   gpointer data)
{
	GList *target_uri_list;
	GList *source_uri_list;
	GnomeVFSURI *target_uri, *parent_uri, *source_uri;
	char *filename;
	NewFileTransferState *state;
	SyncTransferInfo *sync_transfer_info;

	state = g_new (NewFileTransferState, 1);
	state->done_callback = done_callback;
	state->data = data;
	state->parent_view = parent_view;
	state->empty_file = NULL;

	/* pass in the target directory and the new folder name as a destination URI */
	parent_uri = gnome_vfs_uri_new (parent_dir);

	if (source_uri_text != NULL) {
		source_uri = gnome_vfs_uri_new (source_uri_text);
		if (source_uri == NULL) {
			(*done_callback) (NULL, data);
			g_free (state);
			return;
		}
		filename = gnome_vfs_uri_extract_short_path_name (source_uri);
		target_uri = gnome_vfs_uri_append_string (parent_uri, filename);
		g_free (filename);
	} else {
		char empty_file[] = "/tmp/emptyXXXXXX";
		char *empty_uri;
		int fd;

		fd = mkstemp (empty_file);
		if (fd == -1) {
			(*done_callback) (NULL, data);
			g_free (state);
		}
		close (fd);

		empty_uri = gnome_vfs_get_uri_from_local_path (empty_file);
		source_uri = gnome_vfs_uri_new (empty_uri);
		g_free (empty_uri);
		
		state->empty_file = g_strdup (empty_file);
		
		filename = g_filename_from_utf8 (_("new file"), -1, NULL, NULL, NULL);
		target_uri = gnome_vfs_uri_append_file_name (parent_uri, filename);
		g_free (filename);
	}
	
	state->debuting_uris = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	eel_add_weak_pointer (&state->parent_view);

	target_uri_list = g_list_prepend (NULL, target_uri);
	source_uri_list = g_list_prepend (NULL, source_uri);

	sync_transfer_info = g_new (SyncTransferInfo, 1);
	sync_transfer_info->iterator = NULL;
	sync_transfer_info->debuting_uris = state->debuting_uris;
	
	gnome_vfs_async_xfer (&state->handle, source_uri_list, target_uri_list,
	      		      GNOME_VFS_XFER_USE_UNIQUE_NAMES,
	      		      GNOME_VFS_XFER_ERROR_MODE_QUERY, 
	      		      GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
			      GNOME_VFS_PRIORITY_DEFAULT,
			      new_file_transfer_callback, state,
	      		      sync_transfer_callback, sync_transfer_info);

	gnome_vfs_uri_list_free (target_uri_list);
	gnome_vfs_uri_list_free (source_uri_list);
	gnome_vfs_uri_unref (parent_uri);
}

void 
nautilus_file_operations_delete (const GList *item_uris, 
				 GtkWidget *parent_view)
{
	GList *uri_list;
	const GList *p;
	const char *item_uri;
	NautilusFile *file;
	TransferInfo *transfer_info;

	uri_list = NULL;
	for (p = item_uris; p != NULL; p = p->next) {
		item_uri = (const char *) p->data;

		if (eel_uri_is_desktop (item_uri)) {
			file = nautilus_file_get_existing (item_uri);
			if (file != NULL) {
				if (NAUTILUS_IS_DESKTOP_ICON_FILE (file)) {
					NautilusDesktopLink *link;

					link = nautilus_desktop_icon_file_get_link (NAUTILUS_DESKTOP_ICON_FILE (file));

					nautilus_desktop_link_monitor_delete_link (nautilus_desktop_link_monitor_get (),
										   link,
										   parent_view);
					
					g_object_unref (link);
				}
				nautilus_file_unref (file);
			}
		} else {
			uri_list = g_list_prepend (uri_list, 
						   gnome_vfs_uri_new (item_uri));
		}
	}
	uri_list = g_list_reverse (uri_list);

	if (uri_list == NULL) {
		return;
	}

	transfer_info = transfer_info_new (parent_view);

	/* localizers: progress dialog title */
	transfer_info->operation_title = _("Deleting files");
	/* localizers: label prepended to the progress count */
	transfer_info->action_label =_("Files deleted:");
	/* localizers: label prepended to the name of the current file deleted */
	transfer_info->progress_verb =_("Deleting");
	transfer_info->preparation_name =_("Preparing to Delete files...");
	transfer_info->cleanup_name ="";

	transfer_info->error_mode = GNOME_VFS_XFER_ERROR_MODE_QUERY;
	transfer_info->overwrite_mode = GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE;
	transfer_info->kind = TRANSFER_DELETE;
	
	gnome_vfs_async_xfer (&transfer_info->handle, uri_list,  NULL,
	      		      GNOME_VFS_XFER_DELETE_ITEMS | GNOME_VFS_XFER_RECURSIVE,
	      		      GNOME_VFS_XFER_ERROR_MODE_QUERY, 
	      		      GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
			      GNOME_VFS_PRIORITY_DEFAULT,
	      		      update_transfer_callback, transfer_info,
	      		      sync_transfer_callback, NULL);

	gnome_vfs_uri_list_free (uri_list);
}

static void
do_empty_trash (GtkWidget *parent_view)
{
	TransferInfo *transfer_info;
	GList *trash_dir_list;

	trash_dir_list = nautilus_trash_monitor_get_trash_directories ();
	if (trash_dir_list != NULL) {
		/* set up the move parameters */
		transfer_info = transfer_info_new (parent_view);

		/* localizers: progress dialog title */
		transfer_info->operation_title = _("Emptying the Trash");
		/* localizers: label prepended to the progress count */
		transfer_info->action_label =_("Files deleted:");
		/* localizers: label prepended to the name of the current file deleted */
		transfer_info->progress_verb =_("Deleting");
		transfer_info->preparation_name =_("Preparing to Empty the Trash...");
		transfer_info->cleanup_name ="";
		transfer_info->error_mode = GNOME_VFS_XFER_ERROR_MODE_QUERY;
		transfer_info->overwrite_mode = GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE;
		transfer_info->kind = TRANSFER_EMPTY_TRASH;

		gnome_vfs_async_xfer (&transfer_info->handle, trash_dir_list, NULL,
		      		      GNOME_VFS_XFER_EMPTY_DIRECTORIES,
		      		      GNOME_VFS_XFER_ERROR_MODE_QUERY, 
		      		      GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
				      GNOME_VFS_PRIORITY_DEFAULT,
		      		      update_transfer_callback, transfer_info,
		      		      sync_transfer_callback, NULL);
	}

	gnome_vfs_uri_list_free (trash_dir_list);
}

static gboolean
confirm_empty_trash (GtkWidget *parent_view)
{
	GtkWidget *dialog;
	int response;
	GtkWidget *hbox, *vbox, *image, *label, *button;
	gchar     *str;
	GdkScreen *screen;

	/* Just Say Yes if the preference says not to confirm. */
	if (!eel_preferences_get_boolean (NAUTILUS_PREFERENCES_CONFIRM_TRASH)) {
		return TRUE;
	}
	
	screen = gtk_widget_get_screen (parent_view);

	dialog = gtk_dialog_new ();
	gtk_window_set_screen (GTK_WINDOW (dialog), screen);
	atk_object_set_role (gtk_widget_get_accessible (dialog), ATK_ROLE_ALERT);
	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), "empty_trash",
				"Nautilus");

	/* Make transient for the window group */
        gtk_widget_realize (dialog);
	gdk_window_set_transient_for (GTK_WIDGET (dialog)->window,
				      gdk_screen_get_root_window (screen));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox,
			    FALSE, FALSE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
					  GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show (vbox);

	str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", 
		_("Are you sure you want to empty "
		"all of the items from the trash?"), 
		"</span>", 
		NULL);
		
	label = gtk_label_new (str);  
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	g_free (str);

	label = gtk_label_new (_("If you empty the trash, items "
		"will be permanently deleted."));
	
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);

	button = eel_gtk_button_new_with_stock_icon (_("_Empty"),
						     GTK_STOCK_DELETE);
	gtk_widget_show (button);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button,
				      GTK_RESPONSE_YES);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_YES);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_object_destroy (GTK_OBJECT (dialog));

	return response == GTK_RESPONSE_YES;
}

void 
nautilus_file_operations_empty_trash (GtkWidget *parent_view)
{
	g_return_if_fail (parent_view != NULL);

	/* 
	 * I chose to use a modal confirmation dialog here. 
	 * If we used a modeless dialog, we'd have to do work to
	 * make sure that no more than one appears on screen at
	 * a time. That one probably couldn't be parented, because
	 * otherwise you'd get into weird layer-shifting problems
	 * selecting "Empty Trash" from one window when there was
	 * already a modeless "Empty Trash" dialog parented on a 
	 * different window. And if the dialog were not parented, it
	 * might show up in some weird place since window manager
	 * placement is unreliable (i.e., sucks). So modal it is.
	 */
	if (confirm_empty_trash (parent_view)) {
		do_empty_trash (parent_view);
	}
}

#if !defined (NAUTILUS_OMIT_SELF_CHECK)

void
nautilus_self_check_file_operations (void)
{
	/* test the next duplicate name generator */
	EEL_CHECK_STRING_RESULT (get_duplicate_name (" (copy)", 1), " (another copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo", 1), "foo (copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name (".bashrc", 1), ".bashrc (copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name (".foo.txt", 1), ".foo (copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo", 1), "foo foo (copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo.txt", 1), "foo (copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo.txt", 1), "foo foo (copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo.txt txt", 1), "foo foo (copy).txt txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo...txt", 1), "foo.. (copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo...", 1), "foo... (copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo. (copy)", 1), "foo. (another copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (copy)", 1), "foo (another copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (copy).txt", 1), "foo (another copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (another copy)", 1), "foo (3rd copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (another copy).txt", 1), "foo (3rd copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo (another copy).txt", 1), "foo foo (3rd copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (13th copy)", 1), "foo (14th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (13th copy).txt", 1), "foo (14th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (21st copy)", 1), "foo (22nd copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (21st copy).txt", 1), "foo (22nd copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (22nd copy)", 1), "foo (23rd copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (22nd copy).txt", 1), "foo (23rd copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (23rd copy)", 1), "foo (24th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (23rd copy).txt", 1), "foo (24th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (24th copy)", 1), "foo (25th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (24th copy).txt", 1), "foo (25th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo (24th copy)", 1), "foo foo (25th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo (24th copy).txt", 1), "foo foo (25th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo foo (100000000000000th copy).txt", 1), "foo foo (copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (10th copy)", 1), "foo (11th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (10th copy).txt", 1), "foo (11th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (11th copy)", 1), "foo (12th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (11th copy).txt", 1), "foo (12th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (12th copy)", 1), "foo (13th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (12th copy).txt", 1), "foo (13th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (110th copy)", 1), "foo (111th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (110th copy).txt", 1), "foo (111th copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (122nd copy)", 1), "foo (123rd copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (122nd copy).txt", 1), "foo (123rd copy).txt");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (123rd copy)", 1), "foo (124th copy)");
	EEL_CHECK_STRING_RESULT (get_duplicate_name ("foo (123rd copy).txt", 1), "foo (124th copy).txt");
}

#endif
