/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to rpmMD5Init, call rpmMD5Update as
 * needed on buffers full of bytes, and then call rpmMD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

/* parts of this file are :
 * Written March 1993 by Branko Lankester
 * Modified June 1993 by Colin Plumb for altered md5.c.
 * Modified October 1995 by Erik Troan for RPM
 */


#ifndef TRILOBITE_MD5_TOOLS_H
#define TRILOBITE_MD5_TOOLS_H

#include <glib.h>

typedef struct {
	guint32 buf[4];
	guint32 bits[2];
	guchar in[64];
	int doByteReverse;
} MD5Context ;


void trilobite_md5_get_digest (const char *buffer, int buffer_size, guchar digest[16]);
void trilobite_md5_get_digest_from_file (const char *filename, guchar digest[16]);

/* Converts a 32 char hex'ified md5 to a 16 byte md5 digest */
void trilobite_md5_get_digest_from_md5_string (const char *md5string, guchar pmd5[16]);

/* Converts a 16 byte md5 digest to a 32 char hex'ified */
const char *trilobite_md5_get_string_from_md5_digest (const guchar md5[16]);

#endif /* TRILOBITE_MD5_TOOLS_H */
