/**********************************************************************
 * File Exchange collect client
 *
 * $Author: brlewis $
 * $Source: /afs/dev.mit.edu/source/repository/athena/lib/neos/clients/collect.c,v $
 * $Header: /afs/dev.mit.edu/source/repository/athena/lib/neos/clients/collect.c,v 1.1 1990-08-14 15:01:29 brlewis Exp $
 *
 * Copyright (c) 1989, Massachusetts Institute of Technology
 **********************************************************************/

#ifndef lint
static char rcsid_collect_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/neos/clients/collect.c,v 1.1 1990-08-14 15:01:29 brlewis Exp $";
#endif /* lint */

#include <stdio.h>
#include <sys/time.h>
#include <fxcl.h>
#include <memory.h>
#include <ctype.h>
#include <strings.h>
#include <sys/errno.h>
/* #include "full_name.c"		/* because I am confused */

/*** Global variables ***/
Paper **paperv;
char *full_name();
char dump_err_context[1024];

/*
 * compar -- compares two papers, used by qsort
 */

compar(p1, p2)
     Paper **p1, **p2;
{
  int ret;

  ret = strcmp((*p1)->author, (*p2)->author);
  if (ret) return(ret);
  ret = strcmp((*p1)->filename, (*p2)->filename);
  if (ret) return(ret);
  ret = (int) (*p1)->modified.tv_sec - (*p2)->modified.tv_sec;
  if (ret) return(ret);
  return((int) ((*p1)->modified.tv_usec - (*p2)->modified.tv_usec));
}

/*
 * get_array -- fill in the array of papers to collect
 */

get_array(list)
     Paperlist list;
{
  Paperlist node;
  int i = 0, count = 0;

  /* Count papers in list */
  for(node = list; node; node = node->next) i++;

  if (i == 0) return(0);

  paperv = NewArray(Paper *, i);
  if (!paperv) return(0);

  /* Put papers into sorted array */
  for(node = list; node; node = node->next)
    paperv[count++] = &node->p;
  qsort((char *) paperv, count, sizeof(Paper *), compar);

  return(count);
}

/*
 * do_dump -- dumps papers into files
 */

long
do_dump(fxp, criterion, verbose, listonly, preserve)
     FX *fxp;
     Paper *criterion;
     int verbose, listonly, preserve;
{
  extern int errno;
  long code;
  Paperlist_res *plist;
  Paper taken;
  int count, i, warned_t = 0, warned_m = 0;
  char *s, *old_student = "";
  char filename[256];
  int kbytes = 0;               /* disk space used */
  int tilde = 0;		/* number for backup ("file~n~") */
  struct timeval tvp[2];	/* for changing mod time */

  /******** get list of papers from server ********/
  code = fx_list(fxp, criterion, &plist);
  if (code) {
    strcpy(dump_err_context, "while retrieving list");
    return(code);
  }

  count = get_array(plist->Paperlist_res_u.list);

  /******** deal with empty list ********/
  if (count == 0) {
    if (verbose) {
      if (criterion->author)
	printf("%s:\n", criterion->author);
      printf("No papers turned in\n");
    }
    return(0L);
  }

  /******** main loop through list ********/
  for (i=0; i<count; i++) {
    if (strcmp(paperv[i]->author, old_student)) {

      /******** deal with new student ********/
      if (verbose) {
	printf("%s:\n", full_name(paperv[i]->author));
      }
      kbytes++;
      if (!listonly)
	if (mkdir(paperv[i]->author, 0777))
	  if (errno != EEXIST) {
	    sprintf(dump_err_context, "while creating directory \"%s\"",
		    paperv[i]->author);
	    return ((long) errno);
	  }
      old_student = paperv[i]->author;
    }

    /******** create filename to dump into ********/
    (void) strcpy(filename, paperv[i]->author);
    (void) strcat(strcat(filename, "/"), paperv[i]->filename);
    /*** change spaces to underscores ***/
    for (s=filename; *s != '\0'; s++)
      if (*s == ' ') *s = '_';

    /******** rename old copies of the same file ********/
    if (count - i > 1) {
      if (strcmp(paperv[i]->filename, paperv[i+1]->filename) == 0
	  && strcmp(paperv[i]->author, paperv[i+1]->author) == 0) {
	sprintf(filename, "%s.~%d~", filename, ++tilde);
      } else tilde = 0;
    }

    if (verbose) {
      /******** print information about file ********/
      printf("%9d  %-16.16s  %s\n", paperv[i]->size,
	     ctime(&(paperv[i]->created.tv_sec)), filename);
    }
    kbytes += ((paperv[i]->size + 1023) >> 10);

    if (!listonly) {
      /******** retrieve file from server ********/
      code = fx_retrieve_file(fxp, paperv[i], filename);
      if (code) {
	sprintf(dump_err_context, "while retrieving \"%s\"", filename);
	return(code);
      }

      if (!preserve && !warned_t) {
	/******** mark file on server as TAKEN ********/
	paper_copy(paperv[i], &taken);
	taken.type = TAKEN;
	if ((code = fx_move(fxp, paperv[i], &taken)) && !warned_t) {
	  com_err("Warning", code, "-- files not marked TAKEN on server.", "");
	  warned_t = 1;
	}
      }

      /******** change accessed, updated times of local file ********/
      tvp[0].tv_sec = paperv[i]->modified.tv_sec;
      tvp[0].tv_usec = paperv[i]->modified.tv_usec;
      tvp[1].tv_sec = paperv[i]->created.tv_sec;
      tvp[1].tv_usec = paperv[i]->created.tv_usec;
      if (!warned_m && utimes(filename, tvp)) {
	com_err("Warning", code,
		"-- file modify times don't reflect turnin times.", "");
	warned_m = 1;
      }
    }
  }

  /******** clean up ********/
  if (verbose) printf("%d kbytes total\n", kbytes);
  fx_list_destroy(&plist);
  free((char *) paperv);
  return(0L);
}

/*
 * main dump procedure
 */

main(argc, argv)
  int argc;
  char *argv[];
{
  FX *fxp = NULL;
  long code;
  Paper p;
  int i;
  char *course;
  int verbose = 1, listonly = 0, specific = 0, preserve = 0;
  static char USAGE[] = "Usage: %s [-c course] [-d destdir] [options] [assignment] [student ...]\n";

  course = (char *) getenv("COURSE");
  paper_clear(&p);
  p.type = TURNEDIN;

  for (i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'c':
	course = argv[++i];
	if (fxp) {
	  fx_close(fxp);
	  fxp = NULL;
	}
	break;
      case 'q':
	verbose = 0;
	break;
      case 'v':
	verbose = 1;
	break;
      case 'e':
        p.type = EXCHANGE;
        break;
      case 'h':
        p.type = HANDOUT;
        break;
      case 'g':
        p.type = GRADED;
        break;
      case 't':
        p.type = TURNEDIN;
        break;
      case 'p':
	preserve = 1;
	break;
      case 'd':
	if (chdir(argv[++i])) {
	  perror(argv[i]);
	  exit(1);
	}
	break;
      case 'l':
	listonly = 1;
	break;
      default:
	fprintf(stderr, USAGE, argv[0]);
	break;
      }
      continue;
    }
    if (isdigit(argv[i][0])) {
      p.assignment = atoi(argv[i]);
      continue;
    }
    if (isalpha(argv[i][0])) {
      if (!fxp) {
	if (!course) {
	  fprintf(stderr, "%s: No course specified.\n", argv[0]);
	  exit(1);
	}
	fxp = fx_open(course, &code);
	if (!fxp) {
	  com_err(argv[0], code, "trying to open %s", course);
	  exit(1);
	}
	if (code)
	  fprintf(stderr, "%s: Warning: %s at %s\n", argv[0],
		  error_message(code), fxp->host);
      }
      specific = 1;
      p.author = argv[i];
      code = do_dump(fxp, &p, verbose, listonly, preserve);
      if (code) com_err(argv[0], code, "%s", dump_err_context);
    }
  }
  if (!specific)
    if (!fxp) {
      if (!course) {
	fprintf(stderr, "%s: No course specified.\n", argv[0]);
	exit(1);
      }
      fxp = fx_open(course, &code);
      if (!fxp) {
	com_err(argv[0], code, "trying to open %s", course);
	exit(1);
      }
      if (code)
	fprintf(stderr, "%s: Warning: %s at %s\n", argv[0],
		error_message(code), fxp->host);

      code = do_dump(fxp, &p, verbose, listonly, preserve);
      if (code) com_err(argv[0], code, "%s", dump_err_context);
    }

  fx_close(fxp);
  exit(0);
}
