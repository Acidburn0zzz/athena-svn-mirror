/* renderdemo.c: Common code for rendering demos
 *
 * Copyright (C) 1999, 2004 Red Hat Software
 * Copyright (C) 2001 Sun Microsystems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define DEFAULT_FONT_FAMILY "Sans"
#define DEFAULT_FONT_SIZE 18

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argcontext.h"

#include <pango/pango.h>

#include "renderdemo.h"

#define _MAKE_FONT_NAME(family, size) family " " #size
#define MAKE_FONT_NAME(family, size) _MAKE_FONT_NAME(family, size)

char *prog_name;

gboolean opt_display = FALSE;
int opt_dpi = 96;
char *opt_font = MAKE_FONT_NAME (DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE);
gboolean opt_header = FALSE;
char *opt_output = NULL;
int opt_margin = 10;
int opt_markup = FALSE;
gboolean opt_rtl = FALSE;
int opt_rotate = 0;
gboolean opt_auto_dir = TRUE;
char *opt_text = NULL;
gboolean opt_waterfall = FALSE;
int opt_width = -1;
int opt_indent = 0;
PangoEllipsizeMode opt_ellipsize = PANGO_ELLIPSIZE_NONE;
HintMode opt_hinting = HINT_DEFAULT;

/* Text (or markup) to render */
char *text;

void
fail (const char *format, ...)
{
  const char *msg;
  
  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  g_printerr ("%s: %s\n", prog_name, msg);
  
  exit (1);
}

PangoFontDescription *
get_font_description (void)
{
  PangoFontDescription *font_description = pango_font_description_from_string (opt_font);
  
  if ((pango_font_description_get_set_fields (font_description) & PANGO_FONT_MASK_FAMILY) == 0)
    pango_font_description_set_family (font_description, DEFAULT_FONT_FAMILY);

  if ((pango_font_description_get_set_fields (font_description) & PANGO_FONT_MASK_SIZE) == 0)
    pango_font_description_set_size (font_description, DEFAULT_FONT_SIZE * PANGO_SCALE);

  return font_description;
}

static PangoLayout *
make_layout(PangoContext *context,
	    const char   *text,
	    double        size)
{
  static PangoFontDescription *font_description;
  PangoDirection base_dir;
  PangoLayout *layout;

  layout = pango_layout_new (context);
  if (opt_markup)
    pango_layout_set_markup (layout, text, -1);
  else
    pango_layout_set_text (layout, text, -1);

  pango_layout_set_auto_dir (layout, opt_auto_dir);
  pango_layout_set_ellipsize (layout, opt_ellipsize);

  font_description = get_font_description ();
  if (size > 0)
    pango_font_description_set_size (font_description, size * PANGO_SCALE);
    
  if (opt_width > 0)
    pango_layout_set_width (layout, (opt_width * opt_dpi * PANGO_SCALE + 32) / 72);

  if (opt_indent != 0)
    pango_layout_set_indent (layout, (opt_indent * opt_dpi * PANGO_SCALE + 32) / 72);

  base_dir = pango_context_get_base_dir (context);
  pango_layout_set_alignment (layout,
			      base_dir == PANGO_DIRECTION_LTR ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);
  
  pango_layout_set_font_description (layout, font_description);

  pango_font_description_free (font_description);

  return layout;
}

gchar *
get_options_string (void)
{
  PangoFontDescription *font_description = get_font_description ();
  gchar *font_name;
  gchar *result;

  if (opt_waterfall)
    pango_font_description_unset_fields (font_description, PANGO_FONT_MASK_SIZE);

  font_name = pango_font_description_to_string (font_description);
  result = g_strdup_printf ("%s: dpi=%d", font_name, opt_dpi);
  pango_font_description_free (font_description);
  g_free (font_name);

  return result;
}

static void
transform_point (PangoMatrix *matrix,
		 double       x_in,
		 double       y_in,
		 double      *x_out,
		 double      *y_out)
{
  *x_out = x_in * matrix->xx + y_in * matrix->xy + matrix->x0;
  *y_out = x_in * matrix->yx + y_in * matrix->yy + matrix->y0;
}

static void
output_body (PangoContext  *context,
	     const char    *text,
	     RenderCallback render_cb,
	     gpointer       render_data,
	     PangoMatrix   *matrix,
	     int           *width,
	     int           *height)
{
  PangoLayout *layout;
  PangoRectangle logical_rect;
  int size, start_size, end_size, increment;
  int dy;
  
  if (opt_waterfall)
    {
      start_size = 8;
      end_size = 48;
      increment = 4;
    }
  else
    {
      start_size = end_size = -1;
      increment = 1;
    }

  *width = 0;
  *height = 0;
  dy = 0;
  
  for (size = start_size; size <= end_size; size += increment)
    {
      pango_context_set_matrix (context, matrix);
      
      layout = make_layout (context, text, size);
      pango_layout_get_extents (layout, NULL, &logical_rect);
      
      *width = MAX (*width, PANGO_PIXELS (logical_rect.width));
      *height += PANGO_PIXELS (logical_rect.height);

      if (render_cb)
	(*render_cb) (layout, 0, dy, render_data);
      
      dy += PANGO_PIXELS (logical_rect.height);

      g_object_unref (layout);
    }
}

void
do_output (PangoContext  *context,
	   RenderCallback render_cb,
	   gpointer       render_data,
	   int           *width_out,
	   int           *height_out)
{
  PangoLayout *layout;
  PangoRectangle logical_rect;
  PangoMatrix matrix = PANGO_MATRIX_INIT;
  int rotated_width, rotated_height;
  int x = opt_margin;
  int y = opt_margin;
  double p1x, p1y;
  double p2x, p2y;
  double p3x, p3y;
  double p4x, p4y;
  double minx, miny;
  double maxx, maxy;
  int width, height;
  
  width = 0;
  height = 0;

  pango_context_set_language (context, pango_language_from_string ("en_US"));
  pango_context_set_base_dir (context,
			      opt_rtl ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);
  
  if (opt_header)
    {
      char *options_string = get_options_string ();
      layout = make_layout (context, options_string, 10);
      pango_layout_get_extents (layout, NULL, &logical_rect);
      
      width = MAX (width, PANGO_PIXELS (logical_rect.width));
      height += PANGO_PIXELS (logical_rect.height);
      
      if (render_cb)
	(*render_cb) (layout, x, y, render_data);
      
      y += PANGO_PIXELS (logical_rect.height);
      
      g_object_unref (layout);
      g_free (options_string);
    }

  output_body (context, text, NULL, NULL, NULL, &rotated_width, &rotated_height);

  pango_matrix_rotate (&matrix, opt_rotate);
  
  transform_point (&matrix, 0,             0,              &p1x, &p1y);
  transform_point (&matrix, rotated_width, 0,              &p2x, &p2y);
  transform_point (&matrix, rotated_width, rotated_height, &p3x, &p3y);
  transform_point (&matrix, 0,             rotated_height, &p4x, &p4y);

  minx = MIN (MIN (p1x, p2x), MIN (p3x, p4x));
  miny = MIN (MIN (p1y, p2y), MIN (p3y, p4y));

  maxx = MAX (MAX (p1x, p2x), MAX (p3x, p4x));
  maxy = MAX (MAX (p1y, p2y), MAX (p3y, p4y));

  matrix.x0 = x - minx;
  matrix.y0 = y - miny;

  if (render_cb)
    output_body (context, text, render_cb, render_data, &matrix, &rotated_width, &rotated_height);

  width = MAX (width, maxx - minx);
  height += maxy - miny;

  width += 2 * opt_margin;
  height += 2 * opt_margin;

  if (width_out)
    *width_out = width;
  if (height_out)
    *height_out = height;
}

static void
show_help (ArgContext *context,
	   const char *name,
	   const char *arg,
	   gpointer    data)
{
  g_print ("%s - An example viewer for Pango\n"
	   "\n"
	   "Syntax:\n"
	   "    %s [options] FILE\n"
	   "\n"
	   "Options:\n",
	   prog_name, prog_name);
  arg_context_print_help (context);
  exit (0);
}

/* This function gets called to convert a matched pattern into what
 * we'll use to actually load the font. We turn off hinting since we
 * want metrics that are independent of scale.
 */
void
fc_substitute_func (FcPattern *pattern, gpointer   data)
{
  if (opt_hinting != HINT_DEFAULT)
    {
      FcPatternDel (pattern, FC_HINTING);
      FcPatternAddBool (pattern, FC_HINTING, opt_hinting != HINT_NONE);
      
      FcPatternDel (pattern, FC_AUTOHINT);
      FcPatternAddBool (pattern, FC_AUTOHINT, opt_hinting == HINT_AUTO);
    }
}

void
parse_ellipsis (ArgContext *arg_context,
		const char *name,
		const char *arg,
		gpointer    data)
{
  static GEnumClass *class = NULL;
  GEnumValue *value;

  if (!class)
    class = g_type_class_ref (PANGO_TYPE_ELLIPSIZE_MODE);
  
  value = g_enum_get_value_by_nick (class, arg);
  if (!value)
    fail ("--ellipsize option must be one of none/start/middle/end");

  opt_ellipsize = value->value;
}

void
parse_hinting (ArgContext *arg_context,
	       const char *name,
	       const char *arg,
	       gpointer    data)
{
  static GEnumClass *class = NULL;

  if (!class)
    class = g_type_class_ref (PANGO_TYPE_ELLIPSIZE_MODE);

  if (strcmp (arg, "none") == 0)
    opt_hinting = HINT_NONE;
  else if (strcmp (arg, "auto") == 0)
    opt_hinting = HINT_AUTO;
  else if (strcmp (arg, "full") == 0)
    opt_hinting = HINT_FULL;
  else
    fail ("--hinting option must be one of none/auto/full");
}

void
parse_options (int argc, char *argv[])
{
  static const ArgDesc args[] = {
    { "no-auto-dir","Don't set layout direction according to contents",
      ARG_NOBOOL,   &opt_auto_dir },
    { "display",    "Show output using ImageMagick",
      ARG_BOOL,     &opt_display },
    { "dpi",        "Set the dpi'",
      ARG_INT,      &opt_dpi },
    { "ellipsize",  "Ellipsization mode [=none/start/middle/end]",
      ARG_CALLBACK, NULL, parse_ellipsis },
    { "font",       "Set the font name",
      ARG_STRING,   &opt_font },
    { "header",     "Display the options in the output",
      ARG_BOOL,     &opt_header },
    { "help",       "Show this output",
      ARG_CALLBACK, NULL, show_help, },
    { "hinting",    "Hinting style [=none/auto/full]",
      ARG_CALLBACK, NULL, parse_hinting, },
    { "margin",     "Set the margin on the output in pixels",
      ARG_INT,      &opt_margin },
    { "markup",     "Interpret contents as Pango markup",
      ARG_BOOL,     &opt_markup },
    { "output",     "Name of output file",
      ARG_STRING,   &opt_output },
    { "rtl",        "Set base dir to RTL",
      ARG_BOOL,     &opt_rtl },
    { "rotate",     "Angle at which to rotate results",
      ARG_INT,      &opt_rotate },
    { "text",       "Text to display (instead of a file)",
      ARG_STRING,   &opt_text },
    { "waterfall",  "Create a waterfall display",
      ARG_BOOL,     &opt_waterfall },
    { "width",      "Width in points to which to wrap output",
      ARG_INT,      &opt_width },
    { "indent",     "Width in points to indent paragraphs",
      ARG_INT,      &opt_indent },
    { NULL }
  };

  ArgContext *arg_context;
  GError *error = NULL;
  size_t len;
  char *p;

  prog_name = g_path_get_basename (argv[0]);
  
  arg_context = arg_context_new (NULL);
  arg_context_add_table (arg_context, args);

  if (!arg_context_parse (arg_context, &argc, &argv, &error))
    fail ("%s", error->message);
  
  if ((opt_text && argc != 1) ||
      (!opt_text && argc != 2))
    {
      if (opt_text && argc == 2)
	fail ("When specifying --text, no file should be given");
	
      g_printerr ("Usage: %s [options] FILE\n", prog_name);
      exit (1);
    }
  
  if (!opt_display && !opt_output)
    {
      g_printerr ("%s: --output not specified, assuming --display\n", prog_name);
      opt_display = TRUE;
    }

  /* Get the text
   */
  if (opt_text)
    {
      text = g_strdup (opt_text);
      len = strlen (text);
    }
  else
    {
      if (!g_file_get_contents (argv[1], &text, &len, &error))
	fail ("%s\n", error->message);
      if (!g_utf8_validate (text, len, NULL))
	fail ("Text is not valid UTF-8");
    }
  
  /* Strip trailing whitespace
   */
  p = text + len;
  while (p > text)
    {
      gunichar ch;
      p = g_utf8_prev_char (p);
      ch = g_utf8_get_char (p);
      if (!g_unichar_isspace (ch))
	break;
      else
	*p = '\0';
    }

  /* Make sure we have valid markup
   */
  if (opt_markup &&
      !pango_parse_markup (text, -1, 0, NULL, NULL, NULL, &error))
    fail ("Cannot parse input as markup: %s", error->message);
}
