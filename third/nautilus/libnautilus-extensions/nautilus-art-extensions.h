/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-art-extensions.h - interface of libart extension functions.

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

   Authors: Darin Adler <darin@eazel.com>
            Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef NAUTILUS_ART_EXTENSIONS_H
#define NAUTILUS_ART_EXTENSIONS_H

#include <libgnome/gnome-defs.h>
#include <libart_lgpl/art_rect.h>
#include <glib.h>

BEGIN_GNOME_DECLS

typedef struct {
	int x;
	int y;
} NautilusArtIPoint;

extern ArtIRect NAUTILUS_ART_IRECT_EMPTY;
extern NautilusArtIPoint NAUTILUS_ART_IPOINT_ZERO;

/* More functions for ArtIRect and ArtDRect. */
gboolean nautilus_art_irect_equal          (const ArtIRect    *rect_a,
					    const ArtIRect    *rect_b);
gboolean nautilus_art_drect_equal          (const ArtDRect    *rect_a,
					    const ArtDRect    *rect_b);
gboolean nautilus_art_irect_hits_irect     (const ArtIRect    *rect_a,
					    const ArtIRect    *rect_b);
gboolean nautilus_art_irect_contains_irect (const ArtIRect    *outer_rect,
					    const ArtIRect    *inner_rect);
gboolean nautilus_art_irect_contains_point (const ArtIRect    *outer_rect,
					    int                x,
					    int                y);
void     nautilus_art_irect_assign         (ArtIRect          *rect,
					    int                x,
					    int                y,
					    int                width,
					    int                height);
void     nautilus_art_ipoint_assign        (NautilusArtIPoint *point,
					    int                x,
					    int                y);
int      nautilus_art_irect_get_width      (const ArtIRect    *rect);
int      nautilus_art_irect_get_height     (const ArtIRect    *rect);
ArtIRect nautilus_art_irect_align          (const ArtIRect    *container,
					    int                aligned_width,
					    int                aligned_height,
					    float              x_alignment,
					    float              y_alignment,
					    int                x_padding,
					    int                y_padding);
END_GNOME_DECLS

#endif /* NAUTILUS_ART_EXTENSIONS_H */
