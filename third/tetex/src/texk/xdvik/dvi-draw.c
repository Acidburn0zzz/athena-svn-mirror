/*========================================================================*\

Copyright (c) 1990-1999  Paul Vojta

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
PAUL VOJTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

NOTE:
	xdvi is based on prior work, as noted in the modification history
	in xdvi.c.

\*========================================================================*/

#include "xdvi-config.h"
#include <kpathsea/c-ctype.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-vararg.h>
#include "dvi.h"

static	struct frame	frame0;		/* dummy head of list */
#ifdef	TEXXET
static	struct frame	*scan_frame;	/* head frame for scanning */
#endif

#ifndef	DVI_BUFFER_LEN
#define	DVI_BUFFER_LEN	512
#endif

static	ubyte	dvi_buffer[DVI_BUFFER_LEN];
static	struct frame	*current_frame;

#ifndef	TEXXET
#define	DIR	1
#else
#define	DIR	currinf.dir
#endif

/*
 *	Explanation of the following constant:
 *	offset_[xy]   << 16:	margin (defaults to one inch)
 *	shrink_factor << 16:	one pixel page border
 *	shrink_factor << 15:	rounding for pixel_conv
 */
#define OFFSET_X	(offset_x << 16) + (shrink_factor * 3 << 15)
#define OFFSET_Y	(offset_y << 16) + (shrink_factor * 3 << 15)

#if (BMBYTES == 1)
BMUNIT	bit_masks[9] = {
	0x0,	0x1,	0x3,	0x7,
	0xf,	0x1f,	0x3f,	0x7f,
	0xff
};
#else
#if (BMBYTES == 2)
BMUNIT	bit_masks[17] = {
	0x0,	0x1,	0x3,	0x7,
	0xf,	0x1f,	0x3f,	0x7f,
	0xff,	0x1ff,	0x3ff,	0x7ff,
	0xfff,	0x1fff,	0x3fff,	0x7fff,
	0xffff
};
#else	/* BMBYTES == 4 */
BMUNIT	bit_masks[33] = {
	0x0,		0x1,		0x3,		0x7,
	0xf,		0x1f,		0x3f,		0x7f,
	0xff,		0x1ff,		0x3ff,		0x7ff,
	0xfff,		0x1fff,		0x3fff,		0x7fff,
	0xffff,		0x1ffff,	0x3ffff,	0x7ffff,
	0xfffff,	0x1fffff,	0x3fffff,	0x7fffff,
	0xffffff,	0x1ffffff,	0x3ffffff,	0x7ffffff,
	0xfffffff,	0x1fffffff,	0x3fffffff,	0x7fffffff,
	0xffffffff
};
#endif
#endif

#ifdef	VMS
#define	off_t	int
#endif
extern	off_t	lseek();

#ifndef	SEEK_SET	/* if <unistd.h> is not provided (or for <X11R5) */
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

static	void	draw_part();

/*
 *	X routines.
 */

/*
 *	Put a rectangle on the screen.
 */

/* HTEX needs this to be non-static -janl */
void
put_rule(x, y, w, h)
	int		x, y;
	unsigned int	w, h;
{
	if (x < max_x && x + (int) w >= min_x
	  && y < max_y && y + (int) h >= min_y) {
	    if (--event_counter == 0) read_events(False);
#ifdef HTEX
	    XFillRectangle(DISP, currwin.win, highlight? highGC: ruleGC,
#else
	    XFillRectangle(DISP, currwin.win, ruleGC,
#endif
		x - currwin.base_x, y - currwin.base_y, w ? w : 1, h ? h : 1);
	}
}

static	void
put_bitmap(bitmap, x, y)
	register struct bitmap *bitmap;
	register int x, y;
{

#ifdef HTEX
	if (HTeXAnestlevel > 0) {
		htex_recordbits(x, y, bitmap->w, bitmap->h);
	}
#endif

	if (debug & DBG_BITMAP)
		Printf("X(%d,%d)\n", x - currwin.base_x, y - currwin.base_y);
	if (x < max_x && x + (int) bitmap->w >= min_x &&
	    y < max_y && y + (int) bitmap->h >= min_y) {
		if (--event_counter == 0) read_events(False);
		image->width = bitmap->w;
		image->height = bitmap->h;
		image->data = bitmap->bits;
		image->bytes_per_line = bitmap->bytes_wide;
#ifdef HTEX
		XPutImage(DISP, currwin.win, HTeXreflevel > 0? highGC: foreGC, image,
#else
		XPutImage(DISP, currwin.win, foreGC, image,
#endif
			0, 0,
			x - currwin.base_x, y - currwin.base_y,
			bitmap->w, bitmap->h);
		if (foreGC2)
		    XPutImage(DISP, currwin.win, foreGC2, image,
			0, 0,
			x - currwin.base_x, y - currwin.base_y,
			bitmap->w, bitmap->h);
	}
}

#ifdef	GREY
static	void
put_image(g, x, y)
	register struct glyph *g;
	register int x, y;
{
	register XImage *img = g->image2;

#ifdef HTEX
	if (HTeXAnestlevel > 0) {
		htex_recordbits(x, y, img->width, img->height);
	}
#endif
	if (x < max_x && x + img->width >= min_x &&
	    y < max_y && y + img->height >= min_y) {

	    if (--event_counter == 0) read_events(False);

#ifdef HTEX
	    XPutImage(DISP, currwin.win, foreGC, img,
#else
	    XPutImage(DISP, currwin.win, foreGC, img,
#endif
		    0, 0,
		    x - currwin.base_x, y - currwin.base_y,
		    (unsigned int) img->width, (unsigned int) img->height);
	    if (foreGC2)
		XPutImage(DISP, currwin.win, foreGC2, img,
		    0, 0,
		    x - currwin.base_x, y - currwin.base_y,
		    (unsigned int) img->width, (unsigned int) img->height);
	    if (pixeltbl_t != NULL) {
		img->data = g->pixmap2_t;
		XPutImage(DISP, currwin.win, foreGC2, img,
			0, 0,
			x - currwin.base_x, y - currwin.base_y,
			(unsigned int) img->width, (unsigned int) img->height);
		img->data = g->pixmap2;
	    }
	}
}
#endif	/* GREY */

/*
 *	Draw the border of a rectangle on the screen.
 */

static	void
put_border(x, y, width, height, ourGC)
	int		x, y;
	unsigned int	width, height;
	GC		ourGC;
{

	--width;
	--height;
	/* top */
	XFillRectangle(DISP, currwin.win, ourGC, x, y, width, 1);
	/* right */
	XFillRectangle(DISP, currwin.win, ourGC, x + (int) width, y, 1, height);
	/* bottom */
	XFillRectangle(DISP, currwin.win, ourGC, x + 1, y + (int) height,
	    width, 1);
	/* left */
	XFillRectangle(DISP, currwin.win, ourGC, x, y + 1, 1, height);
}

#ifdef GRID
/* drawing grid */
void
put_grid(x, y, width, height, unit, gc1, gc2, gc3)
	int		x, y;
	unsigned int	width, height, unit;
	GC		gc1, gc2, gc3;
{
	int i;			     /* looping variable */
	float sep;		     /* grid separation */
	int tmp;		     /* temporary variable */

	--width;
	--height;
  

/* drawing vertial grid */
#define DRAWGRID_VER(gc) for (i=1; \
			       (tmp=x+(int) (((float) i)*sep)) < x+width; \
			       i++) \
  			  XFillRectangle(DISP, currwin.win, (gc), \
				   tmp, y, 1, height)
/* drawing horizontal grid */
#define DRAWGRID_HOR(gc) for (i=1; \
			       (tmp=y+(int) (((float) i)*sep)) < y+height; \
			       i++) \
		          XFillRectangle(DISP, currwin.win, (gc), \
				   x, tmp, width, 1)

	if ( grid_mode > 2 )	     /* third level grid */
	  {
	    sep = (float) unit / 4.0;
	    DRAWGRID_VER ( gc3 );    /* vertical grid */
	    DRAWGRID_HOR ( gc3 );    /* horizontal grid */
	  }
  
	if ( grid_mode > 1 )	     /* second */
	  {
	    sep = (float) unit / 2.0;
	    DRAWGRID_VER ( gc2 );    /* vertical grid */
	    DRAWGRID_HOR ( gc2 );    /* horizontal grid */
	  }
  
	if ( grid_mode > 0 )	     /* first level */
	  {
	    sep = (float) unit ;
	    DRAWGRID_VER ( gc1 );    /* vertical grid */
	    DRAWGRID_HOR ( gc1 );    /* horizontal grid */
	  }
}

#endif /* GRID */

/*
 *	Byte reading routines for dvi file.
 */

#define	xtell(pos)	((long) (lseek(fileno(dvi_file), 0L, SEEK_CUR) - \
			    (currinf.end - (pos))))

static	ubyte
xxone()
{
	if (currinf.virtual) {
	    ++currinf.pos;
	    return EOP;
	}
	currinf.end = dvi_buffer +
	    read(fileno(dvi_file), (char *) (currinf.pos = dvi_buffer),
		DVI_BUFFER_LEN);
	return currinf.end > dvi_buffer ? *(currinf.pos)++ : EOF;
}

#define	xone()  (currinf.pos < currinf.end ? *(currinf.pos)++ : xxone())

static	unsigned long
xnum(size)
	register ubyte size;
{
	register long x = 0;

	while (size--) x = (x << 8) | xone();
	return x;
}

static	long
xsnum(size)
	register ubyte size;
{
	register long x;

#if	__STDC__
	x = (signed char) xone();
#else
	x = xone();
	if (x & 0x80) x -= 0x100;
#endif
	while (--size) x = (x << 8) | xone();
	return x;
}

#define	xsfour()	xsnum(4)

static	void
xskip(offset)
	long	offset;
{
	currinf.pos += offset;
	if (!currinf.virtual && currinf.pos > currinf.end)
	    (void) lseek(fileno(dvi_file), (long) (currinf.pos - currinf.end),
		SEEK_CUR);
}

#ifdef HAVE_PROTOTYPES
static	NORETURN void
tell_oops(_Xconst char *message, ...)
#else
/* VARARGS */
static	NORETURN void
tell_oops(va_alist)
	va_dcl
#endif
{
#ifndef HAVE_PROTOTYPES
	_Xconst char *message;
#endif
	va_list	args;

	Fprintf(stderr, "%s: ", prog);
#ifdef HAVE_PROTOTYPES
	va_start(args, message);
#else
	va_start(args);
	message = va_arg(args, _Xconst char *);
#endif
	(void) vfprintf(stderr, message, args);
	va_end(args);
	if (currinf.virtual)
	    Fprintf(stderr, " in virtual font %s\n", currinf.virtual->fontname);
	else
	    Fprintf(stderr, ", offset %ld\n", xtell(currinf.pos - 1));
#if	PS
	ps_destroy();
#endif
	exit(1);
}


/*
 *	Code for debugging options.
 */

static	void
print_bitmap(bitmap)
	register struct bitmap *bitmap;
{
	register BMUNIT *ptr = (BMUNIT *) bitmap->bits;
	register int x, y, i;

	if (ptr == NULL) oops("print_bitmap called with null pointer.");
	Printf("w = %d, h = %d, bytes wide = %d\n",
	    bitmap->w, bitmap->h, bitmap->bytes_wide);
	for (y = 0; y < (int) bitmap->h; ++y) {
	    for (x = bitmap->bytes_wide; x > 0; x -= BMBYTES) {
#ifndef	WORDS_BIGENDIAN
		for (i = 0; i < BMBITS; ++i)
#else
		for (i = BMBITS - 1; i >= 0; --i)
#endif
		    Putchar((*ptr & (1 << i)) ? '@' : ' ');
		++ptr;
	    }
	    Putchar('\n');
	}
}

static	void
print_char(ch, g)
	ubyte ch;
	struct glyph *g;
{
	Printf("char %d", ch);
	if (ISPRINT(ch))
	    Printf(" (%c)", ch);
	Putchar('\n');
	Printf("x = %d, y = %d, dvi = %ld\n", g->x, g->y, g->dvi_adv);
	print_bitmap(&g->bitmap);
}

static	_Xconst	char	*dvi_table1[] = {
#ifdef Omega
	"SET1", "SET2", NULL, NULL, "SETRULE", "PUT1", "PUT2", NULL,
#else
	"SET1", NULL, NULL, NULL, "SETRULE", "PUT1", NULL, NULL,
#endif
	NULL, "PUTRULE", "NOP", "BOP", "EOP", "PUSH", "POP", "RIGHT1",
	"RIGHT2", "RIGHT3", "RIGHT4", "W0", "W1", "W2", "W3", "W4",
	"X0", "X1", "X2", "X3", "X4", "DOWN1", "DOWN2", "DOWN3",
	"DOWN4", "Y0", "Y1", "Y2", "Y3", "Y4", "Z0", "Z1",
	"Z2", "Z3", "Z4"};

static	_Xconst	char	*dvi_table2[] = {
	"FNT1", "FNT2", "FNT3", "FNT4", "XXX1", "XXX2", "XXX3", "XXX4",
	"FNTDEF1", "FNTDEF2", "FNTDEF3", "FNTDEF4", "PRE", "POST", "POSTPOST",
	"SREFL", "EREFL", NULL, NULL, NULL, NULL};

static	void
print_dvi(ch)
	ubyte ch;
{
	_Xconst	char	*s;

	Printf("%4d %4d ", PXL_H, PXL_V);
	if (ch <= (ubyte) (SETCHAR0 + 127)) {
	    Printf("SETCHAR%-3d", ch - SETCHAR0);
	    if (ISPRINT(ch))
		Printf(" (%c)", ch);
	    Putchar('\n');
	    return;
	}
	else if (ch < FNTNUM0) s = dvi_table1[ch - 128];
	else if (ch <= (ubyte) (FNTNUM0 + 63)) {
	    Printf("FNTNUM%d\n", ch - FNTNUM0);
	    return;
	}
	else s = dvi_table2[ch - (FNTNUM0 + 64)];
	if (s) Puts(s);
	else
	    tell_oops("unknown op-code %d", ch);
}


/*
 *	Count the number of set bits in a given region of the bitmap
 */

char	sample_count[]	= {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

static	int
sample(bits, bytes_wide, bit_skip, w, h)
	BMUNIT	*bits;
	int	bytes_wide, bit_skip, w, h;
{
	BMUNIT	*ptr, *endp;
	register BMUNIT *cp;
	int	bits_left;
	register int n, bit_shift, wid;

	ptr = bits + bit_skip / BMBITS;
	endp = ADD(bits, h * bytes_wide);
	bits_left = w;
#ifndef	WORDS_BIGENDIAN
	bit_shift = bit_skip % BMBITS;
#else
	bit_shift = BMBITS - bit_skip % BMBITS;
#endif
	n = 0;
	while (bits_left) {
#ifndef	WORDS_BIGENDIAN
	    wid = BMBITS - bit_shift;
#else
	    wid = bit_shift;
#endif
	    if (wid > bits_left) wid = bits_left;
	    if (wid > 4) wid = 4;
#ifdef	WORDS_BIGENDIAN
	    bit_shift -= wid;
#endif
	    for (cp = ptr; cp < endp; cp = ADD(cp, bytes_wide))
		n += sample_count[(*cp >> bit_shift) & bit_masks[wid]];
#ifndef	WORDS_BIGENDIAN
	    bit_shift += wid;
	    if (bit_shift == BMBITS) {
		bit_shift = 0;
		++ptr;
	    }
#else
	    if (bit_shift == 0) {
		bit_shift = BMBITS;
		++ptr;
	    }
#endif
	    bits_left -= wid;
	}
	return n;
}

static	void
shrink_glyph(g)
	register struct glyph *g;
{
	int		shrunk_bytes_wide, shrunk_height;
	int		rows_left, rows, init_cols;
	int		cols_left;
	register int	cols;
	BMUNIT		*old_ptr, *new_ptr;
	register BMUNIT	m, *cp;
	int	min_sample = shrink_factor * shrink_factor * density / 100;

	/* These machinations ensure that the character is shrunk according to
	   its hot point, rather than its upper left-hand corner. */
	g->x2 = g->x / shrink_factor;
	init_cols = g->x - g->x2 * shrink_factor;
	if (init_cols <= 0) init_cols += shrink_factor;
	else ++g->x2;
	g->bitmap2.w = g->x2 + ROUNDUP((int) g->bitmap.w - g->x, shrink_factor);
	/* include row zero with the positively numbered rows */
	cols = g->y + 1; /* spare register variable */
	g->y2 = cols / shrink_factor;
	rows = cols - g->y2 * shrink_factor;
	if (rows <= 0) {
	    rows += shrink_factor;
	    --g->y2;
	}
	g->bitmap2.h = shrunk_height = g->y2 +
	    ROUNDUP((int) g->bitmap.h - cols, shrink_factor) + 1;
	alloc_bitmap(&g->bitmap2);
	old_ptr = (BMUNIT *) g->bitmap.bits;
	new_ptr = (BMUNIT *) g->bitmap2.bits;
	shrunk_bytes_wide = g->bitmap2.bytes_wide;
	rows_left = g->bitmap.h;
	bzero((char *) new_ptr, shrunk_bytes_wide * shrunk_height);
	while (rows_left) {
	    if (rows > rows_left) rows = rows_left;
	    cols_left = g->bitmap.w;
#ifndef	WORDS_BIGENDIAN
	    m = (1 << 0);
#else
	    m = ((BMUNIT) 1 << (BMBITS-1));
#endif
	    cp = new_ptr;
	    cols = init_cols;
	    while (cols_left) {
		if (cols > cols_left) cols = cols_left;
		if (sample(old_ptr, g->bitmap.bytes_wide,
			(int) g->bitmap.w - cols_left, cols, rows)
			>= min_sample)
		    *cp |= m;
#ifndef	WORDS_BIGENDIAN
		if (m == ((BMUNIT)1 << (BMBITS-1))) {
		    m = (1 << 0);
		    ++cp;
		}
		else m <<= 1;
#else
		if (m == (1 << 0)) {
		    m = ((BMUNIT) 1 << (BMBITS-1));
		    ++cp;
		}
		else m >>= 1;
#endif
		cols_left -= cols;
		cols = shrink_factor;
	    }
	    *((char **) &new_ptr) += shrunk_bytes_wide;
	    *((char **) &old_ptr) += rows * g->bitmap.bytes_wide;
	    rows_left -= rows;
	    rows = shrink_factor;
	}
	g->y2 = g->y / shrink_factor;
	if (debug & DBG_BITMAP)
	    print_bitmap(&g->bitmap2);
}

#ifdef	GREY
static	void
shrink_glyph_grey(g)
	register struct glyph *g;
{
	int		rows_left, rows, init_cols;
	int		cols_left;
	register int	cols;
	int		x, y;
	long		thesample;
	BMUNIT		*old_ptr;
	unsigned int	size;

	/* These machinations ensure that the character is shrunk according to
	   its hot point, rather than its upper left-hand corner. */
	g->x2 = g->x / shrink_factor;
	init_cols = g->x - g->x2 * shrink_factor;
	if (init_cols <= 0) init_cols += shrink_factor;
	else ++g->x2;
	g->bitmap2.w = g->x2 + ROUNDUP((int) g->bitmap.w - g->x, shrink_factor);
	/* include row zero with the positively numbered rows */
	cols = g->y + 1; /* spare register variable */
	g->y2 = cols / shrink_factor;
	rows = cols - g->y2 * shrink_factor;
	if (rows <= 0) {
	    rows += shrink_factor;
	    --g->y2;
	}
	g->bitmap2.h = g->y2 + ROUNDUP((int) g->bitmap.h - cols, shrink_factor)
	    + 1;

	g->image2 = XCreateImage(DISP, our_visual, our_depth, ZPixmap,
	  0, (char *) NULL, g->bitmap2.w, g->bitmap2.h, BMBITS, 0);
	size = g->image2->bytes_per_line * g->bitmap2.h;
	g->pixmap2 = g->image2->data = xmalloc(size != 0 ? size : 1);
	if (pixeltbl_t != NULL)
	    g->pixmap2_t = xmalloc(size != 0 ? size : 1);

	old_ptr = (BMUNIT *) g->bitmap.bits;
	rows_left = g->bitmap.h;
	y = 0;
	while (rows_left) {
	    x = 0;
	    if (rows > rows_left) rows = rows_left;
	    cols_left = g->bitmap.w;
	    cols = init_cols;
	    while (cols_left) {
		if (cols > cols_left) cols = cols_left;

		thesample = sample(old_ptr, g->bitmap.bytes_wide,
			(int) g->bitmap.w - cols_left, cols, rows);
		XPutPixel(g->image2, x, y, pixeltbl[thesample]);
		if (pixeltbl_t != NULL) {
		    g->image2->data = g->pixmap2_t;
		    XPutPixel(g->image2, x, y, pixeltbl_t[thesample]);
		    g->image2->data = g->pixmap2;
		}

		cols_left -= cols;
		cols = shrink_factor;
		x++;
	    }
	    *((char **) &old_ptr) += rows * g->bitmap.bytes_wide;
	    rows_left -= rows;
	    rows = shrink_factor;
	    y++;
	}

	while (y < (int) g->bitmap2.h) {
	    for (x = 0; x < (int) g->bitmap2.w; x++) {
		XPutPixel(g->image2, x, y, *pixeltbl);
		if (pixeltbl_t != NULL) {
		    g->image2->data = g->pixmap2_t;
		    XPutPixel(g->image2, x, y, *pixeltbl_t);
		    g->image2->data = g->pixmap2;
		}
	    }
	    y++;
	}

	g->y2 = g->y / shrink_factor;
}
#endif	/* GREY */

/*
 *	Find font #n.
 */

static	void
change_font(n)
	unsigned long n;
{
	register struct tn *tnp;

	if (n < currinf.tn_table_len) currinf.fontp = currinf.tn_table[n];
	else {
	    currinf.fontp = NULL;
	    for (tnp = currinf.tn_head; tnp != NULL; tnp = tnp->next)
		if (tnp->TeXnumber == n) {
		    currinf.fontp = tnp->fontp;
		    break;
		}
	}
	if (currinf.fontp == NULL) tell_oops("non-existent font #%d", n);
	maxchar = currinf.fontp->maxchar;
	currinf.set_char_p = currinf.fontp->set_char_p;
}


/*
 *	Open a font file.
 */

static	void
open_font_file(fontp)
	struct font *fontp;
{
	if (fontp->file == NULL) {
	    fontp->file = xfopen_local(fontp->filename, OPEN_MODE);
	    if (fontp->file == NULL)
		oops("Font file disappeared:  %s", fontp->filename);
	}
}

/*
 *	Read special string.
 */

static	char *
read_special(nbytes)
	long	nbytes;
{
	static	char	*spcl	= NULL;
	static	long	spcl_len = -1;
	char	*p;

	if (spcl_len < nbytes) {
	    if (spcl != NULL) free(spcl);
	    spcl = xmalloc((unsigned) nbytes + 1);
	    spcl_len = nbytes;
	}
	p = spcl;
	for (;;) {
	    int i = currinf.end - currinf.pos;

	    if (i > nbytes) i = nbytes;
	    bcopy((char *) currinf.pos, p, i);
	    currinf.pos += i;
	    p += i;
	    nbytes -= i;
	    if (nbytes == 0) break;
	    (void) xxone();
	    --currinf.pos;
	}
	*p = '\0';
	return spcl;
}

#if	PS

/*
 *	Size of page interval for "Scanning pages xx-xx" message.
 */

#ifndef	REPORT_INCR
#define	REPORT_INCR	50
#endif

/*
 *	Table used for scanning.  If >= 0, then skip that many bytes.
 *	M1 means end of page, M2 means special, M3 means FNTDEF,
 *	M4 means unrecognizable, and M5 means doesn't belong here.
 */

#define	M1	255
#define	M2	254
#define	M3	253
#define	M4	252
#define	M5	251
#define	MM	251

static	ubyte	scantable[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* chars 0 - 127 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
#ifdef Omega
	1,2,			/* SET1,SET2 (128,129) */
		/* -,-,SETRULE,PUT1,PUT2,-,-,PUTRULE,NOP,BOP (130-139) */
	M4,M4,8,1,2,M4,M4,8,0,44,
#else

	1,M4,			/* SET1,- (128,129) */
			/* -,-,SETRULE,PUT1,-,-,-,PUTRULE,NOP,BOP (130-139) */
	M4,M4,8,1,M4,M4,M4,8,0,44,
#endif
	M1,0,0,1,2,3,4,0,1,2,	/* EOP,PUSH,POP,RIGHT1-4,W0M2 (140-149) */
	3,4,0,1,2,3,4,1,2,3,	/* W3-4,X0-4,DOWN1-3 (150-159) */
	4,0,1,2,3,4,0,1,2,3,	/* DOWN4,Y0-4,Z0-3 (160-169) */
	4,			/* Z4 (170) */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* change font 171 - 234 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,2,3,4,M2,		/* FNT1-4,XXX1 (235-239) */
			/* XXX2-4,FNTDEF1-4,PRE,POST,POSTPOST (240-249) */
	M2,M2,M2,M3,M3,M3,M3,M5,M5,M5,
	0,0,M4,M4,M4,M4};	/* SREFL,EREFL,-,-,-,- (250-255) */

/*
 *	Prescanning routine for dvi file.  This looks for specials like
 *	`header=' and `!'.
 */

static	void
prescan()
{
	ubyte	ch;
	ubyte	n;
	long	a;
	int	nextreportpage;
	char	scanmsg[40];

	if (!resource._postscript) {
	    scanned_page = total_pages;
	    return;
	}
	nextreportpage = scanned_page;
	++scanned_page;
	(void) lseek(fileno(dvi_file), page_offset[scanned_page], SEEK_SET);
	currinf.pos = currinf.end = dvi_buffer;
	for (;;) {
	    if (debug & DBG_PS)
		Printf("Scanning page %d\n", scanned_page);
	    if (scanned_page > nextreportpage) {
		nextreportpage += REPORT_INCR;
		if (nextreportpage > current_page)
		    nextreportpage = current_page;
		Sprintf(scanmsg, "Scanning pages %d-%d",
		    scanned_page + pageno_correct,
		    nextreportpage + pageno_correct);
		showmessage(scanmsg);
		XFlush(DISP);
	    }
	    for (;;) {
		ch = xone();
		n = scantable[ch];
		if (n < MM)
		    while (n-- != 0)
			(void) xone();
		else if (n == M1) break;	/* end of page */
		else switch (n) {
		    case M2:	/* special */
			a = xnum(ch - XXX1 + 1);
			if (a > 0)
			    scan_special(read_special(a));
			break;
		    case M3:	/* FNTDEF */
			xskip((long) (12 + ch - FNTDEF1 + 1));
			ch = xone();
			xskip((long) ch + (long) xone());
			break;
		    case M4:	/* unrecognizable */
			tell_oops("unknown op-code %d", ch);
			break;
		    case M5:	/* doesn't belong */
			tell_oops("shouldn't happen: %s encountered",
				dvi_table2[ch - (FNTNUM0 + 64)]);
			break;
		}
	    }
	    if (scanned_page >= current_page) break;
	    ++scanned_page;
	}
	scanned_page_bak = scanned_page;
	XClearWindow(DISP, mane.win);
	psp.endheader();
}

#endif	/* PS */

/*
 *	Routines to print characters.
 */

#ifndef	TEXXET
#define	ERRVAL	0L
#else
#define	ERRVAL
#endif

#ifndef	TEXXET
long
set_char P1C(wide_ubyte, ch)
#else
void
set_char P2C(wide_ubyte, cmd, wide_ubyte, ch)
#endif
{
	register struct glyph *g;
#ifdef	TEXXET
	long	dvi_h_sav;
#endif

	if (ch > maxchar) realloc_font(currinf.fontp, WIDENINT ch);
	if ((g = &currinf.fontp->glyph[ch])->bitmap.bits == NULL) {
	    if (g->addr == 0) {
		if (!hush_chars)
		    Fprintf(stderr, "Character %d not defined in font %s\n", ch,
			currinf.fontp->fontname);
		g->addr = -1;
		return ERRVAL;
	    }
	    if (g->addr == -1)
		return ERRVAL;	/* previously flagged missing char */
	    open_font_file(currinf.fontp);
	    Fseek(currinf.fontp->file, g->addr, 0);
	    (*currinf.fontp->read_char)(currinf.fontp, ch);
	    if (debug & DBG_BITMAP) print_char((ubyte) ch, g);
	    currinf.fontp->timestamp = ++current_timestamp;
	}

#ifdef	TEXXET
	dvi_h_sav = DVI_H;
	if (currinf.dir < 0) DVI_H -= g->dvi_adv;
	if (scan_frame == NULL) {
#endif
	    if (shrink_factor == 1)
		put_bitmap(&g->bitmap, PXL_H - g->x, PXL_V - g->y);
	    else {
#ifdef	GREY
		if (use_grey) {
		    if (g->pixmap2 == NULL) {
			shrink_glyph_grey(g);
		    }
		    put_image(g, PXL_H - g->x2, PXL_V - g->y2);
		} else {
		    if (g->bitmap2.bits == NULL) {
			shrink_glyph(g);
		    }
		    put_bitmap(&g->bitmap2, PXL_H - g->x2, PXL_V - g->y2);
		}
#else
		if (g->bitmap2.bits == NULL) {
		    shrink_glyph(g);
		}
		put_bitmap(&g->bitmap2, PXL_H - g->x2, PXL_V - g->y2);
#endif
	    }
#ifndef	TEXXET
	return g->dvi_adv;
#else
	}
#ifdef Omega
	if ((cmd == PUT1) || (cmd == PUT2))
#else
	if (cmd == PUT1)
#endif
	    DVI_H = dvi_h_sav;
	else
	    if (currinf.dir > 0) DVI_H += g->dvi_adv;
#endif
}


/* ARGSUSED */
#ifndef	TEXXET
static	long
set_empty_char P1C(wide_ubyte, ch)
#else
static	void
set_empty_char P2C(wide_ubyte, cmd, wide_ubyte, ch)
#endif
{
#ifndef	TEXXET
	return 0;
#else
	return;
#endif
}


#ifndef	TEXXET
long
load_n_set_char P1C(wide_ubyte, ch)
#else
void
load_n_set_char P2C(wide_ubyte, cmd, wide_ubyte, ch)
#endif
{
	if (load_font(currinf.fontp)) {	/* if not found */
	    Fputs("Character(s) will be left blank.\n", stderr);
	    currinf.set_char_p = currinf.fontp->set_char_p = set_empty_char;
	    currinf.fontp->glyph = NULL;        /* init for read_postamble */
#ifndef	TEXXET
	    return 0;
#else
	    return;
#endif
	}
	maxchar = currinf.fontp->maxchar;
	currinf.set_char_p = currinf.fontp->set_char_p;
#ifndef	TEXXET
	return (*currinf.set_char_p)(ch);
#else
	(*currinf.set_char_p)(cmd, ch);
	return;
#endif
}


#ifndef	TEXXET
long
set_vf_char P1C(wide_ubyte, ch)
#else
void
set_vf_char P2C(wide_ubyte, cmd, wide_ubyte, ch)
#endif
{
	register struct macro *m;
	struct drawinf	oldinfo;
#ifdef Omega
	wide_ubyte	oldmaxchar;
#else
	ubyte	oldmaxchar;
#endif
	static	ubyte	c;
#ifdef	TEXXET
	long	dvi_h_sav;
#endif

	if (ch > maxchar) realloc_virtual_font(currinf.fontp, ch);
	if ((m = &currinf.fontp->macro[ch])->pos == NULL) {
	    if (!hush_chars)
		Fprintf(stderr, "Character %d not defined in font %s\n", ch,
		    currinf.fontp->fontname);
	    m->pos = m->end = &c;
	    return ERRVAL;
	}
#ifdef	TEXXET
	dvi_h_sav = DVI_H;
	if (currinf.dir < 0) DVI_H -= m->dvi_adv;
	if (scan_frame == NULL) {
#endif
	    oldinfo = currinf;
	    oldmaxchar = maxchar;
	    WW = XX = YY = ZZ = 0;
	    currinf.tn_table_len = VFTABLELEN;
	    currinf.tn_table = currinf.fontp->vf_table;
	    currinf.tn_head = currinf.fontp->vf_chain;
	    currinf.pos = m->pos;
	    currinf.end = m->end;
	    currinf.virtual = currinf.fontp;
	    draw_part(current_frame, currinf.fontp->dimconv);
	    if (currinf.pos != currinf.end + 1)
		tell_oops("virtual character macro does not end correctly");
	    currinf = oldinfo;
	    maxchar = oldmaxchar;
#ifndef	TEXXET
	return m->dvi_adv;
#else
	}
#ifdef Omega
	if ((cmd == PUT1) || (cmd == PUT2))
#else
	if (cmd == PUT1)
#endif
	    DVI_H = dvi_h_sav;
	else
	    if (currinf.dir > 0) DVI_H += m->dvi_adv;
#endif
}


#ifndef	TEXXET
static	long
set_no_char P1C(wide_ubyte, ch)
#else
static	void
set_no_char P2C(wide_ubyte, cmd, wide_ubyte, ch)
#endif
{
	if (currinf.virtual) {
	    currinf.fontp = currinf.virtual->first_font;
	    if (currinf.fontp != NULL) {
		maxchar = currinf.fontp->maxchar;
		currinf.set_char_p = currinf.fontp->set_char_p;
#ifndef	TEXXET
		return (*currinf.set_char_p)(ch);
#else
		(*currinf.set_char_p)(cmd, ch);
		return;
#endif
	    }
	}
	tell_oops("attempt to set character of unknown font");
	/* NOTREACHED */
}


/*
 *	Set rule.  Arguments are coordinates of lower left corner.
 */

static	void
set_rule(h, w)
	int h, w;
{
#ifndef	TEXXET
	put_rule(PXL_H, PXL_V - h + 1, (unsigned int) w, (unsigned int) h);
#else
	put_rule(PXL_H - (currinf.dir < 0 ? w - 1 : 0), PXL_V - h + 1,
	    (unsigned int) w, (unsigned int) h);
#endif
}

#define	xspell_conv(n)	spell_conv0(n, current_dimconv)

static	void
draw_part(minframe, current_dimconv)
	struct frame	*minframe;
	double		current_dimconv;
{
	ubyte ch;
#ifdef	TEXXET
	struct drawinf	oldinfo;
	ubyte	oldmaxchar;
	off_t	file_pos;
	int	refl_count;
#endif

	currinf.fontp = NULL;
	currinf.set_char_p = set_no_char;
#ifdef	TEXXET
	currinf.dir = 1;
	scan_frame = NULL;	/* indicates we're not scanning */
#endif
	for (;;) {
	    ch = xone();
	    if (debug & DBG_DVI)
		print_dvi(ch);
	    if (ch <= (ubyte) (SETCHAR0 + 127))
#ifndef	TEXXET
		DVI_H += (*currinf.set_char_p)(ch);
#else
		(*currinf.set_char_p)(ch, ch);
#endif
	    else if (FNTNUM0 <= ch && ch <= (ubyte) (FNTNUM0 + 63))
		change_font((unsigned long) (ch - FNTNUM0));
	    else {
		long a, b;

		switch (ch) {
		    case SET1:
		    case PUT1:
#ifndef	TEXXET
			a = (*currinf.set_char_p)(xone());
			if (ch != PUT1) DVI_H += a;
#else
			(*currinf.set_char_p)(ch, xone());
#endif
			break;

#ifdef Omega
                  case SET2:
                  case PUT2:
#ifndef       TEXXET
                        a = (*currinf.set_char_p)(xnum(2));
                        if (ch != PUT2) DVI_H += a;
#else
                        (*currinf.set_char_p)(ch, xnum(2));
#endif
                        break;
#endif /* Omega */

		    case SETRULE:
			/* Be careful, dvicopy outputs rules with
			   height = 0x80000000.  We don't want any
			   SIGFPE here. */
			a = xsfour();
			b = xspell_conv(xsfour());
#ifndef	TEXXET
			if (a > 0 && b > 0)
#else
			if (a > 0 && b > 0 && scan_frame == NULL)
#endif
			    set_rule(pixel_round(xspell_conv(a)),
				pixel_round(b));
			DVI_H += DIR * b;
			break;

		    case PUTRULE:
			a = xspell_conv(xsfour());
			b = xspell_conv(xsfour());
#ifndef	TEXXET
			if (a > 0 && b > 0)
#else
			if (a > 0 && b > 0 && scan_frame == NULL)
#endif
			    set_rule(pixel_round(a), pixel_round(b));
			break;

		    case NOP:
			break;

		    case BOP:
			xskip((long) 11 * 4);
			DVI_H = OFFSET_X;
			DVI_V = OFFSET_Y;
			PXL_V = pixel_conv(DVI_V);
			WW = XX = YY = ZZ = 0;
			break;

		    case EOP:
			if (current_frame != minframe)
			    tell_oops("stack not empty at EOP");
			return;

		    case PUSH:
			if (current_frame->next == NULL) {
			    struct frame *newp = xmalloc(sizeof(struct frame));
			    current_frame->next = newp;
			    newp->prev = current_frame;
			    newp->next = NULL;
			}
			current_frame = current_frame->next;
			current_frame->data = currinf.data;
			break;

		    case POP:
			if (current_frame == minframe)
			    tell_oops("more POPs than PUSHes");
			currinf.data = current_frame->data;
			current_frame = current_frame->prev;
			break;

#ifdef	TEXXET
		    case SREFL:
			if (scan_frame == NULL) {
			    /* we're not scanning:  save some info. */
			    oldinfo = currinf;
			    oldmaxchar = maxchar;
			    if (!currinf.virtual)
				file_pos = xtell(currinf.pos);
			    scan_frame = current_frame; /* now we're scanning */
			    refl_count = 0;
			    break;
			}
			/* we are scanning */
			if (current_frame == scan_frame) ++refl_count;
			break;

		    case EREFL:
			if (scan_frame != NULL) {	/* if we're scanning */
			    if (current_frame == scan_frame && --refl_count < 0)
			    {
				/* we've hit the end of our scan */
				scan_frame = NULL;
				/* first:  push */
				if (current_frame->next == NULL) {
				    struct frame *newp = 
				      xmalloc(sizeof(struct frame));
				    current_frame->next = newp;
				    newp->prev = current_frame;
				    newp->next = NULL;
				}
				current_frame = current_frame->next;
				current_frame->data = currinf.data;
				/* next:  restore old file position, XX, etc. */
				if (!currinf.virtual) {
				    off_t bgn_pos = xtell(dvi_buffer);

				    if (file_pos >= bgn_pos) {
					oldinfo.pos = dvi_buffer
					    + (file_pos - bgn_pos);
					oldinfo.end = currinf.end;
				    }
				    else {
					(void) lseek(fileno(dvi_file), file_pos,
					    SEEK_SET);
					oldinfo.pos = oldinfo.end;
				    }
				}
				currinf = oldinfo;
				maxchar = oldmaxchar;
				/* and then:  recover position info. */
				DVI_H = current_frame->data.dvi_h;
				DVI_V = current_frame->data.dvi_v;
				PXL_V = current_frame->data.pxl_v;
				/* and finally, reverse direction */
				currinf.dir = -currinf.dir;
			    }
			    break;
			}
			/* we're not scanning, */
			/* so just reverse direction and then pop */
			currinf.dir = -currinf.dir;
			currinf.data = current_frame->data;
			current_frame = current_frame->prev;
			break;
#endif	/* TEXXET */

		    case RIGHT1:
		    case RIGHT2:
		    case RIGHT3:
		    case RIGHT4:
			DVI_H += DIR * xspell_conv(xsnum(ch - RIGHT1 + 1));
			break;

		    case W1:
		    case W2:
		    case W3:
		    case W4:
			WW = xspell_conv(xsnum(ch - W0));
		    case W0:
			DVI_H += DIR * WW;
			break;

		    case X1:
		    case X2:
		    case X3:
		    case X4:
			XX = xspell_conv(xsnum(ch - X0));
		    case X0:
			DVI_H += DIR * XX;
			break;

		    case DOWN1:
		    case DOWN2:
		    case DOWN3:
		    case DOWN4:
			DVI_V += xspell_conv(xsnum(ch - DOWN1 + 1));
			PXL_V = pixel_conv(DVI_V);
			break;

		    case Y1:
		    case Y2:
		    case Y3:
		    case Y4:
			YY = xspell_conv(xsnum(ch - Y0));
		    case Y0:
			DVI_V += YY;
			PXL_V = pixel_conv(DVI_V);
			break;

		    case Z1:
		    case Z2:
		    case Z3:
		    case Z4:
			ZZ = xspell_conv(xsnum(ch - Z0));
		    case Z0:
			DVI_V += ZZ;
			PXL_V = pixel_conv(DVI_V);
			break;

		    case FNT1:
		    case FNT2:
		    case FNT3:
		    case FNT4:
			change_font(xnum(ch - FNT1 + 1));
			break;

		    case XXX1:
		    case XXX2:
		    case XXX3:
		    case XXX4:
			a = xnum(ch - XXX1 + 1);
			if (a > 0)
			    applicationDoSpecial(read_special(a));
			break;

		    case FNTDEF1:
		    case FNTDEF2:
		    case FNTDEF3:
		    case FNTDEF4:
			xskip((long) (12 + ch - FNTDEF1 + 1));
			a = (long) xone();
			xskip(a + (long) xone());
			break;

#ifndef	TEXXET
		    case SREFL:
		    case EREFL:
#endif
		    case PRE:
		    case POST:
		    case POSTPOST:
			tell_oops("shouldn't happen: %s encountered",
				dvi_table2[ch - (FNTNUM0 + 64)]);
			break;

		    default:
			tell_oops("unknown op-code %d", ch);
		} /* end switch*/
	    } /* end else (ch not a SETCHAR or FNTNUM) */
	} /* end for */
}

#undef	xspell_conv

#ifdef HTEX 
static void
check_for_anchor() /* see if anchor requested, if so go to it */
{
	if (anchor_name != NULL) {
		htex_reinit(); /* Set up hypertext stuff */
		htex_do_loc(anchor_name);
		free(anchor_name);
		anchor_name = NULL;
	}
}
#endif /* HTEX */

void
draw_page()
{
	/* Check for changes in dvi file. */
	if (!check_dvi_file()) return;

#if	PS
	if (scanned_page < current_page) prescan();
#endif

	put_border(-currwin.base_x, -currwin.base_y,
	    ROUNDUP(unshrunk_paper_w, shrink_factor) + 2,
	    ROUNDUP(unshrunk_paper_h, shrink_factor) + 2, highGC);
#ifdef GRID
	if ( grid_mode > 0 )	/* grid is wanted */
	  put_grid(-currwin.base_x, -currwin.base_y,
		   ROUNDUP(unshrunk_paper_w, shrink_factor) + 2,
		   ROUNDUP(unshrunk_paper_h, shrink_factor) + 2, 
		   ROUNDUP(unshrunk_paper_unit, shrink_factor), 
		   grid1GC, grid2GC, grid3GC);
#endif /* GRID */
	(void) lseek(fileno(dvi_file), page_offset[current_page], SEEK_SET);

	bzero((char *) &currinf.data, sizeof(currinf.data));
	currinf.tn_table_len = TNTABLELEN;
	currinf.tn_table = tn_table;
	currinf.tn_head = tn_head;
	currinf.pos = currinf.end = dvi_buffer;
	currinf.virtual = NULL;
#ifdef HTEX
 	htex_initpage(); /* Start up the hypertext stuff for this page */
#endif
	psfig_begun = False;
	draw_part(current_frame = &frame0, dimconv);
#ifdef HTEX
 	htex_donepage(current_page, 1); /* Finish up the hypertext stuff for this page */
 	check_for_anchor(); /* see if a new anchor was requested */
#endif
#if	PS
	psp.endpage();
#endif
}

#ifdef HTEX
extern int *htex_parsedpages; /* List of pages parsed */

/* Parse dvi pages not yet scanned by xdvi, searching for html anchors */
void htex_parsepages()
{
	int i;

	for (i=0; i < total_pages; i++) {
		if (htex_parsedpages[i] != 0) continue;
		htex_parse_page(i);
	}
	/* Return to original position in dvi file? */
}

void htex_parse_page(i)
int i;
{
	/* Skip to beginning of page i */
/* Stuff from draw_page in dvi_draw.c: */
	(void) lseek(fileno(dvi_file), page_offset[i], SEEK_SET);
	bzero((char *) &currinf.data, sizeof(currinf.data));
	currinf.tn_head = tn_head;
	currinf.pos = currinf.end = dvi_buffer;
	currinf.virtual = NULL;
	htex_initpage();
	htex_scanpage(i);
	htex_donepage(i, 0);
}


/* HTeX parse page without drawing it function: */
/* Basically the same as is done by draw_part, except we don't
	do anything other than record html anchors */
/* Assumes page has been set up properly as above */
void htex_scanpage(pageno)
int pageno;
{
	ubyte ch;
	long a;

	/* Read in page, searching for XXX1 to XXX4 (\special)
		sections and ignoring everything else */
	for (;;) {
	    ch = xone();
	    if (ch <= SETCHAR0 + 127) { /* It's a character - do nothing */
	    } else if (FNTNUM0 <= ch && ch <= FNTNUM0 + 63) {
	    	/* It's a previously numbered font - again do nothing */
	    } else {
	    	switch(ch) {
		    case PUSH: /* No args */
		    case POP: /* No args */
	    	    case NOP: /* Nothing! */
	    	    	break;
	    	    case SET1:
	    	    case PUT1:
	    	    	xskip((long) 1); /* Set or put 1 char - skip it */
	    	    	break;
#ifdef Omega
                    case SET2:
                    case PUT2:
                        xskip((long) 2); /* Set or put 16-bit char - skip it */
                        break;
#endif
		    case SETRULE:
		    case PUTRULE:
		    	xskip((long) 8); /* two integers a and b */
		    	break;
		    case BOP: /* Beginning of page stuff */
		    	xskip((long) 11*4);
		    	break;
		    case EOP: /* End of the page! */
		    	return;
		    case SREFL:
		    case EREFL:
		    	break;
		    case RIGHT1:
		    case RIGHT2:
		    case RIGHT3:
		    case RIGHT4:
		    	xskip(ch - RIGHT1 + 1);
		    	break;
		    case W1:
		    case W2:
		    case W3:
		    case W4:
		    	xskip(ch - W0);
		    case W0:
		    	break;
		    case X1:
		    case X2:
		    case X3:
		    case X4:
		    	xskip(ch - X0);
		    case X0:
		    	break;
		    case DOWN1:
		    case DOWN2:
		    case DOWN3:
		    case DOWN4:
		    	xskip(ch - DOWN1 + 1);
		    	break;
		    case Y1:
		    case Y2:
		    case Y3:
		    case Y4:
		    	xskip(ch - Y0);
		    case Y0:
		    	break;
		    case Z1:
		    case Z2:
		    case Z3:
		    case Z4:
		    	xskip(ch - Z0);
		    case Z0:
		    	break;
		    case FNT1:
		    case FNT2:
		    case FNT3:
		    case FNT4:
		    	xskip(ch - FNT1 + 1);
		    	break;
		    case XXX1:
		    case XXX2:
		    case XXX3:
		    case XXX4: /* Only thing we pay attention to! */
		    	a = xnum(ch - XXX1 + 1);
		    	if (a > 0)
		    		htex_dospecial(a, pageno);
			break;
		    case FNTDEF1:
		    case FNTDEF2:
		    case FNTDEF3:
		    case FNTDEF4:
		    	xskip((long) (12 + ch - FNTDEF1 + 1));
		    	xskip((long) xone() + (long) xone());
		    	break;
		    case PRE:
		    case POST:
		    case POSTPOST:
		    default:
		    	break;
	    	}
	    }
	}
}

/* Do the same same stuff as special() in dvi_draw.c */
/* But only check HTeX specials */
void htex_dospecial(nbytes, pageno)
long nbytes;
int pageno;
{
	static char *cmd = NULL;
	static long cmdlen = -1;
	char *p;

	if (cmdlen < nbytes) {
		if (cmd) free(cmd);
		cmd = xmalloc((unsigned) nbytes+1);
		cmdlen = nbytes;
	}
	p = cmd;
	for (;;) {
		int i = currinf.end - currinf.pos;
		if (i > nbytes) i = nbytes;
		bcopy((_Xconst char *) currinf.pos, p, i);
		currinf.pos += i;
		p += i;
		nbytes -= i;
		if (nbytes == 0) break;
		(void) xxone();
		--(currinf.pos);
	}
	*p = '\0';
	p = cmd;
	while (isspace(*p)) ++p;
	checkHyperTeX(p, pageno);
}
#endif /* HTEX */
