/*
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: confctx.c,v 1.1.1.1 2001-10-22 13:07:37 ghudson Exp $ */

#include <config.h>

#include <isc/mem.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <isc/util.h>

#include <dns/confctx.h>
#include <dns/log.h>
#include <dns/peer.h>

#include "confpvt.h"

#define SETBOOL(FUNC, FIELD) SETBYTYPE(isc_boolean_t, FUNC, FIELD)
#define GETBOOL(FUNC, FIELD) GETBYTYPE(isc_boolean_t, FUNC, FIELD)
#define UNSETBOOL(FUNC, FIELD) UNSETBYTYPE(isc_boolean_t, FUNC, FIELD)
#define BOOL_FUNCS(FUNC, FIELD) \
	SETBOOL(FUNC, FIELD) \
	GETBOOL(FUNC, FIELD) \
	UNSETBOOL(FUNC, FIELD) 


#define SETNOTIFYTYPE(FUNC, FIELD) SETBYTYPE(dns_notifytype_t, FUNC, FIELD)
#define GETNOTIFYTYPE(FUNC, FIELD) GETBYTYPE(dns_notifytype_t, FUNC, FIELD)
#define UNSETNOTIFYTYPE(FUNC, FIELD) UNSETBYTYPE(dns_notifytype_t, FUNC, FIELD)
#define NOTIFYTYPE_FUNCS(FUNC, FIELD) \
	SETNOTIFYTYPE(FUNC, FIELD) \
	GETNOTIFYTYPE(FUNC, FIELD) \
	UNSETNOTIFYTYPE(FUNC, FIELD) 


#define SETINT32(FUNC, FIELD) SETBYTYPE(isc_int32_t, FUNC, FIELD)
#define GETINT32(FUNC, FIELD) GETBYTYPE(isc_int32_t, FUNC, FIELD)
#define UNSETINT32(FUNC, FIELD) UNSETBYTYPE(isc_int32_t, FUNC, FIELD)
#define INT32_FUNCS(FUNC, FIELD) \
	SETINT32(FUNC, FIELD) \
	GETINT32(FUNC, FIELD) \
	UNSETINT32(FUNC, FIELD) 


#define SETUINT32(FUNC, FIELD) SETBYTYPE(isc_uint32_t, FUNC, FIELD)
#define GETUINT32(FUNC, FIELD) GETBYTYPE(isc_uint32_t, FUNC, FIELD)
#define UNSETUINT32(FUNC, FIELD) UNSETBYTYPE(isc_uint32_t, FUNC, FIELD)
#define UINT32_FUNCS(FUNC, FIELD) \
	SETUINT32(FUNC, FIELD) \
	GETUINT32(FUNC, FIELD) \
	UNSETUINT32(FUNC, FIELD) 


#define SETSOCKADDR(FUNC, FIELD) SETBYTYPE(isc_sockaddr_t, FUNC, FIELD)
#define GETSOCKADDR(FUNC, FIELD) GETBYTYPE(isc_sockaddr_t, FUNC, FIELD)
#define UNSETSOCKADDR(FUNC, FIELD) UNSETBYTYPE(isc_sockaddr_t, FUNC, FIELD)
#define SOCKADDR_FUNCS(FUNC, FIELD) \
	SETSOCKADDR(FUNC, FIELD) \
	GETSOCKADDR(FUNC, FIELD) \
	UNSETSOCKADDR(FUNC, FIELD) 

#ifdef PVT_CONCAT
#undef PVT_CONCAT
#endif

#define PVT_CONCAT(x,y) x ## y

#define SETBYTYPE(TYPE, FUNCNAME, FIELDNAME)				\
isc_result_t								\
PVT_CONCAT(dns_c_ctx_set, FUNCNAME)(dns_c_ctx_t *cfg, TYPE newval)	\
{									\
	isc_result_t result;						\
	isc_boolean_t existed = ISC_FALSE;				\
	dns_c_options_t *options;					\
									\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));				\
	if (cfg->options == NULL) {					\
		result = dns_c_ctx_optionsnew(cfg->mem, &cfg->options);	\
		if (result != ISC_R_SUCCESS) {				\
			return (result);				\
		}							\
	}								\
									\
	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));                     \
	options = cfg->options;						\
									\
	if (options->FIELDNAME == NULL) {				\
		options->FIELDNAME = isc_mem_get(options->mem,		\
						 sizeof (TYPE));	\
		if (options->FIELDNAME == NULL) {			\
			return (ISC_R_NOMEMORY);			\
		}							\
	} else {							\
		existed = ISC_TRUE;					\
	}								\
									\
	*options->FIELDNAME = newval;					\
									\
	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);		\
}

#define GETBYTYPE(TYPE, FUNCNAME, FIELDNAME)				\
isc_result_t								\
PVT_CONCAT(dns_c_ctx_get, FUNCNAME)(dns_c_ctx_t *cfg, TYPE *retval)	\
{									\
	dns_c_options_t *options;					\
									\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));				\
	REQUIRE(retval != NULL);					\
									\
	options = cfg->options;						\
									\
	if (options == NULL) {						\
		return (ISC_R_NOTFOUND);				\
	}								\
									\
	REQUIRE(DNS_C_CONFOPT_VALID(options));				\
									\
	if (options->FIELDNAME == NULL) {				\
		return (ISC_R_NOTFOUND);				\
	} else {							\
		*retval = *options->FIELDNAME;				\
		return (ISC_R_SUCCESS);					\
	}								\
}

#define UNSETBYTYPE(TYPE, FUNCNAME, FIELDNAME)			\
isc_result_t							\
PVT_CONCAT(dns_c_ctx_unset, FUNCNAME)(dns_c_ctx_t *cfg)		\
{								\
	dns_c_options_t *options;				\
								\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));			\
								\
	options = cfg->options;					\
								\
	if (options == NULL) {					\
		return (ISC_R_NOTFOUND);			\
	}							\
								\
	REQUIRE(DNS_C_CONFOPT_VALID(options));			\
								\
	if (options->FIELDNAME == NULL) {			\
		return (ISC_R_NOTFOUND);			\
	} else {						\
		isc_mem_put(options->mem, options->FIELDNAME,	\
			    sizeof (options->FIELDNAME));	\
		options->FIELDNAME = NULL;			\
								\
		return (ISC_R_SUCCESS);				\
	}							\
}

#define BYTYPE_FUNCS(TYPE, FUNC, FIELD) \
	SETBYTYPE(TYPE, FUNC, FIELD) \
	GETBYTYPE(TYPE, FUNC, FIELD) \
	UNSETBYTYPE(TYPE, FUNC, FIELD)


#define SETSTRING(FUNC, FIELD)						     \
isc_result_t								     \
PVT_CONCAT(dns_c_ctx_set, FUNC)(dns_c_ctx_t *cfg, const char *newval)	     \
{									     \
	isc_result_t res;						     \
									     \
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));				     \
	REQUIRE(newval != NULL);					     \
									     \
	res = make_options(cfg);					     \
	if (res != ISC_R_SUCCESS) {					     \
		return (res);						     \
	}								     \
									     \
	return (cfg_set_string(cfg->options, &cfg->options->FIELD, newval)); \
}


#define GETSTRING(FUNC, FIELD)						\
isc_result_t								\
PVT_CONCAT(dns_c_ctx_get, FUNC)(dns_c_ctx_t *cfg, char **retval)	\
{									\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));				\
	REQUIRE(retval != NULL);					\
									\
	if (cfg->options == NULL) {					\
		return (ISC_R_NOTFOUND);				\
	}								\
									\
	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));			\
									\
	*retval = cfg->options->FIELD;					\
									\
	return (*retval == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);	\
}


#define UNSETSTRING(FUNC, FIELD)				\
isc_result_t							\
PVT_CONCAT(dns_c_ctx_unset, FUNC)(dns_c_ctx_t *cfg)		\
{								\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));			\
								\
	if (cfg->options == NULL) {				\
		return (ISC_R_NOTFOUND);			\
	}							\
								\
	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));		\
								\
	if (cfg->options->FIELD == NULL) {			\
		return (ISC_R_NOTFOUND);			\
	}							\
								\
	isc_mem_free(cfg->options->mem, cfg->options->FIELD);	\
	cfg->options->FIELD = NULL;				\
								\
	return (ISC_R_SUCCESS);					\
}
#define STRING_FUNCS(FUNC, FIELD) \
	SETSTRING(FUNC, FIELD) \
	GETSTRING(FUNC, FIELD) \
	UNSETSTRING(FUNC, FIELD)




#define SETIPMLIST(FUNCNAME, FIELD)					\
isc_result_t								\
PVT_CONCAT(dns_c_ctx_set, FUNCNAME)(dns_c_ctx_t *cfg,			\
				    dns_c_ipmatchlist_t *newval)	\
{									\
	isc_result_t res;						\
									\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));				\
									\
	res = make_options(cfg);					\
	if (res != ISC_R_SUCCESS) {					\
		return (res);						\
	}								\
									\
	REQUIRE(newval != NULL);					\
									\
	if (cfg->options->FIELD != NULL) {				\
		dns_c_ipmatchlist_detach(&cfg->options->FIELD);		\
	}								\
									\
	dns_c_ipmatchlist_attach(newval, &cfg->options->FIELD);		\
	return (ISC_R_SUCCESS);						\
}									\



#define GETIPMLIST(FUNC, FIELD)						\
isc_result_t								\
PVT_CONCAT(dns_c_ctx_get, FUNC)(dns_c_ctx_t *cfg,			\
				dns_c_ipmatchlist_t **retval)		\
{									\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));				\
									\
	if (cfg->options == NULL) {					\
		return (ISC_R_NOTFOUND);				\
	}								\
									\
	REQUIRE(retval != NULL);					\
									\
	if (cfg->options->FIELD != NULL) {				\
		dns_c_ipmatchlist_attach(cfg->options->FIELD, retval);	\
		return (ISC_R_SUCCESS);					\
	} else {							\
		return (ISC_R_NOTFOUND);				\
	}								\
}




#define UNSETIPMLIST(FUNC, FIELD)			\
isc_result_t						\
PVT_CONCAT(dns_c_ctx_unset, FUNC)(dns_c_ctx_t *cfg)	\
{							\
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));		\
							\
	if (cfg->options == NULL) {			\
		return (ISC_R_NOTFOUND);		\
	}						\
							\
	dns_c_ipmatchlist_detach(&cfg->options->FIELD);	\
							\
	return (ISC_R_SUCCESS);				\
}

#define IPMLIST_FUNCS(FUNC, FIELD) \
	SETIPMLIST(FUNC, FIELD) \
	GETIPMLIST(FUNC, FIELD) \
	UNSETIPMLIST(FUNC, FIELD) \





static isc_result_t cfg_set_iplist(dns_c_options_t *options,
				   dns_c_iplist_t **fieldaddr,
				   dns_c_iplist_t *newval,
				   isc_boolean_t copy);
static isc_result_t cfg_set_string(dns_c_options_t *options,
				   char **field,
				   const char *newval);


static isc_result_t cfg_get_iplist(dns_c_options_t *options,
				   dns_c_iplist_t *field,
				   dns_c_iplist_t **resval);
static isc_result_t acl_init(dns_c_ctx_t *cfg);
static isc_result_t logging_init (dns_c_ctx_t *cfg);
static isc_result_t make_options(dns_c_ctx_t *cfg);


isc_result_t
dns_c_checkconfig(dns_c_ctx_t *cfg)
{
	isc_boolean_t 		bval;
	char     	       *cpval;
	dns_severity_t	severity;
	isc_uint32_t		uintval;
	dns_c_ipmatchlist_t    *ipml;
	isc_result_t 		result = ISC_R_SUCCESS;
	isc_result_t		tmpres;
	dns_c_rrsolist_t       *olist;
	dns_c_lstnlist_t       *listenlist;
	dns_c_ctrllist_t       *controls;

	if (dns_c_ctx_getnamedxfer(cfg, &cpval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'named-xfer' is obsolete");
	}

	if (dns_c_ctx_getmemstatsfilename(cfg, &cpval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'memstatistics-file' is not yet "
			      "implemented");
	}

	if (dns_c_ctx_getauthnxdomain(cfg, &bval) == ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "the default for the 'auth-nxdomain' option "
			      "is now 'no'");
	}

	if (dns_c_ctx_getdealloconexit(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'deallocate-on-exit' is obsolete");
	}

	if (dns_c_ctx_getfakeiquery(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'fake-iquery' is obsolete");
	}

	if (dns_c_ctx_getfetchglue(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'fetch-glue' is obsolete");
	}

	if (dns_c_ctx_gethasoldclients(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'has-old-clients' is obsolete");
	}

	if (dns_c_ctx_gethoststatistics(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'host-statistics' is not yet "
			      "implemented");
	}

	if (dns_c_ctx_getmultiplecnames(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'multiple-cnames' is obsolete");
	}

	if (dns_c_ctx_getrfc2308type1(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'rfc2308-type-1' is not yet "
			      "implemented");
	}

	if (dns_c_ctx_getuseidpool(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'use-id-pool' is obsolete");
	}

	if (dns_c_ctx_getuseixfr(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'use-ixfr' is obsolete");
	}

	if (dns_c_ctx_gettreatcrasspace(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'treat-cr-as-space' is obsolete");
	}

	if ((dns_c_ctx_getchecknames(cfg, dns_trans_primary,
				     &severity) != ISC_R_NOTFOUND) ||
	    (dns_c_ctx_getchecknames(cfg, dns_trans_secondary,
				     &severity) != ISC_R_NOTFOUND) ||
	    (dns_c_ctx_getchecknames(cfg, dns_trans_response,
				     &severity) != ISC_R_NOTFOUND)) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'check-names' is not implemented");
	}

	if (dns_c_ctx_getmaxcachesize(cfg, &uintval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'max-cache-size' is not yet "
			      "implemented");
	}

	if (dns_c_ctx_getminroots(cfg, &uintval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'min-roots' is obsolete");
	}

	if (dns_c_ctx_getserialqueries(cfg, &uintval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'serial-queries' is obsolete");
	}

	if (dns_c_ctx_getmaintainixfrbase(cfg, &bval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'maintain-ixfr-base' is obsolete");
	}

	if (dns_c_ctx_getmaxlogsizeixfr(cfg, &uintval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'max-ixfr-log-size' is obsolete");
	}

	if (dns_c_ctx_getstatsinterval(cfg, &uintval) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'statistics-interval' is not yet "
			      "implemented");
	}

	if (dns_c_ctx_gettopology(cfg, &ipml) != ISC_R_NOTFOUND) {
		dns_c_ipmatchlist_detach(&ipml);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'topology' is deprecated");
	}

	if (dns_c_ctx_getrrsetorderlist(cfg, &olist) != ISC_R_NOTFOUND) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'rrset-order' is not yet implemented");
	}

	if (cfg->zlist != NULL) {
		tmpres = dns_c_zonelist_checkzones(cfg->zlist);
		if (tmpres != ISC_R_SUCCESS) {
			result = tmpres;
		}
	}

	if (cfg->views != NULL) {
		tmpres = dns_c_viewtable_checkviews(cfg->views);
		if (tmpres != ISC_R_SUCCESS) {
			result = tmpres;
		}
	}

	if (dns_c_ctx_getlistenlist(cfg, &listenlist) != ISC_R_NOTFOUND) {
		tmpres = dns_c_lstnlist_validate(listenlist);
		if (tmpres != ISC_R_SUCCESS) {
			result = tmpres;
		}
	}

	if (dns_c_ctx_getv6listenlist(cfg, &listenlist) != ISC_R_NOTFOUND) {
		tmpres = dns_c_lstnlistv6_validate(listenlist);
		if (tmpres != ISC_R_SUCCESS) {
			result = tmpres;
		}
	}

	if (dns_c_ctx_getcontrols(cfg, &controls) != ISC_R_NOTFOUND) {
		tmpres = dns_c_ctrllist_validate(controls);
		if (tmpres != ISC_R_SUCCESS) {
			result = tmpres;
		}
	}

	if (dns_c_ctx_getcachefile(cfg, &cpval) == ISC_R_SUCCESS &&
	    cfg->views != NULL)
	{
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_WARNING,
			      "option 'cache-file' cannot be present if views "
			      "are present");
		result = ISC_R_FAILURE;
	}

	return (result);
}


/* ************************************************************************ */

isc_result_t
dns_c_ctx_new(isc_mem_t *mem, dns_c_ctx_t **cfg)
{
	dns_c_ctx_t *tmpcfg;
	isc_result_t r;

	REQUIRE(mem != NULL);

	tmpcfg = isc_mem_get(mem, sizeof *tmpcfg);
	if (tmpcfg == NULL) {
		return (ISC_R_NOMEMORY);
	}

	tmpcfg->magic = DNS_C_CONFIG_MAGIC;
	tmpcfg->mem = mem;
	tmpcfg->warnings = 0;
	tmpcfg->errors = 0;
	tmpcfg->acls = NULL;
	tmpcfg->options = NULL;
	tmpcfg->zlist = NULL;
	tmpcfg->peers = NULL;
	tmpcfg->acls = NULL;
	tmpcfg->keydefs = NULL;
	tmpcfg->trusted_keys = NULL;
	tmpcfg->logging = NULL;
	tmpcfg->resolver = NULL;
	tmpcfg->cache = NULL;
	tmpcfg->views = NULL;
	tmpcfg->lwres = NULL;

	tmpcfg->currview = NULL;
	tmpcfg->currzone = NULL;

	r = acl_init(tmpcfg);
	if (r != ISC_R_SUCCESS) {
		return (r);
	}

	r = logging_init(tmpcfg);
	if (r != ISC_R_SUCCESS) {
		return (r);
	}


#if 1					/* XXX brister */
	tmpcfg->controls = NULL;
#else
	r = dns_c_ctrllist_new(mem, &tmpcfg->controls);
	if (r != ISC_R_SUCCESS) {
		dns_c_ctx_delete(&tmpcfg);
		return r;
	}
#endif

	*cfg = tmpcfg;

	return (ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_delete(dns_c_ctx_t **cfg)
{
	dns_c_ctx_t *c;

	REQUIRE(cfg != NULL);
	REQUIRE(*cfg != NULL);
	REQUIRE(DNS_C_CONFCTX_VALID(*cfg));

	c = *cfg;

	REQUIRE(c->mem != NULL);

	if (c->options != NULL)
		dns_c_ctx_optionsdelete(&c->options);

	if (c->controls != NULL)
		dns_c_ctrllist_delete(&c->controls);

	if (c->peers != NULL)
		dns_peerlist_detach(&c->peers);

	if (c->acls != NULL)
		dns_c_acltable_delete(&c->acls);

	if (c->keydefs != NULL)
		dns_c_kdeflist_delete(&c->keydefs);

	if (c->zlist != NULL)
		dns_c_zonelist_delete(&c->zlist);

	if (c->trusted_keys != NULL)
		dns_c_tkeylist_delete(&c->trusted_keys);

	if (c->logging != NULL)
		dns_c_logginglist_delete(&c->logging);

	if (c->views != NULL)
		dns_c_viewtable_delete(&c->views);

	if (c->lwres != NULL)
		dns_c_lwreslist_delete(&c->lwres);

	c->magic = 0;
	isc_mem_put(c->mem, c, sizeof *c);
	*cfg = NULL;

	return (ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_setcontrols(dns_c_ctx_t *cfg, dns_c_ctrllist_t *ctrls)
{
	isc_boolean_t existed = ISC_FALSE;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(DNS_C_CONFCTLLIST_VALID(ctrls));

	if (cfg->controls != NULL) {
		existed = ISC_TRUE;
		dns_c_ctrllist_delete(&cfg->controls);
	}

	cfg->controls = ctrls;

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_c_ctx_getcontrols(dns_c_ctx_t *cfg, dns_c_ctrllist_t **ctrls)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	*ctrls = cfg->controls;

	return (cfg->controls != NULL ? ISC_R_SUCCESS : ISC_R_NOTFOUND);
}




isc_result_t
dns_c_ctx_setcurrzone(dns_c_ctx_t *cfg, dns_c_zone_t *zone)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	cfg->currzone = zone;		/* zone may be NULL */

	/* XXX should we validate that the zone is in our table? */

	return (ISC_R_SUCCESS);
}



dns_c_zone_t *
dns_c_ctx_getcurrzone(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	return (cfg->currzone);
}



isc_result_t
dns_c_ctx_setcurrview(dns_c_ctx_t *cfg,
		      dns_c_view_t *view)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	cfg->currview = view;

	/* XXX should we validate that the zone is in our table? */

	return (ISC_R_SUCCESS);
}



dns_c_view_t *
dns_c_ctx_getcurrview(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	return (cfg->currview);
}



isc_result_t
dns_c_ctx_getpeerlist(dns_c_ctx_t *cfg, dns_peerlist_t **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(retval != NULL);

	if (cfg->peers == NULL) {
		*retval = NULL;
		return (ISC_R_NOTFOUND);
	} else {
		dns_peerlist_attach(cfg->peers, retval);
		return (ISC_R_SUCCESS);
	}
}


isc_result_t
dns_c_ctx_unsetpeerlist(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->peers != NULL) {
		dns_peerlist_detach(&cfg->peers);
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_FAILURE);
	}
}


isc_result_t
dns_c_ctx_setpeerlist(dns_c_ctx_t *cfg, dns_peerlist_t *newval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->peers != NULL) {
		dns_peerlist_detach(&cfg->peers);
	}

	dns_peerlist_attach(newval, &cfg->peers);

	return (ISC_R_SUCCESS);
}




void
dns_c_ctx_print(FILE *fp, int indent, dns_c_ctx_t *cfg)
{
	REQUIRE(fp != NULL);
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->logging != NULL) {
		dns_c_logginglist_print(fp, indent,
					cfg->logging, ISC_FALSE);
		fprintf(fp,"\n");
	}


	if (cfg->keydefs != NULL) {
		dns_c_kdeflist_print(fp, indent, cfg->keydefs);
		fprintf(fp, "\n");
	}


	if (cfg->trusted_keys != NULL) {
		dns_c_tkeylist_print(fp, indent, cfg->trusted_keys);
		fprintf(fp, "\n");
	}


	if (cfg->acls != NULL) {
		dns_c_acltable_print(fp, indent, cfg->acls);
		fprintf(fp,"\n");
	}


	if (cfg->options != NULL) {
		dns_c_ctx_optionsprint(fp, indent, cfg->options);
		fprintf(fp,"\n");
	}


	if (cfg->views != NULL) {
		dns_c_viewtable_print(fp, indent, cfg->views);
		fprintf(fp, "\n");
	}


	if (cfg->zlist != NULL) {
		dns_c_zonelist_print(fp, indent, cfg->zlist, NULL);
		fprintf(fp, "\n");
	}

	if (cfg->controls != NULL) {
		dns_c_ctrllist_print(fp, indent, cfg->controls);
		fprintf(fp, "\n");
	}


	if (cfg->peers != NULL) {
		dns_c_peerlist_print(fp, indent, cfg->peers);
		fprintf(fp, "\n");
	}
}



void
dns_c_ctx_optionsprint(FILE *fp, int indent, dns_c_options_t *options)
{
	dns_severity_t nameseverity;
	in_port_t port;
	in_port_t defport = DNS_C_DEFAULTPORT;

	REQUIRE(fp != NULL);

	if (options == NULL) {
		return;
	}

	REQUIRE(DNS_C_CONFOPT_VALID(options));

	if (options->port != NULL) {
		defport = *options->port;
	}

#define PRINT_INTEGER(FIELD, NAME)					\
	if (options->FIELD != NULL) {					\
		dns_c_printtabs(fp, indent + 1);			\
		fprintf(fp, "%s %d;\n",NAME,(int)*options->FIELD);	\
	}

#define PRINT_AS_MINUTES(FIELD, NAME)				\
	if (options->FIELD != NULL) {				\
		dns_c_printtabs(fp, indent + 1);		\
		fprintf(fp, "%s %lu;\n",NAME,			\
			(unsigned long)(*options->FIELD / 60));	\
	}

#define PRINT_AS_DAYS(FIELD, NAME)				\
	if (options->FIELD != NULL) {				\
		dns_c_printtabs(fp, indent + 1);		\
		fprintf(fp, "%s %lu;\n",NAME,			\
			(unsigned long)(*options->FIELD / (24 * 60 * 60))); \
	}

#define PRINT_IF_EQUAL(VAL, STRVAL, FIELD, NAME)		\
	if (options->FIELD != NULL) {				\
		if ((*options->FIELD) == (VAL)) {		\
			dns_c_printtabs(fp, indent + 1);	\
			fprintf(fp, "%s %s;\n", NAME, STRVAL);	\
		}						\
	}

#define PRINT_AS_BOOLEAN(FIELD, NAME)				\
	if (options->FIELD != NULL) {				\
		dns_c_printtabs(fp, indent + 1);		\
		fprintf(fp, "%s %s;\n",NAME,			\
			(*options->FIELD ? "true" : "false"));	\
	}

#define PRINT_AS_SIZE_CLAUSE(FIELD, NAME)				\
	if (options->FIELD != NULL) {					\
		dns_c_printtabs(fp, indent + 1);			\
		fprintf(fp, "%s ",NAME);				\
		if (*options->FIELD == DNS_C_SIZE_SPEC_DEFAULT) {	\
			fprintf(fp, "default");				\
		} else {						\
			dns_c_printinunits(fp, *options->FIELD);	\
		}							\
		fprintf(fp, ";\n");					\
	}

#define PRINT_CHAR_P(FIELD, NAME)					\
	if (options->FIELD != NULL) {					\
		dns_c_printtabs(fp, indent + 1);			\
		fprintf(fp, "%s \"%s\";\n", NAME, options->FIELD);	\
	}

#define PRINT_IPANDPORT(FIELD, NAME)				\
	if (options->FIELD != NULL) {				\
		port = isc_sockaddr_getport(options->FIELD);	\
								\
		dns_c_printtabs(fp, indent + 1);		\
		fprintf(fp, "%s ", NAME);			\
								\
		dns_c_print_ipaddr(fp, options->FIELD);		\
								\
		if (port == 0) {				\
			fprintf(fp, " port *");			\
		} else {					\
			fprintf(fp, " port %d", port);		\
		}						\
		fprintf(fp, " ;\n");				\
	}

#define PRINT_QUERYSOURCE(FIELD, NAME)				\
	if (options->FIELD != NULL) {				\
		port = isc_sockaddr_getport(options->FIELD);	\
								\
		dns_c_printtabs(fp, indent + 1);		\
		fprintf(fp, "%s address ", NAME);		\
								\
		dns_c_print_ipaddr(fp, options->FIELD);		\
								\
		if (port == 0) {				\
			fprintf(fp, " port *");			\
		} else {					\
			fprintf(fp, " port %d", port);		\
		}						\
		fprintf(fp, " ;\n");				\
	}

#define	 PRINT_IP(FIELD, NAME)				\
	if (options->FIELD != NULL) {			\
		dns_c_printtabs(fp, indent + 1);	\
		fprintf(fp, NAME " ");			\
		dns_c_print_ipaddr(fp, options->FIELD);	\
		fprintf(fp, ";\n");			\
	}

#define PRINT_CHECKNAME(INDEX)						    \
	if (options->check_names[INDEX] != NULL) {			    \
		nameseverity = *options->check_names[INDEX];		    \
		dns_c_printtabs(fp, indent + 1);			    \
		fprintf(fp, "check-names %s %s;\n",			    \
			dns_c_transport2string(INDEX, ISC_TRUE),	    \
			dns_c_nameseverity2string(nameseverity, ISC_TRUE)); \
	}


#define PRINT_IPMLIST(FIELD, NAME)					 \
	if (options->FIELD != NULL) {					 \
		dns_c_printtabs(fp, indent + 1);			 \
		fprintf(fp, NAME " ");					 \
		dns_c_ipmatchlist_print(fp, indent + 2, options->FIELD); \
		fprintf(fp, ";\n");					 \
	}


	dns_c_printtabs(fp, indent);
	fprintf (fp, "options {\n");

	PRINT_CHAR_P(version, "version");
	PRINT_CHAR_P(directory, "directory");
	PRINT_CHAR_P(dump_filename, "dump-file");
	PRINT_CHAR_P(pid_filename, "pid-file");
	PRINT_CHAR_P(stats_filename, "statistics-file");
	PRINT_CHAR_P(memstats_filename, "memstatistics-file");
	PRINT_CHAR_P(cache_filename, "cache-file");
	PRINT_CHAR_P(named_xfer, "named-xfer");
	PRINT_CHAR_P(random_device, "random-device");
	PRINT_CHAR_P(random_seed_file, "random-seed-file");

	PRINT_INTEGER(port, "port");

	PRINT_INTEGER(transfers_in, "transfers-in");
	PRINT_INTEGER(transfers_per_ns, "transfers-per-ns");
	PRINT_INTEGER(transfers_out, "transfers-out");
	PRINT_INTEGER(max_log_size_ixfr, "max-ixfr-log-size");


	PRINT_AS_MINUTES(clean_interval, "cleaning-interval");
	PRINT_AS_MINUTES(interface_interval, "interface-interval");
	PRINT_AS_MINUTES(stats_interval, "statistics-interval");
	PRINT_AS_MINUTES(heartbeat_interval, "heartbeat-interval");

	PRINT_AS_MINUTES(max_transfer_time_in, "max-transfer-time-in");
	PRINT_AS_MINUTES(max_transfer_time_out, "max-transfer-time-out");
	PRINT_AS_MINUTES(max_transfer_idle_in, "max-transfer-idle-in");
	PRINT_AS_MINUTES(max_transfer_idle_out, "max-transfer-idle-out");

	PRINT_INTEGER(lamettl, "lame-ttl");
	PRINT_INTEGER(tcp_clients, "tcp-clients");
	PRINT_INTEGER(recursive_clients, "recursive-clients");
	PRINT_INTEGER(min_roots, "min-roots");
	PRINT_INTEGER(serial_queries, "serial-queries");
	PRINT_AS_DAYS(sig_valid_interval, "sig-validity-interval");

	PRINT_INTEGER(min_retry_time, "min-retry-time");
	PRINT_INTEGER(max_retry_time, "max-retry-time");
	PRINT_INTEGER(min_refresh_time, "min-refresh-time");
	PRINT_INTEGER(max_refresh_time, "max-refresh-time");

	PRINT_AS_SIZE_CLAUSE(max_cache_size, "max-cache-size");

	PRINT_AS_SIZE_CLAUSE(data_size, "datasize");
	PRINT_AS_SIZE_CLAUSE(stack_size, "stacksize");
	PRINT_AS_SIZE_CLAUSE(core_size, "coresize");
	PRINT_AS_SIZE_CLAUSE(files, "files");

	PRINT_INTEGER(max_ncache_ttl, "max-ncache-ttl");
	PRINT_INTEGER(max_cache_ttl, "max-cache-ttl");

	PRINT_AS_BOOLEAN(expert_mode, "expert-mode");
	PRINT_AS_BOOLEAN(fake_iquery, "fake-iquery");
	PRINT_AS_BOOLEAN(recursion, "recursion");
	PRINT_AS_BOOLEAN(fetch_glue, "fetch-glue");
	PRINT_IF_EQUAL(dns_notifytype_no, "no", notify, "notify");
	PRINT_IF_EQUAL(dns_notifytype_yes, "yes", notify, "notify");
	PRINT_IF_EQUAL(dns_notifytype_explicit, "explicit", notify, "notify");
	PRINT_AS_BOOLEAN(host_statistics, "host-statistics");
	PRINT_AS_BOOLEAN(dealloc_on_exit, "deallocate-on-exit");
	PRINT_AS_BOOLEAN(use_ixfr, "use-ixfr");
	PRINT_AS_BOOLEAN(maintain_ixfr_base, "maintain-ixfr-base");
	PRINT_AS_BOOLEAN(has_old_clients, "has-old-clients");
	PRINT_AS_BOOLEAN(auth_nx_domain, "auth-nxdomain");
	PRINT_AS_BOOLEAN(multiple_cnames, "multiple-cnames");
	PRINT_AS_BOOLEAN(use_id_pool, "use-id-pool");
	PRINT_AS_BOOLEAN(statistics, "statistics");
	PRINT_IF_EQUAL(dns_dialuptype_no, "no", dialup, "dialup");
	PRINT_IF_EQUAL(dns_dialuptype_yes, "yes", dialup, "dialup");
	PRINT_IF_EQUAL(dns_dialuptype_notify, "notify", dialup, "dialup");
	PRINT_IF_EQUAL(dns_dialuptype_refresh, "refresh", dialup, "dialup");
	PRINT_IF_EQUAL(dns_dialuptype_passive, "passive", dialup, "dialup");
	PRINT_IF_EQUAL(dns_dialuptype_notifypassive, "notify-passive",
		       dialup, "dialup");
	PRINT_AS_BOOLEAN(rfc2308_type1, "rfc2308-type1");
	PRINT_AS_BOOLEAN(request_ixfr, "request-ixfr");
	PRINT_AS_BOOLEAN(provide_ixfr, "provide-ixfr");
	PRINT_AS_BOOLEAN(treat_cr_as_space, "treat-cr-as-space");
	PRINT_AS_BOOLEAN(additional_from_auth, "additional-from-auth");
	PRINT_AS_BOOLEAN(additional_from_cache, "additional-from-cache");

	if (options->transfer_format != NULL) {
		dns_c_printtabs(fp, indent + 1);
		fprintf(fp, "transfer-format %s;\n",
			dns_c_transformat2string(*options->transfer_format,
						 ISC_TRUE));
	}

	PRINT_IPANDPORT(notify_source, "notify-source");
	PRINT_IPANDPORT(notify_source_v6, "notify-source-v6");

	PRINT_IPANDPORT(transfer_source, "transfer-source");
	PRINT_IPANDPORT(transfer_source_v6, "transfer-source-v6");

	PRINT_QUERYSOURCE(query_source, "query-source");
	PRINT_QUERYSOURCE(query_source_v6, "query-source-v6");

	if (options->additional_data != NULL) {
		dns_c_printtabs(fp, indent + 1);
		fprintf(fp, "additional-data %s;\n",
			dns_c_addata2string(*options->additional_data,
					    ISC_TRUE));
	}

	PRINT_CHECKNAME(dns_trans_primary);
	PRINT_CHECKNAME(dns_trans_secondary);
	PRINT_CHECKNAME(dns_trans_response);

	fprintf(fp, "\n");

	PRINT_IPMLIST(queryacl, "allow-notify");
	PRINT_IPMLIST(queryacl, "allow-query");
	PRINT_IPMLIST(transferacl, "allow-transfer");
	PRINT_IPMLIST(recursionacl, "allow-recursion");
	PRINT_IPMLIST(blackhole, "blackhole");
	PRINT_IPMLIST(topology, "topology");
	PRINT_IPMLIST(sortlist, "sortlist");
	PRINT_IPMLIST(allowupdateforwarding, "allow-update-forwarding");

	if (options->listens != NULL) {
		dns_c_lstnlist_print(fp, indent + 1,
				     options->listens,
				     defport);
	}

	if (options->v6listens != NULL) {
		dns_c_lstnlistv6_print(fp, indent + 1,
				       options->v6listens,
				       defport);
	}

	dns_c_ctx_forwarderprint(fp, indent + 1, options);

	if (options->ordering != NULL) {
		dns_c_rrsolist_print(fp, indent + 1, options->ordering);
	}

	if (options->also_notify != NULL) {
		dns_c_printtabs(fp, indent + 1);
		fprintf(fp, "also-notify ") ;
		dns_c_iplist_printfully(fp, indent + 2, ISC_TRUE,
					options->also_notify);
		fprintf(fp, ";\n");
	}

	PRINT_CHAR_P(tkeydomain, "tkey-domain");

	if (options->tkeydhkeycp != NULL) {
		dns_c_printtabs(fp, indent + 1);
		fprintf(fp, "tkey-dhkey \"%s\" %d ;\n",
			options->tkeydhkeycp, options->tkeydhkeyi);
	}

	PRINT_CHAR_P(tkeygsscred, "tkey-gssapi-credential");

	dns_c_printtabs(fp, indent);
	fprintf(fp,"};\n");

#undef PRINT_INTEGER
#undef PRINT_AS_MINUTES
#undef PRINT_AS_BOOLEAN
#undef PRINT_AS_SIZE_CLAUSE
#undef PRINT_CHAR_P
#undef PRINT_IPMLIST
#undef PRINT_IPANDPORT
#undef PRINT_IP
#undef PRINT_CHECKNAME

}

void
dns_c_ctx_forwarderprint(FILE *fp, int indent, dns_c_options_t *options)
{
	REQUIRE(fp != NULL);
	REQUIRE(indent >= 0);

	if (options == NULL) {
		return;
	}

	REQUIRE(DNS_C_CONFOPT_VALID(options));

	if (options->forward != NULL) {
		dns_c_printtabs(fp, indent);
		fprintf(fp, "forward %s;\n",
			dns_c_forward2string(*options->forward, ISC_TRUE));
	}

	if (options->forwarders != NULL) {
		dns_c_printtabs(fp, indent);
		fprintf(fp, "forwarders ");
		dns_c_iplist_print(fp, indent + 1,
				   options->forwarders);
		fprintf(fp, ";\n");
	}
}




isc_result_t
dns_c_ctx_getoptions(dns_c_ctx_t *cfg, dns_c_options_t **options)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(options != NULL);

	if (cfg->options != NULL) {
		REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));
	}

	*options = cfg->options;

	return (cfg->options == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}

isc_result_t
dns_c_ctx_unsetoptions(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));

	dns_c_ctx_optionsdelete(&cfg->options);

	return (ISC_R_SUCCESS);
}




isc_result_t
dns_c_ctx_getlogging(dns_c_ctx_t *cfg, dns_c_logginglist_t **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	*retval = cfg->logging;

	return (cfg->logging == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_setlogging(dns_c_ctx_t *cfg, dns_c_logginglist_t *newval,
		     isc_boolean_t deepcopy)
{
	dns_c_logginglist_t *ll;
	isc_result_t res;
	isc_boolean_t existed;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	existed = ISC_TF(cfg->logging != NULL);

	if (deepcopy) {
		res = dns_c_logginglist_copy(cfg->mem, &ll, newval);
		if (res != ISC_R_SUCCESS) {
			return (res);
		}
	} else {
		ll = newval;
	}

	cfg->logging = ll;

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_unsetlogging(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->logging == NULL) {
		return (ISC_R_NOTFOUND);
	}

	return (dns_c_logginglist_delete(&cfg->logging));
}



isc_result_t
dns_c_ctx_getkdeflist(dns_c_ctx_t *cfg,
                      dns_c_kdeflist_t **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
        REQUIRE(retval != NULL);

	*retval = cfg->keydefs;

        if (cfg->keydefs == NULL) {
		return (ISC_R_NOTFOUND);
	} else {
		return (ISC_R_SUCCESS);
	}
}


isc_result_t
dns_c_ctx_setkdeflist(dns_c_ctx_t *cfg,
		      dns_c_kdeflist_t *newval, isc_boolean_t deepcopy)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->keydefs != NULL) {
		dns_c_kdeflist_delete(&cfg->keydefs);
	}

	if (newval == NULL) {
		cfg->keydefs = NULL;
		res = ISC_R_SUCCESS;
	} else if (deepcopy) {
		res = dns_c_kdeflist_copy(cfg->mem,
					  &cfg->keydefs, newval);
	} else {
		cfg->keydefs = newval;
		res = ISC_R_SUCCESS;
	}

	return (res);
}


isc_result_t
dns_c_ctx_addfile_channel(dns_c_ctx_t *cfg, const char *name,
			  dns_c_logchan_t **chan)
{
	dns_c_logchan_t *newc;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(name != NULL);
	REQUIRE(chan != NULL);
	REQUIRE(cfg->logging != NULL);

	res = dns_c_logchan_new(cfg->mem, name, dns_c_logchan_file,
				&newc);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	res = dns_c_logginglist_addchannel(cfg->logging, newc,
					   ISC_FALSE);

	*chan = newc;

	return (res);
}


isc_result_t
dns_c_ctx_addsyslogchannel(dns_c_ctx_t *cfg, const char *name,
			   dns_c_logchan_t **chan)
{
	dns_c_logchan_t *newc;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(name != NULL);
	REQUIRE(chan != NULL);
	REQUIRE(cfg->logging != NULL);

	res = dns_c_logchan_new(cfg->mem, name,
				dns_c_logchan_syslog, &newc);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	res = dns_c_logginglist_addchannel(cfg->logging, newc,
					   ISC_FALSE);

	*chan = newc;

	return (res);
}


isc_result_t
dns_c_ctx_addnullchannel(dns_c_ctx_t *cfg, const char *name,
			 dns_c_logchan_t **chan)
{
	dns_c_logchan_t *newc;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(name != NULL);
	REQUIRE(chan != NULL);
	REQUIRE(cfg->logging != NULL);

	res = dns_c_logchan_new(cfg->mem, name, dns_c_logchan_null,
				&newc);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	res = dns_c_logginglist_addchannel(cfg->logging, newc,
					   ISC_FALSE);

	*chan = newc;

	return (res);
}


isc_result_t
dns_c_ctx_addstderrchannel(dns_c_ctx_t *cfg, const char *name,
                           dns_c_logchan_t **chan)
{
	dns_c_logchan_t *newc;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(name != NULL);
	REQUIRE(chan != NULL);
	REQUIRE(cfg->logging != NULL);

	res = dns_c_logchan_new(cfg->mem, name, dns_c_logchan_stderr, &newc);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	res = dns_c_logginglist_addchannel(cfg->logging, newc, ISC_FALSE);

	*chan = newc;

	return (res);
}


isc_result_t
dns_c_ctx_addcategory(dns_c_ctx_t *cfg, const char *catname,
		      dns_c_logcat_t **newcat)
{
	dns_c_logcat_t *newc;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(newcat != NULL);
	REQUIRE(cfg->logging != NULL);

	res = dns_c_logcat_new(cfg->mem, catname, &newc);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	res = dns_c_logginglist_addcategory(cfg->logging, newc,
					    ISC_FALSE);

	*newcat = newc;

	return (res);
}


isc_result_t
dns_c_ctx_currchannel(dns_c_ctx_t *cfg, dns_c_logchan_t **channel)
{
	dns_c_logchan_t *newc;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(channel != NULL);
	REQUIRE(cfg->logging != NULL);

	newc = ISC_LIST_TAIL(cfg->logging->channels);

	*channel = newc;

	return (newc == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}


isc_boolean_t
dns_c_ctx_channeldefinedp(dns_c_ctx_t *cfg, const char *name)
{
	isc_result_t res;
	dns_c_logchan_t *chan;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(name != NULL);

	res = dns_c_logginglist_chanbyname(cfg->logging, name, &chan);

	return (ISC_TF(res == ISC_R_SUCCESS));
}



isc_result_t
dns_c_ctx_currcategory(dns_c_ctx_t *cfg, dns_c_logcat_t **category)
{
	dns_c_logcat_t *newc;
	dns_c_logginglist_t *llist;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(category != NULL);

	res = dns_c_ctx_getlogging(cfg, &llist);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	newc = ISC_LIST_TAIL(llist->categories);

	*category = newc;

	return (newc == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}



/* ***************************************************************** */
/* ***********                    OPTIONS                *********** */
/* ***************************************************************** */


isc_result_t
dns_c_ctx_optionsnew(isc_mem_t *mem, dns_c_options_t **options)
{
	dns_c_options_t *opts = NULL;

	REQUIRE(mem != NULL);
	REQUIRE(options != NULL);

	*options = NULL;

	opts = isc_mem_get(mem, sizeof *opts);
	if (opts == NULL) {
		return (ISC_R_NOMEMORY);
	}

	opts->mem = mem;
	opts->magic = DNS_C_OPTION_MAGIC;

	opts->directory = NULL;
	opts->version = NULL;
	opts->dump_filename = NULL;
	opts->pid_filename = NULL;
	opts->stats_filename = NULL;
	opts->memstats_filename = NULL;
	opts->cache_filename = NULL;
	opts->named_xfer = NULL;
	opts->random_device = NULL;
	opts->random_seed_file = NULL;

	opts->port = NULL;

	opts->transfers_in = NULL;
	opts->transfers_per_ns = NULL;
	opts->transfers_out = NULL;
	opts->max_log_size_ixfr = NULL;
	opts->clean_interval = NULL;
	opts->interface_interval = NULL;
	opts->stats_interval = NULL;
	opts->heartbeat_interval = NULL;

	opts->max_transfer_time_in = NULL;
	opts->max_transfer_time_out = NULL;
	opts->max_transfer_idle_in = NULL;
	opts->max_transfer_idle_out = NULL;
	opts->lamettl = NULL;
	opts->tcp_clients = NULL;
	opts->recursive_clients = NULL;
	opts->min_roots = NULL;
	opts->serial_queries = NULL;
	opts->sig_valid_interval = NULL;

	opts->data_size = NULL;
	opts->stack_size = NULL;
	opts->core_size = NULL;
	opts->files = NULL;
	opts->max_cache_size = NULL;
	opts->max_ncache_ttl = NULL;
	opts->max_cache_ttl = NULL;

	opts->min_retry_time = NULL;
	opts->max_retry_time = NULL;
	opts->min_refresh_time = NULL;
	opts->max_refresh_time = NULL;

	opts->expert_mode = NULL;
	opts->fake_iquery = NULL;
	opts->recursion = NULL;
	opts->fetch_glue = NULL;
	opts->notify = NULL;
	opts->host_statistics = NULL;
	opts->dealloc_on_exit = NULL;
	opts->use_ixfr = NULL;
	opts->maintain_ixfr_base = NULL;
	opts->has_old_clients = NULL;
	opts->auth_nx_domain = NULL;
	opts->multiple_cnames = NULL;
	opts->use_id_pool = NULL;
	opts->dialup = NULL;
	opts->statistics = NULL;
	opts->rfc2308_type1 = NULL;
	opts->request_ixfr = NULL;
	opts->provide_ixfr = NULL;
	opts->treat_cr_as_space = NULL;
	opts->additional_from_auth = NULL;
	opts->additional_from_cache = NULL;

	opts->notify_source = NULL;
	opts->notify_source_v6 = NULL;
	opts->transfer_source = NULL;
	opts->transfer_source_v6 = NULL;
	opts->query_source = NULL;
	opts->query_source_v6 = NULL;

	opts->additional_data = NULL;
	opts->forward = NULL;

	opts->tkeydhkeycp = NULL;
	opts->tkeydhkeyi = 0;
	opts->tkeydomain = NULL;
	opts->tkeygsscred = NULL;

	opts->also_notify = NULL;

	opts->check_names[dns_trans_primary] = NULL;
	opts->check_names[dns_trans_secondary] = NULL;
	opts->check_names[dns_trans_response] = NULL;

	opts->transfer_format = NULL;

	opts->notifyacl = NULL;
	opts->queryacl = NULL;
	opts->transferacl = NULL;
	opts->recursionacl = NULL;
	opts->blackhole = NULL;
	opts->topology = NULL;
	opts->sortlist = NULL;
	opts->allowupdateforwarding = NULL;

	opts->listens = NULL;
	opts->v6listens = NULL;

	opts->ordering = NULL;

	opts->forwarders = NULL;

	*options = opts;

	return (ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_optionsdelete(dns_c_options_t **opts)
{
	dns_c_options_t *options;
	isc_result_t r, result;

	REQUIRE(opts != NULL);

	options = *opts;
	if (options == NULL) {
		return (ISC_R_SUCCESS);
	}

	REQUIRE(DNS_C_CONFOPT_VALID(options));


#define FREEFIELD(FIELD)					\
	do { if (options->FIELD != NULL) {			\
		isc_mem_put(options->mem, options->FIELD,	\
			    sizeof (*options->FIELD));		\
		options->FIELD = NULL;				\
	} } while (0)

#define FREESTRING(FIELD)					\
        do { if (options->FIELD != NULL) {			\
		isc_mem_free(options->mem, options->FIELD);	\
	} } while (0)

#define FREEIPMLIST(FIELD)						\
	do { if (options->FIELD != NULL) {				\
		(void)dns_c_ipmatchlist_detach(&options->FIELD);	\
	} } while (0)



	FREESTRING(directory);
	FREESTRING(version);
	FREESTRING(dump_filename);
	FREESTRING(pid_filename);
	FREESTRING(stats_filename);
	FREESTRING(memstats_filename);
	FREESTRING(cache_filename);
	FREESTRING(named_xfer);
	FREESTRING(random_device);
	FREESTRING(random_seed_file);


	FREEFIELD(expert_mode);
	FREEFIELD(fake_iquery);
	FREEFIELD(recursion);
	FREEFIELD(fetch_glue);
	FREEFIELD(notify);
	FREEFIELD(host_statistics);
	FREEFIELD(dealloc_on_exit);
	FREEFIELD(use_ixfr);
	FREEFIELD(maintain_ixfr_base);
	FREEFIELD(has_old_clients);
	FREEFIELD(auth_nx_domain);
	FREEFIELD(multiple_cnames);
	FREEFIELD(use_id_pool);
	FREEFIELD(dialup);
	FREEFIELD(statistics);
	FREEFIELD(rfc2308_type1);
	FREEFIELD(request_ixfr);
	FREEFIELD(provide_ixfr);
	FREEFIELD(treat_cr_as_space);
	FREEFIELD(additional_from_cache);
	FREEFIELD(additional_from_auth);

	FREEFIELD(port);

	FREEFIELD(transfers_in);
	FREEFIELD(transfers_per_ns);
	FREEFIELD(transfers_out);
	FREEFIELD(max_log_size_ixfr);
	FREEFIELD(clean_interval);
	FREEFIELD(interface_interval);
	FREEFIELD(stats_interval);
	FREEFIELD(heartbeat_interval);
	FREEFIELD(max_transfer_time_in);
	FREEFIELD(max_transfer_time_out);
	FREEFIELD(max_transfer_idle_in);
	FREEFIELD(max_transfer_idle_out);
	FREEFIELD(lamettl);
	FREEFIELD(tcp_clients);
	FREEFIELD(recursive_clients);
	FREEFIELD(min_roots);
	FREEFIELD(serial_queries);
	FREEFIELD(sig_valid_interval);


	FREEFIELD(data_size);
	FREEFIELD(stack_size);
	FREEFIELD(core_size);
	FREEFIELD(files);
	FREEFIELD(max_cache_size);
	FREEFIELD(max_ncache_ttl);
	FREEFIELD(max_cache_ttl);

	FREEFIELD(min_retry_time);
	FREEFIELD(max_retry_time);
	FREEFIELD(min_refresh_time);
	FREEFIELD(max_refresh_time);


	FREEFIELD(notify_source);
	FREEFIELD(notify_source_v6);
	FREEFIELD(transfer_source);
	FREEFIELD(transfer_source_v6);
	FREEFIELD(query_source);
	FREEFIELD(query_source_v6);

	FREEFIELD(additional_data);
	FREEFIELD(forward);

	FREESTRING(tkeydomain);
	FREESTRING(tkeydhkeycp);
	FREESTRING(tkeygsscred);

	if (options->also_notify != NULL) {
		dns_c_iplist_detach(&options->also_notify);
	}

	FREEFIELD(check_names[dns_trans_primary]);
	FREEFIELD(check_names[dns_trans_secondary]);
	FREEFIELD(check_names[dns_trans_response]);

	FREEFIELD(transfer_format);

	FREEIPMLIST(notifyacl);
	FREEIPMLIST(queryacl);
	FREEIPMLIST(transferacl);
	FREEIPMLIST(recursionacl);
	FREEIPMLIST(blackhole);
	FREEIPMLIST(topology);
	FREEIPMLIST(sortlist);
	FREEIPMLIST(allowupdateforwarding);

	result = ISC_R_SUCCESS;

	if (options->listens != NULL) {
		r = dns_c_lstnlist_delete(&options->listens);
		if (r != ISC_R_SUCCESS)
			result = r;
	}

	if (options->v6listens != NULL) {
		r = dns_c_lstnlist_delete(&options->v6listens);
		if (r != ISC_R_SUCCESS)
			result = r;
	}

	if (options->ordering != NULL) {
		r = dns_c_rrsolist_delete(&options->ordering);
		if (r != ISC_R_SUCCESS)
			result = r;
	}

	if (options->forwarders != NULL) {
		r = dns_c_iplist_detach(&options->forwarders);
		if (r != ISC_R_SUCCESS)
			result = r;
	}

	*opts = NULL;
	options->magic = 0;

	isc_mem_put(options->mem, options, sizeof *options);

	return (result);

#undef FREEFIELD
#undef FREESTRING
#undef FREEIPMLIST

}

STRING_FUNCS(directory, directory)
STRING_FUNCS(version, version)
STRING_FUNCS(dumpfilename, dump_filename)
STRING_FUNCS(pidfilename, pid_filename)
STRING_FUNCS(statsfilename, stats_filename)
STRING_FUNCS(memstatsfilename, memstats_filename)
STRING_FUNCS(cachefile, cache_filename)
STRING_FUNCS(namedxfer, named_xfer)
STRING_FUNCS(randomdevice, random_device)
STRING_FUNCS(randomseedfile, random_seed_file)

BYTYPE_FUNCS(in_port_t, port, port)

UINT32_FUNCS(transfersin, transfers_in)
UINT32_FUNCS(transfersperns, transfers_per_ns)
UINT32_FUNCS(transfersout, transfers_out)
UINT32_FUNCS(maxlogsizeixfr, max_log_size_ixfr)
UINT32_FUNCS(cleaninterval, clean_interval)
UINT32_FUNCS(interfaceinterval, interface_interval)
UINT32_FUNCS(statsinterval, stats_interval)
UINT32_FUNCS(heartbeatinterval, heartbeat_interval)
UINT32_FUNCS(maxtransfertimein, max_transfer_time_in)
UINT32_FUNCS(maxtransfertimeout, max_transfer_time_out)
UINT32_FUNCS(maxtransferidlein, max_transfer_idle_in)
UINT32_FUNCS(maxtransferidleout, max_transfer_idle_out)
UINT32_FUNCS(lamettl, lamettl)
UINT32_FUNCS(tcpclients, tcp_clients)
UINT32_FUNCS(recursiveclients, recursive_clients)
UINT32_FUNCS(minroots, min_roots)
UINT32_FUNCS(serialqueries, serial_queries)
UINT32_FUNCS(sigvalidityinterval, sig_valid_interval)
UINT32_FUNCS(datasize, data_size)
UINT32_FUNCS(stacksize, stack_size)
UINT32_FUNCS(coresize, core_size)
UINT32_FUNCS(files, files)
UINT32_FUNCS(maxcachesize, max_cache_size)
UINT32_FUNCS(maxncachettl, max_ncache_ttl)
UINT32_FUNCS(maxcachettl, max_cache_ttl)

UINT32_FUNCS(minretrytime, min_retry_time)
UINT32_FUNCS(maxretrytime, max_retry_time)
UINT32_FUNCS(minrefreshtime, min_refresh_time)
UINT32_FUNCS(maxrefreshtime, max_refresh_time)

BOOL_FUNCS(expertmode, expert_mode)
BOOL_FUNCS(fakeiquery, fake_iquery)
BOOL_FUNCS(recursion, recursion)
BOOL_FUNCS(fetchglue, fetch_glue)

NOTIFYTYPE_FUNCS(notify, notify)


BOOL_FUNCS(hoststatistics, host_statistics)
BOOL_FUNCS(dealloconexit, dealloc_on_exit)
BOOL_FUNCS(useixfr, use_ixfr)
BOOL_FUNCS(maintainixfrbase, maintain_ixfr_base)
BOOL_FUNCS(hasoldclients, has_old_clients)
BOOL_FUNCS(authnxdomain, auth_nx_domain)
BOOL_FUNCS(multiplecnames, multiple_cnames)
BOOL_FUNCS(useidpool, use_id_pool)
BOOL_FUNCS(statistics, statistics)
BOOL_FUNCS(rfc2308type1, rfc2308_type1)
BOOL_FUNCS(requestixfr, request_ixfr)
BOOL_FUNCS(provideixfr, provide_ixfr)
BOOL_FUNCS(treatcrasspace, treat_cr_as_space)
BOOL_FUNCS(additionalfromauth, additional_from_auth)
BOOL_FUNCS(additionalfromcache, additional_from_cache)

SOCKADDR_FUNCS(notifysource, notify_source)
SOCKADDR_FUNCS(notifysourcev6, notify_source_v6)
SOCKADDR_FUNCS(transfersource, transfer_source)
SOCKADDR_FUNCS(transfersourcev6, transfer_source_v6)
SOCKADDR_FUNCS(querysource, query_source)
SOCKADDR_FUNCS(querysourcev6, query_source_v6)

BYTYPE_FUNCS(dns_c_forw_t, forward, forward)
BYTYPE_FUNCS(dns_transfer_format_t, transferformat, transfer_format)
BYTYPE_FUNCS(dns_c_addata_t, additionaldata, additional_data)
BYTYPE_FUNCS(dns_dialuptype_t, dialup, dialup)




/*
 * Modifiers for options.
 *
 */



isc_result_t
dns_c_ctx_settkeydomain(dns_c_ctx_t *cfg, const char *newval)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	return (cfg_set_string(cfg->options,
			       &cfg->options->tkeydomain,
			       newval));
}


isc_result_t
dns_c_ctx_settkeydhkey(dns_c_ctx_t *cfg,
		       const char *charval, isc_uint32_t uintval)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	cfg->options->tkeydhkeyi = uintval;
	return (cfg_set_string(cfg->options,
			       &cfg->options->tkeydhkeycp,
			       charval));
}


isc_result_t
dns_c_ctx_settkeygsscred(dns_c_ctx_t *cfg, const char *newval)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	return (cfg_set_string(cfg->options,
			       &cfg->options->tkeygsscred,
			       newval));
}





isc_result_t
dns_c_ctx_setchecknames(dns_c_ctx_t *cfg,
			dns_c_trans_t transtype,
			dns_severity_t newval)
{
	isc_boolean_t existed = ISC_FALSE;
	isc_result_t res;
	dns_severity_t **ptr = NULL;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	switch(transtype) {
	case dns_trans_primary:
	case dns_trans_secondary:
	case dns_trans_response:
		ptr = &cfg->options->check_names[transtype];
		existed = ISC_TF(*ptr != NULL);
		break;

	default:
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_CRITICAL,
			      "bad transport value: %d", transtype);
		return (ISC_R_FAILURE);
	}

	if (!existed) {
		*ptr = isc_mem_get(cfg->options->mem,
				   sizeof (**ptr));
	}

	**ptr = newval;

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_getchecknames(dns_c_ctx_t *cfg,
			dns_c_trans_t transtype,
			dns_severity_t *retval)
{
	isc_result_t result;
	dns_severity_t **ptr = NULL;
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(retval != NULL);

	switch (transtype) {
	case dns_trans_primary:
	case dns_trans_secondary:
	case dns_trans_response:
		ptr = &cfg->options->check_names[transtype];
		break;

	default:
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_CRITICAL,
			      "bad transport value: %d", transtype);
		return (ISC_R_FAILURE);
	}

	if (*ptr != NULL) {
		*retval = *cfg->options->check_names[transtype];
		result = ISC_R_SUCCESS;
	} else {
		result = ISC_R_NOTFOUND;
	}

	return (result);
}


isc_result_t
dns_c_ctx_unsetchecknames(dns_c_ctx_t *cfg,
			  dns_c_trans_t transtype)
{
	isc_result_t res;
	dns_severity_t **ptr = NULL;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	switch(transtype) {
	case dns_trans_primary:
	case dns_trans_secondary:
	case dns_trans_response:
		ptr = &cfg->options->check_names[transtype];
		break;

	default:
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_CONFIG,
			      DNS_LOGMODULE_CONFIG, ISC_LOG_CRITICAL,
			      "bad transport value: %d", transtype);
		return (ISC_R_FAILURE);
	}

	if (*ptr == NULL) {
		return (ISC_R_NOTFOUND);
	}

	isc_mem_put(cfg->options->mem, *ptr, sizeof (**ptr));

	return (ISC_R_SUCCESS);
}

IPMLIST_FUNCS(allownotify, notifyacl)
IPMLIST_FUNCS(allowquery, queryacl)
IPMLIST_FUNCS(allowtransfer, transferacl)
IPMLIST_FUNCS(allowrecursion, recursionacl)
IPMLIST_FUNCS(blackhole, blackhole)
IPMLIST_FUNCS(topology, topology)
IPMLIST_FUNCS(sortlist, sortlist)
IPMLIST_FUNCS(allowupdateforwarding, allowupdateforwarding)



isc_result_t
dns_c_ctx_setrrsetorderlist(dns_c_ctx_t *cfg, isc_boolean_t copy,
			    dns_c_rrsolist_t *olist)
{
	isc_boolean_t existed;
	dns_c_options_t *opts;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	opts = cfg->options;

	existed = (opts->ordering == NULL ? ISC_FALSE : ISC_TRUE);

	if (copy) {
		if (opts->ordering == NULL) {
			res = dns_c_rrsolist_new(opts->mem,
						 &opts->ordering);
			if (res != ISC_R_SUCCESS) {
				return (res);
			}
		} else {
			dns_c_rrsolist_clear(opts->ordering);
		}

		res = dns_c_rrsolist_append(opts->ordering, olist);
	} else {
		if (opts->ordering != NULL) {
			dns_c_rrsolist_delete(&opts->ordering);
		}

		opts->ordering = olist;
		res = ISC_R_SUCCESS;
	}

	if (res == ISC_R_SUCCESS && existed) {
		res = ISC_R_EXISTS;
	}

	return (res);
}


isc_result_t
dns_c_ctx_settrustedkeys(dns_c_ctx_t *cfg, dns_c_tkeylist_t *list,
			 isc_boolean_t copy)
{
	isc_boolean_t existed;
	dns_c_tkeylist_t *newl;
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	existed = (cfg->trusted_keys == NULL ? ISC_FALSE : ISC_TRUE);

	if (cfg->trusted_keys != NULL) {
		res = dns_c_tkeylist_delete(&cfg->trusted_keys);
		if (res != ISC_R_SUCCESS) {
			return (res);
		}
	}

	if (copy) {
		res = dns_c_tkeylist_copy(cfg->mem, &newl, list);
		if (res != ISC_R_SUCCESS) {
			return (res);
		}
	} else {
		newl = list;
	}

	cfg->trusted_keys = newl;

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}


/**
 ** Accessors
 **/


isc_result_t
dns_c_ctx_gettkeydomain(dns_c_ctx_t *cfg, char **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(retval != NULL);

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));

	*retval = cfg->options->tkeydomain;

	return (*retval == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_gettkeydhkey(dns_c_ctx_t *cfg,
		       char **charpval, isc_uint32_t *uintval)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(charpval != NULL);
	REQUIRE(uintval != NULL);

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));

	if (cfg->options->tkeydhkeycp == NULL) {
		res = ISC_R_NOTFOUND;
	} else {
		*charpval = cfg->options->tkeydhkeycp;
		*uintval = cfg->options->tkeydhkeyi;
		res = ISC_R_SUCCESS;
	}

	return (res);
}


isc_result_t
dns_c_ctx_gettkeygsscred(dns_c_ctx_t *cfg, char **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(retval != NULL);

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));

	*retval = cfg->options->tkeygsscred;

	return (*retval == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}



isc_result_t
dns_c_ctx_addlisten_on(dns_c_ctx_t *cfg, in_port_t port,
		       dns_c_ipmatchlist_t *ml,
		       isc_boolean_t copy)
{
	dns_c_lstnon_t *lo;
	isc_result_t res;
	dns_c_options_t *opts;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	opts = cfg->options;

	if (opts->listens == NULL) {
		res = dns_c_lstnlist_new(cfg->mem, &opts->listens);
		if (res != ISC_R_SUCCESS) {
			return (res);
		}
	}

#if 0
	lo = ISC_LIST_HEAD(opts->listens->elements);
	while (lo != NULL) {
		/* XXX we should probably check that a listen on statement
		 * hasn't been done for the same post, ipmatch list
		 * combination
		 */
		if (lo->port == port) { /* XXX incomplete */
			return (ISC_R_FAILURE);
		}
		lo = ISC_LIST_NEXT(lo, next);
	}
#endif

	res = dns_c_lstnon_new(cfg->mem, &lo);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	lo->port = port;
	res = dns_c_lstnon_setiml(lo, ml, copy);

	ISC_LIST_APPEND(opts->listens->elements, lo, next);

	return (res);
}



isc_result_t
dns_c_ctx_getlistenlist(dns_c_ctx_t *cfg, dns_c_lstnlist_t **ll)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(ll != NULL);

	*ll = NULL;

	if (cfg->options->listens != NULL) {
		*ll = cfg->options->listens;
	}

	return (*ll == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}




isc_result_t
dns_c_ctx_addv6listen_on(dns_c_ctx_t *cfg, in_port_t port,
			 dns_c_ipmatchlist_t *ml, isc_boolean_t copy)
{
	dns_c_lstnon_t *lo;
	isc_result_t res;
	dns_c_options_t *opts;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	opts = cfg->options;

	if (opts->v6listens == NULL) {
		res = dns_c_lstnlist_new(cfg->mem, &opts->v6listens);
		if (res != ISC_R_SUCCESS) {
			return (res);
		}
	}

#if 0
	lo = ISC_LIST_HEAD(opts->v6listens->elements);
	while (lo != NULL) {
		/* XXX we should probably check that a listen on statement
		 * hasn't been done for the same post, ipmatch list
		 * combination
		 */
		if (lo->port == port) { /* XXX incomplete */
			return (ISC_R_FAILURE);
		}
		lo = ISC_LIST_NEXT(lo, next);
	}
#endif

	res = dns_c_lstnon_new(cfg->mem, &lo);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	lo->port = port;
	res = dns_c_lstnon_setiml(lo, ml, copy);

	ISC_LIST_APPEND(opts->v6listens->elements, lo, next);

	return (res);
}



isc_result_t
dns_c_ctx_getv6listenlist(dns_c_ctx_t *cfg, dns_c_lstnlist_t **ll)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(ll != NULL);

	*ll = NULL;

	if (cfg->options->v6listens != NULL) {
		*ll = cfg->options->v6listens;
	}

	return (*ll == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}






isc_result_t
dns_c_ctx_setforwarders(dns_c_ctx_t *cfg, isc_boolean_t copy,
			dns_c_iplist_t *ipl)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	res = make_options(cfg);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	res = cfg_set_iplist(cfg->options, &cfg->options->forwarders,
			     ipl, copy);

	return (res);
}


isc_result_t
dns_c_ctx_getforwarders(dns_c_ctx_t *cfg, dns_c_iplist_t **list)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(list != NULL);

	return (cfg_get_iplist(cfg->options,
			       cfg->options->forwarders, list));
}


isc_result_t
dns_c_ctx_unsetforwarders(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	if (cfg->options->forwarders != NULL) {
		return (dns_c_iplist_detach(&cfg->options->forwarders));
	} else {
		return (ISC_R_SUCCESS);
	}
}



isc_result_t
dns_c_ctx_getrrsetorderlist(dns_c_ctx_t *cfg, dns_c_rrsolist_t **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(retval != NULL);

	if (cfg->options == NULL || cfg->options->ordering == NULL) {
		return (ISC_R_NOTFOUND);
	} else {
		*retval = cfg->options->ordering;
		return (ISC_R_SUCCESS);
	}
}


isc_result_t
dns_c_ctx_gettrustedkeys(dns_c_ctx_t *cfg, dns_c_tkeylist_t **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(retval != NULL);

	if (cfg->trusted_keys == NULL) {
		return (ISC_R_NOTFOUND);
	} else {
		*retval = cfg->trusted_keys;
		return (ISC_R_SUCCESS);
	}
}


isc_result_t
dns_c_ctx_getlwres(dns_c_ctx_t *cfg,
                   dns_c_lwreslist_t **retval)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
        REQUIRE(retval != NULL);

	*retval = cfg->lwres;

        if (cfg->lwres == NULL) {
		return (ISC_R_NOTFOUND);
	} else {
		return (ISC_R_SUCCESS);
	}
}


isc_result_t
dns_c_ctx_setlwres(dns_c_ctx_t *cfg,
		   dns_c_lwreslist_t *newval)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->lwres != NULL) {
		dns_c_lwreslist_delete(&cfg->lwres);
	}

	if (newval == NULL) {
		cfg->lwres = NULL;
		res = ISC_R_SUCCESS;
	} else {
		cfg->lwres = newval;
		res = ISC_R_SUCCESS;
	}

	return (res);
}


/*
**
*/

isc_result_t
dns_c_ctx_setalsonotify(dns_c_ctx_t *cfg,
			dns_c_iplist_t *iml)
{
	isc_result_t result;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	result = make_options(cfg);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	REQUIRE(iml != NULL);

	if (cfg->options->also_notify != NULL)
		dns_c_iplist_detach(&cfg->options->also_notify);

	dns_c_iplist_attach(iml, &cfg->options->also_notify);

	return (ISC_R_SUCCESS);
}


isc_result_t
dns_c_ctx_getalsonotify(dns_c_ctx_t *cfg, dns_c_iplist_t **ret)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL || cfg->options->also_notify == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(ret != NULL);

	dns_c_iplist_attach(cfg->options->also_notify, ret);

	return (ISC_R_SUCCESS);
}



isc_result_t
dns_c_ctx_unsetalsonotify(dns_c_ctx_t *cfg)
{
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		return (ISC_R_NOTFOUND);
	}

	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));

	if (cfg->options->also_notify != NULL) {
		dns_c_iplist_detach(&cfg->options->also_notify);
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}


/*
**
*/


isc_boolean_t
dns_c_ctx_keydefinedp(dns_c_ctx_t *cfg, const char *keyname)
{
	dns_c_kdef_t *keyid;
	isc_result_t res;
	isc_boolean_t rval = ISC_FALSE;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(keyname != NULL);

	if (cfg->keydefs != NULL) {
		res = dns_c_kdeflist_find(cfg->keydefs, keyname, &keyid);
		if (res == ISC_R_SUCCESS) {
			rval = ISC_TRUE;
		}
	}

	return rval;
}





/***************************************************************************/


static isc_result_t
cfg_set_string(dns_c_options_t *options, char **field, const char *newval)
{
	char *p;
	isc_boolean_t existed = ISC_FALSE;

	REQUIRE(DNS_C_CONFOPT_VALID(options));
	REQUIRE(field != NULL);

	p = *field;
	*field = NULL;

	if (p != NULL) {
		existed = ISC_TRUE;
	}

	if (newval == NULL) {
		if (p != NULL) {
			isc_mem_free(options->mem, p);
		}
		p = NULL;
	} else if (p == NULL) {
		p = isc_mem_strdup(options->mem, newval);
		if (p == NULL) {
			return (ISC_R_NOMEMORY);
		}
	} else if (strlen(p) >= strlen(newval)) {
		strcpy(p, newval);
	} else {
		isc_mem_free(options->mem, p);
		p = isc_mem_strdup(options->mem, newval);
		if (p == NULL) {
			return (ISC_R_NOMEMORY);
		}
	}

	*field = p;

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}


static isc_result_t
cfg_set_iplist(dns_c_options_t *options,
	       dns_c_iplist_t **fieldaddr,
	       dns_c_iplist_t *newval,
	       isc_boolean_t copy)
{
	isc_result_t res;
	isc_boolean_t existed = ISC_FALSE;

	REQUIRE(DNS_C_CONFOPT_VALID(options));
	REQUIRE(fieldaddr != NULL);
	REQUIRE(newval != NULL);

	if (*fieldaddr != NULL) {
		existed = ISC_TRUE;
	}

	if (copy) {
		if (*fieldaddr != NULL) {
			dns_c_iplist_detach(fieldaddr);
		}

		res = dns_c_iplist_copy(options->mem, fieldaddr,
					newval);
	} else {
		if (*fieldaddr != NULL) {
			res = dns_c_iplist_detach(fieldaddr);
			if (res != ISC_R_SUCCESS) {
				return (res);
			}
		}

		res = ISC_R_SUCCESS;

		*fieldaddr = newval;
	}

	if (res == ISC_R_SUCCESS && existed) {
		res = ISC_R_EXISTS;
	}

	return (res);
}







static isc_result_t
cfg_get_iplist(dns_c_options_t *options,
	       dns_c_iplist_t *field,
	       dns_c_iplist_t **resval)
{
	isc_result_t res;

	UNUSED(options);

	REQUIRE(DNS_C_CONFOPT_VALID(options));
	REQUIRE(resval != NULL);

	if (field != NULL && field->nextidx != 0) {
		dns_c_iplist_attach(field, resval);
		res = ISC_R_SUCCESS;
	} else {
		*resval = NULL;
		res = ISC_R_NOTFOUND;
	}

	return (res);
}



static isc_result_t
acl_init(dns_c_ctx_t *cfg)
{
	isc_result_t r;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	r = dns_c_acltable_new(cfg->mem, &cfg->acls);
	if (r != ISC_R_SUCCESS) return (r);

	return (ISC_R_SUCCESS);
}



static isc_result_t
logging_init (dns_c_ctx_t *cfg)
{
	isc_result_t res;

	REQUIRE(DNS_C_CONFCTX_VALID(cfg));
	REQUIRE(cfg->logging == NULL);

	res = dns_c_logginglist_new(cfg->mem, &cfg->logging);
	if (res != ISC_R_SUCCESS) {
		return (res);
	}

	return (ISC_R_SUCCESS);
}



static isc_result_t
make_options(dns_c_ctx_t *cfg)
{
	isc_result_t res = ISC_R_SUCCESS;
	REQUIRE(DNS_C_CONFCTX_VALID(cfg));

	if (cfg->options == NULL) {
		res = dns_c_ctx_optionsnew(cfg->mem, &cfg->options);
		if (res != ISC_R_SUCCESS) {
			return (res);
		}
	}

	REQUIRE(DNS_C_CONFOPT_VALID(cfg->options));

	return (res);
}
