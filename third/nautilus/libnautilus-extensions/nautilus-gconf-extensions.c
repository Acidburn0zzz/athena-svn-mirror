/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-gconf-extensions.c - Stuff to make GConf easier to use in Nautilus.

   Copyright (C) 2000 Eazel, Inc.

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

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>
#include "nautilus-gconf-extensions.h"

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>

static GConfClient *global_gconf_client = NULL;

static void
global_client_free (void)
{
	if (global_gconf_client == NULL) {
		return;
	}
	
	gtk_object_unref (GTK_OBJECT (global_gconf_client));
	global_gconf_client = NULL;
}

/* Public */
GConfClient *
nautilus_gconf_client_get_global (void)
{
	/* Initialize gconf if needed */
	if (!gconf_is_initialized ()) {
		char *argv[] = { "nautilus", NULL };
		GError *error = NULL;
		
		if (!gconf_init (1, argv, &error)) {
			if (nautilus_gconf_handle_error (&error)) {
				return NULL;
			}
		}
	}
	
	if (global_gconf_client == NULL) {
		global_gconf_client = gconf_client_get_default ();
		g_atexit (global_client_free);
	}
	
	return global_gconf_client;
}

gboolean
nautilus_gconf_handle_error (GError **error)
{
	static gboolean shown_dialog = FALSE;
	
	g_return_val_if_fail (error != NULL, FALSE);

	if (*error != NULL) {
		g_warning (_("GConf error:\n  %s"), (*error)->message);
		if ( ! shown_dialog) {
			char *message;
			GtkWidget *dialog;

			shown_dialog = TRUE;

			message = g_strdup_printf (_("GConf error:\n  %s\n"
						     "All further errors shown "
						     "only on terminal"),
						   (*error)->message);

			dialog = gnome_error_dialog (message);
		}
		g_error_free (*error);
		*error = NULL;

		return TRUE;
	}

	return FALSE;
}

void
nautilus_gconf_set_boolean (const char *key,
			    gboolean boolean_value)
{
	GConfClient *client;
	GError *error = NULL;
	
	g_return_if_fail (key != NULL);

	client = nautilus_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_set_bool (client, key, boolean_value, &error);
	nautilus_gconf_handle_error (&error);
}

gboolean
nautilus_gconf_get_boolean (const char *key)
{
	gboolean result;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, FALSE);
	
	client = nautilus_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, FALSE);
	
	result = gconf_client_get_bool (client, key, &error);
	
	if (nautilus_gconf_handle_error (&error)) {
		result = FALSE;
	}
	
	return result;
}

void
nautilus_gconf_set_integer (const char *key,
			    int int_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = nautilus_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_set_int (client, key, int_value, &error);
	nautilus_gconf_handle_error (&error);
}

int
nautilus_gconf_get_integer (const char *key)
{
	int result;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, 0);
	
	client = nautilus_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, 0);
	
	result = gconf_client_get_int (client, key, &error);

	if (nautilus_gconf_handle_error (&error)) {
		result = 0;
	}
	
	return result;
}

void
nautilus_gconf_set_string (const char *key,
			   const char *string_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = nautilus_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_set_string (client, key, string_value, &error);
	nautilus_gconf_handle_error (&error);
}

char *
nautilus_gconf_get_string (const char *key)
{
	char *result;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, NULL);
	
	client = nautilus_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);
	
	result = gconf_client_get_string (client, key, &error);
	
	if (nautilus_gconf_handle_error (&error)) {
		result = g_strdup ("");
	}
	
	return result;
}

void
nautilus_gconf_set_string_list (const char *key,
				   GSList *string_list_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = nautilus_gconf_client_get_global ();
	g_return_if_fail (client != NULL);

	gconf_client_set_list (client, key, GCONF_VALUE_STRING, string_list_value, &error);
	nautilus_gconf_handle_error (&error);
}

GSList *
nautilus_gconf_get_string_list (const char *key)
{
	GSList *result;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, NULL);
	
	client = nautilus_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);
	
	result = gconf_client_get_list (client, key, GCONF_VALUE_STRING, &error);
	
	if (nautilus_gconf_handle_error (&error)) {
		result = NULL;
	}
	
	return result;
}

gboolean
nautilus_gconf_is_default (const char *key)
{
	gboolean result;
	GConfValue *value;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, FALSE);
	
	value = gconf_client_get_without_default  (nautilus_gconf_client_get_global (), key, &error);

	if (nautilus_gconf_handle_error (&error)) {
		if (value != NULL) {
			gconf_value_free (value);
		}
		return FALSE;
	}

	result = (value == NULL);

	if (value != NULL) {
		gconf_value_free (value);
	}
	
	return result;
}

gboolean
nautilus_gconf_monitor_directory (const char *directory)
{
	GError *error = NULL;
	GConfClient *client;

	g_return_val_if_fail (directory != NULL, FALSE);

	client = gconf_client_get_default ();

	g_return_val_if_fail (client != NULL, FALSE);

	gconf_client_add_dir (client,
			      directory,
			      GCONF_CLIENT_PRELOAD_NONE,
			      &error);
	
	if (nautilus_gconf_handle_error (&error)) {
		return FALSE;
	}

	return TRUE;
}

void
nautilus_gconf_suggest_sync (void)
{
	GConfClient *client;
	GError *error = NULL;

	client = nautilus_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_suggest_sync (client, &error);
	nautilus_gconf_handle_error (&error);
}
