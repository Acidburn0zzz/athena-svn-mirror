/*
 * Declarations for globally shared data used by Kerberos v5 library
 *
 * $Header: /afs/dev.mit.edu/source/repository/third/krb5/src/mac/libraries/Kerberos v5 Globals/Krb5GlobalsData.h,v 1.1.1.2 1999-12-26 03:33:48 ghudson Exp $
 */
 
#ifndef __Krb5GlobalsData_h__
#define __Krb5GlobalsData_h__

#include <Types.h>

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
#	pragma import on
#endif

extern	UInt32	gKerberos5GlobalsRefCount;
extern	char*	gKerberos5SystemDefaultCacheName;
extern	UInt32	gKerberos5SystemDefaultCacheNameModification;

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
#	pragma import reset
#endif

#endif /* __Krb5GlobalsData_h__ */