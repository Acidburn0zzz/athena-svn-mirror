/*
 * lib/krb5/krb/copy_data.c
 *
 * Copyright 1990,1991 by the Massachusetts Institute of Technology.
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
 *
 * krb5_copy_data()
 */

#include "k5-int.h"

/*
 * Copy a data structure, with fresh allocation.
 */
krb5_error_code
krb5_copy_data(context, indata, outdata)
    krb5_context context;
    const krb5_data *indata;
    krb5_data **outdata;
{
    krb5_data *tempdata;

    if (!indata) {
	*outdata = 0;
	return 0;
    }
    
    if (!(tempdata = (krb5_data *)malloc(sizeof(*tempdata))))
	return ENOMEM;

    tempdata->length = indata->length;
    if (tempdata->length) {
	if (!(tempdata->data = malloc(tempdata->length))) {
	    krb5_xfree(tempdata);
	    return ENOMEM;
	}
	memcpy((char *)tempdata->data, (char *)indata->data, tempdata->length);
    } else
	tempdata->data = 0;
    tempdata->magic = KV5M_DATA;
    *outdata = tempdata;
    return 0;
}
