/* Machine generated file -- Do NOT edit */

#ifndef	_RXGEN_BOSINT_
#define	_RXGEN_BOSINT_

#ifdef	KERNEL
/* The following 'ifndefs' are not a good solution to the vendor's omission of surrounding all system includes with 'ifndef's since it requires that this file is included after the system includes...*/
#include "../afs/param.h"
#ifdef	UKERNEL
#include "../afs/sysincludes.h"
#include "../rx/xdr.h"
#include "../rx/rx.h"
#include "../rx/rx_globals.h"
#else	/* UKERNEL */
#include "../h/types.h"
#ifndef	SOCK_DGRAM  /* XXXXX */
#include "../h/socket.h"
#endif
#ifndef	DTYPE_SOCKET  /* XXXXX */
#ifdef AFS_DEC_ENV
#include "../h/smp_lock.h"
#endif
#ifndef AFS_LINUX22_ENV
#include "../h/file.h"
#endif
#endif
#ifndef	S_IFMT  /* XXXXX */
#include "../h/stat.h"
#endif
#ifndef	IPPROTO_UDP /* XXXXX */
#include "../netinet/in.h"
#endif
#ifndef	DST_USA  /* XXXXX */
#include "../h/time.h"
#endif
#ifndef AFS_LINUX22_ENV
#include "../rpc/types.h"
#endif /* AFS_LINUX22_ENV */
#ifndef	XDR_GETLONG /* XXXXX */
#ifdef AFS_LINUX22_ENV
#ifndef quad_t
#define quad_t __quad_t
#define u_quad_t __u_quad_t
#endif
#endif
#ifdef AFS_LINUX22_ENV
#include "../rx/xdr.h"
#else /* AFS_LINUX22_ENV */
#include "../rpc/xdr.h"
#endif /* AFS_LINUX22_ENV */
#endif /* XDR_GETLONG */
#endif   /* UKERNEL */
#include "../afsint/rxgen_consts.h"
#include "../afs/afs_osi.h"
#include "../rx/rx.h"
#include "../rx/rx_globals.h"
#else	/* KERNEL */
#include <afs/param.h>
#include <afs/stds.h>
#include <sys/types.h>
#include <rx/xdr.h>
#include <rx/rx.h>
#include <rx/rx_globals.h>
#include <afs/rxgen_consts.h>
#endif	/* KERNEL */

#ifdef AFS_NT40_ENV
#ifndef AFS_RXGEN_EXPORT
#define AFS_RXGEN_EXPORT __declspec(dllimport)
#endif /* AFS_RXGEN_EXPORT */
#else /* AFS_NT40_ENV */
#define AFS_RXGEN_EXPORT
#endif /* AFS_NT40_ENV */


typedef int bstring;
bool_t xdr_bstring();

#define BOZO_BSSIZE 256
#undef min

struct bozo_netKTime {
	int mask;
	short hour;
	short min;
	short sec;
	short day;
};
typedef struct bozo_netKTime bozo_netKTime;
bool_t xdr_bozo_netKTime();


struct bozo_key {
	char data[8];
};
typedef struct bozo_key bozo_key;
bool_t xdr_bozo_key();


struct bozo_keyInfo {
	int32 mod_sec;
	int32 mod_usec;
	u_int32 keyCheckSum;
	int32 spare2;
};
typedef struct bozo_keyInfo bozo_keyInfo;
bool_t xdr_bozo_keyInfo();


struct bozo_status {
	int32 goal;
	int32 fileGoal;
	int32 procStartTime;
	int32 procStarts;
	int32 lastAnyExit;
	int32 lastErrorExit;
	int32 errorCode;
	int32 errorSignal;
	int32 flags;
	int32 spare[8];
};
typedef struct bozo_status bozo_status;
bool_t xdr_bozo_status();

#define BOZO_HASCORE		1	/* core file exists */
#define BOZO_ERRORSTOP		2	/* stopped due to too many errors */
#define BOZO_BADDIRACCESS	4	/* bad mode bits on /usr/afs dirs */
#define BOZO_PRUNEOLD		1	/* prune .OLD files */
#define BOZO_PRUNEBAK		2	/* prune .BAK files */
#define BOZO_PRUNECORE		4	/* prune core files */

/* Opcode-related useful stats for package: BOZO_ */
#define BOZO_LOWEST_OPCODE   80
#define BOZO_HIGHEST_OPCODE	114
#define BOZO_NUMBER_OPCODES	35

#define BOZO_NO_OF_CLIENT_STAT_FUNCS	37

#define BOZO_NO_OF_SERVER_STAT_FUNCS	35

AFS_RXGEN_EXPORT
extern const char *BOZO_client_function_names[];

AFS_RXGEN_EXPORT
extern const char *BOZO_server_function_names[];

#endif	/* _RXGEN_BOSINT_ */
