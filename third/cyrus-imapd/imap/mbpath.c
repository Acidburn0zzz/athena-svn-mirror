/* mbpath.c -- help the sysadmin to find the path matching the mailbox
 *
 * $Id: mbpath.c,v 1.1.1.1 2002-10-13 18:05:05 ghudson Exp $
 * 
 * Copyright (c) 1999-2000 Carnegie Mellon University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any other legal
 *    details, please contact  
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* static char _rcsid[] = "$Id: mbpath.c,v 1.1.1.1 2002-10-13 18:05:05 ghudson Exp $"; */

#include <config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "acl.h"
#include "util.h"
#include "auth.h"
#include "prot.h"
#include "imparse.h"
#include "lock.h"
#include "imapconf.h"
#include "exitcodes.h"
#include "imap_err.h"
#include "mailbox.h"
#include "xmalloc.h"
#include "mboxlist.h"

extern int optind;
extern char *optarg;

extern int errno;

void
fatal(const char *s, int code) 
{
  if (s) {
    fprintf(stderr,"%s\n",s);
  }
  mboxlist_done();
  exit(code);
}

static int 
usage(void) {
  fprintf(stderr,"usage: cdmb [-C <alt_config>] [-q] <mailbox name>...\n");
  fprintf(stderr,"\t-q\tquietly drop any error messages\n");
  fatal(NULL, -1);
}

int
main(int argc, char **argv)
{
  char *path;
  int rc, i, quiet = 0, stop_on_error=0;
  char opt;
  char *alt_config = NULL;

  while ((opt = getopt(argc, argv, "C:qs")) != EOF) {
    switch(opt) {
    case 'C': /* alt config file */
      alt_config = optarg;
      break;
    case 'q':
      quiet = 1;
      break;
    case 's':
      stop_on_error = 1;
      break;

    default:
      usage();
    }
  }

  config_init(alt_config, "mbpath");

  mboxlist_init(0);
  mboxlist_open(NULL);

  for (i = optind; i < argc; i++) {
    (void)memset(&path, 0, sizeof(path));
    if ((rc = mboxlist_lookup(argv[i], &path, NULL, NULL)) == 0) {
      printf("%s\n", path);
    } else {
      if (!quiet && (rc == IMAP_MAILBOX_NONEXISTENT)) {
	fprintf(stderr, "Invalid mailbox name: %s\n", argv[i]);
      }
      if (stop_on_error) {
	if (quiet) {
	  fatal("", -1);
	} else {
	  fatal("Error in processing mailbox. Stopping\n", -1);
	}
      }
    }
  }

  mboxlist_close();
  mboxlist_done();

  exit(0);
}

/* $Header: /afs/dev.mit.edu/source/repository/third/cyrus-imapd/imap/mbpath.c,v 1.1.1.1 2002-10-13 18:05:05 ghudson Exp $ */

