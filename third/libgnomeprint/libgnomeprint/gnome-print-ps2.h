#ifndef __GNOME_PRINT_PS2_H__
#define __GNOME_PRINT_PS2_H__

/*
 * gnome-print-ps2.h: A Postscript driver for GnomePrint
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 *   Lauris Kaplinski <lauris@helixcode.com>
 *
 * Copyright 2000-2001 Ximian, Inc. and authors
 *
 */

#include <glib.h>

G_BEGIN_DECLS

#define GNOME_TYPE_PRINT_PS2 (gnome_print_ps2_get_type ())
#define GNOME_PRINT_PS2(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_PRINT_PS2, GnomePrintPs2))
#define GNOME_PRINT_PS2_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GNOME_TYPE_PRINT_PS2, GnomePrintPs2Class))
#define GNOME_IS_PRINT_PS2(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_PRINT_PS2))
#define GNOME_IS_PRINT_PS2_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_PRINT_PS2))
#define GNOME_PRINT_PS2_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GNOME_TYPE_PRINT_PS2, GnomePrintPs2Class))

typedef struct _GnomePrintPs2 GnomePrintPs2;
typedef struct _GnomePrintPs2Class GnomePrintPs2Class;

#include "gnome-print.h"

GType gnome_print_ps2_get_type (void);

GnomePrintContext *gnome_print_ps2_new (GnomePrintConfig *config);

G_END_DECLS

#endif /* __GNOME_PRINT_PS2_H__ */






