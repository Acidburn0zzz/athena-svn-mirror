#include "test.h"

#include <libnautilus-extensions/nautilus-image.h>
#include <libnautilus-extensions/nautilus-image-with-background.h>
#include <libnautilus-extensions/nautilus-string-picker.h>
#include <libnautilus-extensions/nautilus-preferences.h>

#include <unistd.h>

static void
user_level_changed_callback (gpointer callback_data)
{
	char *name;
	int user_level;
	int visible_user_level;

	g_return_if_fail (NAUTILUS_IS_STRING_PICKER (callback_data));


	name = nautilus_caption_get_title_label (NAUTILUS_CAPTION (callback_data));

	user_level = nautilus_preferences_get_user_level ();
	visible_user_level = nautilus_preferences_get_visible_user_level (name);

	if (visible_user_level <= user_level) {
		gtk_widget_show (GTK_WIDGET (callback_data));
	} else {
		gtk_widget_hide (GTK_WIDGET (callback_data));
	}

#if 0
	g_print ("%s(data=%s) user_level = %d, visible_user_level = %d, action = %s\n",
		 __FUNCTION__,
		 name,
		 user_level,
		 visible_user_level,
		 (visible_user_level <= user_level) ? "show" : "hide");
#endif
	
	g_free (name);
}

static void
fruits_changed_callback (gpointer callback_data)
{
	g_print ("Something underneath 'fruits' changed, dunno what\n");
}

static void
int_picker_changed_callback (NautilusStringPicker *string_picker,
			     gpointer callback_data)
{
	char *selected_string;
	int new_value;

	g_return_if_fail (NAUTILUS_IS_STRING_PICKER (string_picker));
	g_return_if_fail (callback_data != NULL);

	selected_string = nautilus_string_picker_get_selected_string (string_picker);
	
	new_value = nautilus_string_picker_get_index_for_string (string_picker, selected_string);

	nautilus_preferences_set_integer ((const char *) callback_data, new_value);

	g_free (selected_string);
}

static void
user_level_picker_changed_callback (NautilusStringPicker *string_picker,
				    gpointer callback_data)
{
	char *selected_string;
	int new_user_level;

	g_return_if_fail (NAUTILUS_IS_STRING_PICKER (string_picker));
	g_return_if_fail (callback_data != NULL);

	selected_string = nautilus_string_picker_get_selected_string (string_picker);
	
	new_user_level = nautilus_string_picker_get_index_for_string (string_picker, selected_string);

	nautilus_preferences_set_user_level (new_user_level);

	g_free (selected_string);
}

static GtkWidget *
picker_new (const char *name,
	    const NautilusStringList *entries)
{
	GtkWidget *string_picker;
	
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (entries != NULL, NULL);
	
	string_picker = nautilus_string_picker_new ();
	nautilus_caption_set_title_label (NAUTILUS_CAPTION (string_picker), name);
	gtk_signal_connect (GTK_OBJECT (string_picker), "changed", GTK_SIGNAL_FUNC (int_picker_changed_callback),
			    (gpointer) name);
	
	nautilus_string_picker_set_string_list (NAUTILUS_STRING_PICKER (string_picker), entries);
	nautilus_string_picker_set_selected_string_index (NAUTILUS_STRING_PICKER (string_picker), 
							  nautilus_preferences_get_integer (name));
	
	nautilus_preferences_add_callback ("user_level", user_level_changed_callback, string_picker);
	user_level_changed_callback (string_picker);

	return string_picker;
}

static GtkWidget *
user_level_picker_new (const char *name,
		       const NautilusStringList *entries)
{
	GtkWidget *string_picker;
	
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (entries != NULL, NULL);
	
	string_picker = nautilus_string_picker_new ();
	nautilus_caption_set_title_label (NAUTILUS_CAPTION (string_picker), name);
	gtk_signal_connect (GTK_OBJECT (string_picker), "changed", GTK_SIGNAL_FUNC (user_level_picker_changed_callback),
			    (gpointer) name);
	
	nautilus_string_picker_set_string_list (NAUTILUS_STRING_PICKER (string_picker), entries);
	nautilus_string_picker_set_selected_string_index (NAUTILUS_STRING_PICKER (string_picker), 
							  nautilus_preferences_get_user_level ());
	
	nautilus_preferences_add_callback ("user_level", user_level_changed_callback, string_picker);
	user_level_changed_callback (string_picker);

	return string_picker;
}

int 
main (int argc, char *argv[])
{
	GtkWidget *window;

	GtkWidget *user_level_picker;
	GtkWidget *green_picker;
	GtkWidget *yellow_picker;
	GtkWidget *red_picker;
	GtkWidget *fruits_apple_picker;
	GtkWidget *fruits_orange_picker;
	GtkWidget *fruits_pear_picker;

	GtkWidget *vbox;

	NautilusStringList *user_level_entries;
	NautilusStringList *color_entries;
	NautilusStringList *fruits_entries;

	test_init (&argc, &argv);

	user_level_entries = nautilus_string_list_new_from_tokens ("Beginner,Intermediate,Hacker", ",", TRUE);
	color_entries = nautilus_string_list_new_from_tokens ("0,1,2,3,4,5,6,7,8,9,10", ",", TRUE);
	fruits_entries = nautilus_string_list_new_from_tokens ("0,1,2,3", ",", TRUE);

	nautilus_preferences_default_set_string ("user_level",
						 NAUTILUS_USER_LEVEL_NOVICE,
						 "hacker");

	nautilus_preferences_default_set_integer ("green",
					      NAUTILUS_USER_LEVEL_NOVICE,
					      3);

	nautilus_preferences_default_set_integer ("yellow",
					      NAUTILUS_USER_LEVEL_NOVICE,
					      9);

	nautilus_preferences_default_set_integer ("red",
					      NAUTILUS_USER_LEVEL_NOVICE,
					      7);

	nautilus_preferences_default_set_integer ("fruits/apple",
					      NAUTILUS_USER_LEVEL_NOVICE,
					      1);
	nautilus_preferences_default_set_integer ("fruits/orange",
					      NAUTILUS_USER_LEVEL_NOVICE,
					      2);
	nautilus_preferences_default_set_integer ("fruits/pear",
					      NAUTILUS_USER_LEVEL_NOVICE,
					      3);

	nautilus_preferences_set_visible_user_level ("yellow", 1);
	nautilus_preferences_set_visible_user_level ("green", 0);
	nautilus_preferences_set_visible_user_level ("red", 2);

	//sleep (10);

	window = test_window_new (NULL, 4);
	test_window_set_title_with_pid (GTK_WINDOW (window), "Preferences Change");

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	
	user_level_picker = user_level_picker_new ("user_level", user_level_entries);
	green_picker = picker_new ("green", color_entries);
	yellow_picker = picker_new ("yellow", color_entries);
	red_picker = picker_new ("red", color_entries);
	fruits_apple_picker = picker_new ("fruits/apple", fruits_entries);
	fruits_orange_picker = picker_new ("fruits/orange", fruits_entries);
	fruits_pear_picker = picker_new ("fruits/pear", fruits_entries);

	gtk_box_pack_start (GTK_BOX (vbox), user_level_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), green_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), yellow_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), red_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), fruits_apple_picker, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox), fruits_orange_picker, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox), fruits_pear_picker, FALSE, FALSE, 20);

	nautilus_string_list_free (user_level_entries);
	nautilus_string_list_free (color_entries);
	nautilus_string_list_free (fruits_entries);

	nautilus_preferences_add_callback ("fruits", fruits_changed_callback, NULL);

	gtk_widget_show (vbox);
	gtk_widget_show (window);

// 	user_level_changed_callback (green_picker);
// 	user_level_changed_callback (yellow_picker);
// 	user_level_changed_callback (red_picker);

	gtk_main ();

	return 0;
}
