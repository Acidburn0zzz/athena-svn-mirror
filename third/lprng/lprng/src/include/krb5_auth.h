/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-1999, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: krb5_auth.h,v 1.2 2001-03-07 01:20:09 ghudson Exp $
 ***************************************************************************/



#ifndef _KRB5_AUTH_H
#define _KRB5_AUTH_H 1

/* PROTOTYPES */
int server_krb5_auth( char *keytabfile, char *service, int sock,
	char **auth, char *err, int errlen, char *file );
int server_krb5_status( int sock, char *err, int errlen, char *file );
int server_krb5_status( int sock, char *err, int errlen, char *file );
int client_krb5_auth( char *keytabfile, char *service, char *host,
	char *server_principal,
	int options, char *life, char *renew_time,
	int sock, char *err, int errlen, char *file );
int remote_principal_krb5( char *service, char *host, char *err, int errlen );

#endif
