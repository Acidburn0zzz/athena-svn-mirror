/* Pango
 * pangofc-fontmap.c: Base fontmap type for fontconfig-based backends
 *
 * Copyright (C) 2000-2003 Red Hat, Inc.
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

/* Size of fontset cache */
#define FONTSET_CACHE_SIZE 16

#include "pango-context.h"
#include "pangofc-fontmap.h"
#include "pangofc-private.h"
#include "modules.h"

typedef struct _PangoFcCoverageKey  PangoFcCoverageKey;
typedef struct _PangoFcFace         PangoFcFace;
typedef struct _PangoFcFamily       PangoFcFamily;
typedef struct _PangoFcPatternSet   PangoFcPatternSet;
typedef struct _PangoFcFindFuncInfo PangoFcFindFuncInfo;

#define PANGO_FC_TYPE_FAMILY              (pango_fc_family_get_type ())
#define PANGO_FC_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FAMILY, PangoFcFamily))
#define PANGO_FC_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FAMILY))

#define PANGO_FC_TYPE_FACE              (pango_fc_face_get_type ())
#define PANGO_FC_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FACE, PangoFcFace))
#define PANGO_FC_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FACE))

struct _PangoFcFontMapPrivate
{
  /* We have one map from  PangoFontDescription -> PangoXftPatternSet
   * per language tag.
   */
  GList *fontset_hash_list; 
  /* pattern_hash is used to make sure we only store one copy of
   * each identical pattern. (Speeds up lookup).
   */
  GHashTable *pattern_hash; 
  GHashTable *coverage_hash; /* Maps font file name/id -> PangoCoverage */

  GHashTable *fonts; /* Maps XftPattern -> PangoXftFont */

  GQueue *fontset_cache;	/* Recently used fontsets */

  /* List of all families availible */
  PangoFcFamily **families;
  int n_families;		/* -1 == uninitialized */

  /* Decoders */
  GSList *findfuncs;

  guint closed : 1;
};

struct _PangoFcCoverageKey
{
  char *filename;	
  int id;            /* needed to handle TTC files with multiple faces */
};

struct _PangoFcFace
{
  PangoFontFace parent_instance;

  PangoFcFamily *family;
  char *style;
};

struct _PangoFcFamily
{
  PangoFontFamily parent_instance;

  PangoFcFontMap *fontmap;
  char *family_name;

  PangoFcFace **faces;
  int n_faces;		/* -1 == uninitialized */

  int spacing;  /* FC_SPACING */
};

struct _PangoFcPatternSet
{
  int n_patterns;
  FcPattern **patterns;
  PangoFontset *fontset;
  GList *cache_link;
};

struct _PangoFcFindFuncInfo
{
  PangoFcDecoderFindFunc findfunc;
  gpointer               user_data;
  GDestroyNotify         dnotify;
  gpointer               ddata;
};

static GType    pango_fc_family_get_type     (void);
static GType    pango_fc_face_get_type       (void);

static void          pango_fc_font_map_finalize      (GObject                      *object);
static PangoFont *   pango_fc_font_map_load_font     (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *description);
static PangoFontset *pango_fc_font_map_load_fontset  (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *desc,
						       PangoLanguage                *language);
static void          pango_fc_font_map_list_families (PangoFontMap                 *fontmap,
						       PangoFontFamily            ***families,
						       int                          *n_families);


static void pango_fc_pattern_set_free      (PangoFcPatternSet *patterns);

static guint    pango_fc_pattern_hash       (FcPattern           *pattern);
static gboolean pango_fc_pattern_equal      (FcPattern           *pattern1,
					      FcPattern           *pattern2);
static guint    pango_fc_coverage_key_hash  (PangoFcCoverageKey *key);
static gboolean pango_fc_coverage_key_equal (PangoFcCoverageKey *key1,
					      PangoFcCoverageKey *key2);

G_DEFINE_TYPE (PangoFcFontMap, pango_fc_font_map, PANGO_TYPE_FONT_MAP)

static void 
pango_fc_font_map_init (PangoFcFontMap *fcfontmap)
{
  static gboolean registered_modules = FALSE;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  priv = fcfontmap->priv = G_TYPE_INSTANCE_GET_PRIVATE (fcfontmap,
							PANGO_TYPE_FC_FONT_MAP,
							PangoFcFontMapPrivate);

  if (!registered_modules)
    {
      int i;
      
      registered_modules = TRUE;
      
      for (i = 0; _pango_included_fc_modules[i].list; i++)
        pango_module_register (&_pango_included_fc_modules[i]);
    }

  priv->n_families = -1;

  priv->fonts = g_hash_table_new ((GHashFunc)g_direct_hash, NULL);
  priv->coverage_hash = g_hash_table_new_full ((GHashFunc)pango_fc_coverage_key_hash,
					       (GEqualFunc)pango_fc_coverage_key_equal,
					       (GDestroyNotify)g_free,
					       (GDestroyNotify)pango_coverage_unref);
  priv->fontset_cache = g_queue_new ();
}

static void
pango_fc_font_map_class_init (PangoFcFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  
  object_class->finalize = pango_fc_font_map_finalize;
  fontmap_class->load_font = pango_fc_font_map_load_font;
  fontmap_class->load_fontset = pango_fc_font_map_load_fontset;
  fontmap_class->list_families = pango_fc_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_FC;

  g_type_class_add_private (object_class, sizeof (PangoFcFontMapPrivate));
}

static guint
pango_fc_pattern_hash (FcPattern *pattern)
{
#if 1
  return FcPatternHash (pattern);
#else
  /* Hashing only part of the pattern can improve speed a bit.
   */
  char *str;
  int i;
  double d;
  guint hash = 0;

  FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **) &str);
  if (str)
    hash = g_str_hash (str);

  if (FcPatternGetInteger (pattern, FC_INDEX, 0, &i) == FcResultMatch)
    hash ^= i;

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch)
    hash ^= (guint) (d*1000.0);

  return hash;
#endif  
}

static gboolean
pango_fc_pattern_equal (FcPattern *pattern1,
			 FcPattern *pattern2)
{
  if (pattern1 == pattern2)
    return TRUE;
  else
    return FcPatternEqual (pattern1, pattern2);
}

static guint
pango_fc_coverage_key_hash (PangoFcCoverageKey *key)
{
  return g_str_hash (key->filename) ^ key->id;
}

static gboolean
pango_fc_coverage_key_equal (PangoFcCoverageKey *key1,
			       PangoFcCoverageKey *key2)
{
  return key1->id == key2->id && strcmp (key1->filename, key2->filename) == 0;
}

typedef struct _FontsetHashListNode FontsetHashListNode;

struct _FontsetHashListNode {
  GHashTable    *fontset_hash;
  PangoLanguage *language;
};

/* Get the description => fontset map for a particular
 * language tag.
 */
static GHashTable *
pango_fc_get_fontset_hash (PangoFcFontMap *fcfontmap,
			   PangoLanguage  *language)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
	
  /* We treat NULL as a distinct language tag, but
   * we should actually determine the real language
   * tag it corresponds to to avoid duplicate entries
   * in the list.
   */
  GList *tmp_list = priv->fontset_hash_list;
  while (tmp_list)
    {
      FontsetHashListNode *node = tmp_list->data;
      if (node->language == language)
	{
	  if (tmp_list != priv->fontset_hash_list)
	    {
	      /* Put the found node at the beginning
	       */
	      priv->fontset_hash_list = g_list_remove_link (priv->fontset_hash_list, tmp_list);
	      priv->fontset_hash_list->prev = tmp_list;
	      tmp_list->next = priv->fontset_hash_list;
	      priv->fontset_hash_list = tmp_list;
	    }
	  
	  return node->fontset_hash;
	}
      
      tmp_list = tmp_list->next;
    }

  {
    FontsetHashListNode *node = g_new (FontsetHashListNode, 1);
    priv->fontset_hash_list = g_list_prepend (priv->fontset_hash_list, node);
    
    node->fontset_hash =
      g_hash_table_new_full ((GHashFunc)pango_font_description_hash,
			     (GEqualFunc)pango_font_description_equal,
			     (GDestroyNotify)pango_font_description_free,
			     (GDestroyNotify)pango_fc_pattern_set_free);
    node->language = language;

    return node->fontset_hash;
  }
}

static void
pango_fc_clear_pattern_hashes (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  GList *tmp_list;

  tmp_list = priv->fontset_hash_list;
  while (tmp_list)
    {
      FontsetHashListNode *node = tmp_list->data;
      
      g_hash_table_destroy (node->fontset_hash);
      g_free (node);
      
      tmp_list = tmp_list->next;
    }

  g_list_free (priv->fontset_hash_list);
  priv->fontset_hash_list = NULL;
}

/**
 * pango_fc_font_map_add_decoder_find_func:
 * @fcfontmap: The #PangoFcFontMap to add this method to.
 * @findfunc: The #PangoFcDecoderFindFunc callback function
 * @user_data: User data.
 * @dnotify: A #GDestroyNotify callback that will be called when the
 *  fontmap is finalized and the decoder is released.
 *
 * This function saves a callback method in the #PangoFcFontMap that
 * will be called whenever new fonts are created.  If the
 * function returns a #PangoFcDecoder, that decoder will be used to
 * determine both coverage via a #FcCharSet and a one-to-one mapping of
 * characters to glyphs.  This will allow applications to have
 * application-specific encodings for various fonts.
 *
 * Since: 1.6.
 **/
void
pango_fc_font_map_add_decoder_find_func (PangoFcFontMap        *fcfontmap,
					 PangoFcDecoderFindFunc findfunc,
					 gpointer               user_data,
					 GDestroyNotify         dnotify)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcFindFuncInfo *info;

  info = g_new (PangoFcFindFuncInfo, 1);

  info->findfunc = findfunc;
  info->user_data = user_data;
  info->dnotify = dnotify;

  priv->findfuncs = g_slist_append (priv->findfuncs, info);
}

static void
pango_fc_font_map_finalize (GObject *object)
{
  PangoFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (object);
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  pango_fc_font_map_cache_clear (fcfontmap);
  g_queue_free (priv->fontset_cache);
  g_hash_table_destroy (priv->coverage_hash);

  if (priv->fonts)
    g_hash_table_destroy (priv->fonts);

  if (priv->pattern_hash)
    g_hash_table_destroy (priv->pattern_hash);

  while (priv->findfuncs)
    {
      PangoFcFindFuncInfo *info;
      info = priv->findfuncs->data;
      if (info->dnotify)
	info->dnotify (info->user_data);

      g_free (info);
      priv->findfuncs = g_slist_delete_link (priv->findfuncs, priv->findfuncs);
    }

  G_OBJECT_CLASS (pango_fc_font_map_parent_class)->finalize (object);
}

/* Add a mapping from xfont->font_pattern to xfont */
static void
pango_fc_font_map_add (PangoFcFontMap *fcfontmap,
		       PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  g_assert (fcfont->fontmap == NULL);
	
  g_hash_table_insert (priv->fonts,
		       fcfont->font_pattern,
		       fcfont);
}

/* Remove mapping from xfont->font_pattern to xfont */
void
_pango_fc_font_map_remove (PangoFcFontMap *fcfontmap,
			   PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  
  g_hash_table_remove (priv->fonts,
		       fcfont->font_pattern);
  fcfont->fontmap = NULL;
  g_object_unref (fcfontmap);
}

static PangoFcFamily *
create_family (PangoFcFontMap *fcfontmap,
	       const char     *family_name,
               int             spacing)
{
  PangoFcFamily *family = g_object_new (PANGO_FC_TYPE_FAMILY, NULL);
  family->fontmap = fcfontmap;
  family->family_name = g_strdup (family_name);
  family->spacing = spacing;

  return family;
}

static gboolean
is_alias_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'm':
    case 'M':
      return (g_ascii_strcasecmp (family_name, "monospace") == 0);
    case 's':
    case 'S':
      return (g_ascii_strcasecmp (family_name, "sans") == 0 ||
	      g_ascii_strcasecmp (family_name, "serif") == 0);
    }

  return FALSE;
}

static void
pango_fc_font_map_list_families (PangoFontMap      *fontmap,
				 PangoFontFamily ***families,
				 int               *n_families)
{
  PangoFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (fontmap);
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcFontSet *fontset;
  int i;
  int count;

  if (priv->closed)
    {
      if (families)
	*families = NULL;
      if (n_families)
	n_families = 0;

      return;
    }

  if (priv->n_families < 0)
    {
      FcObjectSet *os = FcObjectSetBuild (FC_FAMILY, FC_SPACING, NULL);
      FcPattern *pat = FcPatternCreate ();
      /* use hash table to avoid duplicate listings if different faces in
       * the same family have different spacing values */
      GHashTable *temp_family_hash;

      fontset = FcFontList (NULL, pat, os);
      
      FcPatternDestroy (pat);
      FcObjectSetDestroy (os);
      
      priv->families = g_new (PangoFcFamily *, fontset->nfont + 3); /* 3 standard aliases */
      temp_family_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      count = 0;
      for (i = 0; i < fontset->nfont; i++)
	{
	  FcChar8 *s;
	  FcResult res;
          int spacing;
	  
	  res = FcPatternGetString (fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **) &s);
	  g_assert (res == FcResultMatch);

          res = FcPatternGetInteger (fontset->fonts[i], FC_SPACING, 0, &spacing);
          g_assert (res == FcResultMatch || res == FcResultNoMatch);
          if (res == FcResultNoMatch)
            spacing = FC_PROPORTIONAL;
	  
	  if (!is_alias_family (s) && !g_hash_table_lookup (temp_family_hash, s))
            {
              PangoFcFamily *temp_family = create_family (fcfontmap, (gchar *)s, spacing);
              g_hash_table_insert (temp_family_hash, g_strdup (s), s);
              priv->families[count++] = temp_family;
            }
	}

      FcFontSetDestroy (fontset);
      g_hash_table_destroy (temp_family_hash);

      priv->families[count++] = create_family (fcfontmap, "Sans", FC_PROPORTIONAL);
      priv->families[count++] = create_family (fcfontmap, "Serif", FC_PROPORTIONAL);
      priv->families[count++] = create_family (fcfontmap, "Monospace", FC_MONO);
      
      priv->n_families = count;
    }

  if (n_families)
    *n_families = priv->n_families;
  
  if (families)
    *families = g_memdup (priv->families, priv->n_families * sizeof (PangoFontFamily *));
}

static int
pango_fc_convert_weight_to_fc (PangoWeight pango_weight)
{
  if (pango_weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_LIGHT) / 2)
    return FC_WEIGHT_LIGHT;
  else if (pango_weight < (PANGO_WEIGHT_NORMAL + 600) / 2)
    return FC_WEIGHT_MEDIUM;
  else if (pango_weight < (600 + PANGO_WEIGHT_BOLD) / 2)
    return FC_WEIGHT_DEMIBOLD;
  else if (pango_weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    return FC_WEIGHT_BOLD;
  else
    return FC_WEIGHT_BLACK;
}

static int
pango_fc_convert_slant_to_fc (PangoStyle pango_style)
{
  switch (pango_style)
    {
    case PANGO_STYLE_NORMAL:
      return FC_SLANT_ROMAN;
    case PANGO_STYLE_ITALIC:
      return FC_SLANT_ITALIC;
    case PANGO_STYLE_OBLIQUE:
      return FC_SLANT_OBLIQUE;
    default:
      return FC_SLANT_ROMAN;
    }
}

#ifdef FC_WIDTH
static int
pango_fc_convert_width_to_fc (PangoStretch pango_stretch)
{
  switch (pango_stretch)
    {
    case PANGO_STRETCH_NORMAL:
      return FC_WIDTH_NORMAL;
    case PANGO_STRETCH_ULTRA_CONDENSED:
      return FC_WIDTH_ULTRACONDENSED;
    case PANGO_STRETCH_EXTRA_CONDENSED:
      return FC_WIDTH_EXTRACONDENSED;
    case PANGO_STRETCH_CONDENSED:
      return FC_WIDTH_CONDENSED;
    case PANGO_STRETCH_SEMI_CONDENSED:
      return FC_WIDTH_SEMICONDENSED;
    case PANGO_STRETCH_SEMI_EXPANDED:
      return FC_WIDTH_SEMIEXPANDED;
    case PANGO_STRETCH_EXPANDED:
      return FC_WIDTH_EXPANDED;
    case PANGO_STRETCH_EXTRA_EXPANDED:
      return FC_WIDTH_EXTRAEXPANDED;
    case PANGO_STRETCH_ULTRA_EXPANDED:
      return FC_WIDTH_ULTRAEXPANDED;
    default:
      return FC_WIDTH_NORMAL;
    }
}
#endif

static FcPattern *
pango_fc_make_pattern (const PangoFontDescription *description)
{
  FcPattern *pattern;
  int slant;
  int weight;
  double size;
  char **families;
  int i;
#ifdef FC_WIDTH
  int width;
#endif

  slant = pango_fc_convert_slant_to_fc (pango_font_description_get_style (description));
  weight = pango_fc_convert_weight_to_fc (pango_font_description_get_weight (description));
#ifdef FC_WIDTH
  width = pango_fc_convert_width_to_fc (pango_font_description_get_stretch (description));
#endif

  size = (double) pango_font_description_get_size (description) / PANGO_SCALE;
  
  pattern = FcPatternBuild (0,
			    FC_WEIGHT, FcTypeInteger, weight,
			    FC_SLANT,  FcTypeInteger, slant,
#ifdef FC_WIDTH
			    FC_WIDTH,  FcTypeInteger, width,
#endif
			    FC_SIZE,   FcTypeDouble,  size,
			    NULL);

  families = g_strsplit (pango_font_description_get_family (description), ",", -1);
  
  for (i = 0; families[i]; i++)
    FcPatternAddString (pattern, FC_FAMILY, families[i]);

  g_strfreev (families);

  return pattern;
}

static PangoFont *
pango_fc_font_map_new_font (PangoFontMap      *fontmap,
			    const PangoMatrix *pango_matrix,
			    FcPattern     *match)
{
  PangoFcFontMap *fcfontmap = (PangoFcFontMap *)fontmap;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *pattern;
  PangoFcFont *fcfont;
  GSList *l;

  /* Returning NULL here actually violates a contract
   * that loading load_font() will never return NULL.
   * We probably should actually create a dummy
   * font that doesn't draw anything and has empty
   * metrics.
   */
  if (priv->closed)
    return NULL;
  
  /* Look up cache */
  if (!pango_matrix)
    {
      fcfont = g_hash_table_lookup (priv->fonts, match);
  
      if (fcfont)
	return g_object_ref (fcfont);
    }

  if (pango_matrix)
    {
      FcMatrix fc_matrix;

      /* FontConfig has the Y axis pointing up, Pango, down.
       */
      fc_matrix.xx = pango_matrix->xx;
      fc_matrix.xy = - pango_matrix->xy;
      fc_matrix.yx = - pango_matrix->yx;
      fc_matrix.yy = pango_matrix->yy;
      
      pattern = FcPatternDuplicate (match);
      FcPatternAddMatrix (pattern, FC_MATRIX, &fc_matrix);
    }
  else
    pattern = match;

  fcfont = PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->new_font (fcfontmap, pattern);

  if (!pango_matrix)
    pango_fc_font_map_add (fcfontmap, fcfont);

  if (pango_matrix)
    FcPatternDestroy (pattern);

  fcfont->fontmap = g_object_ref (fcfontmap);

  /*
   * Give any custom decoders a crack at this font now that it's been
   * created.
   */
  for (l = priv->findfuncs; l && l->data; l = l->next)
    {
      PangoFcFindFuncInfo *info = l->data;
      PangoFcDecoder *decoder;

      decoder = info->findfunc (match, info->user_data);
      if (decoder)
	{
	  _pango_fc_font_set_decoder (fcfont, decoder);
	  break;
	}
    }

  return (PangoFont *)fcfont;
}

static FcPattern *
uniquify_pattern (PangoFcFontMap *fcfontmap,
		  FcPattern      *pattern)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *old_pattern;
  
  if (!priv->pattern_hash)
    priv->pattern_hash =
      g_hash_table_new_full ((GHashFunc)pango_fc_pattern_hash,
			     (GEqualFunc)pango_fc_pattern_equal,
			     (GDestroyNotify)FcPatternDestroy, NULL);
  
  old_pattern = g_hash_table_lookup (priv->pattern_hash, pattern);
  if (old_pattern)
    {
      FcPatternDestroy (pattern);
      FcPatternReference (old_pattern);
      return old_pattern;
    }
  else
    {
      FcPatternReference (pattern);
      g_hash_table_insert (priv->pattern_hash, pattern, pattern);
      return pattern;
    }
}

static void
pango_fc_default_substitute (PangoFcFontMap    *fontmap,
			     FcPattern          *pattern)
{
  if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute (fontmap, pattern);
}

static PangoFcPatternSet *
pango_fc_font_map_get_patterns (PangoFontMap               *fontmap,
				 PangoContext               *context,
				 const PangoFontDescription *desc,
				 PangoLanguage              *language)
{
  PangoFcFontMap *fcfontmap = (PangoFcFontMap *)fontmap;
  FcPattern *pattern, *font_pattern;
  FcResult res;
  int f;
  PangoFcPatternSet *patterns;
  FcFontSet *font_patterns;
  GHashTable *fontset_hash;

  if (!language && context)
    language = pango_context_get_language (context);
  
  fontset_hash = pango_fc_get_fontset_hash (fcfontmap, language);
  patterns = g_hash_table_lookup (fontset_hash, desc);

  if (patterns == NULL)
    {
      pattern = pango_fc_make_pattern (desc);
      if (language)
	FcPatternAddString (pattern, FC_LANG, (FcChar8 *) pango_language_to_string (language));

      pango_fc_default_substitute (fcfontmap, pattern);
      
      font_patterns = FcFontSort (NULL, pattern, FcTrue, 0, &res);

      if (!font_patterns)
	{
	  g_printerr ("No fonts found; this probably means that the fontconfig\n"
		      "library is not correctly configured. You may need to\n"
		      "edit the fonts.conf configuration file. More information\n"
		      "about fontconfig can be found in the fontconfig(3) manual\n"
		      "page and on http://fontconfig.org\n");

	  /* There is no point in proceeding; we'll just get a segfault later
	   * on, and a bunch more possibly confusing error messages in between.
	   */
	  
	  /* return NULL; */
	  exit (1);
	}

      patterns = g_new (PangoFcPatternSet, 1);
      patterns->patterns = g_new (FcPattern *, font_patterns->nfont);
      patterns->n_patterns = 0;
      patterns->fontset = NULL;
      patterns->cache_link = NULL;

      for (f = 0; f < font_patterns->nfont; f++)
	{
	  font_pattern = FcFontRenderPrepare (NULL, pattern,
					      font_patterns->fonts[f]);

	  if (font_pattern)
	    {
#ifdef FC_PATTERN
	      /* The FC_PATTERN element, which points back to our the original
	       * pattern defeats our hash tables.
	       */
	      FcPatternDel (font_pattern, FC_PATTERN);
#endif /* FC_PATTERN */

	      patterns->patterns[patterns->n_patterns++] = uniquify_pattern (fcfontmap, font_pattern);
	    }
	}
      
      FcPatternDestroy (pattern);
      
      FcFontSetSortDestroy (font_patterns);

      g_hash_table_insert (fontset_hash,
			   pango_font_description_copy (desc),
			   patterns);
    }

  return patterns;
}

static PangoFont *
pango_fc_font_map_load_font (PangoFontMap               *fontmap,
			     PangoContext               *context,
			     const PangoFontDescription *description)
{
  PangoFcPatternSet *patterns = pango_fc_font_map_get_patterns (fontmap, context, description, NULL);
  if (!patterns)
    return NULL;

  if (patterns->n_patterns > 0)
    {
      return pango_fc_font_map_new_font (fontmap,
					 context ? pango_context_get_matrix (context) : NULL,
					 patterns->patterns[0]);
    }
  
  return NULL;
}

static void
pango_fc_pattern_set_free (PangoFcPatternSet *patterns)
{
  int i;

  if (patterns->fontset)
    g_object_remove_weak_pointer (G_OBJECT (patterns->fontset),
				  (gpointer *)&patterns->fontset);
  
  for (i = 0; i < patterns->n_patterns; i++)
    FcPatternDestroy (patterns->patterns[i]);

  g_free (patterns->patterns);
  g_free (patterns);
}

static void
pango_fc_font_map_cache_fontset (PangoFcFontMap    *fcfontmap,
				 PangoFcPatternSet *patterns)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  GQueue *cache = priv->fontset_cache;
  
  if (patterns->cache_link)
    {
      /* Already in cache, move to head
       */
      if (patterns->cache_link == cache->tail)
	cache->tail = patterns->cache_link->prev;

      cache->head = g_list_remove_link (cache->head, patterns->cache_link);
      cache->length--;
    }
  else
    {
      /* Add to cache initially
       */
      if (cache->length == FONTSET_CACHE_SIZE)
	{
	  PangoFcPatternSet *tmp_patterns = g_queue_pop_tail (cache);
	  tmp_patterns->cache_link = NULL;
	  g_object_unref (tmp_patterns->fontset);
	}
	
      g_object_ref (patterns->fontset);
      patterns->cache_link = g_list_prepend (NULL, patterns);
    }

  g_queue_push_head_link (cache, patterns->cache_link);
}

static PangoFontset *
pango_fc_font_map_load_fontset (PangoFontMap                 *fontmap,
				PangoContext                 *context,
				const PangoFontDescription   *desc,
				PangoLanguage                *language)
{
  PangoFcPatternSet *patterns = pango_fc_font_map_get_patterns (fontmap, context, desc, language);
  PangoFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (fontmap);
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFontset *result;
  const PangoMatrix *matrix;
  int i;
  
  if (!patterns)
    return NULL;

  if (context)
    matrix = pango_context_get_matrix (context);
  else
    matrix = NULL;

  /* We never cache fontsets when a transformation is in place
   */
  if (!patterns->fontset || matrix)
    {
      PangoFontsetSimple *simple;
      simple = pango_fontset_simple_new (language);
      result = PANGO_FONTSET (simple);
      
      for (i = 0; i < patterns->n_patterns; i++)
	{
	  PangoFont *font;

	  font = pango_fc_font_map_new_font (fontmap, matrix, patterns->patterns[i]);
	  if (font)
	    pango_fontset_simple_append (simple, font);
	}

      if (!matrix)
	{
	  patterns->fontset = PANGO_FONTSET (simple);
	  g_object_add_weak_pointer (G_OBJECT (patterns->fontset),
				     (gpointer *)&patterns->fontset);
	}
    }
  else
    result = g_object_ref (patterns->fontset);
  
  if (!matrix &&
      (!patterns->cache_link ||
       patterns->cache_link != priv->fontset_cache->head))
    pango_fc_font_map_cache_fontset (fcfontmap, patterns);

  return result;
}

static void
uncache_patterns (PangoFcPatternSet *patterns)
{
  g_object_unref (patterns->fontset);
}

static void
pango_fc_font_map_clear_fontset_cache (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  GQueue *cache = priv->fontset_cache;
  
  g_list_foreach (cache->head, (GFunc)uncache_patterns, NULL);
  g_list_free (cache->head);
  cache->head = NULL;
  cache->tail = NULL;
  cache->length = 0;
}

/**
 * pango_fc_font_map_cache_clear:
 * @fcfontmap: a #PangoFcFontmap
 * 
 * Clear all cached information and fontsets for this font map;
 * this should be called whenever there is a change in the
 * output of the default_substitute() virtual function.
 *
 * This function is intended to be used only by backend implementations
 * deriving from #PangoFcFontmap.
 *
 * Since: 1.4
 **/
void
pango_fc_font_map_cache_clear (PangoFcFontMap *fcfontmap)
{
  /* Clear the fontset cache first, since any entries
   * in the fontset_cache must also be in the pattern cache.
   */
  pango_fc_font_map_clear_fontset_cache (fcfontmap);
  pango_fc_clear_pattern_hashes (fcfontmap);
}

static void
pango_fc_font_map_set_coverage (PangoFcFontMap            *fcfontmap,
				PangoFcCoverageKey        *key,
				PangoCoverage             *coverage)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcCoverageKey *key_dup;

  key_dup = g_malloc (sizeof (PangoFcCoverageKey) + strlen (key->filename) + 1);
  key_dup->id = key->id;
  key_dup->filename = (char *) (key_dup + 1);
  strcpy (key_dup->filename, key->filename);
  
  g_hash_table_insert (priv->coverage_hash,
		       key_dup, pango_coverage_ref (coverage));
}

PangoCoverage *
_pango_fc_font_map_get_coverage (PangoFcFontMap *fcfontmap,
				 PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcCoverageKey key;
  PangoCoverage *coverage;
  FcCharSet *charset;
  
  /*
   * Assume that coverage information is identified by
   * a filename/index pair; there shouldn't be any reason
   * this isn't true, but it's not specified anywhere
   */
  if (FcPatternGetString (fcfont->font_pattern, FC_FILE, 0, (FcChar8 **) &key.filename) != FcResultMatch)
    return NULL;

  if (FcPatternGetInteger (fcfont->font_pattern, FC_INDEX, 0, &key.id) != FcResultMatch)
    return NULL;
  
  coverage = g_hash_table_lookup (priv->coverage_hash, &key);
  if (coverage)
    return pango_coverage_ref (coverage);

  /*
   * Pull the coverage out of the pattern, this
   * doesn't require loading the font
   */
  if (FcPatternGetCharSet (fcfont->font_pattern, FC_CHARSET, 0, &charset) != FcResultMatch)
    return NULL;

  coverage = _pango_fc_font_map_fc_to_coverage (charset);

  pango_fc_font_map_set_coverage (fcfontmap, &key, coverage);
 
  return coverage;
}

/**
 * _pango_fc_font_map_fc_to_coverage:
 * @charset: #FcCharSet to convert to a #PangoCoverage object.
 *
 * Convert the given #FcCharSet into a new #PangoCoverage object.  The
 * caller is responsible for freeing the newly created object.
 *
 * Since: 1.6
 **/

PangoCoverage  *
_pango_fc_font_map_fc_to_coverage (FcCharSet *charset)
{
  PangoCoverage *coverage;
  FcChar32  ucs4, pos;
  FcChar32  map[FC_CHARSET_MAP_SIZE];
  int i;

  /*
   * Convert an Fc CharSet into a pango coverage structure.  Sure
   * would be nice to just use the Fc structure in place...
   */
  coverage = pango_coverage_new ();
  for (ucs4 = FcCharSetFirstPage (charset, map, &pos);
       ucs4 != FC_CHARSET_DONE;
       ucs4 = FcCharSetNextPage (charset, map, &pos))
    {
      for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
	{
	  FcChar32  bits = map[i];
	  FcChar32  base = ucs4 + i * 32;
	  int b = 0;
	  bits = map[i];
	  while (bits)
	    {
	      if (bits & 1)
		pango_coverage_set (coverage, base + b, PANGO_COVERAGE_EXACT);

	      bits >>= 1;
	      b++;
	    }
	}
    }

  /* Awful hack so Hangul Tone marks get rendered with the same
   * font and in the same run as other Hangul characters. If a font
   * covers the first composed Hangul glyph, then it is declared to cover
   * the Hangul tone marks. This hack probably needs to be formalized
   * by choosing fonts for scripts rather than individual code points.
   */
  if (pango_coverage_get (coverage, 0xac00) == PANGO_COVERAGE_EXACT)
    {
      pango_coverage_set (coverage, 0x302e, PANGO_COVERAGE_EXACT);
      pango_coverage_set (coverage, 0x302f, PANGO_COVERAGE_EXACT);
    }

  return coverage;
}

/**
 * pango_fc_font_map_create_context:
 * @fcfontmap: a #PangoFcFontMap
 * 
 * Creates a new context for this fontmap. This function is intended
 * only for backend implementations deriving from #PangoFcFontmap;
 * it is possible that a backend will store additional information
 * needed for correct operation on the #PangoContext after calling
 * this function.
 * 
 * Return value: a new #PangoContext 
 *
 * Since: 1.4
 **/
PangoContext *
pango_fc_font_map_create_context (PangoFcFontMap *fcfontmap)
{
  PangoContext *context = pango_context_new ();
  pango_context_set_font_map (context, PANGO_FONT_MAP (fcfontmap));

  return context;
}

static void
cleanup_font (gpointer        key,
	      PangoFcFont    *fcfont)
{
  _pango_fc_font_shutdown (fcfont);

  g_object_unref (fcfont->fontmap);
  fcfont->fontmap = NULL;
}

/**
 * pango_fc_font_map_shutdown:
 * @fcfontmap: a #PangoFcFontmap
 * 
 * Clears all cached information for the fontmap and marks
 * all fonts open for the fontmap as dead. (See the shutdown()
 * virtual function of PangoFcFont.) This function might be used
 * by a backend when the underlying windowing system for the font
 * map exits. This function is only intended to be called from
 * only for backend implementations deriving from #PangoFcFontmap.
 *
 * Since: 1.4
 **/
void
pango_fc_font_map_shutdown (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  pango_fc_font_map_cache_clear (fcfontmap);
  
  g_hash_table_foreach (priv->fonts, (GHFunc)cleanup_font, NULL);
  g_hash_table_destroy (priv->fonts);
  priv->fonts = NULL;
  priv->closed = TRUE;
}

static PangoWeight
pango_fc_convert_weight_to_pango (int fc_weight)
{
  if (fc_weight < FC_WEIGHT_LIGHT)
    return PANGO_WEIGHT_ULTRALIGHT;
  else if (fc_weight < (FC_WEIGHT_LIGHT + FC_WEIGHT_MEDIUM) / 2)
    return PANGO_WEIGHT_LIGHT;
  else if (fc_weight < (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
    return PANGO_WEIGHT_NORMAL;
  else if (fc_weight < (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
    return 600;
  else if (fc_weight < (FC_WEIGHT_BOLD + FC_WEIGHT_BLACK) / 2)
    return PANGO_WEIGHT_BOLD;
  else
    return PANGO_WEIGHT_ULTRABOLD;
}

static PangoStyle
pango_fc_convert_slant_to_pango (int fc_style)
{
  switch (fc_style)
    {
    case FC_SLANT_ROMAN:
      return PANGO_STYLE_NORMAL;
    case FC_SLANT_ITALIC:
      return PANGO_STYLE_ITALIC;
    case FC_SLANT_OBLIQUE:
      return PANGO_STYLE_OBLIQUE;
    default:
      return PANGO_STYLE_NORMAL;
    }
}

#ifdef FC_WIDTH
static PangoStretch
pango_fc_convert_width_to_pango (int fc_stretch)
{
  switch (fc_stretch)
    {
    case FC_WIDTH_NORMAL:
      return PANGO_STRETCH_NORMAL;
    case FC_WIDTH_ULTRACONDENSED:
      return PANGO_STRETCH_ULTRA_CONDENSED;
    case FC_WIDTH_EXTRACONDENSED:
      return PANGO_STRETCH_EXTRA_CONDENSED;
    case FC_WIDTH_CONDENSED:
      return PANGO_STRETCH_CONDENSED;
    case FC_WIDTH_SEMICONDENSED:
      return PANGO_STRETCH_SEMI_CONDENSED;
    case FC_WIDTH_SEMIEXPANDED:
      return PANGO_STRETCH_SEMI_EXPANDED;
    case FC_WIDTH_EXPANDED:
      return PANGO_STRETCH_EXPANDED;
    case FC_WIDTH_EXTRAEXPANDED:
      return PANGO_STRETCH_EXTRA_EXPANDED;
    case FC_WIDTH_ULTRAEXPANDED:
      return PANGO_STRETCH_ULTRA_EXPANDED;
    default:
      return PANGO_STRETCH_NORMAL;
    }
}
#endif

/**
 * pango_fc_font_description_from_pattern:
 * @pattern: a #FcPattern
 * @include_size: if %TRUE, the pattern will include the size from
 *   the @pattern; otherwise the resulting pattern will be unsized.
 * 
 * Creates a #PangoFontDescription that matches the specified
 * Fontconfig pattern as closely as possible. Many possible Fontconfig
 * pattern values, such as %FC_RASTERIZER or %FC_DPI, don't make sense in
 * the context of #PangoFontDescription, so will be ignored.
 * 
 * Return value: a new #PangoFontDescription. Free with
 *  pango_font_description_free().
 *
 * Since: 1.4
 **/
PangoFontDescription *
pango_fc_font_description_from_pattern (FcPattern *pattern, gboolean include_size)
{
  PangoFontDescription *desc;
  PangoStyle style;
  PangoWeight weight;
  PangoStretch stretch;
  double size;
  
  FcChar8 *s;
  int i;
  FcResult res;

  desc = pango_font_description_new ();

  res = FcPatternGetString (pattern, FC_FAMILY, 0, (FcChar8 **) &s);
  g_assert (res == FcResultMatch);

  pango_font_description_set_family (desc, (gchar *)s);
  
  if (FcPatternGetInteger (pattern, FC_SLANT, 0, &i) == FcResultMatch)
    style = pango_fc_convert_slant_to_pango (i);
  else
    style = PANGO_STYLE_NORMAL;

  pango_font_description_set_style (desc, style);

  if (FcPatternGetInteger (pattern, FC_WEIGHT, 0, &i) == FcResultMatch)
    weight = pango_fc_convert_weight_to_pango (i);
  else
    weight = PANGO_WEIGHT_NORMAL;
  
  pango_font_description_set_weight (desc, weight);

#ifdef FC_WIDTH
  if (FcPatternGetInteger (pattern, FC_WIDTH, 0, &i) == FcResultMatch)
    stretch = pango_fc_convert_width_to_pango (i);
  else
#endif
    stretch = PANGO_STRETCH_NORMAL;

  pango_font_description_set_stretch (desc, stretch);
  
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);

  if (include_size && FcPatternGetDouble (pattern, FC_SIZE, 0, &size) == FcResultMatch)
    pango_font_description_set_size (desc, size * PANGO_SCALE);

  return desc;
}

/* 
 * PangoFcFace
 */

static PangoFontDescription *
make_alias_description (PangoFcFamily *fcfamily,
			gboolean        bold,
			gboolean        italic)
{
  PangoFontDescription *desc = pango_font_description_new ();

  pango_font_description_set_family (desc, fcfamily->family_name);
  pango_font_description_set_style (desc, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  return desc;
}

static PangoFontDescription *
pango_fc_face_describe (PangoFontFace *face)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);
  PangoFcFamily *fcfamily = fcface->family;
  PangoFontDescription *desc = NULL;
  FcResult res;
  FcPattern *match_pattern;
  FcPattern *result_pattern;

  if (is_alias_family (fcfamily->family_name))
    {
      if (strcmp (fcface->style, "Regular") == 0)
	return make_alias_description (fcfamily, FALSE, FALSE);
      else if (strcmp (fcface->style, "Bold") == 0)
	return make_alias_description (fcfamily, TRUE, FALSE); 
      else if (strcmp (fcface->style, "Italic") == 0)
	return make_alias_description (fcfamily, FALSE, TRUE);
      else			/* Bold Italic */
	return make_alias_description (fcfamily, TRUE, TRUE);
    }
  
  match_pattern = FcPatternBuild (NULL,
				  FC_FAMILY, FcTypeString, fcfamily->family_name,
				  FC_STYLE, FcTypeString, fcface->style,
				  NULL);

  g_assert (match_pattern);
  
  result_pattern = FcFontMatch (NULL, match_pattern, &res);
  if (result_pattern)
    {
      desc = pango_fc_font_description_from_pattern (result_pattern, FALSE);
      FcPatternDestroy (result_pattern);
    }

  FcPatternDestroy (match_pattern);
  
  return desc;
}

static const char *
pango_fc_face_get_face_name (PangoFontFace *face)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);

  return fcface->style;
}

static int
compare_ints (gconstpointer ap,
              gconstpointer bp)
{
  int a = *(int *)ap;
  int b = *(int *)bp;

  if (a == b)
    return 0;
  else if (a > b)
    return 1;
  else
    return -1;
}

static void
pango_fc_face_list_sizes (PangoFontFace  *face,
                          int           **sizes,
                          int            *n_sizes)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);
  FcPattern *pattern;
  FcFontSet *fontset;
  FcObjectSet *objectset;

  pattern = FcPatternCreate ();
  FcPatternAddString (pattern, FC_FAMILY, fcface->family->family_name);
  FcPatternAddString (pattern, FC_STYLE, fcface->style);

  objectset = FcObjectSetCreate ();
  FcObjectSetAdd (objectset, FC_PIXEL_SIZE);

  fontset = FcFontList (NULL, pattern, objectset);

  if (fontset)
    {
      GArray *size_array;
      double size, dpi = -1.0;
      int i, size_i;
      
      size_array = g_array_new (FALSE, FALSE, sizeof (int));

      for (i = 0; i < fontset->nfont; i++)
        {
          if (FcPatternGetDouble (fontset->fonts[i], FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
            {
              if (dpi < 0)
                {
                  FcPattern *tmp = FcPatternDuplicate (fontset->fonts[i]);
                  pango_fc_default_substitute (fcface->family->fontmap, tmp);
                  if (FcPatternGetDouble (tmp, FC_DPI, 0, &dpi) != FcResultMatch)
                    {
                      g_warning ("Error getting DPI from fontconfig, using 72.0");
                      dpi = 72.0;
                    }
                  FcPatternDestroy (tmp);
                }

              size_i = (int) (PANGO_SCALE * size * 72.0 / dpi);
              g_array_append_val (size_array, size_i);
            }
        }

      g_array_sort (size_array, compare_ints);
      
      if (size_array->len == 0)
        {
          *n_sizes = 0;
          if (sizes)
            *sizes = NULL;
          g_array_free (size_array, TRUE);
        }
      else
        {
          *n_sizes = size_array->len;
          if (sizes)
            {
              *sizes = (int *) size_array->data;
              g_array_free (size_array, FALSE);
            }
          else
            g_array_free (size_array, TRUE);
        }

      FcFontSetDestroy (fontset);
    }
  else
    {
      *n_sizes = 0;
      if (sizes)
        *sizes = NULL;
    }

  FcPatternDestroy (pattern);
  FcObjectSetDestroy (objectset);
}

static gboolean
pango_fc_family_is_monospace (PangoFontFamily *family)
{
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->spacing == FC_MONO || 
#ifdef FC_DUAL
         fcfamily->spacing == FC_DUAL ||
#endif
         fcfamily->spacing == FC_CHARCELL;
}

static void
pango_fc_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_fc_face_describe;
  class->get_face_name = pango_fc_face_get_face_name;
  class->list_sizes = pango_fc_face_list_sizes;
}

static GType
pango_fc_face_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_fc_face_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFcFace),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
                                            "PangoFcFace",
                                            &object_info, 0);
    }
  
  return object_type;
}

/*
 * PangoFcFamily
 */
static PangoFcFace *
create_face (PangoFcFamily *fcfamily,
	     const char     *style)
{
  PangoFcFace *face = g_object_new (PANGO_FC_TYPE_FACE, NULL);
  face->style = g_strdup (style);
  face->family = fcfamily;

  return face;
}

static void
pango_fc_family_list_faces (PangoFontFamily  *family,
			     PangoFontFace  ***faces,
			     int              *n_faces)
{
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (family);
  PangoFcFontMap *fcfontmap = fcfamily->fontmap;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  
  if (fcfamily->n_faces < 0)
    {
      FcFontSet *fontset;
      int i;
      
      if (is_alias_family (fcfamily->family_name) || priv->closed)
	{
	  fcfamily->n_faces = 4;
	  fcfamily->faces = g_new (PangoFcFace *, fcfamily->n_faces);

	  i = 0;
	  fcfamily->faces[i++] = create_face (fcfamily, "Regular");
	  fcfamily->faces[i++] = create_face (fcfamily, "Bold");
	  fcfamily->faces[i++] = create_face (fcfamily, "Italic");
	  fcfamily->faces[i++] = create_face (fcfamily, "Bold Italic");
	}
      else
	{
	  FcObjectSet *os = FcObjectSetBuild (FC_STYLE, NULL);
	  FcPattern *pat = FcPatternBuild (NULL, 
					   FC_FAMILY, FcTypeString, fcfamily->family_name,
					   NULL);
      
	  fontset = FcFontList (NULL, pat, os);
      
	  FcPatternDestroy (pat);
	  FcObjectSetDestroy (os);
      
	  fcfamily->n_faces = fontset->nfont;
	  fcfamily->faces = g_new (PangoFcFace *, fcfamily->n_faces);
	  
	  for (i = 0; i < fontset->nfont; i++)
	    {
	      FcChar8 *s;
	      FcResult res;

	      res = FcPatternGetString (fontset->fonts[i], FC_STYLE, 0, &s);
	      if (res != FcResultMatch)
		s = "Regular";

	      fcfamily->faces[i] = create_face (fcfamily, s);
	    }

	  FcFontSetDestroy (fontset);
	}
    }
  
  if (n_faces)
    *n_faces = fcfamily->n_faces;
  
  if (faces)
    *faces = g_memdup (fcfamily->faces, fcfamily->n_faces * sizeof (PangoFontFace *));
}

static const char *
pango_fc_family_get_name (PangoFontFamily  *family)
{
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->family_name;
}

static void
pango_fc_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_fc_family_list_faces;
  class->get_name = pango_fc_family_get_name;
  class->is_monospace = pango_fc_family_is_monospace;
}

static void
pango_fc_family_init (PangoFcFamily *fcfamily)
{
  fcfamily->n_faces = -1;
}

static GType
pango_fc_family_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFamilyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_fc_family_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFcFamily),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_fc_family_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
                                            "PangoFcFamily",
                                            &object_info, 0);
    }
  
  return object_type;
}
