/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for the Client Manager subsystem of the Zephyr server.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /afs/dev.mit.edu/source/repository/athena/lib/zephyr/server/client.c,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#if !defined (lint) && !defined (SABER)
static const char rcsid_client_c[] =
    "$Header: /afs/dev.mit.edu/source/repository/athena/lib/zephyr/server/client.c,v 1.19 1990-11-13 17:05:00 raeburn Exp $";
#endif

/*
 * External functions:
 *
 * Code_t client_register(notice, who, client, server, wantdefaults)
 *	ZNotice_t *notice;
 *	struct sockaddr_in *who;
 *	ZClient_t **client; (RETURN)
 *	ZServerDesc_t *server;
 *	int wantdefaults;
 *
 * Code_t client_deregister(client, host, flush)
 *	ZClient_t *client;
 *	ZHostList_t *host;
 *	int flush;
 *
 * ZClient_t *client_which_client(who, notice)
 *	struct sockaddr_in *who;
 *	ZNotice_t *notice;
 *
 * void client_dump_clients(fp, clist)
 *	FILE *fp;
 *	ZClientList_t *clist;
 */

#include "zserver.h"
#include <sys/socket.h>

/*
 * register a client: allocate space, find or insert the address in the
 * 	server's list of hosts, initialize and insert the client into
 *	the host's list of clients.
 *
 * This routine assumes that the client has not been registered yet.
 * The caller should check by calling client_which_client
 */

Code_t
client_register(ZNotice_t *notice, struct sockaddr_in *who, register ZClient_t **client, ZServerDesc_t *server, int wantdefaults)
{
	register ZHostList_t *hlp = server->zs_hosts;
	register ZHostList_t *hlp2;
	register ZClientList_t *clist;
	int omask;

	/* chain the client's host onto this server's host list */

	if (!hlp) {			/* bad host list */
		syslog(LOG_ERR, "cl_register: bad server host list");
		abort();
	}

	if (!notice->z_port) {
	    /* must be a non-zero port # */
	    return(ZSRV_BADSUBPORT);
	}
	if (!(hlp2 = hostm_find_host(&who->sin_addr))) {
		/* not here */
		return(ZSRV_HNOTFOUND);
	}

	/* hlp2 is now pointing to the client's host's address struct */

	if (!hlp2->zh_clients) {
		return(EINVAL);
	}

	/* allocate a client struct */
	*client = new ZClient_t;
	if (!*client)
		return(ENOMEM);

	if (!(clist = (ZClientList_t *) xmalloc(sizeof(ZClientList_t)))) {
		delete *client;
		return(ENOMEM);
	}

	clist->q_forw = clist->q_back = clist;
	clist->zclt_client = *client;

	/* initialize the struct */
	bzero((caddr_t) &(*client)->zct_sin,
	      sizeof(struct sockaddr_in));
	bzero((caddr_t) &(*client)->zct_cblock,
	      sizeof((*client)->zct_cblock));
	(*client)->zct_sin.sin_addr.s_addr = who->sin_addr.s_addr;
	(*client)->zct_sin.sin_port = notice->z_port;
	(*client)->zct_sin.sin_family = AF_INET;
	(*client)->zct_subs = NULLZST;
	(*client)->zct_subs = NULLZST;
	(*client)->zct_principal = ZString (notice->z_sender);

	/* chain him in to the clients list in the host list*/

	omask = sigblock(sigmask(SIGFPE)); /* don't let db dumps start */
	xinsque(clist, hlp2->zh_clients);
	(void) sigsetmask(omask);

	if (!server->zs_dumping && wantdefaults)
		/* add default subscriptions only if this is not
		   resulting from a brain dump, AND this request
		   wants defaults */
		return(subscr_def_subs(*client));
	else
		return(ZERR_NONE);
}

/*
 * Deregister the client, freeing resources.  
 * Remove any packets in the nack queue, release subscriptions, release
 * locations, and dequeue him from the host.
 */

void
client_deregister(ZClient_t *client, ZHostList_t *host, int flush)
{
	ZClientList_t *clients;
	int omask = sigblock(sigmask(SIGFPE)); /* don't let db dumps start */

	/* release any not-acked packets in the rexmit queue */
	nack_release(client);

	/* release subscriptions */
	(void) subscr_cancel_client(client);

	if (flush)
		/* release locations if this is a punted client */
		(void) uloc_flush_client(&client->zct_sin);

	/* unthread and release this client */

	if (host->zh_clients)
		for (clients = host->zh_clients->q_forw;
		     clients != host->zh_clients;
		     clients = clients->q_forw)
			if (clients->zclt_client == client) {
				xremque(clients);
				delete client;
				xfree(clients);
				(void) sigsetmask(omask);
				return;
			}
	syslog(LOG_CRIT, "clt_dereg: clt not in host list");
	abort();
	/*NOTREACHED*/
}

/*
 * find the client which sent the notice
 */

ZClient_t *
client_which_client(struct sockaddr_in *who, ZNotice_t *notice)
{
	register ZHostList_t *hlt;
	register ZClientList_t *clients;

	if (!(hlt = hostm_find_host(&who->sin_addr))) {
#if 0
		zdbug((LOG_DEBUG,"cl_wh_clt: host not found"));
#endif
		return(NULLZCNT);
	}

	if (!hlt->zh_clients) {
#if 0
		zdbug((LOG_DEBUG,"cl_wh_clt: no clients"));
#endif
		return(NULLZCNT);
	}

	for (clients = hlt->zh_clients->q_forw;
	     clients != hlt->zh_clients;
	     clients = clients->q_forw)
		if (clients->zclt_client->zct_sin.sin_port == notice->z_port) {
			return(clients->zclt_client);
		}

#if 0
	zdbug((LOG_DEBUG, "cl_wh_clt: no port"));
#endif
	return(NULLZCNT);
}

/*
 * dump info about clients in this clist onto the fp.
 * assumed to be called with SIGFPE blocked
 * (true if called from signal handler)
 */

void
client_dump_clients(FILE *fp, ZClientList_t *clist)
{
	register ZClientList_t *ptr;

	for (ptr = clist->q_forw; ptr != clist; ptr = ptr->q_forw) {
		(void) fprintf(fp, "\t%d (%s):\n",
			       ntohs(ptr->zclt_client->zct_sin.sin_port),
			       ptr->zclt_client->zct_principal.value ());
		subscr_dump_subs(fp, ptr->zclt_client->zct_subs);
	}
	return;
}
