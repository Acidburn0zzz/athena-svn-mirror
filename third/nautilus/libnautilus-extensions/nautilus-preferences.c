/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-preferences.c - Preference peek/poke/notify implementation.

   Copyright (C) 1999, 2000 Eazel, Inc.

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
#include "nautilus-preferences.h"

#include "nautilus-gconf-extensions.h"
#include "nautilus-string-list.h"
#include "nautilus-string.h"
#include "nautilus-glib-extensions.h"
#include "nautilus-enumeration.h"
#include "nautilus-lib-self-check-functions.h"

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include <gtk/gtksignal.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>


/*
 * PreferencesEntry:
 *
 * A structure to manage preference hash table nodes.
 * Preferences are hash tables.  The hash key is the preference name
 * (a string).  The  hash value is a pointer of the following struct:
 */
typedef struct {
	char *name;
	char *description;
	GList *callback_list;
	int gconf_connection_id;
	NautilusEnumeration *enumeration;
} PreferencesEntry;

/*
 * PreferencesCallbackEntry:
 *
 * A structure to manage callback lists.  A callback list is a GList.
 * The callback_data in each list node is a pointer to the following 
 * struct:
 */
typedef struct {
	NautilusPreferencesCallback callback;
	gpointer callback_data;
} PreferencesCallbackEntry;


static const char *user_level_names_for_display[] =
{
	N_("Beginner"),
	N_("Intermediate"),
	N_("Advanced")
};

static const char *user_level_names_for_storage[] =
{
	"novice",
	"intermediate",
	"hacker"
};

static char *       preferences_get_path                            (void);
static char *       preferences_get_defaults_path                   (void);
static char *       preferences_get_visibility_path                 (void);
static char *       preferences_get_user_level_key                  (void);
static GConfClient *preferences_global_client_get                   (void);
static void         preferences_global_client_remove_notification   (void);
static gboolean     preferences_preference_is_internal              (const char               *name);
static gboolean     preferences_preference_is_user_level            (const char               *name);
static gboolean     preferences_preference_is_default               (const char               *name);
static char *       preferences_key_make                            (const char               *name);
static char *       preferences_key_make_for_getter                 (const char               *name);
static char *       preferences_key_make_for_default                (const char               *name,
								     int                       user_level);
static char *       preferences_key_make_for_default_getter         (const char               *name,
								     int                       user_level);
static char *       preferences_key_make_for_visibility             (const char               *name);
static void         preferences_user_level_changed_notice           (GConfClient              *client,
								     guint                     connection_id,
								     GConfEntry               *gconf_entry,
								     gpointer                  user_data);
static void         preferences_something_changed_notice            (GConfClient              *client,
								     guint                     connection_id,
								     GConfEntry               *gconf_entry,
								     gpointer                  user_data);
static void         preferences_global_table_check_changes_function (gpointer                  key,
								     gpointer                  value,
								     gpointer                  callback_data);
static GHashTable  *preferences_global_table_get_global             (void);
static void         preferences_callback_entry_free                 (PreferencesCallbackEntry *callback_entry);
static int          preferences_user_level_check_range              (int                       user_level);

static int user_level_changed_connection_id = -1;
static GHashTable *global_table = NULL;

static char *
preferences_get_path (void)
{
	return g_strdup ("/apps/nautilus");
}

static char *
preferences_get_defaults_path (void)
{
	char *defaults_path;
	char *path;
	
	path = preferences_get_path ();
	defaults_path = g_strdup_printf ("%s/defaults", path);
	g_free (path);
	return defaults_path;
}

static char *
preferences_get_visibility_path (void)
{
	char *visibility_path;
	char *path;
	
	path = preferences_get_path ();
	visibility_path = g_strdup_printf ("%s/visibility", path);
	g_free (path);

	return visibility_path;
}

static char *
preferences_get_user_level_key (void)
{
	char *user_level_key;
	char *path;

	path = preferences_get_path ();
	user_level_key = g_strdup_printf ("%s/user_level", path);
	g_free (path);

	return user_level_key;
}

/* If the preference name begind with a "/", we interpret 
 * it as a straight gconf key. */
static gboolean
preferences_preference_is_internal (const char *name)
{
	g_return_val_if_fail (name != NULL, FALSE);
	
	if (nautilus_str_has_prefix (name, "/")) {
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
preferences_preference_is_user_level (const char *name)
{
	gboolean result;
	char *user_level_key;

	g_return_val_if_fail (name != NULL, FALSE);

	user_level_key = preferences_get_user_level_key ();

	result = nautilus_str_is_equal (name, user_level_key)
		|| nautilus_str_is_equal (name, "user_level");

	g_free (user_level_key);

	return result;
}

static char *
preferences_key_make (const char *name)
{
	char *key;
	char *path;

	g_return_val_if_fail (name != NULL, NULL);
	
	if (!preferences_preference_is_internal (name)) {
		return g_strdup (name);
	}

	/* Otherwise, we prefix it with the path */
	path = preferences_get_path ();
	key = g_strdup_printf ("%s/%s", path, name);
	g_free (path);

	return key;
}

static char *
preferences_key_make_for_default (const char *name,
				  int user_level)
{
	char *key;
	char *default_key = NULL;
	char *defaults_path;
	char *storage_name;

	g_return_val_if_fail (name != NULL, NULL);

	user_level = preferences_user_level_check_range (user_level);

	key = preferences_key_make (name);
	defaults_path = preferences_get_defaults_path ();

	storage_name = nautilus_preferences_get_user_level_name_for_storage (user_level);
	default_key = g_strdup_printf ("%s/%s%s",
				       defaults_path,
				       storage_name,
				       key);
	g_free (storage_name);
	g_free (key);
	g_free (defaults_path);
	
	return default_key;
}

static char *
preferences_key_make_for_default_getter (const char *name,
					 int user_level)
{
	char *default_key_for_getter = NULL;
	gboolean done;

	g_return_val_if_fail (name != NULL, NULL);

	user_level = preferences_user_level_check_range (user_level);

	done = FALSE;
	while (!done) {
		default_key_for_getter = preferences_key_make_for_default (name, user_level);
		
		done = (user_level == 0) || (!nautilus_gconf_is_default (default_key_for_getter));
		
		if (!done) {
			g_free (default_key_for_getter);
			user_level--;
		}
	}

	return default_key_for_getter;
}

static char *
preferences_key_make_for_visibility (const char *name)
{
	char *default_key;
	char *key;
	char *visibility_path;

	g_return_val_if_fail (name != NULL, NULL);

	key = preferences_key_make (name);

	visibility_path = preferences_get_visibility_path ();
	default_key = g_strdup_printf ("%s%s", visibility_path, key);
	g_free (visibility_path);
	
	return default_key;
}

static void
preferences_global_client_remove_notification (void)
{
	GConfClient *client;

	client = preferences_global_client_get ();

	g_return_if_fail (client != NULL);

	gconf_client_notify_remove (client, user_level_changed_connection_id);
	user_level_changed_connection_id = -1;
}

static GConfClient *
preferences_global_client_get (void)
{
	static GConfClient *global_gconf_client = NULL;
	GError *error = NULL;
	char *path;
	char *user_level_key;
	
	if (global_gconf_client != NULL) {
		return global_gconf_client;
	}

	global_gconf_client = nautilus_gconf_client_get_global ();
	
	g_return_val_if_fail (global_gconf_client != NULL, NULL);
	
	user_level_key = preferences_get_user_level_key ();
	error = NULL;
	user_level_changed_connection_id = gconf_client_notify_add (global_gconf_client,
								    user_level_key,
								    preferences_user_level_changed_notice,
								    NULL,
								    NULL,
								    &error);
	g_free (user_level_key);

	if (nautilus_gconf_handle_error (&error)) {
		global_gconf_client = NULL;
		return NULL;
	}

	path = preferences_get_path ();
	nautilus_gconf_monitor_directory (path);
	g_free (path);
	
	g_atexit (preferences_global_client_remove_notification);

	return global_gconf_client;
}

static gboolean
preferences_preference_is_default (const char *name)
{
	gboolean result;
	char *key;
	
	g_return_val_if_fail (name != NULL, FALSE);
	
	key = preferences_key_make (name);
	result = nautilus_gconf_is_default (key);
	g_free (key);

	return result;
}

static char *
preferences_make_user_level_filtered_key (const char *name)
{
	char *key;
	
	g_return_val_if_fail (name != NULL, NULL);

	if (nautilus_preferences_is_visible (name)) {
		key = preferences_key_make (name);
	} else {
		key = preferences_key_make_for_default (name, nautilus_preferences_get_user_level ());
	}

	return key;
}

/* Public preferences functions */
int
nautilus_preferences_get_visible_user_level (const char *name)
{
  	int result;
	char *visible_key;
	
	g_return_val_if_fail (name != NULL, FALSE);
	
	visible_key = preferences_key_make_for_visibility (name);
	result = nautilus_gconf_get_integer (visible_key);
	g_free (visible_key);

	return result;
}

void
nautilus_preferences_set_visible_user_level (const char *name,
					     int visible_user_level)
{
	char *visible_key;
	
	g_return_if_fail (name != NULL);
	
	visible_key = preferences_key_make_for_visibility (name);
	nautilus_gconf_set_integer (visible_key, visible_user_level);
	g_free (visible_key);
}

void
nautilus_preferences_set_boolean (const char *name,
				  gboolean boolean_value)
{
	char *key;

	g_return_if_fail (name != NULL);
	
	key = preferences_key_make (name);
	nautilus_gconf_set_boolean (key, boolean_value);
	g_free (key);

	nautilus_gconf_suggest_sync ();
}

static char *
preferences_key_make_for_getter (const char *name)
{
	char *key;
	
	g_return_val_if_fail (name != NULL, NULL);

	if (preferences_preference_is_default (name)) {
		key = preferences_key_make_for_default_getter (name, nautilus_preferences_get_user_level ());
	} else {
		key = preferences_make_user_level_filtered_key (name);
	}

	return key;
}

gboolean
nautilus_preferences_get_boolean (const char *name)
{
 	gboolean result;
	char *key;
	
	g_return_val_if_fail (name != NULL, FALSE);
	
	key = preferences_key_make_for_getter (name);
	result = nautilus_gconf_get_boolean (key);
	g_free (key);

	return result;
}

void
nautilus_preferences_set_integer (const char *name,
				  int int_value)
{
	char *key;

	g_return_if_fail (name != NULL);
	
	key = preferences_key_make (name);
	nautilus_gconf_set_integer (key, int_value);
	g_free (key);

	nautilus_gconf_suggest_sync ();
}

int
nautilus_preferences_get_integer (const char *name)
{
 	int result;
	char *key;

	g_return_val_if_fail (name != NULL, 0);

	key = preferences_key_make_for_getter (name);
	result = nautilus_gconf_get_integer (key);

	g_free (key);

	return result;
}

void
nautilus_preferences_set (const char *name,
			  const char *string_value)
{
	char *key;

	g_return_if_fail (name != NULL);
	
	key = preferences_key_make (name);
	nautilus_gconf_set_string (key, string_value);
	g_free (key);

	nautilus_gconf_suggest_sync ();
}

char *
nautilus_preferences_get (const char *name)
{
 	char *result;
	char *key;
	
	g_return_val_if_fail (name != NULL, NULL);

	key = preferences_key_make_for_getter (name);
	result = nautilus_gconf_get_string (key);
	g_free (key);

	if (result == NULL) {
		result = g_strdup ("");
	}

	return result;
}

void
nautilus_preferences_set_string_list (const char *name,
				      GSList *string_list_value)
{
	char *key;

	g_return_if_fail (name != NULL);
	
	key = preferences_key_make (name);
	nautilus_gconf_set_string_list (key, string_list_value);
	g_free (key);

	nautilus_gconf_suggest_sync ();
}

GSList *
nautilus_preferences_get_string_list (const char *name)
{
 	GSList *result;
	char *key;
	
	g_return_val_if_fail (name != NULL, NULL);
	
	key = preferences_key_make_for_getter (name);
	result = nautilus_gconf_get_string_list (key);
	g_free (key);

	return result;
}

int
nautilus_preferences_get_user_level (void)
{
	char *key;
	char *user_level;
	int result = 0;

	/* This is a little silly, but it is technically possible
	 * to have different user_level defaults in each user level.
	 *
	 * This is a consequence of using gconf to store the user
	 * level itself.  So, we special case the "user_level" setting
	 * to always return the default for the first user level.
	 */
	if (preferences_preference_is_default ("user_level")) {
		key = preferences_key_make_for_default ("user_level", 0);
	} else {
		key = preferences_key_make ("user_level");
	}

	user_level = nautilus_gconf_get_string (key);
	g_free (key);

	if (nautilus_str_is_equal (user_level, "hacker")) {
		result = 2;
	} else if (nautilus_str_is_equal (user_level, "intermediate")) {
		result = 1;
	} else {
		result = 0;
	}
	
	return result;
}

void
nautilus_preferences_set_user_level (int user_level)
{
	char *user_level_key;

	user_level = preferences_user_level_check_range (user_level);

	user_level_key = preferences_get_user_level_key ();
	nautilus_gconf_set_string (user_level_key, user_level_names_for_storage[user_level]);
	g_free (user_level_key);

	nautilus_gconf_suggest_sync ();
}

void
nautilus_preferences_default_set_integer (const char *name,
					  int user_level,
					  int int_value)
{
	char *default_key;

	g_return_if_fail (name != NULL);
	
	default_key = preferences_key_make_for_default (name, user_level);
	nautilus_gconf_set_integer (default_key, int_value);
	g_free (default_key);
}

int
nautilus_preferences_default_get_integer (const char *name,
					  int user_level)
{
 	int result;
	char *default_key;

	g_return_val_if_fail (name != NULL, 0);
	
	default_key = preferences_key_make_for_default (name, user_level);
	result = nautilus_gconf_get_integer (default_key);
	g_free (default_key);

	return result;
}

void
nautilus_preferences_default_set_boolean (const char *name,
					  int user_level,
					  gboolean boolean_value)
{
	char *default_key;
	
	g_return_if_fail (name != NULL);
	
	default_key = preferences_key_make_for_default (name, user_level);
	nautilus_gconf_set_boolean (default_key, boolean_value);
	g_free (default_key);
}

gboolean
nautilus_preferences_default_get_boolean (const char *name,
					  int user_level)
{
 	gboolean result;
	char *default_key;

	g_return_val_if_fail (name != NULL, FALSE);
	
	default_key = preferences_key_make_for_default (name, user_level);
	result = nautilus_gconf_get_boolean (default_key);
	g_free (default_key);

	return result;
}

void
nautilus_preferences_default_set_string (const char *name,
					 int user_level,
					 const char *string_value)
{
	char *default_key;
	
	g_return_if_fail (name != NULL);
	
	default_key = preferences_key_make_for_default (name, user_level);
	nautilus_gconf_set_string (default_key, string_value);
	g_free (default_key);
}

char *
nautilus_preferences_default_get_string (const char *name,
					 int user_level)
{
 	char *result;
	char *default_key;

	g_return_val_if_fail (name != NULL, NULL);
	
	default_key = preferences_key_make_for_default (name, user_level);
	result = nautilus_gconf_get_string (default_key);
	g_free (default_key);

	return result;
}

void
nautilus_preferences_default_set_string_list (const char *name,
					      int user_level,
					      GSList *string_list_value)
{
	char *default_key;
	
	g_return_if_fail (name != NULL);
	
	default_key = preferences_key_make_for_default (name, user_level);
	nautilus_gconf_set_string_list (default_key, string_list_value);
	g_free (default_key);
}

GSList *
nautilus_preferences_default_get_string_list (const char *name,
					      int user_level)
{
 	GSList *result;
	char *default_key;
	
	g_return_val_if_fail (name != NULL, NULL);
	
	default_key = preferences_key_make_for_default (name, user_level);
	result = nautilus_gconf_get_string_list (default_key);
	g_free (default_key);

	return result;
}

/**
 * preferences_callback_entry_invoke_function
 *
 * A function that invokes a callback from the given struct.  It is meant to be fed to 
 * g_list_foreach ()
 * @data: The list data privately maintained by the GList.
 * @callback_data: The callback_data privately maintained by the GList.
 **/
static void
preferences_callback_entry_invoke_function (gpointer data,
				     gpointer callback_data)
{
	PreferencesCallbackEntry *callback_entry;

	g_return_if_fail (data != NULL);
	
	callback_entry = data;

 	(* callback_entry->callback) (callback_entry->callback_data);
}

static void
preferences_something_changed_notice (GConfClient *client, 
				      guint connection_id, 
				      GConfEntry *entry, 
				      gpointer notice_data)
{
	PreferencesEntry *preferences_entry;

	g_return_if_fail (entry != NULL);
	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (notice_data != NULL);
	
	preferences_entry = notice_data;
	
	/* FIXME bugzilla.eazel.com 5875: 
	 * We need to make sure that the value has actually changed before 
	 * invoking the callbacks.
	 */
	/* Invoke callbacks for this entry */
	if (preferences_entry->callback_list) {
		g_list_foreach (preferences_entry->callback_list,
				preferences_callback_entry_invoke_function,
				NULL);
	}
}

static void
preferences_global_table_check_changes_function (gpointer key,
						 gpointer value,
						 gpointer user_data)
{
	PreferencesEntry *entry;

	g_return_if_fail (key != NULL);
	g_return_if_fail (value != NULL);

	entry = value;

	g_return_if_fail (entry->name != NULL);

	/* We dont worry about the 'user_level' itself for recursive reasons */
	if (preferences_preference_is_user_level (entry->name)) {
		return;
	}

	/* FIXME: We need to make sure that the value changed before 
	 *        invoking the callbacks.
	 */
#if 0
 	int user_level;
 	int visible_user_level;
 	user_level = nautilus_preferences_get_user_level ();
 	visible_user_level = nautilus_preferences_get_visible_user_level (entry->name);
#endif

	/* Invoke callbacks for this entry */
	if (entry->callback_list) {
		g_list_foreach (entry->callback_list,
				preferences_callback_entry_invoke_function,
				NULL);
	}
}

static void
preferences_user_level_changed_notice (GConfClient *client, 
				       guint connection_id, 
				       GConfEntry *gconf_entry, 
				       gpointer user_data)
{
	g_return_if_fail (gconf_entry != NULL);
	g_return_if_fail (gconf_entry->key != NULL);
	g_return_if_fail (nautilus_str_has_suffix (gconf_entry->key, "user_level"));
	
	g_hash_table_foreach (preferences_global_table_get_global (),
			      preferences_global_table_check_changes_function,
			      NULL);
}

/**
 * preferences_entry_add_callback
 *
 * Add a callback to a pref node.  Callbacks are fired whenever
 * the pref value changes.
 * @preferences_entry: The hash node.
 * @callback: The user supplied callback.
 * @callback_data: The user supplied closure.
 **/
static void
preferences_entry_add_callback (PreferencesEntry *entry,
				NautilusPreferencesCallback callback,
				gpointer callback_data)
{
	PreferencesCallbackEntry *callback_entry;

	g_return_if_fail (entry != NULL);
	g_return_if_fail (callback != NULL);

	callback_entry = g_new0 (PreferencesCallbackEntry, 1);
	callback_entry->callback = callback;
	callback_entry->callback_data = callback_data;
	
	g_return_if_fail (callback_entry != NULL);
	
	entry->callback_list = g_list_append (entry->callback_list, callback_entry);

	/*
	 * We install only one gconf notification for each preference entry.
	 * Otherwise, we would invoke the installed callbacks more than once
	 * per registered callback.
	 */
	if (entry->gconf_connection_id == 0) {
		GError *error = NULL;
		GConfClient *client;
		char *key;
		
		g_return_if_fail (entry->name != NULL);

		client = preferences_global_client_get ();

		g_return_if_fail (client != NULL);

		key = preferences_key_make (entry->name);

		entry->gconf_connection_id = gconf_client_notify_add (client,
								     key,
								     preferences_something_changed_notice,
								     entry,
								     NULL,
								     &error);
		if (nautilus_gconf_handle_error (&error)) {
			entry->gconf_connection_id = 0;
		}

		g_free (key);
	}
}

/**
 * preferences_entry_remove_callback
 *
 * remove a callback from a pref entry.  Both the callback and the callback_data must
 * match in order for a callback to be removed from the entry.
 * @preferences_entry: The hash entry.
 * @callback: The user supplied callback.
 * @callback_data: The user supplied closure.
 **/
static void
preferences_entry_remove_callback (PreferencesEntry *entry,
				   NautilusPreferencesCallback callback,
				   gpointer callback_data)
{
	GList *new_list;
	GList *iterator;

	g_return_if_fail (entry != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (entry->callback_list != NULL);
	
	new_list = g_list_copy (entry->callback_list);
	
	for (iterator = new_list; iterator != NULL; iterator = iterator->next) {
		PreferencesCallbackEntry *callback_entry = iterator->data;
		
		g_return_if_fail (callback_entry != NULL);
		
		if (callback_entry->callback == callback &&
		    callback_entry->callback_data == callback_data) {
			entry->callback_list = g_list_remove (entry->callback_list, 
							      callback_entry);
			
			preferences_callback_entry_free (callback_entry);
		}
	}

	g_list_free (new_list);
	
	/*
	 * If there are no callbacks left in the entry, remove the gconf 
	 * notification as well.
	 */
	if (entry->callback_list == NULL) {
		GConfClient *client;

		client = preferences_global_client_get ();
		
		if (entry->gconf_connection_id != 0) {
			gconf_client_notify_remove (client, entry->gconf_connection_id);
		}
		
		entry->gconf_connection_id = 0;
	}
}

/**
 * preferences_callback_entry_free
 *
 * Free a callback info struct.
 * @preferences_callback_entry: The struct to free.
 **/
static void
preferences_callback_entry_free (PreferencesCallbackEntry *callback_entry)
{
	g_return_if_fail (callback_entry != NULL);

	callback_entry->callback = NULL;
	callback_entry->callback_data = NULL;

	g_free (callback_entry);
}

/**
 * preferences_callback_entry_free_func
 *
 * A function that frees a callback info struct.  It is meant to be fed to 
 * g_list_foreach ()
 * @data: The list data privately maintained by the GList.
 * @callback_data: The callback_data privately maintained by the GList.
 **/
static void
preferences_callback_entry_free_func (gpointer	data,
			       gpointer	callback_data)
{
	g_return_if_fail (data != NULL);
	
	preferences_callback_entry_free (data);
}

/**
 * preferences_entry_free
 *
 * Free a preference hash node members along with the node itself.
 * @preferences_hash_node: The node to free.
 **/
static void
preferences_entry_free (PreferencesEntry *entry)
{
	g_return_if_fail (entry != NULL);

	if (entry->gconf_connection_id != 0) {
		GConfClient *client;

		client = preferences_global_client_get ();
		g_assert (client != NULL);

		gconf_client_notify_remove (client, entry->gconf_connection_id);
		entry->gconf_connection_id = 0;
	}
	
	nautilus_g_list_free_deep_custom (entry->callback_list,
					  preferences_callback_entry_free_func,
					  NULL);
	
	entry->callback_list = NULL;

	g_free (entry->name);
	g_free (entry->description);

	nautilus_enumeration_free (entry->enumeration);

	g_free (entry);
}

/**
 * preferences_entry_free_func
 *
 * A function that frees a pref hash node.  It is meant to be fed to 
 * g_hash_table_foreach ()
 * @key: The hash key privately maintained by the GHashTable.
 * @value: The hash value privately maintained by the GHashTable.
 * @callback_data: The callback_data privately maintained by the GHashTable.
 **/
static void
preferences_entry_free_func (gpointer key,
		      gpointer value,
		      gpointer callback_data)
{
	g_assert (value != NULL);

	preferences_entry_free (value);
}

static void
preferences_global_table_free (void)
{
	if (global_table == NULL) {
		return;
	}
	
	g_hash_table_foreach (global_table, preferences_entry_free_func, NULL);
	g_hash_table_destroy (global_table);
	global_table = NULL;
}

static GHashTable *
preferences_global_table_get_global (void)
{
	if (global_table == NULL) {
		global_table = g_hash_table_new (g_str_hash, g_str_equal);
		g_atexit (preferences_global_table_free);
	}
	
	return global_table;
}

static PreferencesEntry *
preferences_global_table_lookup (const char *name)
{
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (preferences_global_table_get_global () != NULL, NULL);
	
	return g_hash_table_lookup (preferences_global_table_get_global (), name);
}

static PreferencesEntry *
preferences_global_table_insert (const char *name)
{
	PreferencesEntry *entry;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (preferences_global_table_get_global () != NULL, NULL);
	g_return_val_if_fail (preferences_global_table_lookup (name) == NULL, NULL);
	
	entry = g_new0 (PreferencesEntry, 1);
	entry->name = g_strdup (name);

	g_hash_table_insert (preferences_global_table_get_global (), entry->name, entry);

	g_return_val_if_fail (entry == preferences_global_table_lookup (name), NULL);

	return entry;
}

static PreferencesEntry *
preferences_global_table_lookup_or_insert (const char *name)
{
	PreferencesEntry *entry;

	g_return_val_if_fail (name != NULL, NULL);
	
	entry = preferences_global_table_lookup (name);

	if (entry != NULL) {
		return entry;
	}

	entry = preferences_global_table_insert (name);
	g_assert (entry != NULL);

	return entry;
}

void
nautilus_preferences_add_callback (const char *name,
				   NautilusPreferencesCallback callback,
				   gpointer callback_data)
{
	PreferencesEntry *entry;

	g_return_if_fail (name != NULL);
	g_return_if_fail (callback != NULL);

	entry = preferences_global_table_lookup_or_insert (name);
	g_assert (entry != NULL);

	preferences_entry_add_callback (entry, callback, callback_data);
}

typedef struct
{
	char *name;
	NautilusPreferencesCallback callback;
	gpointer callback_data;
} WhileAliveData;

static void
preferences_while_alive_disconnector (GtkObject *object, gpointer callback_data)
{
	WhileAliveData *data;

	g_return_if_fail (GTK_IS_OBJECT (object));
	g_return_if_fail (callback_data != NULL);

	data = callback_data;

	nautilus_preferences_remove_callback (data->name,
					      data->callback,
					      data->callback_data);

	g_free (data->name);
	g_free (data);
}

void
nautilus_preferences_add_callback_while_alive (const char *name,
					       NautilusPreferencesCallback callback,
					       gpointer callback_data,
					       GtkObject *alive_object)
{
	WhileAliveData *data;

	g_return_if_fail (name != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (GTK_IS_OBJECT (alive_object));

	data = g_new (WhileAliveData, 1);
	data->name = g_strdup (name);
	data->callback = callback;
	data->callback_data = callback_data;

	nautilus_preferences_add_callback (name, callback, callback_data);

	gtk_signal_connect (alive_object,
			    "destroy",
			    GTK_SIGNAL_FUNC (preferences_while_alive_disconnector),
			    data);
}

static GList *remove_list = NULL;

static void
preferences_while_process_running_remover (void)
{
	GList *iterator;

	for (iterator = remove_list; iterator != NULL; iterator = iterator->next) {
		WhileAliveData *data;
		
		data = iterator->data;
		g_assert (data != NULL);

		nautilus_preferences_remove_callback (data->name,
						      data->callback,
						      data->callback_data);

		g_free (data->name);
		g_free (data);
	}

	g_list_free (remove_list);
	remove_list = NULL;
}

/*
 * nautilus_preferences_add_callback_while_process_is_running:
 *
 * @name: Preference name.
 * @callback: Callback function.
 * @callback_data: Data for callback function.
 *
 * Add a preference callback for the duration of the currently
 * running process.  The callback will be automatically removed
 * at exit time.  This is useful for preference values that need
 * to be stored be stored statically
 */
void
nautilus_preferences_add_callback_while_process_is_running (const char *name,
							    NautilusPreferencesCallback callback,
							    gpointer callback_data)
{
	static gboolean while_process_running_remover_installed = FALSE;
	WhileAliveData *data;

	g_return_if_fail (name != NULL);
	g_return_if_fail (callback != NULL);

	/* Setup the at exit disconnector once */
	if (while_process_running_remover_installed == FALSE) {
		g_atexit (preferences_while_process_running_remover);
		while_process_running_remover_installed = TRUE;
	}

	data = g_new (WhileAliveData, 1);
	data->name = g_strdup (name);
	data->callback = callback;
	data->callback_data = callback_data;
	
	nautilus_preferences_add_callback (name, callback, callback_data);

	remove_list = g_list_append (remove_list, data);
}


void
nautilus_preferences_remove_callback (const char *name,
				      NautilusPreferencesCallback callback,
				      gpointer callback_data)
{
	PreferencesEntry *entry;

	g_return_if_fail (name != NULL);
	g_return_if_fail (callback != NULL);

	entry = preferences_global_table_lookup_or_insert (name);

	if (entry == NULL) {
		g_warning ("Trying to remove a callback without adding it first.");
		return;
	}
	
	preferences_entry_remove_callback (entry, callback, callback_data);
}

void
nautilus_preferences_enumeration_insert (const char *name,
					 const char *entry,
					 const char *description,
					 int value)
{
	PreferencesEntry *preferences_entry;

	g_return_if_fail (name != NULL);
	g_return_if_fail (entry != NULL);

	preferences_entry = preferences_global_table_lookup_or_insert (name);
	g_assert (preferences_entry != NULL);

	if (preferences_entry->enumeration == NULL) {
		preferences_entry->enumeration = nautilus_enumeration_new ();
	}

	nautilus_enumeration_insert (preferences_entry->enumeration, entry, description, value);
}

char *
nautilus_preferences_enumeration_get_nth_entry (const char *name,
						guint n)
{
	PreferencesEntry *preferences_entry;

	g_return_val_if_fail (name != NULL, NULL);

	preferences_entry = preferences_global_table_lookup_or_insert (name);
	g_assert (preferences_entry != NULL);

	return nautilus_enumeration_get_nth_entry (preferences_entry->enumeration, n);
}

char *
nautilus_preferences_enumeration_get_nth_description (const char *name,
						      guint n)
{
	PreferencesEntry *preferences_entry;

	g_return_val_if_fail (name != NULL, NULL);

	preferences_entry = preferences_global_table_lookup_or_insert (name);
	g_assert (preferences_entry != NULL);

	return nautilus_enumeration_get_nth_description (preferences_entry->enumeration, n);
}

int
nautilus_preferences_enumeration_get_nth_value (const char *name,
						guint n)
{
	PreferencesEntry *preferences_entry;
	
	g_return_val_if_fail (name != NULL, 0);
	
	preferences_entry = preferences_global_table_lookup_or_insert (name);
	g_assert (preferences_entry != NULL);

	return nautilus_enumeration_get_nth_value (preferences_entry->enumeration, n);
}

guint
nautilus_preferences_enumeration_get_num_entries (const char *name)
{
	PreferencesEntry *preferences_entry;
	
	g_return_val_if_fail (name != NULL, 0);
	
	preferences_entry = preferences_global_table_lookup_or_insert (name);
	g_assert (preferences_entry != NULL);

	return nautilus_enumeration_get_num_entries (preferences_entry->enumeration);
}

void
nautilus_preferences_set_description (const char *name,
				      const char *description)
{
	PreferencesEntry *entry;

	g_return_if_fail (name != NULL);
	g_return_if_fail (description != NULL);

	entry = preferences_global_table_lookup_or_insert (name);
	g_assert (entry != NULL);

	g_free (entry->description);
	entry->description = g_strdup (description);
}

char *
nautilus_preferences_get_description (const char *name)
{
	PreferencesEntry *entry;

	g_return_val_if_fail (name != NULL, NULL);

	entry = preferences_global_table_lookup_or_insert (name);

	return g_strdup (entry->description ? entry->description : "");
}

char *
nautilus_preferences_get_user_level_name_for_display (int user_level)
{
	user_level = preferences_user_level_check_range (user_level);
	
	return g_strdup (user_level_names_for_display[user_level]);
}

char *
nautilus_preferences_get_user_level_name_for_storage (int user_level)
{
	user_level = preferences_user_level_check_range (user_level);
	
	return g_strdup (user_level_names_for_storage[user_level]);
}

static int
preferences_user_level_check_range (int user_level)
{
	user_level = MAX (user_level, 0);
	user_level = MIN (user_level, 2);

	return user_level;
}

gboolean
nautilus_preferences_monitor_directory (const char *directory)
{
	return nautilus_gconf_monitor_directory (directory);
}

gboolean
nautilus_preferences_is_visible (const char *name)
{
	int user_level;
	int visible_user_level;

	g_return_val_if_fail (name != NULL, FALSE);

	user_level = nautilus_preferences_get_user_level ();
	visible_user_level = nautilus_preferences_get_visible_user_level (name);

	return visible_user_level <= user_level;
}

#if !defined (NAUTILUS_OMIT_SELF_CHECK)
void
nautilus_self_check_preferences (void)
{
}
#endif /* !NAUTILUS_OMIT_SELF_CHECK */
