/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-1999, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: lpq.h,v 1.1.1.3.2.1 2001-03-07 01:42:46 ghudson Exp $
 ***************************************************************************/



#ifndef _LPQ_H_
#define _LPQ_H_ 1
EXTERN int LP_mode;			/* LP mode */
EXTERN int Longformat;      /* Long format */
EXTERN int Displayformat;   /* Display format */
EXTERN int All_printers;    /* show all printers */
EXTERN int Status_line_count; /* number of status lines */
EXTERN int Clear_scr;       /* clear screen */
EXTERN int Interval;        /* display interval */

/* PROTOTYPES */
int main(int argc, char *argv[], char *envp[]);
void Show_status(char **argv);
int Read_status_info( char *host, int sock,
	int output, int timeout, int displayformat,
	int status_line_count );
int Remove_excess( struct line_list *l, int status_line_count, int output );
void Get_parms(int argc, char *argv[] );
void usage(void);

#endif
