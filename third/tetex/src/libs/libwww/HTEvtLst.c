/*				    				     HTEvtLst.c
**	EVENT MANAGER
**
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGH.
**	@(#) $Id: HTEvtLst.c,v 1.1.1.1 2000-03-10 17:52:57 ghudson Exp $
**
**	Updated HTEvent module 
**	This new module combines the functions of the old HTEvent module and 
**	the HTThread module. We retain the old HTThread module, but it
**	consists of calls to the HTEvent interfaces
**
** Authors:
**	HFN	Henrik Frystyk <frystyk@w3.org>
**	CLB    	Charlie Brooks <cbrooks@osf.org>
** Bugs
**
*/

/*   WSAAsyncSelect and windows app stuff need the following definitions:
 *   WWW_WIN_ASYNC - enable WSAAsyncSelect instead of select
 *   _WIN23 - win32 libararies - may be window or console app
 *   _WINSOCKAPI_ - using WINSOCK.DLL - not necessarily the async routines.
 *   _CONSOLE - the console app for NT
 *
 * first pass: EGP - 10/26/95
 */

/* Implementation dependent include files */
#include "wwwsys.h"
#include "WWWUtil.h"
#include "WWWCore.h"
#include "HTReqMan.h"
#include "HTTimer.h"
#include "HTEvent.h"
#include "HTEvtLst.h"					 /* Implemented here */

/* Type definitions and global variables etc. local to this module */
#define PRIME_TABLE_SIZE	67
#define MILLI_PER_SECOND	1000
#define HASH(s)			((s) % PRIME_TABLE_SIZE) 
#define HT_EVENT_ORDER				  /* use event ordering code */
#define EVENTS_TO_EXECUTE	5  /* how many to execute in one select loop */

#ifdef WWW_WIN_ASYNC
#define TIMEOUT	1 /* WM_TIMER id */
PRIVATE HWND HTSocketWin;
PRIVATE unsigned long HTwinMsg;
#else /* WWW_WIN_ASYNC */
PRIVATE fd_set FdArray[HTEvent_TYPES];
PRIVATE SOCKET MaxSock = 0;			  /* max socket value in use */
#endif /* !WWW_WIN_ASYNC */

#define HT_FD_BYTES(a)	((a/16)+1)*4

typedef struct {
    SOCKET 	s ;	 		/* our socket */
    HTEvent * 	events[HTEvent_TYPES];	/* event parameters for read, write, oob */
#ifndef IN_EVENT
    HTTimer *	timeouts[HTEvent_TYPES];
#endif
} SockEvents;

typedef enum {
    SockEvents_mayCreate,
    SockEvents_find
} SockEvents_action;

HTList * HashTable[PRIME_TABLE_SIZE]; 
PRIVATE int HTEndLoop = 0;		       /* If !0 then exit event loop */

/* ------------------------------------------------------------------------- */

#ifdef WWW_WIN_ASYNC
PRIVATE BOOL Timer_setWindowsTimer (HTTimer * timer)
{
    HWND hwnd;
    UINT id;
    hwnd = HTEventList_getWinHandle(&id);
    return SetTimer(hwnd, (UINT)timer, (UINT)HTTimer_getTime(timer), NULL) != 0;
}

PRIVATE BOOL Timer_deleteWindowsTimer (HTTimer * timer)
{
    HWND hwnd;
    UINT id;
    hwnd = HTEventList_getWinHandle(&id);
    return KillTimer(hwnd, (UINT)timer) != 0;
}
#endif /* WWW_WIN_ASYNC */

PRIVATE SockEvents * SockEvents_get (SOCKET s, SockEvents_action action)
{
    long v = HASH(s);
    HTList* cur;
    SockEvents * pres;

    if (HashTable[v] == NULL)
	HashTable[v] = HTList_new();
    cur = HashTable[v];
    while ((pres = (SockEvents *) HTList_nextObject(cur)))
	if (pres->s == s)
	    return pres;

    if (action == SockEvents_mayCreate) {
        if ((pres = (SockEvents *) HT_CALLOC(1, sizeof(SockEvents))) == NULL)
	    HT_OUTOFMEM("HTEventList_register");
	pres->s = s;
	HTList_addObject(HashTable[v], (void *)pres);
	return pres;
    }
    return NULL;
}

PUBLIC void HTEvent_traceHead(void)
{
    HTTrace("     event: pri millis  callback   param    request  ");
}
PUBLIC void HTEvent_trace(HTEvent * event)
{
    if (event == NULL)
	return;
    HTTrace("%8p: %3d %6d %8p %8p %8p", event, event->priority, event->millis, event->cbf, event->param, event->request);
}
PUBLIC void HTTimer_traceHead(void)
{
    HTTrace("     timer: millis expires ?   param   callback  ");
}
PRIVATE char * MyTime(unsigned long int time, int len)
{
static char space[100];
    sprintf(space, "1234567");
    return space;
}
struct _HTTimer {
    HTTimer *	next;		/* The next guy in line */
    ms_t	millis;		/* Relative value in millis */
    ms_t	expires;	/* Absolute value in millis */
    BOOL	relative;
    void *	param;		/* Client supplied context */
    HTTimerCallback * cbf;
};

PUBLIC void HTTimer_trace(HTTimer * timer)
{
    if (timer == NULL)
	return;
    HTTrace("%8p: %6d %7s %c %8p %8p", timer, timer->millis, MyTime(timer->expires, 7), 
	    timer->relative == YES ? 'R' : 'A', timer->param, timer->cbf);
}
/*
**  A simple debug function that dumps all the socket arrays
**  as trace messages
*/
PRIVATE void EventList_dump (void)
{
    int v = 0;
    HTList* cur;
    SockEvents * pres;
#if 0
    if (HashTable[v] == NULL) {
	HTTrace("Event....... No sockets registered\n");
	return;
    }
#endif
    HTTrace("Event....... Dumping socket events\n");
    HTTrace("soc ");
    HTEvent_traceHead();
    HTTrace(" ");
    HTTimer_traceHead();
    HTTrace("\n");
    for (v = 0; v < PRIME_TABLE_SIZE; v++) {
	cur = HashTable[v];
	while ((pres = (SockEvents *) HTList_nextObject(cur))) {
	    int i;
	    HTTrace("%3d \n", pres->s);
	    for (i = 0; i < HTEvent_TYPES; i++)
		if (pres->events[i]) {
		    static char * names[HTEvent_TYPES] = {"read", "writ", "xcpt"};
		    HTTrace("%s ", names[i]);
		    HTEvent_trace(pres->events[i]);
		    HTTrace(" ");
#ifndef IN_EVENT
		    HTTimer_trace(pres->timeouts[i]);
		    HTTrace(" ");
#endif
		}
	    HTTrace("\n");
	}
    }
}

/* ------------------------------------------------------------------------- */
/*		T I M E O U T   H A N D L E R				     */
PRIVATE int EventListTimerHandler (HTTimer * timer, void * param, HTEventType type)
{
    SockEvents * sockp = (SockEvents *) param;
    HTEvent * event = NULL;
    /* HTMemLog_flush(); keep around - very useful for debugging crashes - EGP */
#ifdef IN_EVENT
    if (sockp->events[HTEvent_INDEX(HTEvent_READ)]->timer == timer)
#else /* IN_EVENT */
    if (sockp->timeouts[HTEvent_INDEX(HTEvent_READ)] == timer)
#endif /* !IN_EVENT */
    {
	event = sockp->events[HTEvent_INDEX(HTEvent_READ)];
	if (THD_TRACE) HTTrace("Event....... READ timed out on %d.\n", sockp->s);
	return (*event->cbf) (sockp->s, event->param, HTEvent_TIMEOUT);
    }
#ifdef IN_EVENT
    if (sockp->events[HTEvent_INDEX(HTEvent_WRITE)]->timer == timer)
#else /* IN_EVENT */
    if (sockp->timeouts[HTEvent_INDEX(HTEvent_WRITE)] == timer)
#endif /* !IN_EVENT */
    {
	event = sockp->events[HTEvent_INDEX(HTEvent_WRITE)];
	if (THD_TRACE) HTTrace("Event....... WRITE timed out on %d.\n", sockp->s);
	return (*event->cbf) (sockp->s, event->param, HTEvent_TIMEOUT);
    }
#ifdef IN_EVENT
    if (sockp->events[HTEvent_INDEX(HTEvent_OOB)]->timer == timer)
#else /* IN_EVENT */
    if (sockp->timeouts[HTEvent_INDEX(HTEvent_OOB)] == timer)
#endif /* !IN_EVENT */
    {
	event = sockp->events[HTEvent_INDEX(HTEvent_OOB)];
	if (THD_TRACE) HTTrace("Event....... OOB timed out on %d.\n", sockp->s);
	return (*event->cbf) (sockp->s, event->param, HTEvent_TIMEOUT);
    }
    if (THD_TRACE)
	HTTrace("Event....... Can't find event for timer %p with context %p\n",
		timer, param);
    return HT_ERROR;
}

/* ------------------------------------------------------------------------- */
/*		E V E N T   O R D E R I N G   S T U F F			     */
#ifdef HT_EVENT_ORDER
typedef struct {
    HTEvent *	event;
    SOCKET	s;
    HTEventType	type;
    HTPriority	skipped;
} EventOrder;

HTList * EventOrderList = NULL;
#if 0
/*
**	return -1 if a should be after b
 */
int EventOrderComparer (const void * a, const void * b)
{
    EventOrder * placeMe = (EventOrder *)a;
    EventOrder * maybeHere = (EventOrder *)b;
    if (placeMe->event->priority+placeMe->skipped >= maybeHere->event->priority+maybeHere->skipped)
	return 1;
    return -1;
}
#endif

int EventOrder_add (SOCKET s, HTEventType type, ms_t now)
{
    EventOrder * pres;
    HTList * cur = EventOrderList;
    HTList * insertAfter = cur;
    SockEvents * sockp = SockEvents_get(s, SockEvents_find);
    HTEvent * event;

    if (sockp == NULL || (event = sockp->events[HTEvent_INDEX(type)]) == NULL) {
	HTTrace("EventOrder.. no event found for socket %d, type %s.\n", s, HTEvent_type2str(type));
	return HT_ERROR;
    }

    /*	Fixup the timeout
    */
#ifdef IN_EVENT
    if (event->timer)
	HTTimer_refresh(event->timer, now);
#else
    if (sockp->timeouts[HTEvent_INDEX(type)])
	HTTimer_refresh(sockp->timeouts[HTEvent_INDEX(type)], now);
#endif

    /*
    **	Look to see if it's already here from before
    */
    while ((pres = (EventOrder *) HTList_nextObject(cur))) {
	if (pres->s == s && pres->event == event && pres->type == type) {
	    pres->skipped++;
	    return HT_OK;
	}
	if (pres->event->priority+pres->skipped > event->priority)
	    insertAfter = cur;
    }

    /*
    **	No, so create a new element
    */
    if ((pres = (EventOrder *) HT_CALLOC(1, sizeof(EventOrder))) == NULL)
	HT_OUTOFMEM("EventOrder_add");
    pres->event = event;
    pres->s = s;
    pres->type = type;
    HTList_addObject(insertAfter, (void *)pres);
    return HT_OK;
}

PUBLIC int EventOrder_executeAndDelete (void) 
{
    HTList * cur = EventOrderList;
    EventOrder * pres;
    int i = 0;
    if (THD_TRACE) HTTrace("EventOrder.. execute ordered events\n");
    if (cur == NULL) return NO;
    while ((pres = (EventOrder *) HTList_removeLastObject(cur)) && i < EVENTS_TO_EXECUTE) {
	HTEvent * event = pres->event;
	int ret;
	if (THD_TRACE)
	HTTrace("EventList... calling socket %d, request %p handler %p type %s\n",
		pres->s, (void *) event->request,
		(void *) event->cbf, HTEvent_type2str(pres->type));
	ret = (*pres->event->cbf)(pres->s, pres->event->param, pres->type);
	HT_FREE(pres);
	if (ret != HT_OK)
	    return ret;
	i++;
    }
    return HT_OK;
}

PUBLIC BOOL EventOrder_deleteAll (void) 
{
    HTList * cur = EventOrderList;
    EventOrder * pres;
    if (THD_TRACE) HTTrace("EventOrder.. all ordered events\n");
    if (cur == NULL) return NO;
    while ((pres = (EventOrder *) HTList_nextObject(cur)))
	HT_FREE(pres);
    HTList_delete(EventOrderList);
    EventOrderList = NULL;
    return YES;
}
#endif /* HT_EVENT_ORDER */

/* ------------------------------------------------------------------------- */

/*
** ResetMaxSock - reset the value of the maximum socket in use 
*/
#ifndef WWW_WIN_ASYNC
PRIVATE void __ResetMaxSock (void)
{
    SOCKET cnt;
    SOCKET t_max = 0;
    SOCKET old_max = MaxSock;
    for (cnt = 0 ; cnt <= MaxSock; cnt++) { 
	if (FD_ISSET(cnt, (FdArray + HTEvent_INDEX(HTEvent_READ))) ||
	    FD_ISSET(cnt, (FdArray + HTEvent_INDEX(HTEvent_WRITE))) ||
	    FD_ISSET(cnt, (FdArray + HTEvent_INDEX(HTEvent_OOB))))
  	    if (cnt > t_max) t_max = cnt;
    }
    MaxSock = t_max+1;
    if (THD_TRACE)
	HTTrace("Event....... Reset MaxSock from %u to %u\n", old_max, MaxSock);
    return;
}  
#endif /* !WWW_WIN_ASYNC */

PRIVATE int EventList_remaining(SockEvents * pres)
{
    int ret = 0;
    int i;
    for (i = 0; i < HTEvent_TYPES; i++)
	if (pres->events[i] != NULL)
	    ret |= 1<<i;
    return ret;
}

/*
**  For a given socket, reqister a request structure, a set of operations, 
**  a HTEventCallback function, and a priority. For this implementation, 
**  we allow only a single HTEventCallback function for all operations.
**  and the priority field is ignored.
*/
PUBLIC int HTEventList_register (SOCKET s, HTEventType type, HTEvent * event)
{
    int newset = 0;
    SockEvents * sockp;
    if (THD_TRACE) 
	HTTrace("Event....... Register socket %d, request %p handler %p type %s at priority %d\n",
		s, (void *) event->request,
		(void *) event->cbf, HTEvent_type2str(type),
		(unsigned) event->priority);
    if (s==INVSOC || HTEvent_INDEX(type) >= HTEvent_TYPES)
	return 0;

    /*
    ** Insert socket into appropriate file descriptor set. We also make sure
    ** that it is registered in the global set.
    */
    if (THD_TRACE) HTTrace("Event....... Registering socket for %s\n", HTEvent_type2str(type));
    sockp = SockEvents_get(s, SockEvents_mayCreate);
    sockp->s = s;
    sockp->events[HTEvent_INDEX(type)] = event;
    newset = EventList_remaining(sockp);
#ifdef WWW_WIN_ASYNC
    if (WSAAsyncSelect(s, HTSocketWin, HTwinMsg, HTEvent_BITS(newset)) < 0) {
        if (THD_TRACE) HTTrace("Event....... WSAAsyncSelect returned error!");
	return HT_ERROR;
    }
#else /* WWW_WIN_ASYNC */
    FD_SET(s, FdArray+HTEvent_INDEX(type));
    if (s > MaxSock) {
	MaxSock = s ;
	if (THD_TRACE) HTTrace("Event....... New value for MaxSock is %d\n", MaxSock);
    }
#endif /* !WWW_WIN_ASYNC */

    /*
    **  If the timeout has been set (relative in millis) then we register 
    **  a new timeout for this event unless we already have a timer.
    */
    if (event->millis >= 0) {
#ifdef IN_EVENT
	event->timer = HTTimer_new(event->timer, EventListTimerHandler,
				   sockp, event->millis, YES);
#else
	sockp->timeouts[HTEvent_INDEX(type)] =
	    HTTimer_new(sockp->timeouts[HTEvent_INDEX(type)],
			EventListTimerHandler, sockp, event->millis, YES, YES);
#endif
    }

    return HT_OK;
}

/*
** Remove the registered information for the specified socket for the actions 
** specified in ops. if no actions remain after the unregister, the registered
** info is deleted, and, if the socket has been registered for notification, 
** the HTEventCallback will be invoked.
*/
PUBLIC int HTEventList_unregister(SOCKET s, HTEventType type) 
{
    long 		v = HASH(s);
    HTList * 		cur = HashTable[v];
    HTList * 		last = cur;
    SockEvents *	pres;
    int			ret = HT_ERROR;

    while (cur && (pres = (SockEvents *) HTList_nextObject(cur))) {
        if (pres->s == s) {
	    int	remaining = 0;

	    /*
	    **  Unregister the event from this action
	    */
	    pres->events[HTEvent_INDEX(type)] = NULL;
            remaining = EventList_remaining(pres);

	    /*
	    **  Check to see of there was a timeout connected with the event.
	    **  If so then delete the timeout as well.
	    */
	    {
#ifdef IN_EVENT
		HTTimer * timer = pres->events[HTEvent_INDEX(type)]->timer;
                if (timer) HTTimer_delete(timer);
                pres->events[HTEvent_INDEX(type)]->timer = NULL;
#else
		HTTimer * timer = pres->timeouts[HTEvent_INDEX(type)];
                if (timer) HTTimer_delete(timer);
                pres->timeouts[HTEvent_INDEX(type)] = NULL;
#endif
		
	    }
	    
#ifdef WWW_WIN_ASYNC
	    if (WSAAsyncSelect(s, HTSocketWin, HTwinMsg, remaining) < 0)
		ret = HT_ERROR;
#else /* WWW_WIN_ASYNC */
	    FD_CLR(s, FdArray+HTEvent_INDEX(type));
	    HTTraceData((char*)FdArray+HTEvent_INDEX(type), 8, "HTEventList_unregister: (s:%d)", s);
#endif /* !WWW_WIN_ASYNC */

	    /*
	    **  Check to see if we can delete the action completely. We do this
	    **  if there are no more events registered.
	    */
	    if (remaining == 0) {
		HTList * doomed = cur;
		if (THD_TRACE)
		    HTTrace("Event....... No more events registered for socket %d\n", s);


#ifndef WWW_WIN_ASYNC
		/* Check to see if we have to update MaxSock */
		if(pres->s >= MaxSock) __ResetMaxSock();
#endif /* !WWW_WIN_ASYNC */

		HT_FREE(pres);
		pres = (SockEvents *) HTList_nextObject(cur);
		HTList_quickRemoveElement(doomed, last);
	    }
	    ret = HT_OK;

      	    if (THD_TRACE) HTTrace("Event....... Socket %d unregistered for %s\n", s,
				   HTEvent_type2str(type));

	    /*
	    **  We found the socket and can break
	    */
	    break;
	}
	last = cur;
    }
    if (ret == HT_ERROR && THD_TRACE) HTTrace("Event....... Couldn't find socket %d. Can't unregister type %s\n",
        s, HTEvent_type2str(type));
    return ret;
}

/*
** Unregister all sockets 
** N.B. we just remove them for our internal data structures: it is up to the 
** application to actually close the socket. 
*/
PUBLIC int HTEventList_unregisterAll (void) 
{
    int i;
    if (THD_TRACE) HTTrace("Unregister.. all sockets\n");
    for (i = 0 ; i < PRIME_TABLE_SIZE; i++) {
	HTList * cur = HashTable[i];
	SockEvents * pres;
	while ((pres = (SockEvents *) HTList_nextObject(cur))) {
#ifdef WWW_WIN_ASYNC
	    WSAAsyncSelect(pres->s, HTSocketWin, HTwinMsg, 0);
#endif /* WWW_WIN_ASYNC */
	    HT_FREE(pres);
	}
	HTList_delete(HashTable[i]);
	HashTable[i] = NULL;
    }
#ifndef WWW_WIN_ASYNC
    MaxSock = 0 ;
    if (THD_TRACE) HTTrace("Event....... New value for MaxSock is %d\n", MaxSock);
    FD_ZERO(FdArray+HTEvent_INDEX(HTEvent_READ));
    FD_ZERO(FdArray+HTEvent_INDEX(HTEvent_WRITE));
    FD_ZERO(FdArray+HTEvent_INDEX(HTEvent_OOB));
#endif /* !WWW_WIN_ASYNC */
#ifdef HT_EVENT_ORDER
    EventOrder_deleteAll();
#endif /* HT_EVENT_ORDER */
    return 0;
}

/*
**  Dispatch the event to the appropriate event handler.
**  If no event handler is found then just return.
*/
PUBLIC int HTEventList_dispatch (SOCKET s, HTEventType type, ms_t now)
{
    SockEvents * sockp = SockEvents_get(s, SockEvents_find);
    if (sockp) {
	HTEvent * event = sockp->events[HTEvent_INDEX(type)];

	/*	Fixup the timeout
	*/
#ifdef IN_EVENT
	if (event->timer)
	    HTTimer_refresh(event->timer, now);
#else
	if (sockp->timeouts[HTEvent_INDEX(type)])
	    HTTimer_refresh(sockp->timeouts[HTEvent_INDEX(type)], now);
#endif
	/*
	**  If we have found an event object for this event then see
	**  is we should call it.
	*/
	if (event && event->priority!=HT_PRIORITY_OFF)
	    return (*event->cbf) (s, event->param, type);
	if (THD_TRACE) HTTrace("Dispatch.... Handler %p NOT called\n", sockp);
	return HT_OK;
    }
    if (THD_TRACE) HTTrace("Dispatch.... Bad socket %d\n", s);
    return NO;
}

/*
**  Stops the (select based) event loop. The function does not guarantee
**  that all requests have terminated. This is for the app to do
*/
PUBLIC void HTEventList_stopLoop (void)
{
    HTEndLoop = 1;
}

PUBLIC HTEvent * HTEventList_lookup (SOCKET s, HTEventType type)
{
    SockEvents * sockp = NULL;
    if ((sockp = SockEvents_get(s, SockEvents_find)) == NULL)
	return NULL;
    return sockp->events[HTEvent_INDEX(type)];
}

/*	REGISTER DEFULT EVENT MANAGER
**	-----------------------------
**	Not done automaticly - may be done by application!
*/
PUBLIC BOOL HTEventInit (void)
{
#ifdef WWW_WIN_ASYNC
    /*
    **	We are here starting a hidden window to take care of events from
    **  the async select() call in the async version of the event loop in
    **	the Internal event manager (HTEvtLst.c)
    */
    static char className[] = "AsyncWindowClass";
    WNDCLASS wc;
    OSVERSIONINFO osInfo;
    
    wc.style=0;
    wc.lpfnWndProc=(WNDPROC)AsyncWindowProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hIcon=0;
    wc.hCursor=0;
    wc.hbrBackground=0;
    wc.lpszMenuName=(LPSTR)0;
    wc.lpszClassName=className;

    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    GetVersionEx(&osInfo);
    if (osInfo.dwPlatformId == VER_PLATFORM_WIN32s || osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	wc.hInstance=GetModuleHandle(NULL); /* 95 and non threaded platforms */
    else
	wc.hInstance=GetCurrentProcess(); /* NT and hopefully everything following */
    if (!RegisterClass(&wc)) {
    	HTTrace("HTLibInit.. Can't RegisterClass \"%s\"\n", className);
	    return NO;
    }
    if (!(HTSocketWin = CreateWindow(className, "WWW_WIN_ASYNC", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 
                                     CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, wc.hInstance,0))) {
	char space[50];
       	HTTrace("HTLibInit.. Can't CreateWindow \"WWW_WIN_ASYNC\" - error:");
	sprintf(space, "%ld\n", GetLastError());
	HTTrace(space);
    	return NO;
    }
    HTwinMsg = WM_USER;  /* use first available message since app uses none */

    /*
    **  Register platform specific timer handlers for windows
    */
    HTTimer_registerSetTimerCallback(Timer_setWindowsTimer);
    HTTimer_registerDeleteTimerCallback(Timer_deleteWindowsTimer);

#endif /* WWW_WIN_ASYNC */

#ifdef _WINSOCKAPI_
    /*
    ** Initialise WinSock DLL. This must also be shut down! PMH
    */
    {
        WSADATA            wsadata;
	if (WSAStartup(DESIRED_WINSOCK_VERSION, &wsadata)) {
	    if (WWWTRACE)
		HTTrace("HTEventInit. Can't initialize WinSoc\n");
            WSACleanup();
            return NO;
        }
        if (wsadata.wVersion < MINIMUM_WINSOCK_VERSION) {
            if (WWWTRACE)
		HTTrace("HTEventInit. Bad version of WinSoc\n");
            WSACleanup();
            return NO;
        }
	if (APP_TRACE)
	    HTTrace("HTEventInit. Using WinSoc version \"%s\".\n", 
		    wsadata.szDescription);
    }
#endif /* _WINSOCKAPI_ */

    HTEvent_setRegisterCallback(HTEventList_register);
    HTEvent_setUnregisterCallback(HTEventList_unregister);
    return YES;
}

PUBLIC BOOL HTEventTerminate (void)
{
#ifdef _WINSOCKAPI_
    WSACleanup();
#endif /* _WINSOCKAPI_ */
    return YES;
}

#ifdef WWW_WIN_ASYNC

/*	HTEventList_get/setWinHandle
**	--------------------------
**	Managing the windows handle on Windows
*/
PUBLIC BOOL HTEventList_setWinHandle (HWND window, unsigned long message)
{
    HTSocketWin = window;
    HTwinMsg = message;
    return YES;
}

PUBLIC HWND HTEventList_getWinHandle (unsigned long * pMessage)
{
    if (pMessage)
        *pMessage = HTwinMsg;
    return (HTSocketWin);
}

/* only responsible for WM_TIMER and WSA_AsyncSelect */    	
PUBLIC LRESULT CALLBACK AsyncWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WORD event;
    SOCKET sock;
    HTEventType type;
    ms_t now = HTGetTimeInMillis();

    /* timeout stuff */
    if (uMsg == WM_TIMER) {
	HTTimer_dispatch((HTTimer *)wParam);
	return (0);
    }

    if (uMsg != HTwinMsg)	/* not our async message */
    	return (DefWindowProc(hwnd, uMsg, wParam, lParam));

    event = LOWORD(lParam);
    sock = (SOCKET)wParam;
    switch (event) {
    case FD_READ: type = HTEvent_READ; break;
    case FD_WRITE: type = HTEvent_WRITE; break;
    case FD_ACCEPT: type = HTEvent_ACCEPT; break;
    case FD_CONNECT: type = HTEvent_CONNECT; break;
    case FD_OOB: type = HTEvent_OOB; break;
    case FD_CLOSE: type = HTEvent_CLOSE; break;
    default: HTDebugBreak(__FILE__, __LINE__, "Unknown event %d\n", event);
    }
    if (HTEventList_dispatch((int)sock, type, now) != HT_OK)
	HTEndLoop = -1;
    return (0);
}

PUBLIC int HTEventList_newLoop (void)
{
    return HTEventList_loop (NULL);
}

PUBLIC int HTEventList_loop (HTRequest * theRequest )
{
    MSG msg;
    int status;
    while (GetMessage(&msg,0,0,0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
    }

    status = HTEndLoop;
    
    /* Reset HTEndLoop in case we want to start again */
    HTEndLoop = 0;
    
    return (status == 1 ? HT_OK : HT_ERROR);
}

#else /* WWW_WIN_ASYNC */

PUBLIC int HTEventList_newLoop (void)
{
    return HTEventList_loop (NULL);
}

/*
**  We wait for activity from one of our registered 
**  channels, and dispatch on that.
**
**  There are now two versions of the event loop. The first is if you want
**  to use async I/O on windows, and the other is if you want to use normal
**  Unix setup with sockets
*/
PUBLIC int HTEventList_loop (HTRequest * theRequest) 
{
    fd_set treadset, twriteset, texceptset;
    struct timeval waittime, * wt;
    int active_sockets;
    int maxfds;
    ms_t timeout;
    ms_t now;
    SOCKET s;
    int status = HT_OK;

    EventOrderList = HTList_new();	/* is kept around until EventOrder_deleteAll */

    /* Don't leave this loop until we leave the application */
    while (!HTEndLoop) {
        treadset = FdArray[HTEvent_INDEX(HTEvent_READ)];
        twriteset = FdArray[HTEvent_INDEX(HTEvent_WRITE)];
        texceptset = FdArray[HTEvent_INDEX(HTEvent_OOB)];

        /*
	**  Timeval struct copy needed for linux, as it set the value to the
	**  remaining timeout while exiting the select. (and perhaps for
	**  other OS). Code borrowed from X server.
	*/
	wt = NULL;
	if ((status = HTTimer_next(&timeout)))
	    return status;
	if (timeout != 0) {
	    waittime.tv_sec = timeout / MILLI_PER_SECOND;
	    waittime.tv_usec = (timeout % MILLI_PER_SECOND) *
		(1000000 / MILLI_PER_SECOND);
	    wt = &waittime;
	}

        maxfds = MaxSock; 
	if (THD_TRACE) HTTrace("Event Loop.. calling select: maxfds is %d\n", maxfds);

#ifdef EVENT_TRACE
#define HT_FS_BYTES(a)  ((((a)/16)+1) * 4)
	HTTraceData((char*)&treadset, HT_FS_BYTES(maxfds), "HTEventList_loop pre treadset: (maxfd:%d)", maxfds);
	HTTraceData((char*)&twriteset, HT_FS_BYTES(maxfds), "HTEventList_loop pre twriteset:");
	HTTraceData((char*)&texceptset, HT_FS_BYTES(maxfds), "HTEventList_loop pre texceptset:");
#endif /* EVENT_TRACE */

#ifdef __hpux 
        active_sockets = select(maxfds+1, (int *)&treadset, (int *)&twriteset,
				(int *)&texceptset, wt);
#else
        active_sockets = select(maxfds+1, &treadset, &twriteset, &texceptset, wt);
#endif

	now = HTGetTimeInMillis();

#ifdef EVENT_TRACE
	HTTraceData((char*)&treadset, HT_FS_BYTES(maxfds), "HTEventList_loop post treadset: (active_sockets:%d)", active_sockets);
	HTTraceData((char*)&twriteset, HT_FS_BYTES(maxfds), "HTEventList_loop post twriteset: (errno:%d)", errno);
	HTTraceData((char*)&texceptset, HT_FS_BYTES(maxfds), "HTEventList_loop post texceptset:");
#endif /* EVENT_TRACE */

	if (THD_TRACE) HTTrace("Event Loop.. select returns %d\n", active_sockets);

        if (active_sockets == -1) {
#ifdef EINTR
	    if (socerrno == EINTR) {
		/*
		** EINTR     The select() function was interrupted  before  any
		**           of  the  selected  events  occurred and before the
		**           timeout interval expired.
		**
		**           If SA_RESTART has been set  for  the  interrupting
		**           signal,  it  is  implementation-dependent  whether
		**	     select() restarts or returns with EINTR.
		*/
		if (THD_TRACE)
		    HTTrace("Event Loop.. select was interruted - try again\n");
		continue;
	    }
#endif /* EINTR */
	    if (THD_TRACE) HTTrace("Event Loop.. select returned error %d\n", socerrno);
	    EventList_dump();
	    return HT_ERROR;
        }

	/*
	**  We had a timeout so now we check and see if we have a timeout
	**  handler to call. Let HTTimer_next get it.
	*/ 
	if (active_sockets == 0)
	    continue;

	/*
	**  There were active sockets. Determine which fd sets they were in
	*/
#ifdef HT_EVENT_ORDER
#define DISPATCH(socket, type, now) EventOrder_add(socket, type, now)
#else /* HT_EVENT_ORDER */
#define DISPATCH(socket, type, now) HTEventList_dispatch(socket, type, now)
#endif /* !HT_EVENT_ORDER */
	for (s = 0 ; s <= maxfds ; s++) { 
	    if (FD_ISSET(s, &texceptset))
		if ((status = DISPATCH(s, HTEvent_OOB, now)) != HT_OK)
		    return status;
	    if (FD_ISSET(s, &twriteset))
		if ((status = DISPATCH(s, HTEvent_WRITE, now)) != HT_OK)
		    return status;
	    if (FD_ISSET(s, &treadset))
		if ((status = DISPATCH(s, HTEvent_READ, now)) != HT_OK)
		    return status;
	}
#ifdef HT_EVENT_ORDER
	if ((status = EventOrder_executeAndDelete()) != HT_OK)
	    return status;
#endif /* HT_EVENT_ORDER */
    };

    /* Reset HTEndLoop in case we want to start again */
    HTEndLoop = 0;

    return HT_OK;
}

#endif /* !WWW_WIN_ASYNC */

PUBLIC void CheckSockEvent(HTTimer * timer, HTTimerCallback * cbf, void * param)
{
    SockEvents * sockp = (SockEvents *)param;
    if (cbf == EventListTimerHandler && 
	sockp->timeouts[0] != timer && 
	sockp->timeouts[1] != timer && 
	sockp->timeouts[2] != timer) {
	HTDebugBreak(__FILE__, __LINE__, "Bad timer %p\n", timer);
    }
}

