/*
 *    Kerberos v5 authentication and ticket-passing routines.
 * 
 * $FreeBSD: src/crypto/openssh/auth-krb5.c,v 1.6 2001/02/13 16:58:04 assar Exp $
 * $OpenBSD: auth-krb5.c,v 1.1 2001/06/26 16:15:23 dugsong Exp $
 */

#include "includes.h"
#include "ssh.h"
#include "ssh1.h"
#include "packet.h"
#include "xmalloc.h"
#include "log.h"
#include "servconf.h"
#include "uidswap.h"
#include "auth.h"

#ifdef KRB5
#include <krb5.h>
#ifndef HEIMDAL
#define krb5_get_err_text(context,code) error_message(code)
#endif /* !HEIMDAL */

extern ServerOptions	 options;

static int
krb5_init(void *context)
{
	Authctxt *authctxt = (Authctxt *)context;
	krb5_error_code problem;
	static int cleanup_registered = 0;
	int fd;
#ifndef HEIMDAL
	krb5_principal rcache_server = 0;
	krb5_rcache rcache;
#endif	

	if (authctxt->krb5_ctx == NULL) {
		problem = krb5_init_context(&authctxt->krb5_ctx);
		if (problem)
			return (problem);
		krb5_init_ets(authctxt->krb5_ctx);
		
	}
	if (authctxt->krb5_auth_ctx == NULL) {
		problem = krb5_auth_con_init(authctxt->krb5_ctx,
		    &authctxt->krb5_auth_ctx);
		if (problem)
		    return problem;

		krb5_parse_name(authctxt->krb5_ctx, "sshd", &rcache_server);
		krb5_get_server_rcache(authctxt->krb5_ctx,
		    krb5_princ_component(authctxt->krb5_ctx, 
		    rcache_server, 0), &rcache);
		krb5_auth_con_setrcache(authctxt->krb5_ctx,
		    authctxt->krb5_auth_ctx, 
		    rcache);
	
		fd = packet_get_connection_in();
#ifdef HEIMDAL
		problem = krb5_auth_con_setaddrs_from_fd(authctxt->krb5_ctx,
		   authctxt->krb5_auth_ctx, &fd);
#else
		problem = krb5_auth_con_genaddrs(authctxt->krb5_ctx, 
		    authctxt->krb5_auth_ctx,fd,
		    KRB5_AUTH_CONTEXT_GENERATE_REMOTE_FULL_ADDR |
		    KRB5_AUTH_CONTEXT_GENERATE_LOCAL_FULL_ADDR);
#endif
		if (problem)
		  return problem;
	}

	if (!cleanup_registered) {
		fatal_add_cleanup(krb5_cleanup_proc, authctxt);
		cleanup_registered = 1;
	}
	return (0);
}

/*
 * Try krb5 authentication. server_user is passed for logging purposes
 * only, in auth is received ticket, in client is returned principal
 * from the ticket
 */
int 
auth_krb5(Authctxt *authctxt, krb5_data *auth, char **client)
{
	krb5_error_code problem;
	krb5_principal server;
	krb5_data reply;
	krb5_ticket *ticket;
	
	server = NULL;
	ticket = NULL;
	reply.length = 0;
	
	problem = krb5_init(authctxt);
	if (problem) 
		goto err;
	
	problem = krb5_sname_to_principal(authctxt->krb5_ctx,  NULL, NULL ,
	    KRB5_NT_SRV_HST, &server);
	if (problem)
		goto err;
	
	problem = krb5_rd_req(authctxt->krb5_ctx, &authctxt->krb5_auth_ctx,
	    auth, server, NULL, NULL, &ticket);
	if (problem)
		goto err;
#ifdef HEIMDAL	
	problem = krb5_copy_principal(authctxt->krb5_ctx, ticket->client,
	    &authctxt->krb5_user);
#else
	problem = krb5_copy_principal(authctxt->krb5_ctx, ticket->enc_part2->client,
	    &authctxt->krb5_user);
#endif
	if (problem)
		goto err;
	
	/* if client wants mutual auth */
	problem = krb5_mk_rep(authctxt->krb5_ctx, authctxt->krb5_auth_ctx,
	    &reply);
	if (problem)
		goto err;
	
	/* Check .k5login authorization now. */
	if (!krb5_kuserok(authctxt->krb5_ctx, authctxt->krb5_user,
	    authctxt->pw->pw_name)) {
		/* XXX This is not the right error, but anything works ok. */
		problem = KRB5_NO_LOCALNAME;
		goto err;
	}
	
	if (client) {
		char *tmp;
		/* Is this necessary? */
		krb5_unparse_name(authctxt->krb5_ctx, authctxt->krb5_user,
		    client);
		tmp=*client;
		*client=xstrdup(tmp);
		krb5_free_unparsed_name(authctxt->krb5_ctx, tmp);
	}
	
	packet_start(SSH_SMSG_AUTH_KERBEROS_RESPONSE);
	packet_put_string((char *) reply.data, reply.length);
	packet_send();
	packet_write_wait();
	
 err:
	if (server)
		krb5_free_principal(authctxt->krb5_ctx, server);
	if (ticket)
		krb5_free_ticket(authctxt->krb5_ctx, ticket);
	if (reply.length)
		xfree(reply.data);
	
	if (problem) {
		debug("Kerberos v5 authentication failed: %s",
		    krb5_get_err_text(authctxt->krb5_ctx, problem));
		return (0);
	}
	return (1);
}

int
auth_krb5_tgt(Authctxt *authctxt, krb5_data *tgt)
{
	krb5_error_code problem;
	krb5_ccache ccache = NULL;
	char *pname;
	krb5_creds **creds;
	
	problem = krb5_init(authctxt);
	if (problem)
		goto fail;

	if (authctxt->pw == NULL)
		return (0);
	
	temporarily_use_uid(authctxt->pw);

	problem = krb5_rd_cred(authctxt->krb5_ctx, authctxt->krb5_auth_ctx,
	    tgt, &creds, NULL);
	if (problem) 
		goto fail;
#ifdef HEIMDAL	
	problem = krb5_cc_gen_new(authctxt->krb5_ctx, &krb5_fcc_ops, &ccache);
#else
{
	char ccname[35];
	
	snprintf(ccname, sizeof(ccname), "FILE:/tmp/krb5cc_p%d", getpid());
	problem = krb5_cc_resolve(authctxt->krb5_ctx, ccname, &ccache);
}
#endif
	if (problem)
		goto fail;
	
	/* XXX should we krb5_copy_principal()
	 *  (*creds)->client to authctxt->krb5_user? Instead we'll just
	 *  replace krb5_user references with (*creds)->client
	 */
	problem = krb5_copy_principal(authctxt->krb5_ctx, (*creds)->client,
				      &authctxt->krb5_user);
	if (problem)
		goto fail;
	problem = krb5_cc_initialize(authctxt->krb5_ctx, ccache,
	    authctxt->krb5_user);
	if (problem)
		goto fail;

	problem = krb5_cc_store_cred(authctxt->krb5_ctx, ccache, *creds);
	if (problem)
	        goto fail;
	
	authctxt->krb5_fwd_ccache = ccache;
	ccache = NULL;
	
	authctxt->krb5_ticket_file = (char *)krb5_cc_get_name(authctxt->krb5_ctx, authctxt->krb5_fwd_ccache);
	
	problem = krb5_unparse_name(authctxt->krb5_ctx, authctxt->krb5_user,
	    &pname);
	if (problem)
		goto fail;
	
	debug("Kerberos v5 TGT accepted (%s)", pname);
	
	restore_uid();
	
	return (1);
	
 fail:
	if (problem)
		debug("Kerberos v5 TGT passing failed: %s",
		    krb5_get_err_text(authctxt->krb5_ctx, problem));
	if (ccache)
		krb5_cc_destroy(authctxt->krb5_ctx, ccache);
	
	restore_uid();
	
	return (0);
}

int
auth_krb5_password(Authctxt *authctxt, const char *password)
{
#ifndef HEIMDAL
        krb5_creds creds;
	krb5_principal server;
	krb5_verify_init_creds_opt vopt;
	char ccname[35];
#endif	
	krb5_error_code problem;
	
	if (authctxt->pw == NULL)
		return (0);
	
	temporarily_use_uid(authctxt->pw);
	
	problem = krb5_init(authctxt);
	if (problem)
		goto out;
	
	problem = krb5_parse_name(authctxt->krb5_ctx, authctxt->pw->pw_name,
		    &authctxt->krb5_user);
	if (problem)
		goto out;

#ifdef HEIMDAL	
	problem = krb5_cc_gen_new(authctxt->krb5_ctx, &krb5_fcc_ops,
	    &authctxt->krb5_fwd_ccache);
#else
	snprintf(ccname, sizeof(ccname), "FILE:/tmp/krb5cc_p%d",
            getpid());
	problem = krb5_cc_resolve(authctxt->krb5_ctx, ccname,
	    &authctxt->krb5_fwd_ccache);
#endif
	if (problem)
		goto out;
	
	problem = krb5_cc_initialize(authctxt->krb5_ctx,
	    authctxt->krb5_fwd_ccache, authctxt->krb5_user);
	if (problem)
		goto out;

	restore_uid();
#ifdef HEIMDAL	
	problem = krb5_verify_user(authctxt->krb5_ctx, authctxt->krb5_user,
	    authctxt->krb5_fwd_ccache, password, 1, NULL);
	if (problem)
		goto out;
#else
        problem = krb5_get_init_creds_password(authctxt->krb5_ctx, &creds, 
            authctxt->krb5_user, password, NULL, NULL, 0, NULL, NULL);
        if (problem)
        	goto out;

        problem = krb5_sname_to_principal(authctxt->krb5_ctx, NULL, NULL, 
            KRB5_NT_SRV_HST, &server);
        if (problem)
        	goto out;

	krb5_verify_init_creds_opt_init(&vopt);
#if 0	/* This should be set in krb5.conf if at all */
        krb5_verify_init_creds_opt_set_ap_req_nofail(&vopt,1);
#endif
	temporarily_use_uid(authctxt->pw);

        problem = krb5_verify_init_creds(authctxt->krb5_ctx, &creds, server, NULL, NULL, 
            &vopt);
                                                                                                                                                                                                                                                                                                                                                                                                         
        krb5_free_principal(authctxt->krb5_ctx, server);
        if (problem)
        	goto out;

	problem = krb5_cc_store_cred(authctxt->krb5_ctx, authctxt->krb5_fwd_ccache, &creds);
	if (problem)
		goto out;

#endif /* HEIMDAL */

	authctxt->krb5_ticket_file = (char *)krb5_cc_get_name(authctxt->krb5_ctx, authctxt->krb5_fwd_ccache);
	
 out:
	restore_uid();
	
	if (problem) {
		debug("Kerberos password authentication failed: %s",
		    krb5_get_err_text(authctxt->krb5_ctx, problem));
#if 0
		/* We can't really punt here completely, because if we
		   do, we won't be able to decrypt forwarded credentials. */
		krb5_cleanup_proc(authctxt);
#endif
		
		if (options.kerberos_or_local_passwd)
			return (-1);
		else
			return (0);
	}
	return (1);
}

void
krb5_cleanup_proc(void *context)
{
	Authctxt *authctxt = (Authctxt *)context;
	
	debug("krb5_cleanup_proc called");
	if (authctxt->krb5_fwd_ccache) {
		krb5_cc_destroy(authctxt->krb5_ctx, authctxt->krb5_fwd_ccache);
		authctxt->krb5_fwd_ccache = NULL;
	}
	if (authctxt->krb5_user) {
		krb5_free_principal(authctxt->krb5_ctx, authctxt->krb5_user);
		authctxt->krb5_user = NULL;
	}
	if (authctxt->krb5_auth_ctx) {
		krb5_auth_con_free(authctxt->krb5_ctx,
		    authctxt->krb5_auth_ctx);
		authctxt->krb5_auth_ctx = NULL;
	}
	if (authctxt->krb5_ctx) {
		krb5_free_context(authctxt->krb5_ctx);
		authctxt->krb5_ctx = NULL;
	}
}

#endif /* KRB5 */
