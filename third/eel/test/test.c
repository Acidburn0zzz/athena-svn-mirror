#include "test.h"

#include <libart_lgpl/art_rgb.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <unistd.h>

void
test_init (int *argc,
	   char ***argv)
{
/* Currently the gnome_program_init() function makes the test
 * programs hang for me.  Using just the gtk_init () works for
 * most tests, so I am using just that until the gnome_program_init ()
 * function is fixed or we learn how to use it properly. -re
 */
#if 0
	gnome_program_init ("test-eel-widgets", VERSION,
			    libgnomeui_module_info_get (), *argc, *argv,
			    NULL);
#else
	gtk_init (argc, argv);
#endif
	gnome_vfs_init ();

	eel_make_warnings_and_criticals_stop_in_debugger ();
}

int
test_quit (int exit_code)
{
	//gnome_vfs_shutdown ();

	if (gtk_main_level () > 0) {
		gtk_main_quit ();
	}

	return exit_code;
}

void
test_delete_event (GtkWidget *widget,
		   GdkEvent *event,
		   gpointer callback_data)
{
	test_quit (0);
}

GtkWidget *
test_window_new (const char *title, guint border_width)
{
	GtkWidget *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	if (title != NULL) {
		gtk_window_set_title (GTK_WINDOW (window), title);
	}

	g_signal_connect (window,
			    "delete_event",
			    G_CALLBACK (test_delete_event),
			    NULL);
	
	gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (window), border_width);
	
	return window;
}

void
test_gtk_widget_set_background_image (GtkWidget *widget,
				      const char *image_name)
{
	EelBackground *background;
	char *path, *uri;

	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (image_name != NULL);

	background = eel_get_widget_background (widget);
	
	path = g_strconcat ("EEL_DATADIR", image_name, NULL);
	uri = gnome_vfs_get_uri_from_local_path (path);
	g_free (path);

	eel_background_set_image_uri (background, uri);

	g_free (uri);
}

void
test_gtk_widget_set_background_color (GtkWidget *widget,
				      const char *color_spec)
{
	EelBackground *background;

	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (color_spec != NULL);

	background = eel_get_widget_background (widget);
	
	eel_background_set_color (background, color_spec);
}

GdkPixbuf *
test_pixbuf_new_named (const char *name, float scale)
{
	GdkPixbuf *pixbuf;
	char *path;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (scale >= 0.0, NULL);

	if (name[0] == '/') {
		path = g_strdup (name);
	} else {
		path = g_strdup_printf ("%s/%s", "EEL_DATADIR", name);
	}

	pixbuf = gdk_pixbuf_new_from_file (path, NULL);

	g_free (path);

	g_return_val_if_fail (pixbuf != NULL, NULL);
	
	if (scale != 1.0) {
		GdkPixbuf *scaled;
		float width = gdk_pixbuf_get_width (pixbuf) * scale;
		float height = gdk_pixbuf_get_width (pixbuf) * scale;

		scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);

		g_object_unref (pixbuf);

		g_return_val_if_fail (scaled != NULL, NULL);

		pixbuf = scaled;
	}

	return pixbuf;
}

// /* Preferences hacks */
// void
// test_text_caption_set_text_for_int_preferences (EelTextCaption *text_caption,
// 					 const char *name)
// {
// 	int int_value;
// 	char *text;

// 	g_return_if_fail (EEL_IS_TEXT_CAPTION (text_caption));
// 	g_return_if_fail (name != NULL);
	
// 	int_value = eel_preferences_get_integer (name);

// 	text = g_strdup_printf ("%d", int_value);

// 	eel_text_caption_set_text (EEL_TEXT_CAPTION (text_caption), text);

// 	g_free (text);
// }

// void
// test_text_caption_set_text_for_string_preferences (EelTextCaption *text_caption,
// 						   const char *name)
// {
// 	char *text;

// 	g_return_if_fail (EEL_IS_TEXT_CAPTION (text_caption));
// 	g_return_if_fail (name != NULL);
	
// 	text = eel_preferences_get (name);
	
// 	eel_text_caption_set_text (EEL_TEXT_CAPTION (text_caption), text);

// 	g_free (text);
// }

// void
// test_text_caption_set_text_for_default_int_preferences (EelTextCaption *text_caption,
// 							const char *name)
// {
// 	int int_value;
// 	char *text;
	
// 	g_return_if_fail (EEL_IS_TEXT_CAPTION (text_caption));
// 	g_return_if_fail (name != NULL);
	
// 	int_value = 0;

// 	text = g_strdup_printf ("%d", int_value);

// 	eel_text_caption_set_text (EEL_TEXT_CAPTION (text_caption), text);

// 	g_free (text);
// }

// void
// test_text_caption_set_text_for_default_string_preferences (EelTextCaption *text_caption,
// 							   const char *name)
// {
// 	char *text;
	
// 	g_return_if_fail (EEL_IS_TEXT_CAPTION (text_caption));
// 	g_return_if_fail (name != NULL);
	
// 	text = g_strdup ("");

// 	eel_text_caption_set_text (EEL_TEXT_CAPTION (text_caption), text);

// 	g_free (text);
// }

#if 0
int
test_text_caption_get_text_as_int (const EelTextCaption *text_caption)
{
	int result = 0;
	char *text;

	g_return_val_if_fail (EEL_IS_TEXT_CAPTION (text_caption), 0);

	text = eel_text_caption_get_text (text_caption);
	eel_str_to_int (text, &result);
	g_free (text);

	return result;
}
#endif

void 
test_window_set_title_with_pid (GtkWindow *window,
				const char *title)
{
	char *tmp;
	
	g_return_if_fail (GTK_IS_WINDOW (window));

	tmp = g_strdup_printf ("%d: %s", getpid (), title);
	gtk_window_set_title (GTK_WINDOW (window), tmp);
	g_free (tmp);
}

void
test_pixbuf_draw_rectangle_tiled (GdkPixbuf *pixbuf,
				  const char *tile_name,
				  int x0,
				  int y0,
				  int x1,
				  int y1,
				  int opacity)
{
	ArtIRect area;
	GdkPixbuf *tile_pixbuf;

	g_return_if_fail (eel_gdk_pixbuf_is_valid (pixbuf));
	g_return_if_fail (tile_name != NULL);
 	g_return_if_fail (opacity > EEL_OPACITY_FULLY_TRANSPARENT);
 	g_return_if_fail (opacity <= EEL_OPACITY_FULLY_OPAQUE);

	tile_pixbuf = test_pixbuf_new_named (tile_name, 1.0);

 	g_return_if_fail (tile_pixbuf != NULL);

	if (x0 == -1 && y0 == -1 && x1 == -1 && y1 == -1) {
		EelDimensions dimensions;
		dimensions = eel_gdk_pixbuf_get_dimensions (pixbuf);
		area = eel_art_irect_assign_dimensions (0, 0, dimensions);
	} else {
		g_return_if_fail (x0 >= 0);
		g_return_if_fail (y0 >= 0);
		g_return_if_fail (x1 > x0);
		g_return_if_fail (y1 > y0);

		area.x0 = x0;
		area.y0 = y0;
		area.x1 = x1;
		area.y1 = y1;
	}
	
	eel_gdk_pixbuf_draw_to_pixbuf_tiled (tile_pixbuf,
					     pixbuf,
					     area,
					     gdk_pixbuf_get_width (tile_pixbuf),
					     gdk_pixbuf_get_height (tile_pixbuf),
					     0,
					     0,
					     opacity,
					     GDK_INTERP_NEAREST);

	g_object_unref (tile_pixbuf);
}

char *
eel_pixmap_file (const char *partial_path)
{
	char *path;

	/* Look for a non-GPL Eazel logo version. */
	path = g_strdup_printf ("%s/%s", DATADIR "/pixmaps/nautilus/eazel-logos", partial_path);
	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		return path;
	}
	g_free (path);

	/* Look for a GPL version. */
	path = g_strdup_printf ("%s/%s", DATADIR "/pixmaps/nautilus", partial_path);
	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		return path;
	}
	g_free (path);

	return NULL;
}
