/*
 * This file is part of the OLC On-Line Consulting System.
 * It contains definitions for the OLC daemon and the lumberjack program.
 *
 *      Chris VanHaren
 *	Lucien Van Elsen
 *      MIT Project Athena
 *
 * Copyright (C) 1990 by the Massachusetts Institute of Technology.
 * For copying and distribution information, see the file "mit-copyright.h".
 *
 *	$Source: /afs/dev.mit.edu/source/repository/athena/bin/olc/include/lumberjack.h,v $
 *	$Id: lumberjack.h,v 1.5 1991-01-15 17:50:29 lwvanels Exp $
 *	$Author: lwvanels $
 */

#include <mit-copyright.h>

#ifndef __lumberjack_h
#define __lumberjack_h __FILE__

#include <syslog.h>

#define DONE_DIR	"/usr/spool/olc/donelogs"

#define DSPIPE	"/usr/athena/dspipe" /* name of program to send off logs */
#define DSPAV0	"dspipe"	     /* argv[0] for DSPIPE */

#define PREFIXFILE "/usr/lib/olc/ds_prefix" /* File with log prefix to use */

#define LOCKFILE  "lockfile"	/* name of lockfile */
#define SIZE	256

#endif /* __lumberjack_h */
