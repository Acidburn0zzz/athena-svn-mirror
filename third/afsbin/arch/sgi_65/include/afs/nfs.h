/* $Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sgi_65/include/afs/nfs.h,v 1.1.1.2 1999-12-22 20:05:27 ghudson Exp $ */
/* $Source: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sgi_65/include/afs/nfs.h,v $ */

#include <errno.h>

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidnfs = "$Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sgi_65/include/afs/nfs.h,v 1.1.1.2 1999-12-22 20:05:27 ghudson Exp $";
#endif

#ifndef	AFS_VOL_NFS_H
#define	AFS_VOL_NFS_H 1
/*
 * P_R_P_Q_# (C) COPYRIGHT IBM CORPORATION 1987
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/*

	System:		VICE-TWO
	Module:		nfs.h
	Institution:	The Information Technology Center, Carnegie-Mellon University

 */
#define private static
#ifndef NULL
#define NULL	0
#endif
#define TRUE	1
#define FALSE	0
typedef u_int32 bit32;	/* Unsigned, 32 bits */
typedef unsigned short bit16;	/* Unsigned, 16 bits */
typedef unsigned char byte;	/* Unsigned, 8 bits */

typedef bit32	Device;		/* Unix device number */
#ifndef	Error
#define	Error	bit32
#endif
#endif /* AFS_VOL_NFS_H */
