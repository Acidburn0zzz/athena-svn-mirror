/*
 *
 * Status request for DISCUSS
 *
 * $Header: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/status.c,v 1.4 1986-12-07 00:39:56 rfrench Exp $
 * $Source: /afs/dev.mit.edu/source/repository/athena/bin/discuss/client/status.c,v $
 * $Locker:  $
 *
 * Copyright (C) 1986 by the MIT Student Information Processing Board
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  86/11/16  06:26:03  wesommer
 * Now prints out info similar to that printed when one goes to a
 * meeting.
 * 
 * Revision 1.2  86/10/29  10:29:40  srz
 * Part of global variable cleanup.
 * 
 * Revision 1.1  86/08/09  20:16:10  spook
 * Initial revision
 * 
 *
 */

#include <stdio.h>
#include "ss.h"
#include "interface.h"
#include "config.h"
#include "globals.h"

status(sci_idx, argc, argv)
	int sci_idx;
	int argc;
	char **argv;
{
	printf("Discuss version %s\n", CURRENT_VERSION);
	if (!dsc_public.attending) {
		printf("No current meeting\n");
		return;
	}
	printf("Attending %s (%s) meeting",
		       dsc_public.m_info.long_name, rindex(dsc_public.m_info.location, '/')+1);
	if (dsc_public.m_info.public_flag) printf(" (public)");
	if (acl_is_subset("c", dsc_public.m_info.access_modes)) 
		printf(" (You are a chairman)");
	if (!acl_is_subset("w", dsc_public.m_info.access_modes)) {
		if (!acl_is_subset("a", dsc_public.m_info.access_modes)) 
			printf(" (Read only)");
		else printf(" (Reply only)");
	} else if (!acl_is_subset("a", dsc_public.m_info.access_modes))
		printf(" (No replies)");
	printf(".\n");
	if (dsc_public.current == 0) {
		printf("No current transaction selected; %d highest.\n",
		       dsc_public.m_info.last);
		return;
	}
	printf("Transaction %d of %d.\n", dsc_public.current, dsc_public.m_info.last);
	return;
}
