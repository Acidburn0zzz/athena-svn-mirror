/*
 * auth.h:
 * This file is automatically generated; please do not edit it.
 */
/* Including auth.p.h at beginning of auth.h file. */

/* Copyright (C) 1990 Transarc Corporation - All rights reserved */
/*
 * P_R_P_Q_# (C) COPYRIGHT IBM CORPORATION 1987, 1988
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */

/* $Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sgi_65/include/afs/auth.h,v 1.1.1.2 1999-12-22 20:05:14 ghudson Exp $ */

#ifndef __AUTH_AFS_INCL_
#define	__AUTH_AFS_INCL_    1

#include <rx/rxkad.h>			/* to get ticket parameters/contents */

/* super-user pincipal used by servers when talking to other servers */
#define AUTH_SUPERUSER        "afs"

struct ktc_token {
    int32 startTime;
    int32 endTime;
    struct ktc_encryptionKey sessionKey;
    short kvno;  /* XXX UNALIGNED */
    int ticketLen;
    char ticket[MAXKTCTICKETLEN];
};

#ifdef AFS_NT40_ENV
extern int ktc_SetToken(
        struct ktc_principal *server,
        struct ktc_token *token,
        struct ktc_principal *client,
        int flags
);

extern int ktc_GetToken(
        struct ktc_principal *server,
        struct ktc_token *token,
        int tokenLen,
        struct ktc_principal *client
);

extern int ktc_ListTokens(
        int cellNum,
        int *cellNumP,
        struct ktc_principal *serverName
);

extern int ktc_ForgetToken(
        struct ktc_principal *server
);

extern int ktc_ForgetAllTokens(void);

/* Flags for the flag word sent along with a token */
#define PIOCTL_LOGON		0x1	/* invoked from integrated logon */

#endif /* AFS_NT40_ENV */

/* Flags for ktc_SetToken() */
#define AFS_SETTOK_SETPAG	0x1
#define AFS_SETTOK_LOGON	0x2	/* invoked from integrated logon */

#endif /* __AUTH_AFS_INCL_ */

/* End of prolog file auth.p.h. */

#define KTC_ERROR                                (11862784L)
#define KTC_TOOBIG                               (11862785L)
#define KTC_INVAL                                (11862786L)
#define KTC_NOENT                                (11862787L)
#define KTC_PIOCTLFAIL                           (11862788L)
#define KTC_NOPIOCTL                             (11862789L)
#define KTC_NOCELL                               (11862790L)
#define KTC_NOCM                                 (11862791L)
#define KTC_RPC                                  (11862792L)
#define KTC_NOCMRPC                              (11862793L)
extern void initialize_ktc_error_table ();
#define ERROR_TABLE_BASE_ktc (11862784L)

/* for compatibility with older versions... */
#define init_ktc_err_tbl initialize_ktc_error_table
#define ktc_err_base ERROR_TABLE_BASE_ktc
