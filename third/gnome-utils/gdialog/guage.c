/*
 *  guage.c -- implements the guage dialog
 *
 *  AUTHOR: Marc Ewing, Red Hat Software
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dialog.h"

/*
 * FIXME: The gnome mode of this just doesn't work
 */

/*
 * Display a guage, or progress meter.  Starts at percent% and
 * reads stdin.  If stdin is not XXX, then it is interpreted as
 * a percentage, and the display is updated accordingly.  Otherwise
 * the next line is the percentage, and subsequent lines up to
 * another XXX are used for the new prompt.  Note that the size
 * of the window never changes, so the prompt can not get any
 * larger than the height and width specified.
 */
int
dialog_guage (const char *title, const char *prompt, int height,
		int width, int percent)
{
    int i, x, y;
    char buf[1024];
    char prompt_buf[1024];
    WINDOW *dialog;
    gint input;

    /* center dialog box on screen */
    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    if(percent < 0) {
        percent = 0;
      }      
    else if (percent > 100) {
        percent = 100;
      }
    if(gnome_mode)
      {
	GtkWidget *w = gtk_dialog_new_with_buttons (title,NULL,GTK_DIALOG_DESTROY_WITH_PARENT,NULL);
	GtkWidget *p = gtk_progress_bar_new();
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	guint input;
	GIOChannel *giochannel;

	GList *labellist;
	
	label_autowrap(vbox, prompt, width);
	gtk_progress_set_percentage( GTK_PROGRESS( p ), ( (gfloat) percent ) / 100);
	gtk_box_pack_start(GTK_BOX(vbox), p, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w)->vbox), vbox,
			   TRUE, TRUE, GNOME_PAD);
	gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);

	giochannel = g_io_channel_unix_new (0);
	input = g_io_add_watch(giochannel, G_IO_IN, callback_progress_bar, (gpointer) p);

	if(GTK_IS_ACCESSIBLE(gtk_widget_get_accessible(p))) {
		labellist = gtk_container_get_children(GTK_CONTAINER(vbox));
		add_atk_namedesc(p, _("Gdialog progressbar"), _("Indicates the percentage specified at the standard input"));
		add_atk_relation(GTK_WIDGET(labellist->data), p, ATK_RELATION_LABEL_FOR);
		add_atk_relation(p, GTK_WIDGET(labellist->data), ATK_RELATION_LABELLED_BY);
	}

	gtk_widget_show_all(w);
	gtk_main();
	exit( 0 );
      }
#ifndef NO_COLOR_CURSES
    if (use_shadow)
      draw_shadow (stdscr, y, x, height, width);
#endif
    dialog = newwin (height, width, y, x);
    keypad (dialog, TRUE);
    do {
	werase (dialog);
	draw_box (dialog, 0, 0, height, width, dialog_attr, border_attr);

	if (title != NULL) {
	    wattrset (dialog, title_attr);
	    wmove (dialog, 0, (width - strlen (title)) / 2 - 1);
	    waddch (dialog, ' ');
	    waddstr (dialog, title);
	    waddch (dialog, ' ');
	}
	wattrset (dialog, dialog_attr);
	print_autowrap (dialog, prompt, width - 2, 1, 2);

	draw_box (dialog, height - 4, 3, 3, width - 6, dialog_attr,
		  border_attr);

	wmove (dialog, height - 3, 4);
	wattrset (dialog, title_attr);
	for (i = 0; i < (width - 8); i++)
	    waddch (dialog, ' ');

	wattrset (dialog, title_attr);
	wmove (dialog, height - 3, (width / 2) - 2);
	g_snprintf (buf, sizeof(buf), "%3d%%", percent);
	waddstr (dialog, buf);

	x = (percent * (width - 8)) / 100;
	wattrset (dialog, item_selected_attr);
	wmove (dialog, height - 3, 4);
	for (i = 0; i < x; i++)
	    waddch (dialog, winch (dialog));

	wrefresh (dialog);

	if (feof (stdin))
	    break;
	fgets (buf, sizeof(buf)-1, stdin);
	if (buf[0] == 'X') {
	    /* Next line is percentage */
	    fgets (buf, sizeof (buf)-1, stdin);
	    percent = atoi (buf);

	    /* Rest is message text */
	    prompt_buf[0] = '\0';
	    while (strncmp (fgets (buf, sizeof (buf)-1, stdin), "XXX", 3))
		strcat (prompt_buf, buf);
	    prompt = prompt_buf;
	} else
	    percent = atoi (buf);
    } while (1);

    delwin (dialog);
    return (0);
}

#ifdef WITH_GNOME

/*
 *	Gnome Callbacks
 */

gboolean callback_progress_bar (GIOChannel *giochannel, GIOCondition condition,
				gpointer p)
{
	gint percent;
	gchar buf[1024];

	if (feof (stdin)) {
	    /*gdk_input_remove(input);*/
	    gtk_main_quit();
	}

	fgets (buf, sizeof(buf)-1, stdin);
	percent = atoi (buf);
	if(percent < 0) {
            percent = 0;
	}      
	else if (percent > 100) {
	    percent = 100;
	}

	gtk_progress_bar_set_fraction ( p, ( (gfloat) percent ) / 100);


}

#endif
