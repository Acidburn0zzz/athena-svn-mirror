/* $Header: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sun4x_57/include/afs/ptclient.h,v 1.1.1.1 2000-03-29 21:27:20 ghudson Exp $ */
/* $Source: /afs/dev.mit.edu/source/repository/third/afsbin/arch/sun4x_57/include/afs/ptclient.h,v $ */


/*
 * P_R_P_Q_# (C) COPYRIGHT IBM CORPORATION 1988
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */

/*	
       Sherri Nichols
       Information Technology Center
       November, 1988
*/

#if defined(UKERNEL)
#include "../afs/lock.h"
#include "../afs/ubik.h"
#include "../afsint/ptint.h"
#include "../afs/ptserver.h"
#else /* defined(UKERNEL) */
#include <lock.h>
#include <ubik.h>
#include "ptint.h"
#include "ptserver.h"
#endif /* defined(UKERNEL) */


extern int PR_INewEntry();
extern int PR_WhereIsIt();
extern int PR_DumpEntry();
extern int PR_AddToGroup();
extern int PR_NameToID();
extern int PR_IDToName();
extern int PR_Delete();
extern int PR_RemoveFromGroup();
extern int PR_GetCPS();
extern int PR_NewEntry();
extern int PR_ListMax();
extern int PR_SetMax();
extern int PR_ListEntry();
extern int PR_ListEntries();
extern int PR_ChangeEntry();
extern int PR_ListElements();
extern int PR_IsAMemberOf();
extern int PR_SetFieldsEntry();
extern int PR_ListOwned();
extern int PR_GetCPS2();
extern int PR_GetHostCPS();
extern int PR_UpdateEntry();

#define pr_ErrorMsg error_message
