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

/* $Id: confctx.h,v 1.1.1.1 2001-10-22 13:08:17 ghudson Exp $ */

#ifndef DNS_CONFCTX_H
#define DNS_CONFCTX_H 1

/*****
 ***** Module Info
 *****/

/*
 * Defines the structures and accessor/modifier functions for the top level
 * structures created by the config file parsing routines.
 */

/*
 *
 * MP:
 *
 *
 * Reliability:
 *
 *
 * Resources:
 *
 *
 * Security:
 *
 *
 * Standards:
 *
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/confacl.h>
#include <dns/confcache.h>
#include <dns/confctl.h>
#include <dns/conflog.h>
#include <dns/conflsn.h>
#include <dns/conflwres.h>
#include <dns/confresolv.h>
#include <dns/confview.h>

#define DNS_C_CONFIG_MAGIC		0x434f4e46U /* CONF */
#define DNS_C_OPTION_MAGIC		0x4f707473U /* Opts */

#define DNS_C_CONFCTX_VALID(ptr) ISC_MAGIC_VALID(ptr, DNS_C_CONFIG_MAGIC)
#define DNS_C_CONFOPT_VALID(ptr) ISC_MAGIC_VALID(ptr, DNS_C_OPTION_MAGIC)

/***
 *** Types
 ***/

typedef struct dns_c_options		dns_c_options_t;
typedef struct dns_c_ctx		dns_c_ctx_t;

/*
 * The main baby. A pointer to one of these is what the caller gets back
 * when the parsing routine is called.
 */
struct dns_c_ctx {
	isc_uint32_t		magic;
	isc_mem_t	       *mem;

	int			warnings; /* semantic warning count */
	int			errors;	/* semantic error count */

	dns_c_options_t	       *options;
	dns_c_cache_t	       *cache;
	dns_c_resolv_t	       *resolver;
	dns_c_ctrllist_t       *controls;
	dns_peerlist_t	       *peers;
	dns_c_acltable_t       *acls;
	dns_c_kdeflist_t       *keydefs;
	dns_c_zonelist_t       *zlist;
	dns_c_tkeylist_t       *trusted_keys;
	dns_c_logginglist_t    *logging;
	dns_c_viewtable_t      *views;
	dns_c_lwreslist_t      *lwres;

	dns_c_zone_t	       *currzone;
	dns_c_view_t	       *currview;
};

/*
 * This structure holds all the values defined by a config file 'options'
 * statement. If a field is NULL then it has never been set.
 */
struct dns_c_options {
	isc_uint32_t		magic;
	isc_mem_t	       *mem;

	char		       *directory;
	char		       *version;
	char		       *dump_filename;
	char		       *pid_filename;
	char		       *stats_filename;
	char		       *memstats_filename;
	char		       *cache_filename;
	char		       *named_xfer;
	char		       *random_device;
	char		       *random_seed_file;

	in_port_t 	       *port;

	isc_uint32_t	       *transfers_in;
	isc_uint32_t	       *transfers_per_ns;
	isc_uint32_t	       *transfers_out;
	isc_uint32_t	       *max_log_size_ixfr;
	isc_uint32_t	       *clean_interval;
	isc_uint32_t	       *interface_interval;
	isc_uint32_t	       *stats_interval;
	isc_uint32_t	       *heartbeat_interval;

	isc_uint32_t	       *max_transfer_time_in;
	isc_uint32_t	       *max_transfer_time_out;
	isc_uint32_t	       *max_transfer_idle_in;
	isc_uint32_t	       *max_transfer_idle_out;
	isc_uint32_t	       *lamettl;
	isc_uint32_t	       *tcp_clients;
	isc_uint32_t	       *recursive_clients;
	isc_uint32_t	       *min_roots;
	isc_uint32_t	       *serial_queries;
	isc_uint32_t	       *sig_valid_interval;

	isc_uint32_t	       *data_size;
	isc_uint32_t	       *stack_size;
	isc_uint32_t	       *core_size;
	isc_uint32_t	       *files;
	isc_uint32_t	       *max_cache_size;
	isc_uint32_t	       *max_ncache_ttl;
	isc_uint32_t	       *max_cache_ttl;

	isc_uint32_t	       *min_retry_time;
	isc_uint32_t	       *max_retry_time;
	isc_uint32_t	       *min_refresh_time;
	isc_uint32_t	       *max_refresh_time;

	isc_boolean_t	       *expert_mode;
	isc_boolean_t	       *fake_iquery;
	isc_boolean_t	       *recursion;
	isc_boolean_t	       *fetch_glue;
	isc_boolean_t	       *host_statistics;
	isc_boolean_t	       *dealloc_on_exit;
	isc_boolean_t	       *use_ixfr;
	isc_boolean_t	       *maintain_ixfr_base;
	isc_boolean_t	       *has_old_clients;
	isc_boolean_t	       *auth_nx_domain;
	isc_boolean_t	       *multiple_cnames;
	isc_boolean_t	       *use_id_pool;
	dns_dialuptype_t       *dialup;
	isc_boolean_t	       *statistics;
	isc_boolean_t	       *rfc2308_type1;
	isc_boolean_t	       *request_ixfr;
	isc_boolean_t	       *provide_ixfr;
	isc_boolean_t	       *treat_cr_as_space;
	isc_boolean_t	       *additional_from_cache;
	isc_boolean_t	       *additional_from_auth;

	isc_sockaddr_t	       *notify_source;
	isc_sockaddr_t	       *notify_source_v6;
	isc_sockaddr_t	       *transfer_source;
	isc_sockaddr_t	       *transfer_source_v6;
	isc_sockaddr_t	       *query_source;
	isc_sockaddr_t	       *query_source_v6;

	dns_c_addata_t	       *additional_data;

	dns_c_forw_t	       *forward;

	char 		       *tkeydhkeycp;
	isc_uint32_t		tkeydhkeyi;
	char 		       *tkeydomain;
	char 		       *tkeygsscred;

	dns_notifytype_t       *notify;
	dns_c_iplist_t	       *also_notify;

	dns_severity_t 	       *check_names[DNS_C_TRANSCOUNT];

	dns_transfer_format_t  *transfer_format;

	dns_c_ipmatchlist_t    *notifyacl;
	dns_c_ipmatchlist_t    *queryacl;
	dns_c_ipmatchlist_t    *transferacl;
	dns_c_ipmatchlist_t    *recursionacl;
	dns_c_ipmatchlist_t    *blackhole;
	dns_c_ipmatchlist_t    *topology;
	dns_c_ipmatchlist_t    *sortlist;
	dns_c_ipmatchlist_t    *allowupdateforwarding;

	dns_c_lstnlist_t       *listens;
	dns_c_lstnlist_t       *v6listens;

	dns_c_rrsolist_t       *ordering;

	dns_c_iplist_t	       *forwarders;
};

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t dns_c_checkconfig(dns_c_ctx_t *ctx);

isc_result_t dns_c_ctx_new(isc_mem_t *mem, dns_c_ctx_t **cfg);
isc_result_t dns_c_ctx_delete(dns_c_ctx_t **cfg);
void dns_c_ctx_print(FILE *fp, int indent, dns_c_ctx_t *cfg);

void dns_c_ctx_optionsprint(FILE *fp, int indent,
			    dns_c_options_t *options);
void dns_c_ctx_forwarderprint(FILE *fp, int indent, dns_c_options_t *options);

isc_result_t dns_c_ctx_setcurrzone(dns_c_ctx_t *cfg, dns_c_zone_t *zone);
dns_c_zone_t *dns_c_ctx_getcurrzone(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_setcurrview(dns_c_ctx_t *cfg, dns_c_view_t *view);
dns_c_view_t *dns_c_ctx_getcurrview(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_getoptions(dns_c_ctx_t *cfg, dns_c_options_t **options);
isc_result_t dns_c_ctx_unsetoptions(dns_c_ctx_t *cfg);

/* detach when done with retval */
isc_result_t dns_c_ctx_getpeerlist(dns_c_ctx_t *cfg, dns_peerlist_t **retval);

/* cfg will attach to newval */
isc_result_t dns_c_ctx_setpeerlist(dns_c_ctx_t *cfg, dns_peerlist_t *newval);
isc_result_t dns_c_ctx_unsetpeerlist(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_getcontrols(dns_c_ctx_t *cfg, dns_c_ctrllist_t **ctrls);
isc_result_t dns_c_ctx_setcontrols(dns_c_ctx_t *cfg, dns_c_ctrllist_t *ctrls);
/* XXX need unsetcontrols */

isc_result_t dns_c_ctx_setlogging(dns_c_ctx_t *cfg,
				  dns_c_logginglist_t *newval,
				  isc_boolean_t deepcopy);
isc_result_t dns_c_ctx_getlogging(dns_c_ctx_t *cfg,
				  dns_c_logginglist_t **retval);
isc_result_t dns_c_ctx_unsetlogging(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_addfile_channel(dns_c_ctx_t *cfg, const char *name,
				       dns_c_logchan_t **chan);
isc_result_t dns_c_ctx_addsyslogchannel(dns_c_ctx_t *cfg, const char *name,
					dns_c_logchan_t **chan);
isc_result_t dns_c_ctx_addnullchannel(dns_c_ctx_t *cfg, const char *name,
				      dns_c_logchan_t **chan);
isc_result_t dns_c_ctx_addstderrchannel(dns_c_ctx_t *cfg, const char *name,
                                        dns_c_logchan_t **chan);

isc_result_t dns_c_ctx_addcategory(dns_c_ctx_t *cfg, const char *catname,
				   dns_c_logcat_t **newcat);
isc_result_t dns_c_ctx_currchannel(dns_c_ctx_t *cfg,
				   dns_c_logchan_t **channel);
isc_result_t dns_c_ctx_currcategory(dns_c_ctx_t *cfg,
				    dns_c_logcat_t **category);
isc_boolean_t dns_c_ctx_channeldefinedp(dns_c_ctx_t *cfg, const char *name);

isc_result_t dns_c_ctx_getkdeflist(dns_c_ctx_t *cfg,
				   dns_c_kdeflist_t **retval);
isc_result_t dns_c_ctx_setkdeflist(dns_c_ctx_t *cfg, dns_c_kdeflist_t *newval,
				   isc_boolean_t deepcopy);

/* XXX need unsetkdeflist */
isc_boolean_t dns_c_ctx_keydefinedp(dns_c_ctx_t *ctx, const char *keyname);

/*
**
*/

isc_result_t dns_c_ctx_optionsnew(isc_mem_t *mem, dns_c_options_t **options);


isc_result_t dns_c_ctx_optionsdelete(dns_c_options_t **options);


isc_result_t dns_c_ctx_setdirectory(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getdirectory(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetdirectory(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setversion(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getversion(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetversion(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setdumpfilename(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getdumpfilename(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetdumpfilename(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setpidfilename(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getpidfilename(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetpidfilename(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setstatsfilename(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getstatsfilename(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetstatsfilename(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setmemstatsfilename(dns_c_ctx_t *ctx,
					   const char *newval);
isc_result_t dns_c_ctx_getmemstatsfilename(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetmemstatsfilename(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setcachefile(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getcachefile(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetcachefile(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setnamedxfer(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getnamedxfer(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetnamedxfer(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setrandomdevice(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getrandomdevice(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetrandomdevice(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setrandomseedfile(dns_c_ctx_t *ctx, const char *newval);
isc_result_t dns_c_ctx_getrandomseedfile(dns_c_ctx_t *ctx, char **retval);
isc_result_t dns_c_ctx_unsetrandomseedfile(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setport(dns_c_ctx_t *cfg, in_port_t newval);
isc_result_t dns_c_ctx_getport(dns_c_ctx_t *cfg, in_port_t *retval);
isc_result_t dns_c_ctx_unsetport(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settransfersin(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_gettransfersin(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsettransfersin(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settransfersperns(dns_c_ctx_t *cfg,
					 isc_uint32_t newval);
isc_result_t dns_c_ctx_gettransfersperns(dns_c_ctx_t *cfg,
					 isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsettransfersperns(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settransfersout(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_gettransfersout(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsettransfersout(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxlogsizeixfr(dns_c_ctx_t *cfg,
					 isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxlogsizeixfr(dns_c_ctx_t *cfg,
					 isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxlogsizeixfr(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setcleaninterval(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getcleaninterval(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetcleaninterval(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setinterfaceinterval(dns_c_ctx_t *cfg,
					    isc_uint32_t newval);
isc_result_t dns_c_ctx_getinterfaceinterval(dns_c_ctx_t *cfg,
					    isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetinterfaceinterval(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setstatsinterval(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getstatsinterval(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetstatsinterval(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setheartbeatinterval(dns_c_ctx_t *cfg,
					    isc_uint32_t newval);
isc_result_t dns_c_ctx_getheartbeatinterval(dns_c_ctx_t *cfg,
					    isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetheartbeatinterval(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxtransfertimein(dns_c_ctx_t *cfg,
					    isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxtransfertimein(dns_c_ctx_t *cfg,
					    isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxtransfertimein(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxtransfertimeout(dns_c_ctx_t *cfg,
					     isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxtransfertimeout(dns_c_ctx_t *cfg,
					     isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxtransfertimeout(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxtransferidlein(dns_c_ctx_t *cfg,
					    isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxtransferidlein(dns_c_ctx_t *cfg,
					    isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxtransferidlein(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxtransferidleout(dns_c_ctx_t *cfg,
					     isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxtransferidleout(dns_c_ctx_t *cfg,
					     isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxtransferidleout(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setlamettl(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getlamettl(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetlamettl(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settcpclients(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_gettcpclients(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsettcpclients(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setrecursiveclients(dns_c_ctx_t *cfg,
					   isc_uint32_t newval);
isc_result_t dns_c_ctx_getrecursiveclients(dns_c_ctx_t *cfg,
					   isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetrecursiveclients(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setminroots(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getminroots(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetminroots(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setserialqueries(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getserialqueries(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetserialqueries(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setsigvalidityinterval(dns_c_ctx_t *cfg,
					   isc_uint32_t newval);
isc_result_t dns_c_ctx_getsigvalidityinterval(dns_c_ctx_t *cfg,
					   isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetsigvalidityinterval(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setdatasize(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getdatasize(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetdatasize(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setstacksize(dns_c_ctx_t *cfg,
				    isc_uint32_t newval);
isc_result_t dns_c_ctx_getstacksize(dns_c_ctx_t *cfg,
				    isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetstacksize(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setcoresize(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getcoresize(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetcoresize(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setfiles(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getfiles(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetfiles(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxcachesize(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxcachesize(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxcachesize(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setminretrytime(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getminretrytime(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetminretrytime(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxretrytime(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxretrytime(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxretrytime(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setminrefreshtime(dns_c_ctx_t *cfg,
					 isc_uint32_t newval);
isc_result_t dns_c_ctx_getminrefreshtime(dns_c_ctx_t *cfg,
					 isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetminrefreshtime(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxrefreshtime(dns_c_ctx_t *cfg,
					 isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxrefreshtime(dns_c_ctx_t *cfg,
					 isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxrefreshtime(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_setmaxncachettl(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxncachettl(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxncachettl(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaxcachettl(dns_c_ctx_t *cfg, isc_uint32_t newval);
isc_result_t dns_c_ctx_getmaxcachettl(dns_c_ctx_t *cfg, isc_uint32_t *retval);
isc_result_t dns_c_ctx_unsetmaxcachettl(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setexpertmode(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getexpertmode(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetexpertmode(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setfakeiquery(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getfakeiquery(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetfakeiquery(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setrecursion(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getrecursion(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetrecursion(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setfetchglue(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getfetchglue(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetfetchglue(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setnotify(dns_c_ctx_t *cfg, dns_notifytype_t newval);
isc_result_t dns_c_ctx_getnotify(dns_c_ctx_t *cfg, dns_notifytype_t *retval);
isc_result_t dns_c_ctx_unsetnotify(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_sethoststatistics(dns_c_ctx_t *cfg,
					 isc_boolean_t newval);
isc_result_t dns_c_ctx_gethoststatistics(dns_c_ctx_t *cfg,
					 isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsethoststatistics(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setdealloconexit(dns_c_ctx_t *cfg,
					isc_boolean_t newval);
isc_result_t dns_c_ctx_getdealloconexit(dns_c_ctx_t *cfg,
					isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetdealloconexit(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setuseixfr(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getuseixfr(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetuseixfr(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmaintainixfrbase(dns_c_ctx_t *cfg,
					   isc_boolean_t newval);
isc_result_t dns_c_ctx_getmaintainixfrbase(dns_c_ctx_t *cfg,
					   isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetmaintainixfrbase(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_sethasoldclients(dns_c_ctx_t *cfg,
					isc_boolean_t newval);
isc_result_t dns_c_ctx_gethasoldclients(dns_c_ctx_t *cfg,
					isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsethasoldclients(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setauthnxdomain(dns_c_ctx_t *cfg,
				       isc_boolean_t newval);
isc_result_t dns_c_ctx_getauthnxdomain(dns_c_ctx_t *cfg,
				       isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetauthnxdomain(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setmultiplecnames(dns_c_ctx_t *cfg,
					 isc_boolean_t newval);
isc_result_t dns_c_ctx_getmultiplecnames(dns_c_ctx_t *cfg,
					 isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetmultiplecnames(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setuseidpool(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getuseidpool(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetuseidpool(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setdialup(dns_c_ctx_t *cfg, dns_dialuptype_t newval);
isc_result_t dns_c_ctx_getdialup(dns_c_ctx_t *cfg, dns_dialuptype_t *retval);
isc_result_t dns_c_ctx_unsetdialup(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_setstatistics(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getstatistics(dns_c_ctx_t *cfg, isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetstatistics(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_setrfc2308type1(dns_c_ctx_t *cfg,
				       isc_boolean_t newval);
isc_result_t dns_c_ctx_getrfc2308type1(dns_c_ctx_t *cfg,
				       isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetrfc2308type1(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setrequestixfr(dns_c_ctx_t *cfg,
				      isc_boolean_t newval);
isc_result_t dns_c_ctx_getrequestixfr(dns_c_ctx_t *cfg,
				      isc_boolean_t *retval);
isc_result_t dns_c_ctx_unsetrequestixfr(dns_c_ctx_t *cfg);
isc_result_t dns_c_ctx_setprovideixfr(dns_c_ctx_t *cfg, isc_boolean_t newval);
isc_result_t dns_c_ctx_getprovideixfr(dns_c_ctx_t *cfg, isc_boolean_t *retval);


isc_result_t dns_c_ctx_unsetprovideixfr(dns_c_ctx_t *cfg);
isc_result_t dns_c_ctx_settreatcrasspace(dns_c_ctx_t *cfg,
					 isc_boolean_t newval);
isc_result_t dns_c_ctx_gettreatcrasspace(dns_c_ctx_t *cfg,
					 isc_boolean_t *retval);


isc_result_t dns_c_ctx_getadditionalfromcache(dns_c_ctx_t *cfg,
					      isc_boolean_t *retval);
isc_result_t dns_c_ctx_setadditionalfromcache(dns_c_ctx_t *cfg,
					      isc_boolean_t newval);
isc_result_t dns_c_ctx_unsetadditionalfromcache(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_getadditionalfromauth(dns_c_ctx_t *cfg,
					     isc_boolean_t *retval);
isc_result_t dns_c_ctx_setadditionalfromauth(dns_c_ctx_t *cfg,
					     isc_boolean_t newval);
isc_result_t dns_c_ctx_unsetadditionalfromauth(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_unsettreatcrasspace(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settransfersource(dns_c_ctx_t *ctx,
					 isc_sockaddr_t transfer_source);
isc_result_t dns_c_ctx_gettransfersource(dns_c_ctx_t *ctx,
					 isc_sockaddr_t *transfer_source);
isc_result_t dns_c_ctx_unsettransfersource(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_settransfersourcev6(dns_c_ctx_t *ctx,
					   isc_sockaddr_t transfer_source_v6);
isc_result_t dns_c_ctx_gettransfersourcev6(dns_c_ctx_t *ctx,
					   isc_sockaddr_t *transfer_source_v6);
isc_result_t dns_c_ctx_unsettransfersourcev6(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setnotifysource(dns_c_ctx_t *ctx,
				       isc_sockaddr_t notify_source);
isc_result_t dns_c_ctx_getnotifysource(dns_c_ctx_t *ctx,
				       isc_sockaddr_t *notify_source);
isc_result_t dns_c_ctx_unsetnotifysource(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setnotifysourcev6(dns_c_ctx_t *ctx,
					 isc_sockaddr_t notify_source_v6);
isc_result_t dns_c_ctx_getnotifysourcev6(dns_c_ctx_t *ctx,
					 isc_sockaddr_t *notify_source_v6);
isc_result_t dns_c_ctx_unsetnotifysourcev6(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setquerysource(dns_c_ctx_t *ctx,
				      isc_sockaddr_t query_source);
isc_result_t dns_c_ctx_getquerysource(dns_c_ctx_t *ctx,
				      isc_sockaddr_t *query_source);
isc_result_t dns_c_ctx_unsetquerysource(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setquerysourcev6(dns_c_ctx_t *ctx, isc_sockaddr_t
					query_source_v6);
isc_result_t dns_c_ctx_getquerysourcev6(dns_c_ctx_t *ctx,
					isc_sockaddr_t *query_source_v6);
isc_result_t dns_c_ctx_unsetquerysourcev6(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setadditionaldata(dns_c_ctx_t *ctx,
					 dns_c_addata_t addata);
isc_result_t dns_c_ctx_getadditionaldata(dns_c_ctx_t *ctx,
					 dns_c_addata_t *addata);
isc_result_t dns_c_ctx_unsetadditionaldata(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setforward(dns_c_ctx_t *cfg, dns_c_forw_t forward);
isc_result_t dns_c_ctx_getforward(dns_c_ctx_t *cfg, dns_c_forw_t *forward);
isc_result_t dns_c_ctx_unsetforward(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settkeydhkey(dns_c_ctx_t *cfg, const char *newcpval,
				    isc_uint32_t newival);
isc_result_t dns_c_ctx_gettkeydhkey(dns_c_ctx_t *cfg, char **retcpval,
				    isc_uint32_t *retival);
/* XXX need unset version */


isc_result_t dns_c_ctx_settkeydomain(dns_c_ctx_t *cfg, const char *newval);
isc_result_t dns_c_ctx_gettkeydomain(dns_c_ctx_t *cfg, char **retval);
/* XXX need unset version */

isc_result_t dns_c_ctx_settkeygsscred(dns_c_ctx_t *cfg, const char *newval);
isc_result_t dns_c_ctx_gettkeygsscred(dns_c_ctx_t *cfg, char **retval);
/* XXX need unset version */


isc_result_t dns_c_ctx_setalsonotify(dns_c_ctx_t *ctx, dns_c_iplist_t *newval);
isc_result_t dns_c_ctx_getalsonotify(dns_c_ctx_t *ctx, dns_c_iplist_t **ret);
isc_result_t dns_c_ctx_unsetalsonotify(dns_c_ctx_t *ctx);


isc_result_t dns_c_ctx_setchecknames(dns_c_ctx_t *cfg, dns_c_trans_t transtype,
				     dns_severity_t newval);
isc_result_t dns_c_ctx_getchecknames(dns_c_ctx_t *cfg, dns_c_trans_t transtype,
				     dns_severity_t *retval);
isc_result_t dns_c_ctx_unsetchecknames(dns_c_ctx_t *cfg,
				       dns_c_trans_t transtype);


isc_result_t dns_c_ctx_settransferformat(dns_c_ctx_t *cfg,
					 dns_transfer_format_t tformat);
isc_result_t dns_c_ctx_gettransferformat(dns_c_ctx_t *cfg,
					 dns_transfer_format_t *tformat);
isc_result_t dns_c_ctx_unsettransferformat(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_setallownotify(dns_c_ctx_t *cfg,
				      dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getallownotify(dns_c_ctx_t *cfg,
				      dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetallownotify(dns_c_ctx_t *cfg);

isc_result_t dns_c_ctx_setallowquery(dns_c_ctx_t *cfg,
				     dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getallowquery(dns_c_ctx_t *cfg,
				     dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetallowquery(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setallowtransfer(dns_c_ctx_t *cfg,
					dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getallowtransfer(dns_c_ctx_t *cfg,
					dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetallowtransfer(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setallowrecursion(dns_c_ctx_t *cfg,
					 dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getallowrecursion(dns_c_ctx_t *cfg,
					 dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetallowrecursion(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setblackhole(dns_c_ctx_t *cfg,
				    dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getblackhole(dns_c_ctx_t *cfg,
				    dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetblackhole(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_settopology(dns_c_ctx_t *cfg,
				   dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_gettopology(dns_c_ctx_t *cfg,
				   dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsettopology(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setsortlist(dns_c_ctx_t *cfg,
				   dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getsortlist(dns_c_ctx_t *cfg,
				   dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetsortlist(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setallowupdateforwarding(dns_c_ctx_t *cfg,
						dns_c_ipmatchlist_t *iml);
isc_result_t dns_c_ctx_getallowupdateforwarding(dns_c_ctx_t *cfg,
						dns_c_ipmatchlist_t **list);
isc_result_t dns_c_ctx_unsetallowupdateforwarding(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_setforwarders(dns_c_ctx_t *cfg, isc_boolean_t copy,
				     dns_c_iplist_t *iml);
isc_result_t dns_c_ctx_getforwarders(dns_c_ctx_t *cfg, dns_c_iplist_t **list);
isc_result_t dns_c_ctx_unsetforwarders(dns_c_ctx_t *cfg);


isc_result_t dns_c_ctx_addlisten_on(dns_c_ctx_t *cfg, in_port_t port,
				    dns_c_ipmatchlist_t *ml,
				    isc_boolean_t copy);
isc_result_t dns_c_ctx_getlistenlist(dns_c_ctx_t *cfg,
				     dns_c_lstnlist_t **ll);


isc_result_t dns_c_ctx_addv6listen_on(dns_c_ctx_t *cfg, in_port_t port,
				      dns_c_ipmatchlist_t *ml,
				      isc_boolean_t copy);
isc_result_t dns_c_ctx_getv6listenlist(dns_c_ctx_t *cfg,
				       dns_c_lstnlist_t **ll);


isc_result_t dns_c_ctx_setrrsetorderlist(dns_c_ctx_t *cfg, isc_boolean_t copy,
					 dns_c_rrsolist_t *olist);
isc_result_t dns_c_ctx_getrrsetorderlist(dns_c_ctx_t *cfg,
					 dns_c_rrsolist_t **olist);


isc_result_t dns_c_ctx_gettrustedkeys(dns_c_ctx_t *cfg,
				      dns_c_tkeylist_t **retval);
isc_result_t dns_c_ctx_settrustedkeys(dns_c_ctx_t *cfg, dns_c_tkeylist_t *list,
				      isc_boolean_t copy);

isc_result_t dns_c_ctx_getlwres(dns_c_ctx_t *cfg, dns_c_lwreslist_t **retval);
isc_result_t dns_c_ctx_setlwres(dns_c_ctx_t *cfg, dns_c_lwreslist_t *list);


ISC_LANG_ENDDECLS

#endif /* DNS_CONFCTX_H */

