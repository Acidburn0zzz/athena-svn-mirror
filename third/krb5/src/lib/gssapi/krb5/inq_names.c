/*
 * lib/gssapi/krb5/inq_names.c
 *
 * Copyright 1995 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 */

/*
 * inq_names.c - Return set of nametypes supported by the KRB5 mechanism.
 */
#include "gssapiP_krb5.h"

OM_uint32
krb5_gss_inquire_names_for_mech(minor_status, mechanism, name_types)
    OM_uint32	*minor_status;
    gss_OID	mechanism;
    gss_OID_set	*name_types;
{
    krb5_context context;
    OM_uint32	major, minor;

    if (GSS_ERROR(kg_get_context(minor_status, &context)))
       return(GSS_S_FAILURE);

    /*
     * We only know how to handle our own mechanism.
     */
    if ((mechanism != GSS_C_NULL_OID) &&
	!g_OID_equal(gss_mech_krb5, mechanism) &&
	!g_OID_equal(gss_mech_krb5_old, mechanism)) {
	*minor_status = 0;
	return(GSS_S_FAILURE);
    }

    /* We're okay.  Create an empty OID set */
    major = gss_create_empty_oid_set(minor_status, name_types);
    if (major == GSS_S_COMPLETE) {
	/* Now add our members. */
	if (
	    ((major = gss_add_oid_set_member(minor_status,
					     (gss_OID) gss_nt_user_name,
					     name_types)
	      ) == GSS_S_COMPLETE) &&
	    ((major = gss_add_oid_set_member(minor_status,
					     (gss_OID) gss_nt_machine_uid_name,
					     name_types)
	      ) == GSS_S_COMPLETE) &&
	    ((major = gss_add_oid_set_member(minor_status,
					     (gss_OID) gss_nt_string_uid_name,
					     name_types)
	      ) == GSS_S_COMPLETE) &&
	    ((major = gss_add_oid_set_member(minor_status,
					     (gss_OID) gss_nt_service_name,
					     name_types)
	      ) == GSS_S_COMPLETE) &&
	    ((major = gss_add_oid_set_member(minor_status,
					     (gss_OID) gss_nt_krb5_name,
					     name_types)
	      ) == GSS_S_COMPLETE)
	    ) {
	    major = gss_add_oid_set_member(minor_status,
					   (gss_OID) gss_nt_krb5_principal,
					   name_types);
	}

	/*
	 * If we choked, then release the set, but don't overwrite the minor
	 * status with the release call.
	 */
	if (major != GSS_S_COMPLETE)
	    (void) gss_release_oid_set(&minor,
				       name_types);
    }
    return(major);
}
