/*
 *  menubox.c -- implements the menu box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
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

/* Prototypes */

void cancel_callback (GtkWidget *widget, gpointer data);

static const char* const *the_items;

static void
esc_cancel (void)
{
	exit (-1);
}

static void okayed(GtkWidget *w, int i)
{
	write(2, the_items[2*i], strlen(the_items[2*i]));
	write(2, "\n", 1);
	exit(0);
}

void cancel_callback (GtkWidget *widget, gpointer data)
{
	exit (1);
}

static int menu_width, tag_x, item_x;

/*
 * Print menu item
 */
static void print_item(WINDOW * win, const char *tag, const char *item,
		       int choice, int selected)
{
	int i;

	/* Clear 'residue' of last item */
	wattrset(win, menubox_attr);
	wmove(win, choice, 0);
	for (i = 0; i < menu_width; i++)
		waddch(win, ' ');
	wmove(win, choice, tag_x);
	wattrset(win, selected ? tag_key_selected_attr : tag_key_attr);
	waddch(win, tag[0]);
	wattrset(win, selected ? tag_selected_attr : tag_attr);
	waddstr(win, tag + 1);
	wmove(win, choice, item_x);
	wattrset(win, selected ? item_selected_attr : item_attr);
	waddstr(win, item);
}

/*
 * Display a menu for choosing among a number of options
 */
int dialog_menu(const char *title, const char *prompt, int height, int width,
		int menu_height, int item_no, const char *const *items)
{
	int i, x, y, cur_x, cur_y, box_x, box_y;
	int key = 0, button = GTK_RESPONSE_OK, choice = 0, scroll = 0, max_choice;
	WINDOW *dialog, *menu;

 	max_choice = MIN(menu_height, item_no);

	if (gnome_mode)	{
		GtkWidget *w;
		GtkWidget *but;
		GtkWidget *first_button;
		GtkWidget *butbox;
		GtkWidget *sw, *viewport;
 
		w = gtk_dialog_new_with_buttons (title,	NULL,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL,
				NULL);		
                gtk_signal_connect (GTK_OBJECT (w), "response",
                              GTK_SIGNAL_FUNC (esc_cancel), NULL);
		gtk_window_set_position (GTK_WINDOW (w), GTK_WIN_POS_CENTER);
		
		label_autowrap (GTK_DIALOG(w)->vbox, prompt, width);

		/*
		 * Setup the containers.
		 */
		sw = gtk_scrolled_window_new (NULL, NULL);
 		gtk_box_pack_start_defaults (
			GTK_BOX (GTK_DIALOG (w)->vbox), sw);
		gtk_widget_show (sw);

 		viewport = gtk_viewport_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER (sw), viewport);
 		gtk_widget_show (viewport);
 
 		butbox = gtk_vbox_new (FALSE, 0);
 		gtk_container_add (GTK_CONTAINER (viewport), butbox);
 		gtk_widget_show (butbox);

		gtk_container_set_border_width (GTK_CONTAINER (butbox),
						GNOME_PAD);
		/*
		 * Add the buttons.
		 */
		the_items = items;
		first_button = NULL;
		
		for(i=0; i< max_choice; i++)
		{
			char *x = (char *)items[2*i];
			char *y = (char *)items[2*i+1];
			char *p;

			p = g_strdup_printf("%s  -   %s", x, y);
			but=gtk_button_new_with_label(p);
			g_free(p);

			if (first_button == NULL)
				first_button = but;

			gtk_box_pack_start(GTK_BOX(butbox), but, TRUE, TRUE, 0);
			gtk_signal_connect(GTK_OBJECT(but), "clicked",
			GTK_SIGNAL_FUNC(okayed), GUINT_TO_POINTER(i));
		}

		gtk_widget_show_all(w);
		if (first_button != NULL)
			gtk_widget_grab_focus (first_button);
		gtk_main();
		return 0;
	}

	/* center dialog box on screen */
	x = (COLS - width) / 2;
	y = (LINES - height) / 2;

#ifndef NO_COLOR_CURSES
	if (use_shadow)
		draw_shadow(stdscr, y, x, height, width);
#endif
	dialog = newwin(height, width, y, x);
#ifdef WITH_GPM
	mouse_setbase(x, y);
#endif
	keypad(dialog, TRUE);

	draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
	wattrset(dialog, border_attr);
	wmove(dialog, height - 3, 0);
	waddch(dialog, ACS_LTEE);
	for (i = 0; i < width - 2; i++)
		waddch(dialog, ACS_HLINE);
	wattrset(dialog, dialog_attr);
	waddch(dialog, ACS_RTEE);
	wmove(dialog, height - 2, 1);
	for (i = 0; i < width - 2; i++)
		waddch(dialog, ' ');

	if (title != NULL) {
		wattrset(dialog, title_attr);
		wmove(dialog, 0, (width - strlen(title)) / 2 - 1);
		waddch(dialog, ' ');
		waddstr(dialog, title);
		waddch(dialog, ' ');
	}
	wattrset(dialog, dialog_attr);
	print_autowrap(dialog, prompt, width - 2, 1, 3);

	menu_width = width - 6;
	getyx(dialog, cur_y, cur_x);
	box_y = cur_y + 1;
	box_x = (width - menu_width) / 2 - 1;

	/* create new window for the menu */
	menu = subwin(dialog, menu_height, menu_width, y + box_y + 1,
		      x + box_x + 1);
	keypad(menu, TRUE);

	/* draw a box around the menu items */
	draw_box(dialog, box_y, box_x, menu_height + 2, menu_width + 2,
		 menubox_border_attr, menubox_attr);

	tag_x = 0;
	item_x = 0;
	/* Find length of longest item in order to center menu */
	for (i = 0; i < max_choice; i++) {
		tag_x = MAX(tag_x,
		    strlen(items[i * 2]) + strlen(items[i * 2 + 1]) + 2);
		item_x = MAX(item_x, strlen(items[i * 2]));
	}
	tag_x = (menu_width - tag_x) / 2;
	item_x = tag_x + item_x + 2;

	/* Print the menu */
	for (i = 0; i < item_no; i++)
		print_item(menu, items[i * 2], items[i * 2 + 1],
			   i, i == choice);
	wnoutrefresh(menu);

	/* register the new window, along with its borders */
#ifdef WITH_GPM
	mouse_mkbigregion(box_y, box_x, menu_height + 2, menu_width + 2,
	      item_no, item_x /* the threshold */ , 1 /* dirty mode */ );
#endif

	if (menu_height < item_no) {
		wattrset(dialog, darrow_attr);
		wmove(dialog, box_y + menu_height + 1, box_x + tag_x + 1);
		waddch(dialog, ACS_DARROW);
		wmove(dialog, box_y + menu_height + 1, box_x + tag_x + 2);
		waddstr(dialog, "(+)");
	}
	x = width / 2 - 11;
	y = height - 2;
	print_button(dialog, "Cancel", y, x + 14, FALSE);
	print_button(dialog, "  OK  ", y, x, TRUE);
	wrefresh(dialog);

	while (key != ESC) {
		key = mouse_wgetch(dialog);
		/* Check if key pressed matches first character of any
		   item tag in menu */
		for (i = 0; i < max_choice; i++)
			if (toupper(key) == toupper(items[(scroll + i) * 2][0]))
				break;

		if (i < max_choice ||
		    (key >= '1' && key <= MIN('9', '0' + max_choice)) ||
		    key == KEY_UP || key == KEY_DOWN || key == '-' ||
		 key == '+' || (key >= M_EVENT && key - M_EVENT < ' ')) {
			if (key >= '1' && key <= MIN('9', '0' + max_choice))
				i = key - '1';
			else if (key >= M_EVENT)
				i = key - M_EVENT;
			else if (key == KEY_UP || key == '-') {
				if (!choice) {
					if (scroll) {

						/* Scroll menu down */
						getyx(dialog, cur_y, cur_x);
						if (menu_height > 1) {
							/* De-highlight current first item */
							print_item(menu, items[scroll * 2],
								   items[scroll * 2 + 1], 0, FALSE);
							scrollok(menu, TRUE);
							wscrl(menu, -1);
							scrollok(menu, FALSE);
						}
						scroll--;
						print_item(menu, items[scroll * 2],
							   items[scroll * 2 + 1], 0, TRUE);
						wnoutrefresh(menu);

						/* print the up/down arrows */
						wmove(dialog, box_y, box_x + tag_x + 1);
						wattrset(dialog, scroll ? uarrow_attr : menubox_attr);
						waddch(dialog, scroll ? ACS_UARROW : ACS_HLINE);
						wmove(dialog, box_y, box_x + tag_x + 2);
						waddch(dialog, scroll ? '(' : ACS_HLINE);
						wmove(dialog, box_y, box_x + tag_x + 3);
						waddch(dialog, scroll ? '-' : ACS_HLINE);
						wmove(dialog, box_y, box_x + tag_x + 4);
						waddch(dialog, scroll ? ')' : ACS_HLINE);
						wattrset(dialog, darrow_attr);
						wmove(dialog, box_y + menu_height + 1,
						      box_x + tag_x + 1);
						waddch(dialog, ACS_DARROW);
						wmove(dialog, box_y + menu_height + 1,
						      box_x + tag_x + 2);
						waddstr(dialog, "(+)");
						wmove(dialog, cur_y, cur_x);
						wrefresh(dialog);
					}
					continue;	/* wait for another key press */
				} else
					i = choice - 1;
			} else if (key == KEY_DOWN || key == '+') {
				if (choice == max_choice - 1) {
					if (scroll + choice < item_no - 1) {
						/* Scroll menu up */
						getyx(dialog, cur_y, cur_x);
						if (menu_height > 1) {
							/* De-highlight current last item */
							print_item(menu, items[(scroll + max_choice - 1)
									       * 2], items[(scroll + max_choice - 1)
											   * 2 + 1], max_choice - 1, FALSE);
							scrollok(menu, TRUE);
							wscrl(menu,1);
							scrollok(menu, FALSE);
						}
						scroll++;
						print_item(menu, items[(scroll + max_choice - 1) * 2],
							   items[(scroll + max_choice - 1) * 2 + 1],
						   max_choice - 1, TRUE);
						wnoutrefresh(menu);

						/* print the up/down arrows */
						wattrset(dialog, uarrow_attr);
						wmove(dialog, box_y, box_x + tag_x + 1);
						waddch(dialog, ACS_UARROW);
						wmove(dialog, box_y, box_x + tag_x + 2);
						waddstr(dialog, "(-)");
						wmove(dialog, box_y + menu_height + 1,
						      box_x + tag_x + 1);
						wattrset(dialog, scroll + choice < item_no - 1 ?
							 darrow_attr : menubox_border_attr);
						waddch(dialog, scroll + choice < item_no - 1 ?
						 ACS_DARROW : ACS_HLINE);
						wmove(dialog, box_y + menu_height + 1,
						      box_x + tag_x + 2);
						waddch(dialog, scroll + choice < item_no - 1 ?
						       '(' : ACS_HLINE);
						wmove(dialog, box_y + menu_height + 1,
						      box_x + tag_x + 3);
						waddch(dialog, scroll + choice < item_no - 1 ?
						       '+' : ACS_HLINE);
						wmove(dialog, box_y + menu_height + 1,
						      box_x + tag_x + 4);
						waddch(dialog, scroll + choice < item_no - 1 ?
						       ')' : ACS_HLINE);
						wmove(dialog, cur_y, cur_x);
						wrefresh(dialog);
					}
					continue;	/* wait for another key press */
				} else
					i = choice + 1;
			}

			if (i != choice) {
				/* De-highlight current item */
				getyx(dialog, cur_y, cur_x);	/* Save cursor position */
				print_item(menu, items[(scroll + choice) * 2],
					   items[(scroll + choice) * 2 + 1], choice, FALSE);

				/* Highlight new item */
				choice = i;
				print_item(menu, items[(scroll + choice) * 2],
					   items[(scroll + choice) * 2 + 1], choice, TRUE);
				wnoutrefresh(menu);
				wmove(dialog, cur_y, cur_x);
				wrefresh(dialog);
			}
			continue;	/* wait for another key press */
		}
		switch (key) {
		case 'O':
		case 'o':
		case M_EVENT + 'O':
			delwin(dialog);
			return scroll + choice;
		case 'C':
		case 'c':
		case M_EVENT + 'C':
			delwin(dialog);
			return -2;
		case M_EVENT + 'o':	/* mouse enter... */
		case M_EVENT + 'c':	/* use the code for toggling */
			if(key == M_EVENT + 'o')
				button = GTK_RESPONSE_CANCEL;  
			else
				button = GTK_RESPONSE_OK; 
		case ' ':
		case TAB:
		case KEY_LEFT:
		case KEY_RIGHT:
			if (button == GTK_RESPONSE_OK) {
				button = GTK_RESPONSE_CANCEL;/* Indicates "Cancel" button is selected */
				print_button(dialog, "  OK  ", y, x, FALSE);
				print_button(dialog, "Cancel", y, x + 14, TRUE);
			} else {
				button = GTK_RESPONSE_OK;/* Indicates "OK" button is selected */
				print_button(dialog, "Cancel", y, x + 14, FALSE);
				print_button(dialog, "  OK  ", y, x, TRUE);
			}
			wrefresh(dialog);
			break;
		case '\n':
			delwin(dialog);
			if (button == GTK_RESPONSE_CANCEL) return -2;
			return scroll+choice;
		case ESC:
			break;
		}
	}

	delwin(dialog);
	return -1;		/* ESC pressed */
}
