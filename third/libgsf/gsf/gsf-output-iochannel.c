/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gsf-output-iochannel.c
 *
 * Copyright (C) 2002-2003 Dom Lachowicz (cinamod@hotmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
#include <gsf-config.h>
#include <gsf/gsf-output-iochannel.h>
#include <gsf/gsf-output-impl.h>
#include <gsf/gsf-impl-utils.h>

struct _GsfOutputIOChannel {
	GsfOutput output;
	GIOChannel * channel;
};

typedef struct {
	GsfOutputClass output_class;
} GsfOutputIOChannelClass;

/**
 * gsf_output_iochannel_new :
 *
 * Returns a new file or NULL.
 **/
GsfOutputIOChannel *
gsf_output_iochannel_new  (GIOChannel * channel)
{
	GsfOutputIOChannel *output = NULL;

	g_return_val_if_fail (channel != NULL, NULL);

	output = g_object_new (GSF_OUTPUT_IOCHANNEL_TYPE, NULL);	
	output->channel = channel;
	return output;
}

static gboolean
gsf_output_iochannel_close (GsfOutput *output)
{
	GsfOutputClass *parent_class;
	GsfOutputIOChannel *io = GSF_OUTPUT_IOCHANNEL (output);	

	g_io_channel_shutdown (io->channel, TRUE, NULL);

	parent_class = g_type_class_peek (GSF_OUTPUT_TYPE);
	if (parent_class && parent_class->Close)
		parent_class->Close (output);
	
	return TRUE;
}

static void
gsf_output_iochannel_finalize (GObject *obj)
{
	GObjectClass *parent_class;
	GsfOutput *output = (GsfOutput *)obj;
	GsfOutputIOChannel *io = GSF_OUTPUT_IOCHANNEL (output);
	
	g_io_channel_unref (io->channel);

	parent_class = g_type_class_peek (GSF_OUTPUT_TYPE);
	if (parent_class && parent_class->finalize)
		parent_class->finalize (obj);
}

static gboolean
gsf_output_iochannel_seek (GsfOutput *output, gsf_off_t offset,
			   GSeekType whence)
{
	GsfOutputIOChannel *io = GSF_OUTPUT_IOCHANNEL (output);
	GIOStatus status = G_IO_STATUS_NORMAL;

	status = g_io_channel_seek_position (io->channel, offset, whence, NULL);
	if (status == G_IO_STATUS_NORMAL)
		return TRUE;

	gsf_output_set_error (output, status, " ");
	return FALSE;
}

static gboolean
gsf_output_iochannel_write (GsfOutput *output,
			    size_t num_bytes,
			    guint8 const *buffer)
{
	GsfOutputIOChannel *io = GSF_OUTPUT_IOCHANNEL (output);
	GIOStatus status = G_IO_STATUS_NORMAL;
	size_t bytes_written = 0, total_written = 0;

	g_return_val_if_fail (io != NULL, FALSE);

	while ((status == G_IO_STATUS_NORMAL) && (total_written < num_bytes)) {
		status = g_io_channel_write_chars (io->channel, (const gchar *)(buffer + total_written),
						   num_bytes - total_written, &bytes_written, NULL);
		total_written += bytes_written;
	}

	return (status == G_IO_STATUS_NORMAL && total_written == num_bytes);
}

#define GET_OUTPUT_CLASS(instance) \
         G_TYPE_INSTANCE_GET_CLASS (instance, GSF_OUTPUT_TYPE, GsfOutputClass)

static void
gsf_output_iochannel_init (GObject *obj)
{
	GsfOutputIOChannel *io = GSF_OUTPUT_IOCHANNEL (obj);

	io->channel   = NULL;
}

static void
gsf_output_iochannel_class_init (GObjectClass *gobject_class)
{
	GsfOutputClass *output_class = GSF_OUTPUT_CLASS (gobject_class);
	
	gobject_class->finalize = gsf_output_iochannel_finalize;
	output_class->Close     = gsf_output_iochannel_close;
	output_class->Seek      = gsf_output_iochannel_seek;
	output_class->Write     = gsf_output_iochannel_write;
}

GSF_CLASS (GsfOutputIOChannel, gsf_output_iochannel,
           gsf_output_iochannel_class_init, gsf_output_iochannel_init, GSF_OUTPUT_TYPE)