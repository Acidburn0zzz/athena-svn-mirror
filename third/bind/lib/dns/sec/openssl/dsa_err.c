/* crypto/dsa/dsa_err.c */
/* ====================================================================
 * Copyright (c) 1999 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

/* NOTE: this file was auto generated by the mkerr.pl script: any changes
 * made to it will be overwritten when the script next updates this file,
 * only reason strings will be preserved.
 */

#include <config.h>
#include "../rename.h"

#include <stdio.h>
#include <openssl/err.h>
#include <openssl/dsa.h>

/* BEGIN ERROR CODES */
#ifndef NO_ERR
static ERR_STRING_DATA DSA_str_functs[]=
	{
{ERR_PACK(0,DSA_F_D2I_DSA_SIG,0),	"d2i_DSA_SIG"},
{ERR_PACK(0,DSA_F_DSAPARAMS_PRINT,0),	"DSAparams_print"},
{ERR_PACK(0,DSA_F_DSAPARAMS_PRINT_FP,0),	"DSAparams_print_fp"},
{ERR_PACK(0,DSA_F_DSA_DO_SIGN,0),	"DSA_do_sign"},
{ERR_PACK(0,DSA_F_DSA_DO_VERIFY,0),	"DSA_do_verify"},
{ERR_PACK(0,DSA_F_DSA_NEW,0),	"DSA_new"},
{ERR_PACK(0,DSA_F_DSA_PRINT,0),	"DSA_print"},
{ERR_PACK(0,DSA_F_DSA_PRINT_FP,0),	"DSA_print_fp"},
{ERR_PACK(0,DSA_F_DSA_SIGN,0),	"DSA_sign"},
{ERR_PACK(0,DSA_F_DSA_SIGN_SETUP,0),	"DSA_sign_setup"},
{ERR_PACK(0,DSA_F_DSA_SIG_NEW,0),	"DSA_SIG_new"},
{ERR_PACK(0,DSA_F_DSA_VERIFY,0),	"DSA_verify"},
{ERR_PACK(0,DSA_F_I2D_DSA_SIG,0),	"i2d_DSA_SIG"},
{0,NULL}
	};

static ERR_STRING_DATA DSA_str_reasons[]=
	{
{DSA_R_DATA_TOO_LARGE_FOR_KEY_SIZE       ,"data too large for key size"},
{0,NULL}
	};

#endif

void ERR_load_DSA_strings(void)
	{
	static int init=1;

	if (init)
		{
		init=0;
#ifndef NO_ERR
		ERR_load_strings(ERR_LIB_DSA,DSA_str_functs);
		ERR_load_strings(ERR_LIB_DSA,DSA_str_reasons);
#endif

		}
	}
