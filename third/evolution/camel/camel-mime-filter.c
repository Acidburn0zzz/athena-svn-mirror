/*
 *  Copyright (C) 2000 Ximian Inc.
 *
 *  Authors: Michael Zucchi <notzed@ximian.com>
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
 */

#include <string.h>
#include "camel-mime-filter.h"

/*#define MALLOC_CHECK */ /* for some malloc checking, requires mcheck enabled */

/* only suitable for glibc */
#ifdef MALLOC_CHECK
#include <mcheck.h>
#endif

struct _CamelMimeFilterPrivate {
	char *inbuf;
	size_t inlen;
};

#define PRE_HEAD (64)
#define BACK_HEAD (64)
#define _PRIVATE(o) (((CamelMimeFilter *)(o))->priv)
#define FCLASS(o) ((CamelMimeFilterClass *)(CAMEL_OBJECT_GET_CLASS(o)))

static CamelObjectClass *camel_mime_filter_parent;

static void complete (CamelMimeFilter *mf, char *in, size_t len, 
		      size_t prespace, char **out, size_t *outlen, 
		      size_t *outprespace);

static void
camel_mime_filter_class_init (CamelMimeFilterClass *klass)
{
	camel_mime_filter_parent = camel_type_get_global_classfuncs (camel_object_get_type ());

	klass->complete = complete;
}

static void
camel_mime_filter_init (CamelMimeFilter *obj)
{
	obj->outreal = NULL;
	obj->outbuf = NULL;
	obj->outsize = 0;

	obj->backbuf = NULL;
	obj->backsize = 0;
	obj->backlen = 0;

	_PRIVATE(obj) = g_malloc0(sizeof(*obj->priv));
}

static void
camel_mime_filter_finalize(CamelObject *o)
{
	CamelMimeFilter *f = (CamelMimeFilter *)o;
	struct _CamelMimeFilterPrivate *p = _PRIVATE(f);

	g_free(f->outreal);
	g_free(f->backbuf);
	g_free(p->inbuf);
	g_free(p);
}

CamelType
camel_mime_filter_get_type (void)
{
	static CamelType camel_mime_filter_type = CAMEL_INVALID_TYPE;
	
	if (camel_mime_filter_type == CAMEL_INVALID_TYPE) {
		camel_mime_filter_type = camel_type_register (CAMEL_OBJECT_TYPE, "CamelMimeFilter",
							      sizeof (CamelMimeFilter),
							      sizeof (CamelMimeFilterClass),
							      (CamelObjectClassInitFunc) camel_mime_filter_class_init,
							      NULL,
							      (CamelObjectInitFunc) camel_mime_filter_init,
							      (CamelObjectFinalizeFunc) camel_mime_filter_finalize);
	}
	
	return camel_mime_filter_type;
}

static void
complete(CamelMimeFilter *mf, char *in, size_t len, size_t prespace, char **out, size_t *outlen, size_t *outprespace)
{
	/* default - do nothing */
}

/**
 * camel_mime_filter_new:
 *
 * Create a new CamelMimeFilter object.
 * 
 * Return value: A new CamelMimeFilter widget.
 **/
CamelMimeFilter *
camel_mime_filter_new (void)
{
	CamelMimeFilter *new = CAMEL_MIME_FILTER ( camel_object_new (camel_mime_filter_get_type ()));
	return new;
}

#ifdef MALLOC_CHECK
static void
checkmem(void *p)
{
	if (p) {
		int status = mprobe(p);

		switch (status) {
		case MCHECK_HEAD:
			printf("Memory underrun at %p\n", p);
			abort();
		case MCHECK_TAIL:
			printf("Memory overrun at %p\n", p);
			abort();
		case MCHECK_FREE:
			printf("Double free %p\n", p);
			abort();
		}
	}
}
#endif

static void filter_run(CamelMimeFilter *f,
		       char *in, size_t len, size_t prespace,
		       char **out, size_t *outlen, size_t *outprespace,
		       void (*filterfunc)(CamelMimeFilter *f,
					  char *in, size_t len, size_t prespace,
					  char **out, size_t *outlen, size_t *outprespace))
{
	struct _CamelMimeFilterPrivate *p;

#ifdef MALLOC_CHECK
	checkmem(f->outreal);
	checkmem(f->backbuf);
#endif
	/*
	  here we take a performance hit, if the input buffer doesn't
	  have the pre-space required.  We make a buffer that does ...
	*/
	if (prespace < f->backlen) {
		int newlen = len+prespace+f->backlen;
		p = _PRIVATE(f);
		if (p->inlen < newlen) {
			/* NOTE: g_realloc copies data, we dont need that (slower) */
			g_free(p->inbuf);
			p->inbuf = g_malloc(newlen+PRE_HEAD);
			p->inlen = newlen+PRE_HEAD;
		}
		/* copy to end of structure */
		memcpy(p->inbuf+p->inlen - len, in, len);
		in = p->inbuf+p->inlen - len;
		prespace = p->inlen - len;
	}

#ifdef MALLOC_CHECK
	checkmem(f->outreal);
	checkmem(f->backbuf);
#endif

	/* preload any backed up data */
	if (f->backlen > 0) {
		memcpy(in-f->backlen, f->backbuf, f->backlen);
		in -= f->backlen;
		len += f->backlen;
		prespace -= f->backlen;
		f->backlen = 0;
	}
	
	filterfunc(f, in, len, prespace, out, outlen, outprespace);

#ifdef MALLOC_CHECK
	checkmem(f->outreal);
	checkmem(f->backbuf);
#endif

}

void camel_mime_filter_filter(CamelMimeFilter *f,
			      char *in, size_t len, size_t prespace,
			      char **out, size_t *outlen, size_t *outprespace)
{
	if (FCLASS(f)->filter)
		filter_run(f, in, len, prespace, out, outlen, outprespace, FCLASS(f)->filter);
	else
		g_error("Filter function unplmenented in class");
}

void camel_mime_filter_complete(CamelMimeFilter *f,
				char *in, size_t len, size_t prespace,
				char **out, size_t *outlen, size_t *outprespace)
{
	if (FCLASS(f)->complete)
		filter_run(f, in, len, prespace, out, outlen, outprespace, FCLASS(f)->complete);
}

void camel_mime_filter_reset(CamelMimeFilter *f)
{
	if (FCLASS(f)->reset) {
		FCLASS(f)->reset(f);
	}

	/* could free some buffers, if they are really big? */
	f->backlen = 0;
}

/* sets number of bytes backed up on the input, new calls replace previous ones */
void camel_mime_filter_backup(CamelMimeFilter *f, const char *data, size_t length)
{
	if (f->backsize < length) {
		/* g_realloc copies data, unnecessary overhead */
		g_free(f->backbuf);
		f->backbuf = g_malloc(length+BACK_HEAD);
		f->backsize = length+BACK_HEAD;
	}
	f->backlen = length;
	memcpy(f->backbuf, data, length);
}

/* ensure this much size available for filter output (if required) */
void camel_mime_filter_set_size(CamelMimeFilter *f, size_t size, int keep)
{
	if (f->outsize < size) {
		int offset = f->outptr - f->outreal;
		if (keep) {
			f->outreal = g_realloc(f->outreal, size + PRE_HEAD*4);
		} else {
			g_free(f->outreal);
			f->outreal = g_malloc(size + PRE_HEAD*4);
		}
		f->outptr = f->outreal + offset;
		f->outbuf = f->outreal + PRE_HEAD*4;
		f->outsize = size;
		/* this could be offset from the end of the structure, but 
		   this should be good enough */
		f->outpre = PRE_HEAD*4;
	}
}

