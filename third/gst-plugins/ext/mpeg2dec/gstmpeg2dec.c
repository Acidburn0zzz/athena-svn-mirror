/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include <inttypes.h>

#include "gstmpeg2dec.h"

/* mpeg2dec changed a struct name after 0.3.1, here's a workaround */
/* mpeg2dec also only defined MPEG2_RELEASE after 0.3.1
#if MPEG2_RELEASE < MPEG2_VERSION(0,3,2)
*/
#ifndef MPEG2_RELEASE
#define MPEG2_VERSION(a,b,c) ((((a)&0xff)<<16)|(((b)&0xff)<<8)|((c)&0xff))
#define MPEG2_RELEASE MPEG2_VERSION(0,3,1)
typedef picture_t mpeg2_picture_t;
typedef gint mpeg2_state_t;

#define STATE_BUFFER 0
#endif

GST_DEBUG_CATEGORY_EXTERN (GST_CAT_SEEK);
GST_DEBUG_CATEGORY_STATIC (mpeg2dec_debug);
#define GST_CAT_DEFAULT (mpeg2dec_debug)

/* table with framerates expressed as fractions */
static gdouble fpss[] = { 24.0 / 1.001, 24.0, 25.0,
  30.0 / 1.001, 30.0, 50.0,
  60.0 / 1.001, 60.0, 0
};

/* frame periods */
static guint frame_periods[] = {
  1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000, 0
};

/* elementfactory information */
static GstElementDetails gst_mpeg2dec_details = {
  "mpeg1 and mpeg2 video decoder",
  "Codec/Decoder/Video",
  "Uses libmpeg2 to decode MPEG video streams",
  "Wim Taymans <wim.taymans@chello.be>",
};

/* Mpeg2dec signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0
      /* FILL ME */
};

/*
 * We can't use fractions in static pad templates, so
 * we do something manual...
 */
static GstPadTemplate *
src_templ (void)
{
  static GstPadTemplate *templ = NULL;

  if (!templ) {
    GstCaps *caps;
    GstStructure *structure;
    GValue list = { 0 }
    , fps = {
    0}
    , fmt = {
    0};
    char *fmts[] = { "YV12", "I420", "Y42B", NULL };
    guint n;

    caps = gst_caps_new_simple ("video/x-raw-yuv",
        "format", GST_TYPE_FOURCC,
        GST_MAKE_FOURCC ('I', '4', '2', '0'),
        "width", GST_TYPE_INT_RANGE, 16, 4096,
        "height", GST_TYPE_INT_RANGE, 16, 4096, NULL);

    structure = gst_caps_get_structure (caps, 0);

    g_value_init (&list, GST_TYPE_LIST);
    g_value_init (&fps, G_TYPE_DOUBLE);
    for (n = 0; fpss[n] != 0; n++) {
      g_value_set_double (&fps, fpss[n]);
      gst_value_list_append_value (&list, &fps);
    }
    gst_structure_set_value (structure, "framerate", &list);
    g_value_unset (&list);
    g_value_unset (&fps);

    g_value_init (&list, GST_TYPE_LIST);
    g_value_init (&fmt, GST_TYPE_FOURCC);
    for (n = 0; fmts[n] != NULL; n++) {
      gst_value_set_fourcc (&fmt, GST_STR_FOURCC (fmts[n]));
      gst_value_list_append_value (&list, &fmt);
    }
    gst_structure_set_value (structure, "format", &list);
    g_value_unset (&list);
    g_value_unset (&fmt);

    templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, caps);
  }
  return templ;
}

#ifdef enable_user_data
static GstStaticPadTemplate user_data_template_factory =
GST_STATIC_PAD_TEMPLATE ("user_data",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);
#endif

static GstStaticPadTemplate sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg, "
        "mpegversion = (int) [ 1, 2 ], " "systemstream = (boolean) false")
    );

static void gst_mpeg2dec_base_init (gpointer g_class);
static void gst_mpeg2dec_class_init (GstMpeg2decClass * klass);
static void gst_mpeg2dec_init (GstMpeg2dec * mpeg2dec);

static void gst_mpeg2dec_dispose (GObject * object);

static void gst_mpeg2dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_mpeg2dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_mpeg2dec_set_index (GstElement * element, GstIndex * index);
static GstIndex *gst_mpeg2dec_get_index (GstElement * element);

static const GstFormat *gst_mpeg2dec_get_src_formats (GstPad * pad);
static const GstEventMask *gst_mpeg2dec_get_src_event_masks (GstPad * pad);
static gboolean gst_mpeg2dec_src_event (GstPad * pad, GstEvent * event);
static const GstQueryType *gst_mpeg2dec_get_src_query_types (GstPad * pad);
static gboolean gst_mpeg2dec_src_query (GstPad * pad, GstQueryType type,
    GstFormat * format, gint64 * value);

static const GstFormat *gst_mpeg2dec_get_sink_formats (GstPad * pad);
static gboolean gst_mpeg2dec_convert_sink (GstPad * pad, GstFormat src_format,
    gint64 src_value, GstFormat * dest_format, gint64 * dest_value);
static gboolean gst_mpeg2dec_convert_src (GstPad * pad, GstFormat src_format,
    gint64 src_value, GstFormat * dest_format, gint64 * dest_value);

static GstElementStateReturn gst_mpeg2dec_change_state (GstElement * element);

static void gst_mpeg2dec_chain (GstPad * pad, GstData * _data);

static GstElementClass *parent_class = NULL;

/*static guint gst_mpeg2dec_signals[LAST_SIGNAL] = { 0 };*/

GType
gst_mpeg2dec_get_type (void)
{
  static GType mpeg2dec_type = 0;

  if (!mpeg2dec_type) {
    static const GTypeInfo mpeg2dec_info = {
      sizeof (GstMpeg2decClass),
      gst_mpeg2dec_base_init,
      NULL,
      (GClassInitFunc) gst_mpeg2dec_class_init,
      NULL,
      NULL,
      sizeof (GstMpeg2dec),
      0,
      (GInstanceInitFunc) gst_mpeg2dec_init,
    };

    mpeg2dec_type =
        g_type_register_static (GST_TYPE_ELEMENT, "GstMpeg2dec", &mpeg2dec_info,
        0);
  }

  GST_DEBUG_CATEGORY_INIT (mpeg2dec_debug, "mpeg2dec", 0,
      "MPEG2 decoder element");

  return mpeg2dec_type;
}

static void
gst_mpeg2dec_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class, src_templ ());
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template_factory));
#ifdef enable_user_data
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&user_data_template_factory));
#endif
  gst_element_class_set_details (element_class, &gst_mpeg2dec_details);
}

static void
gst_mpeg2dec_class_init (GstMpeg2decClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

  gobject_class->set_property = gst_mpeg2dec_set_property;
  gobject_class->get_property = gst_mpeg2dec_get_property;
  gobject_class->dispose = gst_mpeg2dec_dispose;

  gstelement_class->change_state = gst_mpeg2dec_change_state;
  gstelement_class->set_index = gst_mpeg2dec_set_index;
  gstelement_class->get_index = gst_mpeg2dec_get_index;
}

static void
gst_mpeg2dec_init (GstMpeg2dec * mpeg2dec)
{
  /* create the sink and src pads */
  mpeg2dec->sinkpad =
      gst_pad_new_from_template (gst_static_pad_template_get
      (&sink_template_factory), "sink");
  gst_element_add_pad (GST_ELEMENT (mpeg2dec), mpeg2dec->sinkpad);
  gst_pad_set_chain_function (mpeg2dec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_chain));
  gst_pad_set_formats_function (mpeg2dec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_get_sink_formats));
  gst_pad_set_convert_function (mpeg2dec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_convert_sink));

  mpeg2dec->srcpad = gst_pad_new_from_template (src_templ (), "src");
  gst_element_add_pad (GST_ELEMENT (mpeg2dec), mpeg2dec->srcpad);
  gst_pad_use_explicit_caps (mpeg2dec->srcpad);
  gst_pad_set_formats_function (mpeg2dec->srcpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_get_src_formats));
  gst_pad_set_event_mask_function (mpeg2dec->srcpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_get_src_event_masks));
  gst_pad_set_event_function (mpeg2dec->srcpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_src_event));
  gst_pad_set_query_type_function (mpeg2dec->srcpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_get_src_query_types));
  gst_pad_set_query_function (mpeg2dec->srcpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_src_query));
  gst_pad_set_convert_function (mpeg2dec->srcpad,
      GST_DEBUG_FUNCPTR (gst_mpeg2dec_convert_src));

#ifdef enable_user_data
  mpeg2dec->userdatapad =
      gst_pad_new_from_template (gst_static_pad_template_get
      (&user_data_template_factory), "user_data");
  gst_element_add_pad (GST_ELEMENT (mpeg2dec), mpeg2dec->userdatapad);
#endif

  /* initialize the mpeg2dec acceleration */
  mpeg2_accel (MPEG2_ACCEL_DETECT);
  mpeg2dec->closed = TRUE;
  mpeg2dec->have_fbuf = FALSE;

  GST_FLAG_SET (GST_ELEMENT (mpeg2dec), GST_ELEMENT_EVENT_AWARE);
}

static void
gst_mpeg2dec_close_decoder (GstMpeg2dec * mpeg2dec)
{
  if (!mpeg2dec->closed) {
    mpeg2_close (mpeg2dec->decoder);
    mpeg2dec->closed = TRUE;
    mpeg2dec->decoder = NULL;
  }
}

static void
gst_mpeg2dec_open_decoder (GstMpeg2dec * mpeg2dec)
{
  gst_mpeg2dec_close_decoder (mpeg2dec);
  mpeg2dec->decoder = mpeg2_init ();
  mpeg2dec->closed = FALSE;
  mpeg2dec->have_fbuf = FALSE;
}

static void
gst_mpeg2dec_dispose (GObject * object)
{
  GstMpeg2dec *mpeg2dec = GST_MPEG2DEC (object);

  gst_mpeg2dec_close_decoder (mpeg2dec);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_mpeg2dec_set_index (GstElement * element, GstIndex * index)
{
  GstMpeg2dec *mpeg2dec = GST_MPEG2DEC (element);

  mpeg2dec->index = index;

  gst_index_get_writer_id (index, GST_OBJECT (element), &mpeg2dec->index_id);
}

static GstIndex *
gst_mpeg2dec_get_index (GstElement * element)
{
  GstMpeg2dec *mpeg2dec = GST_MPEG2DEC (element);

  return mpeg2dec->index;
}

static GstBuffer *
gst_mpeg2dec_alloc_buffer (GstMpeg2dec * mpeg2dec, const mpeg2_info_t * info,
    gint64 offset)
{
  GstBuffer *outbuf = NULL;
  gint size = mpeg2dec->width * mpeg2dec->height;
  guint8 *buf[3], *out;
  const mpeg2_picture_t *picture;

  if (mpeg2dec->format == MPEG2DEC_FORMAT_I422) {
    outbuf =
        gst_pad_alloc_buffer (mpeg2dec->srcpad, GST_BUFFER_OFFSET_NONE,
        size * 2);

    out = GST_BUFFER_DATA (outbuf);

    buf[0] = out;
    buf[1] = buf[0] + size;
    buf[2] = buf[1] + size / 2;

  } else {
    outbuf =
        gst_pad_alloc_buffer (mpeg2dec->srcpad, GST_BUFFER_OFFSET_NONE,
        (size * 3) / 2);

    out = GST_BUFFER_DATA (outbuf);

    buf[0] = out;
    if (mpeg2dec->format == MPEG2DEC_FORMAT_I420) {
      buf[0] = out;
      buf[1] = buf[0] + size;
      buf[2] = buf[1] + size / 4;
    } else {
      buf[0] = out;
      buf[2] = buf[0] + size;
      buf[1] = buf[2] + size / 4;
    }
  }

  gst_buffer_ref (outbuf);

  mpeg2_custom_fbuf (mpeg2dec->decoder, 1);
  mpeg2_set_buf (mpeg2dec->decoder, buf, outbuf);

  picture = info->current_picture;

  if (picture
      && (picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_KEY_UNIT);
  } else {
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_KEY_UNIT);
  }
  /* we store the original byteoffset of this picture in the stream here
   * because we need it for indexing */
  GST_BUFFER_OFFSET (outbuf) = offset;

  return outbuf;
}

static gboolean
gst_mpeg2dec_negotiate_format (GstMpeg2dec * mpeg2dec)
{
  GstCaps *caps;
  guint32 fourcc, myFourcc;
  gboolean ret;
  const mpeg2_info_t *info;
  const mpeg2_sequence_t *sequence;

  if (!GST_PAD_IS_LINKED (mpeg2dec->srcpad)) {
    mpeg2dec->format = MPEG2DEC_FORMAT_I420;
    return TRUE;
  }

  info = mpeg2_info (mpeg2dec->decoder);
  sequence = info->sequence;

  if (sequence->width != sequence->chroma_width &&
      sequence->height != sequence->chroma_height)
    myFourcc = GST_STR_FOURCC ("I420");
  else if (sequence->width == sequence->chroma_width ||
      sequence->height == sequence->chroma_height)
    myFourcc = GST_STR_FOURCC ("Y42B");
  else {
    g_warning ("mpeg2dec: 4:4:4 format not yet supported");
    return (FALSE);
  }

  caps = gst_caps_new_simple ("video/x-raw-yuv",
      "format", GST_TYPE_FOURCC, myFourcc,
      "width", G_TYPE_INT, mpeg2dec->width,
      "height", G_TYPE_INT, mpeg2dec->height,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, mpeg2dec->pixel_width,
      mpeg2dec->pixel_height,
      "framerate", G_TYPE_DOUBLE, mpeg2dec->frame_rate, NULL);

  ret = gst_pad_set_explicit_caps (mpeg2dec->srcpad, caps);
  if (!ret)
    return FALSE;

  /* it worked, try to find what it was again */
  gst_structure_get_fourcc (gst_caps_get_structure (caps, 0),
      "format", &fourcc);

  if (fourcc == GST_STR_FOURCC ("Y42B")) {
    mpeg2dec->format = MPEG2DEC_FORMAT_I422;
  } else if (fourcc == GST_STR_FOURCC ("I420")) {
    mpeg2dec->format = MPEG2DEC_FORMAT_I420;
  } else {
    mpeg2dec->format = MPEG2DEC_FORMAT_YV12;
  }

  return TRUE;
}

#if 0
static void
update_streaminfo (GstMpeg2dec * mpeg2dec)
{
  GstCaps *caps;
  GstProps *props;
  GstPropsEntry *entry;
  const mpeg2_info_t *info;

  info = mpeg2_info (mpeg2dec->decoder);

  props = gst_props_empty_new ();

  entry =
      gst_props_entry_new ("framerate",
      G_TYPE_DOUBLE (GST_SECOND / (float) mpeg2dec->frame_period));
  gst_props_add_entry (props, entry);
  entry =
      gst_props_entry_new ("bitrate",
      G_TYPE_INT (info->sequence->byte_rate * 8));
  gst_props_add_entry (props, entry);

  caps = gst_caps_new ("mpeg2dec_streaminfo",
      "application/x-gst-streaminfo", props);

  gst_caps_replace_sink (&mpeg2dec->streaminfo, caps);
  g_object_notify (G_OBJECT (mpeg2dec), "streaminfo");
}
#endif

static void
gst_mpeg2dec_flush_decoder (GstMpeg2dec * mpeg2dec)
{
  if (mpeg2dec->decoder) {
#if MPEG2_RELEASE < MPEG2_VERSION(0,4,0)
    const mpeg2_info_t *info = mpeg2_info (mpeg2dec->decoder);
    mpeg2_state_t state;

    do {
      state = mpeg2_parse (mpeg2dec->decoder);
      if (state == STATE_END) {
        if (info->discard_fbuf && info->discard_fbuf->id) {
          gst_buffer_unref ((GstBuffer *) info->discard_fbuf->id);
        }
      }
    }
    while (state != STATE_BUFFER && state != -1);
#else
    GST_DEBUG_OBJECT (mpeg2dec, "resetting mpeg2 stream decoder");
    /* 0 starts at next picture, 1 at next sequence header */
    mpeg2_reset (mpeg2dec->decoder, 0);
#endif
  }
}

static void
gst_mpeg2dec_chain (GstPad * pad, GstData * _data)
{
  GstBuffer *buf = GST_BUFFER (_data);
  GstMpeg2dec *mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));
  guint32 size;
  guint8 *data, *end;
  gint64 pts;
  const mpeg2_info_t *info;
  mpeg2_state_t state;
  gboolean done = FALSE;

  if (GST_IS_EVENT (buf)) {
    GstEvent *event = GST_EVENT (buf);

    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_DISCONTINUOUS:
      {
        GST_DEBUG_OBJECT (mpeg2dec, "discont, resetting next_time to 0");
        mpeg2dec->next_time = 0;
        mpeg2dec->discont_state = MPEG2DEC_DISC_NEW_PICTURE;
        gst_mpeg2dec_flush_decoder (mpeg2dec);
        gst_pad_event_default (pad, event);
        return;
      }
      case GST_EVENT_EOS:
        if (mpeg2dec->index && mpeg2dec->closed) {
          gst_index_commit (mpeg2dec->index, mpeg2dec->index_id);
        }
      default:
        GST_DEBUG_OBJECT (mpeg2dec, "Got event of type %d on sink pad",
            GST_EVENT_TYPE (event));
        gst_pad_event_default (pad, event);
        return;
    }
  }

  size = GST_BUFFER_SIZE (buf);
  data = GST_BUFFER_DATA (buf);
  pts = GST_BUFFER_TIMESTAMP (buf);
  GST_LOG_OBJECT (mpeg2dec, "received buffer, timestamp %"
      GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)));

  info = mpeg2_info (mpeg2dec->decoder);
  end = data + size;

  if (pts != -1) {
    gint64 mpeg_pts = GST_TIME_TO_MPEG_TIME (pts);

    GST_DEBUG_OBJECT (mpeg2dec,
        "have pts: %" G_GINT64_FORMAT " (%" GST_TIME_FORMAT ")", mpeg_pts,
        GST_TIME_ARGS (MPEG_TIME_TO_GST_TIME (mpeg_pts)));

#if MPEG2_RELEASE >= MPEG2_VERSION(0,4,0)
    mpeg2_tag_picture (mpeg2dec->decoder, mpeg_pts & 0xffffffff,
        mpeg_pts >> 32);
#else
    mpeg2_pts (mpeg2dec->decoder, mpeg_pts);
#endif
  } else {
    GST_LOG ("no pts");
  }

  GST_LOG ("calling mpeg2_buffer");
  mpeg2_buffer (mpeg2dec->decoder, data, end);
  GST_LOG ("calling mpeg2_buffer done");

  while (!done) {
    gboolean slice = FALSE;

    GST_LOG ("calling parse");
    state = mpeg2_parse (mpeg2dec->decoder);
    GST_DEBUG_OBJECT (mpeg2dec, "parse state %d", state);
    switch (state) {
      case STATE_SEQUENCE:
      {
        gint i;

        mpeg2dec->width = info->sequence->width;
        mpeg2dec->height = info->sequence->height;
        mpeg2dec->pixel_width = info->sequence->pixel_width;
        mpeg2dec->pixel_height = info->sequence->pixel_height;
        mpeg2dec->total_frames = 0;

        /* find framerate */
        for (i = 0; i < 9; i++) {
          if (info->sequence->frame_period == frame_periods[i]) {
            mpeg2dec->frame_rate = fpss[i];
          }
        }
        mpeg2dec->frame_period =
            info->sequence->frame_period * GST_USECOND / 27;

        GST_DEBUG_OBJECT (mpeg2dec,
            "sequence flags: %d, frame period: %d, frame rate: %d",
            info->sequence->flags, info->sequence->frame_period,
            mpeg2dec->frame_rate);
        GST_DEBUG_OBJECT (mpeg2dec, "profile: %02x, colour_primaries: %d",
            info->sequence->profile_level_id, info->sequence->colour_primaries);
        GST_DEBUG_OBJECT (mpeg2dec, "transfer chars: %d, matrix coef: %d",
            info->sequence->transfer_characteristics,
            info->sequence->matrix_coefficients);

        if (!gst_mpeg2dec_negotiate_format (mpeg2dec)) {
          GST_ELEMENT_ERROR (mpeg2dec, CORE, NEGOTIATION, (NULL), (NULL));
          goto exit;
        }

        if (!mpeg2dec->have_fbuf) {
          /* alloc 3 buffers */
          gst_mpeg2dec_alloc_buffer (mpeg2dec, info, GST_BUFFER_OFFSET (buf));
          gst_mpeg2dec_alloc_buffer (mpeg2dec, info, GST_BUFFER_OFFSET (buf));
          gst_mpeg2dec_alloc_buffer (mpeg2dec, info, GST_BUFFER_OFFSET (buf));
          mpeg2dec->have_fbuf = TRUE;
        }

        mpeg2dec->need_sequence = FALSE;
        if (mpeg2dec->pending_event) {
          done =
              GST_EVENT_SEEK_FLAGS (mpeg2dec->
              pending_event) & GST_SEEK_FLAG_FLUSH;

          gst_mpeg2dec_src_event (mpeg2dec->srcpad, mpeg2dec->pending_event);
          mpeg2dec->pending_event = NULL;
        }
        break;
      }
      case STATE_SEQUENCE_REPEATED:
        GST_DEBUG_OBJECT (mpeg2dec, "sequence repeated");
      case STATE_GOP:
        break;
      case STATE_PICTURE:
      {
        gboolean key_frame = FALSE;
        GstBuffer *outbuf;

        if (info->current_picture) {
          key_frame =
              (info->current_picture->flags & PIC_MASK_CODING_TYPE) ==
              PIC_FLAG_CODING_TYPE_I;
        }
        outbuf =
            gst_mpeg2dec_alloc_buffer (mpeg2dec, info, GST_BUFFER_OFFSET (buf));

        GST_DEBUG_OBJECT (mpeg2dec, "picture %s, outbuf %p, offset %"
            G_GINT64_FORMAT ", pts %" G_GINT64_FORMAT,
            key_frame ? ", kf," : "    ", outbuf, GST_BUFFER_OFFSET (outbuf),
            pts);

        if (mpeg2dec->discont_state == MPEG2DEC_DISC_NEW_PICTURE && key_frame)
          mpeg2dec->discont_state = MPEG2DEC_DISC_NEW_KEYFRAME;

        if (!GST_PAD_IS_USABLE (mpeg2dec->srcpad))
          mpeg2_skip (mpeg2dec->decoder, 1);
        else
          mpeg2_skip (mpeg2dec->decoder, 0);
        break;
      }
      case STATE_SLICE_1ST:
        GST_DEBUG_OBJECT (mpeg2dec, "slice 1st");
        break;
      case STATE_PICTURE_2ND:
        GST_DEBUG_OBJECT (mpeg2dec, "picture second");
        break;
      case STATE_SLICE:
        slice = TRUE;
#if MPEG2_RELEASE >= MPEG2_VERSION (0, 4, 0)
      case STATE_INVALID_END:
#endif
      case STATE_END:
      {
        GstBuffer *outbuf = NULL;
        gboolean skip = FALSE;

        if (!slice) {
          mpeg2dec->need_sequence = TRUE;
        }
        GST_DEBUG_OBJECT (mpeg2dec, "picture slice/end %p %p %p %p",
            info->display_fbuf,
            info->display_picture, info->current_picture,
            (info->display_fbuf ? info->display_fbuf->id : NULL));

        if (info->display_fbuf && info->display_fbuf->id) {
          const mpeg2_picture_t *picture;
          gboolean key_frame = FALSE;

          outbuf = (GstBuffer *) info->display_fbuf->id;
          picture = info->display_picture;

          key_frame =
              (picture->flags & PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I;
          GST_DEBUG_OBJECT (mpeg2dec, "picture keyframe %d", key_frame);

          if (key_frame) {
            GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_KEY_UNIT);
          }
          if (mpeg2dec->discont_state == MPEG2DEC_DISC_NEW_KEYFRAME
              && key_frame)
            mpeg2dec->discont_state = MPEG2DEC_DISC_NONE;

#if MPEG2_RELEASE < MPEG2_VERSION(0,4,0)
          if (picture->flags & PIC_FLAG_PTS) {
            GstClockTime time = MPEG_TIME_TO_GST_TIME (picture->pts);
#else
          if (picture->flags & PIC_FLAG_TAGS) {
            GstClockTime time =
                MPEG_TIME_TO_GST_TIME ((guint64) picture->tag2 << 32 | picture->
                tag);
#endif

            GST_DEBUG_OBJECT (mpeg2dec, "picture had pts %" GST_TIME_FORMAT,
                GST_TIME_ARGS (time));
            GST_BUFFER_TIMESTAMP (outbuf) = mpeg2dec->next_time = time;
          } else {
            GST_DEBUG_OBJECT (mpeg2dec,
                "picture didn't have pts using %" GST_TIME_FORMAT,
                GST_TIME_ARGS (mpeg2dec->next_time));
            GST_BUFFER_TIMESTAMP (outbuf) = mpeg2dec->next_time;
          }

          mpeg2dec->next_time +=
              (mpeg2dec->frame_period * picture->nb_fields) >> 1;

          GST_DEBUG_OBJECT (mpeg2dec,
              "picture: %s %s fields:%d off:%" G_GINT64_FORMAT " ts:%"
              GST_TIME_FORMAT,
              (picture->flags & PIC_FLAG_TOP_FIELD_FIRST ? "tff " : "    "),
              (picture->flags & PIC_FLAG_PROGRESSIVE_FRAME ? "prog" : "    "),
              picture->nb_fields, GST_BUFFER_OFFSET (outbuf),
              GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)));

          if (mpeg2dec->index) {
            gst_index_add_association (mpeg2dec->index, mpeg2dec->index_id,
                (key_frame ? GST_ASSOCIATION_FLAG_KEY_UNIT : 0),
                GST_FORMAT_BYTES, GST_BUFFER_OFFSET (outbuf),
                GST_FORMAT_TIME, GST_BUFFER_TIMESTAMP (outbuf), 0);
          }

          if (picture->flags & PIC_FLAG_SKIP) {
            GST_DEBUG_OBJECT (mpeg2dec, "dropping buffer because of skip flag");
            gst_buffer_unref (outbuf);
          } else if (!GST_PAD_IS_USABLE (mpeg2dec->srcpad)) {
            GST_DEBUG_OBJECT (mpeg2dec, "dropping buffer, pad not usable");
            gst_buffer_unref (outbuf);
          } else if (mpeg2dec->discont_state != MPEG2DEC_DISC_NONE) {
            GST_DEBUG_OBJECT (mpeg2dec, "dropping buffer, discont state %d",
                mpeg2dec->discont_state);
            gst_buffer_unref (outbuf);
          } else if (mpeg2dec->next_time < mpeg2dec->segment_start) {
            GST_DEBUG_OBJECT (mpeg2dec, "dropping buffer, next_time %"
                GST_TIME_FORMAT " <  segment_start %" GST_TIME_FORMAT,
                GST_TIME_ARGS (mpeg2dec->next_time),
                GST_TIME_ARGS (mpeg2dec->segment_start));
            gst_buffer_unref (outbuf);
          } else if (skip) {
            GST_DEBUG_OBJECT (mpeg2dec, "dropping buffer, asked to skip");
            gst_buffer_unref (outbuf);
          } else {
            /* TODO set correct offset here based on frame number */
            GST_BUFFER_DURATION (outbuf) = mpeg2dec->frame_period;
            GST_LOG_OBJECT (mpeg2dec, "pushing buffer, timestamp %"
                GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT,
                GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
                GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)));
            gst_pad_push (mpeg2dec->srcpad, GST_DATA (outbuf));
          }
        } else if (info->display_fbuf && !info->display_fbuf->id) {
          GST_WARNING ("Got a frame from libmpeg2, but it has no buffer");
        }

        if (info->discard_fbuf && info->discard_fbuf->id) {
          gst_buffer_unref ((GstBuffer *) info->discard_fbuf->id);
        }
        break;
      }
        /* need more data */
      case STATE_BUFFER:
      case -1:
        done = TRUE;
        break;
        /* error */
      case STATE_INVALID:
        g_warning ("mpeg2dec: decoding error");
        /* it looks like setting a new frame in libmpeg2 avoids a crash */
        /* FIXME figure out how this screws up sync and buffer leakage */
        gst_mpeg2dec_alloc_buffer (mpeg2dec, info, GST_BUFFER_OFFSET (buf));
        break;
      default:
        g_warning ("%s: unhandled state %d, FIXME",
            gst_element_get_name (GST_ELEMENT (mpeg2dec)), state);
        break;
    }

    /*
     * FIXME: should pass more information such as state the user data is from
     */
#ifdef enable_user_data
    if (info->user_data_len > 0) {
      if (GST_PAD_IS_USABLE (mpeg2dec->userdatapad)) {
        GstBuffer *udbuf = gst_buffer_new_and_alloc (info->user_data_len);

        memcpy (GST_BUFFER_DATA (udbuf), info->user_data, info->user_data_len);

        gst_pad_push (mpeg2dec->userdatapad, GST_DATA (udbuf));
      }
    }
#endif
  }
exit:
  gst_buffer_unref (buf);
}

static const GstFormat *
gst_mpeg2dec_get_sink_formats (GstPad * pad)
{
  static const GstFormat formats[] = {
    GST_FORMAT_BYTES,
    GST_FORMAT_TIME,
    0
  };

  return formats;
}

static gboolean
gst_mpeg2dec_convert_sink (GstPad * pad, GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  GstMpeg2dec *mpeg2dec;
  const mpeg2_info_t *info;

  mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));

  if (mpeg2dec->decoder == NULL)
    return FALSE;

  info = mpeg2_info (mpeg2dec->decoder);

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          if (info->sequence && info->sequence->byte_rate) {
            *dest_value = GST_SECOND * src_value / info->sequence->byte_rate;
            break;
          }
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          if (info->sequence && info->sequence->byte_rate) {
            *dest_value = src_value * info->sequence->byte_rate / GST_SECOND;
            break;
          }
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }
  return res;
}

static const GstFormat *
gst_mpeg2dec_get_src_formats (GstPad * pad)
{
  static const GstFormat formats[] = {
    GST_FORMAT_BYTES,
    GST_FORMAT_TIME,
    GST_FORMAT_DEFAULT,
    0
  };

  return formats;
}

static gboolean
gst_mpeg2dec_convert_src (GstPad * pad, GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  GstMpeg2dec *mpeg2dec;
  const mpeg2_info_t *info;
  guint64 scale = 1;

  mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));

  if (mpeg2dec->decoder == NULL)
    return FALSE;

  info = mpeg2_info (mpeg2dec->decoder);

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          scale = 6 * (mpeg2dec->width * mpeg2dec->height >> 2);
        case GST_FORMAT_DEFAULT:
          if (info->sequence && mpeg2dec->frame_period) {
            *dest_value = src_value * scale / mpeg2dec->frame_period;
            break;
          }
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_DEFAULT:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          *dest_value = src_value * mpeg2dec->frame_period;
          break;
        case GST_FORMAT_BYTES:
          *dest_value =
              src_value * 6 * ((mpeg2dec->width * mpeg2dec->height) >> 2);
          break;
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }
  return res;
}

static const GstQueryType *
gst_mpeg2dec_get_src_query_types (GstPad * pad)
{
  static const GstQueryType types[] = {
    GST_QUERY_TOTAL,
    GST_QUERY_POSITION,
    0
  };

  return types;
}

static gboolean
gst_mpeg2dec_src_query (GstPad * pad, GstQueryType type,
    GstFormat * format, gint64 * value)
{
  gboolean res = TRUE;
  GstMpeg2dec *mpeg2dec;
  static const GstFormat *formats;

  mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));

  switch (type) {
    case GST_QUERY_TOTAL:
    {
      switch (*format) {
        case GST_FORMAT_TIME:
        case GST_FORMAT_BYTES:
        case GST_FORMAT_DEFAULT:
        {
          res = FALSE;

          /* get our peer formats */
          formats = gst_pad_get_formats (GST_PAD_PEER (mpeg2dec->sinkpad));

          /* while we did not exhaust our seek formats without result */
          while (formats && *formats) {
            GstFormat peer_format;
            gint64 peer_value;

            peer_format = *formats;

            /* do the probe */
            if (gst_pad_query (GST_PAD_PEER (mpeg2dec->sinkpad),
                    GST_QUERY_TOTAL, &peer_format, &peer_value)) {
              GstFormat conv_format;

              /* convert to TIME */
              conv_format = GST_FORMAT_TIME;
              res = gst_pad_convert (mpeg2dec->sinkpad,
                  peer_format, peer_value, &conv_format, value);
              /* and to final format */
              res &= gst_pad_convert (pad,
                  GST_FORMAT_TIME, *value, format, value);
            }
            formats++;
          }
          break;
        }
        default:
          res = FALSE;
          break;
      }
      break;
    }
    case GST_QUERY_POSITION:
    {
      switch (*format) {
        default:
          res = gst_pad_convert (pad,
              GST_FORMAT_TIME, mpeg2dec->next_time, format, value);
          break;
      }
      break;
    }
    default:
      res = FALSE;
      break;
  }

  return res;
}


static const GstEventMask *
gst_mpeg2dec_get_src_event_masks (GstPad * pad)
{
  static const GstEventMask masks[] = {
    {GST_EVENT_SEEK, GST_SEEK_METHOD_SET | GST_SEEK_FLAG_FLUSH},
    {GST_EVENT_NAVIGATION, GST_EVENT_FLAG_NONE},
    {0,}
  };

  return masks;
}

static gboolean
index_seek (GstPad * pad, GstEvent * event)
{
  GstIndexEntry *entry;
  GstMpeg2dec *mpeg2dec;

  mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));

  entry = gst_index_get_assoc_entry (mpeg2dec->index, mpeg2dec->index_id,
      GST_INDEX_LOOKUP_BEFORE, GST_ASSOCIATION_FLAG_KEY_UNIT,
      GST_EVENT_SEEK_FORMAT (event), GST_EVENT_SEEK_OFFSET (event));

  if (entry) {
    const GstFormat *peer_formats, *try_formats;

    /* since we know the exact byteoffset of the frame, make sure to seek on bytes first */
    const GstFormat try_all_formats[] = {
      GST_FORMAT_BYTES,
      GST_FORMAT_TIME,
      0
    };

    try_formats = try_all_formats;
    peer_formats = gst_pad_get_formats (GST_PAD_PEER (mpeg2dec->sinkpad));

    while (gst_formats_contains (peer_formats, *try_formats)) {
      gint64 value;

      if (gst_index_entry_assoc_map (entry, *try_formats, &value)) {
        GstEvent *seek_event;

        GST_CAT_DEBUG (GST_CAT_SEEK, "index %s %" G_GINT64_FORMAT
            " -> %s %" G_GINT64_FORMAT,
            gst_format_get_details (GST_EVENT_SEEK_FORMAT (event))->nick,
            GST_EVENT_SEEK_OFFSET (event),
            gst_format_get_details (*try_formats)->nick, value);

        /* lookup succeeded, create the seek */
        seek_event =
            gst_event_new_seek (*try_formats | GST_SEEK_METHOD_SET |
            GST_SEEK_FLAG_FLUSH, value);
        /* do the seek */
        if (gst_pad_send_event (GST_PAD_PEER (mpeg2dec->sinkpad), seek_event)) {
          /* seek worked, we're done, loop will exit */
          gst_mpeg2dec_flush_decoder (mpeg2dec);
          mpeg2dec->segment_start = GST_EVENT_SEEK_OFFSET (event);
          return TRUE;
        }
      }
      try_formats++;
    }
  }
  return FALSE;
}


static gboolean
normal_seek (GstPad * pad, GstEvent * event)
{
  gint64 src_offset;
  gboolean flush;
  GstFormat format;
  const GstFormat *peer_formats;
  gboolean res = FALSE;
  GstMpeg2dec *mpeg2dec;

  mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));

  format = GST_FORMAT_TIME;

  /* first bring the src_format to TIME */
  if (!gst_pad_convert (pad,
          GST_EVENT_SEEK_FORMAT (event), GST_EVENT_SEEK_OFFSET (event),
          &format, &src_offset)) {
    /* didn't work, probably unsupported seek format then */
    return res;
  }

  /* shave off the flush flag, we'll need it later */
  flush = GST_EVENT_SEEK_FLAGS (event) & GST_SEEK_FLAG_FLUSH;

  /* get our peer formats */
  peer_formats = gst_pad_get_formats (GST_PAD_PEER (mpeg2dec->sinkpad));

  /* while we did not exhaust our seek formats without result */
  while (peer_formats && *peer_formats) {
    gint64 desired_offset;

    format = *peer_formats;

    /* try to convert requested format to one we can seek with on the sinkpad */
    if (gst_pad_convert (mpeg2dec->sinkpad, GST_FORMAT_TIME, src_offset,
            &format, &desired_offset)) {
      GstEvent *seek_event;

      /* conversion succeeded, create the seek */
      seek_event =
          gst_event_new_seek (format | GST_SEEK_METHOD_SET | flush,
          desired_offset);
      /* do the seekk */
      if (gst_pad_send_event (GST_PAD_PEER (mpeg2dec->sinkpad), seek_event)) {
        /* seek worked, we're done, loop will exit */
        mpeg2dec->segment_start = GST_EVENT_SEEK_OFFSET (event);
        res = TRUE;
        break;
      }
    }
    peer_formats++;
  }
  /* at this point, either the seek worked and res = TRUE or res == FALSE and the seek
   * failed */
  if (res && flush) {
    /* if we need to flush, iterate until the buffer is empty */
    gst_mpeg2dec_flush_decoder (mpeg2dec);
  }

  return res;
}

static gboolean
gst_mpeg2dec_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res = TRUE;
  GstMpeg2dec *mpeg2dec;

  mpeg2dec = GST_MPEG2DEC (gst_pad_get_parent (pad));

  if (mpeg2dec->decoder == NULL)
    return FALSE;

  switch (GST_EVENT_TYPE (event)) {
      /* the all-formats seek logic */
    case GST_EVENT_SEEK:
      if (mpeg2dec->need_sequence) {
        mpeg2dec->pending_event = event;
        return TRUE;
      } else {
        if (mpeg2dec->index)
          res = index_seek (pad, event);
        else
          res = normal_seek (pad, event);

        if (res)
          mpeg2dec->discont_state = MPEG2DEC_DISC_NEW_PICTURE;
      }
      break;
    case GST_EVENT_NAVIGATION:
      /* Forward a navigation event unchanged */
      return gst_pad_send_event (GST_PAD_PEER (mpeg2dec->sinkpad), event);
    default:
      res = FALSE;
      break;
  }
  gst_event_unref (event);
  return res;
}

static GstElementStateReturn
gst_mpeg2dec_change_state (GstElement * element)
{
  GstMpeg2dec *mpeg2dec = GST_MPEG2DEC (element);

  switch (GST_STATE_TRANSITION (element)) {
    case GST_STATE_NULL_TO_READY:
      break;
    case GST_STATE_READY_TO_PAUSED:
    {
      mpeg2dec->next_time = 0;

      /* reset the initial video state */
      mpeg2dec->format = MPEG2DEC_FORMAT_NONE;
      mpeg2dec->width = -1;
      mpeg2dec->height = -1;
      mpeg2dec->segment_start = 0;
      mpeg2dec->segment_end = -1;
      mpeg2dec->discont_state = MPEG2DEC_DISC_NEW_PICTURE;
      mpeg2dec->frame_period = 0;
      gst_mpeg2dec_open_decoder (mpeg2dec);
      mpeg2dec->need_sequence = TRUE;
      break;
    }
    case GST_STATE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_PAUSED_TO_READY:
      gst_mpeg2dec_close_decoder (mpeg2dec);
      break;
    case GST_STATE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return GST_ELEMENT_CLASS (parent_class)->change_state (element);
}

static void
gst_mpeg2dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMpeg2dec *src;

  /* it's not null if we got it, but it might not be ours */
  g_return_if_fail (GST_IS_MPEG2DEC (object));
  src = GST_MPEG2DEC (object);

  switch (prop_id) {
    default:
      break;
  }
}

static void
gst_mpeg2dec_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstMpeg2dec *mpeg2dec;

  /* it's not null if we got it, but it might not be ours */
  g_return_if_fail (GST_IS_MPEG2DEC (object));
  mpeg2dec = GST_MPEG2DEC (object);

  switch (prop_id) {
    default:
      break;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "mpeg2dec", GST_RANK_PRIMARY,
          GST_TYPE_MPEG2DEC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "mpeg2dec",
    "LibMpeg2 decoder", plugin_init, VERSION, "GPL", GST_PACKAGE, GST_ORIGIN)
