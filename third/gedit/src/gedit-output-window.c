/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-output_window.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-popup-menu.h>

#include "gedit-output-window.h"
#include "gedit-debug.h"

struct _GeditOutputWindowPrivate
{
	GtkTooltips 	*tooltips;
	
	GtkWidget 	*close_button;
	GtkWidget 	*copy_button;
	GtkWidget 	*clear_button;

	GtkWidget 	*close_menu_item;
	GtkWidget 	*copy_menu_item;
	GtkWidget 	*clear_menu_item;

	GtkWidget 	*treeview;
	GtkTreeModel 	*model;
};

enum {
	CLOSE_REQUESTED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL];

enum
{
	COLUMN_LINES,
	NUM_COLUMNS
};

static void gedit_output_window_class_init 	(GeditOutputWindowClass	*klass);
static void gedit_output_window_init 		(GeditOutputWindow 	*output_window);
static void gedit_output_window_finalize 	(GObject 	*object);
static void gedit_output_window_destroy	 	(GtkObject 	*object);

static GtkHBoxClass *parent_class = NULL;


GType
gedit_output_window_get_type (void)
{
	static GType output_window_type = 0;

  	if (output_window_type == 0)
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditOutputWindowClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_output_window_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditOutputWindow),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_output_window_init
      		};

      		output_window_type = g_type_register_static (GTK_TYPE_HBOX,
                				    "GeditOutputWindow",
                                       	 	    &our_info,
                                       		    0);
    	}

	return output_window_type;
}

static void
gedit_output_window_class_init (GeditOutputWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_output_window_finalize;
	
	GTK_OBJECT_CLASS (klass)->destroy = gedit_output_window_destroy;

	signals[CLOSE_REQUESTED] = 
		g_signal_new ("close_requested",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditOutputWindowClass, close_requested),
			      NULL, 
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
}

static void		 
gedit_output_window_copy_selection (GeditOutputWindow *ow)
{
	gboolean ret;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	GString *string = NULL;

	g_return_if_fail (GEDIT_IS_OUTPUT_WINDOW (ow));

	selection = gtk_tree_view_get_selection (
			GTK_TREE_VIEW (ow->priv->treeview));

	ret = gtk_tree_model_get_iter_first (ow->priv->model, &iter);
	
	while (ret)
	{
		if (gtk_tree_selection_iter_is_selected (selection, &iter))
		{
			gchar *line;
			
			gtk_tree_model_get (ow->priv->model, &iter, COLUMN_LINES, &line, -1);

			if (string == NULL)
				string = g_string_new (line);
			else
				string = g_string_append (string, line);
			
			string = g_string_append_c (string, '\n');
			
			g_free (line);
		}

		ret = gtk_tree_model_iter_next (ow->priv->model, &iter);
	}

	if (string != NULL)
	{
		gchar *text;

		pango_parse_markup (string->str, string->len, 0, NULL, &text, NULL, NULL);
		
		gtk_clipboard_set_text (gtk_widget_get_clipboard (
						GTK_WIDGET (ow), GDK_SELECTION_CLIPBOARD),
			      		text, 
					-1);

		g_free (text);
	}

	g_string_free (string, TRUE);
}

static void
close_clicked_callback (GtkWidget *widget, gpointer user_data)
{
	GeditOutputWindow *ow;
	
	ow = GEDIT_OUTPUT_WINDOW (user_data);

	g_signal_emit (ow, signals [CLOSE_REQUESTED], 0);
}

static void
clear_clicked_callback (GtkWidget *widget, gpointer user_data)
{
	GeditOutputWindow *ow;
	
	ow = GEDIT_OUTPUT_WINDOW (user_data);

	gedit_output_window_clear (ow);
}

static void
copy_clicked_callback (GtkWidget *widget, gpointer user_data)
{
	GeditOutputWindow *ow;
	
	ow = GEDIT_OUTPUT_WINDOW (user_data);

	gedit_output_window_copy_selection (ow);
}

static gboolean 
gedit_output_window_key_press_event_cb (GtkTreeView *widget, GdkEventKey *event, 
		GeditOutputWindow *ow)
{
	if (event->keyval == GDK_Delete)
	{
		gedit_output_window_clear (ow);
		return TRUE;
	}

	if (event->keyval == 'c')
	{
		gedit_output_window_copy_selection (ow);
		return TRUE;
	}
	
	return FALSE;
}

static GtkWidget *
create_popup_menu (GeditOutputWindow  *output_window)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	
	menu = gtk_menu_new ();

	/* Add the clear button */
	output_window->priv->clear_menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLEAR, NULL);
	gtk_widget_show (output_window->priv->clear_menu_item);
	g_signal_connect (G_OBJECT (output_window->priv->clear_menu_item), "activate",
		      	  G_CALLBACK (clear_clicked_callback), output_window);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), output_window->priv->clear_menu_item);

	/* Add the copy button */
	output_window->priv->copy_menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
	gtk_widget_show (output_window->priv->copy_menu_item);
	g_signal_connect (G_OBJECT (output_window->priv->copy_menu_item), "activate",
		      	  G_CALLBACK (copy_clicked_callback), output_window);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), output_window->priv->copy_menu_item);

	/* Add the separator */
	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the close button */
	output_window->priv->close_menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE, NULL);
	gtk_widget_show (output_window->priv->close_menu_item);
	g_signal_connect (G_OBJECT (output_window->priv->close_menu_item), "activate",
		      	  G_CALLBACK (close_clicked_callback), output_window);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), output_window->priv->close_menu_item);
	
	gtk_widget_set_sensitive (output_window->priv->copy_menu_item, FALSE);
	gtk_widget_set_sensitive (output_window->priv->clear_menu_item, FALSE);

	return menu;
}

static void
gedit_output_window_treeview_selection_changed (GtkTreeSelection *selection, 
						GeditOutputWindow  *output_window)
{
	gboolean selected;
	
	g_return_if_fail (output_window != NULL);
	g_return_if_fail (selection != NULL);

	selected = (gtk_tree_selection_count_selected_rows (selection) > 0);
	
	gtk_widget_set_sensitive (output_window->priv->copy_menu_item, selected);
	gtk_widget_set_sensitive (output_window->priv->copy_button, selected);
}

static void 
gedit_output_window_init (GeditOutputWindow  *output_window)
{
	GtkSettings 		*settings;
	gint 			 w, h;
	GtkWidget 		*vbox1;
	GtkWidget 		*image;
	GtkWidget 		*hbox2;
	GtkWidget		*vseparator;
	GtkWidget 		*vbox2;
	GtkWidget		*scrolledwindow;
	GtkTreeViewColumn	*column;
	GtkCellRenderer 	*cell;
	GtkTreeSelection 	*selection;
	GtkWidget 		*popup_menu;

	GList			*focusable_widgets = NULL;

	gedit_debug (DEBUG_MDI, "");

	output_window->priv = g_new0 (GeditOutputWindowPrivate, 1);

	output_window->priv->tooltips = gtk_tooltips_new ();

	g_object_ref (G_OBJECT (output_window->priv->tooltips ));
	gtk_object_sink (GTK_OBJECT (output_window->priv->tooltips ));

	settings = gtk_widget_get_settings (GTK_WIDGET (output_window));

	gtk_icon_size_lookup_for_settings (settings,
					   GTK_ICON_SIZE_SMALL_TOOLBAR,
					   &w, &h);

	vbox1 = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 2);

	/* Create the close button */
	output_window->priv->close_button = gtk_button_new ();
	gtk_box_pack_start (GTK_BOX (vbox1), output_window->priv->close_button, FALSE, FALSE, 0);
	gtk_widget_set_size_request (output_window->priv->close_button, w + 2, h + 2);

	gtk_tooltips_set_tip (output_window->priv->tooltips, 
			      output_window->priv->close_button, 
			      _("Close the output window"), 
			      NULL);

	gtk_button_set_relief (GTK_BUTTON (output_window->priv->close_button), GTK_RELIEF_NONE);

	image = gtk_image_new_from_stock ("gtk-close", GTK_ICON_SIZE_SMALL_TOOLBAR);
  	gtk_container_add (GTK_CONTAINER (output_window->priv->close_button), image);
	
	g_signal_connect (output_window->priv->close_button, 
			  "clicked",
			  G_CALLBACK (close_clicked_callback),
			  output_window);

	/* Create the 3 vertical separators */
	hbox2 = gtk_hbox_new (TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox2, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox2), 4);

	vseparator = gtk_vseparator_new ();
	gtk_box_pack_start (GTK_BOX (hbox2), vseparator, FALSE, FALSE, 0);

	vseparator = gtk_vseparator_new ();
	gtk_box_pack_start (GTK_BOX (hbox2), vseparator, FALSE, FALSE, 0);

	vseparator = gtk_vseparator_new ();
  	gtk_box_pack_start (GTK_BOX (hbox2), vseparator, FALSE, TRUE, 0);

	/* Create the vbox for the copy and clear buttons */
	vbox2 = gtk_vbox_new (TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, TRUE, 0);

	/* Create the copy button */
	output_window->priv->copy_button = gtk_button_new ();
	gtk_box_pack_start (GTK_BOX (vbox2), output_window->priv->copy_button, FALSE, FALSE, 0);
  	gtk_widget_set_size_request (output_window->priv->copy_button, w + 2, h + 2);

	gtk_tooltips_set_tip (output_window->priv->tooltips, 
			      output_window->priv->copy_button, 
			      _("Copy selected lines"), 
			      NULL);

	gtk_button_set_relief (GTK_BUTTON (output_window->priv->copy_button), GTK_RELIEF_NONE);

	image = gtk_image_new_from_stock ("gtk-copy", GTK_ICON_SIZE_SMALL_TOOLBAR);
  	gtk_container_add (GTK_CONTAINER (output_window->priv->copy_button), image);

	g_signal_connect (output_window->priv->copy_button, 
			  "clicked",
			  G_CALLBACK (copy_clicked_callback),
			  output_window);

	/* Create the clear button */
	output_window->priv->clear_button = gtk_button_new ();
	gtk_box_pack_start (GTK_BOX (vbox2), output_window->priv->clear_button, FALSE, FALSE, 0);
	gtk_widget_set_size_request (output_window->priv->clear_button, w + 2, h + 2);
	
	gtk_tooltips_set_tip (output_window->priv->tooltips, 
			      output_window->priv->clear_button, 
			      _("Clear the output window"), 
			      NULL);
	
	gtk_button_set_relief (GTK_BUTTON (output_window->priv->clear_button), GTK_RELIEF_NONE);

	image = gtk_image_new_from_stock ("gtk-clear", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_container_add (GTK_CONTAINER (output_window->priv->clear_button), image);

	g_signal_connect (output_window->priv->clear_button, 
			  "clicked",
			  G_CALLBACK (clear_clicked_callback),
			  output_window);

	gtk_widget_set_sensitive (output_window->priv->copy_button, FALSE);
	gtk_widget_set_sensitive (output_window->priv->clear_button, FALSE);

  	/* Create the scrolled window */
	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
					GTK_POLICY_AUTOMATIC, 
					GTK_POLICY_AUTOMATIC);
	
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), 
					     GTK_SHADOW_ETCHED_IN);

	output_window->priv->treeview = gtk_tree_view_new ();
  	gtk_container_add (GTK_CONTAINER (scrolledwindow), output_window->priv->treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (output_window->priv->treeview), FALSE);


	/* List */
	output_window->priv->model = GTK_TREE_MODEL (
			gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING));

	gtk_tree_view_set_model (GTK_TREE_VIEW (output_window->priv->treeview), 
				 output_window->priv->model);

	/* Add the suggestions column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Output Lines"), cell, 
			"markup", COLUMN_LINES, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (output_window->priv->treeview), column);

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (output_window->priv->treeview),
			COLUMN_LINES);

	selection = gtk_tree_view_get_selection (
			GTK_TREE_VIEW (output_window->priv->treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	
	gtk_box_pack_end (GTK_BOX (output_window), scrolledwindow, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (output_window), vbox1, FALSE, FALSE, 0);

	gtk_widget_set_size_request (GTK_WIDGET (output_window), 3 * w, 5 * (h + 2));

	g_signal_connect (G_OBJECT (output_window->priv->treeview), "key_press_event",
			  G_CALLBACK (gedit_output_window_key_press_event_cb), output_window);

	g_signal_connect (G_OBJECT (selection), "changed", 
			  G_CALLBACK (gedit_output_window_treeview_selection_changed), 
			  output_window);

	focusable_widgets = g_list_append (focusable_widgets, output_window->priv->treeview);
	focusable_widgets = g_list_append (focusable_widgets, output_window->priv->close_button);
	focusable_widgets = g_list_append (focusable_widgets, output_window->priv->copy_button);
	focusable_widgets = g_list_append (focusable_widgets, output_window->priv->clear_button);

	gtk_container_set_focus_chain (GTK_CONTAINER (output_window), focusable_widgets);

	g_list_free (focusable_widgets);

	popup_menu = create_popup_menu (output_window);
		
	gnome_popup_menu_attach (popup_menu, output_window->priv->treeview, NULL);
}

static void 
gedit_output_window_finalize (GObject *object)
{
	GeditOutputWindow *ow;

	ow = GEDIT_OUTPUT_WINDOW (object);

	g_object_unref (ow->priv->tooltips);
	
	if (ow->priv != NULL)
	{
		g_free (ow->priv);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void 
gedit_output_window_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gedit_output_window_new	(void)
{
	return gtk_widget_new (GEDIT_TYPE_OUTPUT_WINDOW, NULL);
}

void		 
gedit_output_window_clear (GeditOutputWindow *ow)
{
	g_return_if_fail (GEDIT_IS_OUTPUT_WINDOW (ow));

	gtk_list_store_clear (GTK_LIST_STORE (ow->priv->model));

	gtk_widget_set_sensitive (ow->priv->clear_button, FALSE);
	gtk_widget_set_sensitive (ow->priv->clear_menu_item, FALSE);
}

void
gedit_output_window_append_line	(GeditOutputWindow *ow, const gchar *line, gboolean scroll)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;

	g_return_if_fail (GEDIT_IS_OUTPUT_WINDOW (ow));
	g_return_if_fail (line != NULL);
	
	store = GTK_LIST_STORE (ow->priv->model);
	g_return_if_fail (store != NULL);

	gtk_list_store_append (store, &iter);

	gtk_list_store_set (store, &iter, COLUMN_LINES, line, -1);

	gtk_widget_set_sensitive (ow->priv->clear_button, TRUE);
	gtk_widget_set_sensitive (ow->priv->clear_menu_item, TRUE);

	if (!scroll)
		return;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	g_return_if_fail (path != NULL);
		
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (ow->priv->treeview),
				      path, 
				      NULL,
				      TRUE,
				      0.0,
				      0.0);

	gtk_tree_path_free (path);
}

