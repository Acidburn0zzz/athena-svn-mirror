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

/* $Id: confzone.h,v 1.1.1.1 2001-10-22 13:08:19 ghudson Exp $ */

#ifndef DNS_CONFZONE_H
#define DNS_CONFZONE_H 1

/*****
 ***** Module Info
 *****/

/*
 * Zones as seen by the config file parser. The data structures here define
 * the zone data as it is in the config file. The data structures here do
 * *not* define the things like red-black trees for named's internal data
 * structures.
 *
 */

/*
 *
 * MP:
 *	Client must do necessary locking.
 *
 * Reliability:
 *
 *	No problems.
 *
 * resources:
 *
 *	Use memory managers supplied by client.
 *
 * Security:
 *
 *	N/A
 *
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/confip.h>
#include <dns/confkeys.h>

#define DNS_C_ZONELIST_MAGIC		0x5a4c5354 /* ZLST */
#define DNS_C_ZONE_MAGIC		0x7a4f6e45 /* zOnE */

#define DNS_C_ZONELIST_VALID(ptr) ISC_MAGIC_VALID(ptr, DNS_C_ZONELIST_MAGIC)
#define DNS_C_ZONE_VALID(ptr)     ISC_MAGIC_VALID(ptr, DNS_C_ZONE_MAGIC)

/***
 *** Types
 ***/

typedef struct dns_c_master_zone	dns_c_masterzone_t;
typedef struct dns_c_slave_zone		dns_c_slavezone_t;
typedef struct dns_c_stub_zone		dns_c_stubzone_t;
typedef struct dns_c_forward_zone	dns_c_forwardzone_t;
typedef struct dns_c_hint_zone		dns_c_hintzone_t;
typedef struct dns_c_zone		dns_c_zone_t;
typedef struct dns_c_zonelem		dns_c_zonelem_t;



struct dns_c_zonelem {
	dns_c_zone_t	*thezone;
	ISC_LINK(dns_c_zonelem_t) next;
};


struct dns_c_zone_list {
	isc_int32_t 		magic;
	isc_mem_t	       *mem;

	ISC_LIST(dns_c_zonelem_t)	zones;
};


struct dns_c_master_zone {
	char		       *file;
	dns_severity_t		check_names;
	dns_c_ipmatchlist_t    *allow_update;
	dns_c_ipmatchlist_t    *allow_update_forwarding;
	dns_ssutable_t	       *ssuauth;
	dns_c_ipmatchlist_t    *allow_query;
	dns_c_ipmatchlist_t    *allow_transfer;
	dns_dialuptype_t	dialup;
	isc_boolean_t		statistics;
	dns_notifytype_t	notify;
	dns_c_iplist_t	       *also_notify;
	char		       *ixfr_base;
	char		       *ixfr_tmp;
	isc_uint32_t		max_ixfr_log;
	isc_boolean_t		maint_ixfr_base;
	dns_c_pklist_t	       *pubkeylist;
	isc_uint32_t		max_trans_time_out;
	isc_uint32_t		max_trans_idle_out;
	isc_uint32_t		sig_valid_interval;

	isc_sockaddr_t		notify_source;
	isc_sockaddr_t		notify_source_v6;
	isc_sockaddr_t		transfer_source;
	isc_sockaddr_t		transfer_source_v6;
	isc_uint32_t		min_retry_time;
	isc_uint32_t		max_retry_time;
	isc_uint32_t		min_refresh_time;
	isc_uint32_t		max_refresh_time;
	

	dns_c_forw_t		forward;
	dns_c_iplist_t	       *forwarders;

	dns_c_setbits_t		setflags;
};

struct dns_c_slave_zone {
	char		       *file;
	dns_severity_t	check_names;
	dns_c_ipmatchlist_t    *allow_update;
	dns_c_ipmatchlist_t    *allow_update_forwarding;
	dns_c_ipmatchlist_t    *allow_notify;
	dns_c_ipmatchlist_t    *allow_query;
	dns_c_ipmatchlist_t    *allow_transfer;
	dns_c_iplist_t	       *also_notify;
	dns_notifytype_t	notify;
	dns_dialuptype_t	dialup;
	isc_boolean_t		statistics;
	char		       *ixfr_base;
	char		       *ixfr_tmp;
	isc_boolean_t		maint_ixfr_base;
	isc_uint32_t		max_ixfr_log;
	dns_c_pklist_t	       *pubkeylist;
	in_port_t		master_port;
	dns_c_iplist_t	       *master_ips;
	isc_sockaddr_t		notify_source;
	isc_sockaddr_t		notify_source_v6;
	isc_sockaddr_t		transfer_source;
	isc_sockaddr_t		transfer_source_v6;
	isc_uint32_t		max_trans_time_in;
	isc_uint32_t		max_trans_time_out;
	isc_uint32_t		max_trans_idle_in;
	isc_uint32_t		max_trans_idle_out;

	isc_uint32_t		min_retry_time;
	isc_uint32_t		max_retry_time;
	isc_uint32_t		min_refresh_time;
	isc_uint32_t		max_refresh_time;

	dns_c_forw_t		forward;
	dns_c_iplist_t	       *forwarders;

	dns_c_setbits_t		setflags;
};

struct dns_c_stub_zone {
	char		       *file;
	dns_severity_t	check_names;
	dns_c_ipmatchlist_t    *allow_update; /* should be here??? */
	dns_c_ipmatchlist_t    *allow_update_forwarding;
	dns_c_ipmatchlist_t    *allow_query;
	dns_c_ipmatchlist_t    *allow_transfer; /* should be here??? */
	dns_dialuptype_t	dialup;
	isc_boolean_t		statistics;
	dns_c_pklist_t	       *pubkeylist;
	in_port_t		master_port;
	dns_c_iplist_t	       *master_ips;
	isc_sockaddr_t		transfer_source;
	isc_sockaddr_t		transfer_source_v6;
	isc_uint32_t		max_trans_time_in;
	isc_uint32_t		max_trans_idle_in;

	dns_c_forw_t		forward;
	dns_c_iplist_t	       *forwarders;
	isc_uint32_t		min_retry_time;
	isc_uint32_t		max_retry_time;
	isc_uint32_t		min_refresh_time;
	isc_uint32_t		max_refresh_time;

	dns_c_setbits_t		setflags;
};

struct dns_c_forward_zone {
	dns_severity_t	check_names;
	dns_c_forw_t		forward;
	dns_c_iplist_t	       *forwarders;

	dns_c_setbits_t		setflags;
};

struct dns_c_hint_zone {
	char		       *file;
	dns_severity_t		check_names;
	dns_c_pklist_t	       *pubkeylist;

	dns_c_setbits_t		setflags;
};

struct dns_c_zone {
	isc_int32_t			magic;

	isc_mem_t		       *mem;
	isc_uint8_t			refcount;

	char			       *name; /* e.g. "foo.com" */
	char			       *internalname; /* e.g. "foo.com.ext" */
	char			       *database;
	dns_rdataclass_t		zclass;
	dns_c_view_t		       *view;
	isc_boolean_t		       *enabled;

	dns_c_zonetype_t		ztype;
	union
	{
		dns_c_masterzone_t	mzone;
		dns_c_slavezone_t	szone;
		dns_c_stubzone_t	tzone;
		dns_c_forwardzone_t	fzone;
		dns_c_hintzone_t	hzone;
	} u;
};

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS



/*
 */

isc_result_t dns_c_zonelist_new(isc_mem_t *mem, dns_c_zonelist_t **zlist);
isc_result_t dns_c_zonelist_delete(dns_c_zonelist_t **zlist);
isc_result_t dns_c_zonelist_checkzones(dns_c_zonelist_t *zlist);
isc_result_t dns_c_zonelist_find(dns_c_zonelist_t *zlist, const char *name,
				 dns_c_zone_t **retval);
isc_result_t dns_c_zonelist_rmbyname(dns_c_zonelist_t *zlist,
				     const char *name);
isc_result_t dns_c_zonelist_addzone(dns_c_zonelist_t *zlist,
				    dns_c_zone_t *zone);
isc_result_t dns_c_zonelist_rmzone(dns_c_zonelist_t *zlist,
				   dns_c_zone_t *zone);
void dns_c_zonelist_print(FILE *fp, int indent, dns_c_zonelist_t *list,
			  dns_c_view_t *view);
void dns_c_zonelist_printpostopts(FILE *fp, int indent,
				  dns_c_zonelist_t *list);
void dns_c_zonelist_printpreopts(FILE *fp, int indent,
				 dns_c_zonelist_t *list);
dns_c_zone_t * dns_c_zonelist_firstzone(dns_c_zonelist_t *list);
dns_c_zone_t * dns_c_zonelist_nextzone(dns_c_zonelist_t *list,
				       dns_c_zone_t *thezone);


/*
 *
 */
isc_result_t dns_c_zone_new(isc_mem_t *mem, dns_c_zonetype_t ztype,
			    dns_rdataclass_t zclass, const char *name,
			    const char *internalname, dns_c_zone_t **zone);
isc_result_t dns_c_zone_detach(dns_c_zone_t **zone);
void dns_c_zone_attach(dns_c_zone_t *source, dns_c_zone_t **target);
void dns_c_zone_print(FILE *fp, int indent, dns_c_zone_t *zone);
isc_result_t dns_c_zone_validate(dns_c_zone_t *zone);


isc_result_t dns_c_zone_getname(dns_c_zone_t *zone, const char **retval);

isc_result_t dns_c_zone_getinternalname(dns_c_zone_t *zone,
					const char **retval);
isc_result_t dns_c_zone_getpubkeylist(dns_c_zone_t *zone,
				      dns_c_pklist_t **retval);


isc_result_t dns_c_zone_setfile(dns_c_zone_t *zone, const char *newfile);
isc_result_t dns_c_zone_getfile(dns_c_zone_t *zone, const char **retval);


isc_result_t dns_c_zone_setchecknames(dns_c_zone_t *zone,
				      dns_severity_t severity);
isc_result_t dns_c_zone_getchecknames(dns_c_zone_t *zone,
				      dns_severity_t *retval);


isc_result_t dns_c_zone_setallowupdateforwarding(dns_c_zone_t *zone,
						 dns_c_ipmatchlist_t *ipml,
						 isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getallowupdateforwarding(dns_c_zone_t *zone,
						 dns_c_ipmatchlist_t **retval);


isc_result_t dns_c_zone_setssuauth(dns_c_zone_t *zone,
				   dns_ssutable_t *ssutable);
isc_result_t dns_c_zone_getssuauth(dns_c_zone_t *zone,
				   dns_ssutable_t **ssutable);


isc_result_t dns_c_zone_setallownotify(dns_c_zone_t *zone,
				       dns_c_ipmatchlist_t *ipml,
				       isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getallownotify(dns_c_zone_t *zone,
				       dns_c_ipmatchlist_t **retval);

isc_result_t dns_c_zone_setallowquery(dns_c_zone_t *zone,
				      dns_c_ipmatchlist_t *ipml,
				      isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getallowquery(dns_c_zone_t *zone,
				      dns_c_ipmatchlist_t **retval);


isc_result_t dns_c_zone_setallowtransfer(dns_c_zone_t *zone,
					 dns_c_ipmatchlist_t *ipml,
					 isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getallowtransfer(dns_c_zone_t *zone,
					 dns_c_ipmatchlist_t **retval);


isc_result_t dns_c_zone_setdialup(dns_c_zone_t *zone,
				  dns_dialuptype_t newval);
isc_result_t dns_c_zone_getdialup(dns_c_zone_t *zone, dns_dialuptype_t *retval);

isc_result_t dns_c_zone_setstatistics(dns_c_zone_t *zone,
				      isc_boolean_t newval);
isc_result_t dns_c_zone_getstatistics(dns_c_zone_t *zone,
				      isc_boolean_t *retval);


isc_result_t dns_c_zone_setnotify(dns_c_zone_t *zone,
				  dns_notifytype_t newval);
isc_result_t dns_c_zone_getnotify(dns_c_zone_t *zone,
				  dns_notifytype_t *retval);


isc_result_t dns_c_zone_setalsonotify(dns_c_zone_t *zone,
				      dns_c_iplist_t *newval,
				      isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getalsonotify(dns_c_zone_t *zone,
				      dns_c_iplist_t **retval);


isc_result_t dns_c_zone_setmaintixfrbase(dns_c_zone_t *zone,
					 isc_boolean_t newval);
isc_result_t dns_c_zone_getmaintixfrbase(dns_c_zone_t *zone,
					 isc_boolean_t *retval);


isc_result_t dns_c_zone_setixfrbase(dns_c_zone_t *zone, const char *newval);
isc_result_t dns_c_zone_getixfrbase(dns_c_zone_t *zone, const char **retval);


isc_result_t dns_c_zone_setixfrtmp(dns_c_zone_t *zone, const char *newval);
isc_result_t dns_c_zone_getixfrtmp(dns_c_zone_t *zone, const char **retval);


isc_result_t dns_c_zone_addpubkey(dns_c_zone_t *zone, dns_c_pubkey_t *pubkey,
				  isc_boolean_t deepcopy);


isc_result_t dns_c_zone_setmasterport(dns_c_zone_t *zone, in_port_t port);
isc_result_t dns_c_zone_getmasterport(dns_c_zone_t *zone, in_port_t *retval);


isc_result_t dns_c_zone_setmasterips(dns_c_zone_t *zone,
				     dns_c_iplist_t *newval,
				     isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getmasterips(dns_c_zone_t *zone,
				     dns_c_iplist_t **retval);


isc_result_t dns_c_zone_settransfersource(dns_c_zone_t *zone,
					  isc_sockaddr_t newval);
isc_result_t dns_c_zone_gettransfersource(dns_c_zone_t *zone,
					  isc_sockaddr_t *retval);


isc_result_t dns_c_zone_setnotifysource(dns_c_zone_t *zone,
					isc_sockaddr_t newval);
isc_result_t dns_c_zone_getnotifysource(dns_c_zone_t *zone,
					isc_sockaddr_t *retval);


isc_result_t dns_c_zone_settransfersourcev6(dns_c_zone_t *zone,
					    isc_sockaddr_t newval);
isc_result_t dns_c_zone_gettransfersourcev6(dns_c_zone_t *zone,
					    isc_sockaddr_t *retval);


isc_result_t dns_c_zone_setnotifysourcev6(dns_c_zone_t *zone,
					  isc_sockaddr_t newval);
isc_result_t dns_c_zone_getnotifysourcev6(dns_c_zone_t *zone,
					  isc_sockaddr_t *retval);


isc_result_t dns_c_zone_setmaxtranstimein(dns_c_zone_t *zone,
					  isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxtranstimein(dns_c_zone_t *zone,
					  isc_uint32_t *retval);


isc_result_t dns_c_zone_setmaxtranstimeout(dns_c_zone_t *zone,
					   isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxtranstimeout(dns_c_zone_t *zone,
					   isc_uint32_t *retval);





isc_result_t dns_c_zone_setmaxtransidlein(dns_c_zone_t *zone,
					  isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxtransidlein(dns_c_zone_t *zone,
					  isc_uint32_t *retval);


isc_result_t dns_c_zone_setmaxtransidleout(dns_c_zone_t *zone,
					   isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxtransidleout(dns_c_zone_t *zone,
					   isc_uint32_t *retval);


isc_result_t dns_c_zone_setsigvalidityinterval(dns_c_zone_t *zone,
					    isc_uint32_t newval);
isc_result_t dns_c_zone_getsigvalidityinterval(dns_c_zone_t *zone,
					    isc_uint32_t *retval);


isc_result_t dns_c_zone_setminretrytime(dns_c_zone_t *zone,
					isc_uint32_t newval);
isc_result_t dns_c_zone_getminretrytime(dns_c_zone_t *zone,
					isc_uint32_t *retval);

isc_result_t dns_c_zone_setmaxretrytime(dns_c_zone_t *zone,
					isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxretrytime(dns_c_zone_t *zone,
					isc_uint32_t *retval);


isc_result_t dns_c_zone_setminrefreshtime(dns_c_zone_t *zone,
					  isc_uint32_t newval);
isc_result_t dns_c_zone_getminrefreshtime(dns_c_zone_t *zone,
					  isc_uint32_t *retval);

isc_result_t dns_c_zone_setmaxrefreshtime(dns_c_zone_t *zone,
					  isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxrefreshtime(dns_c_zone_t *zone,
					  isc_uint32_t *retval);

isc_result_t dns_c_zone_setmaxixfrlog(dns_c_zone_t *zone,
				      isc_uint32_t newval);
isc_result_t dns_c_zone_getmaxixfrlog(dns_c_zone_t *zone,
				      isc_uint32_t *retval);


isc_result_t dns_c_zone_setforward(dns_c_zone_t *zone,
				   dns_c_forw_t newval);
isc_result_t dns_c_zone_getforward(dns_c_zone_t *zone, dns_c_forw_t *retval);


isc_result_t dns_c_zone_setforwarders(dns_c_zone_t *zone, dns_c_iplist_t *ipml,
				      isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getforwarders(dns_c_zone_t *zone,
				      dns_c_iplist_t **retval);


isc_result_t dns_c_zone_setallowupd(dns_c_zone_t *zone,
				    dns_c_ipmatchlist_t *ipml,
				    isc_boolean_t deepcopy);
isc_result_t dns_c_zone_getallowupd(dns_c_zone_t *zone,
				    dns_c_ipmatchlist_t **retval);
isc_result_t dns_c_zone_unsetallowupd(dns_c_zone_t *zone);


isc_result_t dns_c_zone_setdatabase(dns_c_zone_t *zone, const char *database);
isc_result_t dns_c_zone_getdatabase(dns_c_zone_t *zone, char **retval);
isc_result_t dns_c_zone_unsetdatabase(dns_c_zone_t *zone);


isc_result_t dns_c_zone_setenabled(dns_c_zone_t *zone, isc_boolean_t enabled);
isc_result_t dns_c_zone_getenabled(dns_c_zone_t *zone, isc_boolean_t *retval);
isc_result_t dns_c_zone_unsetenabled(dns_c_zone_t *zone);


ISC_LANG_ENDDECLS

#endif /* DNS_CONFZONE_H */
