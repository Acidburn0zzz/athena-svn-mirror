/*
 * Copyright (C)  1998  Transarc Corporation.  All rights reserved.
 *
 * $Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/i386_linux22/include/afs/pthread_glock.h,v 1.1.1.2 2000-04-12 18:45:45 ghudson Exp $
 */

#ifndef _AFS_PTHREAD_GLOCK_H_
#define _AFS_PTHREAD_GLOCK_H_

#ifdef AFS_PTHREAD_ENV
#include <pthread.h>
#include <assert.h>

typedef struct {
    pthread_mutex_t mut;
    pthread_t owner;
    unsigned int locked;
    unsigned int times_inside;
} pthread_recursive_mutex_t, *pthread_recursive_mutex_p;

#if defined(AFS_NT40_ENV) && defined(AFS_PTHREAD_ENV)
#ifndef AFS_GRMUTEX_DECLSPEC
#define AFS_GRMUTEX_DECLSPEC __declspec(dllimport) extern 
#endif
#else
#define AFS_GRMUTEX_DECLSPEC extern
#endif

AFS_GRMUTEX_DECLSPEC pthread_recursive_mutex_t grmutex;

extern int pthread_recursive_mutex_lock(pthread_recursive_mutex_p);
extern int pthread_recursive_mutex_unlock(pthread_recursive_mutex_p);

#define LOCK_GLOBAL_MUTEX \
    assert(pthread_recursive_mutex_lock(&grmutex)==0);

#define UNLOCK_GLOBAL_MUTEX \
    assert(pthread_recursive_mutex_unlock(&grmutex)==0);

#else

#define LOCK_GLOBAL_MUTEX
#define UNLOCK_GLOBAL_MUTEX

#endif /* AFS_PTHREAD_ENV */

#endif /* _AFS_PTHREAD_GLOCK_H_ */
