/*
 * kautils.h:
 * This file is automatically generated; please do not edit it.
 */
/* Including kautils.p.h at beginning of kautils.h file. */

/* Copyright (C) 1990, 1989 Transarc Corporation - All rights reserved */
/*
 * P_R_P_Q_# (C) COPYRIGHT IBM CORPORATION 1988, 1989
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */

/* $Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/i386_linux22/include/afs/kautils.h,v 1.1 1999-04-09 21:01:47 tb Exp $ */

/* $Log: not supported by cvs2svn $
 * Revision 2.2  1990/09/27  13:51:37  ota
 * Declare (char *) returning function ka_timestr().
 * Cleanups.
 *
 * Revision 2.1  90/08/07  19:11:51  hines
 * Start with clean version to sync test and dev trees.
 * */
/* See RCS log for older history. */

#ifndef __KAUTILS__
#define __KAUTILS__

#if !defined(UKERNEL)
#include <des.h>
#include <afs/auth.h>

#ifndef KAMAJORVERSION
    /* just to be on the safe side, get these two first */
#include <sys/types.h>
#include <rx/xdr.h>

    /* get installed .h file only if not included already from local dir */
#ifndef _RXGEN_KAUTH_
#include <afs/kauth.h>
#endif

#endif

#include <ubik.h>
#include <afs/cellconfig.h>

#else
#include "../afs/ubik.h"
#include "../afs/auth.h"
#include "../afs/cellconfig.h"
#endif /* !defined(UKERNEL) */


#define KA_TIMESTR_LEN 30

extern char *ucstring();
extern char *lcstring();

/*
 * Public function prototypes
 */

extern int32 ka_GetAuthToken (
  char *name,
  char *instance,
  char *cell,
  struct ktc_encryptionKey *key,
  int32  lifetime,
  int32  *pwexpires
);

extern int32 ka_GetServerToken (
  char *name,
  char *instance,
  char *cell,
  Date  lifetime,
  struct ktc_token *token,
  int   new,
  int   dosetpag
);

extern int32 ka_GetAdminToken (
  char *name,
  char *instance,
  char *cell,
  struct ktc_encryptionKey *key,
  int32  lifetime,
  struct ktc_token *token,
  int   new
);

extern int32 ka_VerifyUserToken(
  char *name,
  char *instance,
  char *cell,
  struct ktc_encryptionKey *key
);

extern void ka_ExplicitCell (
  char *cell,
  int32 serverList[]
);

extern int32 ka_GetServers (
  char *cell,
  struct afsconf_cell *cellinfo
);

extern int32 ka_GetSecurity (
  int   service,
  struct ktc_token *token,
  struct rx_securityClass **scP,
  int  *siP
);

extern int32 ka_SingleServerConn (
  char *cell,
  char *server,
  int   service,
  struct ktc_token *token,
  struct ubik_client **conn
);

extern int32 ka_AuthSpecificServersConn (
  int                  service,
  struct ktc_token    *token,
  struct afsconf_cell *cellinfo,
  struct ubik_client **conn
);

extern int32 ka_AuthServerConn (
  char                *cell,
  int                  service,
  struct ktc_token    *token,
  struct ubik_client **conn
);

extern int32 ka_Authenticate (
  char *name,
  char *instance,
  char *cell,
  struct ubik_client *conn,
  int   service,
  struct ktc_encryptionKey *key,
  Date  start,
  Date end,
  struct ktc_token *token,
  int32 *pwexpires
);

extern int32 ka_GetToken (
  char               *name,
  char               *instance,
  char               *cell,
  char               *cname,
  char               *cinst,
  struct ubik_client *conn,
  Date                start,
  Date                end,
  struct ktc_token   *auth_token,
  char               *auth_domain,
  struct ktc_token   *token
);

extern int32 ka_ChangePassword (
  char               *name,
  char               *instance,
  struct ubik_client *conn,
  struct ktc_encryptionKey *oldkey,
  struct ktc_encryptionKey *newkey
);

extern void ka_StringToKey (
  char          *str,
  char          *cell,
  struct ktc_encryptionKey *key
);

extern int32 ka_ReadPassword (
  char          *prompt,
  int            verify,
  char          *cell,
  struct ktc_encryptionKey *key
);

extern int32 ka_ParseLoginName (
  char *login,
  char  name[MAXKTCNAMELEN],
  char  inst[MAXKTCNAMELEN],
  char  cell[MAXKTCREALMLEN]
);

extern int32 ka_Init(
  int flags
);

extern int ka_CellConfig (
  char *dir
);

extern char *ka_LocalCell (
  void
);

extern int ka_ExpandCell (
  char *cell,
  char *fullCell,
  int  *alocal
);

extern int ka_CellToRealm (
  char *cell,
  char *realm,
  int  *local
);

extern void ka_PrintUserID (
  char *prefix,
  char *name,
  char *instance,
  char *postfix
);

extern void ka_PrintBytes (
  char bs[],
  int  bl
);

extern int ka_ConvertBytes (
  char *ascii,
  int   alen,
  char  bs[],
  int   bl
);

extern int ka_ReadBytes (
  char *ascii,
  char *binary,
  int   blen
);

extern int umin (
  u_int32 a,
  u_int32 b
);

extern int32 ka_KeyCheckSum (
  char *key,
  u_int32 *cksumP
);

extern int ka_KeyIsZero(
  register char *akey,
  register int alen
);

extern void ka_timestr (
  int32 time,
  char *tstr,
  int32 tlen
);

extern int32 ka_GetAFSTicket (
  char *name,
  char *instance,
  char *realm,
  Date lifetime,
  int32 flags
);

extern int32 ka_UserAuthenticateGeneral (
  int32 flags,
  char *name,
  char *instance,
  char *realm,
  char *password,
  Date lifetime,
  int32 *password_expires,
  int32 spare2,
  char **reasonP
);

extern int32 ka_UserAuthenticate (
  char *name,
  char *instance,
  char *realm,
  char *password,
  int   doSetPAG,
  char **reasonP
);

extern int32 ka_UserReadPassword (
  char *prompt,
  char *password,
  int   plen,
  char **reasonP
);

extern int32 ka_VerifyUserPassword(
     int32 version,
     char *name,
     char *instance,
     char *realm,
     char *password,
     int spare,
     char **reasonP
);
#define KA_USERAUTH_VERSION 1
#define KA_USERAUTH_VERSION_MASK	0x00ffff
#define KA_USERAUTH_DOSETPAG		0x010000
#define KA_USERAUTH_DOSETPAG2		0x020000
#define KA_USERAUTH_ONLY_VERIFY		0x040000
#define ka_UserAuthenticate(n,i,r,p,d,rP) \
    ka_UserAuthenticateGeneral \
        (KA_USERAUTH_VERSION + ((d) ? KA_USERAUTH_DOSETPAG : 0), \
	 n,i,r,p, /*lifetime*/0, /*spare1,2*/0,0, rP)
#define ka_UserAuthenticateLife(f,n,i,r,p,l,rP) \
    ka_UserAuthenticateGeneral \
        (KA_USERAUTH_VERSION + (f), n,i,r,p,l, /*spare1,2*/0,0, rP)

extern int32 KAM_CreateUser();
extern int32 KAM_DeleteUser();
extern int32 KAA_ChangePassword();
extern int32 KAM_SetPassword();
extern int32 KAA_Authenticate(), KAA_AuthenticateV2();
extern int32 KAT_GetTicket();
extern int32 KAM_SetFields();
#define KA_REUSEPW 1
#define KA_NOREUSEPW 2
#define KA_ISLOCKED 4                

extern int32 KAM_GetEntry();
extern int32 KAM_ListEntry();
extern int32 KAM_GetStats();
extern int32 KAM_GetPassword();
extern int32 KAM_GetRandomKey();
extern int32 KAM_Debug();
extern int32 KAM_Unlock();
extern int32 KAM_LockStatus();

#define KA_AUTHENTICATION_SERVICE 731
#define KA_TICKET_GRANTING_SERVICE 732
#define KA_MAINTENANCE_SERVICE 733

#define RX_SCINDEX_NULL	0		/* No security */
#define RX_SCINDEX_VAB 	1		/* vice tokens, with bcrypt */
#define RX_SCINDEX_KAD	2		/* Kerberos/DES */

#define KA_TGS_NAME "krbtgt"
	/* realm is TGS instance */
#define KA_ADMIN_NAME "AuthServer"
#define KA_ADMIN_INST "Admin"

#define KA_LABELSIZE 4
#define KA_GETTGT_REQ_LABEL "gTGS"
#define KA_GETTGT_ANS_LABEL "tgsT"
#define KA_GETADM_REQ_LABEL "gADM"
#define KA_GETADM_ANS_LABEL "admT"
#define KA_CPW_REQ_LABEL "CPWl"
#define KA_CPW_ANS_LABEL "Pass"
#define KA_GETTICKET_ANS_LABEL "gtkt"

struct ka_gettgtRequest {			/* format of request */
    Date time;				/* time of request */
    char label[KA_LABELSIZE];		/* label to verify correct decrypt */
};

/* old interface: see ka_ticketAnswer instead */
struct ka_gettgtAnswer {		/* format of response */
    Date time;				/* the time of the request plus one */
    struct ktc_encryptionKey
         sessionkey;			/* the session key in the ticket */
    int32 kvno;				/* version # of tkt encrypting key */
    int32 ticket_len;			/* the ticket's length */
    char ticket[MAXKTCTICKETLEN];	/* the ticket itself (no padding) */
    char label[KA_LABELSIZE];		/* label to verify correct decrypt */
};

struct ka_ticketAnswer {		/* format of response */
    int32 cksum;				/* function to be defined */
    Date challenge;			/* the time of the request plus one */
    struct ktc_encryptionKey
         sessionKey;			/* the session key in the ticket */
    Date startTime;
    Date endTime;
    int32 kvno;				/* version of ticket encrypting key */
    int32 ticketLen;			/* the ticket's length */
    char name[MAXKTCNAMELEN];
    char instance[MAXKTCNAMELEN];
    char cell[MAXKTCNAMELEN];
    char sname[MAXKTCNAMELEN];
    char sinstance[MAXKTCNAMELEN];
    char ticket[MAXKTCTICKETLEN];	/* the ticket (no extra chars) */
    char label[KA_LABELSIZE];		/* for detecting decryption errors */
};

struct ka_cpwRequest {			/* format of request */
    Date time;				/* time of request */
    struct ktc_encryptionKey
	 newpw;				/* new key */
    int32 kvno;				/* version number of key */
    int32 spare;				/* must be zero */
    char label[KA_LABELSIZE];		/* label to verify correct decrypt */
};

struct ka_cpwAnswer {			/* format of response */
    Date time;				/* the time of the request plus one */
    char label[KA_LABELSIZE];		/* label to verify correct decrypt */
};

struct ka_getTicketTimes {
    Date start;
    Date end;
};

/* old interface: see ka_ticketAnswer instead */
struct ka_getTicketAnswer {
    struct ktc_encryptionKey sessionKey;
    Date startTime;
    Date endTime;
    int32 kvno;
    int32 ticketLen;
    char name[MAXKTCNAMELEN];
    char instance[MAXKTCNAMELEN];
    char cell[MAXKTCNAMELEN];
    char sname[MAXKTCNAMELEN];
    char sinstance[MAXKTCNAMELEN];
    char ticket[MAXKTCTICKETLEN];
};

#ifndef ERROR_TABLE_BASE_ka
#define ka_ErrorString error_message
#undef  KAMINERROR
#define KAMINERROR ERROR_TABLE_BASE_ka
#define KAMAXERROR (KAMINERROR+255)
#endif


#endif

/* End of prolog file kautils.p.h. */

#define KADATABASEINCONSISTENT                   (180480L)
#define KAEXIST                                  (180481L)
#define KAIO                                     (180482L)
#define KACREATEFAIL                             (180483L)
#define KANOENT                                  (180484L)
#define KAEMPTY                                  (180485L)
#define KABADNAME                                (180486L)
#define KABADINDEX                               (180487L)
#define KANOAUTH                                 (180488L)
#define KAANSWERTOOLONG                          (180489L)
#define KABADREQUEST                             (180490L)
#define KAOLDINTERFACE                           (180491L)
#define KABADARGUMENT                            (180492L)
#define KABADCMD                                 (180493L)
#define KANOKEYS                                 (180494L)
#define KAREADPW                                 (180495L)
#define KABADKEY                                 (180496L)
#define KAUBIKINIT                               (180497L)
#define KAUBIKCALL                               (180498L)
#define KABADPROTOCOL                            (180499L)
#define KANOCELLS                                (180500L)
#define KANOCELL                                 (180501L)
#define KATOOMANYUBIKS                           (180502L)
#define KATOOMANYKEYS                            (180503L)
#define KABADTICKET                              (180504L)
#define KAUNKNOWNKEY                             (180505L)
#define KAKEYCACHEINVALID                        (180506L)
#define KABADSERVER                              (180507L)
#define KABADUSER                                (180508L)
#define KABADCPW                                 (180509L)
#define KABADCREATE                              (180510L)
#define KANOTICKET                               (180511L)
#define KAASSOCUSER                              (180512L)
#define KANOTSPECIAL                             (180513L)
#define KACLOCKSKEW                              (180514L)
#define KANORECURSE                              (180515L)
#define KARXFAIL                                 (180516L)
#define KANULLPASSWORD                           (180517L)
#define KAINTERNALERROR                          (180518L)
#define KAPWEXPIRED                              (180519L)
#define KAREUSED                                 (180520L)
#define KATOOSOON                                (180521L)
#define KALOCKED                                 (180522L)
extern void initialize_ka_error_table ();
#define ERROR_TABLE_BASE_ka (180480L)

/* for compatibility with older versions... */
#define init_ka_err_tbl initialize_ka_error_table
#define ka_err_base ERROR_TABLE_BASE_ka
