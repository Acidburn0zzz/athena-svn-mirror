/*
 * $Id: errors.c,v 1.5 1999-01-22 23:08:58 ghudson Exp $
 *
 * This program is part of a package including delete, undelete,
 * lsdel, expunge and purge.  The software suite is meant as a
 * replacement for rm which allows for file recovery.
 * 
 * Copyright (c) 1989 by the Massachusetts Institute of Technology.
 * For copying and distribution information, see the file "mit-copying.h."
 */

#include <com_err.h>
#include <stdio.h>
#include "delete_errs.h"
#include "mit-copying.h"
#include "util.h"

char *whoami;
int error_reported = 1;
int error_occurred = 0;
int report_errors = 1;
int error_code = 0;

/*
 * Proper use of this procedure requires strict adherance to the way
 * it is supposed to be used by all procedures in a program.  Whenever
 * there is an error, set_error must be called with the error value.
 * Then, either the procedure that detects the error must call
 * error(), or it must pass the error up to its parent for the parent
 * to report.
 */


void error(str)
char *str;
{
     if (report_errors && (! error_reported)) {
	  if (*str)
	       fprintf(stderr, "%s: %s: %s\n", whoami, str,
		       error_message(error_code));
	  else
	       fprintf(stderr, "%s: %s\n", whoami, error_message(error_code));
     }
     error_reported = 1;
}
