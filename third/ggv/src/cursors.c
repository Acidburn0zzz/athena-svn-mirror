/* GGV - mouse cursors
 *
 * Copied from EOG.
 *
 * Copyright (C) 2000-2002 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "cursors.h"

/* Cursor definitions.  Keep in sync with the CursorType enumeration in
 * cursors.h.
 */

#include "cursors/hand-open-data.xbm"
#include "cursors/hand-open-mask.xbm"
#include "cursors/hand-closed-data.xbm"
#include "cursors/hand-closed-mask.xbm"

static struct {
	char *data;
	char *mask;
	int data_width;
	int data_height;
	int mask_width;
	int mask_height;
	int hot_x, hot_y;
} cursors[] = {
	{ hand_open_data_bits, hand_open_mask_bits,
	  hand_open_data_width, hand_open_data_height,
	  hand_open_mask_width, hand_open_mask_height,
	  hand_open_data_width / 2, hand_open_data_height / 2 },
	{ hand_closed_data_bits, hand_closed_mask_bits,
	  hand_closed_data_width, hand_closed_data_height,
	  hand_closed_mask_width, hand_closed_mask_height,
	  hand_closed_data_width / 2, hand_closed_data_height / 2 },
	{ NULL, NULL, 0, 0, 0, 0 }
};

/**
 * cursor_get:
 * @window: Window whose screen and colormap determine the cursor's.
 * @type: A cursor type.
 * 
 * Creates a cursor.
 * 
 * Return value: The newly-created cursor.
 **/
GdkCursor *
cursor_get (GdkWindow *window, CursorType type)
{
	GdkBitmap *data;
	GdkBitmap *mask;
	GdkColor black, white;
	GdkCursor *cursor;

	g_return_val_if_fail (window != NULL, NULL);
	g_return_val_if_fail (type >= 0 && type < CURSOR_NUM_CURSORS, NULL);

	g_assert (cursors[type].data_width == cursors[type].mask_width);
	g_assert (cursors[type].data_height == cursors[type].mask_height);

	data = gdk_bitmap_create_from_data (window,
					    cursors[type].data,
					    cursors[type].data_width,
					    cursors[type].data_height);
	mask = gdk_bitmap_create_from_data (window,
					    cursors[type].mask,
					    cursors[type].mask_width,
					    cursors[type].mask_height);

	g_assert (data != NULL && mask != NULL);

	gdk_color_black (gdk_drawable_get_colormap (GDK_DRAWABLE(window)), &black);
	gdk_color_white (gdk_drawable_get_colormap (GDK_DRAWABLE(window)), &white);

	cursor = gdk_cursor_new_from_pixmap (data, mask, &white, &black,
					     cursors[type].hot_x, cursors[type].hot_y);
	g_assert (cursor != NULL);

	gdk_drawable_unref (GDK_DRAWABLE(data));
	gdk_drawable_unref (GDK_DRAWABLE(mask));

	return cursor;
}
