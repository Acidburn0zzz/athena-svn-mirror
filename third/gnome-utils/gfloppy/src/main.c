/* main.c
 *
 * Copyright (C) 1999 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <fcntl.h>
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>
#include <unistd.h>
#include "gfloppy.h"
#include <sys/types.h>
#include <errno.h>
#include "progress.h"
#define NUM_DEVICES_TO_CHECK 4

extern int errno;

static GFloppy floppy;
static GladeXML *xml;
static GtkWidget *toplevel = NULL;
static GtkWidget *type_option;

/* Only needed if more then one device exists */
static GList *valid_devices = NULL;

static void
devices_option_activated (GtkWidget *menu_item, gchar *device)
{
	g_free (floppy.device);
	g_free (floppy.mdevice);

	floppy.device = g_strdup (device);

	/* FIXME: what the heck is /dev/fd2 and /dev/fd3 ?? */
	if (strncmp (floppy.device, "/dev/fd1", strlen ("/dev/fd1")) == 0)
		floppy.mdevice = g_strdup ("b:");
	else
		floppy.mdevice = g_strdup ("a:");
}

static void
start_format (void)
{
	pipe (floppy.message);

	floppy.pid = fork ();
	if (floppy.pid < 0) {
		g_error ("unable to fork ().\nPlease free up some resources and try again.\n");
		_exit (1);
	}
	if (floppy.pid == 0) {
		/* child */
		close (floppy.message [0]);
		close (STDERR_FILENO);
		close (STDOUT_FILENO);
		format_floppy (&floppy);
		_exit (0);
	}
	close (floppy.message [1]);

	fcntl (floppy.message [0], F_SETFL, O_NONBLOCK);
	setup_progress_and_run (&floppy, toplevel);
}

#include <stdlib.h>

static void
init_commands (void)
{
	const char *path;
	char *newpath;

	floppy.mke2fs_cmd = NULL;
	floppy.mformat_cmd = NULL;
	floppy.badblocks_cmd = NULL;

	path = g_getenv ("PATH");

	if (path)
		newpath = g_strconcat ("PATH=", path, ":/sbin:/usr/sbin:/usr:/usr/bin", NULL);
	else
		newpath = g_strdup ("PATH=/sbin:/usr/sbin:/usr:/usr/bin");

	putenv (newpath); /* Sigh, we have to leak it */

	floppy.mke2fs_cmd    = g_find_program_in_path ("mke2fs");
	floppy.mformat_cmd   = g_find_program_in_path ("mformat");
	floppy.badblocks_cmd = g_find_program_in_path ("mbadblocks");

	if (floppy.mke2fs_cmd == NULL) {
		g_print ("Warning:  Unable to locate mke2fs.  Please confirm it is installed and try again\n");
		exit (1);
	}

}

static void
init_devices (void)
{
	GtkWidget *device_option;
	GtkWidget *device_entry;
	gchar *msg = NULL;
	gint ok_devices_present = 0;

	/* First check --device arg */
	if (floppy.device != NULL) {
		GFloppyStatus status;
		status = test_floppy_device (floppy.device);
		switch (status) {
		case GFLOPPY_NO_DEVICE:
			msg = g_strdup_printf (_("Unable to open the device %s, formatting cannot continue."), floppy.device);
			break;
		case GFLOPPY_DEVICE_DISCONNECTED:
			msg = g_strdup_printf (_("The device %s is disconnected.\nPlease attach device to continue."), floppy.device);
			break;
		case GFLOPPY_INVALID_PERMISSIONS:
			msg = g_strdup_printf (_("You do not have the proper permissions to write to %s, formatting will not be possible.\nContact your system administrator about getting write permissions."), floppy.device);
			break;
		default:
			break;
		}
		if (msg) {
			GtkWidget *toplevel = glade_xml_get_widget (xml, "toplevel");
			GtkWidget *dialog;

			gtk_widget_set_sensitive (toplevel, FALSE);
			dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel), 0,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_CLOSE,
							 msg);
			gtk_dialog_run (GTK_DIALOG (dialog));
			exit (1);
		}
		/* Device is okay */
		ok_devices_present = 1;
	} else {
		GFloppyStatus status[NUM_DEVICES_TO_CHECK];
		gint i;
		char device[32];

		for (i = 0; i < NUM_DEVICES_TO_CHECK; i++) {
			sprintf(device,"/dev/fd%d", i);
			status[i] = test_floppy_device (device);
			if (status[i] == GFLOPPY_DEVICE_OK)
				ok_devices_present++;
		}
		if (ok_devices_present == 0) {
			GtkWidget *toplevel;
			GtkWidget *dialog;

			/* Lets assume that there's only a problem with /dev/fd0 */
			switch (status[0]) {
			case GFLOPPY_NO_DEVICE:
				msg = g_strdup_printf (_("Unable to open the device %s, formatting cannot continue."), "/dev/fd0");
				break;
			case GFLOPPY_DEVICE_DISCONNECTED:
				msg = g_strdup_printf (_("The device %s is disconnected.\nPlease attach device to continue."), "/dev/fd0");
				break;
			case GFLOPPY_INVALID_PERMISSIONS:
				msg = g_strdup_printf (_("You do not have the proper permissions to write to %s, formatting will not be possible.\nContact your system administrator about getting write permissions."), "/dev/fd0");
				break;
			default:
				g_assert_not_reached ();
			}
			toplevel = glade_xml_get_widget (xml, "toplevel");
			gtk_widget_set_sensitive (toplevel, FALSE);
			dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel), 0,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_CLOSE,
							 msg);
			gtk_dialog_run (GTK_DIALOG (dialog));
			exit (1);
		} else if (ok_devices_present == 1) {
			for (i = 0; i < NUM_DEVICES_TO_CHECK; i++) {
				if (status[i] == GFLOPPY_DEVICE_OK) {
					floppy.device = g_strdup_printf ("/dev/fd%d", i);
					break;
				}
			}
		} else {
			for (i = 0; i < NUM_DEVICES_TO_CHECK; i++) {
				if (status[i] == GFLOPPY_DEVICE_OK) {
					valid_devices = g_list_append (valid_devices, g_strdup_printf ("/dev/fd%d", i));
					if (floppy.device == NULL)
						floppy.device = g_strdup_printf ("/dev/fd%d", i);
				}
			}
		}
	}

	/* FIXME: what the heck is /dev/fd2 and /dev/fd3 ?? */
	if (strncmp (floppy.device, "/dev/fd1", strlen ("/dev/fd1")) == 0)
		floppy.mdevice = g_strdup ("b:");
	else
		floppy.mdevice = g_strdup ("a:");

	/* set up the UI */
	device_option = glade_xml_get_widget (xml, "device_option");
	device_entry = glade_xml_get_widget (xml, "device_entry");

	if (ok_devices_present == 1) {
		gtk_widget_hide (device_option);
		gtk_widget_show (device_entry);
		gtk_entry_set_text (GTK_ENTRY (device_entry), floppy.device);
	} else {
		GtkWidget *menu;
		GList *list;

		gtk_widget_show (device_option);
		gtk_widget_hide (device_entry);
		gtk_option_menu_remove_menu (GTK_OPTION_MENU (device_option));
		menu = gtk_menu_new ();
		for (list = valid_devices; list; list = list->next) {
			GtkWidget *menu_item;
			menu_item = gtk_menu_item_new_with_label ((gchar *)list->data);
			gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (devices_option_activated), list->data);
			gtk_widget_show (menu_item);
			gtk_menu_append (GTK_MENU (menu), menu_item);
		}
		gtk_widget_show_all (menu);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (device_option), menu);
		gtk_option_menu_set_history (GTK_OPTION_MENU (device_option), 0);
	}
}

static void
set_floppy_extended_device (void)
{
	switch (floppy.size) {
	case 0:
		floppy.extended_device = g_strdup_printf ("%sH1440", floppy.device);
		break;
	case 1:
		floppy.extended_device = g_strdup_printf ("%sh1200", floppy.device);
		break;
	case 2:
		floppy.extended_device = g_strdup_printf ("%sD720", floppy.device);
		break;
	case 3:
		floppy.extended_device = g_strdup_printf ("%sd360", floppy.device);
		break;
	default:
		g_assert_not_reached ();
	}
}

gint
on_toplevel_delete_event (GtkWidget *w, GdkEventKey *e)
{
	gtk_main_quit ();
	return TRUE;
}

void
on_close_button_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
}

void
on_help_button_clicked (GtkWidget *widget, gpointer user_data)
{
	GError *error = NULL;

	gnome_help_display ("gfloppy", "intro", &error);
	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel), GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						 _("Could not display help for the "
						   "floppy formatter.\n"
						   "%s"),
						 error->message);

		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		gtk_widget_show (dialog);

		g_error_free (error);
	}
}

void
on_format_button_clicked (GtkWidget *widget, gpointer user_data)
{
	GtkWidget *dialog;
	GtkWidget *density_option;
	GtkWidget *quick_format_button;

        density_option = glade_xml_get_widget (xml, "density_option");
	quick_format_button = glade_xml_get_widget (xml, "quick_format_button");

        g_assert (density_option != NULL);
        g_assert (quick_format_button != NULL);

        gtk_widget_set_sensitive (toplevel, FALSE);

        if (floppy.mformat_cmd) {
        	/* Check to see which one is selected. */
                floppy.type = gtk_option_menu_get_history (GTK_OPTION_MENU (type_option));
        } else {
                floppy.type = GFLOPPY_E2FS;
        }
        floppy.size = gtk_option_menu_get_history (GTK_OPTION_MENU (density_option));
        set_floppy_extended_device ();
        floppy.quick_format = GTK_TOGGLE_BUTTON (quick_format_button)->active;

        start_format ();
        if (!toplevel)
        	return;
        gtk_widget_set_sensitive (toplevel, TRUE); /*
        dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         _("Format another floppy?"));
        if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES)
        	exit (0);
        gtk_widget_destroy (dialog); */
}

int
main (int argc, char *argv[])
{
	GtkWidget *ext2_entry;
	GtkWidget *icon_frame;
	GtkWidget *icon;
	gchar *image;

	struct poptOption gfloppy_opts[] = {
		{"device", '\0', POPT_ARG_STRING, NULL, 0, NULL, NULL},
		{NULL, '\0', 0, NULL, 0, NULL, NULL}
	};

	floppy.device = NULL;
	gfloppy_opts[0].arg = &(floppy.device);
	gfloppy_opts[0].descrip = _("The device to format");
	gfloppy_opts[0].argDescrip = _("DEVICE");

	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gnome_program_init ("gfloppy",VERSION, LIBGNOMEUI_MODULE,
			    argc, argv, GNOME_PARAM_POPT_TABLE, gfloppy_opts,
                            GNOME_PARAM_POPT_FLAGS, 0,
                            GNOME_PARAM_APP_DATADIR,DATADIR,NULL);
	/* FIXME: get the right icon */
	image = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,
                                           "document-icons/i-floppy.png", TRUE, NULL);

	
	gnome_window_icon_set_default_from_file (image);
	g_free (image);

	init_commands ();

	/* Now we can set up glade */
	glade_gnome_init();
        xml = glade_xml_new (GLADEDIR "/gfloppy2.glade", NULL, NULL);
	if (xml == NULL)
		g_error ("Cannot load/find gfloppy2.glade");

	init_devices ();
	toplevel = glade_xml_get_widget (xml, "toplevel");

	icon_frame = glade_xml_get_widget (xml, "icon_frame");
	type_option = glade_xml_get_widget (xml, "type_option");
	toplevel = glade_xml_get_widget (xml, "toplevel");
	glade_xml_signal_autoconnect (xml);
	
	icon = gtk_image_new_from_stock (GTK_STOCK_FLOPPY, GTK_ICON_SIZE_DIALOG);
	gtk_container_add (GTK_CONTAINER (icon_frame), icon);
	gtk_widget_show (icon);

	if (floppy.mformat_cmd == NULL) {
		/* We don't have mtools.  Allow ext2 only. */
		ext2_entry = glade_xml_get_widget (xml, "ext2_entry");

		gtk_widget_hide (type_option);
		gtk_widget_show (ext2_entry);
	}

	gtk_widget_show (GTK_WIDGET (toplevel));
	gtk_main ();

	return 0;
}

