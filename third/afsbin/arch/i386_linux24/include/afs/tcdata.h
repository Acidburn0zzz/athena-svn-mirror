/*
 * tcdata.h:
 * This file is automatically generated; please do not edit it.
 */
/* Including tcdata.p.h at beginning of tcdata.h file. */

/*
 * Copyright 2000, International Business Machines Corporation and others.
 * All Rights Reserved.
 * 
 * This software has been released under the terms of the IBM Public
 * License.  For details, see the LICENSE file in the top-level source
 * directory or online at http://www.openafs.org/dl/license10.html
 */

#ifndef _TCDATA_
#define	_TCDATA_    1

#include <afs/butc.h>
#include <afs/budb.h>
#include <afs/bubasics.h>
#include "butm.h"

/* node with info on each dump of interest, associated with taskID
 * Key:
 * G - generated by butc
 */

struct dumpNode
{
    /* administrative fields */
    afs_int32 taskID;	                /* the task id */
    struct dumpNode *next;              /* ptr to the next node on the list */
    statusP statusNodePtr;              /* status node pointer */

    /* common to dumps and restores */
    afs_int32 arraySize;                    /* Size of dump/restore array */

    /* specific to dumps */
    char dumpSetName[TC_MAXNAMELEN];	/* name of the dump "<volset>.<dump>" */
    char *dumpName;			/* full dump path */
    char *volumeSetName;                /* volume set */
    struct tc_tapeSet tapeSetDesc;	/* description of the tape set */
    struct tc_dumpDesc *dumps;		/* list of vols to dump */
    afs_int32 parent;			/* parent dump ID, from main call */
    afs_int32 level;			/* this dump's level, from main call */
    int  doAppend;                      /* Append this dump to a dump set */

    /* restore specific */
    struct tc_restoreDesc *restores; 	/* info needed to restore volumes*/
};

struct deviceSyncNode{
    struct Lock lock; /* this is used to synchronise access to tape drive */
    afs_int32 flags;
};

/* flags used to monitor status of dumps/restores */
#define	INPROGRESS 0x01	/*dump is in progress */
#define	ABORT	0x02	/*abort this dump */
#define	ABORTED	0x04	/*aborted this dump */
#define	DONE	0x08	/*done with this dump */
#define	WAITING	0x10	/*someone is waiting on this dump */
#define	ERROR	0x20	/*error in operation */
#define	PARTIAL	0x40	/*partial failure in operation */

/* define the opcodes */
#define	DUMP 0x01	/*dumping */
#define	RESTORE	0x02	/*restoring */

/*these definitions are temporary */
#define TCPORT 7010
#define TCSERVICE_ID 10

#define	TC_GCTIME   300 /* the interval after which the GC is invoked */
#define	TC_ABORTTIME	30 /*time alotted for process to respond to  abort */
#define TC_MAXVOLNAME 64 /*this should come from VNAMESIZE in volume header*/
/*define some magic numbers to be used with volume headers and trailors on tape */
#define	TC_VOLBEGINMAGIC	0xb0258191  /*32 bit magic numbers */
#define	TC_VOLENDMAGIC		0x9167345a
#define	TC_VOLCONTD		0xffffffff
/*
#define	TC_HEADERFORMAT		"H++NAME#%s#ID#%u#SERVER#%x#PART#%u#FROM#%u#FRAG#%d#BM#%u#--H"
#define	TC_TRAILORFORMAT	"T--NAME#%s#ID#%u#SERVER#%x#PART#%u#FROM#%u#FRAG#%d#CONTD#%x#EM#%u#++T"
*/
#define TC_TRAILERBEGIN		"+=!@#$%><%$#@!=+"	/*indicates that trailer follows */
#define	TC_MAXTAPENAMELEN 100	/*dont know how to estimate these numbers */
#define TC_DEVICEINUSE 0x1000  	/* used to indicate use of device by somebody */
#define TC_NULLTAPENAME  "<NULL>"   /* default tape name */
#define TC_QUOTEDNULLTAPENAME "\"<NULL>\""  /* null tapename in quotes */

/* for user prompt generation */
#define READOPCODE  	0			/* read tape - for restore */
#define WRITEOPCODE 	1			/* write tape - for dump */
#define LABELOPCODE 	2			/* label a tape */
#define READLABELOPCODE 3			/* read tape label */
#define	SCANOPCODE	4			/* scan tape contents */
#define APPENDOPCODE    5                       /* append write tape - for dump */
#define SAVEDBOPCODE    6                       /* save a database tape */
#define RESTOREDBOPCODE 7                       /* restore a database tape */
#define CLOSEOPCODE     8                       /* close a tape drive - for callout */

/* marker used on tape. A header is placed (as a separate block) before the
 * volume data, and is appended (contiguously with the data) to the volume
 * data
 */
struct volumeHeader {
    char preamble[9];
    char postamble[9];
    char volumeName[TC_MAXVOLNAME];
    char pad[2];
    afs_int32 volumeID;
    afs_int32 server;				/* which server */
    afs_int32 part;					/* partition vol. was on */
    afs_int32 from;					/* clone date of vol ?? */
    int frag;
    afs_int32 magic;					/* just for checking */
    afs_int32 contd;
    char dumpSetName[TC_MAXNAMELEN];
    afs_int32 dumpID;				/* ID of enclosing dump */
    afs_int32 level;					/* dump level, 0=full */
    afs_int32 parentID;				/* ID of parent dump */
    afs_int32 endTime;
    afs_int32 versionflags;				/* previously spare[0] */
    afs_int32 cloneDate;				/* when this vol. was cloned */
    afs_int32 spares[2];				/* spare used to be spare[4] */
};

/* Interface structure for STC_LabelTape  */

struct labelTapeIf
{
    struct tc_tapeLabel label;
    afs_uint32 taskId;
};

struct scanTapeIf
{
    afs_int32 addDbFlag;
    afs_uint32 taskId;
};

/* Interface structure for STC_SaveDb */

struct saveDbIf
{
    Date    archiveTime;
    afs_uint32  taskId;
    statusP statusPtr;
};

/* Iterface structure for STC_DeleteDump */
struct deleteDumpIf
{
   afs_uint32 dumpID;
   afs_uint32 taskId;
};

#endif


/* End of prolog file tcdata.p.h. */

#define TC_DUMPERROR                             (156566272L)
#define TC_FORCEDABORT                           (156566273L)
#define TC_ABORTED                               (156566274L)
#define TC_WORKINPROGRESS                        (156566275L)
#define TC_INCOMPLETEDUMP                        (156566276L)
#define TC_ABORTFAILED                           (156566277L)
#define TC_ABORTEDBYREQUEST                      (156566278L)
#define TC_SCANFAILURE                           (156566279L)
#define TC_NODENOTFOUND                          (156566280L)
#define TC_NOTASKS                               (156566281L)
#define TC_VOLUMENOTONTAPE                       (156566282L)
#define TC_PREMATUREEOF                          (156566283L)
#define TC_MISSINGTRAILER                        (156566284L)
#define TC_WRONGTAPE                             (156566285L)
#define TC_TAPEUNUSABLE                          (156566286L)
#define TC_BADVOLHEADER                          (156566287L)
#define TC_INTERNALERROR                         (156566288L)
#define TC_BADQUEUE                              (156566289L)
#define TC_NOMEMORY                              (156566290L)
#define TC_NOTPERMITTED                          (156566291L)
#define TC_SKIPTAPE                              (156566292L)
#define TC_BADTASK                               (156566293L)
extern void initialize_butc_error_table ();
#define ERROR_TABLE_BASE_butc (156566272L)

/* for compatibility with older versions... */
#define init_butc_err_tbl initialize_butc_error_table
#define butc_err_base ERROR_TABLE_BASE_butc
