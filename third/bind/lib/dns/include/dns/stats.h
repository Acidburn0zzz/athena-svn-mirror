/*
 * Copyright (C) 2000, 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: stats.h,v 1.1.1.1 2001-10-22 13:08:28 ghudson Exp $ */

#ifndef DNS_STATS_H
#define DNS_STATS_H 1

#include <dns/types.h>

/*
 * Query statistics counter types.
 */
typedef enum {
	dns_statscounter_success = 0,    /* Successful lookup */
	dns_statscounter_referral = 1,   /* Referral result */
	dns_statscounter_nxrrset = 2,    /* NXRRSET result */
	dns_statscounter_nxdomain = 3,   /* NXDOMAIN result */
	dns_statscounter_recursion = 4,  /* Recursion was used */
	dns_statscounter_failure = 5     /* Some other failure */
} dns_statscounter_t;

#define DNS_STATS_NCOUNTERS 6

extern const char *dns_statscounter_names[];

isc_result_t
dns_stats_alloccounters(isc_mem_t *mctx, isc_uint64_t **ctrp);
/*
 * Allocate an array of query statistics counters from the memory
 * context 'mctx'.
 */

void
dns_stats_freecounters(isc_mem_t *mctx, isc_uint64_t **ctrp);
/*
 * Free an array of query statistics counters allocated from the memory
 * context 'mctx'.
 */

unsigned int
dns_stats_ncounters(void);
/*
 * Return the number of query statistics counters.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_STATS_H */
