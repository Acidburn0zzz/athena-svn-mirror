/*
 * $Source: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sgi_63/include/des_conf.h,v $
 * $Author: ghudson $
 * $Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sgi_63/include/des_conf.h,v 1.1.1.1 1997-10-16 14:40:09 ghudson Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * This file contains configuration information for the DES library
 * which is machine specific; currently, this file contains
 * configuration information for the vax, the "ibm032" (RT), and
 * the "PC8086" (IBM PC). 
 *
 * Note:  cross-compiled targets must appear BEFORE their corresponding
 * cross-compiler host.  Otherwise, both will be defined when running
 * the native compiler on the programs that construct cross-compiled
 * sources. 
 */

#include <afs/param.h>
#include <mit-cpyright.h>

/* Machine-type and OS-type based configuration */

#ifdef PC8086
#define IBMPC
#define BITS16
/* #define BIG */
#define CROSSMSDOS
#define LSBFIRST

#else

#ifdef vax
#define VAX
#ifndef	__STDC__		/* not Berkeley PCC */
#ifndef	__GNU__			/* ditto */
#ifndef	NOASM			/* are we doing C-only? */
#define VAXASM
#endif				/* NOASM */
#endif				/* __GNU__ */
#endif				/* __STDC__ */
#define BITS32
#define BIG
#define BSDUNIX
#define LSBFIRST

#else

#ifdef AFS_AOS_ENV
#define IBMWS
#define IBMWSASM
#define BITS32
#define BIG
#define BSDUNIX
#define MSBFIRST
#define MUSTALIGN

#else

#ifdef sun
#define BITS32
#define BIG
#define BSDUNIX
#define MSBFIRST

#else

#ifdef	AFS_AIX_ENV
/* WARNING: Identical to AOS's; some defines might have to change! */
#define IBMWS
#define IBMWSASM
#define BITS32
#define BIG
#define	BSDUNIX	/*Does it mean the default us S5? NO */
#define MSBFIRST
#define MUSTALIGN
#else
#ifdef multimax
#define BITS32
#define BIG
#define BSDUNIX
#define LSBFIRST
#else

Sorry, you lose.
Figure out what the machine looks like and fix this file to
include it.
#endif /* multimax */
#endif /* AFS_AIX_ENV */
#endif /* sun */
#endif /* ibm032 */
#endif /* vax */
#endif /* pc8086 */

/* Language configuration -- are we ANSI or are we Berkeley? */

#ifndef	__STDC__
#define	const
#endif
