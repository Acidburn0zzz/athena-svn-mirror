/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-viewport.c - A subclass of GtkViewport with non broken drawing.

   Copyright (C) 1999, 2000 Eazel, Inc.

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

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>

#include "nautilus-viewport.h"

#include "nautilus-gtk-macros.h"

#include <gtk/gtksignal.h>

/* Detail member struct */
struct NautilusViewportDetails
{
	gboolean is_smooth;
	gboolean never_smooth;
       
	gboolean constrain_width;
	gboolean constrain_height;
};

/* GtkObjectClass methods */
static void nautilus_viewport_initialize_class     (NautilusViewportClass *viewport_class);
static void nautilus_viewport_initialize           (NautilusViewport      *viewport);
static void nautilus_viewport_destroy              (GtkObject             *object);

/* GtkWidgetClass methods */
static void nautilus_viewport_realize              (GtkWidget             *widget);
static void nautilus_viewport_draw                 (GtkWidget             *widget,
						    GdkRectangle          *area);
static void nautilus_viewport_size_allocate        (GtkWidget             *widget,
						    GtkAllocation         *allocation);
static gint nautilus_viewport_expose_event         (GtkWidget             *widget,
						    GdkEventExpose        *event);
static void nautilus_viewport_paint                (GtkWidget             *widget,
						    GdkRectangle          *area);

/* NautilusViewport signals */
static void nautilus_viewport_set_is_smooth_signal (GtkWidget             *widget,
						    gboolean               is_smooth);

/* Signals */
typedef enum
{
	SET_IS_SMOOTH,
	LAST_SIGNAL
} NautilusViewportSignal;

/* Signals */
static guint nautilus_viewport_signals[LAST_SIGNAL] = { 0 };

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusViewport, nautilus_viewport, GTK_TYPE_VIEWPORT)

/* GtkObjectClass methods */
static void
nautilus_viewport_initialize_class (NautilusViewportClass *nautilus_viewport_class)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (nautilus_viewport_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (nautilus_viewport_class);

	/* GtkObjectClass */
	object_class->destroy = nautilus_viewport_destroy;
	
	/* GtkWidgetClass */
	widget_class->realize = nautilus_viewport_realize;
	widget_class->expose_event = nautilus_viewport_expose_event;
	widget_class->draw = nautilus_viewport_draw;
	widget_class->size_allocate = nautilus_viewport_size_allocate;
	
	/* NautilusViewportClass */
	nautilus_viewport_class->set_is_smooth = nautilus_viewport_set_is_smooth_signal;
	
	nautilus_viewport_signals[SET_IS_SMOOTH] = 
		gtk_signal_new ("set_is_smooth",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (NautilusViewportClass, set_is_smooth),
				gtk_marshal_NONE__BOOL,
				GTK_TYPE_NONE, 
				1,
				GTK_TYPE_BOOL);
	
	gtk_object_class_add_signals (object_class, nautilus_viewport_signals, LAST_SIGNAL);

	/* Let the smooth widget machinery know that our class can be smooth */
	nautilus_smooth_widget_register_type (NAUTILUS_TYPE_VIEWPORT);
}

void
nautilus_viewport_initialize (NautilusViewport *nautilus_viewport)
{
	nautilus_viewport->details = g_new0 (NautilusViewportDetails, 1);
	
	nautilus_smooth_widget_register (GTK_WIDGET (nautilus_viewport));
	nautilus_viewport->details->never_smooth = TRUE;
}

void
nautilus_viewport_destroy (GtkObject *object)
{
	NautilusViewport *viewport;

	g_return_if_fail (NAUTILUS_IS_VIEWPORT (object));

	viewport = NAUTILUS_VIEWPORT (object);
	
	g_free (viewport->details);

	/* Chain destroy */
	NAUTILUS_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

/* GtkWidgetClass methods */
static void
nautilus_viewport_draw (GtkWidget *widget,
			GdkRectangle *area)
{
	NautilusViewport *nautilus_viewport;
	GtkViewport *viewport;
	GtkBin *bin;
	GdkRectangle tmp_area;
	GdkRectangle child_area;
	gint border_width;
	
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (widget));
	g_return_if_fail (area != NULL);
	
	if (!GTK_WIDGET_DRAWABLE (widget)) {
		return;
	}

	nautilus_viewport = NAUTILUS_VIEWPORT (widget);
	viewport = GTK_VIEWPORT (widget);
	bin = GTK_BIN (widget);
	
	border_width = GTK_CONTAINER (widget)->border_width;
	
	tmp_area = *area;
	tmp_area.x -= border_width;
	tmp_area.y -= border_width;
	
	nautilus_viewport_paint (widget, &tmp_area);
	
	tmp_area.x += viewport->hadjustment->value - widget->style->klass->xthickness;
	tmp_area.y += viewport->vadjustment->value - widget->style->klass->ythickness;
	
	/* The gtk_viewport_draw() version does not adjust the width
	 * and height of the tmp_area for the class x/y thickness.  This
	 * causes some drawing to be clipped on the bottom.  This is a bug
	 * in GTK+.
	 */
	
	/* FIXME bugzilla.eazel.com xxxx: 
	 * Remove this widget once the fix makes it to GTK+.
	 */
	tmp_area.width += 2 * widget->style->klass->xthickness;
	tmp_area.height += 2 * widget->style->klass->ythickness;
	
	if (!nautilus_viewport_get_is_smooth (nautilus_viewport)) {
		gtk_paint_flat_box (widget->style, viewport->bin_window, 
				    GTK_STATE_NORMAL, GTK_SHADOW_NONE,
				    &tmp_area, widget, "viewportbin",
				    0, 0, -1, -1);
	}
	
	if (bin->child) {
		if (gtk_widget_intersect (bin->child, &tmp_area, &child_area)) {
			gtk_widget_draw (bin->child, &child_area);
		}
	}
}

static void
nautilus_viewport_size_allocate (GtkWidget *widget,
				 GtkAllocation *allocation)
{
	NautilusViewport *nautilus_viewport;
	GtkViewport *viewport;
	GtkBin *bin;
	GtkAllocation child_allocation;
	gint hval, vval;
	gint border_width;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_VIEWPORT (widget));
	g_return_if_fail (allocation != NULL);
	
	widget->allocation = *allocation;
	nautilus_viewport = NAUTILUS_VIEWPORT (widget);
	viewport = GTK_VIEWPORT (widget);
	bin = GTK_BIN (widget);
	  
	border_width = GTK_CONTAINER (widget)->border_width;
	
	child_allocation.x = 0;
	child_allocation.y = 0;
	
	if (viewport->shadow_type != GTK_SHADOW_NONE)
	{
		child_allocation.x = GTK_WIDGET (viewport)->style->klass->xthickness;
		child_allocation.y = GTK_WIDGET (viewport)->style->klass->ythickness;
	}
	
	child_allocation.width = MAX (1, allocation->width - child_allocation.x * 2 - border_width * 2);
	child_allocation.height = MAX (1, allocation->height - child_allocation.y * 2 - border_width * 2);
	  
	if (GTK_WIDGET_REALIZED (widget))
	{
		gdk_window_move_resize (widget->window,
					allocation->x + border_width,
					allocation->y + border_width,
					allocation->width - border_width * 2,
					allocation->height - border_width * 2);
		
		gdk_window_move_resize (viewport->view_window,
					child_allocation.x,
					child_allocation.y,
					child_allocation.width,
					child_allocation.height);
	}
	
	viewport->hadjustment->page_size = child_allocation.width;
	viewport->hadjustment->page_increment = viewport->hadjustment->page_size / 2;
	viewport->hadjustment->step_increment = 10;
	
	viewport->vadjustment->page_size = child_allocation.height;
	viewport->vadjustment->page_increment = viewport->vadjustment->page_size / 2;
	viewport->vadjustment->step_increment = 10;
	
	hval = viewport->hadjustment->value;
	vval = viewport->vadjustment->value;
	
	if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
	{
		GtkRequisition child_requisition;
		gtk_widget_get_child_requisition (bin->child, &child_requisition);
		
		viewport->hadjustment->lower = 0;
		viewport->hadjustment->upper = MAX (child_requisition.width,
						    child_allocation.width);
		
		hval = CLAMP (hval, 0,
			      viewport->hadjustment->upper -
			      viewport->hadjustment->page_size);
		
		viewport->vadjustment->lower = 0;
		viewport->vadjustment->upper = MAX (child_requisition.height,
						    child_allocation.height);
		
		vval = CLAMP (vval, 0,
			      viewport->vadjustment->upper -
			      viewport->vadjustment->page_size);
	}
	
	if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
	{
		child_allocation.x = 0;
		child_allocation.y = 0;
		
		child_allocation.width = viewport->hadjustment->upper;
		child_allocation.height = viewport->vadjustment->upper;
		
		if (nautilus_viewport_get_constrain_width (nautilus_viewport)) {
			child_allocation.width = widget->allocation.width;
		}
		if (nautilus_viewport_get_constrain_height (nautilus_viewport)) {
			child_allocation.height = widget->allocation.height;
		}
		
		if (GTK_WIDGET_REALIZED (widget))
			gdk_window_resize (viewport->bin_window,
					   child_allocation.width,
					   child_allocation.height);
		
		child_allocation.x = 0;
		child_allocation.y = 0;
		gtk_widget_size_allocate (bin->child, &child_allocation);
	}
	
	gtk_signal_emit_by_name (GTK_OBJECT (viewport->hadjustment), "changed");
	gtk_signal_emit_by_name (GTK_OBJECT (viewport->vadjustment), "changed");
	if (viewport->hadjustment->value != hval)
	{
		viewport->hadjustment->value = hval;
		gtk_signal_emit_by_name (GTK_OBJECT (viewport->hadjustment), "value_changed");
	}
	if (viewport->vadjustment->value != vval)
	{
		viewport->vadjustment->value = vval;
		gtk_signal_emit_by_name (GTK_OBJECT (viewport->vadjustment), "value_changed");
	}
}

static gint
nautilus_viewport_expose_event (GtkWidget *widget,
				GdkEventExpose *event)
{
	NautilusViewport *nautilus_viewport;
	GtkViewport *viewport;
	GtkBin *bin;
	
	g_return_val_if_fail (NAUTILUS_IS_VIEWPORT (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (!GTK_WIDGET_DRAWABLE (widget)) {
		return FALSE;
	}

	nautilus_viewport = NAUTILUS_VIEWPORT (widget);
	viewport = GTK_VIEWPORT (widget);
	bin = GTK_BIN (widget);
	
	if (event->window == widget->window) {
		nautilus_viewport_paint (widget, &event->area);
	} else if (event->window == viewport->bin_window) {
		GdkEventExpose child_event;

		child_event = *event;
		
		if (!nautilus_viewport_get_is_smooth (nautilus_viewport)) {
			gtk_paint_flat_box (widget->style, viewport->bin_window, 
					    GTK_STATE_NORMAL, GTK_SHADOW_NONE,
					    &event->area, widget, "viewportbin",
					    0, 0, -1, -1);
		}
		
		if ((bin->child != NULL) &&
		    GTK_WIDGET_NO_WINDOW (bin->child) &&
		    gtk_widget_intersect (bin->child, &event->area, &child_event.area))
			gtk_widget_event (bin->child, (GdkEvent*) &child_event);
	}
	
	return FALSE;
}

static void
nautilus_viewport_realize (GtkWidget *widget)
{
	NautilusViewport *nautilus_viewport;
	
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (widget));
	
	nautilus_viewport = NAUTILUS_VIEWPORT (widget);

	/* GtkViewport does the actual realization */
	NAUTILUS_CALL_PARENT (GTK_WIDGET_CLASS, realize, (widget));

 	gdk_window_set_static_gravities (GTK_VIEWPORT (nautilus_viewport)->bin_window,
					 nautilus_viewport_get_is_smooth (nautilus_viewport));
}

static void
nautilus_viewport_paint (GtkWidget *widget,
			 GdkRectangle *area)
{
	GtkViewport *viewport;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_VIEWPORT (widget));
	g_return_if_fail (area != NULL);
	
	if (!GTK_WIDGET_DRAWABLE (widget)) {
		return;
	}

	viewport = GTK_VIEWPORT (widget);
	
	gtk_draw_shadow (widget->style, widget->window,
			 GTK_STATE_NORMAL, viewport->shadow_type,
			 0, 0, -1, -1);
}

/* NautilusViewport signals */
static void
nautilus_viewport_set_is_smooth_signal (GtkWidget *widget,
					gboolean is_smooth)
{
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (widget));
	
	nautilus_viewport_set_is_smooth (NAUTILUS_VIEWPORT (widget), is_smooth);
}

/* Public NautilusViewport methods */
GtkWidget*
nautilus_viewport_new (GtkAdjustment *hadjustment,
		       GtkAdjustment *vadjustment)
{
	NautilusViewport *nautilus_viewport;

	nautilus_viewport = NAUTILUS_VIEWPORT (gtk_widget_new (nautilus_viewport_get_type (), NULL));
	
	gtk_viewport_set_hadjustment (GTK_VIEWPORT (nautilus_viewport), hadjustment);
	gtk_viewport_set_vadjustment (GTK_VIEWPORT (nautilus_viewport), vadjustment);

	return GTK_WIDGET (nautilus_viewport);
}

void
nautilus_viewport_set_is_smooth (NautilusViewport *nautilus_viewport,
				 gboolean is_smooth)
{
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport));

	if (nautilus_viewport->details->is_smooth == is_smooth) {
		return;
	}

	nautilus_viewport->details->is_smooth = is_smooth;
	
	if (!GTK_WIDGET_REALIZED (nautilus_viewport)) {
		return;
	}
	
 	gdk_window_set_static_gravities (GTK_VIEWPORT (nautilus_viewport)->bin_window,
					 nautilus_viewport->details->is_smooth);
}

gboolean
nautilus_viewport_get_is_smooth (const NautilusViewport *nautilus_viewport)
{
	g_return_val_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport), FALSE);
	
	return !nautilus_viewport->details->never_smooth && nautilus_viewport->details->is_smooth;
}

void
nautilus_viewport_set_constrain_width (NautilusViewport *nautilus_viewport,
				       gboolean constrain_width)
{
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport));

	nautilus_viewport->details->constrain_width = constrain_width;
}

gboolean
nautilus_viewport_get_constrain_width (const NautilusViewport *nautilus_viewport)
{
	g_return_val_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport), FALSE);

	return nautilus_viewport->details->constrain_width;
}

void nautilus_viewport_set_constrain_height (NautilusViewport *nautilus_viewport,
					     gboolean constrain_height)
{
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport));
	
	nautilus_viewport->details->constrain_height = constrain_height;
}

gboolean
nautilus_viewport_get_constrain_height (const NautilusViewport *nautilus_viewport)
{
	g_return_val_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport), FALSE);

	return nautilus_viewport->details->constrain_height;
}

void
nautilus_viewport_set_never_smooth (NautilusViewport *nautilus_viewport,
				    gboolean never_smooth)
{
	g_return_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport));

	nautilus_viewport->details->never_smooth = never_smooth;

	if (!GTK_WIDGET_REALIZED (nautilus_viewport)) {
		return;
	}

 	gdk_window_set_static_gravities (GTK_VIEWPORT (nautilus_viewport)->bin_window,
					 nautilus_viewport_get_is_smooth (nautilus_viewport));
}

NautilusArtIPoint
nautilus_viewport_get_scroll_offset (const NautilusViewport *nautilus_viewport)
{
	NautilusArtIPoint scroll_offset;
	
	g_return_val_if_fail (NAUTILUS_IS_VIEWPORT (nautilus_viewport), NAUTILUS_ART_IPOINT_ZERO);

	if (!GTK_WIDGET_REALIZED (nautilus_viewport)) {
		return NAUTILUS_ART_IPOINT_ZERO;
	}

	gdk_window_get_position (GTK_VIEWPORT (nautilus_viewport)->bin_window,
				 &scroll_offset.x,
				 &scroll_offset.y);

	return scroll_offset;
}
