/*
 * The code contained in this file is from Neal Nuckolls's (Sun Internet
 * Engineering) DLPI "test" kit.
 */

#ifndef lint
static char *RCSid = "$Revision: 1.1.1.1 $";
#endif

/*
 * Common (shared) DLPI test routines.
 * Mostly pretty boring boilerplate sorta stuff.
 * These can be split into individual library routines later
 * but it's just convenient to keep them in a single file
 * while they're being developed.
 *
 * Not supported:
 *   Connection Oriented stuff
 *   QOS stuff
 */

#include "defs.h"

#if	defined(HAVE_DLPI)

#include	<sys/types.h>
#if	defined(HAVE_SYS_UIO_H)
#include	<sys/uio.h>
#endif	/* HAVE_SYS_UIO_H */
#include	<sys/stream.h>
#include	<sys/stropts.h>
#include	<sys/dlpi.h>
#include	<sys/signal.h>
#include	<stdio.h>
#include	<string.h>
#include	"dlpi.h"

dlattachreq(fd, ppa)
int	fd;
u_long	ppa;
{
	dl_attach_req_t	attach_req;
	struct	strbuf	ctl;
	int	flags;

	attach_req.dl_primitive = DL_ATTACH_REQ;
	attach_req.dl_ppa = ppa;

	ctl.maxlen = 0;
	ctl.len = sizeof (attach_req);
	ctl.buf = (char *) &attach_req;

	flags = 0;

	if (putmsg(fd, &ctl, (struct strbuf*) NULL, flags) < 0)
		SImsg(SIM_GERR, "dlattachreq:  putmsg");

	return 0;
}

dlphysaddrreq(fd, addrtype)
int	fd;
u_long	addrtype;
{
	dl_phys_addr_req_t	phys_addr_req;
	struct	strbuf	ctl;
	int	flags;

	phys_addr_req.dl_primitive = DL_PHYS_ADDR_REQ;
	phys_addr_req.dl_addr_type = addrtype;

	ctl.maxlen = 0;
	ctl.len = sizeof (phys_addr_req);
	ctl.buf = (char *) &phys_addr_req;

	flags = 0;

	if (putmsg(fd, &ctl, (struct strbuf*) NULL, flags) < 0)
		SImsg(SIM_GERR, "dlphysaddrreq:  putmsg");

	return 0;
}

dldetachreq(fd)
int	fd;
{
	dl_detach_req_t	detach_req;
	struct	strbuf	ctl;
	int	flags;

	detach_req.dl_primitive = DL_DETACH_REQ;

	ctl.maxlen = 0;
	ctl.len = sizeof (detach_req);
	ctl.buf = (char *) &detach_req;

	flags = 0;

	if (putmsg(fd, &ctl, (struct strbuf*) NULL, flags) < 0)
		SImsg(SIM_GERR, "dldetachreq:  putmsg");

	return 0;
}

dlbindreq(fd, sap, max_conind, service_mode, conn_mgmt, xidtest)
int	fd;
u_long	sap;
u_long	max_conind;
u_long	service_mode;
u_long	conn_mgmt;
u_long	xidtest;
{
	dl_bind_req_t	bind_req;
	struct	strbuf	ctl;
	int	flags;

	bind_req.dl_primitive = DL_BIND_REQ;
	bind_req.dl_sap = sap;
	bind_req.dl_max_conind = max_conind;
	bind_req.dl_service_mode = service_mode;
	bind_req.dl_conn_mgmt = conn_mgmt;
	bind_req.dl_xidtest_flg = xidtest;

	ctl.maxlen = 0;
	ctl.len = sizeof (bind_req);
	ctl.buf = (char *) &bind_req;

	flags = 0;

	if (putmsg(fd, &ctl, (struct strbuf*) NULL, flags) < 0)
		SImsg(SIM_GERR, "dlbindreq:  putmsg");

	return 0;
}

dlunbindreq(fd)
int	fd;
{
	dl_unbind_req_t	unbind_req;
	struct	strbuf	ctl;
	int	flags;

	unbind_req.dl_primitive = DL_UNBIND_REQ;

	ctl.maxlen = 0;
	ctl.len = sizeof (unbind_req);
	ctl.buf = (char *) &unbind_req;

	flags = 0;

	if (putmsg(fd, &ctl, (struct strbuf*) NULL, flags) < 0)
		SImsg(SIM_GERR, "dlunbindreq:  putmsg");

	return 0;
}

dlokack(fd, bufp)
int	fd;
char	*bufp;
{
	union	DL_primitives	*dlp;
	struct	strbuf	ctl;
	int	flags;

	ctl.maxlen = MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;

	strgetmsg(fd, &ctl, (struct strbuf*)NULL, &flags, "dlokack");

	dlp = (union DL_primitives *) ctl.buf;

	expecting(DL_OK_ACK, dlp);

	if (ctl.len < sizeof (dl_ok_ack_t))
	    SImsg(SIM_GERR, "dlokack:  response ctl.len too short:  %d", 
		  ctl.len);

	if (flags != RS_HIPRI)
	    SImsg(SIM_GERR, "dlokack:  DL_OK_ACK was not M_PCPROTO");

	if (ctl.len < sizeof (dl_ok_ack_t))
	    SImsg(SIM_GERR, "dlokack:  short response ctl.len:  %d", ctl.len);

	return 0;
}

dlbindack(fd, bufp)
int	fd;
char	*bufp;
{
	union	DL_primitives	*dlp;
	struct	strbuf	ctl;
	int	flags;

	ctl.maxlen = MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;

	strgetmsg(fd, &ctl, (struct strbuf*)NULL, &flags, "dlbindack");

	dlp = (union DL_primitives *) ctl.buf;

	expecting(DL_BIND_ACK, dlp);

	if (flags != RS_HIPRI)
	    SImsg(SIM_GERR, "dlbindack:  DL_OK_ACK was not M_PCPROTO");

	if (ctl.len < sizeof (dl_bind_ack_t))
	    SImsg(SIM_GERR, "dlbindack:  short response ctl.len:  %d", 
		  ctl.len);

	return 0;
}

dlphysaddrack(fd, bufp)
int	fd;
char	*bufp;
{
	union	DL_primitives	*dlp;
	struct	strbuf	ctl;
	int	flags;

	ctl.maxlen = MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;

	strgetmsg(fd, &ctl, (struct strbuf*)NULL, &flags, "dlphysaddrack");

	dlp = (union DL_primitives *) ctl.buf;

	expecting(DL_PHYS_ADDR_ACK, dlp);

	if (flags != RS_HIPRI)
	    SImsg(SIM_GERR, "dlbindack:  DL_OK_ACK was not M_PCPROTO");

	if (ctl.len < sizeof (dl_phys_addr_ack_t))
	    SImsg(SIM_GERR, "dlphysaddrack:  short response ctl.len:  %d", 
		  ctl.len);

	return 0;
}

static void
mysigalrm()
{
	SImsg(SIM_GERR, "sigalrm:  TIMEOUT");
	exit(1);
}

strgetmsg(fd, ctlp, datap, flagsp, caller)
int	fd;
struct	strbuf	*ctlp, *datap;
int	*flagsp;
char	*caller;
{
	int	rc;
	static	char	errmsg[80];

	/*
	 * Start timer.
	 */
	(void) signal(SIGALRM, mysigalrm);
	if (alarm((unsigned) MAXWAIT) != 0) {
		(void) snprintf(errmsg, sizeof(errmsg),  "%s:  alarm", caller);
		SImsg(SIM_GERR, errmsg);
	}

	/*
	 * Set flags argument and issue getmsg().
	 */
	*flagsp = 0;
	if ((rc = getmsg(fd, ctlp, datap, flagsp)) < 0) {
		(void) snprintf(errmsg, sizeof(errmsg),  "%s:  getmsg", caller);
		SImsg(SIM_GERR, errmsg);
	}

	/*
	 * Stop timer.
	 */
	if (alarm((unsigned) 0) != 0) {
		(void) snprintf(errmsg, sizeof(errmsg),  "%s:  alarm", caller);
		SImsg(SIM_GERR, errmsg);
	}

	/*
	 * Check for MOREDATA and/or MORECTL.
	 */
	if ((rc & (MORECTL | MOREDATA)) == (MORECTL | MOREDATA))
		SImsg(SIM_GERR, "%s:  MORECTL|MOREDATA", caller);
	if (rc & MORECTL)
		SImsg(SIM_GERR, "%s:  MORECTL", caller);
	if (rc & MOREDATA)
		SImsg(SIM_GERR, "%s:  MOREDATA", caller);

	/*
	 * Check for at least sizeof (long) control data portion.
	 */
	if (ctlp->len < sizeof (long))
		SImsg(SIM_GERR, 
		      "getmsg:  control portion length < sizeof (long):  %d", 
		      ctlp->len);

	return 0;
}

expecting(prim, dlp)
int	prim;
union	DL_primitives	*dlp;
{
	if (dlp->dl_primitive != (u_long)prim)
	    SImsg(SIM_GERR, "DLPI: expected %d got %d", prim,
			     dlp->dl_primitive);

	return 0;
}

/*
 * Return string.
 */
addrtostring(addr, length, s)
u_char	*addr;
u_long	length;
u_char	*s;
{
	int	i;

	for (i = 0; i < length; i++) {
		(void) sprintf((char*) s, "%x:", addr[i] & 0xff);
		s = s + strlen((char*)s);
	}
	if (length)
		*(--s) = '\0';

	return 0;
}

#endif	/* HAVE_DLPI */
