/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Nautilus
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Nautilus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Nautilus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 */

/* This is the implementation of the property browser window, which
 * gives the user access to an extensible palette of properties which
 * can be dropped on various elements of the user interface to
 * customize them 
 */

#include <config.h>
#include "nautilus-property-browser.h"

#include "nautilus-signaller.h"
#include <ctype.h>
#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkselection.h>
#include <gtk/gtktable.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkviewport.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-color-picker.h>
#include <libgnomeui/gnome-file-entry.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libnautilus-extensions/nautilus-background.h>
#include <libnautilus-extensions/nautilus-customization-data.h>
#include <libnautilus-extensions/nautilus-directory.h>
#include <libnautilus-extensions/nautilus-drag-window.h>
#include <libnautilus-extensions/nautilus-file-utilities.h>
#include <libnautilus-extensions/nautilus-file.h>
#include <libnautilus-extensions/nautilus-font-factory.h>
#include <libnautilus-extensions/nautilus-gdk-extensions.h>
#include <libnautilus-extensions/nautilus-gdk-pixbuf-extensions.h>
#include <libnautilus-extensions/nautilus-glib-extensions.h>
#include <libnautilus-extensions/nautilus-global-preferences.h>
#include <libnautilus-extensions/nautilus-gtk-extensions.h>
#include <libnautilus-extensions/nautilus-gtk-macros.h>
#include <libnautilus-extensions/nautilus-image.h>
#include <libnautilus-extensions/nautilus-label.h>
#include <libnautilus-extensions/nautilus-metadata.h>
#include <libnautilus-extensions/nautilus-stock-dialogs.h>
#include <libnautilus-extensions/nautilus-string.h>
#include <libnautilus-extensions/nautilus-theme.h>
#include <libnautilus-extensions/nautilus-xml-extensions.h>
#include <math.h>

/* property types */

typedef enum {
	NAUTILUS_PROPERTY_NONE,
	NAUTILUS_PROPERTY_PATTERN,
	NAUTILUS_PROPERTY_COLOR,
	NAUTILUS_PROPERTY_EMBLEM
} NautilusPropertyType;

struct NautilusPropertyBrowserDetails {
	GtkHBox *container;
	
	GtkWidget *content_container;
	GtkWidget *content_frame;
	GtkWidget *content_table;
	
	GtkWidget *category_container;
	GtkWidget *category_box;
	GtkWidget *selected_button;
	
	GtkWidget *title_box;
	GtkWidget *title_label;
	GtkWidget *help_label;
	
	GtkWidget *bottom_box;
	
	GtkWidget *add_button;
	GtkWidget *add_button_label;	
	GtkWidget *remove_button;
	GtkWidget *remove_button_label;
	
	GtkWidget *dialog;
	
	GtkWidget *keyword;
	GtkWidget *emblem_image;
	GtkWidget *file_entry;
	
	GtkWidget *color_picker;
	GtkWidget *color_name;
	
	GList *keywords;
	
	char *path;
	char *category;
	char *dragged_file;
	char *drag_type;
	char *image_path;
	
	NautilusPropertyType category_type;
	
	int category_position;
	int content_table_width;

	GdkPixbuf *property_chit;
		
	gboolean remove_mode;
	gboolean keep_around;
	gboolean has_local;
	gboolean toggle_button_flag;
};

static void     nautilus_property_browser_initialize_class      (GtkObjectClass          *object_klass);
static void     nautilus_property_browser_initialize            (GtkObject               *object);
static void     nautilus_property_browser_destroy               (GtkObject               *object);
static void     nautilus_property_browser_preferences_changed   (NautilusPropertyBrowser *property_browser);
static void     nautilus_property_browser_update_contents       (NautilusPropertyBrowser *property_browser);
static void     nautilus_property_browser_set_category          (NautilusPropertyBrowser *property_browser,
								 const char              *new_category);
static void     nautilus_property_browser_set_dragged_file      (NautilusPropertyBrowser *property_browser,
								 const char              *dragged_file_name);
static void     nautilus_property_browser_set_drag_type         (NautilusPropertyBrowser *property_browser,
								 const char              *new_drag_type);
static void     add_new_button_callback                         (GtkWidget               *widget,
								 NautilusPropertyBrowser *property_browser);
static void     cancel_remove_mode                              (NautilusPropertyBrowser *property_browser);
static void     done_button_callback                            (GtkWidget               *widget,
								 GtkWidget               *property_browser);
static void     remove_button_callback                          (GtkWidget               *widget,
								 NautilusPropertyBrowser *property_browser);
static gboolean nautilus_property_browser_delete_event_callback (GtkWidget               *widget,
								 GdkEvent                *event,
								 gpointer                 user_data);
static void     nautilus_property_browser_hide_callback 	(GtkWidget               *widget,
								 gpointer                 user_data);
static void     nautilus_property_browser_drag_end              (GtkWidget               *widget,
								 GdkDragContext          *context);
static void     nautilus_property_browser_drag_data_get         (GtkWidget               *widget,
								 GdkDragContext          *context,
								 GtkSelectionData        *selection_data,
								 guint                    info,
								 guint32                  time);
static void     nautilus_property_browser_size_allocate		(GtkWidget		 *widget,
						     		 GtkAllocation		 *allocation);

static void     nautilus_property_browser_theme_changed         (gpointer                 user_data);
static void     emit_emblems_changed_signal                     (void);

/* misc utilities */
static char *   strip_extension                                 (const char              *string_to_strip);

#define BROWSER_BACKGROUND_COLOR "rgb:FFFF/FFFF/FFFF"

#define THEME_SELECT_COLOR "rgb:FFFF/9999/9999"

#define BROWSER_CATEGORIES_FILE_NAME "browser.xml"

#define PROPERTY_BROWSER_WIDTH 528
#define PROPERTY_BROWSER_HEIGHT 322

#define MAX_ICON_WIDTH 63
#define MAX_ICON_HEIGHT 63
#define COLOR_SQUARE_SIZE 48

#define CONTENT_TABLE_HEIGHT 4

#define ERASE_OBJECT_NAME "erase.png"

enum {
	PROPERTY_TYPE,
};

static GtkTargetEntry drag_types[] = {
	{ "text/uri-list",  0, PROPERTY_TYPE }
};

static NautilusPropertyBrowser *main_browser = NULL;

NAUTILUS_DEFINE_CLASS_BOILERPLATE (NautilusPropertyBrowser,
				   nautilus_property_browser,
				   GTK_TYPE_WINDOW)

/* initializing the class object by installing the operations we override */
static void
nautilus_property_browser_initialize_class (GtkObjectClass *object_klass)
{
	NautilusPropertyBrowserClass *klass;
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (object_klass);

	klass = NAUTILUS_PROPERTY_BROWSER_CLASS (object_klass);

	object_klass->destroy = nautilus_property_browser_destroy;
	widget_class->drag_data_get  = nautilus_property_browser_drag_data_get;
	widget_class->drag_end  = nautilus_property_browser_drag_end;
	widget_class->size_allocate = nautilus_property_browser_size_allocate;
}

/* initialize the instance's fields, create the necessary subviews, etc. */

static void
nautilus_property_browser_initialize (GtkObject *object)
{
 	NautilusPropertyBrowser *property_browser;
 	GtkWidget* widget, *temp_box, *temp_hbox, *temp_frame;
	GtkWidget* temp_button, *temp_label;
	GtkWidget *viewport;
	char *temp_str;
	
	property_browser = NAUTILUS_PROPERTY_BROWSER (object);
	widget = GTK_WIDGET (object);

	property_browser->details = g_new0 (NautilusPropertyBrowserDetails, 1);

	property_browser->details->category = g_strdup ("patterns");
	property_browser->details->category_type = NAUTILUS_PROPERTY_PATTERN;
		
	/* load the chit frame */
	temp_str = nautilus_pixmap_file ("chit_frame.png");
	property_browser->details->property_chit = gdk_pixbuf_new_from_file (temp_str);
	g_free (temp_str);
	
	/* set the initial size of the property browser */
	gtk_widget_set_usize (widget, PROPERTY_BROWSER_WIDTH, PROPERTY_BROWSER_HEIGHT);
	gtk_container_set_border_width (GTK_CONTAINER (widget), 0);				

	/* set the title and standard close accelerator */
	gtk_window_set_title (GTK_WINDOW (widget), _("Backgrounds and Emblems"));
	gtk_window_set_wmclass (GTK_WINDOW (widget), "property_browser", "Nautilus");
	nautilus_gtk_window_set_up_close_accelerator (GTK_WINDOW (widget));
		
	/* create the container box */  
  	property_browser->details->container = GTK_HBOX (gtk_hbox_new (FALSE, 0));
	gtk_container_set_border_width (GTK_CONTAINER (property_browser->details->container), 0);				
	gtk_widget_show (GTK_WIDGET (property_browser->details->container));
	gtk_container_add (GTK_CONTAINER (property_browser),
			   GTK_WIDGET (property_browser->details->container));	

	/* make the category container */
	property_browser->details->category_container = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (property_browser->details->category_container), 0 );				
 	property_browser->details->category_position = -1;	
 	
 	viewport = gtk_viewport_new(NULL, NULL);	
	gtk_widget_show (viewport);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);

	gtk_box_pack_start (GTK_BOX (property_browser->details->container),
			    property_browser->details->category_container, FALSE, FALSE, 0);
	gtk_widget_show (property_browser->details->category_container);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (property_browser->details->category_container),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* allocate a table to hold the category selector */
  	property_browser->details->category_box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width ( GTK_CONTAINER (property_browser->details->category_box), 4);
	gtk_container_add(GTK_CONTAINER(viewport), property_browser->details->category_box); 
	gtk_container_add (GTK_CONTAINER (property_browser->details->category_container), viewport);
	gtk_widget_show (GTK_WIDGET (property_browser->details->category_box));

	/* make the content container vbox */
  	property_browser->details->content_container = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (property_browser->details->content_container);
	gtk_box_pack_start (GTK_BOX (property_browser->details->container),
			    property_browser->details->content_container,
			    TRUE, TRUE, 0);
	
  	/* create the title box */
  	property_browser->details->title_box = gtk_event_box_new();
	gtk_container_set_border_width (GTK_CONTAINER (property_browser->details->title_box), 0);				
 	
  	gtk_widget_show(property_browser->details->title_box);
	gtk_box_pack_start (GTK_BOX(property_browser->details->content_container),
			    property_browser->details->title_box,
			    FALSE, FALSE, 0);
  	
  	temp_frame = gtk_frame_new(NULL);
  	gtk_frame_set_shadow_type(GTK_FRAME(temp_frame), GTK_SHADOW_NONE);
  	gtk_widget_show(temp_frame);
  	gtk_container_add(GTK_CONTAINER(property_browser->details->title_box), temp_frame);
  	
  	temp_hbox = gtk_hbox_new(FALSE, 0);
  	gtk_widget_show(temp_hbox);
 	gtk_container_set_border_width (GTK_CONTAINER (temp_hbox), 4);				
 
  	gtk_container_add(GTK_CONTAINER(temp_frame), temp_hbox);
 	
	/* add the title label */
	property_browser->details->title_label = nautilus_label_new ("");
	nautilus_label_make_larger (NAUTILUS_LABEL (property_browser->details->title_label), 4);
	nautilus_label_make_bold   (NAUTILUS_LABEL (property_browser->details->title_label));
 	
	gtk_widget_show(property_browser->details->title_label);
	gtk_box_pack_start (GTK_BOX(temp_hbox), property_browser->details->title_label, FALSE, FALSE, 8);
 
 	/* add the help label */
	property_browser->details->help_label = nautilus_label_new  ("");
	gtk_widget_show(property_browser->details->help_label);
	nautilus_label_make_smaller (NAUTILUS_LABEL (property_browser->details->help_label), 2);
	gtk_box_pack_end (GTK_BOX(temp_hbox), property_browser->details->help_label, FALSE, FALSE, 8);
 	 	
  	/* add the bottom box to hold the command buttons */
  	temp_box = gtk_event_box_new();
	gtk_container_set_border_width (GTK_CONTAINER (temp_box), 0);				
  	gtk_widget_show(temp_box);

  	temp_frame = gtk_frame_new(NULL);
  	gtk_frame_set_shadow_type(GTK_FRAME(temp_frame), GTK_SHADOW_NONE);
  	gtk_widget_show(temp_frame);
  	gtk_container_add(GTK_CONTAINER(temp_box), temp_frame);

  	property_browser->details->bottom_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (property_browser->details->bottom_box);
	gtk_container_set_border_width (GTK_CONTAINER (property_browser->details->bottom_box), 4);
	
	gtk_box_pack_end (GTK_BOX(property_browser->details->content_container), temp_box, FALSE, FALSE, 0);
  	gtk_container_add (GTK_CONTAINER (temp_frame), property_browser->details->bottom_box);
  	
  	/* create the "done" button */
 	temp_button = gtk_button_new ();
	gtk_widget_show(temp_button);
	
	temp_label = gtk_label_new (_("  Done  "));
	gtk_widget_show(temp_label);
	gtk_container_add (GTK_CONTAINER(temp_button), temp_label);
	gtk_box_pack_end (GTK_BOX(property_browser->details->bottom_box), temp_button, FALSE, FALSE, 4);  
 	gtk_signal_connect(GTK_OBJECT (temp_button), "clicked", GTK_SIGNAL_FUNC (done_button_callback), property_browser);
  	
  	/* create the "add new" button */
  	property_browser->details->add_button = gtk_button_new ();
	gtk_widget_show(property_browser->details->add_button);
	
	property_browser->details->add_button_label = gtk_label_new (_("  Add new...  "));
	gtk_widget_show(property_browser->details->add_button_label);
	gtk_container_add (GTK_CONTAINER(property_browser->details->add_button),
			   property_browser->details->add_button_label);
	gtk_box_pack_end (GTK_BOX(property_browser->details->bottom_box),
			  property_browser->details->add_button, FALSE, FALSE, 4);
 	  
 	gtk_signal_connect(GTK_OBJECT (property_browser->details->add_button), "clicked",
			   add_new_button_callback, property_browser);
	
	/* now create the "remove" button */
  	property_browser->details->remove_button = gtk_button_new();
	gtk_widget_show(property_browser->details->remove_button);
	
	property_browser->details->remove_button_label = gtk_label_new (_("  Remove...  "));	
	gtk_widget_show(property_browser->details->remove_button_label);
	gtk_container_add (GTK_CONTAINER(property_browser->details->remove_button),
			   property_browser->details->remove_button_label);
	gtk_box_pack_end (GTK_BOX (property_browser->details->bottom_box),
			  property_browser->details->remove_button,
			  FALSE,
			  FALSE,
			  4);
	
 	gtk_signal_connect (GTK_OBJECT (property_browser->details->remove_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (remove_button_callback),
			    property_browser);

	/* now create the actual content, with the category pane and the content frame */	
	
	/* the actual contents are created when necessary */	
  	property_browser->details->content_frame = NULL;

	/* add callback for preference changes */
	nautilus_preferences_add_callback (NAUTILUS_PREFERENCES_CAN_ADD_CONTENT, 
					   (NautilusPreferencesCallback) nautilus_property_browser_preferences_changed, 
					   property_browser);
	
	/* add a callback for when the theme changes */
	nautilus_preferences_add_callback (NAUTILUS_PREFERENCES_THEME, 
					   nautilus_property_browser_theme_changed,
					   property_browser);	
	
	gtk_signal_connect (GTK_OBJECT (property_browser), "delete_event",
                    	    GTK_SIGNAL_FUNC (nautilus_property_browser_delete_event_callback),
                    	    NULL);

	gtk_signal_connect (GTK_OBJECT (property_browser), "hide",
                    	    nautilus_property_browser_hide_callback,
                    	    NULL);

	/* initially, display the top level */
	nautilus_property_browser_set_path(property_browser, BROWSER_CATEGORIES_FILE_NAME);

	/* Register that things may be dragged from this window */
	nautilus_drag_window_register (GTK_WINDOW (property_browser));
}

static void
nautilus_property_browser_destroy (GtkObject *object)
{
	NautilusPropertyBrowser *property_browser;

	property_browser = NAUTILUS_PROPERTY_BROWSER (object);
	
	g_free (property_browser->details->path);
	g_free (property_browser->details->category);
	g_free (property_browser->details->dragged_file);
	g_free (property_browser->details->drag_type);

	nautilus_g_list_free_deep (property_browser->details->keywords);
		
	if (property_browser->details->property_chit) {
		gdk_pixbuf_unref (property_browser->details->property_chit);
	}
	
	g_free (property_browser->details);
	
	nautilus_preferences_remove_callback(NAUTILUS_PREFERENCES_CAN_ADD_CONTENT,
						(NautilusPreferencesCallback) nautilus_property_browser_preferences_changed, 
						NULL);
	nautilus_preferences_remove_callback (NAUTILUS_PREFERENCES_THEME,
					      nautilus_property_browser_theme_changed,
					      property_browser);
	if (object == GTK_OBJECT (main_browser))
		main_browser = NULL;
		
	NAUTILUS_CALL_PARENT_CLASS (GTK_OBJECT_CLASS, destroy, (object));

}

/* create a new instance */
NautilusPropertyBrowser *
nautilus_property_browser_new (void)
{
	NautilusPropertyBrowser *browser;

	browser = NAUTILUS_PROPERTY_BROWSER
		(gtk_widget_new (nautilus_property_browser_get_type (), NULL));
	
	gtk_container_set_border_width (GTK_CONTAINER (browser), 0);
  	gtk_window_set_policy (GTK_WINDOW(browser), TRUE, TRUE, FALSE);
  	gtk_widget_show (GTK_WIDGET(browser));
	
	return browser;
}

/* show the main property browser */

void
nautilus_property_browser_show (void)
{
	if (main_browser == NULL) {
		main_browser = nautilus_property_browser_new ();
	} else {
		nautilus_gtk_window_present (GTK_WINDOW (main_browser));
	}
}

static gboolean
nautilus_property_browser_delete_event_callback (GtkWidget *widget,
					   GdkEvent  *event,
					   gpointer   user_data)
{
	/* Hide but don't destroy */
	gtk_widget_hide(widget);
	return TRUE;
}

static void
nautilus_property_browser_hide_callback (GtkWidget *widget,
					 gpointer   user_data)
{
	cancel_remove_mode (NAUTILUS_PROPERTY_BROWSER (widget));
}

/* remember the name of the dragged file */
static void
nautilus_property_browser_set_dragged_file (NautilusPropertyBrowser *property_browser,
					    const char *dragged_file_name)
{       
	g_free (property_browser->details->dragged_file);
	property_browser->details->dragged_file = g_strdup (dragged_file_name);
}

/* remember the drag type */
static void
nautilus_property_browser_set_drag_type (NautilusPropertyBrowser *property_browser,
					 const char *new_drag_type)
{       
	g_free (property_browser->details->drag_type);
	property_browser->details->drag_type = g_strdup (new_drag_type);
}

/* drag and drop data get handler */

static void
nautilus_property_browser_drag_data_get (GtkWidget *widget,
					 GdkDragContext *context,
					 GtkSelectionData *selection_data,
					 guint info,
					 guint32 time)
{
	char  *image_file_name, *image_file_uri;
	gboolean is_reset;
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER(widget);
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (context != NULL);

	switch (info) {
	case PROPERTY_TYPE:
		/* formulate the drag data based on the drag type.  Eventually, we will
		   probably select the behavior from properties in the category xml definition,
		   but for now we hardwire it to the drag_type */
		
		is_reset = FALSE;
		if (!strcmp(property_browser->details->drag_type, "property/keyword")) {
			char* keyword_str = strip_extension(property_browser->details->dragged_file);
		        gtk_selection_data_set(selection_data, selection_data->target, 8, keyword_str, strlen(keyword_str));
			g_free(keyword_str);
			return;	
		}
		else if (!strcmp(property_browser->details->drag_type, "application/x-color")) {
		        GdkColor color;
			guint16 colorArray[4];
			
			/* handle the "reset" case as an image */
			if (nautilus_strcmp (property_browser->details->dragged_file, RESET_IMAGE_NAME) != 0) {
				gdk_color_parse(property_browser->details->dragged_file, &color);
				colorArray[0] = color.red;
				colorArray[1] = color.green;
				colorArray[2] = color.blue;
				colorArray[3] = 0xffff;
				
				gtk_selection_data_set(selection_data,
				selection_data->target, 16, (const char *) &colorArray[0], 8);
				return;	
			} else {
				is_reset = TRUE;
			}

		}
		
		image_file_name = g_strdup_printf ("%s/%s/%s",
						   NAUTILUS_DATADIR,
						   is_reset ? "patterns" : property_browser->details->category,
						   property_browser->details->dragged_file);
		
		if (!g_file_exists (image_file_name)) {
			char *user_directory;
			g_free (image_file_name);

			user_directory = nautilus_get_user_directory ();
			image_file_name = g_strdup_printf ("%s/%s/%s",
							   user_directory,
							   property_browser->details->category, 
							   property_browser->details->dragged_file);	

			g_free (user_directory);
		}

		image_file_uri = gnome_vfs_get_uri_from_local_path (image_file_name);
		gtk_selection_data_set (selection_data, selection_data->target, 8, image_file_uri, strlen (image_file_uri));
		g_free (image_file_name);
		g_free (image_file_uri);
		
		break;
	default:
		g_assert_not_reached ();
	}
}

/* drag and drop end handler, where we destroy ourselves, since the transaction is complete */

static void
nautilus_property_browser_drag_end (GtkWidget *widget, GdkDragContext *context)
{
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER(widget);
	if (!property_browser->details->keep_around) {
		gtk_widget_hide (GTK_WIDGET (widget));
	}
}

/* utility routine to check if the passed-in uri is an image file */
static gboolean
ensure_uri_is_image (const char *uri)
{	
	gboolean is_image;
	GnomeVFSResult result;
	GnomeVFSFileInfo *file_info;

	file_info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info
		(uri, file_info,
		 GNOME_VFS_FILE_INFO_GET_MIME_TYPE
		 | GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
        is_image = nautilus_istr_has_prefix (file_info->mime_type, "image/") && (nautilus_strcmp (file_info->mime_type, "image/svg") != 0);
	gnome_vfs_file_info_unref (file_info);
	return is_image;
}

/* create the appropriate pixbuf for the passed in file */

static GdkPixbuf *
make_drag_image (NautilusPropertyBrowser *property_browser, const char* file_name)
{
	GdkPixbuf *pixbuf, *orig_pixbuf;
	char *image_file_name;

	image_file_name = g_strdup_printf ("%s/%s/%s",
					   NAUTILUS_DATADIR,
					   property_browser->details->category,
					   file_name);
	
	if (!g_file_exists (image_file_name)) {
		char *user_directory;
		g_free (image_file_name);

		user_directory = nautilus_get_user_directory ();

		image_file_name = g_strdup_printf ("%s/%s/%s",
						   user_directory,
						   property_browser->details->category,
						   file_name);	

		g_free (user_directory);	
	}
	
	orig_pixbuf = gdk_pixbuf_new_from_file (image_file_name);
	
	if (!strcmp(property_browser->details->category, "patterns")) {
		pixbuf = nautilus_customization_make_pattern_chit (orig_pixbuf, property_browser->details->property_chit, TRUE);
	} else {
		pixbuf = nautilus_gdk_pixbuf_scale_down_to_fit (orig_pixbuf, MAX_ICON_WIDTH, MAX_ICON_HEIGHT);
		gdk_pixbuf_unref (orig_pixbuf);
	}

	g_free (image_file_name);

	return pixbuf;
}


/* create a pixbuf and fill it with a color */

static GdkPixbuf*
make_color_drag_image (NautilusPropertyBrowser *property_browser, const char *color_spec, gboolean trim_edges)
{
	GdkPixbuf *color_square;
	int row, col, stride;
	char *pixels, *row_pixels;
	GdkColor color;

	color_square = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE);
	
	gdk_color_parse(color_spec, &color);
	color.red >>= 8;
	color.green >>= 8;
	color.blue >>= 8;
	
	pixels = gdk_pixbuf_get_pixels (color_square);
	stride = gdk_pixbuf_get_rowstride (color_square);
	
	/* loop through and set each pixel */
	for (row = 0; row < COLOR_SQUARE_SIZE; row++) {
		row_pixels =  (pixels + (row * stride));
		for (col = 0; col < COLOR_SQUARE_SIZE; col++) {		
			*row_pixels++ = color.red;
			*row_pixels++ = color.green;
			*row_pixels++ = color.blue;
			*row_pixels++ = 255;
		}
	}
	
	return nautilus_customization_make_pattern_chit (color_square, 
							    property_browser->details->property_chit,
							    trim_edges);	
}

/* this callback handles button presses on the category widget. It maintains the active state */

static void
category_clicked_callback (GtkWidget *widget, char *category_name)
{
	gboolean save_flag;
	NautilusPropertyBrowser *property_browser;
	
	property_browser = NAUTILUS_PROPERTY_BROWSER (gtk_object_get_user_data (GTK_OBJECT (widget)));
	
	/* special case the user clicking on the already selected button, since we don't want that to toggle */
	if (widget == GTK_WIDGET(property_browser->details->selected_button)) {
		if (!property_browser->details->toggle_button_flag)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (property_browser->details->selected_button), TRUE);		
		return;
	}	

	/* exit remove mode when the user switches categories, since there might be nothing to remove
	   in the new category */
	property_browser->details->remove_mode = FALSE;
		
	save_flag = property_browser->details->toggle_button_flag;
	property_browser->details->toggle_button_flag = TRUE;	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (property_browser->details->selected_button), FALSE);
	property_browser->details->toggle_button_flag = save_flag;	
	
	nautilus_property_browser_set_category (property_browser, category_name);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	property_browser->details->selected_button = widget;
}

/* fetch the path of the xml file.  First, try to find it in the home directory, but it
   we can't find it there, try the shared directory */
   
static char *
get_xml_path (NautilusPropertyBrowser *property_browser)
{
	char *xml_path;
	char *user_directory;

	user_directory = nautilus_get_user_directory ();

	/* first try the user's home directory */
	xml_path = nautilus_make_path (user_directory,
				       property_browser->details->path);
	g_free (user_directory);
	if (g_file_exists (xml_path)) {
		return xml_path;
	}
	g_free (xml_path);
	
	/* next try the shared directory */
	xml_path = nautilus_make_path (NAUTILUS_DATADIR,
				       property_browser->details->path);
	if (g_file_exists (xml_path)) {
		return xml_path;
	}
	g_free (xml_path);

	return NULL;
}

static xmlDocPtr
read_browser_xml (NautilusPropertyBrowser *property_browser)
{
	char *path;
	xmlDocPtr document;

	path = get_xml_path (property_browser);
	if (path == NULL) {
		return NULL;
	}
	document = xmlParseFile (path);
	g_free (path);
	return document;
}

static void
write_browser_xml (NautilusPropertyBrowser *property_browser,
		   xmlDocPtr document)
{
	char *user_directory, *path;

	user_directory = nautilus_get_user_directory ();	
	path = nautilus_make_path (user_directory, property_browser->details->path);
	g_free (user_directory);
	xmlSaveFile (path, document);
	g_free (path);
}

static xmlNodePtr
get_color_category (xmlDocPtr document)
{
	return nautilus_xml_get_root_child_by_name_and_property (document, "category", "name", "colors");
}

/* routines to remove specific category types.  First, handle colors */
/* having trouble removing nodes, so instead I'll mark it invisible - eventually this needs to be fixed */

static void
remove_color (NautilusPropertyBrowser *property_browser, const char* color_value)
{
	/* load the local xml file to remove the color */
	xmlDocPtr document;
	xmlNodePtr cur_node, color_node;
	gboolean match;
	char *color_content;
	char *deleted_value;

	document = read_browser_xml (property_browser);
	if (document == NULL) {
		return;
	}

	/* find the colors category */
	cur_node = get_color_category (document);
	if (cur_node != NULL) {
		/* loop through the colors to find one that matches */
		for (color_node = nautilus_xml_get_children (cur_node);
		     color_node != NULL;
		     color_node = color_node->next) {
			color_content = xmlNodeGetContent(color_node);
			match = color_content != NULL
				&& strcmp (color_content, color_value) == 0;
			xmlFree (color_content);

			deleted_value = xmlGetProp (color_node, "deleted");
			xmlFree (deleted_value);
			
			if (match && deleted_value == NULL) {
				xmlSetProp(color_node, "deleted", "1");
				write_browser_xml (property_browser, document);
				break;
			}
		}
	}
	
	xmlFreeDoc (document);
}

/* remove the pattern matching the passed in name */

static void
remove_pattern(NautilusPropertyBrowser *property_browser, const char* pattern_name)
{
	char *pattern_path, *pattern_uri;
	char *user_directory;

	user_directory = nautilus_get_user_directory ();

	/* build the pathname of the pattern */
	pattern_path = g_strdup_printf ("%s/patterns/%s",
					   user_directory,
					   pattern_name);
	pattern_uri = gnome_vfs_get_uri_from_local_path (pattern_path);
	g_free (pattern_path);

	g_free (user_directory);	

	/* delete the pattern from the pattern directory */
	if (gnome_vfs_unlink (pattern_uri) != GNOME_VFS_OK) {
		char *message = g_strdup_printf (_("Sorry, but pattern %s couldn't be deleted."), pattern_name);
		nautilus_error_dialog (message, _("Couldn't delete pattern"), GTK_WINDOW (property_browser));
		g_free (message);
	}
	
	g_free (pattern_uri);
}

/* remove the emblem matching the passed in name */

static void
remove_emblem (NautilusPropertyBrowser *property_browser, const char* emblem_name)
{
	/* build the pathname of the emblem */
	char *emblem_path, *emblem_uri;
	char *user_directory;

	user_directory = nautilus_get_user_directory ();

	emblem_path = g_strdup_printf ("%s/emblems/%s",
				       user_directory,
				       emblem_name);
	emblem_uri = gnome_vfs_get_uri_from_local_path (emblem_path);
	g_free (emblem_path);

	g_free (user_directory);

	/* delete the emblem from the emblem directory */
	if (gnome_vfs_unlink (emblem_uri) != GNOME_VFS_OK) {
		char *message = g_strdup_printf (_("Sorry, but emblem %s couldn't be deleted."), emblem_name);
		nautilus_error_dialog (message, _("Couldn't delete pattern"), GTK_WINDOW (property_browser));
		g_free (message);
	}
	else {
		emit_emblems_changed_signal ();
	}
	g_free (emblem_uri);
}

/* handle removing the passed in element */

static void
nautilus_property_browser_remove_element (NautilusPropertyBrowser *property_browser, const char* element_name)
{
	/* lookup category and get mode, then case out and handle the modes */
	switch (property_browser->details->category_type) {
	case NAUTILUS_PROPERTY_PATTERN:
		remove_pattern (property_browser, element_name);
		break;
	case NAUTILUS_PROPERTY_COLOR:
		remove_color (property_browser, element_name);
		break;
	case NAUTILUS_PROPERTY_EMBLEM:
		remove_emblem (property_browser, element_name);
		break;
	default:
		break;
	}
}

/* Callback used when the color selection dialog is destroyed */
static gboolean
dialog_destroy (GtkWidget *widget, gpointer data)
{
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER(data);
	property_browser->details->dialog = NULL;
	return FALSE;
}

/* utility to set up the emblem image from the passed-in file */

static void
set_emblem_image_from_file (NautilusPropertyBrowser *property_browser)
{
	GdkPixbuf *pixbuf;
	GdkPixbuf *scaled_pixbuf;

	pixbuf = gdk_pixbuf_new_from_file (property_browser->details->image_path);			
	scaled_pixbuf = nautilus_gdk_pixbuf_scale_down_to_fit (pixbuf, MAX_ICON_WIDTH, MAX_ICON_HEIGHT);			
	gdk_pixbuf_unref (pixbuf);
	
	if (property_browser->details->emblem_image == NULL) {
		property_browser->details->emblem_image = nautilus_image_new (NULL);
		gtk_widget_show(property_browser->details->emblem_image);
	} 
	nautilus_image_set_pixbuf (NAUTILUS_IMAGE (property_browser->details->emblem_image), scaled_pixbuf);
	gdk_pixbuf_unref (scaled_pixbuf);
}

/* this callback is invoked when a file is selected by the file selection */
static void
emblem_image_file_changed (GtkWidget *entry, NautilusPropertyBrowser *property_browser)
{
	char *new_uri;

	new_uri = gnome_vfs_get_uri_from_local_path (gtk_entry_get_text (GTK_ENTRY(entry)));
	if (!ensure_uri_is_image (new_uri)) {
		char *message = g_strdup_printf
			(_("Sorry, but '%s' is not a usable image file!"),
			 gtk_entry_get_text(GTK_ENTRY(entry)));
		nautilus_error_dialog (message, _("Not an Image"), GTK_WINDOW (property_browser));
		g_free (message);
		
		gtk_entry_set_text(GTK_ENTRY(entry), property_browser->details->image_path);
		g_free(new_uri);
		return;
	}
	
	g_free (new_uri);
	g_free (property_browser->details->image_path);
	property_browser->details->image_path = gtk_entry_get_text(GTK_ENTRY(entry));
	if (property_browser->details->image_path)
		property_browser->details->image_path = g_strdup(property_browser->details->image_path);
	
	/* set up the pixmap in the dialog */
	
	set_emblem_image_from_file(property_browser);
}

/* here's where we create the emblem dialog */

static GtkWidget*
nautilus_emblem_dialog_new (NautilusPropertyBrowser *property_browser)
{
	GtkWidget *widget, *entry;
	GtkWidget *dialog = gnome_dialog_new(_("Create a New Emblem:"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
	GtkWidget *table = gtk_table_new(2, 2, FALSE);

	/* make the keyword label and field */	
	
	widget = gtk_label_new(_("Keyword:"));
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 4);
	
  	property_browser->details->keyword = gtk_entry_new_with_max_length (24);
	gtk_widget_show(property_browser->details->keyword);
	gtk_table_attach(GTK_TABLE(table), property_browser->details->keyword, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 4, 4);

	/* default image is the generic emblem */
	g_free(property_browser->details->image_path);
		
	property_browser->details->image_path = nautilus_pixmap_file ("emblem-generic.png"); 
	property_browser->details->emblem_image = NULL; /* created lazily by set_emblem_image */
	set_emblem_image_from_file(property_browser);
	gtk_table_attach(GTK_TABLE(table), property_browser->details->emblem_image, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 4);
 
	/* set up a gnome file entry to pick the image file */
	property_browser->details->file_entry = gnome_file_entry_new ("nautilus", _("Select an image file for the new emblem:"));
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(property_browser->details->file_entry), property_browser->details->image_path);
	
	gtk_widget_show(property_browser->details->file_entry);
	gtk_table_attach(GTK_TABLE(table), property_browser->details->file_entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 4, 4);
	
	/* connect to the activate signal of the entry to change images */
	entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY(property_browser->details->file_entry));
	gtk_entry_set_text(GTK_ENTRY(entry), property_browser->details->image_path);
	
	gtk_signal_connect (GTK_OBJECT (entry), "activate", (GtkSignalFunc) emblem_image_file_changed, property_browser);
		
	/* install the table in the dialog */
	
	gtk_widget_show(table);	
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, TRUE, TRUE, GNOME_PAD);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), GNOME_OK);
	gtk_window_set_wmclass(GTK_WINDOW(dialog), "emblem_dialog", "Nautilus");
	
	return dialog;
}

/* create the color selection dialog */

static GtkWidget*
nautilus_color_selection_dialog_new (NautilusPropertyBrowser *property_browser)
{
	GtkWidget *widget;
	GtkWidget *dialog = gnome_dialog_new(_("Create a New Color:"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
	GtkWidget *table = gtk_table_new(2, 2, FALSE);

	/* make the name label and field */	
	
	widget = gtk_label_new(_("Color name:"));
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 4);
	
  	property_browser->details->color_name = gtk_entry_new_with_max_length (24);
	gtk_widget_show(property_browser->details->color_name);
	gtk_table_attach(GTK_TABLE(table), property_browser->details->color_name, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 4, 4);

	/* default image is the generic emblem */
	g_free(property_browser->details->image_path);
		
	widget = gtk_label_new(_("Color value:"));
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 4);
 
	/* set up a gnome file entry to pick the image file */
	property_browser->details->color_picker = gnome_color_picker_new ();
	gtk_widget_show (property_browser->details->color_picker);
	
	gtk_widget_show(property_browser->details->color_picker);
	gtk_table_attach(GTK_TABLE(table), property_browser->details->color_picker, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 4, 4);
		
	/* install the table in the dialog */
	
	gtk_widget_show(table);	
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, TRUE, TRUE, GNOME_PAD);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), GNOME_OK);
	
	return dialog;
}

/* add the newly selected file to the browser images */
static void
add_pattern_to_browser (GtkWidget *widget, gpointer *data)
{
	gboolean is_image;
	char *directory_path, *source_file_name, *destination_name;
	char *path_uri, *basename;
	char *user_directory;	
	char *directory_uri;
	GnomeVFSResult result;
	
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER(data);

	/* get the file path from the file selection widget */
	char *path_name = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (property_browser->details->dialog)));
	
	gtk_widget_destroy (property_browser->details->dialog);
	property_browser->details->dialog = NULL;

	/* fetch the mime type and make sure that the file is an image */
	path_uri = gnome_vfs_get_uri_from_local_path (path_name);	

	/* don't allow the user to change the reset image */
	basename = nautilus_uri_get_basename (path_uri);
	if (basename && nautilus_strcmp (basename, RESET_IMAGE_NAME) == 0) {
		nautilus_error_dialog (_("Sorry, but you can't replace the reset image."), _("Not an Image"), NULL);
		g_free (path_name);
		g_free (path_uri);
		g_free (basename);
		return;
	}
		
	is_image = ensure_uri_is_image(path_uri);
	g_free(path_uri);	
	g_free (basename);
	
	if (!is_image) {
		char *message = g_strdup_printf (_("Sorry, but '%s' is not a usable image file!"), path_name);
		nautilus_error_dialog (message, _("Not an Image"), NULL);
		g_free (message);
		g_free (path_name);
		return;
	}

	user_directory = nautilus_get_user_directory ();

	/* copy the image file to the patterns directory */
	directory_path = nautilus_make_path (user_directory, property_browser->details->category);
	g_free (user_directory);
	source_file_name = strrchr (path_name, '/');
	destination_name = nautilus_make_path (directory_path, source_file_name + 1);
	
	/* make the directory if it doesn't exist */
	if (!g_file_exists(directory_path)) {
		directory_uri = gnome_vfs_get_uri_from_local_path (directory_path);
		gnome_vfs_make_directory (directory_uri,
					  GNOME_VFS_PERM_USER_ALL
					  | GNOME_VFS_PERM_GROUP_ALL
					  | GNOME_VFS_PERM_OTHER_READ);
		g_free (directory_uri);
	}
	
	g_free(directory_path);
	
	result = nautilus_copy_uri_simple (path_name, destination_name);		
	if (result != GNOME_VFS_OK) {
		char *message = g_strdup_printf (_("Sorry, but the pattern %s couldn't be installed."), path_name);
		nautilus_error_dialog (message, _("Couldn't install pattern"), GTK_WINDOW (property_browser));
		g_free (message);
	}
				
	g_free(path_name);	
	g_free(destination_name);
	
	/* update the property browser's contents to show the new one */
	nautilus_property_browser_update_contents(property_browser);
}

/* here's where we initiate adding a new pattern by putting up a file selector */

static void
add_new_pattern (NautilusPropertyBrowser *property_browser)
{
	if (property_browser->details->dialog) {
		gtk_widget_show(property_browser->details->dialog);
		if (property_browser->details->dialog->window) {
			gdk_window_raise(property_browser->details->dialog->window);
		}
	} else {
		GtkFileSelection *file_dialog;

		property_browser->details->dialog = gtk_file_selection_new
			(_("Select an image file to add as a pattern:"));
		file_dialog = GTK_FILE_SELECTION (property_browser->details->dialog);
		
		gtk_signal_connect (GTK_OBJECT (property_browser->details->dialog),
				    "destroy",
				    (GtkSignalFunc) dialog_destroy,
				    property_browser);
		gtk_signal_connect (GTK_OBJECT (file_dialog->ok_button),
				    "clicked",
				    add_pattern_to_browser,
				    property_browser);
		gtk_signal_connect_object (GTK_OBJECT (file_dialog->cancel_button),
					   "clicked",
					   gtk_widget_destroy,
					   GTK_OBJECT (file_dialog));

		gtk_window_set_position (GTK_WINDOW (file_dialog), GTK_WIN_POS_MOUSE);
		gtk_window_set_transient_for (GTK_WINDOW (file_dialog), GTK_WINDOW (property_browser));
 		gtk_window_set_wmclass (GTK_WINDOW (file_dialog), "file_selector", "Nautilus");
		gtk_widget_show (GTK_WIDGET (file_dialog));
	}
}

/* here's where we add the passed in color to the file that defines the colors */

static void
add_color_to_file (NautilusPropertyBrowser *property_browser, const char *color_spec, const char *color_name)
{
	xmlNodePtr cur_node, new_color_node;
	xmlDocPtr document;

	document = read_browser_xml (property_browser);
	if (document == NULL) {
		return;
	}

	/* find the colors category */
	cur_node = get_color_category (document);
	if (cur_node != NULL) {
		/* add a new color node */
		new_color_node = xmlNewChild (cur_node, NULL, "color", NULL);
		xmlNodeSetContent (new_color_node, color_spec);
		xmlSetProp (new_color_node, "local", "1");
		xmlSetProp (new_color_node, "name", color_name);
		
		write_browser_xml (property_browser, document);
	}
	
	xmlFreeDoc (document);
}

/* handle the OK button being pushed on the color selection dialog */
static void
add_color_to_browser (GtkWidget *widget, int which_button, gpointer *data)
{
	char *color_spec;
	char *color_name, *stripped_color_name;
	
	gdouble color[4];
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER (data);

	if (which_button == GNOME_OK) {	
		gnome_color_picker_get_d (GNOME_COLOR_PICKER (property_browser->details->color_picker), &color[0], &color[1], &color[2], &color[3]);		
		color_spec = g_strdup_printf
			("rgb:%04hX/%04hX/%04hX",
		 	(gushort) (color[0] * 65535.0 + 0.5),
		 	(gushort) (color[1] * 65535.0 + 0.5),
		 	(gushort) (color[2] * 65535.0 + 0.5));

		color_name = gtk_entry_get_text (GTK_ENTRY (property_browser->details->color_name));
		stripped_color_name = g_strstrip (g_strdup (color_name));
		if (strlen (stripped_color_name) == 0) {
			nautilus_error_dialog (_("Sorry, but you must specify a non-blank name for the new color."), 
						_("Couldn't install color"), GTK_WINDOW (property_browser));
		
		} else {
			add_color_to_file (property_browser, color_spec, stripped_color_name);
			nautilus_property_browser_update_contents(property_browser);
		}
		g_free (stripped_color_name);
		g_free(color_spec);	
	} 
	
	gtk_widget_destroy(property_browser->details->dialog);
	property_browser->details->dialog = NULL;
}

/* create the color selection dialog, pre-set with the color that was just selected */
static void
show_color_selection_window (GtkWidget *widget, gpointer *data)
{
	gdouble color[4];
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER(data);

	gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (property_browser->details->dialog)->colorsel), color);
	gtk_widget_destroy (property_browser->details->dialog);

	/* allocate a new color selection dialog */
	property_browser->details->dialog = nautilus_color_selection_dialog_new (property_browser);		

	/* set the color to the one picked by the selector */
	gnome_color_picker_set_d (GNOME_COLOR_PICKER (property_browser->details->color_picker), color[0], color[1], color[2], 1.0);
	
	/* connect the signals to the new dialog */
	
	gtk_signal_connect (GTK_OBJECT (property_browser->details->dialog),
				"destroy",
				(GtkSignalFunc) dialog_destroy, property_browser);
	gtk_signal_connect (GTK_OBJECT (property_browser->details->dialog),
				 "clicked",
				 (GtkSignalFunc) add_color_to_browser, property_browser);
	gtk_window_set_position (GTK_WINDOW (property_browser->details->dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (GTK_WIDGET(property_browser->details->dialog));

}


/* here's the routine to add a new color, by putting up a color selector */

static void
add_new_color (NautilusPropertyBrowser *property_browser)
{
	if (property_browser->details->dialog) {
		gtk_widget_show(property_browser->details->dialog);
		if (property_browser->details->dialog->window)
			gdk_window_raise(property_browser->details->dialog->window);

	} else {
		GtkColorSelectionDialog *color_dialog;

		property_browser->details->dialog = gtk_color_selection_dialog_new (_("Select a color to add:"));
		color_dialog = GTK_COLOR_SELECTION_DIALOG (property_browser->details->dialog);
		
		gtk_signal_connect (GTK_OBJECT (property_browser->details->dialog),
				    "destroy",
				    (GtkSignalFunc) dialog_destroy, property_browser);
		gtk_signal_connect (GTK_OBJECT (color_dialog->ok_button),
				    "clicked",
				    (GtkSignalFunc) show_color_selection_window, property_browser);
		gtk_signal_connect_object (GTK_OBJECT (color_dialog->cancel_button),
					   "clicked",
					   (GtkSignalFunc) gtk_widget_destroy,
					   GTK_OBJECT (color_dialog));
		gtk_widget_hide(color_dialog->help_button);

		gtk_window_set_position (GTK_WINDOW (color_dialog), GTK_WIN_POS_MOUSE);
		gtk_widget_show (GTK_WIDGET(color_dialog));
	}
}

/* utility to make sure the passed-in keyword only contains alphanumeric characters */
static gboolean
emblem_keyword_valid (const char *keyword)
{
	int index, keyword_length;

	keyword_length = strlen (keyword);
	for (index = 0; index < keyword_length; index++) {
		if (!isalnum (keyword[index]) && !isspace (keyword[index])) {
			return FALSE;
		}
	}
	
	return TRUE;
}


/* check for reserved keywords */
static gboolean
is_reserved_keyword (NautilusPropertyBrowser *property_browser, const char *keyword)
{	
	/* check intrinsic emblems */
	if (nautilus_strcasecmp (keyword, NAUTILUS_FILE_EMBLEM_NAME_TRASH) == 0) {
		return TRUE;
	}
	if (nautilus_strcasecmp (keyword, NAUTILUS_FILE_EMBLEM_NAME_CANT_READ) == 0) {
		return TRUE;
	}
	if (nautilus_strcasecmp (keyword, NAUTILUS_FILE_EMBLEM_NAME_CANT_WRITE) == 0) {
		return TRUE;
	}
	
	/* see if the keyword already exists */
	return g_list_find_custom (property_browser->details->keywords,
				   (char *) keyword,
				   (GCompareFunc) nautilus_strcasecmp) != NULL;				
}

/* here's where we handle clicks in the emblem dialog buttons */
static void
emblem_dialog_clicked (GtkWidget *dialog, int which_button, NautilusPropertyBrowser *property_browser)
{
	char *directory_uri, *error_string;
	GnomeVFSResult result;
	
	if (which_button == GNOME_OK) {
		char *destination_name, *extension;
		char *new_keyword, *stripped_keyword;
		char *emblem_path, *emblem_uri;
		char *user_directory;	
		char *directory_path;

		/* update the image path from the file entry */
		if (property_browser->details->file_entry) {
			emblem_path = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (property_browser->details->file_entry),
									FALSE);
			if (emblem_path) {
				emblem_uri = gnome_vfs_get_uri_from_local_path (emblem_path);
				if (ensure_uri_is_image (emblem_uri)) {
					g_free (property_browser->details->image_path);
					property_browser->details->image_path = emblem_path;				
				} else {
					char *message = g_strdup_printf
						(_("Sorry, but '%s' is not a usable image file!"), emblem_path);
					nautilus_error_dialog (message, _("Not an Image"), GTK_WINDOW (property_browser));
					g_free (message);
					g_free (emblem_path);
					return;
				}
				g_free (emblem_uri);
			}
		}
		
		new_keyword = gtk_entry_get_text(GTK_ENTRY(property_browser->details->keyword));		
		if (new_keyword == NULL) {
			stripped_keyword = NULL;
		} else {
			stripped_keyword = g_strstrip (g_strdup (new_keyword));
		}
		
		if (stripped_keyword == NULL || strlen (stripped_keyword) == 0) {
			nautilus_error_dialog (_("Sorry, but you must specify a non-blank keyword for the new emblem."), 
						_("Couldn't install emblem"), GTK_WINDOW (property_browser));
		} else if (!emblem_keyword_valid (stripped_keyword)) {
			nautilus_error_dialog (_("Sorry, but emblem keywords can only contain letters, spaces and numbers."), 
						_("Couldn't install emblem"), GTK_WINDOW (property_browser));
		} else if (is_reserved_keyword (property_browser, stripped_keyword)) {
			error_string = g_strdup_printf (_("Sorry, but \"%s\" is an existing keyword.  Please choose a different name for it."), stripped_keyword);
			nautilus_error_dialog (error_string, 
						_("Couldn't install emblem"), GTK_WINDOW (property_browser));
			g_free (error_string);
		} else {		
			user_directory = nautilus_get_user_directory ();

			/* get the path for emblems in the user's home directory */
			directory_path = nautilus_make_path (user_directory, property_browser->details->category);
			g_free (user_directory);

			/* make the directory if it doesn't exist */
			if (!g_file_exists (directory_path)) {
				directory_uri = gnome_vfs_get_uri_from_local_path (directory_path);
				gnome_vfs_make_directory(directory_uri,
						 	GNOME_VFS_PERM_USER_ALL
						 	| GNOME_VFS_PERM_GROUP_ALL
						 	| GNOME_VFS_PERM_OTHER_READ);
				g_free(directory_uri);
			}

			/* formulate the destination file name */
			extension = strrchr(property_browser->details->image_path, '.');
			destination_name = g_strdup_printf("%s/%s.%s", directory_path, stripped_keyword, extension + 1);
			g_free(directory_path);
				
			/* perform the actual copy */
			result = nautilus_copy_uri_simple (property_browser->details->image_path, destination_name);		
		
			if (result != GNOME_VFS_OK) {
				char *message = g_strdup_printf (_("Sorry, but the image at %s couldn't be installed as an emblem."), property_browser->details->image_path);
				nautilus_error_dialog (message, _("Couldn't install emblem"), GTK_WINDOW (property_browser));
				g_free (message);
			} else {
				emit_emblems_changed_signal ();	
			}
			
			g_free(destination_name);
				
			nautilus_property_browser_update_contents(property_browser);
		}
		g_free (stripped_keyword);
	}
	
	gtk_widget_destroy(dialog);
	
	property_browser->details->keyword = NULL;
	property_browser->details->emblem_image = NULL;
	property_browser->details->file_entry = NULL;
}

/* here's the routine to add a new emblem, by putting up an emblem dialog */

static void
add_new_emblem (NautilusPropertyBrowser *property_browser)
{
	if (property_browser->details->dialog) {
		gtk_widget_show (property_browser->details->dialog);
		if (property_browser->details->dialog->window) {
			gdk_window_raise (property_browser->details->dialog->window);
		}
	} else {
		property_browser->details->dialog = nautilus_emblem_dialog_new (property_browser);		
		gtk_signal_connect (GTK_OBJECT (property_browser->details->dialog),
				    "destroy",
				    (GtkSignalFunc) dialog_destroy, property_browser);
		gtk_signal_connect (GTK_OBJECT (property_browser->details->dialog),
				    "clicked",
				    (GtkSignalFunc) emblem_dialog_clicked, property_browser);
		gtk_window_set_position (GTK_WINDOW (property_browser->details->dialog), GTK_WIN_POS_MOUSE);
		gtk_widget_show (GTK_WIDGET(property_browser->details->dialog));
	}
}

/* cancelremove mode */
static void
cancel_remove_mode (NautilusPropertyBrowser *property_browser)
{
	if (property_browser->details->remove_mode) {
		property_browser->details->remove_mode = FALSE;
		nautilus_property_browser_update_contents(property_browser);
		gtk_widget_show (property_browser->details->help_label);
	}
}

/* handle the add_new button */

static void
add_new_button_callback(GtkWidget *widget, NautilusPropertyBrowser *property_browser)
{
	/* handle remove mode, where we act as a cancel button */
	if (property_browser->details->remove_mode) {
		cancel_remove_mode (property_browser);
		return;
	}

	switch (property_browser->details->category_type) {
		case NAUTILUS_PROPERTY_PATTERN:
			add_new_pattern (property_browser);
			break;
		case NAUTILUS_PROPERTY_COLOR:
			add_new_color (property_browser);
			break;
		case NAUTILUS_PROPERTY_EMBLEM:
			add_new_emblem (property_browser);
			break;
		default:
			break;
	}	
}

/* handle the "done" button */
static void
done_button_callback (GtkWidget *widget, GtkWidget *property_browser)
{
	cancel_remove_mode (NAUTILUS_PROPERTY_BROWSER (property_browser));
	gtk_widget_hide (property_browser);
}

/* handle the "remove" button */
static void
remove_button_callback(GtkWidget *widget, NautilusPropertyBrowser *property_browser)
{
	if (property_browser->details->remove_mode) {
		return;
	}
	
	property_browser->details->remove_mode = TRUE;
	gtk_widget_hide (property_browser->details->help_label);
	nautilus_property_browser_update_contents(property_browser);	
}

/* this callback handles clicks on the image or color based content content elements */

static void
element_clicked_callback (GtkWidget *widget, GdkEventButton *event, char *element_name)
{
	GtkTargetList *target_list;	
	GdkDragContext *context;
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap_for_dragged_file;
	GdkBitmap *mask_for_dragged_file;
	int x_delta, y_delta;
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER(gtk_object_get_user_data(GTK_OBJECT(widget)));
	
	/* handle remove mode by removing the element */
	if (property_browser->details->remove_mode) {
		nautilus_property_browser_remove_element(property_browser, element_name);
		property_browser->details->remove_mode = FALSE;
		nautilus_property_browser_update_contents(property_browser);
		gtk_widget_show (property_browser->details->help_label);
		return;
	}
	
	/* set up the drag and drop type corresponding to the category */
	drag_types[0].target = property_browser->details->drag_type;
	
	/* treat the reset property in the colors section specially */	
	if (strcmp (property_browser->details->drag_type, "application/x-color") == 0 &&
		nautilus_strcmp (element_name, RESET_IMAGE_NAME) == 0) {
		drag_types[0].target = "property/bgimage";	
	}
	
	target_list = gtk_target_list_new (drag_types, NAUTILUS_N_ELEMENTS (drag_types));	
	nautilus_property_browser_set_dragged_file(property_browser, element_name);
	
	context = gtk_drag_begin (GTK_WIDGET (property_browser),
				  target_list,
				  GDK_ACTION_MOVE | GDK_ACTION_COPY,
				  event->button,
				  (GdkEvent *) event);

	/* compute the offsets for dragging */
	
	x_delta = floor(event->x + .5);
	y_delta = floor(event->y + .5) ;
	
	if (strcmp(drag_types[0].target, "application/x-color")) {
		/*it's not a color, so, for now, it must be an image */
		/* fiddle with the category to handle the "reset" case properly */
		char * save_category = property_browser->details->category;
		if (nautilus_strcmp (property_browser->details->category, "colors") == 0) {
			property_browser->details->category = "patterns";
		}
		pixbuf = make_drag_image (property_browser, element_name);
		property_browser->details->category = save_category;
		
		x_delta -= (widget->allocation.width - gdk_pixbuf_get_width (pixbuf)) >> 1;
		y_delta -= (widget->allocation.height - gdk_pixbuf_get_height (pixbuf)) >> 1;

	} else {
		pixbuf = make_color_drag_image (property_browser, element_name, TRUE);
	}

        /* set the pixmap and mask for dragging */       
	if (pixbuf != NULL) {
		gdk_pixbuf_render_pixmap_and_mask
			(pixbuf,
			 &pixmap_for_dragged_file,
			 &mask_for_dragged_file,
			 NAUTILUS_STANDARD_ALPHA_THRESHHOLD);

		gdk_pixbuf_unref (pixbuf);	
		gtk_drag_set_icon_pixmap
			(context,
			 gtk_widget_get_colormap (GTK_WIDGET (property_browser)),
			 pixmap_for_dragged_file,
			 mask_for_dragged_file,
			 x_delta, y_delta);
	}
	
	/* optionally (if the shift key is down) hide the property browser - it will later be destroyed when the drag ends */	
	property_browser->details->keep_around = (event->state & GDK_SHIFT_MASK) == 0;
	if (!property_browser->details->keep_around)
		gtk_widget_hide(GTK_WIDGET(property_browser));
}


/* utility routine to strip the extension from the passed in string */
static char*
strip_extension (const char* string_to_strip)
{
	char *result_str, *temp_str;
	if (string_to_strip == NULL)
		return NULL;
	
	result_str = g_strdup(string_to_strip);
	temp_str = strrchr(result_str, '.');
	if (temp_str)
		*temp_str = '\0';
	return result_str;
}

/* handle preferences changing by updating the browser contents */

static void
nautilus_property_browser_preferences_changed (NautilusPropertyBrowser *property_browser)
{
	nautilus_property_browser_update_contents(property_browser);
}

/* utility routine to add the passed-in widget to the content table */

static void
add_to_content_table (NautilusPropertyBrowser *property_browser, GtkWidget* widget, int position, int padding)
{
	int column_pos = position % property_browser->details->content_table_width;
	int row_pos = position / property_browser->details->content_table_width;
  	
	gtk_table_attach (GTK_TABLE (property_browser->details->content_table),
			  widget, column_pos, column_pos + 1, row_pos ,row_pos + 1, 
			  GTK_FILL, GTK_FILL, padding, padding);
}


/* make_properties_from_directories generates widgets corresponding all of the objects 
   in the public and private directories */

static GtkWidget *
make_property_tile (NautilusPropertyBrowser *property_browser,
		    GtkWidget *pixmap_widget,
		    GtkWidget *label,
		    const char* property_name)
{
	NautilusBackground *background;
	GtkWidget *temp_vbox, *event_box;
	
	temp_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (temp_vbox);

	event_box = gtk_event_box_new ();
	gtk_widget_show (event_box);
		
	background = nautilus_get_widget_background (GTK_WIDGET (event_box));
	nautilus_background_set_color (background, BROWSER_BACKGROUND_COLOR);	

	if (label != NULL) {
		nautilus_label_set_background_mode
			(NAUTILUS_LABEL (label), NAUTILUS_SMOOTH_BACKGROUND_SOLID_COLOR);
		nautilus_label_set_solid_background_color
			(NAUTILUS_LABEL (label), NAUTILUS_RGB_COLOR_WHITE);		
		nautilus_label_make_smaller (NAUTILUS_LABEL (label), 3);
		gtk_box_pack_end (GTK_BOX (temp_vbox), label, FALSE, FALSE, 2);
		gtk_widget_show (label);
	}

	gtk_widget_show (pixmap_widget);
	gtk_container_add (GTK_CONTAINER(event_box), pixmap_widget);
	gtk_box_pack_end (GTK_BOX (temp_vbox), event_box, FALSE, FALSE, 0);
	
	gtk_object_set_user_data (GTK_OBJECT(event_box), property_browser);
	gtk_signal_connect_full
		(GTK_OBJECT (event_box),
		 "button_press_event", 
		 GTK_SIGNAL_FUNC (element_clicked_callback),
		 NULL,
		 g_strdup (property_name),
		 g_free,
		 FALSE,
		 FALSE);
	
	return temp_vbox;
}

static int
make_properties_from_directories (NautilusPropertyBrowser *property_browser)
{
	NautilusCustomizationData *customization_data;
	char *object_name;
	GtkWidget *pixmap_widget;
	GtkWidget *label;
	GtkWidget *erase_object;
	int index, object_position = 0;

	/* make room for the reset property if necessary */
	if (property_browser->details->category_type == NAUTILUS_PROPERTY_PATTERN &&
	    !property_browser->details->remove_mode) {
		index = 1;
	} else {
		index = 0;
	}

	if (property_browser->details->category_type == NAUTILUS_PROPERTY_EMBLEM) {
		nautilus_g_list_free_deep (property_browser->details->keywords);	
		property_browser->details->keywords = NULL;
	}
	
	customization_data = nautilus_customization_data_new (property_browser->details->category,
							      !property_browser->details->remove_mode,
							      FALSE,
							      MAX_ICON_WIDTH,
							      MAX_ICON_HEIGHT);
	if (customization_data == NULL) {
		return index;
	}

	erase_object = NULL;
	
	/* interate through the set of objects and display each */
	while (nautilus_customization_data_get_next_element_for_display (customization_data,
									 &object_name,
									 &pixmap_widget,
									 &label) == GNOME_VFS_OK) {
		GtkWidget *temp_vbox;

		/* set the mode of the returned nautilus_image since the background is fixed */
		nautilus_image_set_background_mode (NAUTILUS_IMAGE (pixmap_widget), NAUTILUS_SMOOTH_BACKGROUND_SOLID_COLOR);	
		nautilus_image_set_solid_background_color (NAUTILUS_IMAGE (pixmap_widget), NAUTILUS_RGB_COLOR_WHITE);	
		
		/* allocate a pixmap and insert it into the table */
		temp_vbox = make_property_tile (property_browser, pixmap_widget, label, object_name);
				
		/* put the reset item in the pole position or the erase image at the end */
		object_position = index++;
		if (property_browser->details->category_type == NAUTILUS_PROPERTY_PATTERN) {
			if (nautilus_strcmp (object_name, RESET_IMAGE_NAME) == 0) {
				object_position = 0;
				index -= 1;	
			}
		} else if (property_browser->details->category_type == NAUTILUS_PROPERTY_EMBLEM) {		
			char *keyword, *extension;
			
			keyword = g_strdup (object_name);
			extension = strchr (keyword, '.');
			if (extension) {
				*extension = '\0';
			}
			property_browser->details->keywords = g_list_prepend (property_browser->details->keywords, keyword);
			if (nautilus_strcmp (object_name, ERASE_OBJECT_NAME) == 0) {
				object_position = -1;
				erase_object = temp_vbox;
				index -= 1;	
			}
		}
		if (object_position >= 0) {
			add_to_content_table(property_browser, temp_vbox, object_position, 2);
		}
		g_free (object_name);
	}
	
	/* add the eraser if necessary */
	if (erase_object != NULL) {
		add_to_content_table(property_browser, erase_object, object_position + 2, 2);
	}
	
	property_browser->details->has_local = nautilus_customization_data_private_data_was_displayed (customization_data);	
	nautilus_customization_data_destroy (customization_data);

	return index;
}

/* utility routine to add a reset property in the first position */
static void
add_reset_property (NautilusPropertyBrowser *property_browser)
{
	char *reset_path;
	GtkWidget *new_property;
	GdkPixbuf *pixbuf, *reset_pixbuf;
	GtkWidget *image_widget, *label;
	
	reset_path = g_strdup_printf ("%s/%s/%s", NAUTILUS_DATADIR, "patterns", RESET_IMAGE_NAME);
	pixbuf = gdk_pixbuf_new_from_file (reset_path);			
	reset_pixbuf = nautilus_customization_make_pattern_chit (pixbuf, property_browser->details->property_chit, FALSE);
	g_free (reset_path);
	
	image_widget = nautilus_image_new (NULL);
	nautilus_image_set_pixbuf (NAUTILUS_IMAGE (image_widget), reset_pixbuf);	
	gdk_pixbuf_unref (reset_pixbuf);

	/* make the label from the name */
	label = nautilus_label_new ("");

	new_property = make_property_tile (property_browser, image_widget, label, RESET_IMAGE_NAME);
	add_to_content_table (property_browser, new_property, 0, 2);
}
	
/* generate properties from the children of the passed in node */
/* for now, we just handle color nodes */

static void
make_properties_from_xml_node (NautilusPropertyBrowser *property_browser,
			       xmlNodePtr node)
{
	xmlNodePtr child_node;
	GdkPixbuf *pixbuf;
	GtkWidget *image_widget, *label, *new_property;
	int index;
	char *deleted, *local, *color, *name;
	
	gboolean local_only = property_browser->details->remove_mode;
	
	/* add a reset property in the first slot */
	if (!property_browser->details->remove_mode) {
		add_reset_property (property_browser);
		index = 1;
	} else
		index = 0;
	
	property_browser->details->has_local = FALSE;
	
	for (child_node = nautilus_xml_get_children (node);
	     child_node != NULL;
	     child_node = child_node->next) {
		deleted = xmlGetProp (child_node, "deleted");
		local = xmlGetProp (child_node, "local");
		
		if (deleted == NULL && (!local_only || local != NULL)) {
			if (local != NULL) {
				property_browser->details->has_local = TRUE;
			}
			
			color = xmlNodeGetContent (child_node);
			name = nautilus_xml_get_property_translated (child_node, "name");
			
			/* make the image from the color spec */
			pixbuf = make_color_drag_image (property_browser, color, FALSE);			
			image_widget = nautilus_image_new (NULL);
			nautilus_image_set_pixbuf (NAUTILUS_IMAGE (image_widget), pixbuf);
			gdk_pixbuf_unref (pixbuf);
			
			/* make the label from the name */
			label = nautilus_label_new (name);
			
			/* make the tile from the pixmap and name */
			new_property = make_property_tile (property_browser, image_widget, label, color);
			add_to_content_table (property_browser, new_property, index++, 2);				
			
			xmlFree (color);
			xmlFree (name);
		}

		xmlFree (local);
		xmlFree (deleted);
	}
}

/* handle theme changes by updating the browser contents */

static void
nautilus_property_browser_theme_changed (gpointer user_data)
{
	NautilusPropertyBrowser *property_browser;
	
	property_browser = NAUTILUS_PROPERTY_BROWSER(user_data);
	nautilus_property_browser_update_contents (property_browser);
}

/* make_category generates widgets corresponding all of the objects in the passed in directory */
static void
make_category(NautilusPropertyBrowser *property_browser, const char* path, const char* mode, xmlNodePtr node, const char *description)
{

	/* set up the description in the help label */
	nautilus_label_set_text (NAUTILUS_LABEL (property_browser->details->help_label), description);
	
	/* case out on the mode */
	if (strcmp(mode, "directory") == 0)
		make_properties_from_directories (property_browser);
	else if (strcmp(mode, "inline") == 0)
		make_properties_from_xml_node (property_browser, node);

}

/* this is a utility routine to generate a category link widget and install it in the browser */
static void
make_category_link (NautilusPropertyBrowser *property_browser, char* name, char *display_name, char* image)
{
	GtkWidget *label, *pix_widget, *button, *temp_vbox;
	char *file_name = nautilus_pixmap_file (image); 
	GtkWidget* temp_box = gtk_vbox_new (FALSE, 0);
	
	pix_widget = GTK_WIDGET (nautilus_image_new (file_name));
	gtk_widget_show (pix_widget);
	gtk_box_pack_start (GTK_BOX (temp_box), pix_widget, FALSE, FALSE, 0);
	
	/* FIXME bugzilla.eazel.com 5587: 
	 * We cant hard code the geometry of buttons because we dont know
	 * what the geometry of the label text is going to be.
	 */
	button = gtk_toggle_button_new();
	gtk_widget_show(button);
	
	/* if the button represents the current category, highlight it */
	
	if (property_browser->details->category && !strcmp(property_browser->details->category, name)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
		property_browser->details->selected_button = button;		
	}
	
	/* put the button in a vbox so it won't grow vertically */
	temp_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (temp_vbox);	
	gtk_box_pack_start (GTK_BOX (temp_vbox), button, FALSE, FALSE, 1);	

	/* use the name as a label */
	label = gtk_label_new (display_name);
			
	gtk_box_pack_start (GTK_BOX (temp_box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	gtk_box_pack_start (GTK_BOX (property_browser->details->category_box),
				temp_vbox, FALSE, FALSE, 8);
	
	property_browser->details->category_position += 1;
	
	gtk_container_add (GTK_CONTAINER (button), temp_box);
	gtk_widget_show (temp_box);
	
	/* add a signal to handle clicks */
	gtk_object_set_user_data (GTK_OBJECT(button), property_browser);
	gtk_signal_connect_full
		(GTK_OBJECT (button),
		 "clicked",
		 GTK_SIGNAL_FUNC (category_clicked_callback),
		 NULL,
		 g_strdup (name),
		 g_free,
		 FALSE,
		 FALSE);
	
	g_free (file_name);
}

/* return the width of the current category for layout */

/* FIXME: this is bogus - we really have to measure the text labels to figure
 * out the category width
 */
 
static int
nautilus_property_browser_get_category_width (NautilusPropertyBrowser *property_browser)
{
	int category_width;
	switch (property_browser->details->category_type) {
		case NAUTILUS_PROPERTY_PATTERN:
			category_width = 80;
			break;
		case NAUTILUS_PROPERTY_COLOR:
			category_width = 80;
			break;
		case NAUTILUS_PROPERTY_EMBLEM:
			category_width = 64;
			break;
		default:
			category_width = 80;
			break;
	}
	return category_width;
}

/* extract the number of columns for the current category from the xml file */
static void
set_up_category_width (NautilusPropertyBrowser *property_browser)
{
	int container_width, category_width;
	
	/* set up the default */
	property_browser->details->content_table_width = 5;

	container_width = property_browser->details->content_container->allocation.width;
	category_width = nautilus_property_browser_get_category_width (property_browser);
	
	if (container_width > 64) {
		property_browser->details->content_table_width = container_width / category_width;
		if (property_browser->details->content_table_width < 1) {
			property_browser->details->content_table_width = 1;
		}
		return;
	} 
	
}

static void
update_category_width (NautilusPropertyBrowser *property_browser)
{
	int current_width;
	
	current_width = property_browser->details->content_table_width;
	set_up_category_width (property_browser);
	if (current_width != property_browser->details->content_table_width) {
		nautilus_property_browser_update_contents (property_browser);
	}
}

/* update_contents populates the property browser with information specified by the path and other state variables */
void
nautilus_property_browser_update_contents (NautilusPropertyBrowser *property_browser)
{
	xmlNodePtr cur_node;
 	xmlDocPtr document;
 	NautilusBackground *background;
	GtkWidget *viewport;
	gboolean show_buttons, got_categories;
	char *name, *image, *type, *description, *display_name, *path, *mode;
	const char *text;

	/* load the xml document corresponding to the path and selection */
	document = read_browser_xml (property_browser);
	if (document == NULL) {
		return;
	}
		
	/* remove the existing content box, if any, and allocate a new one */
	if (property_browser->details->content_frame) {
		gtk_widget_destroy(property_browser->details->content_frame);
	}
	
	/* set up the content_table_width field so we know how many columns to put in the table */
	set_up_category_width (property_browser);
	
	/* allocate a new container, with a scrollwindow and viewport */
	
	property_browser->details->content_frame = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (property_browser->details->content_frame), 0);				
 	
 	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_IN);
	background = nautilus_get_widget_background (viewport);
	nautilus_background_set_color (background, BROWSER_BACKGROUND_COLOR);	
	gtk_container_add (GTK_CONTAINER (property_browser->details->content_container), property_browser->details->content_frame);
	gtk_widget_show (property_browser->details->content_frame);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (property_browser->details->content_frame), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* allocate a table to hold the content widgets */
  	property_browser->details->content_table = gtk_table_new(property_browser->details->content_table_width, CONTENT_TABLE_HEIGHT, FALSE);
	gtk_container_add(GTK_CONTAINER(viewport), property_browser->details->content_table); 
	gtk_container_add (GTK_CONTAINER (property_browser->details->content_frame), viewport);
	gtk_widget_show (GTK_WIDGET (property_browser->details->content_table));
	
	/* iterate through the xml file to generate the widgets */
	got_categories = property_browser->details->category_position >= 0;
	if (!got_categories) {
		property_browser->details->category_position = 0;
	}
	
	for (cur_node = nautilus_xml_get_children (xmlDocGetRootElement (document));
	     cur_node != NULL;
	     cur_node = cur_node->next) {
		if (strcmp (cur_node->name, "category") == 0) {
			name = xmlGetProp (cur_node, "name");
			
			if (property_browser->details->category != NULL
			    && strcmp (property_browser->details->category, name) == 0) {
				path = xmlGetProp (cur_node, "path");
				mode = xmlGetProp (cur_node, "mode");
				description = nautilus_xml_get_property_translated (cur_node, "description");
				type = xmlGetProp (cur_node, "type");
				
				make_category (property_browser,
					       path,
					       mode,
					       cur_node,
					       description);
				nautilus_property_browser_set_drag_type (property_browser, type);
				
				xmlFree (path);
				xmlFree (mode);
				xmlFree (description);
				xmlFree (type);
			}
			
			if (!got_categories) {
				display_name = nautilus_xml_get_property_translated (cur_node, "display_name");
				image = xmlGetProp (cur_node, "image");

				make_category_link (property_browser,
						    name,
						    display_name,
						    image);
				
				xmlFree (display_name);
				xmlFree (image);
			}
			
			xmlFree (name);
		}
	}
	
	/* release the  xml document and we're done */
	xmlFreeDoc (document);

	/* update the title and button */

	show_buttons = nautilus_preferences_get_boolean (NAUTILUS_PREFERENCES_CAN_ADD_CONTENT);

	if (property_browser->details->category == NULL) {
		nautilus_label_set_text(NAUTILUS_LABEL (property_browser->details->title_label), _("Select A Category:"));
		gtk_widget_hide(property_browser->details->add_button);
		gtk_widget_hide(property_browser->details->remove_button);
	
	} else {
		char *label_text;
				
		if (property_browser->details->remove_mode) {
			text = _("Cancel Remove");
		} else {
			switch (property_browser->details->category_type) {
			case NAUTILUS_PROPERTY_PATTERN:
				text = _("  Add a new pattern  ");
				break;
			case NAUTILUS_PROPERTY_COLOR:
				text = _("  Add a new color  ");
				break;
			case NAUTILUS_PROPERTY_EMBLEM:
				text = _("  Add a new emblem  ");
				break;
			default:
				text = NULL;
				break;
			}		
		}
		
		/* enable the "add new" button and update it's name */		
		
		if (text != NULL) {
			gtk_label_set (GTK_LABEL(property_browser->details->add_button_label), text);
		}
		if (show_buttons) {
			gtk_widget_show (property_browser->details->add_button);
		} else {
			gtk_widget_hide (property_browser->details->add_button);
		}
			
		if (property_browser->details->remove_mode) {

			switch (property_browser->details->category_type) {
			case NAUTILUS_PROPERTY_PATTERN:
				label_text = g_strdup (_("Click on a pattern to remove it"));
				break;
			case NAUTILUS_PROPERTY_COLOR:
				label_text = g_strdup (_("Click on a color to remove it"));
				break;
			case NAUTILUS_PROPERTY_EMBLEM:
				label_text = g_strdup (_("Click on an emblem to remove it"));
				break;
			default:
				label_text = NULL;
				break;
			}
		} else {	
			switch (property_browser->details->category_type) {
			case NAUTILUS_PROPERTY_PATTERN:
				label_text = g_strdup (_("Patterns:"));
				break;
			case NAUTILUS_PROPERTY_COLOR:
				label_text = g_strdup (_("Colors:"));
				break;
			case NAUTILUS_PROPERTY_EMBLEM:
				label_text = g_strdup (_("Emblems:"));
				break;
			default:
				label_text = NULL;
				break;
			}
		}
		
		if (label_text) {
			nautilus_label_set_text (NAUTILUS_LABEL (property_browser->details->title_label), label_text);
		}
		g_free(label_text);

		/* enable the remove button (if necessary) and update its name */
		
		/* case out instead of substituting to provide flexibilty for other languages */
		switch (property_browser->details->category_type) {
		case NAUTILUS_PROPERTY_PATTERN:
			text = _("  Remove a pattern  ");
			break;
		case NAUTILUS_PROPERTY_COLOR:
			text = _("  Remove a color  ");
			break;
		case NAUTILUS_PROPERTY_EMBLEM:
			text = _("  Remove an emblem  ");
			break;
		default:
			text = NULL;
			break;
		}
		
		if (!show_buttons
		    || property_browser->details->remove_mode
		    || !property_browser->details->has_local)
			gtk_widget_hide(property_browser->details->remove_button);
		else
			gtk_widget_show(property_browser->details->remove_button);
		if (text != NULL) {
			gtk_label_set (GTK_LABEL(property_browser->details->remove_button_label), text);
		}
	}
}

/* set the category and regenerate contents as necessary */

static void
nautilus_property_browser_set_category (NautilusPropertyBrowser *property_browser,
					const char *new_category)
{       
	/* there's nothing to do if the category is the same as the current one */ 
	if (nautilus_strcmp (property_browser->details->category, new_category) == 0) {
		return;
	}
	
	g_free (property_browser->details->category);
	property_browser->details->category = g_strdup (new_category);
	
	/* set up the property type enum */
	if (nautilus_strcmp (new_category, "patterns") == 0) {
		property_browser->details->category_type = NAUTILUS_PROPERTY_PATTERN;
	} else if (nautilus_strcmp (new_category, "colors") == 0) {	
		property_browser->details->category_type = NAUTILUS_PROPERTY_COLOR;
	} else if (nautilus_strcmp (new_category, "emblems") == 0) {	
		property_browser->details->category_type = NAUTILUS_PROPERTY_EMBLEM;
	} else {
		property_browser->details->category_type = NAUTILUS_PROPERTY_NONE;
	}
	
	/* populate the per-uri box with the info */
	nautilus_property_browser_update_contents (property_browser);  	
}


/* here is the routine that populates the property browser with the appropriate information 
   when the path changes */

void
nautilus_property_browser_set_path (NautilusPropertyBrowser *property_browser, 
				    const char *new_path)
{       
	/* there's nothing to do if the uri is the same as the current one */ 
	if (nautilus_strcmp (property_browser->details->path, new_path) == 0) {
		return;
	}
	
	g_free (property_browser->details->path);
	property_browser->details->path = g_strdup (new_path);
	
	/* populate the per-uri box with the info */
	nautilus_property_browser_update_contents (property_browser);  	
}

/* handle resizing ourselves by relaying out if necessary */
static void
nautilus_property_browser_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	NautilusPropertyBrowser *property_browser = NAUTILUS_PROPERTY_BROWSER (widget);
	
	NAUTILUS_CALL_PARENT_CLASS (GTK_WIDGET_CLASS, size_allocate, (widget, allocation));
	
	update_category_width (property_browser);	
}

static void
emit_emblems_changed_signal (void)
{
	gtk_signal_emit_by_name (nautilus_signaller_get_current (),
			 	 "emblems_changed");
}
