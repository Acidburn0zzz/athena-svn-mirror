/*
 *  Gnome Character Map
 *  callbacks.h - Callbacks for the main window
 *
 *  Copyright (C) Hongli Lai
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <gnome.h>

void edit_menu_set_sensitivity (gboolean flag);
void cb_about_click (GtkWidget *widget, gpointer user_data);
void cb_browsebtn_click (GtkButton *button, gpointer data);
void cb_charbtn_click (GtkButton *button, gpointer user_data);
gboolean cb_charbtn_enter (GtkButton *button, GdkEventFocus *event, gpointer user_data);
gboolean cb_charbtn_leave (GtkButton *button, GdkEventFocus *event, gpointer user_data);
void cb_clear_click (GtkWidget *widget, gpointer user_data);
void cb_copy_click (GtkWidget *widget, gpointer user_data);
void cb_cut_click (GtkWidget *widget, gpointer user_data);
void cb_exit_click (GtkWidget *widget, gpointer user_data);
void cb_help_click (GtkWidget *widget, gpointer user_data);
void cb_set_chartable_font (GtkWidget *widget, gpointer user_data);
void cb_insert_char_click (GtkWidget *widget, gpointer user_data);
void cb_paste_click (GtkWidget *widget, gpointer user_data);
void cb_select_all_click (GtkWidget *widget, gpointer user_data);
gboolean cb_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data);
void cb_entry_changed (GtkWidget *widget, gpointer data);

#endif /* _CALLBACKS_H_ */
