/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZPeekPacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /afs/dev.mit.edu/source/repository/athena/lib/zephyr/lib/ZPeekPkt.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /afs/dev.mit.edu/source/repository/athena/lib/zephyr/lib/ZPeekPkt.c,v 1.3 1987-06-29 00:28:10 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZPeekPacket(buffer,buffer_len,ret_len,from)
	ZPacket_t	buffer;
	int		buffer_len;
	int		*ret_len;
	struct		sockaddr_in *from;
{
	int retval;
	
	if (ZGetFD() < 0)
		return (ZERR_NOPORT);

	if (!ZQLength())
		if ((retval = Z_ReadWait()) != ZERR_NONE)
			return (retval);

	if (buffer_len < __Q_Head->packet_len) {
		*ret_len = buffer_len;
		retval = ZERR_PKTLEN;
	}
	else {
		*ret_len = __Q_Head->packet_len;
		retval = ZERR_NONE;
	}
	
	bcopy(__Q_Head->packet,buffer,*ret_len);
	bcopy(&__Q_Head->from,from,sizeof(struct sockaddr_in));
	
	return (retval);
}
