/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-glib-extensions.h - interface for new functions that conceptually
                                belong in glib. Perhaps some of these will be
                                actually rolled into glib someday.

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

   Authors: John Sullivan <sullivan@eazel.com>
*/

#ifndef NAUTILUS_GLIB_EXTENSIONS_H
#define NAUTILUS_GLIB_EXTENSIONS_H

#include <time.h>
#include <glib.h>

/* A gboolean variant for bit fields. */
typedef guint nautilus_boolean_bit;

/* Use there to start and and end a macro */
#define NAUTILUS_MACRO_BEGIN		G_STMT_START {
#define NAUTILUS_MACRO_END		} G_STMT_END

/* Use this until we can switch to G_N_ELEMENTS. */
#define NAUTILUS_N_ELEMENTS(array) (sizeof (array) / sizeof ((array)[0]))

/* Callback functions that have user data. */
typedef int      (* NautilusCompareFunction)   (gconstpointer a,
						gconstpointer b,
						gpointer callback_data);
typedef int      (* NautilusSearchFunction)    (gconstpointer item,
						gpointer callback_data);

/* Predicate. */
typedef gboolean (* NautilusPredicateFunction) (gpointer data,
						gpointer callback_data);

/* Date & time functions. */
GDate *     nautilus_g_date_new_tm                    (struct tm                  *time_pieces);
char *      nautilus_strdup_strftime                  (const char                 *format,
						       struct tm                  *time_pieces);

/* environment manipulation functions */
int         nautilus_setenv                           (const char                 *name,
						       const char                 *value,
						       gboolean                    overwrite);
void        nautilus_unsetenv                         (const char                 *name);

/* GList functions. */
gboolean    nautilus_g_list_exactly_one_item          (GList                      *list);
gboolean    nautilus_g_list_more_than_one_item        (GList                      *list);
gboolean    nautilus_g_list_equal                     (GList                      *list_a,
						       GList                      *list_b);
GList *     nautilus_g_list_copy                      (GList                      *list);
void        nautilus_g_list_safe_for_each             (GList                      *list,
						       GFunc                       function,
						       gpointer                    user_data);
GList *     nautilus_g_list_partition                 (GList                      *list,
						       NautilusPredicateFunction   predicate,
						       gpointer                    user_data,
						       GList                     **removed);
void        nautilus_g_list_free_deep_custom          (GList                      *list,
						       GFunc                       element_free_func,
						       gpointer                    user_data);

/* List functions for lists of g_free'able objects. */
void        nautilus_g_list_free_deep                 (GList                      *list);
void        nautilus_g_slist_free_deep_custom         (GSList                     *list,
						       GFunc                       element_free_func,
						       gpointer                    user_data);

/* List functions for slists of g_free'able objects. */
void        nautilus_g_slist_free_deep                (GSList                     *list);


/* List functions for lists of C strings. */
gboolean    nautilus_g_str_list_equal                 (GList                      *str_list_a,
						       GList                      *str_list_b);
GList *     nautilus_g_str_list_copy                  (GList                      *str_list);
GList *     nautilus_g_str_list_alphabetize           (GList                      *str_list);

/* GString functions */
void        nautilus_g_string_append_len              (GString                    *string,
						       const char                 *characters,
						       int                         length);

/* GHashTable functions */
GHashTable *nautilus_g_hash_table_new_free_at_exit    (GHashFunc                   hash_function,
						       GCompareFunc                key_compare_function,
						       const char                 *display_name);
void        nautilus_g_hash_table_safe_for_each       (GHashTable                 *hash_table,
						       GHFunc                      callback,
						       gpointer                    callback_data);
gboolean    nautilus_g_hash_table_remove_deep_custom  (GHashTable                 *hash_table,
						       gconstpointer               key,
						       GFunc                       key_free_func,
						       gpointer                    key_free_data,
						       GFunc                       value_free_func,
						       gpointer                    value_free_data);
gboolean    nautilus_g_hash_table_remove_deep         (GHashTable                 *hash_table,
						       gconstpointer               key);
void        nautilus_g_hash_table_destroy_deep_custom (GHashTable                 *hash_table,
						       GFunc                       key_free_func,
						       gpointer                    key_free_data,
						       GFunc                       value_free_func,
						       gpointer                    value_free_data);
void        nautilus_g_hash_table_destroy_deep        (GHashTable                 *hash_table);

/* GPtrArray functions */
GPtrArray * nautilus_g_ptr_array_new_from_list        (GList                      *list);
void        nautilus_g_ptr_array_sort                 (GPtrArray                  *array,
						       NautilusCompareFunction     compare_callback,
						       gpointer                    callback_data);
int         nautilus_g_ptr_array_search               (GPtrArray                  *array,
						       NautilusSearchFunction      search_callback,
						       gpointer                    callback_data,
						       gboolean                    match_only);

/* NULL terminated string arrays (strv). */
int         nautilus_g_strv_find                      (char                      **strv,
						       const char                 *find_me);

/* return the time in microseconds since the machine was started */
gint64      nautilus_get_system_time                  (void);

/* shell */
char *      nautilus_shell_quote                      (const char                 *string);

/* math */
int nautilus_g_round (double d);

#endif /* NAUTILUS_GLIB_EXTENSIONS_H */
