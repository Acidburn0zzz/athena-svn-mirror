/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dumping server state between servers.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /afs/dev.mit.edu/source/repository/athena/lib/zephyr/server/bdump.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
#ifndef SABER
static char rcsid_brain_dump_c[] = "$Header: /afs/dev.mit.edu/source/repository/athena/lib/zephyr/server/bdump.c,v 1.3 1987-07-02 19:10:19 jtkohl Exp $";
#endif SABER
#endif lint

#include "zserver.h"

/*ARGSUSED*/
void
get_brain_dump(who)
struct sockaddr_in *who;
{
}

/*ARGSUSED*/
void
send_brain_dump(who)
struct sockaddr_in *who;
{
}
