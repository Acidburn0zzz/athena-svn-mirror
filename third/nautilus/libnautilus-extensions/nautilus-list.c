/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-list.h: Enhanced version of GtkCList for Nautilus.

   Copyright (C) 1999, 2000 Free Software Foundation
   Copyright (C) 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Federico Mena <federico@nuclecu.unam.mx>,
            Ettore Perazzoli <ettore@gnu.org>,
            John Sullivan <sullivan@eazel.com>,
	    Pavel Cisler <pavel@eazel.com>
 */

#include <config.h>
#include "nautilus-list.h"

#include <ctype.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtkbindings.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkenums.h>
#include <gtk/gtkmain.h>
#include <glib.h>
#include <libnautilus-extensions/nautilus-gdk-pixbuf-extensions.h>
#include <libnautilus-extensions/nautilus-background.h>
#include <libnautilus-extensions/nautilus-graphic-effects.h>
#include <libnautilus-extensions/nautilus-drag.h>
#include <libnautilus-extensions/nautilus-gdk-extensions.h>
#include <libnautilus-extensions/nautilus-gdk-pixbuf-extensions.h>
#include <libnautilus-extensions/nautilus-glib-extensions.h>
#include <libnautilus-extensions/nautilus-gtk-extensions.h>
#include <libnautilus-extensions/nautilus-gtk-macros.h>
#include <libnautilus-extensions/nautilus-list-column-title.h>

/* Timeout for making the row currently selected for keyboard operation visible.
 * Unlike in nautilus-icon-container, there appear to be no adverse effects from
 * making this 0.
 */
#define KEYBOARD_ROW_REVEAL_TIMEOUT 0

/* FIXME bugzilla.eazel.com 2573: This constant and much of the code surrounding its use was copied from
 * nautilus-icon-container; they should share code instead.
 */
#define CONTEXT_MENU_TIMEOUT_INTERVAL 500

#define NO_BUTTON		0
#define ACTION_BUTTON		1
#define CONTEXTUAL_MENU_BUTTON	3

struct NautilusListDetails
{
	/* Single click mode ? */
	gboolean single_click_mode;

	/* The anchor row for range selections */
	int anchor_row;

	/* Mouse information saved on button press */
	int dnd_press_button;
	int dnd_press_x, dnd_press_y;
	int button_down_row;
	guint32 button_down_time;

	/* Timeout used to make a selected row fully visible after a short
	 * period of time. (The timeout is needed to make sure
	 * double-clicking still works, and to optimize holding down arrow key.)
	 */
	guint keyboard_row_reveal_timer_id;
	int keyboard_row_to_reveal;

	/* Typeahead state */
	char *type_select_pattern;
	gint64 last_typeselect_time;

	/* Signal IDs that we sometimes want to block. */
	guint select_row_signal_id;
	guint unselect_row_signal_id;

	/* Drag state */
	NautilusDragInfo *drag_info;
	gboolean drag_started;
	gboolean rejects_dropped_icons;
	
	guint context_menu_timeout_id;
	
	/* Delayed selection information */
	gboolean dnd_select_pending;
	guint dnd_select_pending_state;

	/* Targets for drag data */
	GtkTargetList *target_list; 
	
	NautilusCListRow *drag_prelight_row;
	
	GtkWidget *title;

	/* Rendering state */
	GdkGC *cell_lighter_background;
	GdkGC *cell_darker_background;
	GdkGC *cell_selected_lighter_background;
	GdkGC *cell_selected_darker_background;
	GdkGC *cell_divider_color;
	GdkGC *selection_light_color;
	GdkGC *selection_medium_color;
	GdkGC *selection_main_color;
	GdkGC *text_color;
	GdkGC *selected_text_color;
	GdkGC *link_text_color;

	/* Need RGB background values when compositing images */
	guint32 cell_lighter_background_rgb;
	guint32 cell_darker_background_rgb;
	guint32 cell_selected_lighter_background_rgb;
	guint32 cell_selected_darker_background_rgb;
	guint32 selection_light_color_rgb;
	guint32 selection_medium_color_rgb;
	guint32 selection_main_color_rgb;
};

/* maximum amount of milliseconds the mouse button is allowed to stay down and still be considered a click */
#define MAX_CLICK_TIME 1500

/* horizontal space between images in a pixbuf list cell */
#define PIXBUF_LIST_SPACING	2

/* Some #defines stolen from gtkclist.c that we need for other stolen code. */

/* minimum allowed width of a column */
#define COLUMN_MIN_WIDTH 5

/* this defines the base grid spacing */
#define CELL_SPACING 1

/* added the horizontal space at the beginning and end of a row */
#define COLUMN_INSET 3

/* the width of the column resize windows */
#define DRAG_WIDTH  6

/* gives the left pixel of the given column in context of
 * the clist's hoffset */
#define COLUMN_LEFT_XPIXEL(clist, colnum)  ((clist)->column[(colnum)].area.x + \
					    (clist)->hoffset)

/* gives the top pixel of the given row in context of
 * the clist's voffset */
#define ROW_TOP_YPIXEL(clist, row) (((clist)->row_height * (row)) + \
				    (((row) + 1) * CELL_SPACING) + \
				    (clist)->voffset)

/* returns the row index from a y pixel location in the 
 * context of the clist's voffset */
#define ROW_FROM_YPIXEL(clist, y)  (((y) - (clist)->voffset) / \
				    ((clist)->row_height + CELL_SPACING))

/* returns the GList item for the nth row */
#define	ROW_ELEMENT(clist, row)	(((row) == (clist)->rows - 1) ? \
				 (clist)->row_list_end : \
				 g_list_nth ((clist)->row_list, (row)))

/* returns the total height of the list */
#define LIST_HEIGHT(clist)         (((clist)->row_height * ((clist)->rows)) + \
				    (CELL_SPACING * ((clist)->rows + 1)))

enum {
	CONTEXT_CLICK_SELECTION,
	CONTEXT_CLICK_BACKGROUND,
	ACTIVATE,
	SELECTION_CHANGED,
	SELECT_MATCHING_NAME,
	SELECT_PREVIOUS_NAME,
	SELECT_NEXT_NAME,
	HANDLE_DROPPED_ITEMS,
	HANDLE_DRAGGED_ITEMS,
	GET_DEFAULT_ACTION,
	GET_DRAG_PIXMAP,
	GET_SORT_COLUMN_INDEX,
	LAST_SIGNAL
};

static GtkTargetEntry nautilus_list_dnd_target_table[] = {
	{ NAUTILUS_ICON_DND_GNOME_ICON_LIST_TYPE, 0, NAUTILUS_ICON_DND_GNOME_ICON_LIST },
	{ NAUTILUS_ICON_DND_URI_LIST_TYPE, 0, NAUTILUS_ICON_DND_URI_LIST },
	{ NAUTILUS_ICON_DND_URL_TYPE, 0, NAUTILUS_ICON_DND_URL },
	{ NAUTILUS_ICON_DND_COLOR_TYPE, 0, NAUTILUS_ICON_DND_COLOR },
	{ NAUTILUS_ICON_DND_BGIMAGE_TYPE, 0, NAUTILUS_ICON_DND_BGIMAGE },
	{ NAUTILUS_ICON_DND_KEYWORD_TYPE, 0, NAUTILUS_ICON_DND_KEYWORD }
};

static void     activate_row                            (NautilusList         *list,
							 int                   row);
static int      get_cell_horizontal_start_position      (NautilusCList        *clist,
							 NautilusCListRow     *row,
							 int                   column_index,
							 int                   content_width);
static void     get_cell_style                          (NautilusList         *clist,
							 NautilusCListRow     *row,
							 int                   state,
							 int		       row_index,
							 int                   column_index,
							 GtkStyle            **style,
							 GdkGC               **fg_gc,
							 GdkGC               **bg_gc,
							 guint32	      *bg_rgb);
static void     nautilus_list_initialize_class          (NautilusListClass    *class);
static void     nautilus_list_initialize                (NautilusList         *list);
static void     nautilus_list_destroy                   (GtkObject            *object);
static int      nautilus_list_button_press              (GtkWidget            *widget,
							 GdkEventButton       *event);
static int      nautilus_list_button_release            (GtkWidget            *widget,
							 GdkEventButton       *event);
static int      nautilus_list_motion                    (GtkWidget            *widget,
							 GdkEventMotion       *event);
static void     nautilus_list_drag_end                  (GtkWidget            *widget,
							 GdkDragContext       *context);
static void     nautilus_list_drag_leave                (GtkWidget            *widget,
							 GdkDragContext       *context,
							 guint                 time);
static gboolean nautilus_list_drag_motion               (GtkWidget            *widget,
							 GdkDragContext       *context,
							 int                   x,
							 int                   y,
							 guint                 time);
static gboolean nautilus_list_drag_drop                 (GtkWidget            *widget,
							 GdkDragContext       *context,
							 int                   x,
							 int                   y,
							 guint                 time);
static void     nautilus_list_drag_data_received        (GtkWidget            *widget,
							 GdkDragContext       *context,
							 int                   x,
							 int                   y,
							 GtkSelectionData     *data,
							 guint                 info,
							 guint                 time);
static void     nautilus_list_clear_keyboard_focus      (NautilusList         *list);
static void     nautilus_list_draw_focus                (GtkWidget            *widget);
static int      nautilus_list_key_press                 (GtkWidget            *widget,
							 GdkEventKey          *event);
static void     nautilus_list_unselect_all              (NautilusCList        *clist);
static void     nautilus_list_select_all                (NautilusCList        *clist);
static void     schedule_keyboard_row_reveal            (NautilusList         *list,
							 int                   row);
static void     unschedule_keyboard_row_reveal          (NautilusList         *list);
static void     emit_selection_changed                  (NautilusList         *clist);
static void     nautilus_list_clear                     (NautilusCList        *clist);
static void	nautilus_list_draw 			(GtkWidget 	      *widget, 
							 GdkRectangle 	      *area);
static int	nautilus_list_expose			(GtkWidget            *widget,
							 GdkEventExpose       *event);
static void     draw_rows                               (NautilusCList        *clist, 
							 GdkRectangle         *area);
static void     draw_row                                (NautilusCList        *list,
							 GdkRectangle         *area,
							 int                   row_index,
							 NautilusCListRow     *row);
static void     draw_all                                (NautilusCList        *clist);
static void     nautilus_list_style_set                 (GtkWidget            *widget,
							 GtkStyle             *previous_style);
static void     nautilus_list_realize                   (GtkWidget            *widget);
static void     nautilus_list_unrealize                 (GtkWidget            *widget);
static void     nautilus_list_set_cell_contents         (NautilusCList        *clist,
							 NautilusCListRow     *row,
							 int                   column_index,
							 NautilusCellType      type,
							 const gchar          *text,
							 guint8                spacing,
							 GdkPixmap            *pixmap,
							 GdkBitmap            *mask);
static void     nautilus_list_size_request              (GtkWidget            *widget,
							 GtkRequisition       *requisition);
static void     nautilus_list_resize_column             (NautilusCList        *widget,
							 int                   column_index,
							 int                   width);
static void     nautilus_list_column_resize_track_start (GtkWidget            *widget,
							 int                   column_index);
static void     nautilus_list_column_resize_track       (GtkWidget            *widget,
							 int                   column_index);
static void     nautilus_list_column_resize_track_end   (GtkWidget            *widget,
							 int                   column_index);
static gboolean row_set_selected                        (NautilusList         *list,
							 int                   row_index,
							 NautilusCListRow     *row,
							 gboolean              select);
static gboolean select_row_unselect_others              (NautilusList         *list,
							 int                   row_to_select);
static void	nautilus_list_flush_typeselect_state 	(NautilusList 	      *container);
static int      insert_row                              (NautilusCList        *list,
							 int                   row,
							 char                 *text[]);
static void     nautilus_list_ensure_drag_data          (NautilusList         *list,
							 GdkDragContext       *context,
							 guint32               time);
static void     nautilus_list_start_auto_scroll         (NautilusList         *list);
static void     nautilus_list_stop_auto_scroll          (NautilusList         *list);

static void	unref_gcs				(NautilusList *list);


NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusList, nautilus_list, NAUTILUS_TYPE_CLIST)

static guint list_signals[LAST_SIGNAL];

static GtkTargetEntry drag_types [] = {
	{ NAUTILUS_ICON_DND_GNOME_ICON_LIST_TYPE, 0, NAUTILUS_ICON_DND_GNOME_ICON_LIST },
	{ NAUTILUS_ICON_DND_URI_LIST_TYPE, 0, NAUTILUS_ICON_DND_URI_LIST },
	{ NAUTILUS_ICON_DND_URL_TYPE, 0, NAUTILUS_ICON_DND_URL }
};


/* Standard class initialization function */
static void
nautilus_list_initialize_class (NautilusListClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	NautilusCListClass *clist_class;
	NautilusListClass *list_class;

	GtkBindingSet *clist_binding_set;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
	clist_class = (NautilusCListClass *) klass;
	list_class = (NautilusListClass *) klass;

	list_signals[CONTEXT_CLICK_SELECTION] =
		gtk_signal_new ("context_click_selection",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, context_click_selection),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER);
	list_signals[CONTEXT_CLICK_BACKGROUND] =
		gtk_signal_new ("context_click_background",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, context_click_background),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER);
	list_signals[ACTIVATE] =
		gtk_signal_new ("activate",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, activate),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER);
	list_signals[SELECTION_CHANGED] =
		gtk_signal_new ("selection_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, selection_changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	list_signals[SELECT_MATCHING_NAME] =
		gtk_signal_new ("select_matching_name",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, select_matching_name),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING, 0);
	list_signals[SELECT_PREVIOUS_NAME] =
		gtk_signal_new ("select_previous_name",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, select_previous_name),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	list_signals[SELECT_NEXT_NAME] =
		gtk_signal_new ("select_next_name",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, select_next_name),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	list_signals[HANDLE_DRAGGED_ITEMS] =
		gtk_signal_new ("handle_dragged_items",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, handle_dragged_items),
				nautilus_gtk_marshal_BOOL__INT_POINTER_INT_INT_UINT,
				GTK_TYPE_BOOL, 
				5,
				GTK_TYPE_INT,
				GTK_TYPE_POINTER,
				GTK_TYPE_INT,
				GTK_TYPE_INT,
				GTK_TYPE_UINT);
	list_signals[HANDLE_DROPPED_ITEMS] =
		gtk_signal_new ("handle_dropped_items",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, handle_dropped_items),
				nautilus_gtk_marshal_NONE__INT_POINTER_INT_INT_UINT,
				GTK_TYPE_NONE, 5,
				GTK_TYPE_INT,
				GTK_TYPE_POINTER,
				GTK_TYPE_INT,
				GTK_TYPE_INT,
				GTK_TYPE_UINT);
	list_signals[GET_DEFAULT_ACTION] =
		gtk_signal_new ("get_default_action",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, get_default_action),
				nautilus_gtk_marshal_NONE__POINTER_POINTER_POINTER_POINTER_INT_INT_UINT,
				GTK_TYPE_NONE, 7,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_INT,
				GTK_TYPE_INT,
				GTK_TYPE_UINT);
	list_signals[GET_DRAG_PIXMAP] =
		gtk_signal_new ("get_drag_pixmap",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, get_drag_pixmap),
				nautilus_gtk_marshal_NONE__POINTER_INT_POINTER_POINTER,
				GTK_TYPE_NONE, 4,
				GTK_TYPE_POINTER,
				GTK_TYPE_INT,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);
	list_signals[GET_SORT_COLUMN_INDEX] =
		gtk_signal_new ("get_sort_column_index",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusListClass, get_sort_column_index),
				nautilus_gtk_marshal_INT__NONE,
				GTK_TYPE_INT, 0);

	gtk_object_class_add_signals (object_class, list_signals, LAST_SIGNAL);

	/* Turn off the GtkCList key bindings that we want unbound.
	 * We only need to do this for the keys that we don't handle
	 * in nautilus_list_key_press. These extra ones are turned off
	 * to avoid inappropriate GtkCList code and to standardize the
	 * keyboard behavior in Nautilus.
	 */
	clist_binding_set = gtk_binding_set_by_class (clist_class);

	/* Use Control-A for Select All, not Control-/ */
	gtk_binding_entry_clear (clist_binding_set, 
				 '/', 
				 GDK_CONTROL_MASK);
	/* Don't use Control-\ for Unselect All (maybe invent Nautilus 
	 * standard for this?) */
	gtk_binding_entry_clear (clist_binding_set, 
				 '\\', 
				 GDK_CONTROL_MASK);
	/* Hide GtkCList's weird extend-selection-from-keyboard stuff.
	 * Users can use control-navigation and control-space to create
	 * extended selections.
	 */
	gtk_binding_entry_clear (clist_binding_set, 
				 GDK_Shift_L, 
				 GDK_RELEASE_MASK | GDK_SHIFT_MASK);
	gtk_binding_entry_clear (clist_binding_set, 
				 GDK_Shift_R, 
				 GDK_RELEASE_MASK | GDK_SHIFT_MASK);
	gtk_binding_entry_clear (clist_binding_set, 
				 GDK_Shift_L, 
				 GDK_RELEASE_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK);
	gtk_binding_entry_clear (clist_binding_set, 
				 GDK_Shift_R, 
				 GDK_RELEASE_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK);

	list_class->column_resize_track_start = nautilus_list_column_resize_track_start;
	list_class->column_resize_track = nautilus_list_column_resize_track;
	list_class->column_resize_track_end = nautilus_list_column_resize_track_end;

	clist_class->clear = nautilus_list_clear;
	clist_class->draw_row = draw_row;
	clist_class->draw_rows = draw_rows;
	clist_class->draw_all = draw_all;
	clist_class->insert_row = insert_row;
  	clist_class->resize_column = nautilus_list_resize_column;
  	clist_class->set_cell_contents = nautilus_list_set_cell_contents;
  	clist_class->select_all = nautilus_list_select_all;
  	clist_class->unselect_all = nautilus_list_unselect_all;

	widget_class->button_press_event = nautilus_list_button_press;
	widget_class->button_release_event = nautilus_list_button_release;
	widget_class->motion_notify_event = nautilus_list_motion;

	widget_class->draw = nautilus_list_draw;
	widget_class->expose_event = nautilus_list_expose;
	widget_class->draw_focus = nautilus_list_draw_focus;
	widget_class->key_press_event = nautilus_list_key_press;
	widget_class->style_set = nautilus_list_style_set;
	widget_class->realize = nautilus_list_realize;
	widget_class->unrealize = nautilus_list_unrealize;
	widget_class->size_request = nautilus_list_size_request;

	object_class->destroy = nautilus_list_destroy;
}

static gboolean 
event_state_modifies_selection (guint event_state)
{
	return (event_state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0;
}

void
nautilus_list_set_single_click_mode (NautilusList *list,
				     gboolean single_click_mode)
{
	list->details->single_click_mode = single_click_mode;
}


static void
nautilus_list_dnd_initialize (NautilusList *list)
{

	list->details->drag_info = g_new0 (NautilusDragInfo, 1);

	nautilus_drag_init (list->details->drag_info, drag_types,
			    NAUTILUS_N_ELEMENTS (drag_types), NULL);
	
	gtk_signal_connect (GTK_OBJECT (list), 
			    "drag_end", 
			    GTK_SIGNAL_FUNC(nautilus_list_drag_end), 
			    list);
	
	gtk_signal_connect (GTK_OBJECT (list), 
			    "drag_leave", 
			    GTK_SIGNAL_FUNC(nautilus_list_drag_leave), 
			    list);

	gtk_signal_connect (GTK_OBJECT (list),
			    "drag_motion", 
			    GTK_SIGNAL_FUNC(nautilus_list_drag_motion), 
			    list);

	gtk_signal_connect (GTK_OBJECT (list), 
			    "drag_drop", 
			    GTK_SIGNAL_FUNC(nautilus_list_drag_drop), 
			    list);

	gtk_signal_connect (GTK_OBJECT (list), 
			    "drag_data_received", 
			    GTK_SIGNAL_FUNC(nautilus_list_drag_data_received), 
			    list);


	/* Get ready to accept some dragged stuff. */
	gtk_drag_dest_set (GTK_WIDGET (list),
			   0,
			   nautilus_list_dnd_target_table,
			   NAUTILUS_N_ELEMENTS (nautilus_list_dnd_target_table),
			   GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK
			   | GDK_ACTION_ASK);



}

/* Standard object initialization function */
static void
nautilus_list_initialize (NautilusList *list)
{	

	list->details = g_new0 (NautilusListDetails, 1);
	list->details->anchor_row = -1;

	list->details->drag_prelight_row = NULL;
	
	/* GtkCList does not specify pointer motion by default */
	gtk_widget_add_events (GTK_WIDGET (list), GDK_POINTER_MOTION_MASK);


	nautilus_list_dnd_initialize (list);

	/* Emit "selection changed" signal when parent class changes selection */
	list->details->select_row_signal_id = gtk_signal_connect (GTK_OBJECT (list),
			    					  "select_row",
			    					  emit_selection_changed,
			    					  list);
	list->details->unselect_row_signal_id = gtk_signal_connect (GTK_OBJECT (list),
			    					    "unselect_row",
			    					    emit_selection_changed,
			    					    list);

	gtk_widget_push_composite_child ();
	list->details->title = GTK_WIDGET (nautilus_list_column_title_new());
	gtk_widget_pop_composite_child ();

	list->details->type_select_pattern = NULL;
	list->details->last_typeselect_time = G_GINT64_CONSTANT(0);
}

static void
nautilus_list_destroy (GtkObject *object)
{
	NautilusList *list;

	list = NAUTILUS_LIST (object);

	nautilus_drag_finalize (list->details->drag_info);

	unschedule_keyboard_row_reveal (list);

	NAUTILUS_CALL_PARENT_CLASS (GTK_OBJECT_CLASS, destroy, (object));

	unref_gcs (list);

	g_free (list->details->type_select_pattern);

	/* Must do this after calling the parent, because GtkCList calls
	 * the clear method, which must have a valid details pointer.
	 */
	g_free (list->details);
}

static void
emit_selection_changed (NautilusList *list) 
{
	g_assert (NAUTILUS_IS_LIST (list));
	gtk_signal_emit (GTK_OBJECT (list), list_signals[SELECTION_CHANGED]);
}

static void
activate_row_data_list (NautilusList *list, GList *activate_list)
{
	gtk_signal_emit (GTK_OBJECT (list),
			 list_signals[ACTIVATE],
			 activate_list);
}

static void
activate_selected_rows (NautilusList *list)
{
	GList *selection;

	selection = nautilus_list_get_selection (list);
	activate_row_data_list (list, selection);
	g_list_free (selection);
}

static void
activate_row (NautilusList *list, int row)
{
	GList *singleton_list;

	singleton_list = NULL;
	singleton_list = g_list_append (NULL, nautilus_clist_get_row_data (NAUTILUS_CLIST (list), row));
	activate_row_data_list (list, singleton_list);
	g_list_free (singleton_list);
}

gboolean
nautilus_list_is_row_selected (NautilusList *list, int row)
{
	NautilusCListRow *elem;

	g_return_val_if_fail (row >= 0, FALSE);
	g_return_val_if_fail (row < NAUTILUS_CLIST (list)->rows, FALSE);

	elem = g_list_nth (NAUTILUS_CLIST (list)->row_list, row)->data;

	return elem->state == GTK_STATE_SELECTED;
}

/* Selects the rows between the anchor to the specified row, inclusive.
 * Returns TRUE if selection changed.  */
static gboolean
select_range (NautilusList *list, int row)
{
	int min, max;
	int i;
	gboolean selection_changed;

	selection_changed = FALSE;

	if (list->details->anchor_row == -1) {
		list->details->anchor_row = row;
	}

	if (row < list->details->anchor_row) {
		min = row;
		max = list->details->anchor_row;
	} else {
		min = list->details->anchor_row;
		max = row;
	}

	for (i = min; i <= max; i++) {
		selection_changed |= row_set_selected (list, i, NULL, TRUE);
	}

	return selection_changed;
}

/* Handles row selection according to the specified modifier state */
static void
select_row_from_mouse (NautilusList *list, int row, guint state)
{
	int range, additive;
	gboolean should_select_row;
	gboolean selection_changed;

	selection_changed = FALSE;

	range = (state & GDK_SHIFT_MASK) != 0;
	additive = (state & GDK_CONTROL_MASK) != 0;

	nautilus_list_clear_keyboard_focus (list);

	if (!additive) {
		selection_changed |= select_row_unselect_others (list, -1);
	}

	if (range) {
		selection_changed |= select_range (list, row);
	} else {
		should_select_row = !additive || !nautilus_list_is_row_selected (list, row);
		selection_changed |= row_set_selected (list, row, NULL, should_select_row);
		list->details->anchor_row = row;
	}

	if (selection_changed) {
		emit_selection_changed (list);
	}
}

/* 
 * row_set_selected:
 * 
 * Select or unselect a row. Return TRUE if selection has changed. 
 * Does not emit the SELECTION_CHANGED signal; it's up to the caller
 * to handle that.
 *
 * @list: The NautilusList in question.
 * @row: index of row number to select or unselect.
 * @row: NautilusCListRow pointer for given list. Passing this avoids
 * expensive lookup. If it's NULL, it will be looked up in this function.
 * @select: TRUE if row should be selected, FALSE otherwise.
 * 
 * Return Value: TRUE if selection has changed, FALSE otherwise.
 */
static gboolean
row_set_selected (NautilusList *list, int row_index, NautilusCListRow *row, gboolean select)
{
	g_assert (row_index >= 0 && row_index < NAUTILUS_CLIST (list)->rows);

	if (row == NULL) {
		row = ROW_ELEMENT (NAUTILUS_CLIST (list), row_index)->data;
	}

	if (select == (row->state == GTK_STATE_SELECTED)) {
		return FALSE;
	}

	/* Block signal handlers so we can make sure the selection-changed
	 * signal gets sent only once.
	 */
	gtk_signal_handler_block (GTK_OBJECT(list), 
				  list->details->select_row_signal_id);
	gtk_signal_handler_block (GTK_OBJECT(list), 
				  list->details->unselect_row_signal_id);
	
	if (select) {
		nautilus_clist_select_row (NAUTILUS_CLIST (list), row_index, -1);
	} else {
		nautilus_clist_unselect_row (NAUTILUS_CLIST (list), row_index, -1);
	}

	gtk_signal_handler_unblock (GTK_OBJECT(list), 
				    list->details->select_row_signal_id);
	gtk_signal_handler_unblock (GTK_OBJECT(list), 
				    list->details->unselect_row_signal_id);

	return TRUE;
}

/**
 * select_row_unselect_others:
 * 
 * Change the selected rows as necessary such that only
 * the given row remains selected.
 * 
 * @list: The NautilusList in question.
 * @row_to_select: The row number to leave selected. Use -1 to leave
 * no row selected.
 * 
 * Return value: TRUE if the selection changed; FALSE otherwise.
 */
static gboolean
select_row_unselect_others (NautilusList *list, int row_to_select)
{
	GList *p;
	int row_index;
	gboolean selection_changed;

	g_return_val_if_fail (NAUTILUS_IS_LIST (list), FALSE);

	selection_changed = FALSE;
	for (p = NAUTILUS_CLIST (list)->row_list, row_index = 0; p != NULL; p = p->next, ++row_index) {
		selection_changed |= row_set_selected (list, row_index, p->data, row_index == row_to_select);
	}

	return selection_changed;
}

static void
nautilus_list_unselect_all (NautilusCList *clist)
{
	g_return_if_fail (NAUTILUS_IS_LIST (clist));

	if (select_row_unselect_others (NAUTILUS_LIST (clist), -1)) {
		emit_selection_changed (NAUTILUS_LIST (clist));
	}
}

static void
nautilus_list_select_all (NautilusCList *clist)
{
	GList *p;
	int row_index;
	gboolean selection_changed;

	g_return_if_fail (NAUTILUS_IS_LIST (clist));

	selection_changed = FALSE;
	for (p = clist->row_list, row_index = 0; p != NULL; p = p->next, ++row_index) {
		selection_changed |= row_set_selected (NAUTILUS_LIST (clist), row_index, p->data, TRUE);
	}

	if (selection_changed) {
		emit_selection_changed (NAUTILUS_LIST (clist));
	}
}

typedef struct {
	NautilusList 	*list;
	GdkEventButton	*event;
} ContextMenuParameters;

static ContextMenuParameters *
context_menu_parameters_new (NautilusList *list,
			     GdkEventButton *event)
{
	ContextMenuParameters *parameters;

	parameters = g_new (ContextMenuParameters, 1);
	parameters->list = list;
	parameters->event = (GdkEventButton *)(gdk_event_copy ((GdkEvent *)event));

	return parameters;
}			     

static void
context_menu_parameters_free (ContextMenuParameters *parameters)
{
	gdk_event_free ((GdkEvent *)parameters->event);
	g_free (parameters);
}

static gboolean
show_context_menu_callback (void *cast_to_parameters)
{
	ContextMenuParameters *parameters;

	parameters = (ContextMenuParameters *)cast_to_parameters;

	g_assert (NAUTILUS_IS_LIST (parameters->list));

	/* FIXME bugzilla.eazel.com 2574: 
	 * Need to handle case where button has already been released,
	 * a la NautilusIconContainer code?
	 */

	gtk_timeout_remove (parameters->list->details->context_menu_timeout_id);

	/* Context menu applies to all selected items. The only
	 * odd case is if this click deselected the item under
	 * the mouse, but at least the behavior is consistent.
	 */
	gtk_signal_emit (GTK_OBJECT (parameters->list),
			 list_signals[CONTEXT_CLICK_SELECTION],
			 parameters->event);

	context_menu_parameters_free (parameters);

	return TRUE;
}

/* Our handler for button_press events.  We override all of GtkCList's broken
 * behavior.
 */
static int
nautilus_list_button_press (GtkWidget *widget, GdkEventButton *event)
{
	NautilusList *list;
	NautilusCList *clist;
	int on_row;
	int row_index, column_index;
	int retval;

	g_return_val_if_fail (NAUTILUS_IS_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	list = NAUTILUS_LIST (widget);
	clist = NAUTILUS_CLIST (widget);
	retval = FALSE;

	if (!GTK_WIDGET_HAS_FOCUS (widget)) {
		gtk_widget_grab_focus (widget);
	}


	/* Forget the typeahead state. */
	nautilus_list_flush_typeselect_state (list);

	if (event->window != clist->clist_window)
		return NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, button_press_event, (widget, event));

	on_row = nautilus_clist_get_selection_info (clist, event->x, event->y, &row_index, &column_index);
	list->details->button_down_time = event->time;
	list->details->drag_started = FALSE;

	list->details->button_down_row = -1;
		
	switch (event->type) {
	case GDK_BUTTON_PRESS:
	
		if (event->button == CONTEXTUAL_MENU_BUTTON && !on_row) {
			gtk_signal_emit (GTK_OBJECT (list),
					 list_signals[CONTEXT_CLICK_BACKGROUND],
					 event);

			retval = TRUE;
		} else if (event->button == ACTION_BUTTON || event->button == CONTEXTUAL_MENU_BUTTON) {
			if (on_row) {

				if (event->button == CONTEXTUAL_MENU_BUTTON) {
					/* after a timeout we will decide if this is a
					 * context menu click or a drag start
					 */
					list->details->context_menu_timeout_id = gtk_timeout_add (
						CONTEXT_MENU_TIMEOUT_INTERVAL, 
						show_context_menu_callback, 				
						context_menu_parameters_new (list, event));
				}

				/* Save the clicked row_index for DnD and single-click activate */
				
				list->details->button_down_row = row_index;

				/* Save the mouse info for DnD */

				list->details->dnd_press_button = event->button;
				list->details->dnd_press_x = event->x;
				list->details->dnd_press_y = event->y;
	
				/* Handle selection */

				if ((nautilus_list_is_row_selected (list, row_index)
				     && !event_state_modifies_selection (event->state))
				    || ((event->state & GDK_CONTROL_MASK)
					&& !(event->state & GDK_SHIFT_MASK))) {
					/* don't change selection just yet, wait for 
					 * possible drag
					 */
					list->details->dnd_select_pending = TRUE;
					list->details->dnd_select_pending_state = event->state;
				}

				if (!list->details->dnd_select_pending) {
					select_row_from_mouse (list, row_index, event->state);
				}
			} else {
				nautilus_clist_unselect_all (clist);
			}

			retval = TRUE;
		}

		break;

	case GDK_2BUTTON_PRESS:
		if (event->button == ACTION_BUTTON) {
			list->details->dnd_select_pending = FALSE;
			list->details->dnd_select_pending_state = 0;

			if (on_row && !list->details->single_click_mode) {
				/* We'll just eat the 2nd click if in single-click mode. */
				activate_selected_rows (list);
			}

			retval = TRUE;
			break;
		}

	default:
		break;
	}

	return retval;
}

/* Our handler for button_release events.  We override all of GtkCList's broken
 * behavior.
 */
static int
nautilus_list_button_release (GtkWidget *widget, GdkEventButton *event)
{
	NautilusList *list;
	NautilusCList *clist;
	NautilusCListRow *row;
	int on_row;
	int row_index, column_index;
	GtkStyle *style;
	int text_x, text_width;
	int retval;

	g_return_val_if_fail (NAUTILUS_IS_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	list = NAUTILUS_LIST (widget);
	clist = NAUTILUS_CLIST (widget);
	retval = FALSE;

	if (event->window != clist->clist_window)
		return NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, button_release_event, (widget, event));

	on_row = nautilus_clist_get_selection_info (clist, event->x, event->y, &row_index, &column_index);

	if (event->button != ACTION_BUTTON && event->button != CONTEXTUAL_MENU_BUTTON)
		return FALSE;

	if (on_row) {
		/* Clean up after abortive drag-and-drop attempt (since user can't
		 * reorder list view items, releasing mouse in list view cancels
		 * drag-and-drop possibility). 
		 */
		if (list->details->dnd_select_pending) {
			/* If clicked on a selected item, don't change selection 
			 * (unless perhaps if modifiers were used)
			 */
			if (!nautilus_list_is_row_selected (list, row_index) 
			    || event_state_modifies_selection (list->details->dnd_select_pending_state)) {
				select_row_from_mouse (list,
					    	       list->details->button_down_row,
					    	       list->details->dnd_select_pending_state);
			}

			list->details->dnd_select_pending = FALSE;
			list->details->dnd_select_pending_state = 0;
		}

		if (event->button == CONTEXTUAL_MENU_BUTTON && !list->details->drag_started) {
			/* Right click, drag never happened, immediately show context menu */
			gtk_timeout_remove (list->details->context_menu_timeout_id);
			gtk_signal_emit (GTK_OBJECT (list),
					 list_signals[CONTEXT_CLICK_SELECTION],
					 event);
		}

		/* 
		 * Activate on single click if not extending selection, mouse hasn't moved to
		 * a different row, not too much time has passed, and this is a link-type cell.
		 */
		if (event->button == ACTION_BUTTON && 
		    list->details->single_click_mode && 
		    !event_state_modifies_selection (event->state)) {
			int elapsed_time = event->time - list->details->button_down_time;

			if (elapsed_time < MAX_CLICK_TIME && list->details->button_down_row == row_index) {
				row = ROW_ELEMENT (clist, row_index)->data;
				if (row->cell[column_index].type == NAUTILUS_CELL_LINK_TEXT) {
					/* One final test. Check whether the click was in the
					 * horizontal bounds of the displayed text.
					 */
					get_cell_style (list, row, GTK_STATE_NORMAL, row_index, column_index, &style, NULL, NULL, NULL);
					text_width = gdk_string_width (style->font, NAUTILUS_CELL_TEXT (row->cell[column_index])->text);
					text_x = get_cell_horizontal_start_position (clist, row, column_index, text_width);
					if (event->x >= text_x && event->x <= text_x + text_width) {
						/* Note that we activate only the clicked-on item,
						 * not all selected items. This is because the UI
						 * feedback makes it clear that you're clicking on
						 * a link to activate that link, rather than activating
						 * the whole selection.
						 */
						activate_row (list, row_index);
					}
				}
			}
		}		
	
		retval = TRUE;
	}

	list->details->dnd_press_button = NO_BUTTON;
	list->details->dnd_press_x = 0;
	list->details->dnd_press_y = 0;
	list->details->drag_started = FALSE;

	return retval;
}

static void
nautilus_list_clear_keyboard_focus (NautilusList *list)
{
	if (NAUTILUS_CLIST (list)->focus_row >= 0) {
		gtk_widget_draw_focus (GTK_WIDGET (list));
	}

	NAUTILUS_CLIST (list)->focus_row = -1;
}

static void
nautilus_list_set_keyboard_focus (NautilusList *list, int row_index)
{
	g_assert (row_index >= 0 && row_index < NAUTILUS_CLIST (list)->rows);

	if (row_index == NAUTILUS_CLIST (list)->focus_row) {
		return;
	}

	nautilus_list_clear_keyboard_focus (list);

	NAUTILUS_CLIST (list)->focus_row = row_index;

	gtk_widget_draw_focus (GTK_WIDGET (list));
}

static void
nautilus_list_keyboard_move_to (NautilusList *list, int row_index, GdkEventKey *event)
{
	NautilusCList *clist;

	g_assert (NAUTILUS_IS_LIST (list));
	g_assert (row_index >= 0 || row_index < NAUTILUS_CLIST (list)->rows);

	clist = NAUTILUS_CLIST (list);

	if (event != NULL && (event->state & GDK_CONTROL_MASK) != 0) {
		/* Move the keyboard focus. */
		nautilus_list_set_keyboard_focus (list, row_index);
	} else {
		/* Select row_index and get rid of special keyboard focus. */
		nautilus_list_clear_keyboard_focus (list);
		if (select_row_unselect_others (list, row_index)) {
			emit_selection_changed (list);
		}
	}

	schedule_keyboard_row_reveal (list, row_index);
}

void
nautilus_list_select_row (NautilusList *list, int row_index)
{
	g_assert (NAUTILUS_IS_LIST (list));
	g_assert (row_index >= 0);

	if (row_index >= NAUTILUS_CLIST (list)->rows)
		row_index = NAUTILUS_CLIST (list)->rows - 1;

	nautilus_list_keyboard_move_to (list, row_index, NULL);
}

static gboolean
keyboard_row_reveal_timeout_callback (gpointer data)
{
	NautilusList *list;
	int row_index;

	GDK_THREADS_ENTER ();

	list = NAUTILUS_LIST (data);
	row_index = list->details->keyboard_row_to_reveal;

	if (row_index >= 0 && row_index < NAUTILUS_CLIST (list)->rows) {	
		/* Only reveal the icon if it's still the keyboard
		 * focus or if it's still selected. Someone originally
		 * thought we should cancel this reveal if the user
		 * manages to sneak a direct scroll in before the
		 * timeout fires, but we later realized this wouldn't
		 * actually be an improvement (see bugzilla.eazel.com
		 * 612).
		 */
		if (row_index == NAUTILUS_CLIST (list)->focus_row
		    || nautilus_list_is_row_selected (list, row_index)) {
			nautilus_list_reveal_row (list, row_index);
		}
		list->details->keyboard_row_reveal_timer_id = 0;
	}

	GDK_THREADS_LEAVE ();

	return FALSE;
}

static void
unschedule_keyboard_row_reveal (NautilusList *list) 
{
	if (list->details->keyboard_row_reveal_timer_id != 0) {
		gtk_timeout_remove (list->details->keyboard_row_reveal_timer_id);
	}
}

static void
schedule_keyboard_row_reveal (NautilusList *list, int row_index)
{
	unschedule_keyboard_row_reveal (list);

	list->details->keyboard_row_to_reveal = row_index;
	list->details->keyboard_row_reveal_timer_id
		= gtk_timeout_add (KEYBOARD_ROW_REVEAL_TIMEOUT,
				   keyboard_row_reveal_timeout_callback,
				   list);
}

void
nautilus_list_reveal_row (NautilusList *list, int row_index)
{
	NautilusCList *clist;

	g_return_if_fail (NAUTILUS_IS_LIST (list));
	g_return_if_fail (row_index >= 0 && row_index < NAUTILUS_CLIST (list) ->rows);
	
	clist = NAUTILUS_CLIST (list);
	
	if (ROW_TOP_YPIXEL (clist, row_index) + clist->row_height >
      		      clist->clist_window_height) {
		nautilus_clist_moveto (clist, row_index, -1, 1, 0);
     	} else if (ROW_TOP_YPIXEL (clist, row_index) < 0) {
		nautilus_clist_moveto (clist, row_index, -1, 0, 0);
     	}
}

static void
nautilus_list_keyboard_navigation_key_press (NautilusList *list, GdkEventKey *event,
			          	     GtkScrollType scroll_type, gboolean jump_to_end)
{
	NautilusCList *clist;
	int start_row;
	int destination_row;
	int rows_per_page;

	g_assert (NAUTILUS_IS_LIST (list));

	clist = NAUTILUS_CLIST (list);
	
	if (scroll_type == GTK_SCROLL_JUMP) {
		destination_row = (jump_to_end ?
				   clist->rows - 1 :
				   0);
	} else {
		/* Choose the row to start with.
		 * If we have a keyboard focus, start with it.
		 * If there's a selection, use the selected row farthest toward the end.
		 */

		if (clist->focus_row >= 0) {
			start_row = clist->focus_row;
		} else {
			start_row = (scroll_type == GTK_SCROLL_STEP_FORWARD 
					|| scroll_type == GTK_SCROLL_PAGE_FORWARD ?
				     nautilus_list_get_last_selected_row (list) :
				     nautilus_list_get_first_selected_row (list));
		}

		/* If there's no row to start with, select the row farthest toward the end.
		 * If there is a row to start with, select the next row in the arrow direction.
		 */
		if (start_row < 0) {
			destination_row = (scroll_type == GTK_SCROLL_STEP_FORWARD 
						|| scroll_type == GTK_SCROLL_PAGE_FORWARD 
					   ? 0 : clist->rows - 1);
		} else if (scroll_type == GTK_SCROLL_STEP_FORWARD) {
			destination_row = MIN (clist->rows - 1, start_row + 1);
		} else if (scroll_type == GTK_SCROLL_STEP_BACKWARD) {
			destination_row = MAX (0, start_row - 1);
		} else {
			g_assert (scroll_type == GTK_SCROLL_PAGE_FORWARD || GTK_SCROLL_PAGE_BACKWARD);
			rows_per_page = (2 * clist->clist_window_height -
					 clist->row_height - CELL_SPACING) /
					(2 * (clist->row_height + CELL_SPACING));
			
			if (scroll_type == GTK_SCROLL_PAGE_FORWARD) {
				destination_row = MIN (clist->rows - 1, 
						       start_row + rows_per_page);
			} else {
				destination_row = MAX (0,
						       start_row - rows_per_page);
			}
		}
	}

	nautilus_list_keyboard_move_to (list, destination_row, event);
}			   

static void
nautilus_list_keyboard_home (NautilusList *list, GdkEventKey *event)
{
	/* Home selects the first row.
	 * Control-Home sets the keyboard focus to the first row.
	 */
	nautilus_list_keyboard_navigation_key_press (list, event, GTK_SCROLL_JUMP, FALSE); 
}

static void
nautilus_list_keyboard_end (NautilusList *list, GdkEventKey *event)
{
	/* End selects the last row.
	 * Control-End sets the keyboard focus to the last row.
	 */
	nautilus_list_keyboard_navigation_key_press (list, event, GTK_SCROLL_JUMP, TRUE); 
}

static void
nautilus_list_keyboard_up (NautilusList *list, GdkEventKey *event)
{
	/* Up selects the next higher row.
	 * Control-Up sets the keyboard focus to the next higher icon.
	 */
	nautilus_list_keyboard_navigation_key_press (list, event, GTK_SCROLL_STEP_BACKWARD, FALSE); 
}

static void
nautilus_list_keyboard_down (NautilusList *list, GdkEventKey *event)
{
	/* Down selects the next lower row.
	 * Control-Down sets the keyboard focus to the next lower icon.
	 */
	nautilus_list_keyboard_navigation_key_press (list, event, GTK_SCROLL_STEP_FORWARD, FALSE); 
}

static void
nautilus_list_keyboard_page_up (NautilusList *list, GdkEventKey *event)
{
	/* Page Up selects a row one screenful higher.
	 * Control-Page Up sets the keyboard focus to the row one screenful higher.
	 */
	nautilus_list_keyboard_navigation_key_press (list, event, GTK_SCROLL_PAGE_BACKWARD, FALSE); 
}

static void
nautilus_list_keyboard_page_down (NautilusList *list, GdkEventKey *event)
{
	/* Page Down selects a row one screenful lower.
	 * Control-Page Down sets the keyboard focus to the row one screenful lower.
	 */
	nautilus_list_keyboard_navigation_key_press (list, event, GTK_SCROLL_PAGE_FORWARD, FALSE); 
}

static void
nautilus_list_keyboard_space (NautilusList *list, GdkEventKey *event)
{
	if (event->state & GDK_CONTROL_MASK) {
		gtk_signal_emit_by_name (GTK_OBJECT (list), "toggle_focus_row");
	}
}

static void
nautilus_list_activate_selected_items (NautilusList *list)
{
	int row_index;

	for (row_index = 0; row_index < NAUTILUS_CLIST (list)->rows; ++row_index) {
		if (nautilus_list_is_row_selected (list, row_index)) {
			activate_row (list, row_index);
		}
	}
}

static void
nautilus_list_flush_typeselect_state (NautilusList *list)
{
	g_free (list->details->type_select_pattern);
	list->details->type_select_pattern = NULL;
	list->details->last_typeselect_time = G_GINT64_CONSTANT(0);
}

enum {
	NAUTILUS_TYPESELECT_FLUSH_DELAY = 1000000
	/* After this time the current typeselect buffer will be
	 * thrown away and the new pressed character will be made
	 * the the start of a new pattern.
	 */
};

static gboolean
nautilus_list_handle_typeahead (NautilusList *list, const char *key_string)
{
	char *new_pattern;
	gint64 now;
	gint64 time_delta;
	int key_string_length;
	int index;

	g_assert (key_string != NULL);
	g_assert (strlen (key_string) < 5);

	key_string_length = strlen (key_string);

	if (key_string_length == 0) {
		/* can be an empty string if the modifier was held down, etc. */
		return FALSE;
	}

	/* only handle if printable keys typed */
	for (index = 0; index < key_string_length; index++) {
		if (!isprint (key_string[index])) {
			return FALSE;
		}
	}

	/* find out how long since last character was typed */
	now = nautilus_get_system_time();
	time_delta = now - list->details->last_typeselect_time;
	if (time_delta < 0 || time_delta > NAUTILUS_TYPESELECT_FLUSH_DELAY) {
		/* the typeselect state is too old, start with a fresh one */
		g_free (list->details->type_select_pattern);
		list->details->type_select_pattern = NULL;
	}

	if (list->details->type_select_pattern != NULL) {
		new_pattern = g_strconcat
			(list->details->type_select_pattern,
			 key_string, NULL);
		g_free (list->details->type_select_pattern);
	} else {
		new_pattern = g_strdup (key_string);
	}

	list->details->type_select_pattern = new_pattern;
	list->details->last_typeselect_time = now;
	
	gtk_signal_emit (GTK_OBJECT (list), list_signals[SELECT_MATCHING_NAME], new_pattern);

	return TRUE;
}

static int
nautilus_list_key_press (GtkWidget *widget,
		 	 GdkEventKey *event)
{
	NautilusList *list;

	list = NAUTILUS_LIST (widget);

	switch (event->keyval) {
	case GDK_Home:
		nautilus_list_keyboard_home (list, event);
		break;
	case GDK_End:
		nautilus_list_keyboard_end (list, event);
		break;
	case GDK_Page_Up:
		nautilus_list_keyboard_page_up (list, event);
		break;
	case GDK_Page_Down:
		nautilus_list_keyboard_page_down (list, event);
		break;
	case GDK_Up:
		nautilus_list_keyboard_up (list, event);
		break;
	case GDK_Down:
		nautilus_list_keyboard_down (list, event);
		break;
	case GDK_space:
		nautilus_list_keyboard_space (list, event);
		break;
	case GDK_Return:
		nautilus_list_activate_selected_items (list);
		break;
	case GDK_Tab:
	case GDK_ISO_Left_Tab:
		if ((event->state & GDK_SHIFT_MASK) == 0) {
			gtk_signal_emit (GTK_OBJECT (list), list_signals[SELECT_PREVIOUS_NAME]);
		} else {
			gtk_signal_emit (GTK_OBJECT (list), list_signals[SELECT_NEXT_NAME]);
		}
		break;
	default:
		/* Don't use Control or Alt keys for type-selecting, because they
		 * might be used for menus.
		 */
		if ((event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0 &&
		     nautilus_list_handle_typeahead (list, event->string)) {
			return TRUE;
		}

		return NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, key_press_event, (widget, event));
	}

	return TRUE;
}

static guint32
nautilus_gdk_set_shifted_foreground_gc_color (GdkGC *gc, guint32 color, float shift_by)
{
	guint32 shifted_color;

	shifted_color = nautilus_rgb_shift_color (color, shift_by);
	gdk_rgb_gc_set_foreground (gc, shifted_color);

	return shifted_color;
}

static GdkGC *
nautilus_gdk_gc_copy (GdkGC *source, GdkWindow *window)
{
	GdkGC *result;

	result = gdk_gc_new (window);
	gdk_gc_copy (result, source);

	/* reset some properties to be on the safe side */
	gdk_gc_set_function (result, GDK_COPY);
	gdk_gc_set_fill (result, GDK_SOLID);
	gdk_gc_set_clip_origin (result, 0, 0);
	gdk_gc_set_clip_mask (result, NULL);

	return result;
}

static void
nautilus_list_setup_style_colors (NautilusList *list)
{
	guint32 style_background_color;
	guint32 selection_background_color;
	GdkColor text_color;

	gdk_rgb_init();

	style_background_color = nautilus_gdk_color_to_rgb
		(&GTK_WIDGET (list)->style->bg [GTK_STATE_NORMAL]);
	selection_background_color = nautilus_gdk_color_to_rgb
		(&GTK_WIDGET (list)->style->bg [GTK_STATE_SELECTED]);

	list->details->cell_lighter_background_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->cell_lighter_background, 
							    style_background_color, 1);

	list->details->cell_darker_background_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->cell_darker_background, 
							    style_background_color, 1.05);

	list->details->cell_selected_lighter_background_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->cell_selected_lighter_background, 
							    style_background_color, 1.05);

	list->details->cell_selected_darker_background_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->cell_selected_darker_background, 
							    style_background_color, 1.10);

	nautilus_gdk_set_shifted_foreground_gc_color (list->details->cell_divider_color, 
						      style_background_color, 0.8);

	list->details->selection_main_color_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->selection_main_color, 
							    selection_background_color, 1);
		
	list->details->selection_medium_color_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->selection_medium_color, 
							    selection_background_color, 0.7);

	list->details->selection_light_color_rgb
	    = nautilus_gdk_set_shifted_foreground_gc_color (list->details->selection_light_color, 
							    selection_background_color, 0.5);

	text_color = GTK_WIDGET (list)->style->fg[GTK_STATE_NORMAL];
	nautilus_gdk_gc_choose_foreground_color (list->details->text_color, &text_color,
						 &GTK_WIDGET (list)->style->bg[GTK_STATE_NORMAL]);

	text_color = GTK_WIDGET (list)->style->fg[GTK_STATE_SELECTED];
	nautilus_gdk_gc_choose_foreground_color (list->details->selected_text_color, &text_color,
						 &GTK_WIDGET (list)->style->bg[GTK_STATE_SELECTED]);

	text_color.red = 0;
	text_color.green = 0;
	text_color.blue = 65535;
	nautilus_gdk_gc_choose_foreground_color (list->details->link_text_color, &text_color,
						 &GTK_WIDGET (list)->style->bg[GTK_STATE_NORMAL]);
}

static void
unref_a_gc (GdkGC **gc)
{
	if (*gc != NULL) {
		gdk_gc_unref (*gc);
		*gc = NULL;
	}
}

static void
unref_gcs (NautilusList *list)
{
	g_return_if_fail (NAUTILUS_IS_LIST (list));

	unref_a_gc (&list->details->cell_lighter_background);
	unref_a_gc (&list->details->cell_darker_background);
	unref_a_gc (&list->details->cell_selected_lighter_background);
	unref_a_gc (&list->details->cell_selected_darker_background);
	unref_a_gc (&list->details->cell_divider_color);
	unref_a_gc (&list->details->selection_light_color);
	unref_a_gc (&list->details->selection_medium_color);
	unref_a_gc (&list->details->selection_main_color);
	unref_a_gc (&list->details->text_color);
	unref_a_gc (&list->details->selected_text_color);
	unref_a_gc (&list->details->link_text_color);
}

static void
make_gcs_and_colors (NautilusList *list)
{
	GtkWidget *widget;

	g_return_if_fail (NAUTILUS_IS_LIST (list));
	g_return_if_fail (GTK_IS_WIDGET (list));

	widget = GTK_WIDGET (list);

	/* First unref old gcs */
	unref_gcs (list);

	/* now setup new ones */
	list->details->cell_lighter_background = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_NORMAL], widget->window);
	list->details->cell_darker_background = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_NORMAL], widget->window);
	list->details->cell_selected_lighter_background = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_NORMAL], widget->window);
	list->details->cell_selected_darker_background = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_NORMAL], widget->window);
	list->details->cell_divider_color = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_NORMAL], widget->window);
	list->details->selection_light_color = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_SELECTED], widget->window);
	list->details->selection_medium_color = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_SELECTED], widget->window);
	list->details->selection_main_color = nautilus_gdk_gc_copy (
		widget->style->bg_gc[GTK_STATE_SELECTED], widget->window);

	list->details->text_color = nautilus_gdk_gc_copy (
		widget->style->fg_gc[GTK_STATE_NORMAL], widget->window);
	list->details->selected_text_color = nautilus_gdk_gc_copy (
		widget->style->fg_gc[GTK_STATE_SELECTED], widget->window);
	list->details->link_text_color = nautilus_gdk_gc_copy (
		widget->style->fg_gc[GTK_STATE_NORMAL], widget->window);

	nautilus_list_setup_style_colors (list);
}

static void
nautilus_list_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	NautilusList *list;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));

	list = NAUTILUS_LIST (widget);

	NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, style_set, (widget, previous_style));

	if (GTK_WIDGET_REALIZED (widget)) {
		make_gcs_and_colors (list);
	}
}

static void
nautilus_list_realize (GtkWidget *widget)
{
	NautilusList *list;
	NautilusCList *clist;
	GtkWindow *window;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));

	list = NAUTILUS_LIST (widget);
	clist = NAUTILUS_CLIST (widget);

	clist->column[0].button = list->details->title;

	NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, realize, (widget));

	make_gcs_and_colors (list);

	if (list->details->title) {
		gtk_widget_set_parent_window (list->details->title, clist->title_window);
		gtk_widget_set_parent (list->details->title, GTK_WIDGET (clist));
		gtk_widget_show (list->details->title);
	}

	/* make us the focused widget */
        g_assert (GTK_IS_WINDOW (gtk_widget_get_toplevel (widget)));
        window = GTK_WINDOW (gtk_widget_get_toplevel (widget));
	gtk_window_set_focus (window, widget);
	
	NAUTILUS_CLIST_SET_FLAG (clist, CLIST_SHOW_TITLES);
}

static void
nautilus_list_unrealize (GtkWidget *widget)
{
	GtkWindow *window;
        window = GTK_WINDOW (gtk_widget_get_toplevel (widget));
	gtk_window_set_focus (window, NULL);

	/* unref all the gcs we've created */
	unref_gcs (NAUTILUS_LIST (widget));

	NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, unrealize, (widget));
}

/* this is here just temporarily */
static int
list_requisition_width (NautilusCList *clist) 
{
	int width = CELL_SPACING;
	int i;

	for (i = clist->columns - 1; i >= 0; i--) {
		if (!clist->column[i].visible)
			continue;

		if (clist->column[i].width_set)
			width += clist->column[i].width + CELL_SPACING + (2 * COLUMN_INSET);
		else if (NAUTILUS_CLIST_SHOW_TITLES(clist) && clist->column[i].button)
			width += clist->column[i].button->requisition.width;
	}

	return width;
}


static void
nautilus_list_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	/* stolen from gtk_clist 
	 * make sure the proper title ammount is allocated for the column
	 * title view --  this would not otherwise be done because 
	 * NautilusList depends the buttons being there when doing a size calculation
	 */
	NautilusList *list;
	NautilusCList *clist;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));
	g_return_if_fail (requisition != NULL);

	clist = NAUTILUS_CLIST (widget);
	list = NAUTILUS_LIST (widget);

	requisition->width = 0;
	requisition->height = 0;

	/* compute the size of the column title (title) area */
	clist->column_title_area.height = 0;
	if (NAUTILUS_CLIST_SHOW_TITLES(clist) && list->details->title) {
		GtkRequisition child_requisition;
		
		gtk_widget_size_request (list->details->title,
					 &child_requisition);

		child_requisition.height = 20;
			/* for now */

		clist->column_title_area.height =
			MAX (clist->column_title_area.height,
			     child_requisition.height);
	}

	requisition->width += (widget->style->klass->xthickness +
			       GTK_CONTAINER (widget)->border_width) * 2;
	requisition->height += (clist->column_title_area.height +
				(widget->style->klass->ythickness +
				GTK_CONTAINER (widget)->border_width) * 2);


	requisition->width += list_requisition_width (clist);
	requisition->height += LIST_HEIGHT (clist);
}

static int
new_column_width (NautilusCList *clist, int column_index,  int *x)
{
	int xthickness = GTK_WIDGET (clist)->style->klass->xthickness;
	int width;
	int cx;
	int dx;
	int last_column;

	/* first translate the x position from widget->window
	 * to clist->clist_window */
	cx = *x - xthickness;

	for (last_column = clist->columns - 1;
		last_column >= 0 && !clist->column[last_column].visible; last_column--);

	/* calculate new column width making sure it doesn't end up
	 * less than the minimum width */
	dx = (COLUMN_LEFT_XPIXEL (clist, column_index) + COLUMN_INSET +
		(column_index < last_column) * CELL_SPACING);
	width = cx - dx;

	if (width < MAX (COLUMN_MIN_WIDTH, clist->column[column_index].min_width)) {
		width = MAX (COLUMN_MIN_WIDTH, clist->column[column_index].min_width);
		cx = dx + width;
		*x = cx + xthickness;
	} else if (clist->column[column_index].max_width >= COLUMN_MIN_WIDTH &&
	   width > clist->column[column_index].max_width) {
		width = clist->column[column_index].max_width;
		cx = dx + clist->column[column_index].max_width;
		*x = cx + xthickness;
    	}

	if (cx < 0 || cx > clist->clist_window_width)
		*x = -1;

	return width;
}

static void
size_allocate_columns (NautilusCList *clist, gboolean  block_resize)
{
	int xoffset = CELL_SPACING + COLUMN_INSET;
	int last_column;
	int i;

	/* find last visible column and calculate correct column width */
	for (last_column = clist->columns - 1;
	     last_column >= 0 && !clist->column[last_column].visible; last_column--)
		;

	if (last_column < 0)
		return;

	for (i = 0; i <= last_column; i++)  {
		if (!clist->column[i].visible)
			continue;

		clist->column[i].area.x = xoffset;
		if (clist->column[i].width_set) {
			if (!block_resize && NAUTILUS_CLIST_SHOW_TITLES(clist) &&
				clist->column[i].auto_resize && clist->column[i].button) {
				int width;

				width = (clist->column[i].button->requisition.width -
					(CELL_SPACING + (2 * COLUMN_INSET)));

				if (width > clist->column[i].width)
					nautilus_clist_set_column_width (clist, i, width);
			}

			clist->column[i].area.width = clist->column[i].width;
			xoffset += clist->column[i].width + CELL_SPACING + (2 * COLUMN_INSET);
		} else if (NAUTILUS_CLIST_SHOW_TITLES(clist) && clist->column[i].button) {
			clist->column[i].area.width =
				clist->column[i].button->requisition.width -
				(CELL_SPACING + (2 * COLUMN_INSET));
			xoffset += clist->column[i].button->requisition.width;
		}
	}

	clist->column[last_column].area.width 
		+= MAX (0, clist->clist_window_width + COLUMN_INSET - xoffset);
}

static void
size_allocate_title_buttons (NautilusCList *clist)
{
	GtkAllocation button_allocation;
	int last_column;
	int last_button = 0;
	int i;

	button_allocation.x = clist->hoffset;
	button_allocation.y = 0;
	button_allocation.width = 0;
	button_allocation.height = clist->column_title_area.height;

	/* find last visible column */
	for (last_column = clist->columns - 1; last_column >= 0; last_column--)
		if (clist->column[last_column].visible)
			break;

	for (i = 0; i < last_column; i++) {
		if (!clist->column[i].visible) {
			last_button = i + 1;
			gdk_window_hide (clist->column[i].window);
			continue;
		}

		button_allocation.width += (clist->column[i].area.width +
				  	    CELL_SPACING + 2 * COLUMN_INSET);

		if (!clist->column[i + 1].button) {
			gdk_window_hide (clist->column[i].window);
			continue;
		}

		gtk_widget_size_allocate (clist->column[last_button].button,
					  &button_allocation); 
		button_allocation.x += button_allocation.width;
		button_allocation.width = 0;

		last_button = i + 1;
	}

	button_allocation.width += (clist->column[last_column].area.width +
				    2 * (CELL_SPACING + COLUMN_INSET));
	gtk_widget_size_allocate (clist->column[last_button].button,
				  &button_allocation);

}

static void
nautilus_list_draw_focus (GtkWidget *widget)
{
	GdkGCValues saved_values;
	NautilusCList *clist;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));

	if (!GTK_WIDGET_DRAWABLE (widget) || !GTK_WIDGET_CAN_FOCUS (widget)) {
  		return;
  	}

	clist = NAUTILUS_CLIST (widget);
	if (clist->focus_row < 0) {
		return;
	}

  	gdk_gc_get_values (clist->xor_gc, &saved_values);

  	gdk_gc_set_stipple (clist->xor_gc, nautilus_stipple_bitmap ());
  	gdk_gc_set_fill (clist->xor_gc, GDK_STIPPLED);

    	gdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
		0, ROW_TOP_YPIXEL(clist, clist->focus_row),
		clist->clist_window_width - 1,
		clist->row_height - 1);
	/* Resetting the stipple to the saved value causes death
	 * deep in Bonobo X handling, believe it or not. Fortunately
	 * we don't need to.
	 */
  	gdk_gc_set_fill (clist->xor_gc, saved_values.fill);
}

static int
selected_column_index (NautilusList *list)
{
	int column;

	column = 2;
	gtk_signal_emit_by_name (GTK_OBJECT (list), "get_sort_column_index", &column);
	return column;
}

static void
get_column_background (NautilusList *list, GdkGC **selected, GdkGC **plain)
{
	*plain = list->details->cell_lighter_background;
	*selected = list->details->cell_selected_lighter_background;
}

static void
get_cell_style (NautilusList *list, NautilusCListRow *row,
		int state, int row_index, int column_index, GtkStyle **style,
		GdkGC **fg_gc, GdkGC **bg_gc, guint32 *bg_rgb)
{
	if (style) {
		*style = GTK_WIDGET (list)->style;
	}

	if (state == GTK_STATE_SELECTED) {
		if (fg_gc != NULL) {
			*fg_gc = GTK_WIDGET (list)->style->fg_gc[state];
		}
		if (bg_gc != NULL) {
			if (column_index == selected_column_index (list)) {
				*bg_gc = list->details->selection_medium_color;
			} else  {
				*bg_gc = list->details->selection_light_color;
			}
		}
		if (bg_rgb != NULL) {
			if (column_index == selected_column_index (list)) {
				*bg_rgb = list->details->selection_medium_color_rgb;
			} else  {
				*bg_rgb = list->details->selection_light_color_rgb;
			}
		}


		return;
	}

	if (fg_gc != NULL) {
		*fg_gc = GTK_WIDGET (list)->style->fg_gc[state];
	}

	if (bg_gc != NULL) {
		if (column_index == selected_column_index (list)) {
			if ((row_index % 2) != 0) {
				*bg_gc = list->details->cell_selected_lighter_background;
			} else {
				*bg_gc = list->details->cell_selected_darker_background;
			}
		} else {
			if ((row_index % 2) != 0) {
				*bg_gc = list->details->cell_lighter_background;
			} else {
				*bg_gc = list->details->cell_darker_background;
			}
		}
	}
	if (bg_rgb != NULL) {
		if (column_index == selected_column_index (list)) {
			if ((row_index % 2) != 0) {
				*bg_rgb = list->details->cell_selected_lighter_background_rgb;
			} else {
				*bg_rgb = list->details->cell_selected_darker_background_rgb;
			}
		} else {
			if ((row_index % 2) != 0) {
				*bg_rgb = list->details->cell_lighter_background_rgb;
			} else {
				*bg_rgb = list->details->cell_darker_background_rgb;
			}
		}
	}
}

static void
gdk_window_size_as_rectangle (GdkWindow *gdk_window, GdkRectangle *rectangle)
{
	int width, height;

	gdk_window_get_size (gdk_window, &width, &height);	
	rectangle->width = width;
	rectangle->height = height;
}

static int
draw_cell_pixmap (GdkWindow *window, GdkRectangle *clip_rectangle, GdkGC *fg_gc,
		  GdkPixmap *pixmap, GdkBitmap *mask,
		  int x, int y)
{
	GdkRectangle image_rectangle;
	GdkRectangle intersect_rectangle;
	
	gdk_window_size_as_rectangle (pixmap, &image_rectangle);
	image_rectangle.x = x;
	image_rectangle.y = y;

	if (!gdk_rectangle_intersect (clip_rectangle, &image_rectangle, &intersect_rectangle)) {
		return x;
	}
	
	if (mask) {
		gdk_gc_set_clip_mask (fg_gc, mask);
		gdk_gc_set_clip_origin (fg_gc, x, y);
	}

	gdk_draw_pixmap (window, fg_gc, pixmap, 
			 intersect_rectangle.x - x, intersect_rectangle.y - y, 
			 image_rectangle.x, image_rectangle.y, 
			 intersect_rectangle.width, intersect_rectangle.height);

	if (mask) {
		gdk_gc_set_clip_origin (fg_gc, 0, 0);
		gdk_gc_set_clip_mask (fg_gc, NULL);
	}

	return x + intersect_rectangle.width;
}

static int
draw_cell_pixbuf (GdkWindow *window, GdkRectangle *clip_rectangle,
		  GdkGC *fg_gc, guint32 bg_rgb, GdkPixbuf *pixbuf, int x, int y)
{
	GdkRectangle image_rectangle;
	GdkRectangle intersect_rectangle;
	GdkPixbuf *composited;

	image_rectangle.width = gdk_pixbuf_get_width (pixbuf);
	image_rectangle.height = gdk_pixbuf_get_height (pixbuf);
	image_rectangle.x = x;
	image_rectangle.y = y;

	if (!gdk_rectangle_intersect (clip_rectangle, &image_rectangle, &intersect_rectangle)) {
		return x;
	}

	/* Composite a version of the pixbuf with the background color */
	composited = gdk_pixbuf_composite_color_simple (pixbuf,
							image_rectangle.width,
							image_rectangle.height,
							GDK_INTERP_BILINEAR,
							255, 64,
							bg_rgb, bg_rgb);
	if (composited == NULL) {
		return x;
	}

	gdk_pixbuf_render_to_drawable (composited, window, fg_gc,
				       intersect_rectangle.x - x,
				       intersect_rectangle.y - y, 
				       image_rectangle.x, image_rectangle.y, 
				       intersect_rectangle.width,
				       intersect_rectangle.height,
				       GDK_RGB_DITHER_MAX, 0, 0);

	gdk_pixbuf_unref (composited);

	return x + intersect_rectangle.width;
}

/**
 * get_cell_horizontal_start_position:
 * 
 * Get the leftmost x value at which the contents of this cell are painted.
 * 
 * @clist: The list in question.
 * @row: The row data structure for the target cell.
 * @column_index: The column of the target cell.
 * @content_width: The already-computed width of the cell contents.
 * 
 * Return value: x value at which the contents of this cell are painted.
 */
static int
get_cell_horizontal_start_position (NautilusCList *clist, NautilusCListRow *row, int column_index, int content_width)
{
	int initial_offset;

	initial_offset = clist->column[column_index].area.x + 
			 clist->hoffset + 
			 row->cell[column_index].horizontal;
	
	switch (clist->column[column_index].justification) {
		case GTK_JUSTIFY_LEFT:
			return initial_offset;
		case GTK_JUSTIFY_RIGHT:
			return initial_offset + clist->column[column_index].area.width - content_width;
		case GTK_JUSTIFY_CENTER:
		case GTK_JUSTIFY_FILL:
		default:
			return initial_offset + (clist->column[column_index].area.width - content_width)/2;
	}
} 

static int
last_column_index (NautilusCList *clist)
{
	int result;
	for (result = clist->columns - 1;
		result >= 0 && !clist->column[result].visible; 
		result--) {
	}

	return result;
}

static void
get_cell_rectangle (NautilusCList *clist, int row_index, int column_index, GdkRectangle *result)
{
	result->x = clist->column[column_index].area.x + clist->hoffset;
	result->y = ROW_TOP_YPIXEL (clist, row_index);
	result->width = clist->column[column_index].area.width;
	result->height = clist->row_height;
}

static void
get_cell_greater_rectangle (GdkRectangle *cell_rect, GdkRectangle *result, 
	gboolean last_column)
{
	*result = *cell_rect;
	result->x -= COLUMN_INSET + CELL_SPACING;
	result->width += 2 * COLUMN_INSET + CELL_SPACING;
	if (last_column) {
		result->width += CELL_SPACING;
	}
}

static void
draw_cell (NautilusCList *clist, GdkRectangle *area, int row_index, int column_index, 
	NautilusCListRow *row)
{
	GtkStyle *style;
	GdkGC *fg_gc;
	GdkGC *bg_gc;
	GdkGC *text_gc;
	guint32 bg_rgb;

	GList *p;

	int width;
	int height;
	int pixmap_width;
	int offset = 0;
	int baseline;
	int row_center_offset;

	GdkRectangle cell_rectangle;
	GdkRectangle erase_rectangle;
	GdkRectangle intersect_rectangle;
	
	if (!clist->column[column_index].visible) {
		return;
	}

	get_cell_style (NAUTILUS_LIST(clist), row, row->state, row_index, 
		column_index, &style, &fg_gc, &bg_gc, &bg_rgb);
	get_cell_rectangle (clist, row_index, column_index, &cell_rectangle);
	get_cell_greater_rectangle (&cell_rectangle, &erase_rectangle, 
		column_index == last_column_index (clist));

	/* do we have anything do draw? */
	if (area && !gdk_rectangle_intersect (area, &erase_rectangle, &intersect_rectangle)) {
		return;
	}

	gdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
		  erase_rectangle.x, erase_rectangle.y, 
		  erase_rectangle.width, erase_rectangle.height);

	/* calculate real width for column justification */
	width = 0;
	pixmap_width = 0;
	offset = 0;
	
	switch ((NautilusCellType)row->cell[column_index].type) {
	case NAUTILUS_CELL_TEXT:
	case NAUTILUS_CELL_LINK_TEXT:
		width = gdk_string_width (style->font,
			NAUTILUS_CELL_TEXT (row->cell[column_index])->text);
		break;
	case NAUTILUS_CELL_PIXMAP:
		gdk_window_get_size (NAUTILUS_CELL_PIXMAP (row->cell[column_index])->pixmap,
		       &pixmap_width, &height);
		width = pixmap_width;
		break;
	case NAUTILUS_CELL_PIXTEXT:
		gdk_window_get_size (NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->pixmap,
		       &pixmap_width, &height);
		width = (pixmap_width +
			NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->spacing +
			gdk_string_width (style->font, NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->text));
		break;
	case NAUTILUS_CELL_PIXBUF_LIST:
		for (p = NAUTILUS_CELL_PIXBUF_LIST (row->cell[column_index])->pixbufs; 
			p != NULL; p = p->next) {
			if (width != 0) {
				width += PIXBUF_LIST_SPACING;
			}
			width += gdk_pixbuf_get_width (p->data);
		}
		break;
	case NAUTILUS_CELL_PIXBUF:
		width = gdk_pixbuf_get_width (NAUTILUS_CELL_PIXBUF (row->cell[column_index]));
		break;
	default:
		return;
	}

	offset = get_cell_horizontal_start_position (clist, row, column_index, width);

	/* Draw Text and/or Pixmap */
	switch ((NautilusCellType)row->cell[column_index].type) {
	case NAUTILUS_CELL_PIXMAP:
		{
			NautilusList *list = NAUTILUS_LIST (clist);
			int dark_width, dark_height;
			GdkPixbuf *src_pixbuf, *dark_pixbuf;
			GdkPixmap *dark_pixmap;
			GdkBitmap *dark_mask;

			if (list->details->drag_prelight_row == row) {
				
				gdk_window_get_geometry (NAUTILUS_CELL_PIXMAP (row->cell[column_index])->pixmap,
						 	 NULL, NULL, &dark_width, &dark_height, NULL);
							
				src_pixbuf = gdk_pixbuf_get_from_drawable 
						(NULL,
			      	 		 NAUTILUS_CELL_PIXMAP (row->cell[column_index])->pixmap,
			      	 		 gdk_rgb_get_cmap (),
			      	  		 0, 0, 0, 0, dark_width, dark_height);

				if (src_pixbuf != NULL) {
					/* Create darkened pixmap */			
					dark_pixbuf = nautilus_create_darkened_pixbuf (src_pixbuf,
							      	 	       0.8 * 255,
							       		       0.8 * 255);
					if (dark_pixbuf != NULL) {
						gdk_pixbuf_render_pixmap_and_mask (dark_pixbuf,
				   				   	   	   &dark_pixmap, &dark_mask,
				   				   	   	   NAUTILUS_STANDARD_ALPHA_THRESHHOLD);
				   				   	   	   			
						draw_cell_pixmap (clist->clist_window, &cell_rectangle, fg_gc,
		    					  	  dark_pixmap, NAUTILUS_CELL_PIXMAP (row->cell[column_index])->mask, offset,
		    					  	  cell_rectangle.y + row->cell[column_index].vertical +
		    					  	 (cell_rectangle.height - height) / 2);

						gdk_pixbuf_unref (dark_pixbuf);
					}
					gdk_pixbuf_unref (src_pixbuf);
				}					
			} else {		
				draw_cell_pixmap (clist->clist_window, &cell_rectangle, fg_gc,
		    			NAUTILUS_CELL_PIXMAP (row->cell[column_index])->pixmap,
		    			NAUTILUS_CELL_PIXMAP (row->cell[column_index])->mask,
		    			offset,
		    			cell_rectangle.y + row->cell[column_index].vertical +
		    			(cell_rectangle.height - height) / 2);
			}
		}
		break;

	case NAUTILUS_CELL_PIXTEXT:
		offset = draw_cell_pixmap (clist->clist_window, &cell_rectangle, fg_gc,
		      NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->pixmap,
		      NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->mask,
		      offset,
		      cell_rectangle.y + row->cell[column_index].vertical+
		      (cell_rectangle.height - height) / 2);
		offset += NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->spacing;
		/* fall through */
	case NAUTILUS_CELL_TEXT:
	case NAUTILUS_CELL_LINK_TEXT:
		if (style != GTK_WIDGET (clist)->style) {
			row_center_offset = (((clist->row_height - style->font->ascent -
				style->font->descent - 1) / 2) + 1.5 +
				style->font->ascent);
		} else {
			row_center_offset = clist->row_center_offset;
		}
		baseline = cell_rectangle.y + row_center_offset + row->cell[column_index].vertical;

		if (row->state != GTK_STATE_NORMAL) {
			text_gc = NAUTILUS_LIST (clist)->details->selected_text_color;
		} else if ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_LINK_TEXT
			   && NAUTILUS_LIST (clist)->details->single_click_mode) {
			/* For link text cells, draw with blue link-like color and use underline. */
			text_gc = NAUTILUS_LIST (clist)->details->link_text_color;
		} else {
			text_gc = NAUTILUS_LIST (clist)->details->text_color;
		}

		gdk_gc_set_clip_rectangle (text_gc, &cell_rectangle);

		gdk_draw_string (clist->clist_window, style->font, text_gc,
				 offset,
				 baseline,
				 ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_PIXTEXT) ?
				 NAUTILUS_CELL_PIXTEXT (row->cell[column_index])->text :
				 NAUTILUS_CELL_TEXT (row->cell[column_index])->text);

		if ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_LINK_TEXT
			&& NAUTILUS_LIST (clist)->details->single_click_mode) {
			gdk_draw_line (clist->clist_window, text_gc,
				       offset, baseline + 1,
				       offset + width, baseline + 1);
		}

		gdk_gc_set_clip_rectangle (text_gc, NULL);
		break;
			
	case NAUTILUS_CELL_PIXBUF_LIST: {
		guint pixbuf_width;
		guint ellipsis_width;

		ellipsis_width = gdk_string_width (style->font, "...");
	  
		for (p = NAUTILUS_CELL_PIXBUF_LIST (row->cell[column_index])->pixbufs; p != NULL; p = p->next) {
			pixbuf_width = gdk_pixbuf_get_width (p->data);

			if ((p->next != NULL && (int) (pixbuf_width + ellipsis_width) >= 
		  		cell_rectangle.x + cell_rectangle.width - offset) 
		  			|| ((int) pixbuf_width >= cell_rectangle.x + cell_rectangle.width - offset)) {
				/* Not enough room for this icon & ellipsis, just draw ellipsis. */
			
				gdk_draw_string (clist->clist_window, style->font, fg_gc,
						 offset,
						 cell_rectangle.y + cell_rectangle.height/2,
						 "...");

				break;
			}

			height = gdk_pixbuf_get_height (p->data);

	  		offset = draw_cell_pixbuf (clist->clist_window,
		  				   &cell_rectangle, fg_gc, bg_rgb,
		  				   p->data,
				  		   offset,
				 		   cell_rectangle.y + row->cell[column_index].vertical +
				 		   (cell_rectangle.height - height) / 2);

			offset += PIXBUF_LIST_SPACING;
		}
		break;
	}
	case NAUTILUS_CELL_PIXBUF: {
		GdkPixbuf *pixbuf;
		guint height;
		pixbuf = NAUTILUS_CELL_PIXBUF (row->cell[column_index]);
		height = gdk_pixbuf_get_height (pixbuf);
		offset = draw_cell_pixbuf (clist->clist_window, &cell_rectangle,
					   fg_gc, bg_rgb, pixbuf,
					   offset, cell_rectangle.y
					   + row->cell[column_index].vertical
					   + (cell_rectangle.height - height) / 2);
	}
	default:
		break;
	}
}

static void
draw_row (NautilusCList *clist, GdkRectangle *area, int row_index, NautilusCListRow *row)
{
	GtkWidget *widget;
	GdkRectangle row_rectangle;
	GdkRectangle extended_row_rectangle;
	GdkRectangle intersect_rectangle;
	int colum_index;

	g_return_if_fail (clist != NULL);

	/* bail now if we arn't drawable yet */
	if (!GTK_WIDGET_DRAWABLE (clist) || row_index < 0 || row_index >= clist->rows) {
		return;
	}

	widget = GTK_WIDGET (clist);

	/* if the function is passed the pointer to the row instead of null,
	 * it avoids this expensive lookup 
	 */
	if (!row) {
		row = ROW_ELEMENT (clist, row_index)->data;
	}

	/* rectangle of the entire row */
	row_rectangle.x = 0;
	row_rectangle.y = ROW_TOP_YPIXEL (clist, row_index);
	row_rectangle.width = clist->clist_window_width;
	row_rectangle.height = clist->row_height;

	/* rectangle of the entire row including spacing above and below the row */
	extended_row_rectangle.x = 0;
	extended_row_rectangle.y = row_rectangle.y - CELL_SPACING;
	extended_row_rectangle.width = row_rectangle.width;
	extended_row_rectangle.height = row_rectangle.height + CELL_SPACING;

	if (row->state == GTK_STATE_NORMAL) {
		if (row->fg_set) {
			gdk_gc_set_foreground (clist->fg_gc, &row->foreground);
		}
		if (row->bg_set) {
			gdk_gc_set_foreground (clist->bg_gc, &row->background);
		}
	}

	intersect_rectangle = extended_row_rectangle;
	/* check if we have something to draw */
	if (area && !gdk_rectangle_intersect (area, &extended_row_rectangle, &intersect_rectangle)) {
		return;
	}


	/* iterate and draw all the columns (row cells) and draw their contents */
	for (colum_index = 0; colum_index < clist->columns; colum_index++) {
		draw_cell (clist, area, row_index, colum_index, row);
	}

	/* draw the row spacer */
	gdk_draw_rectangle (clist->clist_window,
			    NAUTILUS_LIST (clist)->details->cell_divider_color,
			    TRUE,
			    intersect_rectangle.x,
			    extended_row_rectangle.y,
			    intersect_rectangle.width,
			    CELL_SPACING);
	gdk_draw_rectangle (clist->clist_window,
			    NAUTILUS_LIST (clist)->details->cell_divider_color,
			    TRUE,
			    intersect_rectangle.x,
			    row_rectangle.y + row_rectangle.height,
			    intersect_rectangle.width,
			    CELL_SPACING);

	/* draw focus rectangle */
	if (clist->focus_row == row_index 
		&& GTK_WIDGET_CAN_FOCUS (widget) && GTK_WIDGET_HAS_FOCUS (widget)) {
		if (!area) {
			gdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
					    row_rectangle.x, row_rectangle.y,
					    row_rectangle.width - 1, row_rectangle.height - 1);
		} else if (gdk_rectangle_intersect (area, &row_rectangle, &intersect_rectangle)) {
			gdk_gc_set_clip_rectangle (clist->xor_gc, &intersect_rectangle);
			gdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
					      row_rectangle.x, row_rectangle.y,
					      row_rectangle.width - 1,
					      row_rectangle.height - 1);
			gdk_gc_set_clip_rectangle (clist->xor_gc, NULL);
		}
	}
}

static void
rectangle_intersect (const GdkRectangle *source1, const GdkRectangle *source2, GdkRectangle *result)
{
	/* convenience call that returns result with defined values even if sources are disjucnt */
	if (!gdk_rectangle_intersect ((GdkRectangle *)source1, (GdkRectangle *)source2, result)) {
		result->width = 0;
		result->height = 0;
	}
}

static void
nautilus_list_clear_from_row (NautilusList *list, int row_index,
	GdkRectangle *area)
{
	NautilusCList *clist;
	GdkRectangle clip_area, tmp;
	GdkRectangle first_column_plain_rectangle, selected_column_rectangle, 
		second_column_plain_rectangle;
	GdkGC *selected_column_gc;
	GdkGC *plain_column_gc;

	g_assert (NAUTILUS_IS_LIST (list));
	g_assert (area);

	clist = NAUTILUS_CLIST (list);

	/* calculate the area we need to erase */
	clip_area = *area;
	clip_area.y = ROW_TOP_YPIXEL (clist, row_index);
	if (clip_area.y < 0) {
		clip_area.y = 0;
	}
	if (clip_area.y - area->y < area->height) {
		clip_area.height = area->height - (clip_area.y - area->y); 
	} else {
		clip_area.height = 0;
	}
	
	if (clip_area.height <= 0) {
		/* nothing visible to erase */
		return;
	}

	/* calculate the rectangle for the selected column */
	get_cell_rectangle (clist, 0, selected_column_index (list), &tmp);
	get_cell_greater_rectangle (&tmp, &tmp, 
		selected_column_index (list) == last_column_index (clist));
	tmp.y = clip_area.y;
	tmp.height = clip_area.height;

	rectangle_intersect (&clip_area, &tmp, &selected_column_rectangle);

	/* calculate the first rectangle */
	tmp = clip_area;
	if (selected_column_rectangle.x > tmp.x) {
		tmp.width = selected_column_rectangle.x - tmp.x;
	} else {
		tmp.width = selected_column_rectangle.x - tmp.x;
	}
	rectangle_intersect (&clip_area, &tmp, &first_column_plain_rectangle);


	/* calculate the last rectangle */
	tmp = clip_area;
	tmp.x = selected_column_rectangle.x + selected_column_rectangle.width;
	rectangle_intersect (&clip_area, &tmp, &second_column_plain_rectangle);

	/* get the colors for drawing */
	get_column_background (list, &selected_column_gc, &plain_column_gc);

	/* draw the first column if non-empty */
	if (first_column_plain_rectangle.width > 0) {
		gdk_draw_rectangle (clist->clist_window, plain_column_gc, TRUE,
			  first_column_plain_rectangle.x, first_column_plain_rectangle.y, 
			  first_column_plain_rectangle.width, first_column_plain_rectangle.height);
	}
	/* draw the selected column if non-empty */
	if (selected_column_rectangle.width > 0) {
		gdk_draw_rectangle (clist->clist_window, selected_column_gc, TRUE,
			  selected_column_rectangle.x, selected_column_rectangle.y, 
			  selected_column_rectangle.width, selected_column_rectangle.height);
	}
	/* draw the last column if non-empty */
	if (second_column_plain_rectangle.width > 0) {
		gdk_draw_rectangle (clist->clist_window, plain_column_gc, TRUE,
			  second_column_plain_rectangle.x, second_column_plain_rectangle.y, 
			  second_column_plain_rectangle.width, second_column_plain_rectangle.height);
	}
}

static void
draw_rows (NautilusCList *clist, GdkRectangle *area)
{
	GList *list;
	int row_index;
	int first_row;
	int last_row;

	g_assert (area != NULL);
	
	if (clist->row_height == 0 || !GTK_WIDGET_DRAWABLE (clist)) {
		return;
	}

	first_row = ROW_FROM_YPIXEL (clist, area->y);
	last_row = ROW_FROM_YPIXEL (clist, area->y + area->height);

	/* this is a small special case which exposes the bottom cell line
	 * on the last row -- it might go away if I change the wall the cell
	 * spacings are drawn
	 */
	if (clist->rows == first_row) {
		first_row--;
	}

	list = ROW_ELEMENT (clist, first_row);
	for (row_index = first_row; row_index <= last_row ; row_index++) {
		if (list == NULL) {
			break;
		}

		NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, 
			draw_row, (clist, area, row_index, list->data));
		list = list->next;
	}

	nautilus_list_clear_from_row (NAUTILUS_LIST (clist), 
		row_index, area);
}

static void
draw_all (NautilusCList *clist)
{
	GdkRectangle area;
	area.x = 0;
	area.y = 0;
	area.width = clist->clist_window_width;
	area.height = clist->clist_window_height;
	NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, draw_rows, (clist, &area));
}

static void
nautilus_list_draw (GtkWidget *widget, GdkRectangle *area)
{
	NautilusCList *clist;
	NautilusList *list;
	
	g_assert (NAUTILUS_IS_LIST (widget));
	g_assert (area != NULL);

	clist = NAUTILUS_CLIST (widget);
	list = NAUTILUS_LIST (widget);

	nautilus_list_setup_style_colors (NAUTILUS_LIST (widget));

	if (GTK_WIDGET_DRAWABLE (widget)) {
		int border_width;
		border_width = GTK_CONTAINER (widget)->border_width;
		gdk_window_clear_area (widget->window,
				area->x - border_width, 
				area->y - border_width,
				area->width, area->height);

		gtk_draw_shadow (widget->style, widget->window,
				GTK_STATE_NORMAL, clist->shadow_type,
				0, 0, 
				clist->clist_window_width +
					(2 * widget->style->klass->xthickness),
				clist->clist_window_height +
					(2 * widget->style->klass->ythickness) +
				clist->column_title_area.height);

		NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, draw_rows, (clist, area));

		/* Draw the title if it exists */
		if (list->details->title) {
			GdkRectangle draw_area;

			if (gtk_widget_intersect (list->details->title,
						  area, &draw_area)) {
				gtk_widget_draw (list->details->title, &draw_area);
			}
		}
	}
}

static int
nautilus_list_expose (GtkWidget *widget, GdkEventExpose *event)
{
	NautilusCList *clist;
	
	g_assert (NAUTILUS_IS_LIST (widget));

	clist = NAUTILUS_CLIST (widget);

	nautilus_list_setup_style_colors (NAUTILUS_LIST (widget));

	if (GTK_WIDGET_DRAWABLE (widget)) {

		gtk_draw_shadow (widget->style, widget->window,
				GTK_STATE_NORMAL, clist->shadow_type,
				0, 0, 
				clist->clist_window_width +
					(2 * widget->style->klass->xthickness),
				clist->clist_window_height +
					(2 * widget->style->klass->ythickness) +
				clist->column_title_area.height);

		NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, draw_rows, 
			(clist, &event->area));
	}

	return FALSE;
}

static void 
nautilus_list_resize_column (NautilusCList *clist, int column_index, int width)
{
	/* override resize column to invalidate the title */
	NautilusList *list;

	list = NAUTILUS_LIST (clist);

	gtk_widget_queue_draw (list->details->title);
		
	NAUTILUS_CALL_PARENT_CLASS (NAUTILUS_CLIST_CLASS, resize_column, (clist, column_index, width));
}


/* redraw the list if it's not frozen */
#define CLIST_UNFROZEN(clist)     (((NautilusCList*) (clist))->freeze_count == 0)

/**
 * nautilus_list_mark_cell_as_link:
 * 
 * Mark a text cell as a link cell. Link cells are drawn differently,
 * and activate rather than select on single-click. The cell must
 * be a text cell (not a pixmap cell or one of the other types).
 * 
 * @list: The NautilusList in question.
 * @column_index: The column of the desired cell.
 * @row: The row of the desired cell.
 */
void
nautilus_list_mark_cell_as_link (NautilusList *list,
				 int row_index,
				 int column_index)
{
	NautilusCListRow *row;
	NautilusCList *clist;

	g_return_if_fail (NAUTILUS_IS_LIST (list));

	clist = NAUTILUS_CLIST (list);

	g_return_if_fail (row_index >= 0 && row_index < clist->rows);
	g_return_if_fail (column_index >= 0 && column_index < clist->columns);
	
	row = ROW_ELEMENT (clist, row_index)->data;

	/* 
	 * We only support changing text cells to links. Maybe someday
	 * we'll support pixmap or pixtext link cells too. 
	 */
	g_return_if_fail ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_TEXT);

	row->cell[column_index].type = NAUTILUS_CELL_LINK_TEXT;
}				


static void
nautilus_list_set_cell_contents (NautilusCList    *clist,
		   		 NautilusCListRow *row,
		   		 int         column_index,
		   		 NautilusCellType  type,
		   		 const gchar *text,
		   		 guint8       spacing,
		   		 GdkPixmap   *pixmap,
		   		 GdkBitmap   *mask)
{
	/* 
	 * Note that we don't do the auto_resize bracketing here that's done
	 * in the parent class. It would require copying over huge additional
	 * chunks of code. We might decide we need that someday, but the
	 * chances seem larger that we'll switch away from CList first.
	 */

	/* Clean up old data, which parent class doesn't know about. */
	if ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_PIXBUF_LIST) {
		nautilus_gdk_pixbuf_list_free (NAUTILUS_CELL_PIXBUF_LIST (row->cell[column_index])->pixbufs);
	} else if ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_PIXBUF) {
		gdk_pixbuf_unref (NAUTILUS_CELL_PIXBUF (row->cell[column_index]));
	}

	/* If old cell was a link-text cell, convert it back to a normal text
	 * cell so it gets cleaned up properly by GtkCList code.
	 */
	if ((NautilusCellType)row->cell[column_index].type == NAUTILUS_CELL_LINK_TEXT) {
		row->cell[column_index].type = NAUTILUS_CELL_TEXT;
	}

	NAUTILUS_CALL_PARENT_CLASS (NAUTILUS_CLIST_CLASS, set_cell_contents, (clist, row, column_index, type, text, spacing, pixmap, mask));

	if ((NautilusCellType)type == NAUTILUS_CELL_PIXBUF_LIST) {
		row->cell[column_index].type = NAUTILUS_CELL_PIXBUF_LIST;
		/* Hideously, we concealed our list of pixbufs in the pixmap parameter. */
	  	NAUTILUS_CELL_PIXBUF_LIST (row->cell[column_index])->pixbufs = (GList *)pixmap;
	} else if ((NautilusCellType)type == NAUTILUS_CELL_PIXBUF) {
		row->cell[column_index].type = NAUTILUS_CELL_PIXBUF;
		/* Hideously, we concealed our pixbuf in the pixmap parameter. */
	  	NAUTILUS_CELL_PIXBUF (row->cell[column_index]) = pixmap;
	}
}

static void
set_list_cell (NautilusList *list,
	       int row_index, int column_index,
	       NautilusCellType type,
	       gpointer data)
{
	NautilusCList    *clist;
	NautilusCListRow *row;

	g_return_if_fail (NAUTILUS_IS_LIST (list));

	clist = NAUTILUS_CLIST (list);

	if (row_index < 0 || row_index >= clist->rows) {
		return;
	}
	
	if (column_index < 0 || column_index >= clist->columns) {
		return;
	}

	row = ROW_ELEMENT (clist, row_index)->data;

	/*
	 * We have to go through the set_cell_contents bottleneck, which only
	 * allows expected parameter types. Since our pixbuf is not an
	 * expected parameter type, we have to sneak it in by casting it into
	 * one of the expected parameters.
	 */
	NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, set_cell_contents, 
		(clist, row, column_index, type, NULL, 0, (GdkPixmap *) data, NULL));

	/* redraw the list if it's not frozen */
	if (CLIST_UNFROZEN (clist) 
		&& nautilus_clist_row_is_visible (clist, row_index) != GTK_VISIBILITY_NONE) {
		NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, draw_row, 
			(clist, NULL, row_index, row));
	}
}

static gpointer
get_list_cell (NautilusList *list,
	       int row_index, int column_index,
	       NautilusCellType type)
{
	NautilusCList    *clist;
	NautilusCListRow *row;

	g_return_val_if_fail (NAUTILUS_IS_LIST (list), NULL);

	clist = NAUTILUS_CLIST (list);

	if (row_index < 0 || row_index >= clist->rows) {
		return NULL;
	}
	
	if (column_index < 0 || column_index >= clist->columns) {
		return NULL;
	}

	row = ROW_ELEMENT (clist, row_index)->data;

	if (row->cell[column_index].type == type) {
		return NAUTILUS_CELL_PIXMAP (row->cell[column_index])->pixmap;
	}

	return NULL;
}

/**
 * nautilus_list_set_pixbuf_list:
 * 
 * Set the contents of a cell to a list of similarly-sized GdkPixbufs.
 * 
 * @list: The NautilusList in question.
 * @row: The row of the target cell.
 * @column_index: The column of the target cell.
 * @pixbufs: A GList of GdkPixbufs.
 */
void 	   
nautilus_list_set_pixbuf_list (NautilusList *list,
			       int row_index,
			       int column_index,
			       GList *pixbufs)
{
	set_list_cell (list, row_index, column_index,
		       NAUTILUS_CELL_PIXBUF_LIST, pixbufs);
}

/**
 * nautilus_list_set_pixbuf:
 * 
 * Similar to nautilus_list_set_pixbuf_list, but with a single pixbuf.
 * 
 * @list: The NautilusList in question.
 * @row: The row of the target cell.
 * @column_index: The column of the target cell.
 * @pixbuf: A GdkPixbuf.
 */
void 	   
nautilus_list_set_pixbuf (NautilusList *list,
			  int row_index,
			  int column_index,
			  GdkPixbuf *pixbuf)
{
	set_list_cell (list, row_index, column_index,
		       NAUTILUS_CELL_PIXBUF, pixbuf);
}

/**
 * nautilus_list_get_pixbuf:
 * 
 * Return the pixbuf stored in the specified position, or a null pointer
 * if the cell isn't a pixbuf.
 * 
 * @list: The NautilusList in question.
 * @row: The row of the target cell.
 * @column_index: The column of the target cell.
 */
GdkPixbuf *
nautilus_list_get_pixbuf (NautilusList *list,
			  int row_index,
			  int column_index)
{
	return get_list_cell (list, row_index, column_index,
			      NAUTILUS_CELL_PIXBUF);
}

static void
nautilus_list_track_new_column_width (NautilusCList *clist, int column_index, int new_width)
{
	NautilusList *list;

	list = NAUTILUS_LIST (clist);

	/* pin new_width to min and max values */
	if (new_width < MAX (COLUMN_MIN_WIDTH, clist->column[column_index].min_width))
		new_width = MAX (COLUMN_MIN_WIDTH, clist->column[column_index].min_width);
	if (clist->column[column_index].max_width >= 0 &&
	    new_width > clist->column[column_index].max_width)
		new_width = clist->column[column_index].max_width;

	/* check to see if the pinned value is still different */
	if (clist->column[column_index].width == new_width)
		return;

	/* set the new width */
	clist->column[column_index].width = new_width;
	clist->column[column_index].width_set = TRUE;

	size_allocate_columns (clist, TRUE);
	size_allocate_title_buttons (clist);

	/* redraw the invalid columns */
	if (clist->freeze_count == 0) {
	
  		GdkRectangle area;

		area = clist->column_title_area;
		area.x = clist->column[column_index].area.x;
		area.height += clist->clist_window_height;

		NAUTILUS_CALL_VIRTUAL (NAUTILUS_CLIST_CLASS, clist, draw_rows, (clist, &area));
	}
}

static void
nautilus_list_drag_start (GtkWidget *widget, GdkEventMotion *event)
{
	NautilusList *list;
	GdkDragContext *context;
	GdkPixmap *pixmap_for_dragged_file;
	GdkBitmap *mask_for_dragged_file;
	int x_offset, y_offset; 

	g_return_if_fail (NAUTILUS_IS_LIST (widget));
	list = NAUTILUS_LIST (widget);

	list->details->drag_started = TRUE;
	list->details->dnd_select_pending = FALSE;
	/* reinit from last dnd if there was one */
	list->details->drag_info->got_drop_data_type = FALSE;
	nautilus_drag_destroy_selection_list (list->details->drag_info->selection_list);
	list->details->drag_info->selection_list = NULL;
	
	context = gtk_drag_begin (widget, list->details->drag_info->target_list,
				  list->details->dnd_press_button == CONTEXTUAL_MENU_BUTTON
				  ? GDK_ACTION_ASK
				  : GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_ASK,
				  list->details->dnd_press_button,
				  (GdkEvent *) event);

	x_offset = 10;
	y_offset = 10;

	gtk_signal_emit (GTK_OBJECT (list), list_signals[GET_DRAG_PIXMAP], 
			 list->details->button_down_row, &pixmap_for_dragged_file, 
			 &mask_for_dragged_file);

	if (pixmap_for_dragged_file) {
	        /* set the pixmap and mask for dragging */
	        gtk_drag_set_icon_pixmap (context,
					  gtk_widget_get_colormap (widget),
					  pixmap_for_dragged_file,
					  mask_for_dragged_file,
					  x_offset, y_offset);
	}
}

/* Our handler for motion_notify events.  We override all of GtkCList's broken
 * behavior.
 */
static int
nautilus_list_motion (GtkWidget *widget, GdkEventMotion *event)
{
	NautilusList *list;
	NautilusCList *clist;

	g_return_val_if_fail (NAUTILUS_IS_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	list = NAUTILUS_LIST (widget);
	clist = NAUTILUS_CLIST (widget);

	if (event->window != clist->clist_window) {
		return NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, motion_notify_event, (widget, event));
	}
	
	if (!((list->details->dnd_press_button == ACTION_BUTTON && (event->state & GDK_BUTTON1_MASK))
	    || (list->details->dnd_press_button == CONTEXTUAL_MENU_BUTTON && (event->state & GDK_BUTTON3_MASK))))
		return FALSE;

	/* This is the same threshold value that is used in gtkdnd.c */

	if (MAX (abs (list->details->dnd_press_x - event->x),
		 abs (list->details->dnd_press_y - event->y)) <= 3) {
		return FALSE;
	}


	if (list->details->button_down_row < 0) {
		/* We didn't hit a row, just blank space */
		return FALSE;
	}

	g_assert (list->details->button_down_row < clist->rows);
	if (!list->details->drag_started) {
		if (list->details->dnd_press_button == CONTEXTUAL_MENU_BUTTON) {
			gtk_timeout_remove (list->details->context_menu_timeout_id);
		}
		nautilus_list_drag_start (widget, event);
	}
	
	return TRUE;
}

void 
nautilus_list_column_resize_track_start (GtkWidget *widget, int column_index)
{
	NautilusCList *clist;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));

	clist = NAUTILUS_CLIST (widget);
	clist->drag_pos = column_index;
}

void 
nautilus_list_column_resize_track (GtkWidget *widget, int column_index)
{
	NautilusCList *clist;
	int x;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));

	clist = NAUTILUS_CLIST (widget);

	gtk_widget_get_pointer (widget, &x, NULL);
	nautilus_list_track_new_column_width (clist, column_index, 
		new_column_width (clist, column_index, &x));
}

void 
nautilus_list_column_resize_track_end (GtkWidget *widget, int column_index)
{
	NautilusCList *clist;

	g_return_if_fail (NAUTILUS_IS_LIST (widget));

	clist = NAUTILUS_CLIST (widget);
	clist->drag_pos = -1;
}


static void
nautilus_list_ensure_drag_data (NautilusList *list,
				GdkDragContext *context,
				guint32 time)
{
	if (!list->details->drag_info->got_drop_data_type) {
		gtk_drag_get_data (GTK_WIDGET (list), context,
				   GPOINTER_TO_INT (context->targets->data),
				   time);
	}
}

static void
nautilus_list_drag_end (GtkWidget *widget, GdkDragContext *context)
{
	NautilusList *list;
	NautilusDragInfo *drag_info;

	list = NAUTILUS_LIST (widget);
	drag_info = list->details->drag_info;

	drag_info->got_drop_data_type = FALSE;
	nautilus_drag_destroy_selection_list (list->details->drag_info->selection_list);
	list->details->drag_info->selection_list = NULL;

}

static void
nautilus_list_drag_leave (GtkWidget *widget, GdkDragContext *context, guint time)
{
	NautilusList *list;
	NautilusDragInfo *drag_info;

	list = NAUTILUS_LIST (widget);
	drag_info = list->details->drag_info;

	nautilus_list_stop_auto_scroll (NAUTILUS_LIST (list));

	nautilus_list_set_drag_prelight_row (list, -1);
}

gboolean
nautilus_list_rejects_dropped_icons (NautilusList *list)
{
	return list->details->rejects_dropped_icons;
}

void
nautilus_list_set_rejects_dropped_icons (NautilusList *list, gboolean new_value)
{
	list->details->rejects_dropped_icons = new_value;
}

static void
nautilus_list_get_drop_action (NautilusList *list, 
			       GdkDragContext *context,
			       int x, int y,
			       int *default_action,
			       int *non_default_action)
{
	NautilusDragInfo *drag_info;

	drag_info = NAUTILUS_LIST (list)->details->drag_info;

	/* FIXME bugzilla.eazel.com 2569: Too much code copied from nautilus-icon-dnd.c.
	 * Need to share more.
	 */

	if (!drag_info->got_drop_data_type) {
		/* drag_data_received didn't get called yet */
		return;
	}

	/* get those actions from a subclass of this object */
	gtk_signal_emit (GTK_OBJECT (list), 
			 list_signals[GET_DEFAULT_ACTION],
			 default_action, 
			 non_default_action,  
			 context,
			 drag_info->selection_list,
			 x, y, 
			 drag_info->data_type);

}			       


static void
nautilus_list_real_scroll (NautilusList *list, float delta_x, float delta_y)
{
	GtkAdjustment *hadj, *vadj;

	hadj = nautilus_clist_get_hadjustment (NAUTILUS_CLIST (list));
	vadj = nautilus_clist_get_vadjustment (NAUTILUS_CLIST (list));

	nautilus_gtk_adjustment_set_value (hadj, hadj->value + (int)delta_x);
	nautilus_gtk_adjustment_set_value (vadj, vadj->value + (int)delta_y);

}

static int
auto_scroll_timeout_callback (gpointer data)
{
	NautilusList *list;
	NautilusDragInfo *drag_info;
	GtkWidget *widget;
	float x_scroll_delta, y_scroll_delta;

	g_assert (NAUTILUS_IS_LIST (data));
	widget = GTK_WIDGET (data);
	list = NAUTILUS_LIST (widget);
	drag_info = list->details->drag_info;

	if (drag_info->waiting_to_autoscroll
	    && drag_info->start_auto_scroll_in > nautilus_get_system_time()) {
		/* not yet */
		return TRUE;
	}

	drag_info->waiting_to_autoscroll = FALSE;

	nautilus_drag_autoscroll_calculate_delta (widget, &x_scroll_delta, &y_scroll_delta);

	nautilus_list_real_scroll (list, x_scroll_delta, y_scroll_delta);

	return TRUE;
}

static void
nautilus_list_start_auto_scroll (NautilusList *list)
{
	g_assert (NAUTILUS_IS_LIST (list));

	nautilus_drag_autoscroll_start (list->details->drag_info,
					GTK_WIDGET (list),
					auto_scroll_timeout_callback,
					list);
}

static void
nautilus_list_stop_auto_scroll (NautilusList *list)
{
	g_assert (NAUTILUS_IS_LIST (list));

	nautilus_drag_autoscroll_stop (list->details->drag_info);
}

static void
nautilus_list_prelight_if_necessary (NautilusList *list, GdkDragContext *context,
				     int x, int y, guint time)
{
	gboolean is_prelight_necessary;
	
	/* should we prelight the current row ? */
	gtk_signal_emit (GTK_OBJECT (list), 
			 list_signals[HANDLE_DRAGGED_ITEMS],
			 context->action, 
			 list->details->drag_info->selection_list,
			 x, y, 
			 list->details->drag_info->data_type, 
		 	 &is_prelight_necessary);

	if (is_prelight_necessary) {
		nautilus_list_set_drag_prelight_row (list, y);
	} else {
		nautilus_list_set_drag_prelight_row (list, -1);
	}
}


static gboolean
nautilus_list_drag_motion (GtkWidget *widget, GdkDragContext *context,
			   int x, int y, guint time)
{
	NautilusList *list;
	int default_action, non_default_action, resulting_action;

	list = NAUTILUS_LIST (widget);

	nautilus_list_ensure_drag_data (list, context,  time);

	nautilus_list_start_auto_scroll (NAUTILUS_LIST (widget));

	default_action = 0; 
	non_default_action = 0;

	nautilus_list_get_drop_action (list, context, x, y, &default_action, &non_default_action);
	resulting_action = nautilus_drag_modifier_based_action (default_action, non_default_action);

	gdk_drag_status (context, resulting_action, time);

	nautilus_list_prelight_if_necessary (list, context, x, y, time);

	return TRUE;
}

static gboolean
nautilus_list_drag_drop (GtkWidget *widget, GdkDragContext *context,
			 int x, int y, guint time)
{
	NautilusList *list;
	
	list = NAUTILUS_LIST (widget);

	/* make sure that drag_data_received is going to be called
	   after this event and will do the actual actions */
	list->details->drag_info->drop_occured = TRUE;
	gtk_drag_get_data (GTK_WIDGET (widget), context,
			   GPOINTER_TO_INT (context->targets->data),
			   time);

	return FALSE;
}

static void
nautilus_list_receive_dropped_icons (NautilusList *list,
				     int action,
				     GtkSelectionData *data,
				     int x, int y, guint info)
{
	NautilusDragInfo *drag_info;
	GList *	selected_items;

	g_assert (NAUTILUS_IS_LIST (list));
	drag_info = list->details->drag_info;

	/* Put selection list in local variable and NULL the global one
	 * so it doesn't get munged in a modal popup-menu event loop
	 * in the handle_dropped_item handler.
	 */
	selected_items = drag_info->selection_list;
	drag_info->selection_list = NULL;
	gtk_signal_emit (GTK_OBJECT (list), list_signals[HANDLE_DROPPED_ITEMS],
			 action, selected_items, x, y, info);
	nautilus_drag_destroy_selection_list (selected_items);
}

static void
nautilus_list_receive_dropped_keyword (NautilusList *list,
				       int action,
				       GtkSelectionData *data,
				       int x, int y,
				       guint info)
{
	GList *emblems;

	emblems = g_list_prepend (NULL, (char *)data->data);

	gtk_signal_emit (GTK_OBJECT (list), 
			 list_signals[HANDLE_DROPPED_ITEMS],
			 action, emblems, x, y, info);

	g_list_free (emblems);
}



static void
nautilus_list_drag_data_received (GtkWidget *widget, GdkDragContext *context,
				  int x, int y, GtkSelectionData *data,
				  guint info, guint time)
{
	NautilusList *list;
	NautilusDragInfo *drag_info;

	list = NAUTILUS_LIST (widget);
	drag_info = list->details->drag_info;


	if (!drag_info->got_drop_data_type) {

		drag_info->data_type = info;
		drag_info->got_drop_data_type = TRUE;
		drag_info->selection_data = data;


		switch (info) {
		case NAUTILUS_ICON_DND_GNOME_ICON_LIST:
			drag_info->selection_list = nautilus_drag_build_selection_list (data);
			break;
		case NAUTILUS_ICON_DND_URI_LIST:
			drag_info->selection_list = nautilus_drag_build_selection_list (data);
			break;
		case NAUTILUS_ICON_DND_COLOR:
			break;
		case NAUTILUS_ICON_DND_BGIMAGE:	
			break;
		case NAUTILUS_ICON_DND_KEYWORD:	
			break;
		default:
			break;
		}
	}

	if (drag_info->drop_occured) {

		switch (info) {
		case NAUTILUS_ICON_DND_GNOME_ICON_LIST:
			nautilus_list_receive_dropped_icons
				(NAUTILUS_LIST (list),
				 context->action, data, x, y, info);
			gtk_drag_finish (context, TRUE, FALSE, time);
			break;
		case NAUTILUS_ICON_DND_URI_LIST:
			nautilus_list_receive_dropped_icons
				(NAUTILUS_LIST (list),
				 context->action, data, x, y, info);
			gtk_drag_finish (context, TRUE, FALSE, time);
			break;
		case NAUTILUS_ICON_DND_COLOR:
			nautilus_background_receive_dropped_color
				(nautilus_get_widget_background (widget),
				 widget, x, y, data);
			nautilus_list_setup_style_colors (NAUTILUS_LIST (list));
			gtk_drag_finish (context, TRUE, FALSE, time);
			break;
		case NAUTILUS_ICON_DND_BGIMAGE:
			nautilus_background_receive_dropped_background_image
				(nautilus_get_widget_background (widget),
				 (char *)data->data);
			gtk_drag_finish (context, TRUE, FALSE, time);
			break;
		case NAUTILUS_ICON_DND_KEYWORD:
			nautilus_list_receive_dropped_keyword
				(NAUTILUS_LIST (list),
				 context->action, data, x, y, info);
			gtk_drag_finish (context, TRUE, FALSE, time);
			break;
		default:
			gtk_drag_finish (context, FALSE, FALSE, time);
			break;
		}


		drag_info->drop_occured = FALSE;
		drag_info->got_drop_data_type = FALSE;
	}
}



/* Our handler for the clear signal of the clist.  We have to reset the anchor
  * to null.
 */
static void
nautilus_list_clear (NautilusCList *clist)
{
	NautilusList *list;

	g_return_if_fail (NAUTILUS_IS_LIST (clist));

	list = NAUTILUS_LIST (clist);
	list->details->anchor_row = -1;

	NAUTILUS_CALL_PARENT_CLASS (NAUTILUS_CLIST_CLASS, clear, (clist));
}


/**
 * nautilus_list_new_with_titles:
 * @columns: The number of columns in the list
 * @titles: The titles for the columns
 * 
 * Return value: The newly-created file list.
 **/
GtkWidget *
nautilus_list_new_with_titles (int columns, const char * const *titles)
{
	NautilusList *list;

	list = NAUTILUS_LIST (gtk_type_new (nautilus_list_get_type ()));
	nautilus_clist_construct (NAUTILUS_CLIST (list), columns, NULL);
	if (titles) {
		NautilusCList *clist;
		int index;

		clist = NAUTILUS_CLIST(list);
		
		for (index = 0; index < columns; index++) {
  			clist->column[index].title = g_strdup (titles[index]);
  		}
    	}

	nautilus_clist_set_selection_mode (NAUTILUS_CLIST (list),
				      GTK_SELECTION_MULTIPLE);

	return GTK_WIDGET (list);
}

NautilusCListRow *
nautilus_list_row_at (NautilusList *list, int y)
{
	NautilusCList *clist;
	int row_index, column_index;

	clist = NAUTILUS_CLIST (list);
	y -= (GTK_CONTAINER (list)->border_width +
		GTK_WIDGET (list)->style->klass->ythickness +
		clist->column_title_area.height);
	
	if (!nautilus_clist_get_selection_info (clist, 10, y, &row_index, &column_index)) {
		return NULL;
	}
	
	return g_list_nth (clist->row_list, row_index)->data;
}

GList *
nautilus_list_get_selection (NautilusList *list)
{
	GList *retval;
	GList *p;

	g_return_val_if_fail (NAUTILUS_IS_LIST (list), NULL);

	retval = NULL;
	for (p = NAUTILUS_CLIST (list)->row_list; p != NULL; p = p->next) {
		NautilusCListRow *row;

		row = p->data;
		if (row->state == GTK_STATE_SELECTED)
			retval = g_list_prepend (retval, row->data);
	}

	return retval;
}

void
nautilus_list_set_selection (NautilusList *list, GList *selection)
{
	gboolean selection_changed;
	GHashTable *hash;
	GList *p;
	int i;
	NautilusCListRow *row;

	g_return_if_fail (NAUTILUS_IS_LIST (list));

	selection_changed = FALSE;

	hash = g_hash_table_new (NULL, NULL);
	for (p = selection; p != NULL; p = p->next) {
		g_hash_table_insert (hash, p->data, p->data);
	}

	for (p = NAUTILUS_CLIST (list)->row_list, i = 0; p != NULL; p = p->next, i++) {
		row = p->data;
		selection_changed |= row_set_selected (list, i, row, g_hash_table_lookup (hash, row->data) != NULL);
	}
	
	g_hash_table_destroy (hash);

	if (selection_changed) {
		emit_selection_changed (list);
	}
}

void
nautilus_list_each_selected_row (NautilusList *list, NautilusEachRowFunction function,
	gpointer data)
{
	NautilusCListRow *row;
	GList *p;

	g_assert (NAUTILUS_IS_LIST (list));

	for (p = NAUTILUS_CLIST (list)->row_list; p != NULL; p = p->next) {
		row = p->data;
		if (row->state != GTK_STATE_SELECTED) 
			continue;

		if (!function(row, data))
			return;
	}
}

/**
 * nautilus_list_get_first_selected_row:
 * 
 * Get the index of the first selected row, or -1 if no rows are selected.
 * @list: Any NautilusList
 **/
int
nautilus_list_get_first_selected_row (NautilusList *list)
{
	NautilusCListRow *row;
	GList *p;
	int row_index;

	g_return_val_if_fail (NAUTILUS_IS_LIST (list), -1);

	for (p = NAUTILUS_CLIST (list)->row_list, row_index = 0; 
	     p != NULL; 
	     p = p->next, ++row_index) {
		row = p->data;
		if (row->state == GTK_STATE_SELECTED) 
			return row_index;
	}

	return -1;
}

/**
 * nautilus_list_get_last_selected_row:
 * 
 * Get the index of the last selected row, or -1 if no rows are selected.
 * @list: Any GtkCList
 **/
int 
nautilus_list_get_last_selected_row (NautilusList *list)
{
	NautilusCListRow *row;
	GList *p;
	int row_index;

	g_return_val_if_fail (NAUTILUS_IS_LIST (list), -1);

	for (p = NAUTILUS_CLIST (list)->row_list_end, row_index = NAUTILUS_CLIST (list)->rows - 1; p != NULL; p = p->prev, --row_index) {
		row = p->data;
		if (row->state == GTK_STATE_SELECTED) {
			return row_index;	
		}
	}

	return -1;
}

/* Workaround for a bug in GtkCList's insert_row.
 * It sets the focus row to 0 if there is exactly one row,
 * even if there was no focus on entry.
 * Although this works for focus, there may still be a problem
 * with selection.
 */
static int
insert_row (NautilusCList *list, int row_index, char *text[])
{
	gboolean had_focus;
	int result;

	had_focus = list->focus_row != -1;

	result = NAUTILUS_CALL_PARENT_CLASS
		(NAUTILUS_CLIST_CLASS, insert_row, (list, row_index, text));

	if (!had_focus) {
		list->focus_row = -1;
	}

	return result;
}


void 	     
nautilus_list_set_drag_prelight_row (NautilusList *list, int y)
{
	NautilusCList *clist;
	NautilusCListRow *row, *last_row;
	GdkRectangle rect;
	int row_index;
	
	clist = NAUTILUS_CLIST (list);

	row = NULL;
	
	if (y >= 0) { 
		row = nautilus_list_row_at (list, y);
	}

	if (row != list->details->drag_prelight_row) {
		last_row = list->details->drag_prelight_row;
		list->details->drag_prelight_row = row;
		
		/* Redraw old cell */
		if (last_row != NULL) {
			row_index = g_list_index (clist->row_list, last_row);
			get_cell_rectangle (clist, row_index, 0, &rect);
			gtk_widget_draw (GTK_WIDGET (list), &rect);			
		}
		
		/* Draw new cell */
		if (list->details->drag_prelight_row != NULL) {
			row_index = g_list_index (clist->row_list, list->details->drag_prelight_row);
			get_cell_rectangle (clist, row_index, 0, &rect);
			gtk_widget_draw (GTK_WIDGET (list), &rect);			
		}
	}
}

