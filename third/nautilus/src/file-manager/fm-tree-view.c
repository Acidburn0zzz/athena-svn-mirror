/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 * Copyright (C) 2000, 2001 Eazel, Inc
 * Copyright (C) 2002 Anders Carlsson
 * Copyright (C) 2002 Darin Adler
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: 
 *       Maciej Stachowiak <mjs@eazel.com>
 *       Anders Carlsson <andersca@gnu.org>
 *       Darin Adler <darin@bentspoon.com>
 */

/* fm-tree-view.c - tree sidebar panel
 */

#include <config.h>
#include "fm-tree-view.h"

#include "fm-tree-model.h"
#include "fm-properties-window.h"
#include <eel/eel-glib-extensions.h>
#include <eel/eel-preferences.h>
#include <eel/eel-string.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-vfs-extensions.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreemodelsort.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenushell.h>
#include <gtk/gtkclipboard.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomeui/gnome-popup-menu.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <libnautilus-private/nautilus-clipboard-monitor.h>
#include <libnautilus-private/nautilus-file-attributes.h>
#include <libnautilus-private/nautilus-file-operations.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-program-choosing.h>
#include <libnautilus-private/nautilus-tree-view-drag-dest.h>
#include <libnautilus-private/nautilus-icon-factory.h>
#include <libnautilus-private/nautilus-cell-renderer-pixbuf-emblem.h>

struct FMTreeViewDetails {
	GtkWidget *scrolled_window;
	GtkTreeView *tree_widget;
	GtkTreeModelSort *sort_model;
	FMTreeModel *child_model;

	NautilusFile *activation_file;
	gboolean activation_in_new_window;

	NautilusTreeViewDragDest *drag_dest;

	char *selection_location;
	gboolean selecting;

	guint show_selection_idle_id;
	
	GtkWidget *popup;
	GtkWidget *popup_open;
	GtkWidget *popup_open_in_new_window;
	GtkWidget *popup_create_folder;
	GtkWidget *popup_cut;
	GtkWidget *popup_copy;
	GtkWidget *popup_paste;
	GtkWidget *popup_rename;
	GtkWidget *popup_trash;
	GtkWidget *popup_properties;
	GtkWidget *popup_unmount_separator;
	GtkWidget *popup_unmount;
	NautilusFile *popup_file;
	
	guint selection_changed_timer;
};

typedef struct {
	GList *uris;
	FMTreeView *view;
} PrependURIParameters;

static GdkAtom copied_files_atom;

enum {
	GNOME_COPIED_FILES
};

static const GtkTargetEntry clipboard_targets[] = {
	{ "x-special/gnome-copied-files", 0, GNOME_COPIED_FILES },
};

BONOBO_CLASS_BOILERPLATE (FMTreeView, fm_tree_view,
			  NautilusView, NAUTILUS_TYPE_VIEW)

static gboolean
show_iter_for_file (FMTreeView *view, NautilusFile *file, GtkTreeIter *iter)
{
	GtkTreeModel *model;
	NautilusFile *parent_file;
	GtkTreeIter parent_iter;
	GtkTreePath *path, *sort_path;
	GtkTreeIter cur_iter;

	if (view->details->child_model == NULL) {
		return FALSE;
	}
	model = GTK_TREE_MODEL (view->details->child_model);

	/* check if file is visible in the same root as the currently selected folder is */
	gtk_tree_view_get_cursor (view->details->tree_widget, &path, NULL);
	if (path != NULL) {
		if (gtk_tree_model_get_iter (model, &cur_iter, path)) {
			if (fm_tree_model_file_get_iter (view->details->child_model,
							       iter, file, &cur_iter)) {
				return TRUE;
			}
		}
	}
	/* check if file is visible at all */
	if (fm_tree_model_file_get_iter (view->details->child_model,
					       iter, file, NULL)) {
		return TRUE;
	}

	parent_file = nautilus_file_get_parent (file);

	if (parent_file == NULL) {
		return FALSE;
	}
	if (!show_iter_for_file (view, parent_file, &parent_iter)) {
		nautilus_file_unref (parent_file);
		return FALSE;
	}
	nautilus_file_unref (parent_file);

	if (parent_iter.user_data == NULL || parent_iter.stamp == 0) {
		return FALSE;
	}
	path = gtk_tree_model_get_path (model, &parent_iter);
	sort_path = gtk_tree_model_sort_convert_child_path_to_path
		(view->details->sort_model, path);
	gtk_tree_path_free (path);
	gtk_tree_view_expand_row (view->details->tree_widget, sort_path, FALSE);
	gtk_tree_path_free (sort_path);

	return FALSE;
}

static gboolean
show_selection_idle_callback (gpointer callback_data)
{
	FMTreeView *view;
	NautilusFile *file, *old_file;
	GtkTreeIter iter;
	GtkTreePath *path, *sort_path;

	view = FM_TREE_VIEW (callback_data);

	view->details->show_selection_idle_id = 0;

	file = nautilus_file_get (view->details->selection_location);
	if (file == NULL) {
		return FALSE;
	}

	if (!nautilus_file_is_directory (file)) {
		old_file = file;
		file = nautilus_file_get_parent (file);
		nautilus_file_unref (old_file);
		if (file == NULL) {
			return FALSE;
		}
	}
	
	view->details->selecting = TRUE;
	if (!show_iter_for_file (view, file, &iter)) {
		nautilus_file_unref (file);
		return FALSE;
	}
	view->details->selecting = FALSE;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->details->child_model), &iter);
	sort_path = gtk_tree_model_sort_convert_child_path_to_path
		(view->details->sort_model, path);
	gtk_tree_path_free (path);
	gtk_tree_view_set_cursor (view->details->tree_widget, sort_path, NULL, FALSE);
	gtk_tree_view_scroll_to_cell (view->details->tree_widget, sort_path, NULL, FALSE, 0, 0);
	gtk_tree_path_free (sort_path);

	nautilus_file_unref (file);

	return FALSE;
}

static void
schedule_show_selection (FMTreeView *view)
{
	if (view->details->show_selection_idle_id == 0) {
		view->details->show_selection_idle_id = g_idle_add (show_selection_idle_callback, view);
	}
}

static void
row_loaded_callback (GtkTreeModel     *tree_model,
		     GtkTreeIter      *iter,
		     FMTreeView *view)
{
	NautilusFile *file, *tmp_file, *selection_file;

	if (view->details->selection_location == NULL
	    || !view->details->selecting
	    || iter->user_data == NULL || iter->stamp == 0) {
		return;
	}

	file = fm_tree_model_iter_get_file (view->details->child_model, iter);
	if (file == NULL) {
		return;
	}
	if (!nautilus_file_is_directory (file)) {
		nautilus_file_unref(file);
		return;
	}

	/* if iter is ancestor of wanted selection_location then update selection */
	selection_file = nautilus_file_get (view->details->selection_location);
	while (selection_file != NULL) {
		if (file == selection_file) {
			nautilus_file_unref (file);
			nautilus_file_unref (selection_file);

			schedule_show_selection (view);
			return;
		}
		tmp_file = nautilus_file_get_parent (selection_file);
		nautilus_file_unref (selection_file);
		selection_file = tmp_file;
	}
	nautilus_file_unref (file);
}

static NautilusFile *
sort_model_iter_to_file (FMTreeView *view, GtkTreeIter *iter)
{
	GtkTreeIter child_iter;

	gtk_tree_model_sort_convert_iter_to_child_iter (view->details->sort_model, &child_iter, iter);
	return fm_tree_model_iter_get_file (view->details->child_model, &child_iter);
}

static NautilusFile *
sort_model_path_to_file (FMTreeView *view, GtkTreePath *path)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (view->details->sort_model), &iter, path)) {
		return NULL;
	}
	return sort_model_iter_to_file (view, &iter);
}

static void
got_activation_uri_callback (NautilusFile *file, gpointer callback_data)
{
        char *uri, *file_uri;
        FMTreeView *view;
	GdkScreen *screen;
	Nautilus_ViewFrame_OpenMode mode;
	
        view = FM_TREE_VIEW (callback_data);

	screen = gtk_widget_get_screen (GTK_WIDGET (view->details->tree_widget));

        g_assert (file == view->details->activation_file);
        
        mode = view->details->activation_in_new_window ? Nautilus_ViewFrame_OPEN_IN_NAVIGATION : Nautilus_ViewFrame_OPEN_ACCORDING_TO_MODE;

	/* FIXME: reenable && !eel_uris_match_ignore_fragments (view->details->current_main_view_uri, uri) */

	uri = nautilus_file_get_activation_uri (file);
	if (uri != NULL
	    && eel_str_has_prefix (uri, NAUTILUS_COMMAND_SPECIFIER)) {

		uri += strlen (NAUTILUS_COMMAND_SPECIFIER);
		nautilus_launch_application_from_command (screen, NULL, uri, NULL, FALSE);

	} else if (uri != NULL
	    	   && eel_str_has_prefix (uri, NAUTILUS_DESKTOP_COMMAND_SPECIFIER)) {
		   
		file_uri = nautilus_file_get_uri (file);
		nautilus_launch_desktop_file (screen, file_uri, NULL, NULL);
		g_free (file_uri);
		
	} else if (uri != NULL
		   && nautilus_file_is_executable (file)
		   && nautilus_file_can_execute (file)
		   && !nautilus_file_is_directory (file)) {	
		   
		file_uri = gnome_vfs_get_local_path_from_uri (uri);

		/* Non-local executables don't get launched. They act like non-executables. */
		if (file_uri == NULL) {
			nautilus_view_open_location
				(NAUTILUS_VIEW (view), 
				 uri, 
				 mode,
				 0,
				 NULL);
		} else {
			nautilus_launch_application_from_command (screen, NULL, file_uri, NULL, FALSE);
			g_free (file_uri);
		}
		   
	} else if (uri != NULL) {
		if (view->details->selection_location == NULL ||
		    strcmp (uri, view->details->selection_location) != 0) {
			if (view->details->selection_location != NULL) {
				g_free (view->details->selection_location);
			}
			view->details->selection_location = g_strdup (uri);
			nautilus_view_open_location
				(NAUTILUS_VIEW (view), 
				 uri,
				 mode,
				 0,
				 NULL);
		}
	}

	g_free (uri);
	nautilus_file_unref (view->details->activation_file);
	view->details->activation_file = NULL;
}

static void
cancel_activation (FMTreeView *view)
{
        if (view->details->activation_file == NULL) {
		return;
	}
	
	nautilus_file_cancel_call_when_ready
		(view->details->activation_file,
		 got_activation_uri_callback, view);
	nautilus_file_unref (view->details->activation_file);
        view->details->activation_file = NULL;
}

static void
row_activated_callback (GtkTreeView *treeview, GtkTreePath *path, 
			GtkTreeViewColumn *column, FMTreeView *view)
{
	if (gtk_tree_view_row_expanded (view->details->tree_widget, path)) {
		gtk_tree_view_collapse_row (view->details->tree_widget, path);
	} else {
		gtk_tree_view_expand_row (view->details->tree_widget, 
					  path, FALSE);
	}
}

static gboolean 
selection_changed_timer_callback(FMTreeView *view)
{
	NautilusFileAttributes attributes;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->details->tree_widget));

	/* no activation if popup menu is open */
	if (view->details->popup_file != NULL) {
		return FALSE;
	}

	cancel_activation (view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		return FALSE;
	}

	view->details->activation_file = sort_model_iter_to_file (view, &iter);
	if (view->details->activation_file == NULL) {
		return FALSE;
	}
	view->details->activation_in_new_window = FALSE;
		
	attributes = NAUTILUS_FILE_ATTRIBUTE_ACTIVATION_URI;
	nautilus_file_call_when_ready (view->details->activation_file, attributes,
				       got_activation_uri_callback, view);
	return FALSE; /* remove timeout */
}

static void
selection_changed_callback (GtkTreeSelection *selection,
			    FMTreeView *view)
{
	GdkEvent *event;
	gboolean is_keyboard;

	if (view->details->selection_changed_timer) {
		g_source_remove (view->details->selection_changed_timer);
		view->details->selection_changed_timer = 0;
	}
	
	event = gtk_get_current_event ();
	if (event) {
		is_keyboard = (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE);
		gdk_event_free (event);

		if (is_keyboard) {
			/* on keyboard event: delay the change */
			/* TODO: make dependent on keyboard repeat rate as per Markus Bertheau ? */
			view->details->selection_changed_timer = g_timeout_add (300, (GSourceFunc) selection_changed_timer_callback, view);
		} else {
			/* on mouse event: show the change immediately */
			selection_changed_timer_callback (view);
		}
	}
}

static int
compare_rows (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer callback_data)
{
	NautilusFile *file_a, *file_b;
	int result;

	if (a->user_data == NULL) {
		return -1;
	}
	else if (b->user_data == NULL) {
		return -1;
	}

	/* don't sort root nodes */
	if (fm_tree_model_iter_is_root (FM_TREE_MODEL (model), a)
	    || fm_tree_model_iter_is_root (FM_TREE_MODEL (model), b)) {
		return 0;
	}

	file_a = fm_tree_model_iter_get_file (FM_TREE_MODEL (model), a);
	file_b = fm_tree_model_iter_get_file (FM_TREE_MODEL (model), b);

	if (file_a == file_b) {
		result = 0;
	} else if (file_a == NULL) {
		result = -1;
	} else if (file_b == NULL) {
		result = +1;
	} else {
		result = nautilus_file_compare_for_sort (file_a, file_b,
							 NAUTILUS_FILE_SORT_BY_DISPLAY_NAME,
							 FALSE, FALSE);
	}

	nautilus_file_unref (file_a);
	nautilus_file_unref (file_b);

	return result;
}


static char *
get_root_uri_callback (NautilusTreeViewDragDest *dest,
		       gpointer user_data)
{
	FMTreeView *view;
	
	view = FM_TREE_VIEW (user_data);

	/* Don't allow drops on background */
	return NULL;
}

static NautilusFile *
get_file_for_path_callback (NautilusTreeViewDragDest *dest,
			    GtkTreePath *path,
			    gpointer user_data)
{
	FMTreeView *view;
	
	view = FM_TREE_VIEW (user_data);

	return sort_model_path_to_file (view, path);
}

static void
move_copy_items_callback (NautilusTreeViewDragDest *dest,
			  const GList *item_uris,
			  const char *target_uri,
			  guint action,
			  int x,
			  int y,
			  gpointer user_data)
{
	FMTreeView *view;

	view = FM_TREE_VIEW (user_data);

	nautilus_file_operations_copy_move
		(item_uris,
		 NULL,
		 target_uri,
		 action,
		 GTK_WIDGET (view->details->tree_widget),
		 NULL, NULL);
}

static void
theme_changed_callback (GObject *icon_factory, gpointer callback_data)
{
        FMTreeView *view; 

        view = FM_TREE_VIEW (callback_data);
	if (view->details->child_model != NULL) {
		fm_tree_model_set_theme (FM_TREE_MODEL (view->details->child_model));
	}
}

static void
add_root_for_volume (FMTreeView *view,
		     GnomeVFSVolume *volume)
{
	char *icon, *mount_uri, *name;

	if (!gnome_vfs_volume_is_user_visible (volume)) {
		return;
	}
	
	icon = gnome_vfs_volume_get_icon (volume);
	mount_uri = gnome_vfs_volume_get_activation_uri (volume);
	name = gnome_vfs_volume_get_display_name (volume);
	
	fm_tree_model_add_root_uri(view->details->child_model,
				   mount_uri, name, icon, volume);

	g_free (icon);
	g_free (name);
	g_free (mount_uri);
	
}

static void
volume_mounted_callback (GnomeVFSVolumeMonitor *volume_monitor,
			 GnomeVFSVolume *volume,
			 FMTreeView *view)
{
	add_root_for_volume (view, volume);
}

static void
volume_unmounted_callback (GnomeVFSVolumeMonitor *volume_monitor,
			   GnomeVFSVolume *volume,
			   FMTreeView *view)
{
	char *mount_uri;
	
	mount_uri = gnome_vfs_volume_get_activation_uri (volume);
	fm_tree_model_remove_root_uri (view->details->child_model,
					     mount_uri);
	g_free (mount_uri);
}

static void
clipboard_contents_received_callback (GtkClipboard     *clipboard,
				      GtkSelectionData *selection_data,
				      gpointer          data)
{
	FMTreeView *view;

	view = FM_TREE_VIEW (data);

	if (selection_data->type == copied_files_atom
	    && selection_data->length > 0) {
		gtk_widget_set_sensitive (view->details->popup_paste, TRUE);
	}
}

static GtkClipboard *
get_clipboard (GtkWidget *widget)
{
	return gtk_clipboard_get_for_display (gtk_widget_get_display (widget),
					      GDK_SELECTION_CLIPBOARD);
}

static gboolean
can_move_uri_to_trash (const char *file_uri_string)
{
	/* Return TRUE if we can get a trash directory on the same volume as this file. */
	GnomeVFSURI *file_uri;
	GnomeVFSURI *directory_uri;
	GnomeVFSURI *trash_dir_uri;
	gboolean result;

	g_return_val_if_fail (file_uri_string != NULL, FALSE);

	file_uri = gnome_vfs_uri_new (file_uri_string);

	if (file_uri == NULL) {
		return FALSE;
	}

	/* FIXME: Why can't we just pass file_uri to gnome_vfs_find_directory? */
	directory_uri = gnome_vfs_uri_get_parent (file_uri);
	gnome_vfs_uri_unref (file_uri);

	if (directory_uri == NULL) {
		return FALSE;
	}

	/*
	 * Create a new trash if needed but don't go looking for an old Trash.
	 * Passing 0 permissions as gnome-vfs would override the permissions 
	 * passed with 700 while creating .Trash directory
	 */
	result = gnome_vfs_find_directory (directory_uri, GNOME_VFS_DIRECTORY_KIND_TRASH,
					   &trash_dir_uri, TRUE, FALSE, 0) == GNOME_VFS_OK;
	if (result) {
		gnome_vfs_uri_unref (trash_dir_uri);
	}
	gnome_vfs_uri_unref (directory_uri);

	return result;
}

static gboolean
eject_for_type (GnomeVFSDeviceType type)
{
	switch (type) {
	case GNOME_VFS_DEVICE_TYPE_CDROM:
	case GNOME_VFS_DEVICE_TYPE_ZIP:
	case GNOME_VFS_DEVICE_TYPE_JAZ:
		return TRUE;
	default:
		return FALSE;
	}
}

static gboolean
button_pressed_callback (GtkTreeView *treeview, GdkEventButton *event,
			 FMTreeView *view)
{
	GtkTreePath *path, *cursor_path;
	char *uri;

	if (event->button == 3) {
		gboolean unmount_is_eject = FALSE;
		gboolean show_unmount = FALSE;
		GnomeVFSVolume *volume = NULL;
		
		if (!gtk_tree_view_get_path_at_pos (treeview, event->x, event->y,
						    &path, NULL, NULL, NULL)) {
			return FALSE;
		}

		view->details->popup_file = sort_model_path_to_file (view, path);
		if (view->details->popup_file == NULL) {
			gtk_tree_path_free (path);
			return FALSE;
		}
		gtk_tree_view_get_cursor (view->details->tree_widget, &cursor_path, NULL);
		gtk_tree_view_set_cursor (view->details->tree_widget, path, NULL, FALSE);
		gtk_tree_path_free (path);
		uri = nautilus_file_get_uri (view->details->popup_file);
		
		gtk_widget_set_sensitive (view->details->popup_open_in_new_window,
			nautilus_file_is_directory (view->details->popup_file));
		gtk_widget_set_sensitive (view->details->popup_create_folder,
			nautilus_file_is_directory (view->details->popup_file) &&
			nautilus_file_can_write (view->details->popup_file));
		gtk_widget_set_sensitive (view->details->popup_paste, FALSE);
		if (nautilus_file_is_directory (view->details->popup_file) &&
			nautilus_file_can_write (view->details->popup_file)) {
			gtk_clipboard_request_contents (get_clipboard (GTK_WIDGET (view->details->tree_widget)),
							copied_files_atom,
							clipboard_contents_received_callback, view);
		}
		gtk_widget_set_sensitive (view->details->popup_trash, can_move_uri_to_trash (uri));
		g_free (uri);
		
		volume = fm_tree_model_get_volume_for_root_node_file (view->details->child_model, view->details->popup_file);
		if (volume) {
			show_unmount = TRUE;
			unmount_is_eject = eject_for_type (gnome_vfs_volume_get_device_type (volume));
		} 
		
		gtk_label_set_text (GTK_LABEL (GTK_BIN (GTK_MENU_ITEM (view->details->popup_unmount))->child),
				    unmount_is_eject? _("E_ject"):_("_Unmount Volume"));
		gtk_label_set_use_underline (GTK_LABEL (GTK_BIN (GTK_MENU_ITEM (view->details->popup_unmount))->child),
				    TRUE);
		if (show_unmount) {
			gtk_widget_show (view->details->popup_unmount_separator);
			gtk_widget_show (view->details->popup_unmount);
		} else {
			gtk_widget_hide (view->details->popup_unmount_separator);
			gtk_widget_hide (view->details->popup_unmount);
		}		

		g_object_ref (view);
		
		gnome_popup_menu_do_popup_modal (view->details->popup,
						 NULL, NULL, event, NULL,
						 GTK_WIDGET (treeview));
		
		gtk_tree_view_set_cursor (view->details->tree_widget, cursor_path, NULL, FALSE);
		gtk_tree_path_free (cursor_path);

		nautilus_file_unref (view->details->popup_file);
		view->details->popup_file = NULL;
		
		g_object_unref (view);

		return TRUE;
	}

	return FALSE;
}

static void
fm_tree_view_activate_file (FMTreeView *view, 
			    NautilusFile *file,
			    gboolean open_in_new_window)
{
	NautilusFileAttributes attributes;

	cancel_activation (view);

	view->details->activation_file = nautilus_file_ref (file);
	view->details->activation_in_new_window = open_in_new_window;
		
	attributes = NAUTILUS_FILE_ATTRIBUTE_ACTIVATION_URI;
	nautilus_file_call_when_ready (view->details->activation_file, attributes,
				       got_activation_uri_callback, view);
}

static void
fm_tree_view_open_cb (GtkWidget *menu_item,
		      FMTreeView *view)
{
	fm_tree_view_activate_file (view, view->details->popup_file, FALSE);
}

static void
fm_tree_view_open_in_new_window_cb (GtkWidget *menu_item,
				    FMTreeView *view)
{
	fm_tree_view_activate_file (view, view->details->popup_file, TRUE);
}

static void
new_folder_done (const char *new_folder_uri, gpointer data)
{
	GList *list;

	/* show the properties window for the newly created
	 * folder so the user can change its name
	 */
	list = g_list_prepend (NULL, nautilus_file_get (new_folder_uri));

	fm_properties_window_present (list, GTK_WIDGET (data));

        nautilus_file_list_free (list);
}

static void
fm_tree_view_create_folder_cb (GtkWidget *menu_item,
			       FMTreeView *view)
{
	char *parent_uri;

	parent_uri = nautilus_file_get_uri (view->details->popup_file);
	nautilus_file_operations_new_folder (GTK_WIDGET (view->details->tree_widget),
					     parent_uri,
					     new_folder_done, view->details->tree_widget);

	g_free (parent_uri);
}

static void
get_clipboard_callback (GtkClipboard	 *clipboard,
			GtkSelectionData *selection_data,
			guint		  info,
			gpointer	  user_data_or_owner)
{
	char *str = user_data_or_owner;
	
	gtk_selection_data_set (selection_data,
				copied_files_atom,
				8,
				str,
				strlen (str));
}

static void
clear_clipboard_callback (GtkClipboard *clipboard,
			  gpointer	user_data_or_owner)
{
	g_free (user_data_or_owner);
}

static char *
convert_file_to_string (NautilusFile *file,
			gboolean cut)
{
	GString *uris;
	char *uri, *result;

	uris = g_string_new (cut ? "cut" : "copy");
	
	uri = nautilus_file_get_uri (file);
	g_string_append_c (uris, '\n');
	g_string_append (uris, uri);
	g_free (uri);

	result = uris->str;
	g_string_free (uris, FALSE);

	return result;
}

static void
copy_or_cut_files (FMTreeView *view,
		   gboolean cut)
{
	char *status_string, *name;
	char *clipboard_string;
	
	clipboard_string = convert_file_to_string (view->details->popup_file, cut);
	
	gtk_clipboard_set_with_data (get_clipboard (GTK_WIDGET (view->details->tree_widget)),
				     clipboard_targets, G_N_ELEMENTS (clipboard_targets),
				     get_clipboard_callback, clear_clipboard_callback,
				     clipboard_string);
	nautilus_clipboard_monitor_emit_changed ();
	
	name = nautilus_file_get_display_name (view->details->popup_file);
	if (cut) {
		status_string = g_strdup_printf (_("\"%s\" will be moved "
						   "if you select the Paste Files command"),
						 name);
	} else {
		status_string = g_strdup_printf (_("\"%s\" will be copied "
						   "if you select the Paste Files command"),
						 name);
	}
	g_free (name);
	
	nautilus_view_report_status (NAUTILUS_VIEW (view),
				     status_string);
	g_free (status_string);
}

static void
fm_tree_view_cut_cb (GtkWidget *menu_item,
		     FMTreeView *view)
{
	copy_or_cut_files (view, TRUE);
}

static void
fm_tree_view_copy_cb (GtkWidget *menu_item,
		      FMTreeView *view)
{
	copy_or_cut_files (view, FALSE);
}

static GList *
convert_lines_to_str_list (char **lines, gboolean *cut)
{
	int i;
	GList *result;

	if (lines[0] == NULL) {
		return NULL;
	}

	if (strcmp (lines[0], "cut") == 0) {
		*cut = TRUE;
	} else if (strcmp (lines[0], "copy") == 0) {
		*cut = FALSE;
	} else {
		return NULL;
	}

	result = NULL;
	for (i = 1; lines[i] != NULL; i++) {
		result = g_list_prepend (result, g_strdup (lines[i]));
	}
	return g_list_reverse (result);
}

static void
paste_clipboard_data (FMTreeView *view,
		      GtkSelectionData *selection_data,
		      char *destination_uri)
{
	char **lines;
	gboolean cut;
	GList *item_uris;
	
	if (selection_data->type != copied_files_atom
	    || selection_data->length <= 0) {
		item_uris = NULL;
	} else {
		/* Not sure why it's legal to assume there's an extra byte
		 * past the end of the selection data that it's safe to write
		 * to. But gtk_editable_selection_received does this, so I
		 * think it is OK.
		 */
		selection_data->data[selection_data->length] = '\0';
		lines = g_strsplit (selection_data->data, "\n", 0);
		item_uris = convert_lines_to_str_list (lines, &cut);
		g_strfreev (lines);
	}

	if (item_uris == NULL|| destination_uri == NULL) {
		nautilus_view_report_status (NAUTILUS_VIEW (view),
					     _("There is nothing on the clipboard to paste."));
	} else {
		nautilus_file_operations_copy_move
			(item_uris, NULL, destination_uri,
			 cut ? GDK_ACTION_MOVE : GDK_ACTION_COPY,
			 GTK_WIDGET (view->details->tree_widget),
			 NULL, NULL);
	}
}

static void
paste_into_clipboard_received_callback (GtkClipboard     *clipboard,
					GtkSelectionData *selection_data,
					gpointer          data)
{
	FMTreeView *view;
	char *directory_uri;

	view = FM_TREE_VIEW (data);

	directory_uri = nautilus_file_get_uri (view->details->popup_file);

	paste_clipboard_data (view, selection_data, directory_uri);

	g_free (directory_uri);
}

static void
fm_tree_view_paste_cb (GtkWidget *menu_item,
		       FMTreeView *view)
{
	gtk_clipboard_request_contents (get_clipboard (GTK_WIDGET (view->details->tree_widget)),
					copied_files_atom,
					paste_into_clipboard_received_callback, view);
}

static void
fm_tree_view_trash_cb (GtkWidget *menu_item,
		       FMTreeView *view)
{
	GList *list;
	char *directory_uri;

	directory_uri = nautilus_file_get_uri (view->details->popup_file);
	
	if (can_move_uri_to_trash (directory_uri))
	{
		list = g_list_prepend (NULL, g_strdup (directory_uri));
	
		nautilus_file_operations_copy_move (list, NULL, 
						    EEL_TRASH_URI, GDK_ACTION_MOVE, GTK_WIDGET (view->details->tree_widget),
						    NULL, NULL);
	}
	
	g_free (directory_uri);
}

static void
fm_tree_view_properties_cb (GtkWidget *menu_item,
			    FMTreeView *view)
{
	GList *list;
        
	list = g_list_prepend (NULL, nautilus_file_ref (view->details->popup_file));

	fm_properties_window_present (list, GTK_WIDGET (view->details->tree_widget));

        nautilus_file_list_free (list);
}

static void
volume_or_drive_unmounted_callback (gboolean succeeded,
				    char *error,
				    char *detailed_error,
				    gpointer data)
{
	gboolean eject;

	eject = GPOINTER_TO_INT (data);
	if (!succeeded) {
		if (eject) {
			eel_show_error_dialog_with_details (error, NULL, 
			                                    _("Eject Error"), detailed_error, NULL);
		} else {
			eel_show_error_dialog_with_details (error, NULL, 
			                                    _("Unmount Error"), detailed_error, NULL);
		}
	}
}


static void
fm_tree_view_unmount_cb (GtkWidget *menu_item,
			    FMTreeView *view)
{
	NautilusFile *file = view->details->popup_file;
	GnomeVFSVolume *volume;
	
	if (file == NULL) {
		return;
	}

	volume = fm_tree_model_get_volume_for_root_node_file (view->details->child_model, file);
	
	if (volume != NULL) {
		if (eject_for_type (gnome_vfs_volume_get_device_type (volume))) {
			gnome_vfs_volume_eject (volume, volume_or_drive_unmounted_callback, GINT_TO_POINTER (TRUE));
		} else {
			gnome_vfs_volume_unmount (volume, volume_or_drive_unmounted_callback, GINT_TO_POINTER (FALSE));
		}
	}
}

static void
create_popup_menu (FMTreeView *view)
{
	GtkWidget *popup, *menu_item, *menu_image, *separator_item;
	
	popup = gtk_menu_new ();
	
	/* add the "open" menu item */
	menu_image = gtk_image_new_from_stock (GTK_STOCK_OPEN,
					       GTK_ICON_SIZE_MENU);
	gtk_widget_show (menu_image);
	menu_item = gtk_image_menu_item_new_with_label (_("Open"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
				       menu_image);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_open_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_open = menu_item;
	
	/* add the "open in new window" menu item */
	menu_item = gtk_image_menu_item_new_with_label (_("Open in New Window"));
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_open_in_new_window_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_open_in_new_window = menu_item;
	
	separator_item = gtk_separator_menu_item_new ();
	gtk_widget_show (separator_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), separator_item);
	
	/* add the "create folder" menu item */
	menu_item = gtk_image_menu_item_new_with_label (_("Create Folder"));
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_create_folder_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_create_folder = menu_item;
	
	separator_item = gtk_separator_menu_item_new ();
	gtk_widget_show (separator_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), separator_item);
	
	/* add the "cut folder" menu item */
	menu_image = gtk_image_new_from_stock (GTK_STOCK_CUT,
					       GTK_ICON_SIZE_MENU);
	gtk_widget_show (menu_image);
	menu_item = gtk_image_menu_item_new_with_label (_("Cut Folder"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
				       menu_image);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_cut_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_cut = menu_item;
	
	/* add the "copy folder" menu item */
	menu_image = gtk_image_new_from_stock (GTK_STOCK_COPY,
					       GTK_ICON_SIZE_MENU);
	gtk_widget_show (menu_image);
	menu_item = gtk_image_menu_item_new_with_label (_("Copy Folder"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
				       menu_image);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_copy_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_copy = menu_item;
	
	/* add the "paste files into folder" menu item */
	menu_image = gtk_image_new_from_stock (GTK_STOCK_PASTE,
					       GTK_ICON_SIZE_MENU);
	gtk_widget_show (menu_image);
	menu_item = gtk_image_menu_item_new_with_label (_("Paste Files into Folder"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
				       menu_image);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_paste_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_paste = menu_item;
	
	separator_item = gtk_separator_menu_item_new ();
	gtk_widget_show (separator_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), separator_item);
	
	/* add the "move to trash" menu item */
	menu_image = gtk_image_new_from_stock (GTK_STOCK_DELETE,
					       GTK_ICON_SIZE_MENU);
	gtk_widget_show (menu_image);
	menu_item = gtk_image_menu_item_new_with_label (_("Move to Trash"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
				       menu_image);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_trash_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_trash = menu_item;
	
	separator_item = gtk_separator_menu_item_new ();
	gtk_widget_show (separator_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), separator_item);
	
	/* add the "properties" menu item */
	menu_image = gtk_image_new_from_stock (GTK_STOCK_PROPERTIES,
					       GTK_ICON_SIZE_MENU);
	gtk_widget_show (menu_image);
	menu_item = gtk_image_menu_item_new_with_label (_("Properties"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
				       menu_image);
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_properties_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_properties = menu_item;

	/* add the unmount separator menu item */
	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_unmount_separator = menu_item;
	
	/* add the "Unmount" menu item */
	menu_item = gtk_image_menu_item_new_with_label ("eject label");
	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (fm_tree_view_unmount_cb),
			  view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
	view->details->popup_unmount = menu_item;

	view->details->popup = popup;
}

static void
create_tree (FMTreeView *view)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GnomeVFSVolumeMonitor *volume_monitor;
	char *home_uri;
	GList *volumes, *l;
	
	view->details->child_model = fm_tree_model_new ();
	view->details->sort_model = GTK_TREE_MODEL_SORT
		(gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (view->details->child_model)));
	view->details->tree_widget = GTK_TREE_VIEW
		(gtk_tree_view_new_with_model (GTK_TREE_MODEL (view->details->sort_model)));
	g_object_unref (view->details->sort_model);
	g_signal_connect_object
		(view->details->child_model, "row_loaded",
		 G_CALLBACK (row_loaded_callback),
		 view, G_CONNECT_AFTER);
	home_uri = gnome_vfs_get_uri_from_local_path (g_get_home_dir ());
	fm_tree_model_add_root_uri (view->details->child_model, home_uri, _("Home Folder"), "gnome-home", NULL);
	g_free (home_uri);
	fm_tree_model_add_root_uri (view->details->child_model, "file:///", _("Filesystem"), "gnome-fs-directory", NULL);
#ifdef NOT_YET_USABLE
	fm_tree_model_add_root_uri (view->details->child_model, "network:///", _("Network Neighbourhood"), "gnome-fs-network", NULL);
#endif
	
	volume_monitor = gnome_vfs_get_volume_monitor ();
	volumes = gnome_vfs_volume_monitor_get_mounted_volumes (volume_monitor);
	for (l = volumes; l != NULL; l = l->next) {
		add_root_for_volume (view, l->data);
		gnome_vfs_volume_unref (l->data);
	}
	g_list_free (volumes);
	
	g_signal_connect_object (volume_monitor, "volume_mounted",
				 G_CALLBACK (volume_mounted_callback), view, 0);
	g_signal_connect_object (volume_monitor, "volume_unmounted",
				 G_CALLBACK (volume_unmounted_callback), view, 0);
	
	g_object_unref (view->details->child_model);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (view->details->sort_model),
						 compare_rows, view, NULL);

	gtk_tree_view_set_headers_visible (view->details->tree_widget, FALSE);

	view->details->drag_dest = 
		nautilus_tree_view_drag_dest_new (view->details->tree_widget);
	g_signal_connect_object (view->details->drag_dest, 
				 "get_root_uri",
				 G_CALLBACK (get_root_uri_callback),
				 view, 0);
	g_signal_connect_object (view->details->drag_dest, 
				 "get_file_for_path",
				 G_CALLBACK (get_file_for_path_callback),
				 view, 0);
	g_signal_connect_object (view->details->drag_dest,
				 "move_copy_items",
				 G_CALLBACK (move_copy_items_callback),
				 view, 0);

	/* Create column */
	column = gtk_tree_view_column_new ();

	cell = nautilus_cell_renderer_pixbuf_emblem_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "pixbuf", FM_TREE_MODEL_CLOSED_PIXBUF_COLUMN,
					     "pixbuf_expander_closed", FM_TREE_MODEL_CLOSED_PIXBUF_COLUMN,
					     "pixbuf_expander_open", FM_TREE_MODEL_OPEN_PIXBUF_COLUMN,
					     "pixbuf_emblem", FM_TREE_MODEL_EMBLEM_PIXBUF_COLUMN,
					     NULL);
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "text", FM_TREE_MODEL_DISPLAY_NAME_COLUMN,
					     "style", FM_TREE_MODEL_FONT_STYLE_COLUMN,
					     "weight", FM_TREE_MODEL_FONT_WEIGHT_COLUMN,
					     NULL);

	gtk_tree_view_append_column (view->details->tree_widget, column);

	gtk_widget_show (GTK_WIDGET (view->details->tree_widget));

	gtk_container_add (GTK_CONTAINER (view->details->scrolled_window),
			   GTK_WIDGET (view->details->tree_widget));

	g_signal_connect_object (gtk_tree_view_get_selection (GTK_TREE_VIEW (view->details->tree_widget)), "changed",
				 G_CALLBACK (selection_changed_callback), view, 0);

	g_signal_connect (G_OBJECT (view->details->tree_widget), 
			  "row-activated", G_CALLBACK (row_activated_callback),
			  view);

	g_signal_connect (G_OBJECT (view->details->tree_widget), 
			  "button_press_event", G_CALLBACK (button_pressed_callback),
			  view);

	schedule_show_selection (view);
}

static void
update_filtering_from_preferences (FMTreeView *view)
{
	Nautilus_ShowHiddenFilesMode mode;
	
	if (view->details->child_model == NULL) {
		return;
	}

	mode = nautilus_view_get_show_hidden_files_mode (NAUTILUS_VIEW (view));

	if (mode == Nautilus_SHOW_HIDDEN_FILES_DEFAULT) {
		fm_tree_model_set_show_hidden_files
			(view->details->child_model,
			 eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES));
	} else {
		fm_tree_model_set_show_hidden_files
			(view->details->child_model,
			 mode == Nautilus_SHOW_HIDDEN_FILES_ENABLE);
	}
	fm_tree_model_set_show_backup_files
		(view->details->child_model,
		 eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES));
	fm_tree_model_set_show_only_directories
		(view->details->child_model,
		 eel_preferences_get_boolean (NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES));
}

static void
tree_activate_callback (BonoboControl *control, gboolean activating, gpointer user_data)
{
	FMTreeView *view;

	view = FM_TREE_VIEW (user_data);

	if (activating && view->details->tree_widget == NULL) {
		create_tree (view);
		update_filtering_from_preferences (view);
	}
}

static void
filtering_changed_callback (gpointer callback_data)
{
	update_filtering_from_preferences (FM_TREE_VIEW (callback_data));
}

static void
load_location_callback (FMTreeView *view, char *location)
{
	if (view->details->selection_location != NULL) {
		g_free (view->details->selection_location);
	}
	view->details->selection_location = g_strdup (location);

	schedule_show_selection (view);
}

static void
fm_tree_view_instance_init (FMTreeView *view)
{
	BonoboControl *control;
	
	view->details = g_new0 (FMTreeViewDetails, 1);
	
	
	view->details->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view->details->scrolled_window), 
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	
	gtk_widget_show (view->details->scrolled_window);

	control = bonobo_control_new (view->details->scrolled_window);
	g_signal_connect_object (control, "activate",
				 G_CALLBACK (tree_activate_callback), view, 0);

	nautilus_view_construct_from_bonobo_control (NAUTILUS_VIEW (view), control);

	view->details->selection_location = NULL;
	g_signal_connect_object (view, "load_location",
				 G_CALLBACK (load_location_callback), view, 0);
	view->details->selecting = FALSE;

	eel_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
				      filtering_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
				      filtering_changed_callback, view);
	eel_preferences_add_callback (NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
				      filtering_changed_callback, view);
	g_signal_connect_object (view, "show_hidden_files_mode_changed",
				 G_CALLBACK (filtering_changed_callback), view, 0);  

	nautilus_view_set_listener_mask
		(NAUTILUS_VIEW (view),
		 NAUTILUS_VIEW_LISTEN_SHOW_HIDDEN_FILES_MODE);

	
	g_signal_connect_object (nautilus_icon_factory_get(), "icons_changed",
				 G_CALLBACK (theme_changed_callback), view, 0);  

	view->details->popup_file = NULL;
	create_popup_menu (view);
}

static void
fm_tree_view_dispose (GObject *object)
{
	FMTreeView *view;
	
	view = FM_TREE_VIEW (object);
	
	if (view->details->selection_changed_timer) {
		g_source_remove (view->details->selection_changed_timer);
		view->details->selection_changed_timer = 0;
	}

	if (view->details->drag_dest) {
		g_object_unref (view->details->drag_dest);
		view->details->drag_dest = NULL;
	}

	if (view->details->show_selection_idle_id) {
		g_source_remove (view->details->show_selection_idle_id);
		view->details->show_selection_idle_id = 0;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
fm_tree_view_finalize (GObject *object)
{
	FMTreeView *view;

	view = FM_TREE_VIEW (object);

	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES,
					 filtering_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES,
					 filtering_changed_callback, view);
	eel_preferences_remove_callback (NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
					 filtering_changed_callback, view);

	cancel_activation (view);
	gtk_widget_destroy (view->details->popup);

	if (view->details->selection_location != NULL) {
		g_free (view->details->selection_location);
	}

	g_free (view->details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
fm_tree_view_class_init (FMTreeViewClass *class)
{
	G_OBJECT_CLASS (class)->dispose = fm_tree_view_dispose;
	G_OBJECT_CLASS (class)->finalize = fm_tree_view_finalize;
	
	copied_files_atom = gdk_atom_intern ("x-special/gnome-copied-files", FALSE);
}
