/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  gnome-print-job.c: A print job interface
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors:
 *    Michael Zucchi <notzed@ximian.com>
 *    Lauris Kaplinski <lauris@ximian.com>
 *
 *  Copyright (C) 2000-2003 Ximian Inc.
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

#include <libart_lgpl/art_affine.h>
#include <libgnomeprint/gpa/gpa-node.h>
#include <libgnomeprint/gnome-print-private.h>
#include <libgnomeprint/gnome-print-meta.h>
#include <libgnomeprint/gnome-print-multipage.h>
#include <libgnomeprint/gnome-print-job-private.h>
#include <libgnomeprint/gnome-print-config-private.h>

typedef struct _JOBPrivate JOBPrivate;

struct _JOBPrivate {
	/* closed flag */
	guint closed : 1;

	/* Layout data */
	gdouble pw, ph;
	gdouble porient[6];
	gdouble lorient[6];
	gdouble lyw, lyh;
	gint num_affines;
	gdouble *affines;

	/* State data */
	gdouble PP2PA[6];
	gdouble PAW, PAH;
	gdouble LP2LY[6];
	gdouble LYW, LYH;
	gdouble LW, LH;
	gdouble *LY_AFFINES;
	GList *LY_LIST;
};

#define GNOME_PRINT_JOB_CLOSED(m) (((JOBPrivate *) (m)->priv)->closed)

static void gnome_print_job_class_init (GnomePrintJobClass *klass);
static void gnome_print_job_init (GnomePrintJob *job);
static void gnome_print_job_finalize (GObject *object);

static void job_update_layout_data (GnomePrintJob *job);
static void job_parse_config_data (GnomePrintJob *job);
static void job_clear_config_data (GnomePrintJob *job);
static gboolean job_parse_transform (guchar *str, gdouble *transform);

static GObjectClass *parent_class;

GType
gnome_print_job_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GnomePrintJobClass),
			NULL, NULL,
			(GClassInitFunc) gnome_print_job_class_init,
			NULL, NULL,
			sizeof (GnomePrintJob),
			0,
			(GInstanceInitFunc) gnome_print_job_init
		};
		type = g_type_register_static (G_TYPE_OBJECT, "GnomePrintJob", &info, 0);
	}
	return type;
}

static void
gnome_print_job_class_init (GnomePrintJobClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gnome_print_job_finalize;
}

static void
gnome_print_job_init (GnomePrintJob *job)
{
	job->config = NULL;

	job->meta = gnome_print_meta_new ();

	job->priv = g_new0 (JOBPrivate, 1);
	job_clear_config_data (job);
}

static void
gnome_print_job_finalize (GObject *object)
{
	GnomePrintJob *job;

	job = GNOME_PRINT_JOB(object);

	if (job->config) {
		g_object_unref (G_OBJECT (job->config));
	}

	if (job->meta != NULL) {
		g_object_unref (G_OBJECT (job->meta));
	}

	if (job->priv) {
		job_clear_config_data (job);
		g_free (job->priv);
		job->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gnome_print_job_new:
 * @config: The job options, can be NULL in which case a
 *          default GnomePrintConfig is created
 * 
 * Creates a new GnomePrintJob.
 * 
 * Return value: A new GnomePrintJob, NULL on error
 **/
GnomePrintJob *
gnome_print_job_new (GnomePrintConfig *config)
{
	GnomePrintJob *job;
	
	job = g_object_new (GNOME_TYPE_PRINT_JOB, NULL);

	if (config)
		job->config = gnome_print_config_ref (config);
	else
		job->config = gnome_print_config_default ();

	return job;
}

/**
 * gnome_print_job_get_config:
 * @job: 
 * 
 * Gets a referenced pointer to the configuration of the job
 * 
 * Return Value: a referenced GnomePrintConfig for this job, NULL on error
 **/
GnomePrintConfig *
gnome_print_job_get_config (GnomePrintJob *job)
{
	g_return_val_if_fail (job != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), NULL);

	if (job->config)
		gnome_print_config_ref (job->config);

	return job->config;
}

/**
 * gnome_print_job_get_context:
 * @job: An initialised GnomePrintJob.
 * 
 * Retrieve the GnomePrintContext which applications
 * print to.
 * 
 * Return value: The printing context, NULL on error
 **/
GnomePrintContext *
gnome_print_job_get_context (GnomePrintJob *job)
{
	g_return_val_if_fail (job != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), NULL);

	if (job->meta)
		g_object_ref (G_OBJECT (job->meta));

	return job->meta;
}

/**
 * gnome_print_job_get_pages:
 * @job: An initialised and closed GnomePrintJob.
 * 
 * Find the number of pages stored in a completed printout.
 * This is the number of physical pages, so if the layout
 * can hold 4 pages per page, and 5 logical pages are printed
 * (5 beginpage/endpage convinations) 2 is returned 
 * 
 * Return value: the number of pages, 0 on error
 **/
int
gnome_print_job_get_pages (GnomePrintJob *job)
{
	JOBPrivate *pp;
	gint mp;

	g_return_val_if_fail (job != NULL, 0);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), 0);

	if (!GNOME_PRINT_JOB_CLOSED (job))
		return 0;

	job_update_layout_data (job);

	mp = gnome_print_meta_get_pages (GNOME_PRINT_META (job->meta));

	pp = job->priv;

	if (pp->num_affines <= 1)
		return mp;

	return (mp + pp->num_affines - 1) / pp->num_affines;
}

/**
 * gnome_print_job_get_page_size_from_config:
 * @config: 
 * @width: 
 * @height: 
 * 
 * Deprecated, use gnome_print_config_get_page_size
 * 
 * Return Value: TRUE on success, FALSE on error
 **/
gboolean
gnome_print_job_get_page_size_from_config (GnomePrintConfig *config, gdouble *width, gdouble *height)
{
	return gnome_print_config_get_page_size (config, width, height);
}

/**
 * gnome_print_job_get_page_size:
 * @job: 
 * @width: 
 * @height: 
 * 
 * Get the imaging area size that is available to the application
 * Sizes are given in PS points (GNOME_PRINT_PS_UNIT)
 * 
 * Return Value: TRUE on success, FALSE on error
 **/
gboolean
gnome_print_job_get_page_size (GnomePrintJob *job, gdouble *width, gdouble *height)
{
	JOBPrivate *pp;

	g_return_val_if_fail (job != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), FALSE);
	g_return_val_if_fail (width != NULL, FALSE);
	g_return_val_if_fail (height != NULL, FALSE);

	pp = job->priv;

	job_update_layout_data (job);

	if (pp->LY_LIST) {
		if (width)
			*width = pp->LW;
		if (height)
			*height = pp->LH;
	} else {
		if (width)
			*width = pp->pw;
		if (height)
			*height = pp->ph;
	}

	return TRUE;
}

/**
 * gnome_print_job_close:
 * @job: A GnomePrintJob which has had printing performed
 * 
 * Closes the GnomePrintJob @job, ready for printing or
 * previewing. To be called after the application has finished
 * sending the drawing commands
 * 
 * Return Value: 
 **/
gint
gnome_print_job_close (GnomePrintJob *job)
{
	JOBPrivate *pp;

	g_return_val_if_fail (job != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), GNOME_PRINT_ERROR_UNKNOWN);

	pp = job->priv;

	if (!pp->closed) {
		pp->closed = TRUE;
		return gnome_print_context_close (job->meta);
	}

	g_warning ("gnome_print_job_close can only be called once\n");

	return GNOME_PRINT_ERROR_UNKNOWN;
}

/**
 * gnome_print_job_print:
 * @job: A closed GnomePrintJob.
 * 
 * Print the pages stored in the GnomePrintJob to
 * the phyisical printing device.
 *
 * Return value: GNOME_PRINT_OK on success GNOME_PRINT_ERROR_UNKNOWN otherwise
 **/
gint
gnome_print_job_print (GnomePrintJob *job)
{
	JOBPrivate *pp;
	GnomePrintContext *ctx;
	gint lpages, copies, nstacks, npages, nsheets;
	gboolean collate, is_multipage;
	gint stack;
	const guchar *buf;
	gint blen;
	gint ret;

	g_return_val_if_fail (job != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (job->priv, GNOME_PRINT_ERROR_UNKNOWN);

	pp = job->priv;

	if (!pp->closed) {
		g_warning ("You should call gnome_print_job_close before calling\n"
			   "gnome_print_job_print\n");
		gnome_print_job_close (job);
	}
	
	ctx = gnome_print_context_new (job->config);
	g_return_val_if_fail (ctx != NULL, GNOME_PRINT_ERROR_UNKNOWN);

	/* Get number of pages in metafile */
	lpages = gnome_print_meta_get_pages (GNOME_PRINT_META (job->meta));
	if (lpages < 1)
		return GNOME_PRINT_OK;
	npages = lpages;

	/* Update layout data */
	is_multipage = FALSE;
	job_update_layout_data (job);
	if (pp->LY_LIST) {
		GnomePrintContext *mp;
		/* Find out physical page count */
		npages = (npages + pp->num_affines - 1) / pp->num_affines;
		/* Create multipage context */
		mp = gnome_print_multipage_new (ctx, pp->LY_LIST);
		g_object_unref (G_OBJECT (ctx));
		ctx = mp;
		is_multipage = TRUE;
	}

	collate = FALSE;
	gnome_print_config_get_boolean (job->config, GNOME_PRINT_KEY_COLLATE, &collate);
	copies = 1;
	gnome_print_config_get_int (job->config, GNOME_PRINT_KEY_NUM_COPIES, &copies);

	if (collate) {
		nstacks = copies;
		nsheets = 1;
	} else {
		nstacks = 1;
		nsheets = copies;
	}
	
	buf  = gnome_print_meta_get_buffer (GNOME_PRINT_META (job->meta));
	blen = gnome_print_meta_get_length (GNOME_PRINT_META (job->meta));

	for (stack = 0; stack < nstacks; stack++) {
		gint page;
		for (page = 0; page < npages; page++) {
			gint sheet;
			for (sheet = 0; sheet < nsheets; sheet++) {
				gint start, i;
				/* Render physical page */
				start = page * pp->num_affines;
				for (i = start; (i < (start + pp->num_affines)) && (i < lpages); i++) {
					ret = gnome_print_meta_render_data_page (ctx, buf, blen, i, TRUE);
					g_return_val_if_fail (ret == GNOME_PRINT_OK, ret);
				}
				/* Flush physical page */
				if (is_multipage) {
					gnome_print_multipage_finish_page (GNOME_PRINT_MULTIPAGE (ctx));
				}
			}
		}
	}

	ret = gnome_print_context_close (ctx);
	g_object_unref (G_OBJECT (ctx));

	return ret;
}


/**
 * gnome_print_job_render:
 * @job: 
 * @ctx: 
 * 
 * Renders printout to specified context
 * (with layout, ignoring copies)
 * 
 * Return Value: 
 **/
gint
gnome_print_job_render (GnomePrintJob *job, GnomePrintContext *ctx)
{
	JOBPrivate *pp;
	const guchar *data;
	gint len;
	gint ret;

	g_return_val_if_fail (job != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (ctx != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (GNOME_IS_PRINT_CONTEXT (ctx), GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (job->priv, GNOME_PRINT_ERROR_UNKNOWN);

	pp = job->priv;

	data = gnome_print_meta_get_buffer (GNOME_PRINT_META (job->meta));
	g_return_val_if_fail (data != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	len = gnome_print_meta_get_length (GNOME_PRINT_META (job->meta));
	g_return_val_if_fail (len > 0, GNOME_PRINT_ERROR_UNKNOWN);

	job_update_layout_data (job);
	if (pp->LY_LIST) {
		GnomePrintContext *mp;
		mp = gnome_print_multipage_new (ctx, pp->LY_LIST);
		ret = gnome_print_meta_render_data (mp, data, len);
		gnome_print_multipage_finish_page (GNOME_PRINT_MULTIPAGE (mp));
		g_object_unref (G_OBJECT (mp));
	} else {
		ret = gnome_print_meta_render_data (ctx, data, len);
	}

	return ret;
}

/**
 * gnome_print_job_render_page:
 * @job: 
 * @ctx: 
 * @page: 
 * @pageops: 
 * 
 * Renders the specified page @page
 * 
 * Return Value: 
 **/
gint
gnome_print_job_render_page (GnomePrintJob *job, GnomePrintContext *ctx, gint page, gboolean pageops)
{
	JOBPrivate *pp;
	const guchar *data;
	gint len, pages;
	gint ret;

	g_return_val_if_fail (job != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (GNOME_IS_PRINT_JOB (job), GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (ctx != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (GNOME_IS_PRINT_CONTEXT (ctx), GNOME_PRINT_ERROR_UNKNOWN);
	g_return_val_if_fail (job->priv, GNOME_PRINT_ERROR_UNKNOWN);

	pp = job->priv;

	data = gnome_print_meta_get_buffer (GNOME_PRINT_META (job->meta));
	g_return_val_if_fail (data != NULL, GNOME_PRINT_ERROR_UNKNOWN);
	len = gnome_print_meta_get_length (GNOME_PRINT_META (job->meta));
	g_return_val_if_fail (len > 0, GNOME_PRINT_ERROR_UNKNOWN);
	pages = gnome_print_meta_get_pages (GNOME_PRINT_META (job->meta));

	job_update_layout_data (job);
	if (pp->LY_LIST) {
		GnomePrintContext *meta, *mp;
		gint start, p;

		meta = gnome_print_meta_new ();
		g_return_val_if_fail (meta != NULL, GNOME_PRINT_ERROR_UNKNOWN);
		/* Create wrapper multipage */
		mp = gnome_print_multipage_new (meta, pp->LY_LIST);
		start = page * pp->num_affines;
		/* Render job pages */
		for (p = start; (p < pages) && (p < start + pp->num_affines); p++) {
			gnome_print_meta_render_data_page (mp, data, len, p, TRUE);
		}
		/* Finish multipage */
		gnome_print_context_close (mp);
		g_object_unref (G_OBJECT (mp));
		/* Render page */
		data = gnome_print_meta_get_buffer (GNOME_PRINT_META (meta));
		len = gnome_print_meta_get_length (GNOME_PRINT_META (meta));
		ret = gnome_print_meta_render_data_page (ctx, data, len, 0, pageops);
		/* Release meta */
		g_object_unref (G_OBJECT (meta));
	} else {
		ret = gnome_print_meta_render_data_page (ctx, data, len, page, pageops);
	}

	return ret;
}

/* We need:
 *
 * Layout data
 *
 * - pw, ph
 * - porient
 * - lorient
 * - lyw, lyh
 * - num_affines
 * - affines
 *
 * State data
 *
 * - PP2PA
 * - PAW, PAH
 * - LP2LY
 * - LYW, LYH
 * - LW, LH
 * - LY_AFFINES
 * - LY_LIST
 */

#define EPSILON 1e-9
#ifdef JOB_VERBOSE
#define PRINT_2(s,a,b) g_print ("%s %g %g\n", s, (a), (b))
#define PRINT_DRECT(s,a) g_print ("%s %g %g %g %g\n", (s), (a)->x0, (a)->y0, (a)->x1, (a)->y1)
#define PRINT_AFFINE(s,a) g_print ("%s %g %g %g %g %g %g\n", (s), *(a), *((a) + 1), *((a) + 2), *((a) + 3), *((a) + 4), *((a) + 5))
#else
#define PRINT_2(s,a,b)
#define PRINT_DRECT(s,a)
#define PRINT_AFFINE(s,a)
#endif

static void
job_update_layout_data (GnomePrintJob *job)
{
	JOBPrivate *pp;
	ArtDRect area, r;
	gdouble t;
	gdouble a[6];
	gint i;

	g_return_if_fail (job->priv);

	pp = job->priv;

	job_parse_config_data (job);

	/* Now comes the fun part */

	if (pp->num_affines < 1)
		return;
	if ((fabs (pp->pw) < EPSILON) || (fabs (pp->ph) < EPSILON))
		return;

	/* Initial setup */
	/* Calculate PP2PA */
	/* We allow only rectilinear setups, so we can cheat */
	pp->PP2PA[0] = pp->porient[0];
	pp->PP2PA[1] = pp->porient[1];
	pp->PP2PA[2] = pp->porient[2];
	pp->PP2PA[3] = pp->porient[3];
	t = pp->pw * pp->PP2PA[0] + pp->ph * pp->PP2PA[2];
	pp->PP2PA[4] = (t < 0) ? -t : 0.0;
	t = pp->pw * pp->PP2PA[1] + pp->ph * pp->PP2PA[3];
	pp->PP2PA[5] = (t < 0) ? -t : 0.0;
	PRINT_AFFINE ("PP2PA:", &pp->PP2PA[0]);

	/* PPDP - Physical Page Dimensions in Printer */
	/* A: PhysicalPage X PhysicalOrientation X TRANSLATE -> Physical Page in Printer */
	area.x0 = 0.0;
	area.y0 = 0.0;
	area.x1 = pp->pw;
	area.y1 = pp->ph;
	art_drect_affine_transform (&r, &area, pp->PP2PA);
	pp->PAW = r.x1 - r.x0;
	pp->PAH = r.y1 - r.y0;
	if ((pp->PAW < EPSILON) || (pp->PAH < EPSILON))
		return;

	/* Now we have to find the size of layout page */
	/* Again, knowing that layouts are rectilinear helps us */
	art_affine_invert (a, pp->affines);
	PRINT_AFFINE ("INV LY:", &a[0]);
	pp->LYW = pp->lyw * fabs (pp->pw * a[0] + pp->ph * a[2]);
	pp->LYH = pp->lyh * fabs (pp->pw * a[1] + pp->ph * a[3]);
	PRINT_2 ("LY Dimensions:", pp->LYW, pp->LYH);

	/* Calculate LP2LY */
	/* We allow only rectilinear setups, so we can cheat */
	pp->LP2LY[0] = pp->lorient[0];
	pp->LP2LY[1] = pp->lorient[1];
	pp->LP2LY[2] = pp->lorient[2];
	pp->LP2LY[3] = pp->lorient[3];
	/* Delay */
	pp->LP2LY[4] = 0.0;
	pp->LP2LY[5] = 0.0;
	/* Meanwhile find logical width and height */
	area.x0 = 0.0;
	area.y0 = 0.0;
	area.x1 = pp->LYW;
	area.y1 = pp->LYH;
	art_affine_invert (a, pp->LP2LY);
	art_drect_affine_transform (&r, &area, a);
	pp->LW = r.x1 - r.x0;
	pp->LH = r.y1 - r.y0;
	if ((pp->LW < EPSILON) || (pp->LH < EPSILON))
		return;
	PRINT_2 ("L Dimensions", pp->LW, pp->LH);
	/* Now complete matrix calculation */
	t = pp->LW * pp->LP2LY[0] + pp->LH * pp->LP2LY[2];
	pp->LP2LY[4] = (t < 0) ? -t : 0.0;
	t = pp->LW * pp->LP2LY[1] + pp->LH * pp->LP2LY[3];
	pp->LP2LY[5] = (t < 0) ? -t : 0.0;
	PRINT_AFFINE ("LP2LY:", &pp->LP2LY[0]);

	/* Good, now generate actual layout matrixes */

	pp->LY_AFFINES = g_new (gdouble, 6 * pp->num_affines);

	/* Extra fun */

	for (i = 0; i < pp->num_affines; i++) {
		gdouble ly2p[6];
		gdouble *ly2pa;
		/* Calculate Layout -> Physical Page affine */
		memcpy (ly2p, pp->affines + 6 * i, 6 * sizeof (gdouble));
		ly2p[4] *= pp->pw;
		ly2p[5] *= pp->ph;
		/* PRINT_AFFINE ("Layout -> Physical:", &l2p[0]); */
		art_affine_multiply (pp->LY_AFFINES + 6 * i, pp->LP2LY, ly2p);
		ly2pa = g_new (gdouble, 6);
		art_affine_multiply (ly2pa, pp->LY_AFFINES + 6 * i, pp->PP2PA);
		pp->LY_LIST = g_list_prepend (pp->LY_LIST, ly2pa);
	}

	pp->LY_LIST = g_list_reverse (pp->LY_LIST);
}

static void
job_parse_config_data (GnomePrintJob *job)
{
	JOBPrivate *pp;
	const GnomePrintUnit *unit;
	GPANode *layout;
	gchar *loc;

	g_return_if_fail (job->priv);

	pp = job->priv;

	job_clear_config_data (job);

	g_return_if_fail (job->config);

	/* Now the fun part */

	loc = g_strdup (setlocale (LC_NUMERIC, NULL));
	setlocale (LC_NUMERIC, "C");

	/* Physical size */
	if (gnome_print_config_get_length (job->config, GNOME_PRINT_KEY_PAPER_WIDTH, &pp->pw, &unit)) {
		gnome_print_convert_distance (&pp->pw, unit, GNOME_PRINT_PS_UNIT);
	}
	if (gnome_print_config_get_length (job->config, GNOME_PRINT_KEY_PAPER_HEIGHT, &pp->ph, &unit)) {
		gnome_print_convert_distance (&pp->ph, unit, GNOME_PRINT_PS_UNIT);
	}
	/* Physical orientation */
	gnome_print_config_get_transform (job->config, GNOME_PRINT_KEY_PAPER_ORIENTATION_MATRIX, pp->porient);
	/* Logical orientation */
	gnome_print_config_get_transform (job->config, GNOME_PRINT_KEY_PAGE_ORIENTATION_MATRIX, pp->lorient);
	/* Layout size */
	gnome_print_config_get_double (job->config, GNOME_PRINT_KEY_LAYOUT_WIDTH, &pp->lyw);
	gnome_print_config_get_double (job->config, GNOME_PRINT_KEY_LAYOUT_HEIGHT, &pp->lyh);

	/* Now come the affines */
	layout = gpa_node_get_child_from_path (GNOME_PRINT_CONFIG_NODE (job->config), GNOME_PRINT_KEY_LAYOUT);
	if (layout) {
		gint numlp;
		numlp = 0;
		if (gpa_node_get_int_path_value (layout, "LogicalPages", &numlp) && (numlp > 0)) {
			GPANode *pages;
			pages = gpa_node_get_child_from_path (layout, "Pages");
			if (pages) {
				GPANode *page;
				gdouble *affines;
				gint pagenum;
				affines = g_new (gdouble, 6 * numlp);
				pagenum = 0;
				for (page = gpa_node_get_child (pages, NULL); page != NULL; page = gpa_node_get_child (pages, page)) {
					guchar *transform;
					transform = gpa_node_get_value (page);
					gpa_node_unref (page);
					if (!transform)
						break;
					job_parse_transform (transform, affines + 6 * pagenum);
					g_free (transform);
					pagenum += 1;
					if (pagenum >= numlp)
						break;
				}
				gpa_node_unref (pages);
				if (pagenum == numlp) {
					pp->num_affines = numlp;
					pp->affines = affines;
				} else {
					g_free (affines);
				}
			}
		}
		gpa_node_unref (layout);
	} else {
		pp->affines = g_new (gdouble, 6);
		art_affine_identity (pp->affines);
		pp->num_affines = 1;
	}

	setlocale (LC_NUMERIC, loc);
	g_free (loc);
}

#define A4_WIDTH (210 * 72 / 25.4)
#define A4_HEIGHT (297 * 72 / 25.4)

static void
job_clear_config_data (GnomePrintJob *job)
{
	JOBPrivate *pp;

	g_return_if_fail (job->priv);

	pp = job->priv;

	pp->pw = A4_WIDTH;
	pp->ph = A4_HEIGHT;
	art_affine_identity (pp->porient);
	art_affine_identity (pp->lorient);
	pp->lyw = pp->pw;
	pp->lyh = pp->ph;
	pp->num_affines = 0;
	if (pp->affines) {
		g_free (pp->affines);
		pp->affines = NULL;
	}
	if (pp->LY_AFFINES) {
		g_free (pp->LY_AFFINES);
		pp->LY_AFFINES = NULL;
	}
	while (pp->LY_LIST) {
		g_free (pp->LY_LIST->data);
		pp->LY_LIST = g_list_remove (pp->LY_LIST, pp->LY_LIST->data);
	}
}


/* FIXME: This function should not be here, we should not be including gpa-private.h too */

/* All keys can be NULL */
GnomePrintLayoutData *
gnome_print_config_get_layout_data (GnomePrintConfig *config,
				    const guchar *pagekey,
				    const guchar *porientkey,
				    const guchar *lorientkey,
				    const guchar *layoutkey)
{
	GnomePrintLayoutData *lyd;
	guchar key[1024];
	const GnomePrintUnit *unit;
	GPANode *layout;
	gchar *loc;
	/* Local data */
	GnomePrintLayoutPageData *pages;
	gdouble porient[6], lorient[6];
	gdouble pw, ph, lyw, lyh;
	gint num_pages;
	gint numlp;
	
	g_return_val_if_fail (config != NULL, NULL);

	if (!pagekey)
		pagekey = GNOME_PRINT_KEY_PAPER_SIZE;
	if (!porientkey)
		porientkey = GNOME_PRINT_KEY_PAPER_ORIENTATION;
	if (!lorientkey)
		lorientkey = GNOME_PRINT_KEY_PAGE_ORIENTATION;
	if (!layoutkey)
		layoutkey = GNOME_PRINT_KEY_LAYOUT;


	/* Initialize */
	pw = 210 * 72.0 / 25.4;
	ph = 297 * 72.0 / 25.4;
	art_affine_identity (porient);
	art_affine_identity (lorient);
	lyw = 1.0;
	lyh = 1.0;
	num_pages = 0;
	pages = NULL;

	/* Now the fun part */

	loc = g_strdup (setlocale (LC_NUMERIC, NULL));
	setlocale (LC_NUMERIC, "C");

	/* Physical size */
	g_snprintf (key, 1024, "%s.Width", pagekey);
	if (gnome_print_config_get_length (config, key, &pw, &unit)) {
		gnome_print_convert_distance (&pw, unit, GNOME_PRINT_PS_UNIT);
	}
	g_snprintf (key, 1024, "%s.Height", pagekey);
	if (gnome_print_config_get_length (config, key, &ph, &unit)) {
		gnome_print_convert_distance (&ph, unit, GNOME_PRINT_PS_UNIT);
	}
	/* Physical orientation */
	g_snprintf (key, 1024, "%s.Paper2PrinterTransform", porientkey);
	gnome_print_config_get_transform (config, key, porient);
	/* Logical orientation */
	g_snprintf (key, 1024, "%s.Page2LayoutTransform", lorientkey);
	gnome_print_config_get_transform (config, key, lorient);
	/* Layout size */
	g_snprintf (key, 1024, "%s.Width", layoutkey);
	gnome_print_config_get_double (config, key, &lyw);
	g_snprintf (key, 1024, "%s.Height", layoutkey);
	gnome_print_config_get_double (config, key, &lyh);

	/* Get the layout tree from the config database */
	layout = gpa_node_get_child_from_path (GNOME_PRINT_CONFIG_NODE (config), layoutkey);
	if (!layout) {
		layout = gpa_node_get_child_from_path (NULL, "Globals.Document.Page.Layout.Plain");
		if (!layout) {
			g_warning ("Could not get Globals.Document.Page.Layout.Plain");
			return NULL;
		}
	}

	/* Now come the affines */
	numlp = 0;
	if (gpa_node_get_int_path_value (layout, "LogicalPages", &numlp) && (numlp > 0)) {
		GPANode *pnodes;
		pnodes = gpa_node_get_child_from_path (layout, "Pages");
		if (pnodes) {
			GPANode *page;
			gint pagenum;
			pages = g_new (GnomePrintLayoutPageData, numlp);
			pagenum = 0;
			for (page = gpa_node_get_child (pnodes, NULL); page != NULL; page = gpa_node_get_child (pnodes, page)) {
				guchar *transform;
				transform = gpa_node_get_value (page);
				gpa_node_unref (page);
				if (!transform)
					break;
				job_parse_transform (transform, pages[pagenum].matrix);
				g_free (transform);
				pagenum += 1;
				if (pagenum >= numlp)
					break;
			}
			gpa_node_unref (pnodes);
			if (pagenum == numlp) {
				num_pages = numlp;
			} else {
				g_free (pages);
			}
		}
	}
	gpa_node_unref (layout);

	setlocale (LC_NUMERIC, loc);
	g_free (loc);
	
	if (num_pages == 0) {
		g_warning ("Could not get_layout_data\n");
		return NULL;
	}

	/* Success */

	lyd = g_new (GnomePrintLayoutData, 1);
	lyd->pw = pw;
	lyd->ph = ph;
	memcpy (lyd->porient, porient, 6 * sizeof (gdouble));
	memcpy (lyd->lorient, lorient, 6 * sizeof (gdouble));
	lyd->lyw = lyw;
	lyd->lyh = lyh;
	lyd->num_pages = num_pages;
	lyd->pages = pages;

	return lyd;
}

void
gnome_print_layout_data_free (GnomePrintLayoutData *lyd)
{
	g_return_if_fail (lyd != NULL);

	if (lyd->pages)
		g_free (lyd->pages);
	g_free (lyd);
}

GnomePrintLayout *
gnome_print_layout_new_from_data (const GnomePrintLayoutData *lyd)
{
	GnomePrintLayout *ly;
	ArtDRect area, r;
	gdouble t;
	gdouble a[6];
	gint i;
	/* Layout data */
	gdouble PP2PA[6], LP2LY[6];
	gdouble PAW, PAH, LYW, LYH, LW, LH;

	g_return_val_if_fail (lyd != NULL, NULL);
	g_return_val_if_fail (lyd->num_pages > 0, NULL);
	g_return_val_if_fail (lyd->pages != NULL, NULL);

	/* Now comes the fun part */

	g_return_val_if_fail ((lyd->pw > EPSILON) && (lyd->ph > EPSILON), NULL);

	/* Initial setup */
	/* Calculate PP2PA */
	/* We allow only rectilinear setups, so we can cheat */
	PP2PA[0] = lyd->porient[0];
	PP2PA[1] = lyd->porient[1];
	PP2PA[2] = lyd->porient[2];
	PP2PA[3] = lyd->porient[3];
	t = lyd->pw * PP2PA[0] + lyd->ph * PP2PA[2];
	PP2PA[4] = (t < 0) ? -t : 0.0;
	t = lyd->pw * PP2PA[1] + lyd->ph * PP2PA[3];
	PP2PA[5] = (t < 0) ? -t : 0.0;
	PRINT_AFFINE ("PP2PA:", &PP2PA[0]);

	/* PPDP - Physical Page Dimensions in Printer */
	/* A: PhysicalPage X PhysicalOrientation X TRANSLATE -> Physical Page in Printer */
	area.x0 = 0.0;
	area.y0 = 0.0;
	area.x1 = lyd->pw;
	area.y1 = lyd->ph;
	art_drect_affine_transform (&r, &area, PP2PA);
	PAW = r.x1 - r.x0;
	PAH = r.y1 - r.y0;
	g_return_val_if_fail ((PAW > EPSILON) || (PAH > EPSILON), NULL);

	/* Now we have to find the size of layout page */
	/* Again, knowing that layouts are rectilinear helps us */
	art_affine_invert (a, lyd->pages[0].matrix);
	PRINT_AFFINE ("INV LY:", &a[0]);
	LYW = lyd->lyw * fabs (lyd->pw * a[0] + lyd->ph * a[2]);
	LYH = lyd->lyh * fabs (lyd->pw * a[1] + lyd->ph * a[3]);
	PRINT_2 ("LY Dimensions:", LYW, LYH);

	/* Calculate LP2LY */
	/* We allow only rectilinear setups, so we can cheat */
	LP2LY[0] = lyd->lorient[0];
	LP2LY[1] = lyd->lorient[1];
	LP2LY[2] = lyd->lorient[2];
	LP2LY[3] = lyd->lorient[3];
	/* Delay */
	LP2LY[4] = 0.0;
	LP2LY[5] = 0.0;
	/* Meanwhile find logical width and height */
	area.x0 = 0.0;
	area.y0 = 0.0;
	area.x1 = LYW;
	area.y1 = LYH;
	art_affine_invert (a, LP2LY);
	art_drect_affine_transform (&r, &area, a);
	LW = r.x1 - r.x0;
	LH = r.y1 - r.y0;
	g_return_val_if_fail ((LW > EPSILON) && (LH > EPSILON), NULL);
	PRINT_2 ("L Dimensions", LW, LH);
	/* Now complete matrix calculation */
	t = LW * LP2LY[0] + LH * LP2LY[2];
	LP2LY[4] = (t < 0) ? -t : 0.0;
	t = LW * LP2LY[1] + LH * LP2LY[3];
	LP2LY[5] = (t < 0) ? -t : 0.0;
	PRINT_AFFINE ("LP2LY:", &LP2LY[0]);

	/* So we are safely here and can allocate target */
	ly = g_new (GnomePrintLayout, 1);
	memcpy (ly->PP2PA, PP2PA, 6 * sizeof (gdouble));
	ly->PAW = PAW;
	ly->PAH = PAH;
	memcpy (ly->LP2LY, LP2LY, 6 * sizeof (gdouble));
	ly->LYW = LYW;
	ly->LYH = LYH;
	ly->LW = LW;
	ly->LH = LH;

	/* Good, now generate actual layout matrixes */

	ly->NLY = lyd->num_pages;
	ly->LYP = g_new (GnomePrintLayoutPage, 6);

	/* Extra fun */

	for (i = 0; i < lyd->num_pages; i++) {
		gdouble ly2p[6];
		/* Calculate Layout -> Physical Page affine */
		memcpy (ly2p, lyd->pages[i].matrix, 6 * sizeof (gdouble));
		ly2p[4] *= lyd->pw;
		ly2p[5] *= lyd->ph;
		/* PRINT_AFFINE ("Layout -> Physical:", &l2p[0]); */
		art_affine_multiply (ly->LYP[i].matrix, LP2LY, ly2p);
	}

	return ly;
}

void
gnome_print_layout_free (GnomePrintLayout *layout)
{
	g_return_if_fail (layout != NULL);

	if (layout->LYP)
		g_free (layout->LYP);
	g_free (layout);
}

static gboolean
job_parse_transform (guchar *str, gdouble *transform)
{
	gdouble t[6];
	guchar *p;
	gchar *e;
	gint i;

	art_affine_identity (transform);

	p = str;
	p = strchr (str, '(');
	if (!p)
		return FALSE;
	p += 1;
	if (!*p)
		return FALSE;
	for (i = 0; i < 6; i++) {
		while (*p && isspace (*p)) p += 1;
		if (!strncmp (p, "SQRT2", 5)) {
			t[i] = M_SQRT2;
			e = p + 5;
		} else if (!strncmp (p, "-SQRT2", 6)) {
			t[i] = -M_SQRT2;
			e = p + 6;
		} else if (!strncmp (p, "SQRT1_2", 7)) {
			t[i] = M_SQRT1_2;
			e = p + 7;
		} else if (!strncmp (p, "-SQRT1_2", 8)) {
			t[i] = -M_SQRT1_2;
			e = p + 8;
		} else {
			t[i] = strtod (p, &e);
		}
		if (e == (gchar *) p)
			return FALSE;
		p = e;
	}

	memcpy (transform, t, 6 * sizeof (gdouble));

	return TRUE;
}


/**
 * gnome_print_job_set_print_to_file:
 * @gmp: job
 * @output: output file, if NULL sets print to file to FALSE
 * 
 * Sets/unsets the print to file option for the job 
 * 
 * Return Value: 
 **/
gint
gnome_print_job_print_to_file (GnomePrintJob *job, gchar *output)
{
	if (output) {
		gnome_print_config_set (job->config, "Settings.Transport.Backend",    "file");
		gnome_print_config_set (job->config, GNOME_PRINT_KEY_OUTPUT_FILENAME, output);
	} else {
		/* In the future we might want to use the default or even better,
		 * go back to the prev. selected printer (Chema)
		 */
		gnome_print_config_set (job->config, "Settings.Transport.Backend", "lpr");
	}
	
	return GNOME_PRINT_OK;
}

/* fixme: Move this (and previous) to gnome-print-config.c (Lauris) */

gboolean
gnome_print_config_get_transform (GnomePrintConfig *config, const guchar *key, gdouble *transform)
{
	guchar *v;
	gdouble t[6];
	gboolean ret;
	gchar *loc;

	g_return_val_if_fail (config != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*key != '\0', FALSE);
	g_return_val_if_fail (config != NULL, FALSE);

	v = gnome_print_config_get (config, key);
	if (!v) {
		return FALSE;
	}

	loc = g_strdup (setlocale (LC_NUMERIC, NULL));
	setlocale (LC_NUMERIC, "C");

	ret = job_parse_transform (v, t);
	g_free (v);
	if (ret) {
		memcpy (transform, t, 6 * sizeof (gdouble));
	}

	setlocale (LC_NUMERIC, loc);
	g_free (loc);

	return ret;
}

