/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2002-2004 Bastien Nocera <hadess@hadess.net>
 *
 * cd-recorder.h
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Bastien Nocera <hadess@hadess.net>
 */

#ifndef CD_RECORDER_H
#define CD_RECORDER_H

#include <glib-object.h>
#include "cd-drive.h"

G_BEGIN_DECLS

typedef void (*CancelFunc) (gpointer       data);

enum {
	RESULT_ERROR,
	RESULT_CANCEL,
	RESULT_FINISHED,
	RESULT_RETRY
};

typedef enum {
  TRACK_TYPE_AUDIO,
  TRACK_TYPE_DATA
} TrackType;

typedef struct Track Track;

struct Track {
  TrackType type;
  union {
    struct {
      char *filename;
      char *cdtext;
    } audio;
    struct {
      char *filename;
    } data;
  } contents;
};

typedef enum {
	CDRECORDER_EJECT			= 1 << 0,
	CDRECORDER_BLANK			= 1 << 1,
	CDRECORDER_DUMMY_WRITE			= 1 << 2,
	CDRECORDER_DISC_AT_ONCE			= 1 << 3,
	CDRECORDER_DEBUG			= 1 << 4,
	CDRECORDER_OVERBURN			= 1 << 5,
	CDRECORDER_BURNPROOF			= 1 << 6
} CDRecorderWriteFlags;

#define CD_TYPE_RECORDER            (cd_recorder_get_type ())
#define CD_RECORDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CD_TYPE_RECORDER, CDRecorder))
#define CD_RECORDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CD_TYPE_RECORDER, CDRecorderClass))
#define CD_IS_RECORDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CD_TYPE_RECORDER))
#define CD_IS_RECORDER_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), CD_TYPE_RECORDER))

typedef struct CDRecorder                CDRecorder;
typedef struct CDRecorderClass           CDRecorderClass;
typedef struct CDRecorderPrivate         CDRecorderPrivate;

typedef enum {
	PREPARING_WRITE,
	WRITING,
	FIXATING,
	BLANKING
} CDRecorderActions;

typedef enum {
	MEDIA_CD,
	MEDIA_DVD,
} CDRecorderMedia;

struct CDRecorder {
	GObject parent;
	CDRecorderPrivate *priv;
};

struct CDRecorderClass {
	GObjectClass parent_class;

	void (*progress_changed)	(CDRecorder *cdrecorder,
					 gdouble fraction);
	void (*action_changed)		(CDRecorder *cdrecorder,
					 CDRecorderActions action,
					 CDRecorderMedia media);
	void (*animation_changed)	(CDRecorder *cdrecorder,
					 gboolean spinning);
	gboolean (*insert_cd_request)	(CDRecorder *cdrecorder,
					 gboolean is_reload,
					 gboolean can_rewrite,
					 gboolean busy_cd);
};

GType		cd_recorder_get_type		(void);
CDRecorder     *cd_recorder_new			(void);

int		cd_recorder_write_tracks	(CDRecorder *cdrecorder,
						 CDDrive *drive,
						 GList *tracks,
						 gint speed,
						 CDRecorderWriteFlags flags);

gboolean	cd_recorder_cancel		(CDRecorder *cdrecorder,
						 gboolean skip_if_dangerous);

const char*	cd_recorder_get_error_message	(CDRecorder *cdrecorder);
const char*	cd_recorder_get_error_message_details
						(CDRecorder *cdrecorder);
void		cd_recorder_track_free		(Track *track);

G_END_DECLS

#endif /* CD_RECORDER_H */
