/* gnome-druid-page-eazel.c
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2000  Eazel, Inc.
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#include <config.h>

#include "nautilus-druid-page-eazel.h"

#include "nautilus-gtk-macros.h"

#include <libgnomeui/gnome-uidefs.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomeui/gnome-druid.h>
#include <libgnomeui/gnome-druid-page.h>

#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
#include "nautilus-druid.h"
#include <libgnome/gnome-i18n.h>

#include <libnautilus-extensions/nautilus-file-utilities.h>

struct NautilusDruidPageEazelDetails
{
	GnomeCanvasItem  *background_item;
	GnomeCanvasItem  *background_image_item;
	GnomeCanvasItem  *topbar_image_item;
	int               topbar_image_width;
	GnomeCanvasItem  *topbar_image_stretch_item;
	GnomeCanvasItem  *title_item;
	GnomeCanvasItem  *text_item;
	GnomeCanvasItem  *sidebar_image_item;
	GnomeCanvasItem  *title_image_item;
	GnomeCanvasItem  *widget_item;
};

static void nautilus_druid_page_eazel_initialize   	(NautilusDruidPageEazel		*druid_page_eazel);
static void nautilus_druid_page_eazel_initialize_class	(NautilusDruidPageEazelClass	*klass);
static void nautilus_druid_page_eazel_destroy       (GtkObject *object);
static void nautilus_druid_page_eazel_finalize      (GtkObject *object);
static void nautilus_druid_page_eazel_construct     (NautilusDruidPageEazel *druid_page_eazel);
static void nautilus_druid_page_eazel_configure_size(NautilusDruidPageEazel *druid_page_eazel,
						     gint width,
						     gint height);
static void nautilus_druid_page_eazel_size_allocate (GtkWidget     *widget,
						     GtkAllocation *allocation);
static void nautilus_druid_page_eazel_size_request  (GtkWidget      *widget,
						     GtkRequisition *requisition);
static void nautilus_druid_page_eazel_prepare       (GnomeDruidPage *page,
						     GtkWidget      *druid,
						     gpointer 	    *data);

#define TITLE_X 34.0
#define TITLE_Y 60.0
#define CONTENT_PADDING 15.0
#define DEFAULT_CONTENT_X 34.0
#define DRUID_PAGE_HEIGHT 322
#define DRUID_PAGE_WIDTH 516

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusDruidPageEazel, nautilus_druid_page_eazel, GNOME_TYPE_DRUID_PAGE)

static void
nautilus_druid_page_eazel_initialize_class (NautilusDruidPageEazelClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;

	parent_class = gtk_type_class (gnome_druid_page_get_type ());

	object_class->destroy = nautilus_druid_page_eazel_destroy;
	object_class->finalize = nautilus_druid_page_eazel_finalize;

	widget_class->size_allocate = nautilus_druid_page_eazel_size_allocate;
	widget_class->size_request = nautilus_druid_page_eazel_size_request;
}

static void
nautilus_druid_page_eazel_initialize (NautilusDruidPageEazel *druid_page_eazel)
{
	druid_page_eazel->details = g_new0(NautilusDruidPageEazelDetails, 1);

	/* Set up the canvas */
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_eazel), 0);
	gtk_widget_push_visual (gdk_rgb_get_visual ());
	gtk_widget_push_colormap (gdk_rgb_get_cmap ());
	druid_page_eazel->canvas = gnome_canvas_new_aa ();
	gtk_widget_pop_visual ();
	gtk_widget_pop_colormap ();
	gtk_widget_set_usize (druid_page_eazel->canvas, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_widget_show (druid_page_eazel->canvas);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (druid_page_eazel->canvas), 0.0, 0.0, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_container_add (GTK_CONTAINER (druid_page_eazel), druid_page_eazel->canvas);

}

static void
nautilus_druid_page_eazel_destroy(GtkObject *object)
{
	NautilusDruidPageEazel *druid_page_eazel =
		NAUTILUS_DRUID_PAGE_EAZEL(object);

	druid_page_eazel->canvas = NULL;
	druid_page_eazel->widget = NULL;

	g_free (druid_page_eazel->title);
	druid_page_eazel->title = NULL;
	g_free (druid_page_eazel->text);
	druid_page_eazel->text = NULL;

	if (druid_page_eazel->title_image != NULL)
		gdk_pixbuf_unref (druid_page_eazel->title_image);
	druid_page_eazel->title_image = NULL;
	if (druid_page_eazel->sidebar_image != NULL)
		gdk_pixbuf_unref (druid_page_eazel->sidebar_image);
	druid_page_eazel->sidebar_image = NULL;

	if (druid_page_eazel->widget != NULL)
		gtk_widget_unref (druid_page_eazel->widget);
	druid_page_eazel->widget = NULL;

	/* Chain destroy */
	NAUTILUS_CALL_PARENT_CLASS (GTK_OBJECT_CLASS, destroy, (object));
}

static void
nautilus_druid_page_eazel_finalize(GtkObject *object)
{
	NautilusDruidPageEazel *druid_page_eazel =
		NAUTILUS_DRUID_PAGE_EAZEL(object);

	g_free(druid_page_eazel->details);
	druid_page_eazel->details = NULL;

	/* Chain finalize */
	NAUTILUS_CALL_PARENT_CLASS (GTK_OBJECT_CLASS, finalize, (object));
}

static void
get_content_xy (NautilusDruidPageEazel *druid_page_eazel,
		double *content_x, double *content_y)
{
	double title_height;

	if (druid_page_eazel->sidebar_image) {
		*content_x = gdk_pixbuf_get_width (druid_page_eazel->sidebar_image);
	} else {
		*content_x = DEFAULT_CONTENT_X;
	}

	if (druid_page_eazel->title_image) {
		*content_y = gdk_pixbuf_get_height (druid_page_eazel->title_image) + TITLE_Y + CONTENT_PADDING;
	} else {
		*content_y = TITLE_Y;
	}

	title_height = 0.0;
	if (druid_page_eazel->title != NULL
	    && druid_page_eazel->title[0] != '\0') {
		gtk_object_get (GTK_OBJECT (druid_page_eazel->details->title_item),
				"text_height", &title_height,
				NULL);
		title_height += CONTENT_PADDING;
	}

	if (*content_y < title_height + TITLE_Y) {
		*content_y = title_height + TITLE_Y;
	}
}


static void
nautilus_druid_page_eazel_configure_size (NautilusDruidPageEazel *druid_page_eazel, gint width, gint height)
{
	double content_x;
	double content_y;

	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	gnome_canvas_item_set (druid_page_eazel->details->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       NULL);

	gnome_canvas_item_set (druid_page_eazel->details->topbar_image_stretch_item,
			       "width", (double) (width - druid_page_eazel->details->topbar_image_width),
			       NULL);

	get_content_xy (druid_page_eazel, &content_x, &content_y);

	if (druid_page_eazel->details->widget_item != NULL)
		gnome_canvas_item_set (druid_page_eazel->details->widget_item,
				       "x", content_x,
				       "y", content_y,
				       "width", width - content_x,
				       "height", height - content_y,
				       NULL);

	gnome_canvas_item_set (druid_page_eazel->details->text_item,
			       "x", content_x,
			       "y", content_y,
			       NULL);


}

static void
set_image (GnomeCanvasItem *item, const char *file,
	   int *width, int *height)
{
	char *fullname;

	if (width != NULL)
		*width = 0;
	if (height != NULL)
		*height = 0;

	fullname = nautilus_pixmap_file (file);
	if (fullname != NULL) {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (fullname);
		if (pixbuf != NULL) {
			if (width != NULL)
				*width = gdk_pixbuf_get_width (pixbuf);
			if (height != NULL)
				*height = gdk_pixbuf_get_height (pixbuf);
			gnome_canvas_item_set (item,
					       "pixbuf", pixbuf,
					       NULL);
			gdk_pixbuf_unref (pixbuf);
		}
		g_free (fullname);
	}
}

static void
nautilus_druid_page_eazel_construct (NautilusDruidPageEazel *druid_page_eazel)
{
	druid_page_eazel->details->background_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_rect_get_type (),
				       "x1", 0.0,
				       "y1", 0.0,
				       "fill_color", "white",
				       NULL);

	druid_page_eazel->details->background_image_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x", 0.0,
				       "y", 0.0,
				       "x_in_pixels", TRUE,
				       "y_in_pixels", TRUE,
				       NULL);
	if (druid_page_eazel->background_image)
		gnome_canvas_item_set (druid_page_eazel->details->background_image_item,
				       "pixbuf", druid_page_eazel->background_image,
				       NULL);

	druid_page_eazel->details->sidebar_image_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x", 0.0,
				       "y", 0.0,
				       "x_in_pixels", TRUE,
				       "y_in_pixels", TRUE,
				       NULL);
	if (druid_page_eazel->sidebar_image)
		gnome_canvas_item_set (druid_page_eazel->details->sidebar_image_item,
				       "pixbuf", druid_page_eazel->sidebar_image,
				       NULL);

	druid_page_eazel->details->topbar_image_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x", 0.0,
				       "y", 0.0,
				       "x_in_pixels", TRUE,
				       "y_in_pixels", TRUE,
				       NULL);
	set_image (druid_page_eazel->details->topbar_image_item,
		   "druid_header.png",
		   &druid_page_eazel->details->topbar_image_width,
		   NULL);

	druid_page_eazel->details->topbar_image_stretch_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x", (double)druid_page_eazel->details->topbar_image_width,
				       "y", 0.0,
				       "width", (double)(DRUID_PAGE_WIDTH - druid_page_eazel->details->topbar_image_width),
				       "width_set", TRUE,
				       "x_in_pixels", TRUE,
				       "y_in_pixels", TRUE,
				       NULL);
	set_image (druid_page_eazel->details->topbar_image_stretch_item,
		   "druid_header_stretch.png", NULL, NULL);

	druid_page_eazel->details->title_image_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x", TITLE_X,
				       "y", TITLE_Y,
				       "x_in_pixels", TRUE,
				       "y_in_pixels", TRUE,
				       NULL);
	if (druid_page_eazel->title_image)
		gnome_canvas_item_set (druid_page_eazel->details->title_image_item,
				       "pixbuf", druid_page_eazel->title_image,
				       NULL);

	druid_page_eazel->details->title_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_text_get_type (),
				       "x", TITLE_X,
				       "y", TITLE_Y,
				       "text", druid_page_eazel->title,
				       "fill_color", "black",
				       "fontset", _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*,*-r-*"),
				       "anchor", GTK_ANCHOR_NW,
				       NULL);

	druid_page_eazel->details->text_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_eazel->text,
				       "fill_color", "black",
				       "fontset", _("-adobe-helvetica-bold-r-normal-*-*-120-*-*-p-*-*-*,*-r-*"),
				       "anchor", GTK_ANCHOR_NW,
				       NULL);

	nautilus_druid_page_eazel_configure_size (druid_page_eazel, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_signal_connect (GTK_OBJECT (druid_page_eazel),
			    "prepare",
			    nautilus_druid_page_eazel_prepare,
			    NULL);
}

static void
nautilus_druid_page_eazel_prepare (GnomeDruidPage *page,
				   GtkWidget *druid,
				   gpointer *data)
{
	switch (NAUTILUS_DRUID_PAGE_EAZEL (page)->position) {
	case NAUTILUS_DRUID_PAGE_EAZEL_START:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), FALSE, TRUE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
		gtk_widget_grab_default (GNOME_DRUID (druid)->next);
		break;
	case NAUTILUS_DRUID_PAGE_EAZEL_FINISH:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, FALSE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), TRUE);
		gtk_widget_grab_default (GNOME_DRUID (druid)->finish);
		break;
	case NAUTILUS_DRUID_PAGE_EAZEL_OTHER:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, TRUE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
	default:
		break;
	}
}


static void
nautilus_druid_page_eazel_size_allocate(GtkWidget               *widget,
					GtkAllocation           *allocation)
{
	NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, size_allocate,
				    (widget, allocation));

	gnome_canvas_set_scroll_region (GNOME_CANVAS (NAUTILUS_DRUID_PAGE_EAZEL (widget)->canvas),
					0.0, 0.0,
					allocation->width,
					allocation->height);
	nautilus_druid_page_eazel_configure_size (NAUTILUS_DRUID_PAGE_EAZEL (widget),
						  allocation->width,
						  allocation->height);
}

static void
nautilus_druid_page_eazel_size_request(GtkWidget           *widget,
				       GtkRequisition      *requisition)
{
	NautilusDruidPageEazel *druid_page_eazel;

	druid_page_eazel = NAUTILUS_DRUID_PAGE_EAZEL (widget);

	NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, size_request,
				    (widget, requisition));

	if (druid_page_eazel->widget) {
		GtkRequisition child_requisition;
		double x, y;

		g_assert (druid_page_eazel->details->widget_item != NULL);

		gtk_object_get (GTK_OBJECT (druid_page_eazel->details->widget_item),
				"x", &x,
				"y", &y,
				NULL);

		gtk_widget_get_child_requisition (druid_page_eazel->widget,
						  &child_requisition);

		if (child_requisition.width + x > requisition->width) {
			requisition->width = child_requisition.width + x;
		}
		if (child_requisition.height + y > requisition->height) {
			requisition->height = child_requisition.height + y;
		}

	}
}


/**
 * nautilus_druid_page_eazel_new:
 *
 * Creates a new NautilusDruidPageEazel widget.
 *
 * Return value: Pointer to new NautilusDruidPageEazel
 **/
/* Public functions */
GtkWidget *
nautilus_druid_page_eazel_new (NautilusDruidPageEazelPosition position)
{
	NautilusDruidPageEazel *page;

	page = NAUTILUS_DRUID_PAGE_EAZEL (gtk_widget_new (nautilus_druid_page_eazel_get_type (), NULL));

	page->position = position;
	page->title = g_strdup ("");
	page->text = g_strdup ("");
	page->title_image = NULL;
	page->sidebar_image = NULL;
	page->background_image = NULL;
	nautilus_druid_page_eazel_construct (page);

	return GTK_WIDGET (page);
}
/**
 * nautilus_druid_page_eazel_new_with_vals:
 * @title: The title.
 * @text: The introduction text.
 * @logo: The logo in the upper right corner.
 * @watermark: The watermark on the left.
 *
 * This will create a new GNOME Druid Eazel page, with the values
 * given.  It is acceptable for any of them to be %NULL.
 *
 * Return value: GtkWidget pointer to new NautilusDruidPageEazel.
 **/
GtkWidget *
nautilus_druid_page_eazel_new_with_vals (NautilusDruidPageEazelPosition position,
					 const gchar *title,
					 const gchar* text,
					 GdkPixbuf *title_image,
					 GdkPixbuf *sidebar_image,
					 GdkPixbuf *background_image)
{
	NautilusDruidPageEazel *page;

	page = NAUTILUS_DRUID_PAGE_EAZEL (gtk_widget_new (nautilus_druid_page_eazel_get_type (), NULL));

	page->position = position;
	page->title = g_strdup (title ? title : "");
	page->text = g_strdup (text ? text : "");

	if (title_image)
		gdk_pixbuf_ref (title_image);
	page->title_image = title_image;

	if (sidebar_image)
		gdk_pixbuf_ref (sidebar_image);
	page->sidebar_image = sidebar_image;

	if (background_image)
		gdk_pixbuf_ref (background_image);
	page->background_image = background_image;

	nautilus_druid_page_eazel_construct (page);

	return GTK_WIDGET (page);
}

void
nautilus_druid_page_eazel_set_text (NautilusDruidPageEazel *druid_page_eazel,
				    const gchar *text)
{
	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	g_free (druid_page_eazel->text);
	druid_page_eazel->text = g_strdup (text ? text : "");
	gnome_canvas_item_set (druid_page_eazel->details->text_item,
			       "text", druid_page_eazel->text,
			       NULL);
}
void
nautilus_druid_page_eazel_set_title (NautilusDruidPageEazel *druid_page_eazel,
				     const gchar *title)
{
	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	g_free (druid_page_eazel->title);
	druid_page_eazel->title = g_strdup (title ? title : "");
	gnome_canvas_item_set (druid_page_eazel->details->title_item,
			       "text", druid_page_eazel->title,
			       NULL);
}
void
nautilus_druid_page_eazel_set_title_image (NautilusDruidPageEazel *druid_page_eazel,
					   GdkPixbuf *title_image)
{
	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	if (druid_page_eazel->title_image)
		gdk_pixbuf_unref (druid_page_eazel->title_image);

	druid_page_eazel->title_image = title_image;
	if (title_image != NULL)
		gdk_pixbuf_ref (title_image);
	gnome_canvas_item_set (druid_page_eazel->details->title_image_item,
			       "pixbuf", druid_page_eazel->title_image, NULL);
}

void
nautilus_druid_page_eazel_set_sidebar_image (NautilusDruidPageEazel *druid_page_eazel,
					     GdkPixbuf *sidebar_image)
{
	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	if (druid_page_eazel->sidebar_image)
		gdk_pixbuf_unref (druid_page_eazel->sidebar_image);

	druid_page_eazel->sidebar_image = sidebar_image;
	if (sidebar_image != NULL)
		gdk_pixbuf_ref (sidebar_image);
	gnome_canvas_item_set (druid_page_eazel->details->sidebar_image_item,
			       "pixbuf", druid_page_eazel->sidebar_image, NULL);
}

void
nautilus_druid_page_eazel_set_background_image (NautilusDruidPageEazel *druid_page_eazel,
						GdkPixbuf *background_image)
{
	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	if (druid_page_eazel->background_image)
		gdk_pixbuf_unref (druid_page_eazel->background_image);

	druid_page_eazel->background_image = background_image;
	if (background_image != NULL)
		gdk_pixbuf_ref (background_image);
	gnome_canvas_item_set (druid_page_eazel->details->background_image_item,
			       "pixbuf", druid_page_eazel->background_image, NULL);
}

void
nautilus_druid_page_eazel_put_widget (NautilusDruidPageEazel *druid_page_eazel,
				      GtkWidget *widget)
{
	double content_x;
	double content_y;

	g_return_if_fail (druid_page_eazel != NULL);
	g_return_if_fail (NAUTILUS_IS_DRUID_PAGE_EAZEL (druid_page_eazel));

	get_content_xy (druid_page_eazel, &content_x, &content_y);

	if (druid_page_eazel->details->widget_item != NULL)
		gtk_object_destroy (GTK_OBJECT (druid_page_eazel->details->widget_item));
	if (druid_page_eazel->widget != NULL)
		gtk_widget_unref (druid_page_eazel->widget);
	druid_page_eazel->widget = widget;
	if (widget != NULL)
		gtk_widget_ref (widget);

	druid_page_eazel->details->widget_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_eazel->canvas)),
				       gnome_canvas_widget_get_type (),
				       "x", content_x,
				       "y", content_y,
				       "width", DRUID_PAGE_WIDTH - content_x,
				       "height", DRUID_PAGE_HEIGHT - content_y,
				       "widget", widget,
				       NULL);

	gtk_widget_queue_resize (GTK_WIDGET (druid_page_eazel));
}
