/* libglade-support.c -- support functions for libglade
   $Id: libglade-support.c,v 1.1.1.2 2003-01-05 00:30:04 ghudson Exp $

   Copyright (C) 1999 John Harper <john@dcs.warwick.ac.uk>

   This file is part of rep-gtk.

   rep-gtk is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   rep-gtk is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with rep-gtk; see the file COPYING.   If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <config.h>
#include <assert.h>
#include <glade/glade.h>
#include "rep-gtk.h"
#include "rep-libglade.h"
#include <string.h>

typedef struct {
    repv func;
} conn_data;

static void
connector (const gchar *handler_name, GtkObject *object,
	   const gchar *signal_name, const gchar *signal_data,
	   GtkObject *connect_object, gboolean after, gpointer user_data)
{
    repv func = ((conn_data *) user_data)->func;

    if (func == Qnil)
    {
	/* Look for a signal handler called HANDLER-NAME. */
	repv sym = Fintern (rep_string_dup (handler_name), Qnil);
	if (sym && rep_SYMBOLP(sym))
	    func = Fsymbol_value (sym, Qt);
    }

    if (Ffunctionp (func) != Qnil)
    {
	/* Looks like we've got something callable */
	sgtk_protshell *data = sgtk_protect (sgtk_wrap_gtkobj (object), func);
	gtk_signal_connect_full (object, signal_name, 0,
				 sgtk_callback_marshal,
				 (gpointer) data, sgtk_callback_destroy,
				 connect_object != 0, after);
    }
}

void
sgtk_glade_xml_signal_connect (GladeXML *self, char *handler_name, repv func)
{
    conn_data data;
    data.func = func;
    glade_xml_signal_connect_full (self, handler_name,
				   connector, (gpointer) &data);
}

void
sgtk_glade_xml_signal_autoconnect (GladeXML *self)
{
    conn_data data;
    data.func = Qnil;
    glade_xml_signal_autoconnect_full (self, connector, (gpointer) &data);
}

GladeXML *
sgtk_glade_xml_new_from_string (repv text, const char *root,
				const char *domain)
{
    if (!rep_STRINGP(text))
    {
	rep_signal_arg_error (text, 1);
	return 0;
    }

    return glade_xml_new_from_memory (rep_STR(text),
				      rep_STRING_LEN(text),
				      root, domain);
}


/* dl hooks / init */

repv
rep_dl_init (void)
{
    repv s;
    char *tem = getenv ("REP_GTK_DONT_INITIALIZE");
    if (tem == 0 || atoi (tem) == 0)
	glade_init ();

    s = rep_push_structure ("gui.gtk-2.libglade");

    sgtk_init_gtk_libglade_glue ();
    return rep_pop_structure (s);
}
