/*
 * clients/kdestroy/kdestroy.c
 *
 * Copyright 1990 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 * 
 *
 * Destroy the contents of your credential cache.
 */

#include "krb5.h"
#include "com_err.h"
#include <string.h>
#include <stdio.h>
#ifdef KRB5_KRB4_COMPAT
#include <kerberosIV/krb.h>
#endif

extern int optind;
extern char *optarg;

void
main(argc, argv)
    int argc;
    char **argv;
{
    krb5_context kcontext;
    krb5_error_code retval;
    int c;
    krb5_ccache cache = NULL;
    char *cache_name = NULL;
    int code;
    int errflg=0;
    int quiet = 0;	
    int v4 = 1;
    
    retval = krb5_init_context(&kcontext);
    if (retval) {
	    com_err(argv[0], retval, "while initializing krb5");
	    exit(1);
    }

    if (strrchr(argv[0], '/'))
	argv[0] = strrchr(argv[0], '/')+1;

    while ((c = getopt(argc, argv, "qc:")) != EOF) {
	switch (c) {
	case 'q':
	    quiet = 1;
	    break;	
	case 'c':
	    v4 = 0;	/* Don't do krb4 kdestroy if cache name given. */
	    if (cache == NULL) {
		cache_name = optarg;
		
		code = krb5_cc_resolve (kcontext, cache_name, &cache);
		if (code != 0) {
		    com_err (argv[0], code, "while resolving %s", cache_name);
		    errflg++;
		}
	    } else {
		fprintf(stderr, "Only one -c option allowed\n");
		errflg++;
	    }
	    break;
	case '?':
	default:
	    errflg++;
	    break;
	}
    }

    if (optind != argc)
	errflg++;
    
    if (errflg) {
	fprintf(stderr, "Usage: %s [-q] [ -c cache-name ]\n", argv[0]);
	exit(2);
    }

    if (cache == NULL) {
	if (code = krb5_cc_default(kcontext, &cache)) {
	    com_err(argv[0], code, "while getting default ccache");
	    exit(1);
	}
    }

    code = krb5_cc_destroy (kcontext, cache);
    if (code != 0) {
	com_err (argv[0], code, "while destroying cache");
	if (quiet)
	    fprintf(stderr, "Ticket cache NOT destroyed!\n");
	else {
#ifdef __STDC__
	    fprintf(stderr, "Ticket cache \aNOT\a destroyed!\n");
#else
	    fprintf(stderr, "Ticket cache \007NOT\007 destroyed!\n");
#endif
	}
	errflg = 1;
    }
#ifdef KRB5_KRB4_COMPAT
    if (v4 && dest_tkt() != KSUCCESS) {
	if (quiet)
	    fprintf(stderr, "Kerberos 4 ticket file NOT destroyed!\n");
	else
	    fprintf(stderr, "Kerberos 4 ticket file \007NOT\007 destroyed!\n");
	errflg = 1;
    }
#endif
    exit (errflg);
}
