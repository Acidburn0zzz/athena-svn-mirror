/*								     HTTrace.c
**	TRACE AND DATA LOGGING UTILITIES
**
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
**	@(#) $Id: HTTrace.c,v 1.1.1.1 2000-03-10 17:53:02 ghudson Exp $
**
**      26 Nov 96 (EGP)	moved HTTrace stuff here from HTString.c and
**			added HTTrace_data
*/

/* Library include files */
#include "wwwsys.h"
/*
**	All the tracing facilities are prototyped in
**	HTUtils.h because they are needed everywhere.
*/
#include "HTUtils.h"

#if WWWTRACE_MODE == WWWTRACE_FILE
PUBLIC FILE *WWWTrace = NULL;
#endif

#ifndef WWW_WIN_DLL
PUBLIC unsigned int WWW_TraceFlag = 0;		/* Global trace flag for ALL W3 code */
#endif

#if TRACECALLBACK
PRIVATE HTTraceCallback * PHTTraceCallback = NULL;

PUBLIC void HTTrace_setCallback (HTTraceCallback * pCall)
{
    PHTTraceCallback = pCall;
}

PUBLIC HTTraceCallback * HTTrace_getCallback (void)
{
    return PHTTraceCallback;
}
#endif

PUBLIC int HTTrace (const char * fmt, ...)
{
    va_list pArgs;
    va_start(pArgs, fmt);
#if TRACECALLBACK
    if (PHTTraceCallback)
	return (*PHTTraceCallback)(fmt, pArgs);
#endif
#ifdef WWW_WIN_WINDOW
    return (0);
#else
    return (vfprintf(stderr, fmt, pArgs));
#endif
}

PRIVATE HTTraceDataCallback * PHTTraceDataCallback = NULL;

PUBLIC void HTTraceData_setCallback (HTTraceDataCallback * pCall)
{
    PHTTraceDataCallback = pCall;
}

PUBLIC HTTraceDataCallback * HTTraceData_getCallback (void)
{
    return PHTTraceDataCallback;
}

PUBLIC int HTTraceData (char * data, size_t len, char * fmt, ...)
{
    va_list pArgs;
    va_start(pArgs, fmt);
    if (PHTTraceDataCallback)
	return (*PHTTraceDataCallback)(data, len, fmt, pArgs);
    return (0);
}

PUBLIC void HTDebugBreak (char * file, unsigned long line, const char * fmt, ...)
{
    va_list pArgs;
    va_start(pArgs, fmt);
    HTTrace("%s:%ld ", file ? file : "", line);
    HTTrace(fmt, pArgs);
#ifdef WWW_MSWINDOWS
    DebugBreak();
#else /* WWW_MSWINDOWS */
    abort();
#endif /* !WWW_MSWINDOWS */
    return;
}

