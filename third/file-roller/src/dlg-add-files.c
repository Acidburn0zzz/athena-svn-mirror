/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <string.h>

#include <gtk/gtk.h>
#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include "file-utils.h"
#include "fr-stock.h"
#include "window.h"
#include "gtk-utils.h"


#define GLADE_FILE "file_roller.glade"


typedef struct {
	FRWindow  *window;
	GtkWidget *dialog;
	GtkWidget *add_if_newer_checkbutton;
} DialogData;


static void
open_file_destroy_cb (GtkWidget  *file_sel,
		      DialogData *data)
{
	g_free (data);
}


static int
file_sel_response_cb (GtkWidget      *widget,
		      int             response,
		      DialogData     *data)
{
	GtkFileChooser *file_sel = GTK_FILE_CHOOSER (widget);
	FRWindow       *window = data->window;
	char           *current_folder;
	gboolean        update;
	GSList         *selections, *iter;
	GList          *item_list = NULL;

	if ((response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT)) {
		gtk_widget_destroy (data->dialog);
		return TRUE;
	}

	current_folder = gtk_file_chooser_get_current_folder (file_sel);

	/* check folder permissions. */

	if (path_is_dir (current_folder) 
	    && access (current_folder, R_OK | X_OK) != 0) {
		GtkWidget *d;
		char      *utf8_path;
		char      *message;

		utf8_path = g_filename_to_utf8 (current_folder, -1, NULL, NULL, NULL);
		message = g_strdup_printf (_("You don't have the right permissions to read files from folder \"%s\""), utf8_path);
		g_free (utf8_path);

		d = _gtk_message_dialog_new (GTK_WINDOW (window->app),
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_DIALOG_ERROR,
					     _("Could not add the files to the archive"),
					     message,
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
					     NULL);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (current_folder);

		return FALSE;
	}

	window_set_add_default_dir (window, current_folder);

	update = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->add_if_newer_checkbutton));

	/**/

	selections = gtk_file_chooser_get_filenames (file_sel);
	for (iter = selections; iter != NULL; iter = iter->next) {
		char *path = iter->data;
		item_list = g_list_prepend (item_list, path);
	}

	if (item_list != NULL) 
		window_archive_add_items (window, item_list, update);

	g_list_free (item_list);
	g_slist_foreach (selections, (GFunc) g_free, NULL);
	g_slist_free (selections);
	g_free (current_folder);

	gtk_widget_destroy (data->dialog);

	return TRUE;
}


static void
selection_changed_cb (GtkWidget  *file_sel, 
 		      DialogData *data)
{
	FRWindow   *window = data->window;
	char       *current_folder;

        current_folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (file_sel));

        /* check folder permissions. */
                                                                                                
        if (path_is_dir (current_folder)
            && access (current_folder, R_OK | X_OK) != 0) {
                GtkWidget *d;
                char      *utf8_path;
                char      *message;
		
                utf8_path = g_filename_to_utf8 (current_folder, -1, NULL, NULL, NULL);
                message = g_strdup_printf (_("You don't have the right permissions to read files from folder \"%s\""), utf8_path);
                g_free (utf8_path);
		
                d = _gtk_message_dialog_new (GTK_WINDOW (window->app),
                                             GTK_DIALOG_MODAL,
                                             GTK_STOCK_DIALOG_ERROR,
                                             _("Could not add the files to the archive"),
                                             message,
                                             GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
                                             NULL);
                gtk_dialog_run (GTK_DIALOG (d));
                gtk_widget_destroy (GTK_WIDGET (d));
                g_free (message);
		
                g_free (current_folder);
        }
}


/* create the "add" dialog. */
void
add_files_cb (GtkWidget *widget, 
	      void      *callback_data)
{
	GtkWidget   *file_sel;
	DialogData  *data;
	char        *dir;
	GtkWidget   *main_box;
 
	data = g_new0 (DialogData, 1);
	data->window = callback_data;
	data->dialog = file_sel = 
		gtk_file_chooser_dialog_new (_("Add Files"),
					     GTK_WINDOW (data->window->app),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL, 
					     GTK_RESPONSE_CANCEL,
					     FR_STOCK_ADD, 
					     GTK_RESPONSE_OK,
					     NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (file_sel), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (file_sel), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (file_sel), GTK_RESPONSE_OK);

	data->add_if_newer_checkbutton = gtk_check_button_new_with_mnemonic (_("_Add only if newer"));

	main_box = gtk_hbox_new (FALSE, 20);
	gtk_container_set_border_width (GTK_CONTAINER (main_box), 0);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (file_sel), main_box);

	gtk_box_pack_start (GTK_BOX (main_box), data->add_if_newer_checkbutton, 
			    TRUE, TRUE, 0);

	gtk_widget_show_all (main_box);
	
	/* set data */

	dir = g_strconcat (data->window->add_default_dir, "/", "*", NULL);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_sel), dir);
	g_free (dir);

	/* signals */
	
	g_signal_connect (G_OBJECT (file_sel),
			  "destroy", 
			  G_CALLBACK (open_file_destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (file_sel),
			  "response",
			  G_CALLBACK (file_sel_response_cb),
			  data);

	g_signal_connect (G_OBJECT (file_sel), 
			  "selection-changed",
			  G_CALLBACK (selection_changed_cb),
			  data);

	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_widget_show (file_sel);
}
