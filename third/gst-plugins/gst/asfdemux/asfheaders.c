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

#include <gst/gst.h>

#include "asfheaders.h"

ASFGuidHash asf_correction_guids[] = {
  {ASF_CORRECTION_ON, {0xBFC3CD50, 0x11CF618F, 0xAA00B28B, 0x20E2B400}
      }
  ,
/*  { ASF_CORRECTION_OFF,    { 0x20FB5700, 0x11CF5B55, 0x8000FDA8, 0x2B445C5F }},*/
  {ASF_CORRECTION_OFF, {0x49F1A440, 0x11D04ECE, 0xA000ACA3, 0xF64803C9}
      }
  ,
  {ASF_CORRECTION_UNDEFINED, {0, 0, 0, 0}
      }
  ,
};

ASFGuidHash asf_stream_guids[] = {
  {ASF_STREAM_VIDEO, {0xBC19EFC0, 0x11CF5B4D, 0x8000FDA8, 0x2B445C5F}
      }
  ,
  {ASF_STREAM_AUDIO, {0xF8699E40, 0x11CF5B4D, 0x8000FDA8, 0x2B445C5F}
      }
  ,
  {ASF_STREAM_UNDEFINED, {0, 0, 0, 0}
      }
  ,
};

/*

2 unknown GUIDs found in an extended header object :

(0x26f18b5d/0x47ec4584/0x650e5f9f/0xc952041f) with size=26
(0xd9aade20/0x4f9c7c17/0x558528bc/0xa2e298dd) with size=34

*/

ASFGuidHash asf_object_guids[] = {
  {ASF_OBJ_STREAM, {0xB7DC0791, 0x11CFA9B7, 0xC000E68E, 0x6553200C}
      }
  ,
  {ASF_OBJ_DATA, {0x75b22636, 0x11cf668e, 0xAA00D9a6, 0x6Cce6200}
      }
  ,
  {ASF_OBJ_FILE, {0x8CABDCA1, 0x11CFA947, 0xC000E48E, 0x6553200C}
      }
  ,
  {ASF_OBJ_HEADER, {0x75B22630, 0x11CF668E, 0xAA00D9A6, 0x6CCE6200}
      }
  ,
  {ASF_OBJ_CONCEAL_NONE, {0x20fb5700, 0x11cf5b55, 0x8000FDa8, 0x2B445C5f}
      }
  ,
  {ASF_OBJ_COMMENT, {0x75b22633, 0x11cf668e, 0xAA00D9a6, 0x6Cce6200}
      }
  ,
  {ASF_OBJ_CODEC_COMMENT, {0x86D15240, 0x11D0311D, 0xA000A4A3, 0xF64803C9}
      }
  ,
  {ASF_OBJ_CODEC_COMMENT1, {0x86d15241, 0x11d0311d, 0xA000A4a3, 0xF64803c9}
      }
  ,
  {ASF_OBJ_INDEX, {0x33000890, 0x11cfe5b1, 0xA000F489, 0xCB4903c9}
      }
  ,
  {ASF_OBJ_HEAD1, {0x5fbf03b5, 0x11cfa92e, 0xC000E38e, 0x6553200c}
      }
  ,
  {ASF_OBJ_HEAD2, {0xabd3d211, 0x11cfa9ba, 0xC000E68e, 0x6553200c}
      }
  ,
  {ASF_OBJ_PADDING, {0x1806D474, 0x4509CADF, 0xAB9ABAA4, 0xE8AA96CB}
      }
  ,
  {ASF_OBJ_BITRATE_PROPS, {0x7bf875ce, 0x11d1468d, 0x6000828d, 0xb2a2c997}
      }
  ,
  {ASF_OBJ_EXT_CONTENT_DESC, {0xd2d0a440, 0x11d2e307, 0xa000f097, 0x50a85ec9}
      }
  ,
  {ASF_OBJ_BITRATE_MUTEX, {0xd6e229dc, 0x11d135da, 0xa0003490, 0xbe4903c9}
      }
  ,
  {ASF_OBJ_LANGUAGE_LIST, {0x7c4346a9, 0x4bfcefe0, 0x3e3929b2, 0x855c41de}
      }
  ,
  {ASF_OBJ_METADATA_OBJECT, {0xc5f8cbea, 0x48775baf, 0x8caa6784, 0xca4cfa44}
      }
  ,
  {ASF_OBJ_EXTENDED_STREAM_PROPS, {0x14e6a5cb, 0x4332c672, 0x69a99983,
              0x5a5b0652}
      }
  ,
  {ASF_OBJ_UNDEFINED, {0, 0, 0, 0}
      }
  ,
};
