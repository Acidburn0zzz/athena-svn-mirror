/* 
 * $Id: aklog.h,v 1.10 1999-08-13 22:26:04 danw Exp $
 *
 * Copyright 1990,1991 by the Massachusetts Institute of Technology
 * For distribution and copying rights, see the file "mit-copyright.h"
 */

#ifndef AKLOG__H
#define AKLOG__H

#include <afs/param.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <krb.h>
#include "linked_list.h"

typedef struct {
    int (*readlink)(const char *, char *, size_t);
    int (*isdir)(char *, unsigned char *);
    char *(*getcwd)(char *, size_t);
    int (*get_cred)(char *, char *, char *, CREDENTIALS *);
    int (*get_user_realm)(char *);
    void (*pstderr)(char *);
    void (*pstdout)(char *);
    void (*exitprog)(char);
} aklog_params;

void aklog(int, char *[], aklog_params *);
void aklog_init_params(aklog_params *);

#endif /* __AKLOG_H__ */
