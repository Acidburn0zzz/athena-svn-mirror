/* Machine generated file -- Do NOT edit */

#ifndef	_RXGEN_UBIK_INT_
#define	_RXGEN_UBIK_INT_

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
extern bool_t xdr_int64();
extern bool_t xdr_uint64();
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


struct BDesc {
	afs_int32 host;
	short portal;
	afs_int32 session;
};
typedef struct BDesc BDesc;
bool_t xdr_BDesc();


struct net_version {
	afs_int32 epoch;
	afs_int32 counter;
};
typedef struct net_version net_version;
bool_t xdr_net_version();


struct net_tid {
	afs_int32 epoch;
	afs_int32 counter;
};
typedef struct net_tid net_tid;
bool_t xdr_net_tid();

#define UBIK_MAX_INTERFACE_ADDR 256

struct ubik_debug {
	afs_int32 now;
	afs_int32 lastYesTime;
	afs_int32 lastYesHost;
	afs_int32 lastYesState;
	afs_int32 lastYesClaim;
	afs_int32 lowestHost;
	afs_int32 lowestTime;
	afs_int32 syncHost;
	afs_int32 syncTime;
	struct net_version syncVersion;
	struct net_tid syncTid;
	afs_int32 amSyncSite;
	afs_int32 syncSiteUntil;
	afs_int32 nServers;
	afs_int32 lockedPages;
	afs_int32 writeLockedPages;
	struct net_version localVersion;
	afs_int32 activeWrite;
	afs_int32 tidCounter;
	afs_int32 anyReadLocks;
	afs_int32 anyWriteLocks;
	afs_int32 recoveryState;
	afs_int32 currentTrans;
	afs_int32 writeTrans;
	afs_int32 epochTime;
	afs_int32 interfaceAddr[UBIK_MAX_INTERFACE_ADDR];
};
typedef struct ubik_debug ubik_debug;
bool_t xdr_ubik_debug();


struct ubik_sdebug {
	afs_int32 addr;
	afs_int32 lastVoteTime;
	afs_int32 lastBeaconSent;
	afs_int32 lastVote;
	struct net_version remoteVersion;
	afs_int32 currentDB;
	afs_int32 beaconSinceDown;
	afs_int32 up;
	afs_int32 altAddr[255];
};
typedef struct ubik_sdebug ubik_sdebug;
bool_t xdr_ubik_sdebug();


struct ubik_debug_old {
	afs_int32 now;
	afs_int32 lastYesTime;
	afs_int32 lastYesHost;
	afs_int32 lastYesState;
	afs_int32 lastYesClaim;
	afs_int32 lowestHost;
	afs_int32 lowestTime;
	afs_int32 syncHost;
	afs_int32 syncTime;
	struct net_version syncVersion;
	struct net_tid syncTid;
	afs_int32 amSyncSite;
	afs_int32 syncSiteUntil;
	afs_int32 nServers;
	afs_int32 lockedPages;
	afs_int32 writeLockedPages;
	struct net_version localVersion;
	afs_int32 activeWrite;
	afs_int32 tidCounter;
	afs_int32 anyReadLocks;
	afs_int32 anyWriteLocks;
	afs_int32 recoveryState;
	afs_int32 currentTrans;
	afs_int32 writeTrans;
	afs_int32 epochTime;
};
typedef struct ubik_debug_old ubik_debug_old;
bool_t xdr_ubik_debug_old();


struct ubik_sdebug_old {
	afs_int32 addr;
	afs_int32 lastVoteTime;
	afs_int32 lastBeaconSent;
	afs_int32 lastVote;
	struct net_version remoteVersion;
	afs_int32 currentDB;
	afs_int32 beaconSinceDown;
	afs_int32 up;
};
typedef struct ubik_sdebug_old ubik_sdebug_old;
bool_t xdr_ubik_sdebug_old();


struct UbikInterfaceAddr {
	afs_int32 hostAddr[UBIK_MAX_INTERFACE_ADDR];
};
typedef struct UbikInterfaceAddr UbikInterfaceAddr;
bool_t xdr_UbikInterfaceAddr();

#define BULK_ERROR 1

typedef struct bulkdata {
	u_int bulkdata_len;
	char *bulkdata_val;
} bulkdata;
bool_t xdr_bulkdata();

#define IOVEC_MAXBUF 65536
#define IOVEC_MAXWRT 64

typedef struct iovec_buf {
	u_int iovec_buf_len;
	char *iovec_buf_val;
} iovec_buf;
bool_t xdr_iovec_buf();


struct ubik_iovec {
	afs_int32 file;
	afs_int32 position;
	afs_int32 length;
};
typedef struct ubik_iovec ubik_iovec;
bool_t xdr_ubik_iovec();


typedef struct iovec_wrt {
	u_int iovec_wrt_len;
	ubik_iovec *iovec_wrt_val;
} iovec_wrt;
bool_t xdr_iovec_wrt();

#define VOTE_STATINDEX 11

#include <rx/rx_multi.h>
#define multi_VOTE_Beacon(state, voteStart, Version, tid) \
	multi_Body(StartVOTE_Beacon(multi_call, state, voteStart, Version, tid), EndVOTE_Beacon(multi_call))

#define DISK_STATINDEX 12

#define multi_DISK_Probe() \
	multi_Body(StartDISK_Probe(multi_call), EndDISK_Probe(multi_call))


#define multi_DISK_UpdateInterfaceAddr(inAddr, outAddr) \
	multi_Body(StartDISK_UpdateInterfaceAddr(multi_call, inAddr), EndDISK_UpdateInterfaceAddr(multi_call, outAddr))


/* Opcode-related useful stats for package: VOTE_ */
#define VOTE_LOWEST_OPCODE   10000
#define VOTE_HIGHEST_OPCODE	10007
#define VOTE_NUMBER_OPCODES	8

#define VOTE_NO_OF_STAT_FUNCS	8

AFS_RXGEN_EXPORT
extern const char *VOTE_function_names[];


/* Opcode-related useful stats for package: DISK_ */
#define DISK_LOWEST_OPCODE   20000
#define DISK_HIGHEST_OPCODE	20013
#define DISK_NUMBER_OPCODES	14

#define DISK_NO_OF_STAT_FUNCS	14

AFS_RXGEN_EXPORT
extern const char *DISK_function_names[];

#endif	/* _RXGEN_UBIK_INT_ */
