/*
 * Copyright (c) 1992-1998 by Michael A. Cooper.
 * All rights reserved.
 */

/*
 * You may disable this code if you meet the terms of the license
 * which qualifies you for a free license.  If this is not the case,
 * YOU MAY NOT CHANGE, DISABLE, OR ALTER THE LICENSE MECHANISM IN
 * ANY MANNER.  DOING SO IS A VIOLATION OF THE LICENSE.
 * And it's not very nice either.
 */

#ifndef lint
static char *RCSid = "$Revision: 1.1.1.1 $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include "mcregex.h"
#include "mcl.h"

#ifndef PRTS
#define PRTS(s)			( ( s && *s ) ? s : "" )
#endif

/*
 * We need to be linked with -lresolv on most platforms.
 */

/*
 * File Globals
 */
static char			MsgBuf[BUFSIZ];
static int 			Debug = 0;

static char *ExceptDomains[] = { "\\.org$",
				 "\\.edu$", "\\.edu\\...$", "\\.sun.com$",
				 "\\.cc\\...\\.us$",
				 "\\.ac\\...$", 
				 "\\.uni-.*\\.de$", "uni-.*\\.de$", 
				 NULL };

#define SP(s)	( (s) ? s : "" )

/* 
 * Find the Type of license
 */
static int FindType(LicType)
     char		       *LicType;
{
    if (!LicType)
	return 0;

    if (strcasecmp(LicType, MCLN_FULL) == 0)
	return(MCL_FULL);
    else if (strcasecmp(LicType, MCLN_DEMO) == 0)
	return(MCL_DEMO);
    else if (strcasecmp(LicType, MCLN_UPGRADE) == 0)
	return(MCL_UPGRADE);
    else
	return 0;
}

/* 
 * Get the Type of license
 */
static char *GetType(LicType)
     int			LicType;
{
    if (LicType == MCL_FULL)
	return(MCLN_FULL);
    else if (LicType == MCL_DEMO)
	return(MCLN_DEMO);
    else if (LicType == MCL_UPGRADE)
	return(MCLN_UPGRADE);
    else
	return NULL;
}

/*
 * Get expiration time
 */
static time_t GetExpires(TimeStr, Query)
     char		       *TimeStr;
     mcl_t		       *Query;
{
    char		       *cp;
    time_t			TimeVal;
    struct stat			StatBuf;

    if (!TimeStr || !Query)
	return (time_t)-1;

    /*
     * If TimeStr starts with "+" the time is relative to time
     * the config file was created.
     */
    if (*TimeStr == '+') {
	if (stat(Query->File, &StatBuf) != 0) {
	    (void) snprintf(MsgBuf, sizeof(MsgBuf), "Cannot stat: %s: %s", 
			    Query->File, SYSERR);
	    Query->Msg = MsgBuf;
	    return (time_t)-1;
	}
	TimeVal = strtol(TimeStr+1, (char **)NULL, 0) + StatBuf.st_ctime;
    } else 
	TimeVal = strtol(TimeStr, (char **) NULL, 0);

    return TimeVal;
}

/*
 * Get our domain name using the DNS resolver library
 */
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
static char *GetMyDomainResolv(Query, MyHostname)
     mcl_t		       *Query;
     char		       *MyHostname;
{
    (void) res_init();
    if (!_res.defdname || !_res.defdname[0]) {
	Query->Msg = "DNS Resolver didn't provide a domain.";
	return ((char *) NULL);
    }

    return((char *)strdup(_res.defdname));
}

/*
 * Get our domain name using gethostbyname() and looking
 * to see if there's a fully qualified domain name in our hostname.
 */
static char *GetMyDomainGetHost(Query, MyHostname)
     mcl_t		       *Query;
     char		       *MyHostname;
{
    struct hostent	       *HostEnt = NULL;
    char		       *Domain = NULL;
    char		       *HostName = NULL;
    register char	       *cp;

    if ((char *) strchr(MyHostname, '.'))
	HostName = (char *) strdup(MyHostname);
    else {
	HostEnt = gethostbyname(MyHostname);
	if (!HostEnt) {
	    snprintf(MsgBuf, sizeof(MsgBuf), 
		     "gethostbyname for `%s' failed: %s", 
		     MyHostname, SYSERR);
	    Query->Msg = MsgBuf;
	    return((char *) NULL);
	}

	if (!HostEnt->h_name || !HostEnt->h_name[0]) {
	    Query->Msg = "Empty hostent->h_name value.";
	    return((char *) NULL);
	}
	HostName = (char *) strdup(HostEnt->h_name);
    }

    if (cp = (char *) strchr(HostName, '.')) {
	Domain = cp+1;
	return(Domain);
    } else {
	snprintf(MsgBuf, sizeof(MsgBuf),
		 "Hostname `%s' does not contain a domain.", HostName);
	Query->Msg = MsgBuf;
	return((char *) NULL);
    }
}

/*
 * Make String all lower case.
 */
static void strtolower(String)
    char		       *String;
{
    register char	       *cp;

    for (cp = String; cp && *cp; ++cp)
	if (isupper(*cp))
	    *cp = tolower(*cp);
}

/*
 * Get our hostname
 */
static char *GetHostName()
{
    static char 		Buf[MAXHOSTNAMELEN+1];

    gethostname(Buf, sizeof(Buf));

    return((Buf[0]) ? Buf : (char *) NULL);
}

static char *GetMyDomain(Query, MyHostname)
     mcl_t		       *Query;
     char		       *MyHostname;
{
    char		       *Domain;

    if (Domain = GetMyDomainGetHost(Query, MyHostname)) {
	strtolower(Domain);
	return(Domain);
    }

    if (Domain = GetMyDomainResolv(Query, MyHostname)) {
	strtolower(Domain);
	return(Domain);
    }

    return((char *) NULL);
}

/*
 * Do we get a freebie based on our domain name?
 */
static int IsExceptedDomain(Query)
     mcl_t		       *Query;
{
    char		       *MyDomain;
    char		       *MyHostname;
    register int		i;

    if (!(MyHostname = GetHostName()))
	return(0);

    if (!(MyDomain = GetMyDomain(Query, MyHostname)))
	return(0);

#if	defined(MCL_DEBUG)
    MyDomain = (char *) getenv("MYDOMAIN");
#endif	/* MCL_DEBUG */
    if (Debug)
	printf("MCL: MyHostname=<%s> Domain=<%s>\n", 
	       SP(MyHostname), SP(MyDomain));

    if (!MyDomain)
	return(0);

    for (i = 0; ExceptDomains[i]; ++i) {
	if (REmatch(MyDomain, ExceptDomains[i], NULL) > 0) {
	    /* Set MsgBuf to name of Domain for later usage */
	    Query->Msg = MyDomain;
	    return(1);
	}
    }

    return(0);
}

/*
 * Get printable time string
 */
static char *GetTime(TimeVal)
     time_t			TimeVal;
{
    struct tm		       *tmVal;
    static char			Buf[128];

    tmVal = localtime(&TimeVal);

    if (strftime(Buf, sizeof(Buf), "%c", tmVal) > 0)
	return(Buf);
    else
	return (char *) NULL;
}

/*
 * Return list of licenses read from license file.
 */
static mcl_t *MCLreadFile(Query)
     mcl_t		       *Query;
{
    mcl_t		       *Licenses = NULL;
    mcl_t		       *New = NULL;
    mcl_t		       *LastPtr = NULL;
    FILE		       *FilePtr;
    static char			Buff[BUFSIZ];
    int				LineNo = 0;
    int				Argc;
    static char		      **Argv;
    register char	       *cp;
    int				i = 0;

    if (!Query || !Query->File) {
	Query->Msg = "Invalid arguments";
	return((mcl_t *)NULL);
    }

    if (Debug)
	printf("MCL: Reading %s\n", Query->File);

    FilePtr = fopen(Query->File, "r");
    if (!FilePtr) {
	(void) snprintf(MsgBuf, sizeof(MsgBuf),
			"Could not open license file: %s: %s",
			Query->File, SYSERR);
	Query->Msg = MsgBuf;
	return((mcl_t *)NULL);
    }

    /*
     * License Strings look like:
     *
     * 	Product|ProdVers|Key|Type|Expires|NumRTU|HomePage|LicenseURL|\
     *		OwnerID|OwnerName
     */
    while (fgets(Buff, sizeof(Buff), FilePtr)) {
	++LineNo;
	if (Buff[0] == '#' || Buff[0] == '\n')
	    continue;
	if (cp = (char *) strchr(Buff, '\n'))	*cp = CNULL;
	if (cp = (char *) strchr(Buff, '#'))	*cp = CNULL;
	if (!Buff[0])
	    continue;

	Argc = StrToArgv(Buff, "|", &Argv, "\"\"", 0);
	if (Argc < 4) 	/* Minimum Number of Fields in file */
	    continue;
	if (strcasecmp(Argv[0], Query->Product) != 0)
	    /* Not the product we're looking for */
	    continue;

	New = (mcl_t *) calloc(1, sizeof(mcl_t));
	if (!New) {
	    Query->Msg = "calloc failed.";
	    fclose(FilePtr);
	    return((mcl_t *) NULL);
	}
	i = 0;
	New->Product = Argv[i++];
	New->ProdVers = Argv[i++];
	New->Key = Argv[i++];
	New->Type = FindType(Argv[i++]);
	if ((New->Expires = GetExpires(Argv[i++], Query)) == (time_t)-1) {
	    fclose(FilePtr);
	    return((mcl_t *) NULL);
	}
	if (i < Argc) {
	    cp = Argv[i++];
	    if (cp && strlen(cp))
		New->NumRTU = atoi(cp);
	}
	if (i < Argc) New->HomePage = Argv[i++];
	if (i < Argc) New->LicenseURL = Argv[i++];
	if (i < Argc) {
	    cp = Argv[i++];
	    if (cp && strlen(cp))
		New->OwnerID = atoi(cp);
	}
	if (i < Argc) New->OwnerName = Argv[i++];

	/*
	 * Add it to list
	 */
	if (!Licenses)
	    LastPtr = Licenses = New;
	else {
	    LastPtr->Next = New;
	    LastPtr = New;
	}
    }

    fclose(FilePtr);

    return(Licenses);
}

static mcl_t *MCLfindByType(LicType, Query, Licenses)
     int			LicType;
     mcl_t		       *Query;
     mcl_t		       *Licenses;
{
    register mcl_t	       *Ptr;

    if (LicType == 0 || !Query || !Query->Product || !Query->ProdVers || 
	!Licenses)
	return((mcl_t *) NULL);

    if (Debug)
	printf("MCL: Find  Type=<%s> Product=<%s> Vers=<%s>\n",
	       PRTS(GetType(LicType)), PRTS(Query->Product), 
	       PRTS(Query->ProdVers));

    for (Ptr = Licenses; Ptr; Ptr = Ptr->Next) {
	if (Debug)
	    printf("MCL: Check Type=<%s> Product=<%s> Vers=<%s>\n",
		   PRTS(GetType(Ptr->Type)), PRTS(Ptr->Product), 
		   PRTS(Ptr->ProdVers));
	if (Ptr->Type == LicType && 
	    (strcasecmp(Ptr->Product, Query->Product) == 0) &&
	    (strcasecmp(Ptr->ProdVers, Query->ProdVers) == 0))
	    return(Ptr);
    }

    return((mcl_t *) NULL);
}

static void MCLfreeList(List, Except)
     mcl_t		       *List;
     mcl_t		       *Except;
{
    register mcl_t	       *Ptr;
    register mcl_t	       *Last = NULL;

    for (Ptr = List; Ptr; Ptr = Ptr->Next) {
	if (Except && (Ptr == Except))
	    continue;

	if (Ptr->Product)	(void) free(Ptr->Product);
	if (Ptr->HomePage)	(void) free(Ptr->HomePage);
	if (Ptr->LicenseURL)	(void) free(Ptr->LicenseURL);
	if (Ptr->OwnerName)	(void) free(Ptr->OwnerName);
	if (Last)
	    (void) free(Last);
	Last = Ptr;
    }
}

/*
 * Turn on debugging if MCLF_DEBUG is set
 */
static void CheckDebug(Query)
     mcl_t		       *Query;
{
    if (getenv("MCL_DEBUG"))
        Debug = 1;
    if (Query && Query->Flags && Query->Flags & MCLF_DEBUG)
	Debug = 1;
}

/*
 * Perform checks for a DEMO license
 */
static int DemoCheck(Query)
     mcl_t		       *Query;
{
    struct stat			Stat;
    time_t			Now;
    time_t			Expires;

#define ONEDAY	(24 * 60 * 60)

    if (!Query || !Query->DemoPeriod || !Query->TimeStampFile)
	return -1;

    Query->Type = MCL_DEMO;

    if (stat(Query->TimeStampFile, &Stat) != 0) {
	if (Debug) printf("MCL: DemoCheck %s: Stat failed: %s\n",
			  Query->TimeStampFile, SYSERR);
	return -1;
    }

    time(&Now);
    Query->Expires = Expires = Query->DemoPeriod + Stat.st_mtime;
    Query->DaysLeft = (Expires - Now) / ONEDAY;
    Query->NumRTU = MCL_UNLIMITED;

    if (Debug) {
	printf("MCL: DemoCheck %s   Mtime=<%s>\n",
	       Query->TimeStampFile, GetTime(Stat.st_mtime));
	printf("MCL: DemoCheck     Now=<%s>\n", GetTime(Now));
	printf("MCL: DemoCheck Expires=<%s>\n", GetTime(Expires));
	printf("MCL: DemoCheck %d Demo Days\n",
	       Query->DemoPeriod / ONEDAY);
    }

    if (Now >= Expires) {
	Query->Status = MCL_STAT_EXPIRED;
	return -1;
    }

    return 0;
}

/*
 * Get an MCL license.
 *
 * 	Query->File used to identify file to find.
 * 	Query->Product used to identify what product to find in Query->File.
 *
 * Return 0 on success.
 * Return -1 on error and Query->ErrMsg set to error string.
 */
extern int MCLcheck(Query)
     mcl_t		       *Query;
{
    mcl_t		       *Licenses;
    mcl_t		       *License;
    static char			WhenExp[256];
    char		       *File = NULL;
    int				Status;

    CheckDebug(Query);

    if (IsExceptedDomain(Query) > 0) {
	Query->Status = MCL_STAT_OK;
	return(0);
    }

    /*
     * Environment variable MC_LICENSE_ENV overrides builtin
     */
    File = (char *) getenv(MC_LICENSE_ENV);
    if (File)
	Query->File = File;

    if (!Query || !Query->File || !Query->Product) {
	Query->Msg = "Invalid arguments";
	Query->Status = MCL_STAT_UNKNOWN;
	return(-1);
    }

    Licenses = MCLreadFile(Query);
    if (!Licenses && Debug)
	printf("MCL: No licenses were found.\n");
    if (!(License = MCLfindByType(MCL_FULL, Query, Licenses))) {
	if (Licenses && Debug) printf("MCL: No FULL license was found.\n");
	/* Look for an upgrade license now */
	if (!(License = MCLfindByType(MCL_UPGRADE, Query, Licenses))) {
	    if (Licenses && Debug) 
		printf("MCL: No UPGRADE license was found.\n");
	    /* Look for a demo license */
	    License = MCLfindByType(MCL_DEMO, Query, Licenses);	
	    if (!License && Licenses && Debug)
		printf("MCL: No DEMO license was found in the license file.\n");
	}
    }

    if (!License && Query->TimeStampFile) {
	if (Debug) printf("MCL: Checking automatic DEMO license.\n");
	Status = DemoCheck(Query);
	MCLfreeList(Licenses, NULL);
	return Status;
    }

    if (!License) {
	snprintf(MsgBuf, sizeof(MsgBuf),
		 "Could not find license entry for `%s %s' in %s",
		 Query->Product, Query->ProdVers, Query->File);
	Query->Msg = MsgBuf;
	MCLfreeList(Licenses, NULL);
	Query->Status = MCL_STAT_NOTFOUND;
	return(-1);
    }

    MCLfreeList(Licenses, License);

    /*
     * Is the license expired?
     */
    if (License->Expires > 0 && (License->Expires < time((time_t *) 0))) {
	(void) snprintf(MsgBuf, sizeof(MsgBuf),
			"Your %s license EXPIRED on %s.",
			GetType(License->Type), GetTime(License->Expires));
	Query->Msg = MsgBuf;
	Query->Status = MCL_STAT_EXPIRED;
	return(-1);
    }

    Query->Product 	= License->Product;
    Query->ProdVers 	= License->ProdVers;
    Query->Key 		= License->Key;
    Query->Type 	= License->Type;
    Query->Expires 	= License->Expires;
    Query->HomePage 	= License->HomePage;
    Query->LicenseURL 	= License->LicenseURL;
    Query->NumRTU 	= License->NumRTU;
    Query->OwnerID 	= License->OwnerID;
    Query->OwnerName 	= License->OwnerName;
    Query->Status 	= MCL_STAT_OK;

    return (0);
}

/*
 * Get printable information about license
 */
extern char *MCLgetInfo(License)
     mcl_t		       *License;
{
    static char			Buff[BUFSIZ];
    static mcl_t		Query;
    char		       *cp;
    int				l;

    CheckDebug(License);

    Buff[0] = CNULL;

    (void) memset(&Query, CNULL, sizeof(Query));
    if (IsExceptedDomain(&Query) > 0) {
	(void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l,
       "Your domain (%s) entitles you to use this software FREE of charge.\n",
			Query.Msg);
	License->Key = "<FREE>";
	License->NumRTU = MCL_UNLIMITED;
	License->Type = MCL_FULL;
    }

    if (License && License->Product && License->Type && License->ProdVers) {
#define OFFSET 18
	(void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l,
			"LICENSE INFO for %s\n", License->Product);

	(void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			OFFSET, "Licensed Product", License->Product);
	(void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			OFFSET, "Product Version", License->ProdVers);

	if (License->Key)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			    OFFSET, "License Key", License->Key);
	if (License->Type)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			    OFFSET, "License Type", GetType(License->Type));

	(void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, 
			"%*s: %s\n", OFFSET, "License Expires",
			(License->Expires > 0) ? GetTime(License->Expires) : 
			"NEVER");

	(void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: ",
			OFFSET, "Number of RTUs");
	if (License->NumRTU == MCL_UNLIMITED)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, 
			    "Unlimited\n");
	else
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%d\n",
			    License->NumRTU);

	if (License->OwnerName)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			    OFFSET, "Licensed To", License->OwnerName);
	if (License->OwnerID)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %d\n",
			    OFFSET, "Licensed To", License->OwnerID);
	if (License->LicenseURL)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			    OFFSET, "License Terms", License->LicenseURL);
	if (License->HomePage)
	    (void) snprintf(&Buff[l=strlen(Buff)], sizeof(Buff)-l, "%*s: %s\n",
			    OFFSET, "Product HomePage", License->HomePage);
#undef OFFSET
    }

    return (Buff[0]) ? Buff : (char *) NULL;
}
