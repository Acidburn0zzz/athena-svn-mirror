/* Metacity size/position constraints */

/* 
 * Copyright (C) 2002 Red Hat, Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef META_CONSTRAINTS_H
#define META_CONSTRAINTS_H

#include "util.h"
#include "window.h"
#include "frame.h"

typedef enum
{
  META_RESIZE_LEFT_OR_TOP,
  META_RESIZE_CENTER,
  META_RESIZE_RIGHT_OR_BOTTOM
} MetaResizeDirection;

void meta_window_constrain (MetaWindow          *window,
                            MetaFrameGeometry   *fgeom,
                            const MetaRectangle *orig,
                            int                  x_move_delta,
                            int                  y_move_delta,
                            MetaResizeDirection  x_direction,
                            int                  x_delta,
                            MetaResizeDirection  y_direction,
                            int                  y_delta,
                            MetaRectangle       *new);

MetaResizeDirection meta_x_direction_from_gravity (int gravity);
MetaResizeDirection meta_y_direction_from_gravity (int gravity);

#endif /* META_CONSTRAINTS_H */









