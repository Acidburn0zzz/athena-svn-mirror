/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 * Copyright (C) 2000 Eazel, Inc
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
 *       Mathieu Lacage <mathieu@eazel.com>
 */

/* nautilus-tree-view-dnd.c: dnd code for the tree view.
 */

#include <config.h>
#include "nautilus-tree-view-dnd.h"

#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkdnd.h>
#include <libgnome/gnome-i18n.h>
#include <libnautilus-extensions/nautilus-glib-extensions.h>
#include <libnautilus-extensions/nautilus-gtk-extensions.h>
#include <libnautilus-extensions/nautilus-drag.h>
#include <libnautilus-extensions/nautilus-file.h>
#include <libnautilus-extensions/nautilus-background.h>
#include <libnautilus-extensions/nautilus-file-operations.h>

#define nopeMATHIEU_DEBUG 1


/* This constant is zero because right now it does not seem we need
   extra delay on horizontal-only auto-scroll. However, it's left in
   so we can easily add this delay if we decide we want it. */

#define AUTOSCROLL_X_ONLY_EXTRA_DELAY 0
/* in microseconds */


static void     nautilus_tree_view_drag_begin   (GtkWidget            *widget,
						 GdkDragContext       *context,
						 gpointer              user_data);
static void     nautilus_tree_view_drag_end     (GtkWidget            *widget,
						 GdkDragContext       *context,
						 gpointer              user_data);
static void     nautilus_tree_view_drag_leave   (GtkWidget            *widget,
						 GdkDragContext       *context,
						 guint                 time,
						 gpointer              user_data);
static gboolean nautilus_tree_view_drag_motion  (GtkWidget            *widget,
						 GdkDragContext       *context,
						 int                   x,
						 int                   y,
						 guint                 time,
						 gpointer              user_data);
static gboolean nautilus_tree_view_drag_drop    (GtkWidget            *widget,
						 GdkDragContext       *context,
						 int                   x,
						 int                   y,
						 guint                 time,
						 gpointer              user_data);
static void     nautilus_tree_view_drag_data_received   (GtkWidget            *widget,
							 GdkDragContext       *context,
							 gint                   x,
							 gint                   y,
							 GtkSelectionData     *data,
							 guint                 info,
							 guint                 time);
static void     nautilus_tree_view_drag_data_get        (GtkWidget            *widget,
							 GdkDragContext       *context,
							 GtkSelectionData     *data,
							 guint                 info,
							 guint                 time,
							 gpointer              user_data);

static int      nautilus_tree_view_button_release       (GtkWidget            *widget, 
							 GdkEventButton       *event);
static int      nautilus_tree_view_button_press         (GtkWidget            *widget, 
							 GdkEventButton       *event);
static int      nautilus_tree_view_motion_notify        (GtkWidget            *widget, 
							 GdkEventButton       *event);
static char    *nautilus_tree_view_item_at              (NautilusTreeView     *tree_view,
							 int                   x, 
							 int                   y);
static void     nautilus_tree_view_move_copy_files    (NautilusTreeView         *tree_view,
						       GList                    *selection_list,
						       GdkDragContext           *context,
						       const char               *target_uri);
static char    *nautilus_tree_view_find_drop_target   (NautilusTreeView         *tree_view,
						       int                       x, 
						       int                       y);
static char    *nautilus_tree_view_get_drag_uri           (NautilusTreeView      *tree_view);

static void     nautilus_tree_view_expand_or_collapse_row (NautilusCTree         *tree, 
							   int                    row);
static gboolean nautilus_tree_view_is_tree_node_directory (NautilusTreeView      *tree_view,
							   NautilusCTreeNode     *node);
static NautilusCTreeNode *nautilus_tree_view_tree_node_at (NautilusTreeView      *tree_view,
							   int                    x, 
							   int                    y);


static void    nautilus_tree_view_start_auto_scroll       (NautilusTreeView      *tree_view);

static void    nautilus_tree_view_stop_auto_scroll        (NautilusTreeView      *tree_view);

static void    nautilus_tree_view_real_scroll             (NautilusTreeView      *tree_view, 
							   float                  x_delta, 
							   float                  y_delta);

static void    nautilus_tree_view_receive_dropped_icons   (NautilusTreeView      *view,
							   GdkDragContext        *context,
							   int                    x, 
							   int                    y);

static void  nautilus_tree_view_make_prelight_if_file_operation (NautilusTreeView *tree_view, 
								 int x, 
								 int y);
static void  nautilus_tree_view_ensure_drag_data (NautilusTreeView *tree_view,
						  GdkDragContext *context,
						  guint32 time);

static void  nautilus_tree_view_get_drop_action (NautilusTreeView    *tree_view, 
						 GdkDragContext      *context,
						 int                  x, 
						 int                  y,
						 int                 *default_action,
						 int                 *non_default_action);
static void  nautilus_tree_view_collapse_all    (NautilusTreeView    *tree_view,
						 NautilusCTreeNode   *current_node);

static void  nautilus_tree_view_set_dnd_icon    (NautilusTreeView    *tree_view,
						 GdkDragContext      *context);

static void nautilus_tree_view_drag_destroy     (NautilusTreeView    *tree_view);
static void nautilus_tree_view_drag_destroy_real (NautilusTreeView   *tree_view);

static GtkTargetEntry nautilus_tree_view_dnd_target_table[] = {
	{ NAUTILUS_ICON_DND_GNOME_ICON_LIST_TYPE, 0, NAUTILUS_ICON_DND_GNOME_ICON_LIST },
	{ NAUTILUS_ICON_DND_URI_LIST_TYPE, 0, NAUTILUS_ICON_DND_URI_LIST }
};


static void 
tree_view_realize_callback (GtkWidget *widget, gpointer user_data) 
{
	GtkStyle *style, *new_style;
	NautilusTreeView *tree_view;
	GdkColor new_prelight_color;
	int i;

	tree_view = NAUTILUS_TREE_VIEW (user_data);	
	style = gtk_widget_get_style (widget);
	tree_view->details->dnd->normal_style = style;
	gtk_style_ref (style);

	new_style = gtk_style_copy (style);
        gtk_style_ref (new_style);

	/* calculate a new prelighting color */
	nautilus_gtk_style_shade (&style->bg[GTK_STATE_SELECTED], 
				  &new_prelight_color, 
				  1.35);
	/* set the new color to our special prelighting Style. */
        for (i = 0; i < 5; i++) {
		new_style->bg[i].red = new_prelight_color.red;
		new_style->bg[i].green = new_prelight_color.green;
		new_style->bg[i].blue = new_prelight_color.blue;
		new_style->base[i].red = new_prelight_color.red;
		new_style->base[i].green = new_prelight_color.green;
		new_style->base[i].blue = new_prelight_color.blue;
		new_style->fg[i].red = style->fg[GTK_STATE_SELECTED].red;
		new_style->fg[i].green = style->fg[GTK_STATE_SELECTED].green;
		new_style->fg[i].blue = style->fg[GTK_STATE_SELECTED].blue;
        }

	tree_view->details->dnd->highlight_style = new_style;
}


void
nautilus_tree_view_init_dnd (NautilusTreeView *view)
{
	view->details->dnd = g_new0 (NautilusTreeViewDndDetails, 1);
	view->details->dnd->expanded_nodes = NULL;

	view->details->dnd->drag_info = g_new0 (NautilusDragInfo, 1);
	nautilus_drag_init (view->details->dnd->drag_info,
			    nautilus_tree_view_dnd_target_table,
			    NAUTILUS_N_ELEMENTS (nautilus_tree_view_dnd_target_table),
			    NULL);


	gtk_drag_dest_set (GTK_WIDGET (view->details->tree), 
			   0,
			   nautilus_tree_view_dnd_target_table,
			   NAUTILUS_N_ELEMENTS (nautilus_tree_view_dnd_target_table),
			   GDK_ACTION_COPY 
			   | GDK_ACTION_MOVE 
			   | GDK_ACTION_LINK 
			   | GDK_ACTION_ASK);


	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_begin", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_begin), 
			    view);
	
	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_end", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_end), 
			    view);
	
	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_leave", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_leave), 
			    view);

	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_motion", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_motion), 
			    view);

	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_drop", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_drop), 
			    view);

	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_data_received", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_data_received), 
			    view);

	gtk_signal_connect (GTK_OBJECT (view->details->tree), 
			    "drag_data_get", 
			    GTK_SIGNAL_FUNC(nautilus_tree_view_drag_data_get), 
			    view);

	/* override the default handlers */
	gtk_signal_connect (GTK_OBJECT (view->details->tree),
			    "button-press-event", 
			    GTK_SIGNAL_FUNC (nautilus_tree_view_button_press), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (view->details->tree),
			    "button-release-event", 
			    GTK_SIGNAL_FUNC (nautilus_tree_view_button_release), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (view->details->tree),
			    "motion-notify-event", 
			    GTK_SIGNAL_FUNC (nautilus_tree_view_motion_notify), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (view->details->tree), "realize", 
			    tree_view_realize_callback, view);
}


void
nautilus_tree_view_free_dnd (NautilusTreeView *view)
{
	/* you do not need to unref the normal style */
	if (view->details->dnd->highlight_style != NULL) {
		gtk_style_unref (view->details->dnd->highlight_style);
	}
	nautilus_drag_finalize (view->details->dnd->drag_info);
	g_free (view->details->dnd);
}


static void     
nautilus_tree_view_drag_begin (GtkWidget *widget, GdkDragContext *context,
			       gpointer user_data)
{
	NautilusTreeView *tree_view;
	NautilusTreeViewDndDetails *dnd;

	tree_view = NAUTILUS_TREE_VIEW (user_data);
	dnd = tree_view->details->dnd;

	/* The drag is started. reinit the drag data. */
	dnd->drag_pending = FALSE;

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_begin");

	dnd->drag_info->got_drop_data_type = FALSE;

	nautilus_tree_view_set_dnd_icon (NAUTILUS_TREE_VIEW (tree_view), context);
}



static void
nautilus_tree_view_drag_end (GtkWidget *widget, GdkDragContext *context,
			     gpointer user_data)
{
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_end");

}


static void
nautilus_tree_view_drag_leave (GtkWidget *widget,
			       GdkDragContext *context,
			       guint time,
			       gpointer user_data)
{
	NautilusTreeView *tree_view;

	g_assert (NAUTILUS_IS_TREE_VIEW (user_data));

	tree_view = NAUTILUS_TREE_VIEW (user_data);

	nautilus_tree_view_drag_destroy (tree_view);

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_leave");
}

static gboolean 
nautilus_tree_view_drag_motion (GtkWidget *widget, GdkDragContext *context,
				int x, int y, guint time, gpointer user_data)
{
	NautilusTreeView *tree_view;
	NautilusTreeViewDndDetails *dnd;
	NautilusDragInfo *drag_info;

	int resulting_action, default_action, non_default_action;

	tree_view = NAUTILUS_TREE_VIEW (user_data);
	dnd = (NautilusTreeViewDndDetails *) (tree_view->details->dnd);
	drag_info = dnd->drag_info;

	/* destroy the data from the previous drag */
	if (drag_info->need_to_destroy) {
		drag_info->need_to_destroy = FALSE;
		nautilus_tree_view_drag_destroy_real (NAUTILUS_TREE_VIEW (tree_view));
	}
			
	/* get the data from the other side of the dnd */
	nautilus_tree_view_ensure_drag_data (tree_view, context, time);	


	/* prelight depending on the type of drop. */
	if (drag_info->got_drop_data_type) {
		switch (drag_info->data_type) {
		case NAUTILUS_ICON_DND_GNOME_ICON_LIST:
		case NAUTILUS_ICON_DND_URI_LIST:

			nautilus_ctree_set_prelight (NAUTILUS_CTREE (tree_view->details->tree), 
						     y);
			
			nautilus_tree_view_make_prelight_if_file_operation (NAUTILUS_TREE_VIEW (tree_view), 
									    x, y);

			break;
		case NAUTILUS_ICON_DND_KEYWORD:	
		case NAUTILUS_ICON_DND_COLOR:
		case NAUTILUS_ICON_DND_BGIMAGE:	
		default:
			break;
		}
	}

	/* auto scroll */
	nautilus_tree_view_start_auto_scroll (tree_view);

	/* update dragging cursor. */
	nautilus_tree_view_get_drop_action  (tree_view, context, x, y, 
					     &default_action, 
					     &non_default_action);
	resulting_action = nautilus_drag_modifier_based_action (default_action,
								non_default_action);
	gdk_drag_status (context, resulting_action, time);



	/* make sure no one will ever get this event except us */
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_motion");
	return TRUE;
}


static gboolean 
nautilus_tree_view_drag_drop (GtkWidget *widget,
			      GdkDragContext *context,
			      int x, int y, guint time,
			      gpointer user_data)
{
	NautilusTreeViewDndDetails *dnd;
	NautilusTreeView *tree_view;

	tree_view = NAUTILUS_TREE_VIEW (user_data);
	dnd = (NautilusTreeViewDndDetails *) (tree_view->details->dnd);

	/* critical piece of code. this ensures that our 
	 * drag_data_received callback is going to be called 
	 * soon and it will execute the right actions since the 
	 * drop occured.
	 */
	dnd->drag_info->drop_occured = TRUE;
	gtk_drag_get_data (GTK_WIDGET (widget), context,
			   GPOINTER_TO_INT (context->targets->data),
			   time);



	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_drop");
	return TRUE;
}


static void 
nautilus_tree_view_drag_data_received (GtkWidget *widget,
				       GdkDragContext *context,
				       gint x, gint y,
				       GtkSelectionData *data,
				       guint info, guint time)
{
	NautilusTreeViewDndDetails *dnd;
	NautilusDragInfo *drag_info;
	NautilusTreeView *tree_view;

	tree_view = NAUTILUS_TREE_VIEW (gtk_object_get_data (GTK_OBJECT (widget), "tree_view"));
	dnd = tree_view->details->dnd;
	drag_info = dnd->drag_info;


	if (!drag_info->got_drop_data_type) {
		drag_info->data_type = info;
		drag_info->got_drop_data_type = TRUE;

		/* save operation for drag motion events */
		switch (info) {
		case NAUTILUS_ICON_DND_GNOME_ICON_LIST:
			drag_info->selection_list = nautilus_drag_build_selection_list (data);
			break;
		case NAUTILUS_ICON_DND_URI_LIST:
			drag_info->selection_list = nautilus_drag_build_selection_list (data);
			break;
		case NAUTILUS_ICON_DND_COLOR:
		case NAUTILUS_ICON_DND_BGIMAGE:	
		case NAUTILUS_ICON_DND_KEYWORD:	
		default:
			/* we do not want to support any of the 3 above */
			break;
		}
	} 

	if (drag_info->drop_occured) {
		/* drop occured: do actual operations on the data */
		switch (info) {
		case NAUTILUS_ICON_DND_GNOME_ICON_LIST:
		case NAUTILUS_ICON_DND_URI_LIST:
			nautilus_tree_view_receive_dropped_icons
				(NAUTILUS_TREE_VIEW (tree_view),
				 context, x, y);
			gtk_drag_finish (context, TRUE, FALSE, time);
			break;
		case NAUTILUS_ICON_DND_COLOR:
		case NAUTILUS_ICON_DND_BGIMAGE:
		case NAUTILUS_ICON_DND_KEYWORD:
		default:
			gtk_drag_finish (context, FALSE, FALSE, time);
		}

		nautilus_tree_view_drag_destroy (NAUTILUS_TREE_VIEW (tree_view));
		
	}
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_data_received");
}


static void nautilus_tree_view_drag_data_get (GtkWidget *widget,
					      GdkDragContext *context,
					      GtkSelectionData *data,
					      guint info, guint time,
					      gpointer user_data)
{
	NautilusTreeView *tree_view;
	char *uri, *selection_string;

	g_assert (widget != NULL);
	g_return_if_fail (context != NULL);

	tree_view = NAUTILUS_TREE_VIEW (gtk_object_get_data (GTK_OBJECT (widget), "tree_view"));

	uri = nautilus_tree_view_get_drag_uri (tree_view);
	selection_string = g_strconcat (uri, "\r\n", NULL);


	gtk_selection_data_set (data,
				data->target,
				8, selection_string, strlen(selection_string));
	g_free (uri);

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				      "drag_data_get");

}









/* --------------------------------------------------------------
   standard gtk events: press/release/motion. used to start drags 
   --------------------------------------------------------------
*/









static int 
nautilus_tree_view_button_press (GtkWidget *widget, GdkEventButton *event)
{
	int retval;
	GtkCList *clist;
	NautilusTreeView *tree_view;
		
	int press_row, press_column, on_row;

	clist = GTK_CLIST (widget);
	retval = FALSE;

	if (event->window != clist->clist_window)
		return retval;

	tree_view = NAUTILUS_TREE_VIEW (gtk_object_get_data (GTK_OBJECT (widget), "tree_view"));

	on_row = gtk_clist_get_selection_info (GTK_CLIST (tree_view->details->tree), 
					       event->x, 
					       event->y, 
					       &press_row, &press_column);
	if (on_row == 0) {
		gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button-press-event");
		return FALSE;
	}

	if (nautilus_ctree_is_hot_spot (NAUTILUS_CTREE (widget), event->x, event->y)) {		
		NautilusCTreeRow *ctree_row;
		NautilusCTreeNode *node;
		
		tree_view->details->dnd->press_x = event->x;
		tree_view->details->dnd->press_y = event->y;
		tree_view->details->dnd->pressed_button = event->button;
		tree_view->details->dnd->pressed_hot_spot = TRUE;

		/* Clicking in the expander should not start a drag */
		tree_view->details->dnd->drag_pending = FALSE;
	
		ctree_row = ROW_ELEMENT (clist, press_row)->data;
		if (ctree_row != NULL) {
			ctree_row->mouse_down = TRUE;
			ctree_row->in_hotspot = TRUE;

			node = nautilus_ctree_find_node_ptr (NAUTILUS_CTREE (widget), ctree_row);
			if (node != NULL) {
				nautilus_ctree_draw_node (NAUTILUS_CTREE (widget), node);
			}
		}
	} else {
		switch (event->type) {
		case GDK_BUTTON_PRESS:
			tree_view->details->dnd->drag_pending = TRUE;
			tree_view->details->dnd->press_x = event->x;
			tree_view->details->dnd->press_y = event->y;
			tree_view->details->dnd->pressed_button = event->button;
			break;
		case GDK_2BUTTON_PRESS:
		default:
			break;
		}
	}

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button-press-event");
	
	return TRUE;
}

#define RADIUS 200

static int
nautilus_tree_view_button_release (GtkWidget *widget, GdkEventButton *event)
{
	int retval;
	GtkCList *clist;
	NautilusTreeView *tree_view;
	int release_row, release_column, on_row;
	int distance_squared;
	gboolean is_still_hot_spot;
	int press_row, press_column;
	NautilusCTreeRow *ctree_row;
	NautilusCTreeNode *node;

	clist = GTK_CLIST (widget);
	retval = FALSE;

	if (event->window != clist->clist_window)
		return retval;

	tree_view = NAUTILUS_TREE_VIEW (gtk_object_get_data (GTK_OBJECT (widget), "tree_view"));
	tree_view->details->dnd->drag_pending = FALSE;
	
	/* Set state of spinner.  Use saved dnd x and y as the mouse may have moved out
	 * of the original row */	
	on_row = gtk_clist_get_selection_info (GTK_CLIST (tree_view->details->tree), 
					       tree_view->details->dnd->press_x, 
					       tree_view->details->dnd->press_y, 
					       &press_row, &press_column);	
	ctree_row = ROW_ELEMENT (clist, press_row)->data;
	if (ctree_row != NULL) {
		ctree_row->mouse_down = FALSE;
		ctree_row->in_hotspot = FALSE;

		/* Redraw spinner */
		node = nautilus_ctree_find_node_ptr (NAUTILUS_CTREE (widget), ctree_row);
		if (node != NULL) {
			nautilus_ctree_draw_node (NAUTILUS_CTREE (widget), node);
		}
	}

	distance_squared = (event->x - tree_view->details->dnd->press_x)
		* (event->x - tree_view->details->dnd->press_x) +
		(event->y - tree_view->details->dnd->press_y)
		* (event->y - tree_view->details->dnd->press_y);
	is_still_hot_spot = nautilus_ctree_is_hot_spot (NAUTILUS_CTREE(tree_view->details->tree), 
						   event->x, event->y);
	
	on_row = gtk_clist_get_selection_info (GTK_CLIST (tree_view->details->tree), 
					       event->x, 
					       event->y, 
					       &release_row, &release_column);

	if (on_row == 1) {

		if (distance_squared <= RADIUS 
		    && tree_view->details->dnd->pressed_hot_spot
		    && is_still_hot_spot) {
			
			tree_view->details->dnd->pressed_hot_spot = FALSE;
			
			nautilus_tree_view_expand_or_collapse_row (NAUTILUS_CTREE(tree_view->details->tree), 
								   release_row);
		} else if (distance_squared <= RADIUS) {
			/* we are close from the place we clicked */
			/* select current row */

			/* Only button 1 triggers a selection */
			if (event->button == 1) {
				gtk_clist_select_row (GTK_CLIST (tree_view->details->tree), 
						      release_row, release_column); 
			}
		}
	}

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button-release-event");

	return TRUE;

}



static int
nautilus_tree_view_motion_notify (GtkWidget *widget, GdkEventButton *event)
{
	GtkCList *clist;
	NautilusTreeView *tree_view;

	clist = GTK_CLIST (widget);


	if (event->window != clist->clist_window)
		return FALSE;

	tree_view = NAUTILUS_TREE_VIEW (gtk_object_get_data (GTK_OBJECT (widget), "tree_view"));

	if (tree_view->details->dnd->drag_pending) {
		int distance_squared;

		distance_squared = (event->x - tree_view->details->dnd->press_x)
			* (event->x - tree_view->details->dnd->press_x) +
			(event->y - tree_view->details->dnd->press_y)
			* (event->y - tree_view->details->dnd->press_y);
		if (distance_squared > RADIUS) {
			/* drag started !! */
			GdkDragAction action;
			
			if (tree_view->details->dnd->pressed_button == 3) {
				action = GDK_ACTION_ASK;
			} else {
				action = GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_ASK;
			}
			gtk_drag_begin (tree_view->details->tree, 
					tree_view->details->dnd->drag_info->target_list, 
					action,
					tree_view->details->dnd->pressed_button,
					(GdkEvent *) event);


			nautilus_ctree_set_prelight (NAUTILUS_CTREE (tree_view->details->tree), 
						     tree_view->details->dnd->press_y);

		}
	} 

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "motion-notify-event");

	return TRUE;
}



















/* -----------------------------------------------------------------------
   helper functions
   -----------------------------------------------------------------------
*/


static void 
nautilus_tree_view_set_dnd_icon (NautilusTreeView *tree_view, GdkDragContext *context)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	NautilusCTreeNode *node;
	gchar       *text;
	guint8       spacing;
	GdkPixmap   *pixmap_opened;
	GdkBitmap   *mask_opened;
	gboolean     is_leaf;
	gboolean     expanded;
	NautilusTreeViewDndDetails *dnd;
	
	g_assert (tree_view != NULL);
	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));

	dnd = tree_view->details->dnd;

	node = nautilus_tree_view_tree_node_at (NAUTILUS_TREE_VIEW (tree_view),
						dnd->press_x,
						dnd->press_y);
		
	nautilus_ctree_get_node_info (NAUTILUS_CTREE (tree_view->details->tree),
				      node, &text,
				      &spacing, &pixmap,
				      &mask, &pixmap_opened,
				      &mask_opened, &is_leaf, 
				      &expanded);

	gtk_drag_set_icon_pixmap (context, 
				  gdk_rgb_get_cmap (),
				  pixmap, mask,
				  10, 10);
}



static void 
nautilus_tree_view_make_prelight_if_file_operation (NautilusTreeView *tree_view, 
						    int x, int y)
{
#if 0
	NautilusTreeViewDndDetails *dnd;
	NautilusCTreeNode *node;
	gboolean is_directory;

	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));
	dnd = tree_view->details->dnd;

	/* make the thing under us prelighted. */
	node = nautilus_tree_view_tree_node_at (NAUTILUS_TREE_VIEW (tree_view), x, y);
	if (node == NULL) {
		return;
	}
	is_directory = nautilus_tree_view_is_tree_node_directory (NAUTILUS_TREE_VIEW (tree_view), node);

	if (!is_directory) {
		NautilusTreeNode *parent_node, *current_node;
		current_node = nautilus_tree_view_node_to_model_node (tree_view, node);
		parent_node = nautilus_tree_node_get_parent (current_node);
		node = nautilus_tree_view_model_node_to_view_node (tree_view, parent_node);
	} 

	if (node != dnd->current_prelighted_node 
	    && dnd->current_prelighted_node != NULL) {
		nautilus_ctree_node_set_row_style (NAUTILUS_CTREE (tree_view->details->tree), 
					      dnd->current_prelighted_node,
					      tree_view->details->dnd->normal_style);
	}
	nautilus_ctree_node_set_row_style (NAUTILUS_CTREE (tree_view->details->tree), 
				      node,
				      tree_view->details->dnd->highlight_style);
	dnd->current_prelighted_node = node;
#endif
}



/* returns if it was expanded or not */
static gboolean
nautilus_tree_view_collapse_node (NautilusCTree *tree, NautilusCTreeNode *node)
{
	char *node_text;
	guint8 node_spacing;
	GdkPixmap *pixmap_closed;
	GdkBitmap *mask_closed;
	GdkPixmap *pixmap_opened;
	GdkBitmap *mask_opened;
	gboolean is_leaf;
	gboolean is_expanded;

	nautilus_ctree_get_node_info (NAUTILUS_CTREE(tree), 
				      node, &node_text,
				      &node_spacing, &pixmap_closed, &mask_closed,
				      &pixmap_opened, &mask_opened, 
				      &is_leaf, &is_expanded);
	if (is_expanded) {
				/* collapse */
		nautilus_ctree_collapse (NAUTILUS_CTREE(tree),
					 node);
	}

	return is_expanded;
}


static void
nautilus_tree_view_expand_or_collapse_row (NautilusCTree *tree, int row)
{
	NautilusCTreeNode *node;
	char *node_text;
	guint8 node_spacing;
	GdkPixmap *pixmap_closed;
	GdkBitmap *mask_closed;
	GdkPixmap *pixmap_opened;
	GdkBitmap *mask_opened;
	gboolean is_leaf;
	gboolean is_expanded;

	node = nautilus_ctree_node_nth (NAUTILUS_CTREE(tree), row);
	nautilus_ctree_get_node_info (NAUTILUS_CTREE(tree), 
				 node, &node_text,
				 &node_spacing, &pixmap_closed, &mask_closed,
				 &pixmap_opened, &mask_opened, 
				 &is_leaf, &is_expanded);
	if (!is_expanded) {
				/* expand */
		nautilus_ctree_expand (NAUTILUS_CTREE(tree),
				  node);
	} else {
				/* collapse */ 
		nautilus_ctree_collapse (NAUTILUS_CTREE(tree),
				    node);
	}

}

/* it actually also supports links */
static void
nautilus_tree_view_move_copy_files (NautilusTreeView *tree_view,
				    GList *selection_list,
				    GdkDragContext *context,
				    const char *target_uri)
{
	GList *source_uris, *p;
	
	source_uris = NULL;
	for (p = selection_list; p != NULL; p = p->next) {
		/* do a shallow copy of all the uri strings of the copied files */
		source_uris = g_list_prepend (source_uris, ((DragSelectionItem *)p->data)->uri);
	}
	source_uris = g_list_reverse (source_uris);
	
	/* start the copy */
	nautilus_file_operations_copy_move (source_uris,
					    NULL, 
					    target_uri,
					    context->action,
					    GTK_WIDGET (tree_view->details->tree),
					    NULL, NULL);

	g_list_free (source_uris);
}



static char *
nautilus_tree_view_find_drop_target (NautilusTreeView *tree_view,
				     int x, int y)
{
	char *target_uri;
	NautilusFile *file;
	NautilusCTreeNode *node;
	gboolean is_directory;
	NautilusTreeNode *current_node;
	
	node = nautilus_tree_view_tree_node_at (NAUTILUS_TREE_VIEW (tree_view), x, y);
	if (node == NULL) {
		return NULL;
	}
	is_directory = nautilus_tree_view_is_tree_node_directory (NAUTILUS_TREE_VIEW (tree_view), node);

	current_node = nautilus_tree_view_node_to_model_node (tree_view, node);

	if (!is_directory) {
		NautilusTreeNode *parent_node;
		parent_node = nautilus_tree_node_get_parent (current_node);
		file = nautilus_tree_node_get_file (parent_node);
	} else {
		file = nautilus_tree_node_get_file (current_node);
	}

	target_uri = nautilus_file_get_uri (file);

	return target_uri;
}


static gboolean
nautilus_tree_view_is_tree_node_directory (NautilusTreeView *tree_view,
					   NautilusCTreeNode *node) 
{
	NautilusTreeNode *model_node;
	NautilusFile *file;
	gboolean is_directory;

	model_node = nautilus_tree_view_node_to_model_node (tree_view, node);

	file = nautilus_tree_node_get_file (model_node);

	is_directory = nautilus_file_is_directory (file);
	
	return is_directory;
}



static NautilusCTreeNode *
nautilus_tree_view_tree_node_at (NautilusTreeView *tree_view,
				 int x, int y) 
{
	int row, column, on_row;
	NautilusCTreeNode *node;


	on_row = gtk_clist_get_selection_info (GTK_CLIST (tree_view->details->tree), 
					       x, y, &row, &column);

	node = NULL;
	if (on_row == 1) {
		node = nautilus_ctree_node_nth (NAUTILUS_CTREE (tree_view->details->tree),
					   row);
	}

	return node;
}

/**
 * nautilus_tree_view_item_at:
 * @tree_view: 
 * @x: coordinates in pixel units.
 * @y:
 *
 * Return value: uri of the node under the x/y pixel 
 *               unit coordinates in the tree. 
 *               will return NULL if not in tree.
 */
static char *
nautilus_tree_view_item_at (NautilusTreeView *tree_view,
			    int x, int y)
{
	NautilusCTreeNode *node;

	node = nautilus_tree_view_tree_node_at (tree_view, x, y);
	if (node == NULL) {
		return NULL;
	}

	return nautilus_file_get_uri (nautilus_tree_view_node_to_file (tree_view, node));
}


/**
 * nautilus_tree_view_get_drag_uri:
 * @tree_view: a %NautilusTreeView.
 *
 * Return value: the uri of the object selected
 *               for drag in the tree.
 */
static char *
nautilus_tree_view_get_drag_uri  (NautilusTreeView *tree_view)
{
	return nautilus_tree_view_item_at (tree_view,
					   tree_view->details->dnd->press_x,
					   tree_view->details->dnd->press_y);
}

static void
nautilus_tree_view_ensure_drag_data (NautilusTreeView *tree_view,
				     GdkDragContext *context,
				     guint32 time)
{
	NautilusDragInfo *drag_info;

	drag_info = tree_view->details->dnd->drag_info;

	if (!drag_info->got_drop_data_type) {
		gtk_drag_get_data (GTK_WIDGET (tree_view->details->tree), context,
				   GPOINTER_TO_INT (context->targets->data),
				   time);
	}
}






/***********************************************************************
 * scrolling helper code. stolen and modified from nautilus-icon-dnd.c *
 ***********************************************************************/


static gboolean
ready_to_start_scrolling (NautilusDragInfo *drag_info,
			  int y_scroll_delta)
{
	return (y_scroll_delta != 0 && drag_info->start_auto_scroll_in < nautilus_get_system_time ()) ||
		drag_info->start_auto_scroll_in +  AUTOSCROLL_X_ONLY_EXTRA_DELAY < nautilus_get_system_time ();
}

static int
auto_scroll_timeout_callback (gpointer data)
{
	NautilusDragInfo *drag_info;
	NautilusTreeView *tree_view;
	GtkWidget *widget;
	float x_scroll_delta, y_scroll_delta;
	
	g_assert (NAUTILUS_IS_TREE_VIEW (data));
	widget = GTK_WIDGET (data);
	tree_view = NAUTILUS_TREE_VIEW (widget);
	drag_info = tree_view->details->dnd->drag_info;

	nautilus_drag_autoscroll_calculate_delta (tree_view->details->tree, &x_scroll_delta, &y_scroll_delta);

	if (drag_info->waiting_to_autoscroll
	    && !ready_to_start_scrolling (drag_info, y_scroll_delta)) {
		/* not yet */
		return TRUE;
	}

	drag_info->waiting_to_autoscroll = FALSE;

	/* make the GtkScrolledWindow actually scroll */
	nautilus_tree_view_real_scroll (tree_view, x_scroll_delta, y_scroll_delta);

	return TRUE;
}
	

static void
nautilus_tree_view_start_auto_scroll (NautilusTreeView *tree_view)
{
	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));

	nautilus_drag_autoscroll_start (tree_view->details->dnd->drag_info,
					tree_view->details->tree,
					auto_scroll_timeout_callback,
					tree_view);
}


static void
nautilus_tree_view_stop_auto_scroll (NautilusTreeView *tree_view)
{
	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));

        nautilus_drag_autoscroll_stop (tree_view->details->dnd->drag_info);
}



static void
nautilus_tree_view_real_scroll (NautilusTreeView *tree_view, float delta_x, float delta_y)
{
	GtkAdjustment *hadj, *vadj;

	hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (tree_view));
	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tree_view));

	nautilus_gtk_adjustment_set_value (hadj, hadj->value + delta_x);
	nautilus_gtk_adjustment_set_value (vadj, vadj->value + delta_y);

}




/******************************************
 * Handle the data dropped on the tree view 
 ******************************************/

static void
nautilus_tree_view_get_drop_action (NautilusTreeView *tree_view, 
				    GdkDragContext *context,
				    int x, int y,
				    int *default_action,
				    int *non_default_action)
{
	NautilusDragInfo *drag_info;
	char *drop_target;

	drag_info = NAUTILUS_TREE_VIEW (tree_view)->details->dnd->drag_info;

	/* FIXME bugzilla.eazel.com 2569: Too much code copied from nautilus-icon-dnd.c. Need to share more. */

	if (!drag_info->got_drop_data_type) {
		/* drag_data_received didn't get called yet */
		return;
	}


	switch (drag_info->data_type) {
	case NAUTILUS_ICON_DND_GNOME_ICON_LIST:
	case NAUTILUS_ICON_DND_URI_LIST:
		if (drag_info->selection_list == NULL) {
			*default_action = 0;
			*non_default_action = 0;
			return;
		}

		drop_target = nautilus_tree_view_find_drop_target (tree_view, x, y);
		if (!drop_target) {
			*default_action = 0;
			*non_default_action = 0;
			return;
		}
		nautilus_drag_default_drop_action_for_icons (context, drop_target, 
							     drag_info->selection_list, 
							     default_action, 
							     non_default_action);
		break;
	case NAUTILUS_ICON_DND_COLOR:
	case NAUTILUS_ICON_DND_KEYWORD:	
	case NAUTILUS_ICON_DND_BGIMAGE:	
		/* we handle none of the above */
		*default_action = context->suggested_action;
		*non_default_action = context->suggested_action;
		break;

	default:
	}

}			       

static void 
nautilus_tree_view_collapse_all (NautilusTreeView *tree_view,
				 NautilusCTreeNode *current_node)
{
	GSList *list, *temp_list;

	list = tree_view->details->dnd->expanded_nodes;
	

	for (temp_list = list; temp_list != NULL; temp_list = temp_list->next) {
		NautilusCTreeNode *expanded_node;
		expanded_node = (NautilusCTreeNode *) temp_list->data;
		if (!nautilus_ctree_is_ancestor (NAUTILUS_CTREE (tree_view->details->tree), 
						 expanded_node, current_node)) {
#ifdef MATHIEU_DEBUG
			{
				char *expanded_uri, *current_uri;
				expanded_uri = nautilus_file_get_uri 
					(nautilus_tree_view_node_to_file (tree_view, expanded_node));
				current_uri = nautilus_file_get_uri 
					(nautilus_tree_view_node_to_file (tree_view, current_node));

				g_print ("collapsing %s in %s\n", expanded_uri, current_uri);
				g_free (expanded_uri);
				g_free (current_uri);
			}
#endif
			nautilus_tree_view_collapse_node (NAUTILUS_CTREE (tree_view->details->tree), 
							  expanded_node);
		}
	}
}


static void
nautilus_tree_view_receive_dropped_icons (NautilusTreeView *view,
					  GdkDragContext *context,
					  int x, int y)
{
	NautilusDragInfo *drag_info;
	NautilusTreeView *tree_view;
	NautilusTreeViewDndDetails *dnd;
	char *drop_target_uri;
	NautilusCTreeNode *dropped_node;

	g_assert (NAUTILUS_IS_TREE_VIEW (view));

	tree_view = NAUTILUS_TREE_VIEW (view);
	dnd = tree_view->details->dnd;
	drag_info = dnd->drag_info;
	

	drop_target_uri = NULL;

	if (drag_info->selection_list == NULL) {
		return;
	}

	if (context->action == GDK_ACTION_ASK) {
		context->action = nautilus_drag_drop_action_ask 
			(GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
	}

	if (context->action > 0) {
		drop_target_uri = nautilus_tree_view_find_drop_target (tree_view, 
								       x, y);
		if (drop_target_uri == NULL) {
			nautilus_drag_destroy_selection_list (drag_info->selection_list);
			return;
		}

#ifdef MATHIEU_DEBUG
		{
			char *action_string;
			switch (context->action) {
			case GDK_ACTION_MOVE:
				action_string = "move";
				break;
			case GDK_ACTION_COPY:
				action_string = "copy";
				break;
			case GDK_ACTION_LINK:
				action_string = "link";
				break;
			default:
				g_assert_not_reached ();
				action_string = "error";
				break;
			}
			g_print ("%s selection in %s\n", 
				 action_string, drop_target_uri);
		}
#endif
		nautilus_tree_view_move_copy_files (tree_view, drag_info->selection_list, 
							    context, drop_target_uri);
		/* collapse all expanded directories during drag except the one we 
		   droped into */
		dropped_node = nautilus_tree_view_tree_node_at (tree_view, x, y);
		if (dropped_node != NULL) {
			nautilus_tree_view_collapse_all (tree_view, dropped_node);
		}

		g_slist_free (dnd->expanded_nodes);
		dnd->expanded_nodes = NULL;

		g_free (drop_target_uri);
	}
}


static void
nautilus_tree_view_prelight_stop (NautilusTreeView *tree_view)
{
#if 0
	NautilusTreeViewDndDetails *dnd;

	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));

	dnd = tree_view->details->dnd;

	if (dnd->current_prelighted_node != NULL) {
		nautilus_ctree_node_set_row_style (NAUTILUS_CTREE (tree_view->details->tree), 
						   dnd->current_prelighted_node,
						   tree_view->details->dnd->normal_style);
	}
	dnd->current_prelighted_node = NULL;
#endif
}



static void
nautilus_tree_view_drag_destroy (NautilusTreeView *tree_view)
{
	NautilusTreeViewDndDetails *dnd;
	NautilusDragInfo *drag_info;

	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));

	dnd = tree_view->details->dnd;
	drag_info = dnd->drag_info;

	drag_info->need_to_destroy = TRUE;

	/* stop autoscroll */
	nautilus_tree_view_stop_auto_scroll (tree_view);

	/* remove prelighting */
	nautilus_ctree_set_prelight (NAUTILUS_CTREE (tree_view->details->tree), 
				     -1);
	nautilus_tree_view_prelight_stop (tree_view);
}


static void
nautilus_tree_view_drag_destroy_real (NautilusTreeView *tree_view)
{
	NautilusTreeViewDndDetails *dnd;
	NautilusDragInfo *drag_info;

	g_assert (NAUTILUS_IS_TREE_VIEW (tree_view));

	dnd = tree_view->details->dnd;
	drag_info = dnd->drag_info;

	/* reset booleans used during drag. */
	drag_info->got_drop_data_type = FALSE;
	nautilus_drag_destroy_selection_list (drag_info->selection_list);
	drag_info->drop_occured = FALSE;

	

	/* misc stuff */
	/*	
	if (dnd_info->shadow != NULL) {
		gtk_object_destroy (GTK_OBJECT (dnd_info->shadow));
		dnd_info->shadow = NULL;
	}
	*/

	if (drag_info->selection_data != NULL) {
		nautilus_gtk_selection_data_free_deep (drag_info->selection_data);
		drag_info->selection_data = NULL;
	}

}
