/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef UI_H
#define UI_H


#include <config.h>
#include <gnome.h>
#include "actions.h"
#include "fr-stock.h"


static GtkActionEntry action_entries[] = {
	{ "ArchiveMenu", NULL, N_("_Archive") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "HelpMenu", NULL, N_("_Help") },
	{ "ArrangeFilesMenu", NULL, N_("_Arrange Files") },
	{ "RecentFilesMenu", NULL, N_("Open R_ecent") },

	{ "About", GNOME_STOCK_ABOUT,
	  N_("_About"), NULL,
	  N_("Information about the program"),
	  G_CALLBACK (activate_action_about) },
	{ "AddFiles", FR_STOCK_ADD,
	  N_("_Add Files..."), NULL,
	  N_("Add files to the archive"),
	  G_CALLBACK (activate_action_add_files) },
	{ "AddFiles_Toolbar", FR_STOCK_ADD,
	  N_("Add"), NULL,
	  N_("Add files to the archive"),
	  G_CALLBACK (activate_action_add_files) },
	{ "AddFolder", FR_STOCK_ADD,
	  N_("Add a _Folder..."), NULL,
	  N_("Add a folder to the archive"),
	  G_CALLBACK (activate_action_add_folder) },
	{ "Close", GTK_STOCK_CLOSE,
	  N_("_Close"), "<control>W",
	  N_("Close the current archive"),
	  G_CALLBACK (activate_action_close) },
	{ "Contents", GTK_STOCK_HELP,
	  N_("_Contents"), "F1",
	  N_("Display the File Roller Manual"),
	  G_CALLBACK (activate_action_manual) },
	{ "Copy", GTK_STOCK_COPY,
	  N_("_Copy"), "<control>C",
	  N_("Copy the selection"),
	  G_CALLBACK (activate_action_copy) },
	{ "CopyArchive", NULL,
	  N_("Cop_y..."), NULL,
	  N_("Copy current archive to another folder"),
	  G_CALLBACK (activate_action_copy_archive) },
	{ "Cut", GTK_STOCK_CUT,
	  N_("Cu_t"), "<control>X",
	  N_("Cut the selection"),
	  G_CALLBACK (activate_action_cut) },
	{ "Delete", GTK_STOCK_REMOVE,
	  N_("_Delete..."), NULL,
	  N_("Delete the selection from the archive"),
	  G_CALLBACK (activate_action_delete) },
	{ "DeleteArchive", GTK_STOCK_DELETE,
	  N_("Move to _Trash"), NULL,
	  N_("Move current archive to trash"),
	  G_CALLBACK (activate_action_delete_archive) },
	{ "DeselectAll", NULL,
	  N_("Dese_lect All"), NULL,
	  N_("Deselect all files"),
	  G_CALLBACK (activate_action_deselect_all) },
	{ "Extract", FR_STOCK_EXTRACT,
	  N_("_Extract..."), NULL,
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract) },
	{ "Extract_Toolbar", FR_STOCK_EXTRACT,
	  N_("Extract"), NULL,
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract) },
	{ "LastOutput", NULL,
	  N_("_Last Output"), NULL,
	  N_("View the output produced by the last executed command"),
	  G_CALLBACK (activate_action_last_output) },
	{ "MoveArchive", NULL,
	  N_("_Move..."), NULL,
	  N_("Move current archive to another folder"),
	  G_CALLBACK (activate_action_move_archive) },
	{ "New", GTK_STOCK_NEW,
	  N_("_New"), "<control>N",
	  N_("Create a new archive"),
	  G_CALLBACK (activate_action_new) },
	{ "Open", GTK_STOCK_OPEN,
	  N_("_Open..."), "<control>O",
	  N_("Open archive"),
	  G_CALLBACK (activate_action_open) },
	{ "Open_Toolbar", GTK_STOCK_OPEN,
	  N_("Open"), NULL,
	  N_("Open archive"),
	  G_CALLBACK (activate_action_open) },
	{ "OpenSelection", NULL,
	  N_("Open Fi_les..."), NULL,
	  N_("Open selected files with an application"),
	  G_CALLBACK (activate_action_open_with) },
	{ "Password", NULL,
	  N_("Pass_word..."), NULL,
	  N_("Specify a password for this archive"),
	  G_CALLBACK (activate_action_password) },
	{ "Paste", GTK_STOCK_PASTE,
	  N_("_Paste"), "<control>V",
	  N_("Paste the clipboard"),
	  G_CALLBACK (activate_action_paste) },
	{ "Properties", GTK_STOCK_PROPERTIES,
	  N_("_Properties"), NULL,
	  N_("Show archive properties"),
	  G_CALLBACK (activate_action_properties) },
	{ "Quit", GTK_STOCK_QUIT,
	  N_("_Quit"), "<control>Q",
	  N_("Quit the application"),
	  G_CALLBACK (activate_action_quit) },
	{ "Reload", GTK_STOCK_REFRESH,
	  N_("_Reload"), "<control>R",
	  N_("Reload current archive"),
	  G_CALLBACK (activate_action_reload) },
	{ "Rename", NULL,
	  N_("_Rename..."), "F2",
	  N_("Rename the selection"),
	  G_CALLBACK (activate_action_rename) },
	{ "RenameArchive", NULL,
	  N_("_Rename..."), NULL,
	  N_("Rename current archive"),
	  G_CALLBACK (activate_action_rename_archive) },
	{ "SaveAs", GTK_STOCK_SAVE_AS,
	  N_("Save _As..."), "<shift><control>S",
	  N_("Save the current archive with a different name"),
	  G_CALLBACK (activate_action_save_as) },
	{ "SelectAll", NULL,
	  N_("Select _All"), "<control>A",
	  N_("Select all files"),
	  G_CALLBACK (activate_action_select_all) },
	{ "Stop", GTK_STOCK_STOP,
	  N_("_Stop"), "Escape",
	  N_("Stop current operation"),
	  G_CALLBACK (activate_action_stop) },
	{ "TestArchive", NULL,
	  N_("_Test Integrity"), NULL,
	  N_("Test whether the archive contains errors"),
	  G_CALLBACK (activate_action_test_archive) },
	{ "ViewSelection", FR_STOCK_VIEW,
	  N_("_View File"), NULL,
	  N_("View the selected file"),
	  G_CALLBACK (activate_action_view_or_open) },
	{ "ViewSelection_Toolbar", FR_STOCK_VIEW,
	  N_("_View"), NULL,
	  N_("View the selected file"),
	  G_CALLBACK (activate_action_view_or_open) },
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);


static GtkToggleActionEntry action_toggle_entries[] = {
	{ "ViewToolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View the main toolbar"),
	  G_CALLBACK (activate_action_view_toolbar), 
	  TRUE },
	{ "ViewStatusbar", NULL,
	  N_("Stat_usbar"), NULL,
	  N_("View the statusbar"),
	  G_CALLBACK (activate_action_view_statusbar), 
	  TRUE },
	{ "SortReverseOrder", NULL,
	  N_("_Reversed Order"), NULL,
	  N_("Reverse the list order"),
	  G_CALLBACK (activate_action_sort_reverse_order), 
	  FALSE },
};
static guint n_action_toggle_entries = G_N_ELEMENTS (action_toggle_entries);


static GtkRadioActionEntry view_as_entries[] = {
	{ "ViewAllFiles", NULL,
	  N_("View All _Files"), NULL,
	  " ", WINDOW_LIST_MODE_FLAT },
	{ "ViewAsFolder", NULL,
	  N_("View as a F_older"), NULL,
	  " ", WINDOW_LIST_MODE_AS_DIR },
};
static guint n_view_as_entries = G_N_ELEMENTS (view_as_entries);


static GtkRadioActionEntry sort_by_entries[] = {
	{ "SortByName", NULL,
	  N_("by _Name"), NULL,
	  N_("Sort file list by name"), WINDOW_SORT_BY_NAME },
	{ "SortBySize", NULL,
	  N_("by _Size"), NULL,
	  N_("Sort file list by file size"), WINDOW_SORT_BY_SIZE },
	{ "SortByType", NULL,
	  N_("by T_ype"), NULL,
	  N_("Sort file list by type"), WINDOW_SORT_BY_TYPE },
	{ "SortByDate", NULL,
	  N_("by _Date modified"), NULL,
	  N_("Sort file list by modification time"), WINDOW_SORT_BY_TIME },
	{ "SortByLocation", NULL,
	  N_("by _Location"), NULL,
	  N_("Sort file list by location"), WINDOW_SORT_BY_PATH },
};
static guint n_sort_by_entries = G_N_ELEMENTS (sort_by_entries);


static const gchar *ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Archive' action='ArchiveMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menu name='OpenRecent' action='RecentFilesMenu'>"
"      </menu>"
"      <separator name='sep01'/>"
"      <menuitem action='SaveAs'/>"
"      <separator name='sep02'/>"
"      <menuitem action='RenameArchive'/>"
"      <menuitem action='CopyArchive'/>"
"      <menuitem action='MoveArchive'/>"
"      <menuitem action='DeleteArchive'/>"
"      <menuitem action='TestArchive'/>"
"      <separator name='sep03'/>"
"      <menuitem action='Properties'/>"
"      <separator name='sep04'/>"
"      <menuitem action='Close'/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='AddFiles'/>"
"      <menuitem action='AddFolder'/>"
"      <menuitem action='Extract'/>"
"      <separator name='sep01'/>"
"      <menuitem action='Cut'/>"
"      <menuitem action='Copy'/>"
"      <menuitem action='Paste'/>"
"      <menuitem action='Rename'/>"
"      <menuitem action='Delete'/>"
"      <separator name='sep02'/>"
"      <menuitem action='SelectAll'/>"
"      <menuitem action='DeselectAll'/>"
"      <separator name='sep03'/>"
"      <menuitem action='OpenSelection'/>"
"      <menuitem action='ViewSelection'/>"
"      <separator name='sep04'/>"
"      <menuitem action='Password'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='ViewToolbar'/>"
"      <menuitem action='ViewStatusbar'/>"
"      <separator name='sep01'/>"
"      <menuitem action='Stop'/>"
"      <menuitem action='Reload'/>"
"      <separator name='sep02'/>"
"      <menuitem action='ViewAllFiles'/>"
"      <menuitem action='ViewAsFolder'/>"
"      <separator name='sep03'/>"
"      <menu action='ArrangeFilesMenu'>"
"        <menuitem action='SortByName'/>"
"        <menuitem action='SortBySize'/>"
"        <menuitem action='SortByType'/>"
"        <menuitem action='SortByDate'/>"
"        <menuitem action='SortByLocation'/>"
"        <separator name='sep01'/>"
"        <menuitem action='SortReverseOrder'/>"
"      </menu>"
"      <separator name='sep04'/>"
"      <menuitem action='LastOutput'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar  name='ToolBar'>"
"    <toolitem action='New'/>"
"    <toolitem action='Open_Toolbar'/>"
"    <separator name='sep01'/>"
"    <toolitem action='AddFiles_Toolbar'/>"
"    <toolitem action='Extract_Toolbar'/>"
"    <toolitem action='ViewSelection_Toolbar'/>"
"    <separator name='sep02'/>"
"    <toolitem action='Stop'/>"
"  </toolbar>"
"  <popup name='ListPopupMenu'>"
"    <menuitem action='ViewSelection'/>"
"    <menuitem action='OpenSelection'/>"
"    <menuitem action='Extract'/>"
"    <separator name='sep01'/>"
"    <menuitem action='Cut'/>"
"    <menuitem action='Copy'/>"
"    <menuitem action='Paste'/>"
"    <menuitem action='Rename'/>"
"    <menuitem action='Delete'/>"
"    <separator name='sep02'/>"
"    <menuitem action='SelectAll'/>"
"    <menuitem action='DeselectAll'/>"
"  </popup>"
"</ui>";


#endif /* UI_H */
