/* Pango
 * pangoxft-font.c: Routines for handling X fonts
 *
 * Copyright (C) 2000 Red Hat Software
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

#include "config.h"

#include <stdlib.h>

#include "pangofc-fontmap.h"
#include "pangoxft-private.h"
#include "pangofc-private.h"

#define PANGO_XFT_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT, PangoXftFont))
#define PANGO_XFT_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_XFT_FONT, PangoXftFontClass))
#define PANGO_XFT_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_XFT_FONT))
#define PANGO_XFT_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_XFT_FONT, PangoXftFontClass))

#define PANGO_XFT_UNKNOWN_FLAG 0x10000000

typedef struct _PangoXftFontClass    PangoXftFontClass;

struct _PangoXftFontClass
{
  PangoFcFontClass  parent_class;
};

static void pango_xft_font_finalize (GObject *object);

static void                  pango_xft_font_get_glyph_extents (PangoFont        *font,
							       PangoGlyph        glyph,
							       PangoRectangle   *ink_rect,
							       PangoRectangle   *logical_rect);

static FT_Face    pango_xft_font_real_lock_face         (PangoFcFont      *font);
static void       pango_xft_font_real_unlock_face       (PangoFcFont      *font);
static gboolean   pango_xft_font_real_has_char          (PangoFcFont      *font,
							 gunichar          wc);
static guint      pango_xft_font_real_get_glyph         (PangoFcFont      *font,
							 gunichar          wc);
static PangoGlyph pango_xft_font_real_get_unknown_glyph (PangoFcFont      *font,
							 gunichar          wc);
static void       pango_xft_font_real_shutdown          (PangoFcFont      *font);

static XftFont *xft_font_get_font (PangoFont *font);

G_DEFINE_TYPE (PangoXftFont, pango_xft_font, PANGO_TYPE_FC_FONT)

static void
pango_xft_font_class_init (PangoXftFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (class);

  object_class->finalize = pango_xft_font_finalize;
  
  font_class->get_glyph_extents = pango_xft_font_get_glyph_extents;

  fc_font_class->lock_face = pango_xft_font_real_lock_face;
  fc_font_class->unlock_face = pango_xft_font_real_unlock_face;
  fc_font_class->has_char = pango_xft_font_real_has_char;
  fc_font_class->get_glyph = pango_xft_font_real_get_glyph;
  fc_font_class->get_unknown_glyph = pango_xft_font_real_get_unknown_glyph;
  fc_font_class->shutdown = pango_xft_font_real_shutdown;
}

static void
pango_xft_font_init (PangoXftFont *xftfont)
{
}

PangoXftFont *
_pango_xft_font_new (PangoXftFontMap *xftfontmap,
		     FcPattern	     *pattern)
{
  PangoFontMap *fontmap = PANGO_FONT_MAP (xftfontmap);
  PangoXftFont *xfont;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  xfont = (PangoXftFont *)g_object_new (PANGO_TYPE_XFT_FONT,
					"pattern", pattern,
					NULL);
  
  xfont->xft_font = NULL;

  return xfont;
}

static PangoFont *
get_mini_font (PangoFont *font)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  PangoFcFont *fcfont = (PangoFcFont *)font;

  g_assert (fcfont->fontmap);

  if (!xfont->mini_font)
    {
      Display *display;
      PangoFontDescription *desc = pango_font_description_new ();
      int i;
      int width = 0, height = 0;
      XGlyphInfo extents;
      XftFont *mini_xft;
      
      _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);

      pango_font_description_set_family_static (desc, "monospace");
      pango_font_description_set_size (desc,
				       0.5 * pango_font_description_get_size (fcfont->description));
      
      xfont->mini_font = pango_font_map_load_font (fcfont->fontmap, NULL, desc);
      pango_font_description_free (desc);
      
      mini_xft = xft_font_get_font (xfont->mini_font);
      
      for (i = 0 ; i < 16 ; i++)
	{
	  char c = i < 10 ? '0' + i : 'A' + i - 10;
	  XftTextExtents8 (display, mini_xft, (FcChar8 *) &c, 1, &extents);
	  width = MAX (width, extents.width);
	  height = MAX (height, extents.height);
	}

      
      xfont->mini_width = width;
      xfont->mini_height = height;
      xfont->mini_pad = MAX (height / 10, 1);
    }

  return xfont->mini_font;
}

static void
draw_rectangle (Display      *display,
		Picture       src_picture,
		Picture       dest_picture,
		XftDraw      *draw,
		XftColor     *color,
		gint          x,
		gint          y,
		gint          width,
		gint          height)
{
  if (draw)
    {
      XftDrawRect (draw, color, x, y, width, height);
    }
  else
    {
      XRenderComposite (display, PictOpOver, src_picture, None, dest_picture,
			0, 0, 0, 0, x, y, width, height);
    }
}

static void
draw_box (Display      *display,
	  Picture       src_picture,
	  Picture       dest_picture,
	  XftDraw      *draw,
	  XftColor     *color,
	  PangoXftFont *xfont,
	  gint          x,
	  gint          y,
	  gint          width,
	  gint          height)
{
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x, y, width, xfont->mini_pad);
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x, y + xfont->mini_pad, xfont->mini_pad, height - xfont->mini_pad * 2);
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x + width - xfont->mini_pad, y + xfont->mini_pad, xfont->mini_pad, height - xfont->mini_pad * 2);
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x, y + height - xfont->mini_pad, width, xfont->mini_pad);
}

/**
 * pango_xft_render:
 * @draw:    the <type>XftDraw</type> object.
 * @color:   the color in which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto an <type>XftDraw</type> object wrapping an X drawable.
 */
static void
pango_xft_real_render (Display          *display,
		       Picture           src_picture,
		       Picture           dest_picture,
		       XftDraw          *draw,
		       XftColor         *color,
		       PangoFont        *font,
		       PangoGlyphString *glyphs,
		       gint              x,
		       gint              y)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (font);
  PangoFcFont *fcfont = PANGO_FC_FONT (font);
  XftFont *xft_font = xft_font_get_font (font);
  int i;
  int x_off = 0;
#define N_XFT_LOCAL	1024
  XftGlyphSpec  xft_glyphs[N_XFT_LOCAL];
  XftCharSpec	chars[6];     /* for unknown */
  int		n_xft_glyph = 0;

  if (!fcfont->fontmap)		/* Display closed */
    return;

#define FLUSH_GLYPHS() G_STMT_START {						\
  if (n_xft_glyph)								\
    {										\
       if (draw)								\
         XftDrawGlyphSpec (draw, color, xft_font, xft_glyphs, n_xft_glyph);	\
       else									\
         XftGlyphSpecRender (display, PictOpOver, src_picture, xft_font,	\
	  		     dest_picture, 0, 0, xft_glyphs, n_xft_glyph);	\
       n_xft_glyph = 0;								\
    }										\
  } G_STMT_END

  if (!display)
    _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);

  for (i=0; i<glyphs->num_glyphs; i++)
    {
      PangoGlyph glyph = glyphs->glyphs[i].glyph;
      int glyph_x = x + PANGO_PIXELS (x_off + glyphs->glyphs[i].geometry.x_offset);
      int glyph_y = y + PANGO_PIXELS (glyphs->glyphs[i].geometry.y_offset);

      /* Clip glyphs into the X coordinate range; we really
       * want to clip glyphs with an ink rect outside the
       * [0,32767] x [0,32767] rectangle but looking up
       * the ink rect here would be a noticeable speed hit.
       * This is close enough.
       */
      if (glyph &&
	  glyph_x >= -16384 && glyph_x <= 32767 &&
	  glyph_y >= -16384 && glyph_y <= 32767)
	{
	  if (glyph & PANGO_XFT_UNKNOWN_FLAG)
	    {
	      char buf[7];
	      int ys[3];
	      int xs[4];
	      int row, col;
              int cols;
	      
	      PangoFont *mini_font = get_mini_font (font);
	      XftFont *mini_xft = xft_font_get_font (mini_font);
      
	      glyph &= ~PANGO_XFT_UNKNOWN_FLAG;
	      
	      ys[0] = glyph_y - xft_font->ascent + (xft_font->ascent + xft_font->descent - xfont->mini_height * 2 - xfont->mini_pad * 5) / 2;
	      ys[1] = ys[0] + 2 * xfont->mini_pad + xfont->mini_height;
	      ys[2] = ys[1] + xfont->mini_height + xfont->mini_pad;

	      xs[0] = glyph_x; 
	      xs[1] = xs[0] + 2 * xfont->mini_pad;
	      xs[2] = xs[1] + xfont->mini_width + xfont->mini_pad;
	      xs[3] = xs[2] + xfont->mini_width + xfont->mini_pad;

              if (glyph > 0xffff)
                {
                  cols = 3;
                  g_snprintf (buf, sizeof(buf), "%06X", glyph);
                }
              else
                {
                  cols = 2;
                  g_snprintf (buf, sizeof(buf), "%04X", glyph);
                }

	      draw_box (display, src_picture, dest_picture, draw, color, xfont,
			xs[0], ys[0],
			xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1),
			xfont->mini_height * 2 + xfont->mini_pad * 5);

	      FLUSH_GLYPHS ();
	      for (row = 0; row < 2; row++)
		for (col = 0; col < cols; col++)
		  {
		    XftCharSpec *c = &chars[row * cols + col];
		    c->ucs4 = buf[row * cols + col] & 0xff;
		    c->x = xs[col+1];
		    c->y = ys[row+1];
		  }
	      if (draw)
		XftDrawCharSpec (draw, color, mini_xft,
				 chars, 2 * cols);
	      else
		XftCharSpecRender (display, PictOpOver, src_picture,
				   mini_xft, dest_picture, 0, 0,
				   chars, 2 * cols);
	    }
	  else if (glyph)
	    {
	      if (n_xft_glyph == N_XFT_LOCAL)
		FLUSH_GLYPHS ();
	      
	      xft_glyphs[n_xft_glyph].x = glyph_x;
	      xft_glyphs[n_xft_glyph].y = glyph_y;
	      xft_glyphs[n_xft_glyph].glyph = glyph;
	      n_xft_glyph++;
	    }
	}
      
      x_off += glyphs->glyphs[i].geometry.width;
    }
  
  FLUSH_GLYPHS ();

#undef FLUSH_GLYPHS
}

/**
 * pango_xft_render:
 * @draw:    the <type>XftDraw</type> object.
 * @color:   the color in which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto an <type>XftDraw</type> object wrapping an X drawable.
 */
void
pango_xft_render (XftDraw          *draw,
		  XftColor         *color,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  gint              x,
		  gint              y)
{
  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);
  
  pango_xft_real_render (NULL, None, None, draw, color, font, glyphs, x, y);
}

/**
 * pango_xft_picture_render:
 * @display:      an X display
 * @src_picture:  the source picture to draw the string with
 * @dest_picture: the destination picture to draw the strign onto
 * @font:         the font in which to draw the string
 * @glyphs:       the glyph string to draw
 * @x:            the x position of start of string (in pixels)
 * @y:            the y position of baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto an Xrender <type>Picture</type> object.
 */
void
pango_xft_picture_render (Display          *display,
			  Picture           src_picture,
			  Picture           dest_picture,
			  PangoFont        *font,
			  PangoGlyphString *glyphs,
			  gint              x,
			  gint              y)
{
  g_return_if_fail (display != NULL);
  g_return_if_fail (src_picture != None);
  g_return_if_fail (dest_picture != None);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);
  
  pango_xft_real_render (display, src_picture, dest_picture, NULL, NULL, font, glyphs, x, y);
}

static void
pango_xft_font_finalize (GObject *object)
{
  PangoXftFont *xfont = (PangoXftFont *)object;
  PangoFcFont *fcfont = (PangoFcFont *)object;

  if (xfont->mini_font)
    g_object_unref (xfont->mini_font);

  if (xfont->xft_font)
    {
      Display *display;
      
      _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);
      XftFontClose (display, xfont->xft_font);
    }

  if (xfont->glyph_info)
    g_hash_table_destroy (xfont->glyph_info);

  G_OBJECT_CLASS (pango_xft_font_parent_class)->finalize (object);
}

static void
get_glyph_extents_missing (PangoXftFont    *xfont,
			   PangoGlyph       glyph,
			   PangoRectangle  *ink_rect,
			   PangoRectangle  *logical_rect)

{
  PangoFont *font = PANGO_FONT (xfont);
  XftFont *xft_font = xft_font_get_font (font);
  
  gint cols = (glyph & ~PANGO_XFT_UNKNOWN_FLAG) > 0xffff ? 3 : 2;
  
  get_mini_font (font);
  
  if (ink_rect)
    {
      ink_rect->x = 0;
      ink_rect->y = PANGO_SCALE * (- xft_font->ascent + (xft_font->ascent + xft_font->descent - xfont->mini_height * 2 - xfont->mini_pad * 5) / 2);
      ink_rect->width = PANGO_SCALE * (xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1));
      ink_rect->height = PANGO_SCALE * (xfont->mini_height * 2 + xfont->mini_pad * 5);
    }
  
  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = - PANGO_SCALE * xft_font->ascent;
      logical_rect->width = PANGO_SCALE * (xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 2));
      logical_rect->height = (xft_font->ascent + xft_font->descent) * PANGO_SCALE;
    }
}

static void
get_glyph_extents_xft (PangoFcFont      *fcfont,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)fcfont);
  XGlyphInfo extents;
  Display *display;
  FT_UInt ft_glyph = glyph;
  
  _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);

  XftGlyphExtents (display, xft_font, &ft_glyph, 1, &extents);

  if (ink_rect)
    {
      ink_rect->x = - extents.x * PANGO_SCALE; /* Xft crack-rock sign choice */
      ink_rect->y = - extents.y * PANGO_SCALE; /*             "              */
      ink_rect->width = extents.width * PANGO_SCALE;
      ink_rect->height = extents.height * PANGO_SCALE;
    }
  
  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = - xft_font->ascent * PANGO_SCALE;
      logical_rect->width = extents.xOff * PANGO_SCALE;
      logical_rect->height = (xft_font->ascent + xft_font->descent) * PANGO_SCALE;
    }
}

typedef struct
{
  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
} Extents;

static void
get_glyph_extents_raw (PangoXftFont     *xfont,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  Extents *extents;

  if (!xfont->glyph_info)
    xfont->glyph_info = g_hash_table_new_full (NULL, NULL,
					       NULL, (GDestroyNotify)g_free);

  extents = g_hash_table_lookup (xfont->glyph_info,
				 GUINT_TO_POINTER (glyph));

  if (!extents)
    {
      extents = g_new (Extents, 1);
     
      pango_fc_font_get_raw_extents (PANGO_FC_FONT (xfont),
				     FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING,
				     glyph,
				     &extents->ink_rect,
				     &extents->logical_rect);

      g_hash_table_insert (xfont->glyph_info,
			   GUINT_TO_POINTER (glyph),
			   extents);
    }
  
  if (ink_rect)
    *ink_rect = extents->ink_rect;

  if (logical_rect)
    *logical_rect = extents->logical_rect;
}

static void
pango_xft_font_get_glyph_extents (PangoFont        *font,
				  PangoGlyph        glyph,
				  PangoRectangle   *ink_rect,
				  PangoRectangle   *logical_rect)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  PangoFcFont *fcfont = PANGO_FC_FONT (font);

  if (!fcfont->fontmap)		/* Display closed */
    goto fallback;

  if (glyph == (PangoGlyph)-1)
    glyph = 0;

  if (glyph & PANGO_XFT_UNKNOWN_FLAG)
    {
      get_glyph_extents_missing (xfont, glyph, ink_rect, logical_rect);
    }
  else if (glyph)
    {
      if (!fcfont->is_transformed)
	get_glyph_extents_xft (fcfont, glyph, ink_rect, logical_rect);
      else
	get_glyph_extents_raw (xfont, glyph, ink_rect, logical_rect);
    }
  else
    {
    fallback:
      
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->width = 0;
	  ink_rect->y = 0;
	  ink_rect->height = 0;
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = 0;
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
    }
}

static void
load_fallback_font (PangoXftFont *xfont)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (xfont);
  Display *display;
  int screen;
  XftFont *xft_font;
  
  _pango_xft_font_map_get_info (fcfont->fontmap, &display, &screen);
      
  xft_font = XftFontOpen (display,  screen,
                          FC_FAMILY, FcTypeString, "sans",
                          FC_SIZE, FcTypeDouble, (double)pango_font_description_get_size (fcfont->description)/PANGO_SCALE,
                          NULL);
  
  if (!xft_font)
    {
      g_warning ("Cannot open fallback font, nothing to do");
      exit (1);
    }

  xfont->xft_font = xft_font;
}

static XftFont *
xft_font_get_font (PangoFont *font)
{
  PangoXftFont *xfont;
  PangoFcFont *fcfont;
  Display *display;
  int screen;

  xfont = (PangoXftFont *)font;
  fcfont = (PangoFcFont *)font;

  if (xfont->xft_font == NULL)
    {
      _pango_xft_font_map_get_info (fcfont->fontmap, &display, &screen);

      xfont->xft_font = XftFontOpenPattern (display, FcPatternDuplicate (fcfont->font_pattern));
      if (!xfont->xft_font)
	{
	  gchar *name = pango_font_description_to_string (fcfont->description);
	  g_warning ("Cannot open font file for font %s", name);
  	  g_free (name);
  	  
 	  load_fallback_font (xfont);
	}
    }
  
  return xfont->xft_font;
}

static FT_Face
pango_xft_font_real_lock_face (PangoFcFont *font)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);
  
  return XftLockFace (xft_font);
}

static void
pango_xft_font_real_unlock_face (PangoFcFont *font)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);
  
  XftUnlockFace (xft_font);
}

static gboolean
pango_xft_font_real_has_char (PangoFcFont *font,
			      gunichar     wc)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);
  
  return XftCharExists (NULL, xft_font, wc);
}

static guint
pango_xft_font_real_get_glyph (PangoFcFont *font,
			       gunichar     wc)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);

  return XftCharIndex (NULL, xft_font, wc);
}

static PangoGlyph
pango_xft_font_real_get_unknown_glyph (PangoFcFont *font,
				       gunichar     wc)
{
  return wc | PANGO_XFT_UNKNOWN_FLAG;
}

static void
pango_xft_font_real_shutdown (PangoFcFont *fcfont)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (fcfont);
  
  if (xfont->xft_font)
    {
      Display *display;

      _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);
      XftFontClose (display, xfont->xft_font);
      xfont->xft_font = NULL;
    }
}

/**
 * pango_xft_font_get_font:
 * @font: a #PangoFont.
 *
 * Returns the XftFont of a font.
 *
 * Returns: the XftFont associated to @font.
 **/
XftFont *
pango_xft_font_get_font (PangoFont *font)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);
  
  return xft_font_get_font (font);
}

/**
 * pango_xft_font_get_display:
 * @font: a #PangoFont.
 *
 * Returns the X display of the XftFont of a font.
 *
 * Returns: the X display of the XftFont associated to @font.
 **/
Display *
pango_xft_font_get_display (PangoFont *font)
{
  PangoFcFont *fcfont;
  Display *display;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  fcfont = PANGO_FC_FONT (font);
  _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);

  return display;
}

/**
 * pango_xft_font_get_unknown_glyph:
 * @font: a #PangoFont.
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use pango_fc_font_get_unknown_glyph() instead.
 *
 * Return value: a glyph index into @font.
 **/
PangoGlyph
pango_xft_font_get_unknown_glyph (PangoFont *font,
				  gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), -1);

  return pango_fc_font_get_unknown_glyph (PANGO_FC_FONT (font), wc);
}

/**
 * pango_xft_font_lock_face:
 * @font: a #PangoFont.
 *
 * Gets the FreeType FT_Face associated with a font,
 * This face will be kept around until you call
 * pango_xft_font_unlock_face().
 *
 * Use pango_fc_font_lock_face() instead.
 *
 * Returns: the FreeType FT_Face associated with @font.
 *
 * Since: 1.2
 **/
FT_Face
pango_xft_font_lock_face (PangoFont *font)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  return pango_fc_font_lock_face (PANGO_FC_FONT (font));
}

/**
 * pango_xft_font_unlock_face:
 * @font: a #PangoFont.
 *
 * Releases a font previously obtained with
 * pango_xft_font_lock_face().
 *
 * Use pango_fc_font_unlock_face() instead.
 *
 * Since: 1.2
 **/
void
pango_xft_font_unlock_face (PangoFont *font)
{
  g_return_if_fail (PANGO_XFT_IS_FONT (font));

  pango_fc_font_unlock_face (PANGO_FC_FONT (font));
}

/**
 * pango_xft_font_get_glyph:
 * @font: a #PangoFont for the Xft backend
 * @wc: Unicode codepoint to look up
 * 
 * Gets the glyph index for a given unicode codepoint
 * for @font. If you only want to determine
 * whether the font has the glyph, use pango_xft_font_has_char().
 * 
 * Use pango_fc_font_get_glyph() instead.
 *
 * Return value: the glyph index, or 0, if the unicode
 *  codepoint doesn't exist in the font.
 *
 * Since: 1.2
 **/
guint
pango_xft_font_get_glyph (PangoFont *font,
			  gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), 0);

  return pango_fc_font_get_glyph (PANGO_FC_FONT (font), wc);
}

/**
 * pango_xft_font_has_char:
 * @font: a #PangoFont for the Xft backend
 * @wc: Unicode codepoint to look up
 * 
 * Determines whether @font has a glyph for the codepoint @wc.
 * 
 * Use pango_fc_font_has_char() instead.
 *
 * Return value: %TRUE if @font has the requested codepoint.
 *
 * Since: 1.2
 **/
gboolean
pango_xft_font_has_char (PangoFont *font,
			 gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), 0);

  return pango_fc_font_has_char (PANGO_FC_FONT (font), wc);
}
