/* DES-encrypted-stream implementation for MIT Kerberos.
 * Written by Ken Raeburn (Raeburn@Cygnus.COM), based on algorithms
 * in the original MIT Kerberos code.
 * Copyright (C) 1991, 1992, 1994 by Cygnus Support.
 *
 * Permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation.
 * Cygnus Support makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * kstream_des.c
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef POSIX
#include <stdlib.h>
#endif
#include <des.h>
#include "kstream_int.h"

#ifdef USE_RAND48
#define random	lrand48
#define srandom	srand48
#endif

#ifdef _WINDOWS
#define STATIC
#define random	rand
#define srandom	srand
#else
#define STATIC static
#endif

typedef struct {
  union {
    long align_me;
    double align_me_harder;
    Key_schedule sched;
  } u;
  des_cblock ivec;
} kstream_des_init_block;

typedef struct {
  kstream_des_init_block x;
  int no_right_justify;
  int protect_rlogin_oob;
  char *buf1, *buf2;
  size_t len1, len2;
} priv;

typedef struct kstream_data_block ksdb;

#ifdef _WINDOWS
STATIC int INTERFACE encrypt (ksdb *outp, ksdb *inp, kstream k);
STATIC int INTERFACE decrypt (ksdb *outp, ksdb *inp, kstream k);
STATIC int INTERFACE rcp_init (kstream k, void *data);
STATIC int INTERFACE rlogin_init (kstream k, void *data);
STATIC void INTERFACE destroy (kstream k);
#endif

/* The data stream consists of four bytes representing a net-order
   integer, followed by enough data to produce that many
   cleartext bytes.  This means the size of that data must be rounded
   up to a multiple of 8, even though the cleartext may only be one
   byte.  For blocks of less than eight bytes, most software (well,
   exactly half of the two already-existing programs, and all of the
   ones we write using this library :-) pads on the *left* with
   random values.

   Some existing software that we have to be compatible with may send
   blocks of data that decrypt to more than 8 bytes, but not an exact
   multiple.  In that case, the padding is on the right, as would be
   considered "normal".  This software currently will not generate such
   a sequence, but a future version could.  Don't break compatibility
   with that mode.  */

#ifdef sun
static kstream_ptr losing_realloc (old_ptr, new_size)
     kstream_ptr old_ptr;
     size_t new_size;
{
  return (old_ptr
	  ? (kstream_ptr) realloc (old_ptr, new_size)
	  : (kstream_ptr) malloc (new_size));
}
#define realloc losing_realloc
#endif

/* Do the actual encryption work.  This routine will handle chunks of
   any size up to 16 bytes, or any multiple of 8 over that.  It makes
   the padding a little easier to write it this way.  Handling sizes
   between 8 and 16 is an annoyance, but rlogin actually relies on being
   able to send 12 bytes in one chunk.  Bleah!  */
static void
do_encrypt (out, inp, p)
     ksdb FAR *out;
     ksdb FAR *inp;
     priv FAR *p;
{
  union {
    char buf[16];
    int junk[16 / sizeof (int)];
  } u;
  ksdb in;
  char *ptr;
  static int seeded;

  if (!seeded) {
    srandom ((int) time ((time_t *) 0));
    seeded = 1;
  }

  in = *inp;
  if (in.length < 8)
    {
      if (! p->no_right_justify)
	{
	  u.junk[0] = random ();
	  memcpy (u.buf + 8 - in.length, in.ptr, in.length);
	}
      else
	{
	  u.junk[(sizeof (u.junk[0]) + 7) / sizeof (u.junk[0]) - 1] = random ();
	  memcpy (u.buf, in.ptr, in.length);
	}
      in.ptr = u.buf;
      in.length = 8;
    }
  else if (in.length == 8)
    {
      memcpy (u.buf, in.ptr, 8);
      in.ptr = u.buf;
    }
  else if (in.length < sizeof (u.buf))
    {
      if (in.length % 8 == 0)
	abort ();
      u.junk[(sizeof (u.junk) / sizeof (u.junk[0])) - 1] = random ();
      memcpy (u.buf, in.ptr, in.length);
      in.ptr = u.buf;
    }
  else if (in.length % 8 != 0)
    abort ();
  {
    unsigned long x;
    x = inp->length;		/* not in.length! */
    ptr = (char *) out->ptr;
    ptr[3] = x & 0xff; x >>= 8;
    ptr[2] = x & 0xff; x >>= 8;
    ptr[1] = x & 0xff; x >>= 8;
    ptr[0] = x & 0xff; x >>= 8;
    ptr += 4;
    if (x)
      abort ();
  }
  des_pcbc_encrypt ((des_cblock *)in.ptr, (des_cblock *)ptr, in.length,
		    p->x.u.sched, &p->x.ivec, ENCRYPT);
  out->ptr = ptr + ((in.length + 7) & ~7);
}

STATIC int INTERFACE
encrypt (outp, inp, k)
     ksdb *outp;
     ksdb *inp;
     kstream k;
{
  const int small_block_size = 16;
  priv *p = (priv *) k->data;

  if (inp->length > small_block_size && inp->length % 8 != 0)
    {
      /* do two */
      ksdb in, out;
      size_t sz;

      in.ptr = inp->ptr;
      in.length = inp->length & ~7;
      sz = in.length + 4;	/* first block */
      sz += 8 + 4; /* second block */
      outp->length = sz;
      out.ptr = outp->ptr = p->buf1 = (kstream_ptr) realloc (p->buf1, sz);
      out.length = sz;
      assert (out.ptr != 0 || out.length == 0);
      do_encrypt (&out, &in, p);
      in.ptr = (char *) in.ptr + in.length;
      in.length = inp->length - in.length;
      do_encrypt (&out, &in, p);
      return inp->length;
    }
  else
    {
      size_t sz = (inp->length + 7) & ~7;
      sz += 4;
      outp->length = sz;
      outp->ptr = p->buf1 = (kstream_ptr) realloc (p->buf1, sz);
      assert (outp->ptr != 0 || outp->length == 0);
      do_encrypt (outp, inp, p);
      outp->ptr = p->buf1;
      return inp->length;
    }
}

int _kstream_des_debug_OOB = 0;

STATIC int INTERFACE
decrypt (outp, inp, k)
     ksdb *outp;
     ksdb *inp;
     kstream k;
{
  char *ptr = inp->ptr;
  unsigned long x = 0;
  int error_count = 0;
  size_t sz;
  priv *p = (priv *) k->data;

  if(inp->length < 1) return -12; /* make sure we have at least one byte */
  if (p->protect_rlogin_oob) {
    /* here's where we handle an attack. The first byte ends up being the
       highest. If it's not zero, skip it. If it is zero, we can't detect it,
       and we still lose... */
    x = *ptr & 0xff;		/* get the first char */
    while (x) {

#ifndef _WINDOWS
      if(_kstream_des_debug_OOB)
	fprintf(stderr,"BAD BYTE %02x\n\r", x);
#endif

      error_count++;		/* count the bad byte */
      ptr++;			/* and skip it */
      if(inp->length == error_count) {
	return -12;		/* we've used up all of the input */
      }
      x = *ptr & 0xff;		/* get the next potentially first char */
    }
    ptr++;
  } else {
    x <<= 8; x += *ptr++ & 0xff;
  }


  /* If we've got four bytes, we can at least determine the correct
     amount that still needs to be read.  If not, we can assume a
     minimum of 12 good bytes (4-byte length plus one 8-byte block)
     and we know how many of the ones we've got are bad. */
  if (inp->length < 4 + error_count)
    return inp->length - error_count - 12;

/*  x <<= 8; x += *ptr++ & 0xff; */ /* x already has first byte loaded */
  x <<= 8; x += *ptr++ & 0xff;
  x <<= 8; x += *ptr++ & 0xff;
  x <<= 8; x += *ptr++ & 0xff;
  sz = (x + 7) & ~7;
  if (inp->length < sz + 4 + error_count)
    return - (sz + 4 + error_count - inp->length);
  assert (sz <= sizeof (k->in_crypt.data));

  if (p->buf1)
    p->buf1 = (kstream_ptr) realloc (p->buf1, sz);
  else
    p->buf1 = (kstream_ptr) malloc (sz);
  assert (p->buf1 != 0 || sz == 0);
  outp->ptr = p->buf1;
  outp->length = x;
  pcbc_encrypt ((des_cblock *)ptr, (des_cblock *)outp->ptr, sz, p->x.u.sched, &p->x.ivec,
		DECRYPT);
  if (p->no_right_justify == 0
      && x < 8)
    outp->ptr = p->buf1 + 8 - x;
  return sz + 4 + error_count;
}

static int
init (k, data)
     kstream k;
     void *data;
{
  priv *p;

  p = (priv *) malloc (sizeof (priv));
  k->data = (kstream_ptr) p;
  if (!p)
    return errno;
  p->buf1 = p->buf2 = 0;
  p->len1 = p->len2 = 0;
  p->no_right_justify = 0;
  p->protect_rlogin_oob = 0;
  p->x = * (kstream_des_init_block *) data;
  return 0;
}

STATIC int INTERFACE
rcp_init (k, data)
     kstream k;
     void *data;
{
  int x = init (k, data);
  ((priv *)(k->data))->no_right_justify = 1;
  return x;
}

STATIC int INTERFACE
rlogin_init (k, data)
     kstream k;
     void *data;
{
  int x = init (k, data);
  ((priv *)(k->data))->protect_rlogin_oob = 1;
  return x;
}

STATIC void INTERFACE
destroy (k)
     kstream k;
{
  priv *p = (priv *) k->data;
  if (p->buf1)
    free (p->buf1);
  memset (p, '\\', sizeof (*p)); /* scribble to make sure it's gone */
  free (p);
  k->data = 0;
}

static const struct kstream_crypt_ctl_block kstream_des_ccb = {
  encrypt, decrypt, rlogin_init, destroy
};

static const struct kstream_crypt_ctl_block kstream_des_rcp_ccb = {
  encrypt, decrypt, rcp_init, destroy
};

kstream INTERFACE
kstream_create_rlogin_from_fd (fd, P_sched, ivec)
     int fd;
     kstream_ptr P_sched;
     des_cblock (*ivec);
{
  Key_schedule *sched = (Key_schedule *) P_sched;
  kstream_des_init_block x;
  kstream k;
  memcpy (&x.u.sched, sched, sizeof (Key_schedule));
  memcpy (&x.ivec, ivec, sizeof (des_cblock));
  k = kstream_create_from_fd (fd, &kstream_des_ccb, &x);
  memset (&x, '\\', sizeof (x));
  return k;
}

kstream	INTERFACE
kstream_create_rcp_from_fd (fd, P_sched, ivec)
     int fd;
     kstream_ptr P_sched;
     des_cblock (*ivec);
{
  Key_schedule *sched = (Key_schedule *) P_sched;
  kstream_des_init_block x;
  kstream k;
  memcpy (&x.u.sched, sched, sizeof (Key_schedule));
  memcpy (&x.ivec, ivec, sizeof (des_cblock));
  k = kstream_create_from_fd (fd, &kstream_des_rcp_ccb, &x);
  memset (&x, '\\', sizeof (x));
  return k;
}
