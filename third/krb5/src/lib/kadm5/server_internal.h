/*
 * Copyright 1993 OpenVision Technologies, Inc., All Rights Reserved
 *
 * $Header: /afs/dev.mit.edu/source/repository/third/krb5/src/lib/kadm5/server_internal.h,v 1.1.1.1 1996-09-12 04:43:54 ghudson Exp $
 */

/*
 * This header file is used internally by the Admin API server
 * libraries and Admin server.  IF YOU THINK YOU NEED TO USE THIS FILE
 * FOR ANYTHING, YOU'RE ALMOST CERTAINLY WRONG.
 */

#ifndef __KADM5_SERVER_INTERNAL_H__
#define __KADM5_SERVER_INTERNAL_H__

#include    <memory.h>
#include    <malloc.h>
#include    "k5-int.h"
#include    <krb5/kdb.h>
#include    <kadm5/admin.h>
#include    "admin_internal.h"
#include    "adb.h"

typedef struct _kadm5_server_handle_t {
	krb5_ui_4	magic_number;
	krb5_ui_4	struct_version;
	krb5_ui_4	api_version;
	krb5_context	context;
	krb5_principal	current_caller;
	kadm5_config_params  params;
	struct _kadm5_server_handle_t *lhandle;
	osa_adb_policy_t policy_db;
} kadm5_server_handle_rec, *kadm5_server_handle_t;

kadm5_ret_t    adb_policy_init(kadm5_server_handle_t handle);
kadm5_ret_t    adb_policy_close(kadm5_server_handle_t handle);
kadm5_ret_t    passwd_check(kadm5_server_handle_t handle,
			    char *pass, int use_policy,
			    kadm5_policy_ent_t policy,
			    krb5_principal principal);
kadm5_ret_t    principal_exists(krb5_principal principal);
krb5_error_code	    kdb_init_master(kadm5_server_handle_t handle,
				    char *r, int from_keyboard);
krb5_error_code	    kdb_init_hist(kadm5_server_handle_t handle,
				  char *r);
krb5_error_code     kdb_get_entry(kadm5_server_handle_t handle,
				  krb5_principal principal, krb5_db_entry *kdb,
				  osa_princ_ent_rec *adb);
krb5_error_code     kdb_free_entry(kadm5_server_handle_t handle,
				   krb5_db_entry *kdb, osa_princ_ent_rec *adb);
krb5_error_code     kdb_put_entry(kadm5_server_handle_t handle,
				  krb5_db_entry *kdb, osa_princ_ent_rec *adb);
krb5_error_code     kdb_delete_entry(kadm5_server_handle_t handle,
				     krb5_principal name);

int		    init_dict(kadm5_config_params *);
int		    find_word(const char *word);
void		    destroy_dict(void);

/*
 * *Warning* 
 * *Warning*	    This is going to break if we     
 * *Warning*	    ever go multi-threaded	     
 * *Warning* 
 */
extern	krb5_principal	current_caller;

/*
 * Why is this (or something similar) not defined *anywhere* in krb5?
 */
#define KSUCCESS	0
#define WORD_NOT_FOUND	1

/*
 * all the various mask bits or'd together
 */

#define	ALL_PRINC_MASK (KADM5_PRINCIPAL | KADM5_PRINC_EXPIRE_TIME | KADM5_PW_EXPIRATION | \
			KADM5_LAST_PWD_CHANGE | KADM5_ATTRIBUTES | KADM5_MAX_LIFE | \
			KADM5_MOD_TIME | KADM5_MOD_NAME | KADM5_KVNO | KADM5_MKVNO | \
			KADM5_AUX_ATTRIBUTES | KADM5_POLICY_CLR | KADM5_POLICY)

#define ALL_POLICY_MASK (KADM5_POLICY | KADM5_PW_MAX_LIFE | KADM5_PW_MIN_LIFE | \
			 KADM5_PW_MIN_LENGTH | KADM5_PW_MIN_CLASSES | KADM5_PW_HISTORY_NUM | \
			 KADM5_REF_COUNT)

#define SERVER_CHECK_HANDLE(handle) \
{ \
	kadm5_server_handle_t srvr = \
	     (kadm5_server_handle_t) handle; \
 \
	if (! srvr->current_caller) \
		return KADM5_BAD_SERVER_HANDLE; \
	if (! srvr->lhandle) \
	        return KADM5_BAD_SERVER_HANDLE; \
}

#define CHECK_HANDLE(handle) \
     GENERIC_CHECK_HANDLE(handle, KADM5_OLD_SERVER_API_VERSION, \
			  KADM5_NEW_SERVER_API_VERSION) \
     SERVER_CHECK_HANDLE(handle)

#endif /* __KADM5_SERVER_INTERNAL_H__ */
