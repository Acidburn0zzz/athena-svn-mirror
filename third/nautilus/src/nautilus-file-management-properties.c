/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-file-management-properties.c - Functions to create and show the nautilus preference dialog.

   Copyright (C) 2002 Jan Arne Petersen

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

   Authors: Jan Arne Petersen <jpetersen@uni-bonn.de>
*/

#include <config.h>

#include "nautilus-file-management-properties.h"

#include <string.h>
#include <time.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtksizegroup.h>

#include <libgnome/gnome-help.h>
#include <libgnome/gnome-i18n.h>

#include <glade/glade.h>

#include <eel/eel-gconf-extensions.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-preferences-glade.h>

#include <libnautilus-private/nautilus-column-chooser.h>
#include <libnautilus-private/nautilus-column-utilities.h>
#include <libnautilus-private/nautilus-global-preferences.h>
#include <libnautilus-private/nautilus-module.h>

/* string enum preferences */
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_DEFAULT_VIEW_WIDGET "default_view_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_ICON_VIEW_ZOOM_WIDGET "iconview_zoom_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_LIST_VIEW_ZOOM_WIDGET "listview_zoom_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_SORT_ORDER_WIDGET "sort_order_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_DATE_FORMAT_WIDGET "date_format_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_TEXT_WIDGET "preview_text_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_IMAGE_WIDGET "preview_image_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_SOUND_WIDGET "preview_sound_optionmenu"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_FOLDER_WIDGET "preview_folder_optionmenu"

/* bool preferences */
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_FOLDERS_FIRST_WIDGET "sort_folders_first_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_COMPACT_LAYOUT_WIDGET "compact_layout_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_LABELS_BESIDE_ICONS_WIDGET "labels_beside_icons_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_ALWAYS_USE_BROWSER_WIDGET "always_use_browser_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_TRASH_CONFIRM_WIDGET "trash_confirm_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_TRASH_DELETE_WIDGET "trash_delete_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_OPEN_NEW_WINDOW_WIDGET "new_window_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_SHOW_HIDDEN_WIDGET "hidden_files_checkbutton"
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_TREE_VIEW_FOLDERS_WIDGET "treeview_folders_checkbutton"

/* int enums */
#define NAUTILUS_FILE_MANAGEMENT_PROPERTIES_THUMBNAIL_LIMIT_WIDGET "preview_image_size_optionmenu"

static const char *default_view_values[] = {
	"icon_view",
	"list_view",
	NULL
};

static const char *zoom_values[] = {
	"smallest",
	"smaller",
	"small",
	"standard",
	"large",
	"larger",
	"largest",
	NULL
};

static const char *sort_order_values[] = {
	"name",
	"size",
	"type",
	"modification_date",
	"emblems",
	NULL
};

static const char *date_format_values[] = {
	"locale",
	"iso",
	"informal",
	NULL
};

static const char *preview_values[] = {
	"always",
	"local_only",
	"never",
	NULL
};

static const char *click_behavior_components[] = {
	"single_click_radiobutton",
	"double_click_radiobutton",
	NULL
};

static const char *click_behavior_values[] = {
	"single",
	"double",
	NULL
};

static const char *executable_text_components[] = {
	"scripts_execute_radiobutton",
	"scripts_view_radiobutton",
	"scripts_confirm_radiobutton",
	NULL
};

static const char *executable_text_values[] = {
	"launch",
	"display",
	"ask",
	NULL
};

static int thumbnail_limit_values[] = {
	102400,
	512000,
	1048576,
	3145728,
	5242880,
	10485760,
	104857600,
	1073741824,
	-1
};

static const char *icon_captions_components[] = {
	"captions_0_optionmenu",
	"captions_1_optionmenu",
	"captions_2_optionmenu",
	NULL
};

static GladeXML *
nautilus_file_management_properties_dialog_create (void)
{
	GladeXML *xml_dialog;

	xml_dialog = glade_xml_new (GLADEDIR "/nautilus-file-management-properties.glade",
				    NULL, NULL);

	return xml_dialog;
}

static void
nautilus_file_management_properties_size_group_create (GladeXML *xml_dialog,
						       char *prefix,
						       int items)
{
	GtkSizeGroup *size_group;
	int i;
	char *item_name;

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	for (i = 0; i < items; i++) {	
		item_name = g_strdup_printf ("%s_%d", prefix, i);
		gtk_size_group_add_widget (size_group,
					   glade_xml_get_widget (xml_dialog, item_name));
		g_free (item_name);
	}
}

static void
nautilus_file_management_properties_dialog_set_icons (GtkWindow *window)
{
	GtkIconTheme *icon_theme;
	GdkPixbuf *icon;
	
	icon_theme =  gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (window)));
	icon = gtk_icon_theme_load_icon (icon_theme, "file-manager", 48, 0, NULL);
	
	if (icon != NULL) {
		gtk_window_set_icon (window, icon);
		g_object_unref (icon);
	}
}

static void
preferences_show_help (GtkWindow *parent,
		       char const *helpfile,
		       char const *sect_id)
{
	GError *error = NULL;
	GtkWidget *dialog;

	g_return_if_fail (helpfile != NULL);
	g_return_if_fail (sect_id != NULL);

	gnome_help_display_desktop (NULL,
				    "user-guide",
				    helpfile, sect_id, &error);

	if (error) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("There was an error displaying help: \n%s"),
						 error->message);

		g_signal_connect (G_OBJECT (dialog),
				  "response", G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		gtk_widget_show (dialog);
		g_error_free (error);
	}
}


static void
nautilus_file_management_properties_dialog_response_cb (GtkDialog *parent,
							int response_id,
							GladeXML *xml_dialog)
{
	char *section;

	if (response_id == GTK_RESPONSE_HELP) {
		switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (glade_xml_get_widget (xml_dialog, "notebook1")))) {
		default:
		case 0:
			section = "gosnautilus-438";
			break;
		case 1:
			section = "gosnautilus-56";
			break;
		case 2:
			section = "gosnautilus-439";
			break;
		case 3:
			section = "gosnautilus-490";
			break;
		case 4:
			section = "gosnautilus-60";
		}
		preferences_show_help (GTK_WINDOW (parent), "user-guide.xml", section);
	} else if (response_id == GTK_RESPONSE_CLOSE) {
		/* remove gconf monitors */
		eel_gconf_monitor_remove ("/apps/nautilus/icon_view");
		eel_gconf_monitor_remove ("/apps/nautilus/list_view");
		eel_gconf_monitor_remove ("/apps/nautilus/preferences");
		eel_gconf_monitor_remove ("/desktop/gnome/file_views");

		g_object_unref (xml_dialog);
	}
}

static void
columns_changed_callback (NautilusColumnChooser *chooser,
			  gpointer callback_data)
{
	GList *visible_columns;
	GList *column_order;
	
	nautilus_column_chooser_get_settings (NAUTILUS_COLUMN_CHOOSER (chooser),
					      &visible_columns,
					      &column_order);

	eel_preferences_set_string_glist (NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS, visible_columns);
	eel_preferences_set_string_glist (NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER, column_order);

	eel_g_list_free_deep (visible_columns);
	eel_g_list_free_deep (column_order);
}

static GtkWidget *
create_icon_caption_menu (GladeXML *xml_dialog,
			  GList *columns)
{
	GtkWidget *menu;
	GList *l;
	GtkWidget *menu_item;
	
	menu = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_label (_("None"));
	gtk_widget_show (menu_item);
	g_object_set_data (G_OBJECT (menu_item), "column_name", "none");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	for (l = columns; l != NULL; l = l->next) {
		NautilusColumn *column;
		char *name;
		char *label;

		column = NAUTILUS_COLUMN (l->data);
		
		g_object_get (G_OBJECT (column), 
			      "name", &name, "label", &label, 
			      NULL);

		/* Don't show name here, it doesn't make sense */
		if (!strcmp (name, "name")) {
			g_free (name);
			g_free (label);
			continue;
		}
		
		menu_item = gtk_menu_item_new_with_label (label);
		gtk_widget_show (menu_item);
		g_object_set_data_full 
			(G_OBJECT (menu_item), "column_name",
			 g_strdup (name),
			 (GDestroyNotify)g_free);
		g_free (name);
		g_free (label);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	}
	
	gtk_widget_show (menu);
	
	return menu;
}

static void
icon_captions_changed_callback (GtkOptionMenu *option_menu,
				gpointer user_data)
{
	GList *captions;
	GladeXML *xml;
	int i;
	
	xml = GLADE_XML (user_data);

	captions = NULL;

	for (i = 0; icon_captions_components[i] != NULL; i++) {
		GtkWidget *option_menu;
		GtkWidget *menu;
		GtkWidget *item;
		char *name;
		
		option_menu = glade_xml_get_widget
			(GLADE_XML (xml), icon_captions_components[i]);
		menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
		item = gtk_menu_get_active (GTK_MENU (menu));

		name = g_object_get_data (G_OBJECT (item), "column_name");
		captions = g_list_prepend (captions, g_strdup (name));
	}
	captions = g_list_reverse (captions);
	eel_preferences_set_string_glist (NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS, captions);
	eel_g_list_free_deep (captions);
}

static void
update_caption_option_menu (GladeXML *xml,
			    const char *option_menu_name,
			    const char *name)
{
	GtkWidget *option_menu;
	GtkWidget *menu;
	GList *l;
	int i;
	
	option_menu = glade_xml_get_widget (xml, option_menu_name);

	g_signal_handlers_block_by_func
		(option_menu,
		 G_CALLBACK (icon_captions_changed_callback),
		 xml);
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	for (l = GTK_MENU_SHELL (menu)->children, i = 0; 
	     l != NULL;
	     l = l->next, i++) {
		char *item_name;
		item_name = g_object_get_data (G_OBJECT (l->data), 
					       "column_name");
		if (!strcmp (name, item_name)) {
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), i);
			break;
		}
	}

	g_signal_handlers_unblock_by_func
		(option_menu,
		 G_CALLBACK (icon_captions_changed_callback),
		 xml);
}

static void
update_icon_captions_from_gconf (GladeXML *xml)
{
	GList *captions;
	int i;
	GList *l;
	

	captions = eel_preferences_get_string_glist (NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS);

	for (l = captions, i = 0; 
	     captions != NULL && icon_captions_components[i] != NULL;
	      l = l->next, i++) {
		update_caption_option_menu (xml, 
					    icon_captions_components[i],
					    (char *)l->data);
	}
	eel_g_list_free_deep (captions);
}

static void
nautilus_file_management_properties_dialog_setup_icon_caption_page (GladeXML *xml_dialog)
{
	GList *columns;
	int i;
	gboolean writable;
	
	writable = eel_preferences_key_is_writable (NAUTILUS_PREFERENCES_ICON_VIEW_CAPTIONS);

	columns = nautilus_get_all_columns ();
	
	for (i = 0; icon_captions_components[i] != NULL; i++) {
		GtkWidget *menu;
		GtkWidget *option_menu;
		
		option_menu = glade_xml_get_widget (xml_dialog, 
						    icon_captions_components[i]);

		menu = create_icon_caption_menu (xml_dialog, columns);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

		gtk_widget_set_sensitive (GTK_WIDGET (option_menu), writable);

		g_signal_connect (option_menu, "changed", 
				  G_CALLBACK (icon_captions_changed_callback),
				  xml_dialog);
	}

	nautilus_column_list_free (columns);

	update_icon_captions_from_gconf (xml_dialog);
}

static void
create_date_format_menu (GladeXML *xml_dialog)
{
	GtkWidget *option_menu;
	GtkWidget *menu;
	GtkWidget *menu_item;
	gchar *date_string;
	time_t now_raw;
	struct tm* now;
	option_menu = glade_xml_get_widget (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_DATE_FORMAT_WIDGET);
	menu = gtk_menu_new ();

	now_raw = time (NULL);
	now = localtime (&now_raw);

	date_string = eel_strdup_strftime ("%c", now);
	menu_item = gtk_menu_item_new_with_label (date_string);
	g_free (date_string);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	date_string = eel_strdup_strftime ("%Y-%m-%d %H:%M:%S", now);
	menu_item = gtk_menu_item_new_with_label (date_string);
	g_free (date_string);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	date_string = eel_strdup_strftime (_("today at %-I:%M:%S %p"), now);
	menu_item = gtk_menu_item_new_with_label (date_string);
	g_free (date_string);
	gtk_widget_show (menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
}

static void
set_columns_from_gconf (NautilusColumnChooser *chooser)
{
	GList *visible_columns;
	GList *column_order;
	
	visible_columns = eel_preferences_get_string_glist (NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
	column_order = eel_preferences_get_string_glist (NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);

	nautilus_column_chooser_set_settings (NAUTILUS_COLUMN_CHOOSER (chooser), 
					      visible_columns,
					      column_order);


	eel_g_list_free_deep (visible_columns);
	eel_g_list_free_deep (column_order);
}

static void 
use_default_callback (NautilusColumnChooser *chooser, 
		      gpointer user_data)
{
	eel_preferences_unset (NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
	eel_preferences_unset (NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);
	set_columns_from_gconf (chooser);
}

static void
nautilus_file_management_properties_dialog_setup_list_column_page (GladeXML *xml_dialog)
{
	GtkWidget *chooser;
	GtkWidget *box;
	
	chooser = nautilus_column_chooser_new ();
	g_signal_connect (chooser, "changed", 
			  G_CALLBACK (columns_changed_callback), chooser);
	g_signal_connect (chooser, "use_default", 
			  G_CALLBACK (use_default_callback), chooser);

	set_columns_from_gconf (NAUTILUS_COLUMN_CHOOSER (chooser));

	gtk_widget_show (chooser);
	box = glade_xml_get_widget (xml_dialog, "list_columns_vbox");
	
	gtk_box_pack_start (GTK_BOX (box), chooser, TRUE, TRUE, 0);
}

static  void
nautilus_file_management_properties_dialog_setup (GladeXML *xml_dialog, GtkWindow *window)
{
	GtkWidget *dialog;

	/* setup gconf stuff */
	eel_gconf_monitor_add ("/apps/nautilus/icon_view");
	eel_gconf_preload_cache ("/apps/nautilus/icon_view", GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_monitor_add ("/apps/nautilus/list_view");
	eel_gconf_preload_cache ("/apps/nautilus/list_view", GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_monitor_add ("/apps/nautilus/preferences");
	eel_gconf_preload_cache ("/apps/nautilus/preferences", GCONF_CLIENT_PRELOAD_ONELEVEL);
	eel_gconf_monitor_add ("/desktop/gnome/file_views");
	eel_gconf_preload_cache ("/desktop/gnome/file_views", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* setup UI */
	nautilus_file_management_properties_size_group_create (xml_dialog, 
							       "views_label",
							       4);
	nautilus_file_management_properties_size_group_create (xml_dialog,
							       "captions_label",
							       3);
	nautilus_file_management_properties_size_group_create (xml_dialog,
							       "preview_label",
							       5);
	create_date_format_menu (xml_dialog);

	/* setup preferences */
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_COMPACT_LAYOUT_WIDGET,
					    NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_USE_TIGHTER_LAYOUT);
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_LABELS_BESIDE_ICONS_WIDGET,
					    NAUTILUS_PREFERENCES_ICON_VIEW_LABELS_BESIDE_ICONS);
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_FOLDERS_FIRST_WIDGET,
					    NAUTILUS_PREFERENCES_SORT_DIRECTORIES_FIRST); 
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_ALWAYS_USE_BROWSER_WIDGET,
					    NAUTILUS_PREFERENCES_ALWAYS_USE_BROWSER);
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_TRASH_CONFIRM_WIDGET,
					    NAUTILUS_PREFERENCES_CONFIRM_TRASH);
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_TRASH_DELETE_WIDGET,
					    NAUTILUS_PREFERENCES_ENABLE_DELETE);
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_SHOW_HIDDEN_WIDGET,
					    NAUTILUS_PREFERENCES_SHOW_HIDDEN_FILES);
	eel_preferences_glade_connect_bool_slave (xml_dialog,
						  NAUTILUS_FILE_MANAGEMENT_PROPERTIES_SHOW_HIDDEN_WIDGET,
						  NAUTILUS_PREFERENCES_SHOW_BACKUP_FILES);
	eel_preferences_glade_connect_bool (xml_dialog,
					    NAUTILUS_FILE_MANAGEMENT_PROPERTIES_TREE_VIEW_FOLDERS_WIDGET,
					    NAUTILUS_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES);

	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_DEFAULT_VIEW_WIDGET,
							       NAUTILUS_PREFERENCES_DEFAULT_FOLDER_VIEWER,
							       default_view_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_ICON_VIEW_ZOOM_WIDGET,						     
							       NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL,
							       zoom_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_LIST_VIEW_ZOOM_WIDGET,
							       NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL,
							       zoom_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_SORT_ORDER_WIDGET,
							       NAUTILUS_PREFERENCES_ICON_VIEW_DEFAULT_SORT_ORDER,
							       sort_order_values);
	eel_preferences_glade_connect_string_enum_option_menu_slave (xml_dialog,
								     NAUTILUS_FILE_MANAGEMENT_PROPERTIES_SORT_ORDER_WIDGET,
								     NAUTILUS_PREFERENCES_LIST_VIEW_DEFAULT_SORT_ORDER);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_TEXT_WIDGET,
							       NAUTILUS_PREFERENCES_SHOW_TEXT_IN_ICONS,
							       preview_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_IMAGE_WIDGET,
							       NAUTILUS_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
							       preview_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_SOUND_WIDGET,
							       NAUTILUS_PREFERENCES_PREVIEW_SOUND,
							       preview_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_PREVIEW_FOLDER_WIDGET,
							       NAUTILUS_PREFERENCES_SHOW_DIRECTORY_ITEM_COUNTS,
							       preview_values);
	eel_preferences_glade_connect_string_enum_option_menu (xml_dialog,
							       NAUTILUS_FILE_MANAGEMENT_PROPERTIES_DATE_FORMAT_WIDGET,
							       NAUTILUS_PREFERENCES_DATE_FORMAT,
							       date_format_values);

	eel_preferences_glade_connect_string_enum_radio_button (xml_dialog,
								click_behavior_components,
								NAUTILUS_PREFERENCES_CLICK_POLICY,
								click_behavior_values);
	eel_preferences_glade_connect_string_enum_radio_button (xml_dialog,
								executable_text_components,
								NAUTILUS_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION,
								executable_text_values);

	eel_preferences_glade_connect_int_enum (xml_dialog,
						NAUTILUS_FILE_MANAGEMENT_PROPERTIES_THUMBNAIL_LIMIT_WIDGET,
						NAUTILUS_PREFERENCES_IMAGE_FILE_THUMBNAIL_LIMIT,
						thumbnail_limit_values);


	nautilus_file_management_properties_dialog_setup_icon_caption_page (xml_dialog);
	nautilus_file_management_properties_dialog_setup_list_column_page (xml_dialog);
	
	/* UI callbacks */
	dialog = glade_xml_get_widget (xml_dialog, "file_management_dialog");
	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (nautilus_file_management_properties_dialog_response_cb),
			  xml_dialog);

	nautilus_file_management_properties_dialog_set_icons (GTK_WINDOW (dialog));

	if (window) {
		gtk_window_set_screen (GTK_WINDOW (dialog), gtk_window_get_screen(window));
	}

	gtk_widget_show (dialog);
}

static gboolean
delete_event_callback (GtkWidget       *widget,
		       GdkEventAny     *event,
		       gpointer         data)
{
	void (*response_callback) (GtkDialog *dialog,
				   gint response_id);

	response_callback = data;

	response_callback (GTK_DIALOG (widget), GTK_RESPONSE_CLOSE);
	
	return TRUE;
}

void
nautilus_file_management_properties_dialog_show (GCallback close_callback, GtkWindow *window)
{
	GladeXML *xml_dialog;

	xml_dialog = nautilus_file_management_properties_dialog_create ();
	
	g_signal_connect (G_OBJECT (glade_xml_get_widget (xml_dialog, "file_management_dialog")),
			  "response", close_callback, NULL);
	g_signal_connect (G_OBJECT (glade_xml_get_widget (xml_dialog, "file_management_dialog")),
			  "delete_event", G_CALLBACK (delete_event_callback), close_callback);

	nautilus_file_management_properties_dialog_setup (xml_dialog, window);
}
