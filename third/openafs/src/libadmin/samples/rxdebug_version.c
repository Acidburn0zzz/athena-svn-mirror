/*
 * Copyright 2000, International Business Machines Corporation and others.
 * All Rights Reserved.
 * 
 * This software has been released under the terms of the IBM Public
 * License.  For details, see the LICENSE file in the top-level source
 * directory or online at http://www.openafs.org/dl/license10.html
 */

/*
 * This file contains sample code for the rxstats interface 
 */

#include <afsconfig.h>
#include <afs/param.h>

RCSID("$Header: /afs/dev.mit.edu/source/repository/third/openafs/src/libadmin/samples/rxdebug_version.c,v 1.1.1.1 2002-01-31 21:49:09 zacheiss Exp $");

#ifdef AFS_NT40_ENV
#include <winsock2.h>
#include <pthread.h>
#endif
#include <afs/afs_Admin.h>
#include <afs/afs_clientAdmin.h>
#include <afs/afs_utilAdmin.h>

void Usage()
{
    fprintf(stderr,
	    "Usage: rxdebug_version <host> <port>\n");
    exit(1);
}

void ParseArgs(
    int argc,
    char *argv[],
    char **srvrName,
    long *srvrPort)
{
    char **argp = argv;

    if (!*(++argp))
	Usage();
    *srvrName = *(argp++);
    if (!*(argp))
	Usage();
    *srvrPort = strtol(*(argp++), NULL, 0);
    if (*srvrPort <= 0 || *srvrPort >= 65536)
	Usage();
    if (*(argp))
	Usage();
}

int main(int argc, char *argv[])
{
    int rc;
    afs_status_t st = 0;
    rxdebugHandle_p handle;
    char *srvrName;
    long srvrPort;
    rxdebugVersion_t version;

    ParseArgs(argc, argv, &srvrName, &srvrPort);

    rc = afsclient_Init(&st);
    if (!rc) {
	fprintf(stderr, "afsclient_Init, status %d\n", st);
	exit(1);
    }

    rc = afsclient_RXDebugOpenPort(srvrName, srvrPort, &handle, &st);
    if (!rc) {
	fprintf(stderr, "afsclient_RXDebugOpenPort, status %d\n", st);
	exit(1);
    }

    rc = util_RXDebugVersion(handle, version, &st);
    if (!rc) {
	fprintf(stderr, "util_RXDebugVersion, status %d\n", st);
	exit(1);
    }

    rc = afsclient_RXDebugClose(handle, &st);
    if (!rc) {
	fprintf(stderr, "afsclient_RXDebugClose, status %d\n", st);
	exit(1);
    }

    printf("\n");
    printf("AFS Version: '%s'\n", version);
    printf("\n");

    exit(0);
}
