/*
 *  checklist.c -- implements the checklist box
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *     Stuart Herbert - S.Herbert@sheffield.ac.uk: radiolist extension
 *     Alessandro Rubini - rubini@ipvvis.unipv.it: merged the two
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

static GtkTreeView *cl;
static int format;

static void cancelled(GtkWidget *w, gpointer *d)
{
	exit(-1);
}

static void err_outputter(GtkTreeModel *model, GtkTreePath *path_buf, 
			GtkTreeIter *iter, GtkTreeView *cl)
{
	char *p;
	GValue value = {0, };

	gtk_tree_model_get_value (model, iter, 0, &value);
	p = g_strdup (g_value_get_string (&value));
	g_value_unset (&value);

	if(format==0)
	{
		write(2, p, strlen(p));
		write(2, "\n", 1);
	}
	else
	{
		if(format++>1)
			write(2," \"", 2);
		else
			write(2, "\"", 1);
		write(2, p, strlen(p));
		write(2, "\"", 1);
	}
}

static void okayed(GtkWidget *w, int button, gpointer *d)
{
	GtkTreeSelection *selection = NULL;

	if(button==GTK_RESPONSE_OK)
	{
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cl));
		gtk_tree_selection_selected_foreach(selection,
		(GtkTreeSelectionForeachFunc)err_outputter,GTK_TREE_VIEW(cl));

		if(format!=0)
			write(2,"\n",1);
	}
	exit(button);
}


static int list_width, check_x, item_x, checkflag;

/*
 * Print list item
 */
static void print_item(WINDOW * win, const char *tag, const char *item, int status,
		       int choice, int selected)
{
	int i;

	/* Clear 'residue' of last item */
	wattrset(win, menubox_attr);
	wmove(win, choice, 0);
	for (i = 0; i < list_width; i++)
		waddch(win, ' ');
	wmove(win, choice, check_x);
	wattrset(win, selected ? check_selected_attr : check_attr);
	if (checkflag == FLAG_CHECK)
		wprintw(win, "[%c]", status ? 'X' : ' ');
	else
		wprintw(win, "(%c)", status ? 'X' : ' ');
	wattrset(win, menubox_attr);
	waddch(win, ' ');
	wattrset(win, selected ? tag_key_selected_attr : tag_key_attr);
	waddch(win, tag[0]);
	wattrset(win, selected ? tag_selected_attr : tag_attr);
	waddstr(win, tag + 1);
	wmove(win, choice, item_x);
	wattrset(win, selected ? item_selected_attr : item_attr);
	waddstr(win, item);
}


static gint lwidth=0 , rwidth=0;

static void clist_add(GtkTreeView *cl, char *l, char *r)
{
	GtkTreeModel *model = gtk_tree_view_get_model (cl);
	gchar *data[2];
	GtkTreeIter  iter;
	PangoLayout *layout;
	PangoRectangle logical_rect;
	layout = gtk_widget_create_pango_layout (GTK_WIDGET(cl), "");
	
	
	data[0]=l;
	data[1]=r;

	
	gtk_list_store_append ( GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter,0, data[0], 1, data[1],-1);

	pango_layout_set_text (layout,l, -1);
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	lwidth = MAX(lwidth,logical_rect.width);

	pango_layout_set_text (layout,r, -1);
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	rwidth = MAX(rwidth,logical_rect.width);
		
}

/*
 * Display a dialog box with a list of options that can be turned on or off
 * The `flag' parameter is used to select between radiolist and checklist.
 */
int dialog_checklist(const char *title, const char *prompt, int height, int width,
	int list_height, int item_no, const char *const *items, int flag,
		     int separate_output)
{
	int i, x, y, cur_x, cur_y, box_x, box_y;
	int key = 0, button = GTK_RESPONSE_OK, choice = 0, scroll = 0, max_choice,
	*status;
	WINDOW *dialog, *list;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	GList *labellist;

	if (gnome_mode) {
		GtkWidget *w  = gtk_dialog_new_with_buttons (title,
			NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			NULL);

		GtkWidget *hbox, *vbox, *sw;
		
		gtk_dialog_set_default_response (GTK_DIALOG(w),GTK_RESPONSE_OK);
		gtk_window_set_title(GTK_WINDOW(w), title);

		hbox = gtk_hbox_new(FALSE, 0);
		vbox = gtk_vbox_new(FALSE, 0);

		label_autowrap(vbox, prompt, width);

		sw=gtk_scrolled_window_new (NULL, NULL);

		/* create list store */
		store = gtk_list_store_new (2,
				G_TYPE_STRING,
				G_TYPE_STRING);

		/* create tree view */
		cl = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));
		g_object_unref (G_OBJECT (store));

		gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (cl));

		if(GTK_IS_ACCESSIBLE(gtk_widget_get_accessible(GTK_WIDGET(cl)))) {
			labellist = gtk_container_get_children(GTK_CONTAINER(vbox));
			add_atk_relation(GTK_WIDGET(labellist->data), GTK_WIDGET(cl), ATK_RELATION_LABEL_FOR);
			add_atk_relation(GTK_WIDGET(cl), GTK_WIDGET(labellist->data), ATK_RELATION_LABELLED_BY);
		}

		if(flag!=FLAG_CHECK || separate_output)
		{
			format=0;
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cl));
            if (!separate_output)
			   gtk_tree_selection_set_mode (selection,GTK_SELECTION_BROWSE);
            else 
               gtk_tree_selection_set_mode (selection,GTK_SELECTION_MULTIPLE);
		}
		else
		{
			format=1;
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cl));
			gtk_tree_selection_set_mode (selection,GTK_SELECTION_MULTIPLE);
		}
		gtk_scrolled_window_set_policy (
			GTK_SCROLLED_WINDOW (sw),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		for (i = 0; i < item_no; i++) {
			char *x = (char *)items[3 * i];
			char *y = (char *)items[3 * i + 1];
			clist_add(cl, x, y);
			}

		/* add columns to the tree view */
		renderer = gtk_cell_renderer_text_new ();

		/* 1st column for tag */
		column = gtk_tree_view_column_new_with_attributes ("text",
							renderer,
							"text",0,
							NULL);

		/* set this column to a fixed sizing */
		gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
					GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN 
							(column),lwidth + 10);
		gtk_tree_view_append_column (cl, column);

		/* 2nd column for item */
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ("text",
								renderer,
								"text",
								1,
								NULL);

		gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
						GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN 
						(column),rwidth);

		gtk_tree_view_append_column (cl, column);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (cl), FALSE);
		gtk_widget_set_size_request(GTK_WIDGET(cl), lwidth+rwidth+30, 
					8*list_height+40);

		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(sw), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w)->vbox),
				   hbox,
				   TRUE, TRUE, GNOME_PAD);
		gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
		gtk_signal_connect(GTK_OBJECT(w), "close",
			GTK_SIGNAL_FUNC(cancelled), NULL);
		gtk_signal_connect(GTK_OBJECT(w), "response",
			GTK_SIGNAL_FUNC(okayed), NULL);
		gtk_widget_show_all(w);
		gtk_main();
		return 0;
	}
	checkflag = flag;

	/* Allocate space for storing item on/off status */
	if ((status = malloc(sizeof(int) * item_no)) == NULL) {
		if(!gnome_mode) endwin();
		fprintf(stderr,
		     "\nCan't allocate memory in dialog_checklist().\n");
		exit(-1);
	}
	/* Initializes status */
	for (i = 0; i < item_no; i++)
		status[i] = !strcasecmp(items[i * 3 + 2], "on");

	max_choice = MIN(list_height, item_no);

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

	list_width = width - 6;
	getyx(dialog, cur_y, cur_x);
	box_y = cur_y + 1;
	box_x = (width - list_width) / 2 - 1;

	/* create new window for the list */
	list = subwin(dialog, list_height, list_width,
		      y + box_y + 1, x + box_x + 1);
	keypad(list, TRUE);

	/* draw a box around the list items */
	draw_box(dialog, box_y, box_x, list_height + 2, list_width + 2,
		 menubox_border_attr, menubox_attr);

	check_x = 0;
	item_x = 0;
	/* Find length of longest item in order to center checklist */
	for (i = 0; i < item_no; i++) {
		check_x = MAX(check_x, strlen(items[i * 3])
			      + strlen(items[i * 3 + 1]) + 6);
		item_x = MAX(item_x, strlen(items[i * 3]));
	}
	check_x = (list_width - check_x) / 2;
	item_x = check_x + item_x + 6;

	/* Print the list */
	for (i = 0; i < max_choice; i++)
		print_item(list, items[i * 3], items[i * 3 + 1],
			   status[i], i, i == choice);
	wnoutrefresh(list);

	/* register the new window, along with its borders */
#ifdef WITH_GPM
	mouse_mkbigregion(box_y, box_x, list_height + 2, list_width + 2,
		  item_no, item_x /* the threshold */ , 0 /* normal */ );
#endif

	if (list_height < item_no) {
		wattrset(dialog, darrow_attr);
		wmove(dialog, box_y + list_height + 1, box_x + check_x + 5);
		waddch(dialog, ACS_DARROW);
		wmove(dialog, box_y + list_height + 1, box_x + check_x + 6);
		waddstr(dialog, "(+)");
	}
	x = width / 2 - 11;
	y = height - 2;
	print_button(dialog, "Cancel", y, x + 14, FALSE);
	print_button(dialog, "  OK  ", y, x, TRUE);
	wrefresh(dialog);

	while (key != ESC) {
		key = mouse_wgetch(dialog);
		/* Check if key pressed matches first character of
		   any item tag in list */
		for (i = 0; i < max_choice; i++)
			if (toupper(key) ==
			    toupper(items[(scroll + i) * 3][0]))
				break;

		if (i < max_choice ||
		    (key >= '1' && key <= MIN('9', '0' + max_choice)) ||
		    key == KEY_UP || key == KEY_DOWN || key == ' ' ||
		    key == '+' || key == '-' ||
		    (key >= M_EVENT && key - M_EVENT < ' ')) {
			if (key >= '1' && key <= MIN('9', '0' + max_choice))
				i = key - '1';
			else if (key == KEY_UP || key == '-') {
				if (!choice) {
					if (!scroll)
						continue;
					/* Scroll list down */
					getyx(dialog, cur_y, cur_x);
					if (list_height > 1) {
						/* De-highlight current first item */
						print_item(list, items[scroll * 3],
							   items[scroll * 3 + 1], status[scroll],
							   0, FALSE);
						scrollok(list, TRUE);
						wscrl(list, -1);
						scrollok(list, FALSE);
					}
					scroll--;
					print_item(list, items[scroll * 3],
						   items[scroll * 3 + 1],
						status[scroll], 0, TRUE);
					wnoutrefresh(list);

					/* print the up/down arrows */
					wmove(dialog, box_y, box_x + check_x + 5);
					wattrset(dialog, scroll ? uarrow_attr : menubox_attr);
					waddch(dialog, scroll ? ACS_UARROW : ACS_HLINE);
					wmove(dialog, box_y, box_x + check_x + 6);
					waddch(dialog, scroll ? '(' : ACS_HLINE);
					wmove(dialog, box_y, box_x + check_x + 7);
					waddch(dialog, scroll ? '-' : ACS_HLINE);
					wmove(dialog, box_y, box_x + check_x + 8);
					waddch(dialog, scroll ? ')' : ACS_HLINE);
					wattrset(dialog, darrow_attr);
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 5);
					waddch(dialog, ACS_DARROW);
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 6);
					waddch(dialog, '(');
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 7);
					waddch(dialog, '+');
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 8);
					waddch(dialog, ')');
					wmove(dialog, cur_y, cur_x);
					wrefresh(dialog);
					continue;	/* wait for another key press */
				} else
					i = choice - 1;
			} else if (key == KEY_DOWN || key == '+') {
				if (choice == max_choice - 1) {
					if (scroll + choice >= item_no - 1)
						continue;
					/* Scroll list up */
					getyx(dialog, cur_y, cur_x);
					if (list_height > 1) {
						/* De-highlight current last item before scrolling up */
						print_item(list, items[(scroll + max_choice - 1) * 3],
							   items[(scroll + max_choice - 1) * 3 + 1],
							   status[scroll + max_choice - 1],
						  max_choice - 1, FALSE);
						scrollok(list, TRUE);
						wscrl(list,1);
						scrollok(list, FALSE);
					}
					scroll++;
					print_item(list, items[(scroll + max_choice - 1) * 3],
						   items[(scroll + max_choice - 1) * 3 + 1],
					 status[scroll + max_choice - 1],
						   max_choice - 1, TRUE);
					wnoutrefresh(list);

					/* print the up/down arrows */
					wattrset(dialog, uarrow_attr);
					wmove(dialog, box_y, box_x + check_x + 5);
					waddch(dialog, ACS_UARROW);
					wmove(dialog, box_y, box_x + check_x + 6);
					waddstr(dialog, "(-)");
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 5);
					wattrset(dialog, scroll + choice < item_no - 1 ?
						 darrow_attr : menubox_border_attr);
					waddch(dialog, scroll + choice < item_no - 1 ?
					       ACS_DARROW : ACS_HLINE);
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 6);
					waddch(dialog, scroll + choice < item_no - 1 ?
					       '(' : ACS_HLINE);
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 7);
					waddch(dialog, scroll + choice < item_no - 1 ?
					       '+' : ACS_HLINE);
					wmove(dialog, box_y + list_height + 1,
					      box_x + check_x + 8);
					waddch(dialog, scroll + choice < item_no - 1 ?
					       ')' : ACS_HLINE);
					wmove(dialog, cur_y, cur_x);
					wrefresh(dialog);
					continue;	/* wait for another key press */
				} else
					i = choice + 1;
			} else if (key == ' ') {	/* Toggle item status */
				if (flag == FLAG_CHECK) {
					status[scroll + choice] = !status[scroll + choice];
					getyx(dialog, cur_y, cur_x);
					wmove(list, choice, check_x);
					wattrset(list, check_selected_attr);
					wprintw(list, "[%c]", status[scroll + choice] ? 'X' : ' ');
				} else {
					if (!status[scroll + choice]) {
						for (i = 0; i < item_no; i++)
							status[i] = 0;
						status[scroll + choice] = 1;
						getyx(dialog, cur_y, cur_x);
						for (i = 0; i < max_choice; i++)
							print_item(list, items[(scroll + i) * 3],
								   items[(scroll + i) * 3 + 1],
								   status[scroll + i], i, i == choice);
					}
				}
				wnoutrefresh(list);
				wmove(dialog, cur_y, cur_x);
				wrefresh(dialog);
				continue;	/* wait for another key press */
			}
			if (i != choice) {
				/* De-highlight current item */
				getyx(dialog, cur_y, cur_x);
				print_item(list, items[(scroll + choice) * 3],
					items[(scroll + choice) * 3 + 1],
				 status[scroll + choice], choice, FALSE);
				/* Highlight new item */
				choice = i;
				print_item(list, items[(scroll + choice) * 3],
					items[(scroll + choice) * 3 + 1],
				  status[scroll + choice], choice, TRUE);
				wnoutrefresh(list);
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
			for (i = 0; i < item_no; i++)
				if (status[i]) {
					if (flag == FLAG_CHECK) {
						if (separate_output) {
							fprintf(stderr, "%s\n", items[i * 3]);
						} else {
							fprintf(stderr, "\"%s\" ", items[i * 3]);
						}
					} else {
						fprintf(stderr, "%s", items[i * 3]);
					}

				}
			free(status);
			return 0;
		case 'C':
		case 'c':
		case M_EVENT + 'C':
			delwin(dialog);
			free(status);
			return 1;
			return 1;
		case M_EVENT + 'o':	/* mouse enter... */
		case M_EVENT + 'c':	/* use the code for toggling */
			if(key == M_EVENT + 'o')
				button = GTK_RESPONSE_CANCEL;
			else
				button = GTK_RESPONSE_OK;

		case TAB:
		case KEY_LEFT:
		case KEY_RIGHT:
			if (button == GTK_RESPONSE_OK) {			 
				button = GTK_RESPONSE_CANCEL;	/* "Cancel" button selected */
				print_button(dialog, "  OK  ", y, x, FALSE);
				print_button(dialog, "Cancel", y, x + 14, TRUE);
			} else {
				button = GTK_RESPONSE_OK;	/* "OK" button selected */
				print_button(dialog, "Cancel", y, x + 14, FALSE);
				print_button(dialog, "  OK  ", y, x, TRUE);
			}
			wrefresh(dialog);
			break;
		case ' ':
		case '\n':
			delwin(dialog);
			if (!GTK_RESPONSE_OK)
				for (i = 0; i < item_no; i++)
					if (status[i]) {
						if (flag == FLAG_CHECK) {
							if (separate_output) {
								fprintf(stderr, "%s\n", items[i * 3]);
							} else {
								fprintf(stderr, "\"%s\" ", items[i * 3]);
							}
						} else {
							fprintf(stderr, "%s", items[i * 3]);
						}

					}
			free(status);
			return button;
		case ESC:
			break;
		}
	}

	delwin(dialog);
	free(status);
	return -1;		/* ESC pressed */
}
