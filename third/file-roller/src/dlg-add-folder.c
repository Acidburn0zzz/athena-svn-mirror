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
	GtkWidget *include_subfold_checkbutton;
	GtkWidget *add_if_newer_checkbutton;
	GtkWidget *exclude_symlinks;
	GtkWidget *include_files_checkbutton;
	GtkWidget *include_files_entry;
	GtkWidget *include_files_label;
	GtkWidget *exclude_files_entry;
	GtkWidget *exclude_files_label;
	GtkWidget *load_button;
	GtkWidget *save_button;
	char      *last_options;
} DialogData;


static void
open_file_destroy_cb (GtkWidget  *widget,
		      DialogData *data)
{
	g_free (data->last_options);
	g_free (data);
}


static gboolean
utf8_only_spaces (const char *text)
{
	const char *scan;
	
	if (text == NULL)
		return TRUE;

	for (scan = text; *scan != 0; scan = g_utf8_next_char (scan)) {
		gunichar c = g_utf8_get_char (scan);
		if (! g_unichar_isspace (c))
			return FALSE;
	}

	return TRUE;
}


static int
file_sel_response_cb (GtkWidget    *widget,
		      int           response,
		      DialogData   *data)
{
	GtkFileChooser *file_sel = GTK_FILE_CHOOSER (widget);
	FRWindow       *window = data->window;
	char           *current_folder;
	gboolean        update, recursive, follow_links;
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
		g_free (message);

		g_free (current_folder);

		return FALSE;
	}

	window_set_add_default_dir (window, current_folder);

	update = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->add_if_newer_checkbutton));
	recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->include_subfold_checkbutton));
	follow_links = ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->exclude_symlinks));

	/**/

	selections = gtk_file_chooser_get_filenames (file_sel);
	for (iter = selections; iter != NULL; iter = iter->next) {
		char *path = iter->data;
		item_list = g_list_prepend (item_list, path);
	}

	if (item_list != NULL) {
		const char *folder = (char*) item_list->data;
		const char *include_files = gtk_entry_get_text (GTK_ENTRY (data->include_files_entry));
		const char *exclude_files = gtk_entry_get_text (GTK_ENTRY (data->exclude_files_entry));
		char       *dest_dir;

		if (utf8_only_spaces (include_files))
			include_files = "*";
		if (utf8_only_spaces (exclude_files))
			exclude_files = NULL;

		if (strcmp (folder, current_folder) == 0)
			dest_dir = NULL;
		else
			dest_dir = g_build_path (G_DIR_SEPARATOR_S,
						 window_get_current_location (window),
						 file_name_from_path (folder),
						 NULL);
		
		window_archive_add_with_wildcard (window,
						  include_files,
						  exclude_files,
						  folder,
						  dest_dir,
						  update,
						  recursive,
						  follow_links,
						  window->password,
						  window->compression);

		g_free (dest_dir);
	}

	g_list_free (item_list);
	g_slist_foreach (selections, (GFunc) g_free, NULL);
	g_slist_free (selections);
	g_free (current_folder);

	gtk_widget_destroy (data->dialog);

	return TRUE;
}


static void
update_sensitivity (DialogData *data)
{
	GSList   *selections;
	gboolean  can_add_wildcard, wildcard_specified = FALSE;


	selections = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (data->dialog));
	can_add_wildcard = (selections != NULL);

	if (can_add_wildcard) {
		const char *files;
		gboolean    include_wildcard;
		gboolean    exclude_wildcard;

		files = gtk_entry_get_text (GTK_ENTRY (data->include_files_entry));
		include_wildcard = ((g_utf8_strchr (files, -1, '*') != NULL) 
				    || (g_utf8_strchr (files, -1, '?') != NULL));

		files = gtk_entry_get_text (GTK_ENTRY (data->exclude_files_entry));
		exclude_wildcard = ((g_utf8_strchr (files, -1, '*') != NULL) 
				    || (g_utf8_strchr (files, -1, '?') != NULL));
		wildcard_specified = include_wildcard || exclude_wildcard;
	}

	gtk_widget_set_sensitive (data->include_files_entry, can_add_wildcard);
	gtk_widget_set_sensitive (data->exclude_files_entry, can_add_wildcard);
	gtk_widget_set_sensitive (data->include_files_label, can_add_wildcard);
	gtk_widget_set_sensitive (data->exclude_files_label, can_add_wildcard);
	gtk_widget_set_sensitive (data->include_subfold_checkbutton, can_add_wildcard);
	gtk_widget_set_sensitive (data->exclude_symlinks, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->include_subfold_checkbutton)) && can_add_wildcard);

	g_slist_foreach (selections, (GFunc) g_free, FALSE);
	g_slist_free (selections);
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

	update_sensitivity (data);
}


static void
wildcard_entry_changed_cb (GtkWidget  *file_sel, 
			   DialogData *data)
{
	update_sensitivity (data);
}


static int
include_subfold_toggled_cb (GtkWidget *widget, 
			    gpointer   callback_data)
{
	DialogData *data = callback_data;
	gtk_widget_set_sensitive (data->exclude_symlinks,
				  GTK_TOGGLE_BUTTON (widget)->active);
	return FALSE;
}


static void load_options_cb (GtkWidget *w, DialogData *data);
static void save_options_cb (GtkWidget *w, DialogData *data);


/* create the "add" dialog. */
void
add_folder_cb (GtkWidget *widget, 
	       void      *callback_data)
{
	GtkWidget   *file_sel;
	DialogData  *data;
	gchar       *dir;
	GtkWidget   *main_box;
	GtkWidget   *vbox, *hbox;
	GtkWidget   *table;	
	GtkTooltips *tooltips;
	
	tooltips = gtk_tooltips_new ();
 
	data = g_new0 (DialogData, 1);
	data->window = callback_data;
	data->dialog = file_sel = 
		gtk_file_chooser_dialog_new (_("Add a Folder"),
					     GTK_WINDOW (data->window->app),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL, 
					     GTK_RESPONSE_CANCEL,
					     FR_STOCK_ADD, 
					     GTK_RESPONSE_OK,
					     NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (file_sel), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (file_sel), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (file_sel), GTK_RESPONSE_OK);

	data->add_if_newer_checkbutton = gtk_check_button_new_with_mnemonic (_("_Add only if newer"));
	data->include_subfold_checkbutton = gtk_check_button_new_with_mnemonic (_("_Include subfolders"));
	data->exclude_symlinks = gtk_check_button_new_with_mnemonic (_("Exclude folders that are symbolic lin_ks"));

	data->include_files_entry = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, data->include_files_entry, _("example: *.o; *.bak"), NULL);
	data->include_files_label = gtk_label_new_with_mnemonic (_("_Include files:"));
	gtk_misc_set_alignment (GTK_MISC (data->include_files_label), 0.0, 0.5);

	data->exclude_files_entry = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, data->exclude_files_entry, _("example: *.o; *.bak"), NULL);
	data->exclude_files_label = gtk_label_new_with_mnemonic (_("E_xclude files:"));
	gtk_misc_set_alignment (GTK_MISC (data->exclude_files_label), 0.0, 0.5);

	data->load_button = gtk_button_new_with_mnemonic (_("_Load Options"));
	data->save_button = gtk_button_new_with_mnemonic (_("Sa_ve Options"));

	main_box = gtk_hbox_new (FALSE, 20);
	gtk_container_set_border_width (GTK_CONTAINER (main_box), 0);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (file_sel), main_box);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), data->add_if_newer_checkbutton, 
			    TRUE, TRUE, 0);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (vbox), table,
			    TRUE, TRUE, 0);

	gtk_table_attach (GTK_TABLE (table), 
			  data->include_files_label,
			  0, 1,
			  0, 1,
			  GTK_FILL, 0,
			  0, 0);
	gtk_table_attach (GTK_TABLE (table),
			  data->include_files_entry, 
			  1, 2,
			  0, 1,
			  GTK_FILL|GTK_EXPAND, 0,
			  0, 0);
	gtk_table_attach (GTK_TABLE (table), 
			  data->exclude_files_label,
			  0, 1,
			  1, 2,
			  GTK_FILL, 0,
			  0, 0);
	gtk_table_attach (GTK_TABLE (table),
			  data->exclude_files_entry, 
			  1, 2,
			  1, 2,
			  GTK_FILL|GTK_EXPAND, 0,
			  0, 0);

	gtk_box_pack_start (GTK_BOX (vbox), data->include_subfold_checkbutton,
			    TRUE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), data->exclude_symlinks,
			    TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox,
			    TRUE, TRUE, 0);

	/**/
	
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (vbox), data->load_button,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), data->save_button,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (main_box);
	
	/* set data */

	dir = g_strconcat (data->window->add_default_dir, "/", "*", NULL);
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_sel), dir);
	g_free (dir);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->include_subfold_checkbutton), TRUE);

	update_sensitivity (data);

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

	g_signal_connect (G_OBJECT (data->include_files_entry), 
			  "changed",
			  G_CALLBACK (wildcard_entry_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->exclude_files_entry), 
			  "changed",
			  G_CALLBACK (wildcard_entry_changed_cb),
			  data);
	
	g_signal_connect (G_OBJECT (data->include_subfold_checkbutton),
			  "toggled", 
			  G_CALLBACK (include_subfold_toggled_cb),
			  data);

	g_signal_connect (G_OBJECT (data->load_button),
			  "clicked",
			  G_CALLBACK (load_options_cb), 
			  data);
	
	g_signal_connect (G_OBJECT (data->save_button),
			  "clicked",
			  G_CALLBACK (save_options_cb), 
			  data);
	
	g_object_set_data (G_OBJECT (file_sel), "tooltips", tooltips);

	gtk_window_set_modal (GTK_WINDOW (file_sel),TRUE);
	gtk_widget_show (file_sel);
}





typedef struct {
	DialogData   *data;
	GladeXML     *gui;
	GtkWidget    *dialog;
	GtkWidget    *aod_treeview;
	GtkTreeModel *aod_model;
} LoadOptionsDialogData;


static void
aod_destroy_cb (GtkWidget             *widget,
		LoadOptionsDialogData *aod_data)
{
	g_object_unref (aod_data->gui);
	g_free (aod_data);
}


static gboolean
config_get_bool (const char *config_file,
		 const char *option)
{
	char     *path;
	gboolean  value;
	
	path = g_strconcat ("=", config_file, "=/Options/", option, NULL);
	value = gnome_config_get_bool (path);
	g_free (path);
	
	return value;
}


static char*
config_get_string (const char *config_file,
		   const char *option)
{
	char *path;
	char *value;
	
	path = g_strconcat ("=", config_file, "=/Options/", option, NULL);
	value = gnome_config_get_string (path);
	g_free (path);
	
	return value;
}


static void
aod_apply_cb (GtkWidget *widget,
	      gpointer   callback_data)
{
	LoadOptionsDialogData *aod_data = callback_data;
	DialogData            *data = aod_data->data;
	GtkTreeSelection      *selection;
	GtkTreeIter            iter;
	char                  *file_path;
	char                  *include_files = NULL;
	char                  *exclude_files = NULL;
	gboolean               update;
	char                  *base_dir = NULL;
	char                  *filename = NULL;
	gboolean               recursive;
	gboolean               no_symlinks;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (aod_data->aod_treeview));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter)) 
		return;

	gtk_tree_model_get (aod_data->aod_model, &iter,
			    1, &file_path,
			    -1);

	g_free (data->last_options);
	data->last_options = g_strdup (file_name_from_path (file_path));

	/* Load options. */
	
	base_dir = config_get_string (file_path, "base_dir");
	filename = config_get_string (file_path, "filename");
	include_files = config_get_string (file_path, "include_files");
	exclude_files = config_get_string (file_path, "exclude_files");
	update = config_get_bool (file_path, "update");
	recursive = config_get_bool (file_path, "recursive");
	no_symlinks = config_get_bool (file_path, "no_symlinks");

	g_free (file_path);

	/* Sync widgets with options. */

	if (base_dir != NULL)
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (data->dialog), base_dir);
	/* FIXME
	if (filename != NULL)
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (data->dialog), filename);
	*/
	if (include_files != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->include_files_entry), include_files);
	if (exclude_files != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->exclude_files_entry), exclude_files);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->add_if_newer_checkbutton), update);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->include_subfold_checkbutton), recursive);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->exclude_symlinks), no_symlinks);
	
	g_free (base_dir);
	g_free (filename);
	g_free (include_files);
	g_free (exclude_files);

	/**/

	gtk_widget_destroy (aod_data->dialog);
}


static void
aod_activated_cb (GtkTreeView       *tree_view,
                  GtkTreePath       *path,
                  GtkTreeViewColumn *column,
                  gpointer           callback_data)
{
	aod_apply_cb (NULL, callback_data);
}


static void
aod_update_option_list (LoadOptionsDialogData *aod_data)
{
	GtkListStore   *list_store = GTK_LIST_STORE (aod_data->aod_model);
	char           *options_dir;
	GnomeVFSResult  result;
	GList          *list = NULL;
	
	gtk_list_store_clear (list_store);

	options_dir = get_home_relative_dir (RC_OPTIONS_DIR);
	ensure_dir_exists (options_dir, 0700);
	
	result = gnome_vfs_directory_list_load (&list, 
						options_dir,
						GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	if (result == GNOME_VFS_OK) {
		GList *scan;

		for (scan = list; scan; scan = scan->next) {
			GnomeVFSFileInfo *file_info = scan->data;
			GtkTreeIter       iter;
			char             *full_path;

			if ((strcmp (file_info->name, ".") == 0)
			    || (strcmp (file_info->name, "..") == 0))
				continue;
			
			full_path = g_build_filename (options_dir,
						      file_info->name,
						      NULL);
			gtk_list_store_append (GTK_LIST_STORE (aod_data->aod_model),
					       &iter);
			gtk_list_store_set (GTK_LIST_STORE (aod_data->aod_model), &iter,
					    0, file_info->name,
					    1, full_path,
					    -1);
			g_free (full_path);
		}
	}
	
	gnome_vfs_file_info_list_free (list);
	g_free (options_dir);
}


static void
aod_remove_cb (GtkWidget             *widget,
	       LoadOptionsDialogData *aod_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	char             *file_path;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (aod_data->aod_treeview));
	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (aod_data->aod_model, &iter,
			    1, &file_path,
			    -1);
	gtk_list_store_remove (GTK_LIST_STORE (aod_data->aod_model), &iter);

	gnome_vfs_unlink (file_path);
	g_free (file_path);
}


static void 
load_options_cb (GtkWidget  *w,
		 DialogData *data)
{
	LoadOptionsDialogData *aod_data;
	GtkWidget             *ok_button;
	GtkWidget             *cancel_button;
	GtkWidget             *remove_button;
	GtkCellRenderer       *renderer;
	GtkTreeViewColumn     *column;

	aod_data = g_new0 (LoadOptionsDialogData, 1);

	aod_data->data = data;
	aod_data->gui = glade_xml_new (GLADEDIR "/" GLADE_FILE , NULL, NULL);
        if (! aod_data->gui) {
                g_warning ("Could not find " GLADE_FILE "\n");
		g_free (aod_data);
                return;
        }

	/* Get the widgets. */

	aod_data->dialog = glade_xml_get_widget (aod_data->gui, "add_options_dialog");
	aod_data->aod_treeview = glade_xml_get_widget (aod_data->gui, "aod_treeview");

	ok_button = glade_xml_get_widget (aod_data->gui, "aod_okbutton");
	cancel_button = glade_xml_get_widget (aod_data->gui, "aod_cancelbutton");
	remove_button = glade_xml_get_widget (aod_data->gui, "aod_remove_button");

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (aod_data->dialog), 
			  "destroy",
			  G_CALLBACK (aod_destroy_cb),
			  aod_data);
	g_signal_connect (G_OBJECT (aod_data->aod_treeview),
                          "row_activated",
                          G_CALLBACK (aod_activated_cb),
                          aod_data);
	g_signal_connect_swapped (G_OBJECT (cancel_button), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (aod_data->dialog));
	g_signal_connect (G_OBJECT (ok_button), 
			  "clicked",
			  G_CALLBACK (aod_apply_cb),
			  aod_data);
	g_signal_connect (G_OBJECT (remove_button), 
			  "clicked",
			  G_CALLBACK (aod_remove_cb),
			  aod_data);

	/* Set data. */

	aod_data->aod_model = GTK_TREE_MODEL (gtk_list_store_new (2, 
								  G_TYPE_STRING,
								  G_TYPE_STRING));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (aod_data->aod_model),
                                              0,
					      GTK_SORT_ASCENDING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (aod_data->aod_treeview),
				 aod_data->aod_model);
	g_object_unref (aod_data->aod_model);

	/**/

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "text", 0,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (aod_data->aod_treeview),
				     column);
	
	aod_update_option_list (aod_data);

	/* Run */

	gtk_window_set_transient_for (GTK_WINDOW (aod_data->dialog), 
				      GTK_WINDOW (data->dialog));
	gtk_window_set_modal (GTK_WINDOW (aod_data->dialog), TRUE);
	gtk_widget_show (aod_data->dialog);
}





static void
config_set_bool (const char *config_file,
		 const char *option,
		 gboolean    value)
{
       char *path;
       
       path = g_strconcat ("=", config_file, "=/Options/", option, NULL);
       gnome_config_set_bool (path, value);

       g_free (path);
}


static void
config_set_string (const char *config_file,
		   const char *option,
		   const char *value)
{
	char *path;
	
	path = g_strconcat ("=", config_file, "=/Options/", option, NULL);
	gnome_config_set_string (path, value);

	g_free (path);
}


static void 
save_options_cb (GtkWidget  *w,
		 DialogData *data)
{
	char       *options_dir;
	char       *opt_filename;
	char       *opt_file_path;
	char       *base_dir;
	char       *filename;
	gboolean    update;
	gboolean    recursive;
	gboolean    no_symlinks;
	const char *include_files;
	const char *exclude_files;

	options_dir = get_home_relative_dir (RC_OPTIONS_DIR);
	ensure_dir_exists (options_dir, 0700);

	opt_filename = _gtk_request_dialog_run (
			GTK_WINDOW (data->dialog),
			GTK_DIALOG_MODAL,
			_("Save Options"),
			_("Options Name:"),
			(data->last_options != NULL)?data->last_options:"",
			1024,
			GTK_STOCK_CANCEL,
			GTK_STOCK_SAVE);

	if (opt_filename == NULL) 
		return;

	opt_file_path = g_build_filename (options_dir, opt_filename, NULL);
	g_free (opt_filename);

	/* Get options. */
	
	base_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (data->dialog));
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->dialog));
	update = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->add_if_newer_checkbutton));
	recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->include_subfold_checkbutton));
	no_symlinks = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->exclude_symlinks));

	include_files = gtk_entry_get_text (GTK_ENTRY (data->include_files_entry));
	if (utf8_only_spaces (include_files))
		include_files = NULL;

	exclude_files = gtk_entry_get_text (GTK_ENTRY (data->exclude_files_entry));
	if (utf8_only_spaces (exclude_files))
		exclude_files = NULL;
	
	/* Save options. */
	
	config_set_string (opt_file_path, "base_dir", base_dir);
	config_set_string (opt_file_path, "filename", filename);
	config_set_string (opt_file_path, "include_files", include_files);
	config_set_string (opt_file_path, "exclude_files", exclude_files);
	config_set_bool   (opt_file_path, "update", update);
	config_set_bool   (opt_file_path, "recursive", recursive);
	config_set_bool   (opt_file_path, "no_symlinks", no_symlinks);
	gnome_config_sync ();

	g_free (data->last_options);
	data->last_options = g_strdup (file_name_from_path (opt_file_path));

	/**/
	
	g_free (base_dir);
	g_free (filename);
	g_free (opt_file_path);
}
