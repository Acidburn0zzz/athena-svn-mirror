/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Ramiro Estrugo <ramiro@eazel.com>
 *  	     Mike Fleming <mfleming@eazel.com>
 *
 */

/*
 * nautilus-mozilla-content-view.c - Mozilla content view component.
 *
 * This component uses the mozilla gecko layout engine via the gtk_moz_embed
 * widget to display and munge html.
 */

#define nopeDEBUG_ramiro 1
#define nopeDEBUG_mfleming 1

#include <config.h>
#include "nautilus-mozilla-content-view.h"

#include "gtkmozembed.h"

#include "mozilla-preferences.h"
#include "mozilla-components.h"
#include "mozilla-events.h"

#include <bonobo/bonobo-control.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomevfs/gnome-vfs.h>
#include <stdlib.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>

#ifdef EAZEL_SERVICES
#include <libtrilobite/libammonite-gtk.h>
#endif

#define NUM_ELEMENTS_IN_ARRAY(_a) (sizeof (_a) / sizeof ((_a)[0]))

/* Code-copied from nsGUIEvent.h */

enum nsEventStatus {  
	/* The event is ignored, do default processing */
 	nsEventStatus_eIgnore,            
 	/* The event is consumed, don't do default processing */
 	nsEventStatus_eConsumeNoDefault, 
 	/* The event is consumed, but do default processing */
	nsEventStatus_eConsumeDoDefault  
};
 
#define NS_DOM_EVENT_IGNORED ((enum nsEventStatus)nsEventStatus_eIgnore)
#define NS_DOM_EVENT_CONSUMED ((enum nsEventStatus)nsEventStatus_eConsumeNoDefault)

#ifdef EAZEL_SERVICES
EazelProxy_UserControl nautilus_mozilla_content_view_user_control = CORBA_OBJECT_NIL;
#endif

struct NautilusMozillaContentViewDetails {
	char *uri;
	GtkWidget *mozilla;
	NautilusView *nautilus_view;
	GdkCursor *busy_cursor;
	gboolean got_called_by_nautilus;
};

static void     nautilus_mozilla_content_view_initialize_class (NautilusMozillaContentViewClass *klass);
static void     nautilus_mozilla_content_view_initialize       (NautilusMozillaContentView      *view);
static void     nautilus_mozilla_content_view_destroy          (GtkObject                       *object);
static void     mozilla_load_location_callback                 (NautilusView                    *nautilus_view,
								const char                      *location,
								NautilusMozillaContentView      *view);

/* Mozilla embed widget callbacks */
static void     mozilla_title_changed_callback                 (GtkMozEmbed                     *mozilla,
								gpointer                         user_data);
static void     mozilla_location_changed_callback              (GtkMozEmbed                     *mozilla,
								gpointer                         user_data);
static void     mozilla_net_state_callback                     (GtkMozEmbed                     *mozilla,
								gint                             state,
								guint                            status,
								gpointer                         user_data);
static void     mozilla_net_stop_callback                      (GtkMozEmbed                     *mozilla,
								gpointer                         user_data);
static void     mozilla_link_message_callback                  (GtkMozEmbed                     *mozilla,
								gpointer                         user_data);
static void     mozilla_progress_callback                      (GtkMozEmbed                     *mozilla,
								gint                             max_progress,
								gint                             current_progress,
								gpointer                         user_data);
static  gint    mozilla_open_uri_callback                      (GtkMozEmbed                     *mozilla,
								const char                      *uri,
								gpointer                         user_data);
static  gint    mozilla_dom_key_press_callback                 (GtkMozEmbed                     *mozilla,
								gpointer                         dom_event,
								gpointer                         user_data);
static  gint    mozilla_dom_mouse_click_callback               (GtkMozEmbed                     *mozilla,
								gpointer                         dom_event,
								gpointer                         user_data);


/* Other mozilla content view functions */
static void     mozilla_content_view_set_busy_cursor           (NautilusMozillaContentView      *view);
static void     mozilla_content_view_clear_busy_cursor         (NautilusMozillaContentView      *view);
static gboolean mozilla_is_uri_handled_by_nautilus             (const char                      *uri);
static gboolean mozilla_is_uri_handled_by_mozilla              (const char                      *uri);
static char *   mozilla_translate_uri_if_needed                (NautilusMozillaContentView      *view,
								const char                      *uri);
static char *   mozilla_untranslate_uri_if_needed              (NautilusMozillaContentView      *view,
								const char                      *uri);
static void     mozilla_content_view_one_time_happenings       (void);
static void     mozilla_content_view_setup_profile_directory   (void);

#ifdef EAZEL_SERVICES

/*
 * URL scheme hack for the eazel-services: scheme
 */

static char *
eazel_services_scheme_translate	(NautilusMozillaContentView	*view,
				 const char			*uri,
				 gboolean			is_retry);

static char *
eazel_services_scheme_untranslate (NautilusMozillaContentView	*view, 
				   const char			*uri);

#ifdef EAZEL_SERVICES_MOZILLA_MODAL_LOGIN
typedef struct  {
	NautilusMozillaContentView *view;
	char *uri;
} LoginAsyncState;

static void /* AmmonitePromptLoginCb */
eazel_services_prompt_login_cb (
	gpointer user_data, 
	const EazelProxy_User *user, 
	const EazelProxy_AuthnFailInfo *fail_info
);
#endif /* EAZEL_SERVICES_MOZILLA_MODAL_LOGIN */

#endif /* EAZEL_SERVICES */


static GtkVBoxClass *parent_class = NULL;

static void
nautilus_mozilla_content_view_initialize_class (NautilusMozillaContentViewClass *klass)
{
	GtkObjectClass *object_class;
	
	object_class = GTK_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (GTK_TYPE_VBOX);
	
	object_class->destroy = nautilus_mozilla_content_view_destroy;

	mozilla_content_view_setup_profile_directory ();
}

static void
nautilus_mozilla_content_view_initialize (NautilusMozillaContentView *view)
{
	view->details = g_new0 (NautilusMozillaContentViewDetails, 1);

	/* Conjure up the beast.  May God have mercy on our souls. */
	view->details->mozilla = gtk_moz_embed_new ();

	/* Do preference/environment setup that needs to happen only once.
	 * We need to do this right after the first gtkmozembed widget gets
	 * created, otherwise the mozilla runtime environment is not properly
	 * setup.
	 */
	mozilla_content_view_one_time_happenings ();

	/* Add callbacks to the beast */
	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"title",
					GTK_SIGNAL_FUNC (mozilla_title_changed_callback),
					view,
					GTK_OBJECT (view->details->mozilla));

	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"location",
					GTK_SIGNAL_FUNC (mozilla_location_changed_callback),
					view,
					GTK_OBJECT (view->details->mozilla));

	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"net_state",
					GTK_SIGNAL_FUNC (mozilla_net_state_callback),
					view,
					GTK_OBJECT (view->details->mozilla));

	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"net_stop",
					GTK_SIGNAL_FUNC (mozilla_net_stop_callback),
					view,
					GTK_OBJECT (view->details->mozilla));
	
	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"link_message",
					GTK_SIGNAL_FUNC (mozilla_link_message_callback),
					view,
					GTK_OBJECT (view->details->mozilla));
	
	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"progress",
					GTK_SIGNAL_FUNC (mozilla_progress_callback),
					view,
					GTK_OBJECT (view->details->mozilla));
	
	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"open_uri",
					GTK_SIGNAL_FUNC (mozilla_open_uri_callback),
					view,
					GTK_OBJECT (view->details->mozilla));

	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"dom_key_press",
					GTK_SIGNAL_FUNC (mozilla_dom_key_press_callback),
					view,
					GTK_OBJECT (view->details->mozilla));
	
	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->mozilla), 
					"dom_mouse_click",
					GTK_SIGNAL_FUNC (mozilla_dom_mouse_click_callback),
					view,
					GTK_OBJECT (view->details->mozilla));
	
	gtk_box_pack_start (GTK_BOX (view), view->details->mozilla, TRUE, TRUE, 1);
	
	gtk_widget_show (view->details->mozilla);
	
	view->details->nautilus_view = nautilus_view_new (GTK_WIDGET (view));
	
	gtk_signal_connect_while_alive (GTK_OBJECT (view->details->nautilus_view), 
					"load_location",
					GTK_SIGNAL_FUNC (mozilla_load_location_callback), 
					view,
					GTK_OBJECT (view->details->mozilla));

	gtk_widget_show_all (GTK_WIDGET (view));
}

static void
nautilus_mozilla_content_view_destroy (GtkObject *object)
{
	NautilusMozillaContentView *view;

#ifdef DEBUG_ramiro
	g_print ("%s()\n", __FUNCTION__);
#endif

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (object);

	g_free (view->details->uri);

	if (view->details->busy_cursor != NULL) {
		gdk_cursor_destroy (view->details->busy_cursor);
	}

	g_free (view->details);

	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

/**
 * nautilus_mozilla_content_view_get_nautilus_view:
 *
 * Return the NautilusView object associated with this view; this
 * is needed to export the view via CORBA/Bonobo.
 * @view: NautilusMozillaContentView to get the nautilus_view from..
 * 
 **/
NautilusView *
nautilus_mozilla_content_view_get_nautilus_view (NautilusMozillaContentView *view)
{
	return view->details->nautilus_view;
}


/**
 * mozilla_vfs_read_callback:
 *
 * Read data from buffer and copy into mozilla stream.
 **/

static char vfs_read_buf[40960];

static void
mozilla_vfs_read_callback (GnomeVFSAsyncHandle *handle, GnomeVFSResult result, gpointer buffer,
			   GnomeVFSFileSize bytes_requested,
			   GnomeVFSFileSize bytes_read,
			   gpointer data)
{
	NautilusMozillaContentView *view = data;

#ifdef DEBUG_ramiro
	g_print ("mozilla_vfs_read_callback: %ld/%ld bytes\n", (long) bytes_read, (long) bytes_requested);
#endif
	nautilus_view_report_load_underway (view->details->nautilus_view);
	if (bytes_read != 0) {
		gtk_moz_embed_append_data (GTK_MOZ_EMBED (view->details->mozilla), buffer, bytes_read);
	}

	if (bytes_read == 0 || result != GNOME_VFS_OK) {
		gtk_moz_embed_close_stream (GTK_MOZ_EMBED (view->details->mozilla));
		gnome_vfs_async_close (handle, (GnomeVFSAsyncCloseCallback) gtk_true, NULL);
		nautilus_view_report_load_complete (view->details->nautilus_view);
		return;
    	}
	
	gnome_vfs_async_read (handle, vfs_read_buf, sizeof (vfs_read_buf), mozilla_vfs_read_callback, view);
}

/**
 * mozilla_vfs_callback:
 *
 * Callback for gnome_vfs_async_open. Attempt to read data from handle
 * and pass to mozilla streaming callback.
 * 
 **/
static void
mozilla_vfs_callback (GnomeVFSAsyncHandle *handle, GnomeVFSResult result, gpointer data)
{
	NautilusMozillaContentView *view = data;

#ifdef DEBUG_ramiro
	g_print ("mozilla_vfs_callback, result was %s\n", gnome_vfs_result_to_string (result));
#endif

	if (result != GNOME_VFS_OK)
	{
		nautilus_view_report_load_failed (view->details->nautilus_view);
		gtk_moz_embed_close_stream (GTK_MOZ_EMBED (view->details->mozilla));
	} else {
		gtk_moz_embed_open_stream (GTK_MOZ_EMBED (view->details->mozilla), "file://", "text/html");
		gnome_vfs_async_read (handle, vfs_read_buf, sizeof (vfs_read_buf), mozilla_vfs_read_callback, view);
	}
}

#if 0
/* Check whether two uris are "equal".  Equal means:
 *
 * Same uri regardless of mozilla canonicalization
 *
 * What happens is that mozilla internall canonicalizes
 * uris and appends trailing slashes to those that it 
 * knows to be directories.  Unfortunately this is 
 * different from Nautilus.
 */
static gboolean 
uris_are_equal (const char *uri_one, const char *uri_two)
{
	guint length_one;
	guint length_two;

	if (uri_one == NULL && uri_two == NULL) {
		return TRUE;
	}

	if (uri_one == NULL || uri_two == NULL) {
		return FALSE;
	}

	length_one = strlen (uri_one);
	length_two = strlen (uri_two);

	if (length_one == 0 && length_two == 0) {
		return TRUE;
	}

	if (length_one == 0 || length_two == 0) {
		return FALSE;
	}

	/* Drop the trailing slashes */
	if (uri_one[length_one - 1] == '/') {
		length_one--;
	}

	if (uri_two[length_two - 1] == '/') {
		length_two--;
	}

	if (length_one != length_two) {
		return FALSE;
	}

	return strncmp (uri_one, uri_two, MIN (length_one, length_two)) == 0;
}
#endif
 
/**
 * nautilus_mozilla_content_view_load_uri:
 *
 * Load the resource pointed to by the specified URI.
 * @view: NautilusMozillaContentView to get the nautilus_view from.
 * 
 **/
void
nautilus_mozilla_content_view_load_uri (NautilusMozillaContentView	*view,
					const char			*uri)
{
	GnomeVFSAsyncHandle *async_handle;
	gboolean same_uri = FALSE;

	g_assert (uri != NULL);

#ifdef DEBUG_mfleming
	g_print ("+%s uri='%s'\n", __FUNCTION__, uri);
#endif

	view->details->got_called_by_nautilus = TRUE;

	/* Check whether its the same uri.  Ignore the mozilla
	 * added canonicalization.
	 */

	/* FIXME bugzilla.eazel.com 2780: 
	 * Reload is broken.
	 */
	/* same_uri = uris_are_equal (view->details->uri, uri); */
	same_uri = FALSE;

	if (view->details->uri) {
		g_free (view->details->uri);
	}
	
	view->details->uri = g_strdup (uri);

#ifdef DEBUG_ramiro
	g_print ("nautilus_mozilla_content_view_load_uri (uri = %s, same = %d)\n", 
		 view->details->uri, same_uri);
#endif
	/* NOTE: (mfleming) I placed this here to fix bugzilla.eazel.com 5249
	 * The belief is that calling load_underway here will cause
	 * the Bonobo::Control::realize call to be made while this call is
	 * pending, rather than later.  It's somewhat safer for that to occur
	 * here.  Anyway, no harm is done: this is a totally legit place
	 * to call load_underway
	 */

	nautilus_view_report_load_underway (view->details->nautilus_view);

	/* If the request can be handled by mozilla, pass the uri as is.  Otherwise,
	 * use gnome-vfs to open the uri and later stream the data into the gtkmozembed
	 * widget.
	 */
	if (mozilla_is_uri_handled_by_mozilla (uri)) {
		if (same_uri) {
			gtk_moz_embed_reload (GTK_MOZ_EMBED (view->details->mozilla),
					      GTK_MOZ_EMBED_FLAG_RELOADBYPASSCACHE);
		}
		else {
			gtk_moz_embed_load_url (GTK_MOZ_EMBED (view->details->mozilla),
						view->details->uri);
		}
	} else {
		gnome_vfs_async_open (&async_handle, uri, GNOME_VFS_OPEN_READ, mozilla_vfs_callback, view);	
	}
}

static void
mozilla_content_view_set_busy_cursor (NautilusMozillaContentView *view)
{
        g_return_if_fail (NAUTILUS_IS_MOZILLA_CONTENT_VIEW (view));

	if (!view->details->busy_cursor) {
		view->details->busy_cursor = gdk_cursor_new (GDK_WATCH);
	}

	g_assert (view->details->busy_cursor != NULL);
	g_assert (GTK_WIDGET_REALIZED (GTK_WIDGET (view->details->mozilla)));

	gdk_window_set_cursor (GTK_WIDGET (view->details->mozilla)->window, 
			       view->details->busy_cursor);

	gdk_flush ();
}

static void
mozilla_content_view_clear_busy_cursor (NautilusMozillaContentView *view)
{
        g_return_if_fail (NAUTILUS_IS_MOZILLA_CONTENT_VIEW (view));

	g_assert (GTK_WIDGET_REALIZED (GTK_WIDGET (view->details->mozilla)));
	
	gdk_window_set_cursor (GTK_WIDGET (view->details->mozilla)->window, NULL);
}

static void
mozilla_load_location_callback (NautilusView *nautilus_view, 
				const char *location,
				NautilusMozillaContentView *view)
{
	char *translated_location;

	g_assert (nautilus_view == view->details->nautilus_view);

#ifdef DEBUG_mfleming
	g_print ("+%s location='%s'\n", __FUNCTION__, location);
#endif

	translated_location = mozilla_translate_uri_if_needed (view, location);

	if (translated_location) {
#ifdef DEBUG_ramiro
		g_print ("%s(%s,%s)\n", __FUNCTION__, location, translated_location);
#endif
		
		nautilus_view_report_load_underway (nautilus_view);
		nautilus_mozilla_content_view_load_uri (view, translated_location);
		g_free (translated_location);
	} else {
		nautilus_view_report_load_failed ( nautilus_view);
	}
}

/* Mozilla embed widget callbacks */
static void
mozilla_title_changed_callback (GtkMozEmbed *mozilla, gpointer user_data)
{
 	NautilusMozillaContentView	*view;
	char				*new_title;

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

	new_title = gtk_moz_embed_get_title (GTK_MOZ_EMBED (view->details->mozilla));

	if (new_title != NULL && strcmp (new_title, "") != 0) {
		nautilus_view_set_title (view->details->nautilus_view,
					 new_title);
	}
	
	g_free (new_title);
}

static gboolean
mozilla_uris_differ_only_by_fragment_identifier (const char *uri1, const char *uri2)
{
	char *uri1_hash;
	char *uri2_hash;
	gboolean ret = FALSE;

	uri1_hash = strchr (uri1, '#');
	uri2_hash = strchr (uri2, '#');

	if (uri1_hash == NULL && uri2_hash == NULL) {
		/* neither has a fragment identifier - return true
		 * only if they match exactly 
		 */
		ret = (strcasecmp (uri1, uri2) == 0);
	} else if (uri1_hash == NULL) {
		/* only the second has an fragment identifier - return
		 * true only if the part before the fragment
		 * identifier matches the first URI 
		 */
		ret = (strncasecmp (uri1, uri2, uri2_hash - uri2) == 0);
	} else if (uri2_hash == NULL) {
		/* only the first has a fragment identifier - return
		 * true only if the part before the fragment
		 * identifier matches the second URI 
		 */
		ret = (strncasecmp (uri1, uri2, uri1_hash - uri1) == 0);
	} else if (uri1_hash - uri1 == uri2_hash - uri2) {
		/* both have a fragment identifier - return true only
		 * if the parts before the fragment identifier match
		 */
		ret = (strncasecmp (uri1, uri2, uri1_hash - uri1) == 0);
	}

#ifdef DEBUG_ramiro
	 if (ret) {
	 	g_print("%s: returning TRUE for URI's '%s' and '%s'\n", __FUNCTION__, uri1, uri2);
	 }
#endif
	
	return ret;
}

static void
mozilla_location_changed_callback (GtkMozEmbed *mozilla, gpointer user_data)
{
 	NautilusMozillaContentView	*view;
	char				*new_location;


	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

	new_location = gtk_moz_embed_get_location (GTK_MOZ_EMBED (view->details->mozilla));

#ifdef DEBUG_mfleming
	g_print ("+%s current moz location='%s'\n", __FUNCTION__, new_location);
#endif

#ifdef DEBUG_ramiro
	g_print ("mozilla_location_changed_callback (%s)\n", new_location);
#endif

	/* If we only changed anchors in the same page, we should
           report some fake progress to Nautilus. */

	if (mozilla_uris_differ_only_by_fragment_identifier (view->details->uri,
							     new_location)) {
		nautilus_view_report_load_underway (view->details->nautilus_view);
		nautilus_view_report_load_complete (view->details->nautilus_view);
	}

	if (view->details->uri) {
		g_free (view->details->uri);
	}
	
	view->details->uri = new_location;
}

#if defined(DEBUG_ramiro)

#define PRINT_FLAG(bits, mask, message)		\
G_STMT_START {					\
  if ((bits) & (mask)) {			\
	  g_print ("%s ", (message));		\
  }						\
} G_STMT_END

static void
debug_print_state_flags (gint state_flags)
{
	g_print ("state_flags = ");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_START, "start");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_REDIRECTING, "redirecting");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_TRANSFERRING, "transferring");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_NEGOTIATING, "negotiating");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_STOP, "stop");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_IS_REQUEST, "is_request");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_IS_DOCUMENT, "is_document");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_IS_NETWORK, "is_network");
	PRINT_FLAG (state_flags, GTK_MOZ_EMBED_FLAG_IS_WINDOW, "is_window");
	g_print ("\n");
}

static void
debug_print_status_flags (guint status_flags)
{
	g_print ("status_flags = ");
	PRINT_FLAG (status_flags, GTK_MOZ_EMBED_STATUS_FAILED_DNS, "failed_dns");
	PRINT_FLAG (status_flags, GTK_MOZ_EMBED_STATUS_FAILED_CONNECT, "failed_connect");
	PRINT_FLAG (status_flags, GTK_MOZ_EMBED_STATUS_FAILED_TIMEOUT, "failed_timeout");
	PRINT_FLAG (status_flags, GTK_MOZ_EMBED_STATUS_FAILED_USERCANCELED, "failed_usercanceled");
	g_print ("\n");
}


#endif


static void
mozilla_net_state_callback (GtkMozEmbed	*mozilla,
			    gint	state_flags,
			    guint	status_flags,
			    gpointer	user_data)
{
 	NautilusMozillaContentView	*view;

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

#if defined(DEBUG_mfleming)
	g_print ("gtkembedmoz signal: 'net_state'\n");
#endif
#if defined(DEBUG_ramiro)
	g_print ("%s\n", __FUNCTION__);
	debug_print_state_flags (state_flags);
	debug_print_status_flags (status_flags);
	g_print ("\n\n");
#endif

	/* win_start */
	if (state_flags & GTK_MOZ_EMBED_FLAG_START) {
		mozilla_content_view_set_busy_cursor (view);
	}

	/* win_stop */
	if (state_flags & GTK_MOZ_EMBED_FLAG_STOP) {
		mozilla_content_view_clear_busy_cursor (view);
	}
}


static void
mozilla_net_stop_callback (GtkMozEmbed 	*mozilla,
			   gpointer	user_data)
{
 	NautilusMozillaContentView	*view;

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

#if defined(DEBUG_mfleming)
	g_print ("gtkembedmoz signal: 'net_stop'\n");
#endif

	nautilus_view_report_load_complete (view->details->nautilus_view);
}

static void
mozilla_link_message_callback (GtkMozEmbed *mozilla, gpointer user_data)
{
 	NautilusMozillaContentView	*view;
	char				*link_message;
	char				*translated_link_message;

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

	link_message = gtk_moz_embed_get_link_message (GTK_MOZ_EMBED (view->details->mozilla));

	/* This is actually not that efficient */
	translated_link_message = mozilla_untranslate_uri_if_needed (view, link_message);

	nautilus_view_report_status (view->details->nautilus_view,
				     translated_link_message);
	g_free (translated_link_message);
	g_free (link_message);
}

static void
mozilla_progress_callback (GtkMozEmbed *mozilla,
			   gint         current_progress,
			   gint         max_progress,
			   gpointer     user_data)
{
 	NautilusMozillaContentView	*view;

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);
	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

#ifdef DEBUG_ramiro
	g_print ("mozilla_progress_callback (max = %d, current = %d)\n", max_progress, current_progress);
#endif

	/* NOTE:
	 * "max_progress" will be -1 if the filesize cannot be determined
	 * On occasion, it appears that current_progress may actuall exceed max_progress
	 */

	if (max_progress == -1 || max_progress == 0) {
		nautilus_view_report_load_progress (view->details->nautilus_view, 
							    0);
	} else if (max_progress < current_progress) {
		nautilus_view_report_load_progress (view->details->nautilus_view, 
							    1.0);
	} else {
		nautilus_view_report_load_progress (view->details->nautilus_view, 
							    current_progress / max_progress);
	}
}


static gint
mozilla_open_uri_callback (GtkMozEmbed *mozilla,
			   const char	*uri,
			   gpointer	user_data)
{
	gint                            abort_uri_open;
 	NautilusMozillaContentView     *view;
	/*FIXME this static is shared between multiple browser windows! */
	static gboolean                 do_nothing = FALSE;

	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_assert (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla));

	/* Determine whether we want to abort this uri load */
	abort_uri_open = mozilla_is_uri_handled_by_nautilus (uri);

#ifdef DEBUG_mfleming
	g_print ("+%s do_nothing = %d, got_called_by_nautilus = %d\n", __FUNCTION__, do_nothing,  view->details->got_called_by_nautilus);
#endif

	/* FIXME: I believe that this will cause a navigation from
	 * a page containing a specific iframe URL to that same URL
	 * to fail.
	 * 
	 * This check is here because we get an "open_uri" call from GtkEmbedMoz
	 * in M18 when mozilla opens locations that appear as IFRAME's
	 * within the current document.  If we didn't do this check, we would
	 * cause Nautilus to navigate to these IFRAME url's
	 */

	if ( mozilla_events_is_url_in_iframe (mozilla, uri)) {
#ifdef DEBUG_mfleming
		g_print ("URI is in an iframe;ignoring '%s'\n", uri);
#endif
		return FALSE;
	}
	
#ifdef DEBUG_ramiro
	g_print ("mozilla_open_uri_callback (uri = %s) abort = %s\n",
		 uri,
		 abort_uri_open ? "TRUE" : "FALSE");
#endif

	if (do_nothing == TRUE) {
		do_nothing = FALSE;
		view->details->got_called_by_nautilus = FALSE;
	} else if (view->details->got_called_by_nautilus == TRUE) {
		view->details->got_called_by_nautilus = FALSE;
	} else {
		char *untranslated_uri;

		/* set it so that we load next time we are called. */
	        do_nothing = TRUE;
		abort_uri_open = TRUE;

		bonobo_object_ref (BONOBO_OBJECT (view->details->nautilus_view));

		/*do untranslate here*/
		untranslated_uri = mozilla_untranslate_uri_if_needed (view, uri);
		nautilus_view_open_location_in_this_window
			(view->details->nautilus_view, untranslated_uri);
		g_free (untranslated_uri);

		bonobo_object_unref (BONOBO_OBJECT (view->details->nautilus_view));
	}

	return abort_uri_open;
}

static gboolean
is_uri_partial (const char *uri)
{
	const char *current;

	/* RFC 2396 section 3.1 */
	for (current = uri ; 
		*current
		&& 	((*current >= 'a' && *current <= 'z')
			 || (*current >= 'A' && *current <= 'Z')
			 || (*current >= '0' && *current <= '9')
			 || ('-' == *current)
			 || ('+' == *current)
			 || ('.' == *current)) ;
	     current++);

	return  !(':' == *current);
}

/*
 * Remove "./" segments
 * Compact "../" segments inside the URI
 * Remove "." at the end of the URL 
 * Leave any ".."'s at the beginning of the URI
 */

/* in case if you were wondering, this is probably one of the least time-efficient ways to do this*/
static void
remove_internal_relative_components (char *uri_current)
{
	char *segment_prev, *segment_cur;
	size_t len_prev, len_cur;

	len_prev = len_cur = 0;
	segment_prev = NULL;

	segment_cur = uri_current;

	while (*segment_cur) {
		len_cur = strcspn (segment_cur, "/");

		if (len_cur == 1 && segment_cur[0] == '.') {
			/* Remove "." 's */
			if (segment_cur[1] == '\0') {
				segment_cur[0] = '\0';
				break;
			} else {
				memmove (segment_cur, segment_cur + 2, strlen (segment_cur + 2) + 1);
				continue;
			}
		} else if (len_cur == 2 && segment_cur[0] == '.' && segment_cur[1] == '.' ) {
			/* Remove ".."'s (and the component to the left of it) that aren't at the
			 * beginning or to the right of other ..'s
			 */
			if (segment_prev) {
				if (! (len_prev == 2
				       && segment_prev[0] == '.'
				       && segment_prev[1] == '.')) {
				       	if (segment_cur[2] == '\0') {
						segment_prev[0] = '\0';
						break;
				       	} else {
						memmove (segment_prev, segment_cur + 3, strlen (segment_cur + 3) + 1);

						segment_cur = segment_prev;
						len_cur = len_prev;

						/* now we find the previous segment_prev */
						if (segment_prev == uri_current) {
							segment_prev = NULL;
						} else if (segment_prev - uri_current >= 2) {
							segment_prev -= 2;
							for ( ; segment_prev > uri_current && segment_prev[0] != '/' 
							      ; segment_prev-- );
							if (segment_prev[0] == '/') {
								segment_prev++;
							}
						}
						continue;
					}
				}
			}
		}

		/*Forward to next segment */

		if (segment_cur [len_cur] == '\0') {
			break;
		}
		 
		segment_prev = segment_cur;
		len_prev = len_cur;
		segment_cur += len_cur + 1;	
	}
	
}


/* If I had known this relative uri code would have ended up this long, I would
 * have done it a different way
 */
static char *
make_full_uri_from_relative (const char *base_uri, const char *uri)
{
	char *result = NULL;

	/* See section 5.2 in RFC 2396 */

	/* FIXME bugzilla.eazel.com 4413: This function does not take
	 * into account a BASE tag in an HTML document, so its
	 * functionality differs from what Mozilla itself would do.
	 */

	if (is_uri_partial (uri)) {
		char *mutable_base_uri;
		char *mutable_uri;

		char *uri_current;
		size_t base_uri_length;
		char *separator;

		mutable_base_uri = g_strdup (base_uri);
		uri_current = mutable_uri = g_strdup (uri);

		/* Chew off Fragment and Query from the base_url */

		separator = strrchr (mutable_base_uri, '#'); 

		if (separator) {
			*separator = '\0';
		}

		separator = strrchr (mutable_base_uri, '?');

		if (separator) {
			*separator = '\0';
		}

		if ('/' == uri_current[0] && '/' == uri_current [1]) {
			/* Relative URI's beginning with the authority
			 * component inherit only the scheme from their parents
			 */

			separator = strchr (mutable_base_uri, ':');

			if (separator) {
				separator[1] = '\0';
			}			  
		} else if ('/' == uri_current[0]) {
			/* Relative URI's beginning with '/' absolute-path based
			 * at the root of the base uri
			 */

			separator = strchr (mutable_base_uri, ':');

			/* g_assert (separator), really */
			if (separator) {
				/* If we start with //, skip past the authority section */
				if ('/' == separator[1] && '/' == separator[2]) {
					separator = strchr (separator + 3, '/');
					if (separator) {
						separator[0] = '\0';
					}
				} else {
				/* If there's no //, just assume the scheme is the root */
					separator[1] = '\0';
				}
			}
		} else if ('#' != uri_current[0]) {
			/* Handle the ".." convention for relative uri's */

			/* If there's a trailing '/' on base_url, treat base_url
			 * as a directory path.
			 * Otherwise, treat it as a file path, and chop off the filename
			 */

			base_uri_length = strlen (mutable_base_uri);
			if ('/' == mutable_base_uri[base_uri_length-1]) {
				/* Trim off '/' for the operation below */
				mutable_base_uri[base_uri_length-1] = 0;
			} else {
				separator = strrchr (mutable_base_uri, '/');
				if (separator) {
					*separator = '\0';
				}
			}

			remove_internal_relative_components (uri_current);

			/* handle the "../"'s at the beginning of the relative URI */
			while (0 == strncmp ("../", uri_current, 3)) {
				uri_current += 3;
				separator = strrchr (mutable_base_uri, '/');
				if (separator) {
					*separator = '\0';
				} else {
					/* <shrug> */
					break;
				}
			}

			/* handle a ".." at the end */
			if (uri_current[0] == '.' && uri_current[1] == '.' 
			    && uri_current[2] == '\0') {

			    	uri_current += 2;
				separator = strrchr (mutable_base_uri, '/');
				if (separator) {
					*separator = '\0';
				}
			}

			/* Re-append the '/' */
			mutable_base_uri [strlen(mutable_base_uri)+1] = '\0';
			mutable_base_uri [strlen(mutable_base_uri)] = '/';
		}

		result = g_strconcat (mutable_base_uri, uri_current, NULL);
		g_free (mutable_base_uri); 
		g_free (mutable_uri); 

#ifdef DEBUG_mfleming
		g_print ("Relative URI converted base '%s' uri '%s' to '%s'\n", base_uri, uri, result);
#endif
	} else {
		result = g_strdup (uri);
	}
	
	return result;
}

static gint
mozilla_dom_key_press_callback (GtkMozEmbed                     *mozilla,
				gpointer                         dom_event,
				gpointer                         user_data)
{
	g_return_val_if_fail (dom_event != NULL, NS_DOM_EVENT_IGNORED);

#ifdef DEBUG_mfleming
	g_print ("%s (%p)\n", __FUNCTION__, dom_event);
#endif

	/* If this keyboard event is going to trigger a URL navigation, we need
	 * to fake it out like the mouse event below
	 */

	if (mozilla_events_is_key_return (dom_event)) {
		return mozilla_dom_mouse_click_callback (mozilla, dom_event, user_data);
	} else {
		return NS_DOM_EVENT_IGNORED;
	}
}

static gint
mozilla_dom_mouse_click_callback (GtkMozEmbed *mozilla,
				  gpointer	dom_event,
				  gpointer	user_data)
{
 	NautilusMozillaContentView	*view;
	char				*href;

	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla), NS_DOM_EVENT_IGNORED);
	g_return_val_if_fail (dom_event != NULL, NS_DOM_EVENT_IGNORED);
	g_return_val_if_fail (NAUTILUS_IS_MOZILLA_CONTENT_VIEW (user_data), NS_DOM_EVENT_IGNORED);
	
	view = NAUTILUS_MOZILLA_CONTENT_VIEW (user_data);

	g_return_val_if_fail (GTK_MOZ_EMBED (mozilla) == GTK_MOZ_EMBED (view->details->mozilla), TRUE);

#ifdef DEBUG_ramiro
	g_print ("%s (%p)\n", __FUNCTION__, dom_event);
#endif

	if (mozilla_events_is_in_form_POST_submit (dom_event)) {
#ifdef DEBUG_mfleming
		g_print ("%s: is a POST submit\n", __FUNCTION__);
#endif
		
		/* So that the post event can be properly submitted without
		 * the Nautilus shell history/navigation framework breaking
		 * its context, we use a hack.  The hack is to lie Nautilus
		 * that the POST event was instigated by Nautilus.  Nautilus
		 * will the call our load_location() away without having 
		 * gnome-vfs try and do the post and thus spoiling it.
		 */
		view->details->got_called_by_nautilus = TRUE;
	} else {
		
		href = mozilla_events_get_href_for_event (dom_event);

		if (href) {
			char * href_full = NULL;

			href_full = make_full_uri_from_relative (view->details->uri, href);

			g_free (href);
			href = href_full;
			href_full = NULL;

#if 0
/* FIXME mfleming I think I should do this here, but I'm not sure */
			href_full = mozilla_untranslate_uri_if_needed (view, href);
			g_free (href);
			href = href_full;
			href_full = NULL;
#endif

#ifdef DEBUG_ramiro
			g_print ("%s() href = %s\n", __FUNCTION__, href);
#endif
			bonobo_object_ref (BONOBO_OBJECT (view->details->nautilus_view));
			nautilus_view_open_location_in_this_window
				(view->details->nautilus_view, href);
			bonobo_object_unref (BONOBO_OBJECT (view->details->nautilus_view));
			g_free (href);

			return NS_DOM_EVENT_CONSUMED;
		} else {
#ifdef DEBUG_ramiro
			g_print ("%s() no href;ignoring\n", __FUNCTION__);
#endif
		}
	}
	
	return NS_DOM_EVENT_IGNORED;
}

#define STRING_LIST_NOT_FOUND -1
static gint
string_list_get_index_of_string (const char *string_list[], guint num_strings, const char *string)
{
	guint i;

	g_return_val_if_fail (string != NULL, STRING_LIST_NOT_FOUND);
	g_return_val_if_fail (string_list != NULL, STRING_LIST_NOT_FOUND);
	g_return_val_if_fail (num_strings > 0, STRING_LIST_NOT_FOUND);
	
	for (i = 0; i < num_strings; i++) {
		g_assert (string_list[i] != NULL);
		if (strlen (string) >= strlen (string_list[i]) 
		    && (strncmp (string, string_list[i], strlen (string_list[i])) == 0)) {
			return i;
		}
	}
	
	return STRING_LIST_NOT_FOUND;
}


/*
 * The issue here is that mozilla handles some protocols "natively" just as nautilus
 * does thanks to gnome-vfs magic.
 *
 * The embedded mozilla beast provides a mechanism for aborting url loading (ie, the
 * thing that happens when a user boinks on a hyperlink).
 *
 * We use this feature to abort uri loads for the following protocol(s):
 *
 */
static gboolean
mozilla_is_uri_handled_by_nautilus (const char *uri)
{
	static const char *handled_by_nautilus[] =
	{
		"ftp",
		"eazel",
		"eazel-install",
		"eazel-pw",
		"eazel-services",
		"man",
		"info",
		"help",
		"gnome-help",
		"ghelp"
	};

	g_return_val_if_fail (uri != NULL, FALSE);
	
	return string_list_get_index_of_string (handled_by_nautilus, NUM_ELEMENTS_IN_ARRAY (handled_by_nautilus),
						uri) != STRING_LIST_NOT_FOUND;
}

/*
 * Conversly, this a list of protocols handled by mozilla.  The strategy is to check 
 * uris for these protocols and pass them through to mozilla as as.  For other uris,
 * we try to use gnome-vfs and later stream the information into the gtkmozembed 
 * widget.  For example, this is how the man: protocol works.
 */
static gboolean
mozilla_is_uri_handled_by_mozilla (const char *uri)
{
	static const char *handled_by_mozilla[] =
	{
		"http",
		"file"
	};

	return string_list_get_index_of_string (handled_by_mozilla, NUM_ELEMENTS_IN_ARRAY (handled_by_mozilla),
						uri) != STRING_LIST_NOT_FOUND;
}


/* A NULL return from this function must trigger a nautilus load error */
static char *
mozilla_translate_uri_if_needed (NautilusMozillaContentView *view, const char *uri)
{
	/* gint i; */
	char *ret;

	g_return_val_if_fail (uri != NULL, NULL);


#ifdef EAZEL_SERVICES
	if (0 == strncmp (uri, "eazel-services:", strlen ("eazel-services:"))) {
		ret = eazel_services_scheme_translate (view, uri, FALSE);

#ifdef DEBUG_mfleming
		g_message ("Mozilla: translated uri '%s' to '%s'\n", uri, ret ? ret : "<no such page>");
#endif

	} else
#endif /* EAZEL_SERVICES */
		{
		ret = g_strdup (uri);
	}

	return ret;

}

static char *
mozilla_untranslate_uri_if_needed (NautilusMozillaContentView *view, const char *uri)
{
#ifdef EAZEL_SERVICES
	return eazel_services_scheme_untranslate (view, uri);
#else
	return g_strdup (uri);
#endif /* EAZEL_SERVICES */
}

GtkType
nautilus_mozilla_content_view_get_type (void)
{
	static GtkType mozilla_content_view_type = 0;
	
	if (!mozilla_content_view_type)
	{
		static const GtkTypeInfo mozilla_content_view_info =
		{
			"NautilusMozillaContentView",
			sizeof (NautilusMozillaContentView),
			sizeof (NautilusMozillaContentViewClass),
			(GtkClassInitFunc) nautilus_mozilla_content_view_initialize_class,
			(GtkObjectInitFunc) nautilus_mozilla_content_view_initialize,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		mozilla_content_view_type = gtk_type_unique (GTK_TYPE_VBOX, &mozilla_content_view_info);
	}
	
	return mozilla_content_view_type;
}


#ifdef EAZEL_SERVICES
/* A NULL return from this function must trigger a nautilus load error */
static char *
eazel_services_scheme_translate	(NautilusMozillaContentView	*view,
				 const char			*uri,
				 gboolean			is_retry)
{
	const char *uri_minus_scheme;
	char *new_uri = NULL;
	char *ret = NULL;
	AmmoniteError err;

	if (CORBA_OBJECT_NIL == nautilus_mozilla_content_view_user_control) {
		return NULL;
	}

	/* Chew off the the scheme, leave the colon */
	uri_minus_scheme = strchr (uri, (unsigned char)':');

	g_assert (uri_minus_scheme);

	err = ammonite_http_url_for_eazel_url (uri_minus_scheme, &new_uri);

	switch (err) {
	case ERR_Success:
		ret = g_strconcat ("http", new_uri, NULL);
		g_free (new_uri);
		new_uri = NULL;
	break;

#ifdef EAZEL_SERVICES_MOZILLA_MODAL_LOGIN
	case ERR_UserNotLoggedIn: 
		if (is_retry) {
			/* We already tried this once and it didn't work */
			ret = NULL;
		} else {
			AmmoniteParsedURL *parsed = NULL;
			LoginAsyncState *p_state;

			p_state = g_new0 (LoginAsyncState, 1);

			p_state->uri = strdup (uri);
			p_state->view = view;

			parsed = ammonite_url_parse (uri);
			g_assert (parsed);
			
			/* FIXME bugzilla.eazel.com 4412: it's possible that "view" will be gone by the time the reponse comes back */
			if (! ammonite_do_prompt_login_async (parsed->user, NULL, NULL, p_state,  prompt_login_cb) ) {
				ret = NULL;
				g_free (p_state->uri);
				g_free (p_state);
			}

			ammonite_url_free (parsed);
		}
	break;
#endif /* EAZEL_SERVICES_MOZILLA_MODAL_LOGIN */

	default:
		ret = NULL;
	break;
	
	}

	return ret;
}

static char *
eazel_services_scheme_untranslate (NautilusMozillaContentView	*view, 
				   const char			*uri)
{
	AmmoniteError err;
	char *ret;

	err = ammonite_eazel_url_for_http_url (uri, &ret);

	if (ERR_Success == err) {
#ifdef DEBUG_mfleming
		g_message ("Mozilla: untranslated uri '%s' to '%s'\n", uri, ret );
#endif
	} else {
		ret = g_strdup (uri);
	}

	return ret;
}

#ifdef EAZEL_SERVICES_MOZILLA_MODAL_LOGIN
static void /* AmmonitePromptLoginCb */
eazel_services_prompt_login_cb (
	gpointer user_data, 
	const EazelProxy_User *user, 
	const EazelProxy_AuthnFailInfo *fail_info
) {
	LoginAsyncState *p_state;

	p_state = (LoginAsyncState *) user_data;

	/* Are we still at the same location? If not, forget about it */
	if ( 0 == strcmp (p_state->uri, p_state->view->details->uri)) {
		goto error;
	}

	if (fail_info) {
		/*fail_info is non-NULL on failure */
		nautilus_view_report_load_failed (p_state->view->details->nautilus_view);
		goto error;
	} else {
		eazel_services_scheme_translate (p_state->view, p_state->uri, TRUE);
	}

error:
	g_free (p_state->uri);
	g_free (p_state);
}

#endif /* EAZEL_SERVICES_MOZILLA_MODAL_LOGIN */

#endif /* EAZEL_SERVICES */

#define TEST_PARTIAL(partial, expected) \
	tmp = make_full_uri_from_relative (base_uri, partial);					\
	if ( 0 != strcmp (tmp, expected) ) {							\
		g_message ("Test failed: partial '%s' expected '%s' got '%s'", partial, expected, tmp);	\
		success = FALSE;								\
		g_free (tmp);									\
	}

gboolean test_make_full_uri_from_relative (void);

gboolean
test_make_full_uri_from_relative (void)
{
	const char * base_uri = "http://a/b/c/d;p?q";
	char *tmp;
	gboolean success = TRUE;

	/* Commented out cases do no t work and should; they are marked above
	 * as fixmes
	 */

	/* From the RFC */

	TEST_PARTIAL ("g", "http://a/b/c/g");
	TEST_PARTIAL ("./g", "http://a/b/c/g");
	TEST_PARTIAL ("g/", "http://a/b/c/g/");
	TEST_PARTIAL ("/g", "http://a/g");

	TEST_PARTIAL ("//g", "http://g");
	
	TEST_PARTIAL ("?y", "http://a/b/c/?y");
	TEST_PARTIAL ("g?y", "http://a/b/c/g?y");
	TEST_PARTIAL ("#s", "http://a/b/c/d;p#s");
	TEST_PARTIAL ("g#s", "http://a/b/c/g#s");
	TEST_PARTIAL ("g?y#s", "http://a/b/c/g?y#s");
	TEST_PARTIAL (";x", "http://a/b/c/;x");
	TEST_PARTIAL ("g;x", "http://a/b/c/g;x");
	TEST_PARTIAL ("g;x?y#s", "http://a/b/c/g;x?y#s");

	TEST_PARTIAL (".", "http://a/b/c/");
	TEST_PARTIAL ("./", "http://a/b/c/");

	TEST_PARTIAL ("..", "http://a/b/");
	TEST_PARTIAL ("../g", "http://a/b/g");
	TEST_PARTIAL ("../..", "http://a/");
	TEST_PARTIAL ("../../", "http://a/");
	TEST_PARTIAL ("../../g", "http://a/g");

	/* Others */
	TEST_PARTIAL ("g/..", "http://a/b/c/");
	TEST_PARTIAL ("g/../", "http://a/b/c/");
	TEST_PARTIAL ("g/../g", "http://a/b/c/g");

	return success;
}

static void
ensure_profile_dir (void)
{
	char *profile_dir;
	
	profile_dir = g_strdup_printf ("%s/.nautilus/MozillaProfile", g_get_home_dir ());

	/* Make sure the profile directory exists */
	mkdir (profile_dir, 0777);
	
	g_free (profile_dir);
}

static void
ensure_cache_dir (void)
{
	char *cache_dir;

	ensure_profile_dir ();
	
	cache_dir = g_strdup_printf ("%s/.nautilus/MozillaProfile/Cache", g_get_home_dir ());

	/* Make sure the cache directory exists */
	mkdir (cache_dir, 0777);
	
	mozilla_preference_set ("browser.cache.directory", cache_dir);
	
	g_free (cache_dir);
}

static void
mozilla_content_view_one_time_happenings (void)
{
	static gboolean once = FALSE;

	if (once == TRUE) {
		return;
	}

	once = TRUE;

	/* Tell the security manager to allow ftp:// and file:// content through. */
	mozilla_preference_set_boolean ("security.checkloaduri", FALSE);

	/* Change http protocol user agent to include the string 'Nautilus' */
	mozilla_preference_set ("general.useragent.misc", "Nautilus/1.0PR3");

	/* We dont want to use the proxy for localhost */
	mozilla_preference_set ("network.proxy.no_proxies_on", "localhost");

	/* Setup routing of proxy preferences from gconf to mozilla */
	mozilla_gconf_listen_for_proxy_changes ();

	ensure_cache_dir ();
}

/* The "Mozilla Profile" directory is the place where mozilla stores 
 * things like cookies and cache.  Here we tell the mozilla embedding
 * widget to use ~/.nautilus/MozillaProfile for this purpose.
 *
 * We need mozilla M19 to support this feature.
 */
static void
mozilla_content_view_setup_profile_directory (void)
{
#if (MOZILLA_MILESTONE >= 19)
	const char *profile_name = "MozillaProfile";
	char *profile_dir;
	
	profile_dir = g_strdup_printf ("%s/.nautilus", g_get_home_dir ());

	/* Its a bug in mozilla embedding that we need to cast the const away */
	gtk_moz_embed_set_profile_path (profile_dir, (char *) profile_name);
#endif
}

