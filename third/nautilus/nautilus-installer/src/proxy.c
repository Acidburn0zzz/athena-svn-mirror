/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * Copyright (C) 2000 Eazel, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Mike Fleming  <mfleming@eazel.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <libtrilobite/trilobite-core-utils.h>
#include "proxy.h"

/**
 * set_http_proxy
 * 
 * just sets "http_proxy" environment variable, since that's all the bootstrap
 * installer needs.
 */
static gboolean
set_http_proxy (const char *proxy_url)
{
	/* set the "http_proxy" environment variable */
	trilobite_setenv ("http_proxy", proxy_url, 1);
	return TRUE;
}

/**
 * getline_dup
 *
 * reads newline or EOF terminated line from stream, allocating the return
 * buffer as appropriate
 */ 
#define GETLINE_INITIAL 256
static char * 
getline_dup (FILE* stream)
{
	char *ret;
	size_t ret_size;
	size_t ret_used;
	int char_read;
	gboolean done;

	ret = g_malloc( GETLINE_INITIAL * sizeof(char) );
	ret_size = GETLINE_INITIAL;

	for ( ret_used = 0, done = FALSE ;
	      !done && (EOF != (char_read = fgetc (stream))) ; 
	      ret_used++
	) {
		if (ret_size == (ret_used + 1)) {
			ret_size *= 2;
			ret = g_realloc (ret, ret_size); 
		}
		if ('\n' == char_read || '\r' == char_read ) {
			done = TRUE;
			ret [ret_used] = '\0';
		} else {
			ret [ret_used] = char_read;
		}
	}

	if ( 0 == ret_used ) {
		g_free (ret);
		ret = NULL;
	} else {
		ret [ret_used] = '\0';
	}

	return ret;
}


#define NETSCAPE_PREFS_PATH "/.netscape/preferences.js"

/* user_pref("network.proxy.http", "localhost");
 * user_pref("network.proxy.http_port", 8080);
 * user_pref("network.proxy.type", 1);
 */
static char *
load_nscp_proxy_settings (const char *homedir)
{
	char * prefs_path = NULL;
	char * ret = NULL;
	char * proxy_host = NULL;
	guint32 proxy_port = 8080;
	gboolean has_proxy_type = FALSE;

	char * line;
	char * current, *end;
	FILE * prefs_file;

	prefs_path = g_strdup_printf ("%s%s", homedir, NETSCAPE_PREFS_PATH);
	prefs_file = fopen (prefs_path, "r");
	if ( NULL == prefs_file ) {
		goto error;
	}

	/* Normally I wouldn't be caught dead doing it this way...*/
	for ( ; NULL != (line = getline_dup (prefs_file)) ; g_free (line) ) {
		if ( NULL != (current = strstr (line, "\"network.proxy.http\"")) ) {
			current += strlen ("\"network.proxy.http\"");

			current = strchr (current, '"');

			if (NULL == current) {
				continue;
			}
			current++;

			end = strchr (current, '"');
			if (NULL == end) {
				continue;
			}

			proxy_host = g_strndup (current, end-current);
		} else if ( NULL != (current = strstr (line, "\"network.proxy.http_port\""))) {
			current += strlen ("\"network.proxy.http_port\"");

			while ( *current && !isdigit(*current)) {
				current++;
			}

			if ( '\0' == *current ) {
				continue;
			}

			proxy_port = strtoul (current, &end, 10);

		} else if ( NULL != (current = strstr (line, "\"network.proxy.type\""))) {
			/* Proxy type must equal '1' */
			current += strlen ("\"network.proxy.type\"");

			while ( *current && !isdigit(*current)) {
				current++;
			}

			has_proxy_type = ('1' == *current);
		}
	}

	if (has_proxy_type && NULL != proxy_host) {
		ret = g_strdup_printf ("http://%s:%u/", proxy_host, proxy_port);
	}
	
error:
	g_free (proxy_host);
	g_free (prefs_path);
	prefs_path = NULL;

	return ret;
}


#define GALEON_PREFS_PATH "/.gnome/galeon"

/* http_proxy=localhost
 * http_proxy_port=4128
 */

static char *
load_galeon_proxy_settings (const char *homedir)
{
	char * prefs_path = NULL;
	char * line;
	FILE * prefs_file;
	char * proxy_host = NULL;
	guint32 proxy_port = 8080;
	char * ret = NULL;

	prefs_path = g_strdup_printf ("%s%s", homedir, GALEON_PREFS_PATH);
	prefs_file = fopen (prefs_path, "r");
	if ( NULL == prefs_file ) {
		goto error;
	}

	/* i have no qualms about doing it "this way" ;) */
	for ( ; NULL != (line = getline_dup (prefs_file)) ; g_free (line) ) {
		if ((g_strncasecmp (line, "http_proxy=", 11) == 0) && (strlen (line+11))) {
			proxy_host = g_strdup (line+11);
		}
		if ((g_strncasecmp (line, "http_proxy_port=", 16) == 0) && (strlen (line+16))) {
			proxy_port = strtoul (line+16, NULL, 10);
		}
	}

	if (proxy_host != NULL) {
		ret = g_strdup_printf ("http://%s:%u", proxy_host, proxy_port);
	}

error:
	g_free (proxy_host);
	g_free (prefs_path);
	prefs_path = NULL;

	return ret;
}


/**
 * attempt_http_proxy_autoconfigure
 *
 * Attempt to discover HTTP proxy settings from environment variables
 * and Netscape 4.x configuation files
 */
gboolean
attempt_http_proxy_autoconfigure (const char *homedir)
{
	static gboolean autoconfigure_attempted = FALSE;
	gboolean success = FALSE;
	char * proxy_url;

	/* If we've already failed once, we're not going to try again */
	if (autoconfigure_attempted) {
		return FALSE;
	}
	
	/* The "http_proxy" environment variable is used by libwww */

	/* Note that g_getenv returns a pointer to a static buffer */
	proxy_url = g_getenv ("http_proxy");
	if (NULL != proxy_url) {
		success = TRUE;
		set_http_proxy (proxy_url);
		g_free (proxy_url);
		proxy_url = NULL;
		goto done;
	}

	/* Check Netscape 4.x settings */
	proxy_url = load_nscp_proxy_settings (homedir);
	if (NULL != proxy_url) {
		success = TRUE;
		set_http_proxy (proxy_url);
		g_free (proxy_url);
		proxy_url = NULL;
		goto done;
	}

	proxy_url = load_galeon_proxy_settings (homedir);
	if (NULL != proxy_url) {
		success = TRUE;
		set_http_proxy (proxy_url);
		g_free (proxy_url);
		proxy_url = NULL;
		goto done;
	}

done:
	autoconfigure_attempted = TRUE;
	return success;
}

