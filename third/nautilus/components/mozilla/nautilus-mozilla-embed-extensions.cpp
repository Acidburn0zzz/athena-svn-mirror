/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

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
 *  Author: Ramiro Estrugo <ramiro@eazel.com>
 *
 */

/*
 * nautilus-mozilla-embed-extensions.cpp - Extensions to GtkMozEmbed.
 */

#include <config.h>

#include "nautilus-mozilla-embed-extensions.h"
#include "nautilus-mozilla-encoding-tables.h"
#include "gtkmozembed_internal.h"

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include "nsIServiceManager.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include <vector>
#include <string>

struct Encoding
{
	Encoding (const char *encoding,
	      const char *encoding_title,
	      const char *translated_encoding_title) :
		m_encoding (encoding),
		m_encoding_title (encoding_title),
		m_translated_encoding_title (translated_encoding_title)
	{
	}

	string m_encoding;
	string m_encoding_title;
	string m_translated_encoding_title;
};

static nsIDocShell *mozilla_embed_get_primary_docshell                     (const GtkMozEmbed *mozilla_embed);
static guint        translated_encoding_get_count                          (void);
static const char * translated_encoding_peek_nth_encoding_title            (guint              n);
static const char * translated_encoding_peek_nth_translated_encoding_title (guint              n);
static const char * translated_encoding_peek_nth_translated_encoding_title (guint              n);
static const char * translated_encoding_find_translated_title              (const char        *title);
static guint        encoding_group_get_count                               (void);
static char *       convert_ns_string_to_c_string                          (const              nsString &ns_string);

static vector<Encoding>
encoding_get_encoding_table (void)
{
	static vector<Encoding> empty_encodings;
	static vector<Encoding> encodings;

	if (encodings.size () > 0) {
		return encodings;
	}
	nsresult rv;

	nsCOMPtr<nsICharsetConverterManager2> charsetManager = 
		do_GetService (NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
	g_return_val_if_fail (NS_SUCCEEDED (rv), empty_encodings);

	nsCOMPtr <nsISupportsArray> decoderArray;

	rv = charsetManager->GetDecoderList (getter_AddRefs (decoderArray));
	g_return_val_if_fail (NS_SUCCEEDED (rv), empty_encodings);

	PRUint32 numDecoders;
	rv = decoderArray->Count (&numDecoders);
	g_return_val_if_fail (NS_SUCCEEDED (rv), empty_encodings);

	for (PRUint32 i = 0; i < numDecoders; i++) {
		nsCOMPtr<nsISupports> decoder = (dont_AddRef) (decoderArray->ElementAt (i));
		nsCOMPtr<nsIAtom> decoderAtom (do_QueryInterface (decoder));

		nsString decoderName;
		rv = decoderAtom->ToString (decoderName);
		g_return_val_if_fail (NS_SUCCEEDED (rv), empty_encodings);

		char *charset = convert_ns_string_to_c_string (decoderName);
		g_assert (charset != NULL);


		nsString decoderTitle;

 		rv = charsetManager->GetCharsetTitle2 (decoderAtom, &decoderTitle);
 		char *charset_title = NULL;

 		if (NS_SUCCEEDED (rv)) {
 			charset_title = convert_ns_string_to_c_string (decoderTitle);
 		}

 		if (charset_title == NULL || strlen (charset_title) <= 0) {
 			g_free (charset_title);
 			charset_title = g_strdup (charset);
		}

 		const char *translated_charset_title = mozilla_encoding_table_find_translated (charset_title);

		if (translated_charset_title == NULL || strlen (translated_charset_title) <= 0) {
			translated_charset_title = charset_title;
		}

		encodings.push_back (Encoding (charset, charset_title, translated_charset_title));

 		g_free (charset);
 		g_free (charset_title);
	}

	return encodings;
}

extern "C" guint
mozilla_charset_get_num_encodings (const GtkMozEmbed *mozilla_embed)
{
	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), 0);

	vector<Encoding> encodings = encoding_get_encoding_table ();

	return encodings.size ();
}

extern "C" char *
mozilla_charset_get_nth_encoding (const GtkMozEmbed *mozilla_embed,
				  guint n)
{
 	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), NULL);
 	g_return_val_if_fail (n < mozilla_charset_get_num_encodings (mozilla_embed), NULL);

	vector<Encoding> encodings = encoding_get_encoding_table ();

	return g_strdup (encodings[n].m_encoding.c_str ());
}

extern "C" char *
mozilla_charset_get_nth_encoding_title (const GtkMozEmbed *mozilla_embed,
					guint n)
{
 	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), NULL);
 	g_return_val_if_fail (n < mozilla_charset_get_num_encodings (mozilla_embed), NULL);

	vector<Encoding> encodings = encoding_get_encoding_table ();

	return g_strdup (encodings[n].m_encoding_title.c_str ());
}

extern "C" char *
mozilla_charset_get_nth_translated_encoding_title (const GtkMozEmbed *mozilla_embed,
						   guint n)
{
 	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), NULL);
 	g_return_val_if_fail (n < mozilla_charset_get_num_encodings (mozilla_embed), NULL);

	vector<Encoding> encodings = encoding_get_encoding_table ();

	return g_strdup (encodings[n].m_translated_encoding_title.c_str ());
}

extern "C" gboolean
mozilla_charset_set_encoding (GtkMozEmbed *mozilla_embed,
			      const char *charset_encoding)
{
	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), FALSE);
	g_return_val_if_fail (charset_encoding != NULL, FALSE);

	nsCOMPtr<nsIDocShell> docShell;
	nsresult rv;

	docShell = mozilla_embed_get_primary_docshell (mozilla_embed);
	
	if (!docShell) {
		return FALSE;
	}

	nsCOMPtr<nsIContentViewer> contentViewer;
	rv = docShell->GetContentViewer (getter_AddRefs (contentViewer));
	if (!NS_SUCCEEDED (rv) || !contentViewer) {
		return FALSE;
	}

	nsCOMPtr<nsIMarkupDocumentViewer> markupDocumentViewer = do_QueryInterface (contentViewer, &rv);
	if (!NS_SUCCEEDED (rv) || !markupDocumentViewer) {
		return FALSE;
	}
	
	nsAutoString charsetString;
	charsetString.AssignWithConversion (charset_encoding);
	rv = markupDocumentViewer->SetForceCharacterSet (charsetString.ToNewUnicode());

	return NS_SUCCEEDED (rv) ? TRUE : FALSE;
}

extern "C" char *
mozilla_charset_find_encoding_group (const GtkMozEmbed *mozilla_embed,
				     const char *encoding)
{
	guint i;

	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), NULL);
	g_return_val_if_fail (encoding != NULL, NULL);

	for (i = 0; i < mozilla_encoding_groups_table_get_count (); i++) {
		const char *group = mozilla_encoding_groups_table_peek_nth (i);
		char *find;
		
		find = strstr (encoding, group);
		
		if (find != NULL) {
			return g_strdup (group);
		}
	}

	return NULL;
}

extern "C" char *
mozilla_charset_encoding_group_get_translated (const GtkMozEmbed *mozilla_embed,
					       const char *encoding_group)
{
	guint i;

	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), NULL);
	g_return_val_if_fail (encoding_group != NULL, NULL);

	for (i = 0; i < mozilla_encoding_groups_table_get_count (); i++) {
		const char *group = mozilla_encoding_groups_table_peek_nth (i);

		if (g_strcasecmp (encoding_group, group) == 0) {
			return g_strdup (mozilla_encoding_groups_table_peek_nth_translated (i));
		}
	}

	return NULL;
}

int
mozilla_charset_get_encoding_group_index (const GtkMozEmbed *mozilla_embed,
					  const char *encoding_group)
{
	guint i;

	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), -1);

	if (encoding_group == NULL) {
		return -1;
	}
	for (i = 0; i < mozilla_encoding_groups_table_get_count (); i++) {
		const char *group = mozilla_encoding_groups_table_peek_nth (i);

		if (g_strcasecmp (group, encoding_group) == 0) {
			return i;
		}
	}
	
	return -1;
}

/* FIXME: This is cut-n-pasted from mozilla-events.cpp */
static nsIDocShell *
mozilla_embed_get_primary_docshell (const GtkMozEmbed *mozilla_embed)
{
	g_return_val_if_fail (GTK_IS_MOZ_EMBED (mozilla_embed), NULL);

	nsIWebBrowser *web_browser;
	gtk_moz_embed_get_nsIWebBrowser (const_cast<GtkMozEmbed *> (mozilla_embed), &web_browser);

	nsCOMPtr<nsIDocShell> doc_shell;
        nsCOMPtr<nsIDocShellTreeItem> browserAsItem = do_QueryInterface (web_browser);
	if (!browserAsItem) return NULL;

	// get the tree owner for that item
	nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
	nsresult rv = browserAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
	if (!NS_SUCCEEDED (rv) || ! treeOwner) return NULL;

	// get the primary content shell as an item
	nsCOMPtr<nsIDocShellTreeItem> contentItem;
	rv = treeOwner->GetPrimaryContentShell(getter_AddRefs(contentItem));
	if (!NS_SUCCEEDED (rv) || ! contentItem) return NULL;

	// QI that back to a docshell
	doc_shell = do_QueryInterface (contentItem);

	return doc_shell;
}

/* This nonsense is needed to get the allocators right */
static char *
convert_ns_string_to_c_string (const nsString & ns_string)
{
	char *c_string;
	char *ns_c_string = ns_string.ToNewCString ();
	
	if (ns_c_string == NULL) {
		return NULL;
	}

	c_string = g_strdup (ns_c_string);

	nsMemory::Free (ns_c_string);

	return c_string;
}
