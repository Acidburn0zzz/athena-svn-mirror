/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * e-printable.h
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Chris Lahey <clahey@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _E_PRINTABLE_H_
#define _E_PRINTABLE_H_

#include <gtk/gtkobject.h>
#include <libgnomeprint/gnome-print.h>

BEGIN_GNOME_DECLS

#define E_PRINTABLE_TYPE        (e_printable_get_type ())
#define E_PRINTABLE(o)          (GTK_CHECK_CAST ((o), E_PRINTABLE_TYPE, EPrintable))
#define E_PRINTABLE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), E_PRINTABLE_TYPE, EPrintableClass))
#define E_IS_PRINTABLE(o)       (GTK_CHECK_TYPE ((o), E_PRINTABLE_TYPE))
#define E_IS_PRINTABLE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), E_PRINTABLE_TYPE))

typedef struct {
	GtkObject   base;
} EPrintable;

typedef struct {
	GtkObjectClass parent_class;

	/*
	 * Signals
	 */

	void        (*print_page)  (EPrintable *etm, GnomePrintContext *context, gdouble width, gdouble height, gboolean quantized);
	gboolean    (*data_left)   (EPrintable *etm);
	void        (*reset)       (EPrintable *etm);
	gdouble     (*height)      (EPrintable *etm, GnomePrintContext *context, gdouble width, gdouble max_height, gboolean quantized);

	/* e_printable_will_fit (ep, ...) should be equal in value to
	 * (e_printable_print_page (ep, ...),
	 * !e_printable_data_left(ep)) except that the latter has the
	 * side effect of doing the printing and advancing the
	 * position of the printable.
	 */

	gboolean    (*will_fit)    (EPrintable *etm, GnomePrintContext *context, gdouble width, gdouble max_height, gboolean quantized);
} EPrintableClass;

GtkType     e_printable_get_type (void);

EPrintable *e_printable_new                 (void);
	
/*
 * Routines for emitting signals on the e_table */
void        e_printable_print_page          (EPrintable        *e_printable,
					     GnomePrintContext *context,
					     gdouble            width,
					     gdouble            height,
					     gboolean           quantized);
gboolean    e_printable_data_left           (EPrintable        *e_printable);
void        e_printable_reset               (EPrintable        *e_printable);
gdouble     e_printable_height              (EPrintable        *e_printable,
					     GnomePrintContext *context,
					     gdouble            width,
					     gdouble            max_height,
					     gboolean           quantized);
gboolean    e_printable_will_fit            (EPrintable        *e_printable,
					     GnomePrintContext *context,
					     gdouble            width,
					     gdouble            max_height,
					     gboolean           quantized);

END_GNOME_DECLS

#endif /* _E_PRINTABLE_H_ */
