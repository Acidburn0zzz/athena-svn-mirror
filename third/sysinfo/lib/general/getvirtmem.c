/*
 * Copyright (c) 1992-2000 MagniComp 
 * This software may only be used in accordance with the license which is 
 * available as http://www.magnicomp.com/sysinfo/4.0/sysinfo-eu-license.shtml
 */

#ifndef lint
static char *RCSid = "$Revision: 1.1.1.1 $";
#endif

/*
 * Virtual Memory related functions.
 */

#include "defs.h"

/*
 * Change memory Amount into a string.
 * Amount should always be in kilobytes.
 */
extern char *GetVirtMemStr(Amount)
    Large_t			Amount;
{
    static char			Buff[32];

    if (Amount == 0)
	return((char *) NULL);

    (void) snprintf(Buff, sizeof(Buff), "%s", GetSizeStr(Amount, KBYTES));

    return(Buff);
}

/*
 * Find amount of virtual memory using "anoninfo" symbol.
 */

#if	defined(HAVE_ANONINFO) && !defined(HAVE_SWAPCTL)
#	include <vm/anon.h>
#if	!defined(ANONINFO_SYM)
#	define ANONINFO_SYM		"_anoninfo"
#endif
#endif	/* HAVE_ANONINFO && !HAVE_SWAPCTL */

extern char *GetVirtMemAnoninfo()
{
#if	defined(HAVE_ANONINFO) && !defined(HAVE_SWAPCTL)
    kvm_t		       *kd;
    static struct anoninfo	AnonInfo;
    Large_t			Amount = 0;
    nlist_t		       *nlPtr;
    int				PageSize;

    if (kd = KVMopen()) {
	if ((nlPtr = KVMnlist(kd, ANONINFO_SYM, (nlist_t *)NULL, 0)) == NULL) {
	    KVMclose(kd);
	    return(0);
	}

	if (CheckNlist(nlPtr)) {
	    KVMclose(kd);
	    return(0);
	}

	if (KVMget(kd, nlPtr->n_value, (char *) &AnonInfo, 
		   sizeof(AnonInfo), KDT_DATA) >= 0)
	    Amount = (Large_t)AnonInfo.ani_max;

	if (Amount) {
	    /*
	     * Try to avoid overflow
	     */
	    PageSize = getpagesize();
	    if (PageSize >= KBYTES)
		Amount = Amount * ((Large_t)((PageSize / KBYTES)));
	    else
		Amount = (Amount * (Large_t)PageSize) / 
		    (Large_t)KBYTES;
	    SImsg(SIM_DBG, "GetVirtMemAnon: Amount=%.0f PageSize=%d",
		  (float) Amount, PageSize);
	}

	KVMclose(kd);
    }

    return(GetVirtMemStr(Amount));
#else	/* !HAVE_ANONINFO */
    return((char *) NULL);
#endif	/* HAVE_ANONINFO && !HAVE_SWAPCTL*/
}

/*
 * Find amount of virtual memory using swapctl()
 */

#if	defined(HAVE_SWAPCTL)
#	include <sys/stat.h>
#	include <sys/swap.h>
#endif	/* HAVE_SWAPCTL */

extern char *GetVirtMemSwapctl()
{
    char		       *ValStr = NULL;
    Large_t			Amount = 0;
#if	defined(HAVE_ANONINFO) && defined(HAVE_SWAPCTL)
    static struct anoninfo	AnonInfo;
    int				PageSize;

    if (swapctl(SC_AINFO, (void *) &AnonInfo) == -1) {
	SImsg(SIM_GERR, "swapctl(SC_AINFO) failed: %s", SYSERR);
	return((char *) NULL);
    }

    SImsg(SIM_DBG, "GetVirtMemSwapctl: max=%d free=%d resv=%d",
	  AnonInfo.ani_max, AnonInfo.ani_free, AnonInfo.ani_resv);

    Amount = (Large_t)AnonInfo.ani_max;

    if (Amount) {
	/*
	 * Try to avoid overflow
	 */
	PageSize = getpagesize();
	if (PageSize >= KBYTES)
	    Amount = Amount * ((Large_t)((PageSize / KBYTES)));
	else
	    Amount = (Amount * (Large_t)PageSize) / (Large_t)KBYTES;
	SImsg(SIM_DBG, "GetVirtMemAnon: Amount=%.0f PageSize=%d",
	      (float) Amount, PageSize);
    }

    ValStr = GetVirtMemStr(Amount);
#endif	/* HAVE_ANONINFO && HAVE_SWAPCTL */

#if	defined(HAVE_SWAPCTL) && defined(SC_GETSWAPVIRT)
    if (swapctl(SC_GETSWAPVIRT, (void *) &Amount) == -1) {
	SImsg(SIM_GERR, "swapctl(SC_GETSWAPVIRT) failed: %s", SYSERR);
	return((char *) NULL);
    }

    SImsg(SIM_DBG, "SC_GETSWAPVIRT = %.0f pages", (float) Amount);

    ValStr = GetVirtMemStr(Amount * (Large_t) 512);
#endif	/* HAVE_SWAPCTL && SC_GETSWAPVIRT */
    
    return(ValStr);
}

/*
 * Use the "nswap" symbol to determine amount of
 * virtual memory.
 */
#if	defined(NSWAP_SYM) && !defined(HAVE_NSWAP)
#	define HAVE_NSWAP
#endif	/* NSWAP_SYM */

#if	defined(HAVE_NSWAP)

#if	!defined(NSWAP_SIZE)
#	define NSWAP_SIZE	512
#endif	/* NSWAP_SIZE */
#if	!defined(NSWAP_SYM)
#	define NSWAP_SYM	"_nswap"
#endif	/* NSWAP_SYM */

#endif	/* HAVE_NSWAP */

extern char *GetVirtMemNswap()
{
#if	defined(HAVE_NSWAP)
    kvm_t		       *kd;
    int				Nswap;
    Large_t			Amount = 0;
    nlist_t		       *nlPtr;

    if (kd = KVMopen()) {
	if ((nlPtr = KVMnlist(kd, NSWAP_SYM, (nlist_t *)NULL, 0)) == NULL) {
	    KVMclose(kd);
	    return(0);
	}

	if (CheckNlist(nlPtr)) {
	    KVMclose(kd);
	    return(0);
	}

	if (KVMget(kd, nlPtr->n_value, (char *) &Nswap,
		   sizeof(Nswap), KDT_DATA) >= 0)
	    Amount = (Large_t)Nswap;

	Amount /= (Large_t)KBYTES / (Large_t)NSWAP_SIZE;
	SImsg(SIM_DBG, "GetVirtMemNswap: Amount=%.0f Nswap=%d",
	      (float) Amount, Nswap);

	KVMclose(kd);
    }

    return(GetVirtMemStr(Amount));
#else	/* !HAVE_NSWAP */
    return((char *) NULL);
#endif	/* HAVE_NSWAP */
}

/*
 * Get Virtual Memory
 */
extern int GetVirtMem(Query)
     MCSIquery_t	      *Query;
{
    extern PSI_t	       GetVirtMemPSI[];
    static char		      *Str;

    if (Query->Op == MCSIOP_CREATE) {
	if (Str = PSIquery(GetVirtMemPSI)) {
	    Query->Out = (Opaque_t) strdup(Str);
	    Query->OutSize = strlen(Str);
	    return 0;
	}
    } else if (Query->Op == MCSIOP_DESTROY) {
	if (Query->Out && Query->OutSize)
	    (void) free(Query->Out);
	return 0;
    }

    return -1;
}
