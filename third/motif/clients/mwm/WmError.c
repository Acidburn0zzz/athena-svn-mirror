/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.4
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: WmError.c,v $ $Revision: 1.1.1.1 $ $Date: 1999-01-30 03:16:32 $"
#endif
#endif
/*
 * (c) Copyright 1987, 1988, 1989, 1990 HEWLETT-PACKARD COMPANY */

/*
 * Included Files:
 */

#include "WmGlobal.h"
#include <stdio.h>


/*
 * Function Declarations:
 */

#ifdef _NO_PROTO
void WmInitErrorHandler ();
int WmXErrorHandler ();
int WmXIOErrorHandler ();
void WmXtErrorHandler ();
void WmXtWarningHandler ();
void Warning ();
#else /* _NO_PROTO */
void WmInitErrorHandler (Display *display);
int WmXErrorHandler (Display *display, XErrorEvent *errorEvent);
int WmXIOErrorHandler (Display *display);
void WmXtErrorHandler (char *message);
void WmXtWarningHandler (char *message);
void Warning (char *message);
#endif /* _NO_PROTO */





/*************************************<->*************************************
 *
 *  WmInitErrorHandler (display)
 *
 *
 *  Description:
 *  -----------
 *  This function initializes the window manager error handler.
 *
 *
 *  Inputs:
 *  ------
 *  display = display we're talking about
 *  -------
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void
WmInitErrorHandler (display)
    Display *display;

#else /* _NO_PROTO */
void
WmInitErrorHandler (Display *display)
#endif /* _NO_PROTO */
{

    XSetErrorHandler (WmXErrorHandler);
    XSetIOErrorHandler (WmXIOErrorHandler);

    XtSetWarningHandler (WmXtWarningHandler);
    XtSetErrorHandler (WmXtErrorHandler);

} /* END OF FUNCTION WmInitErrorHandler */


/*************************************<->*************************************
 *
 *  WmXErrorHandler (display, errorEvent)
 *
 *
 *  Description:
 *  -----------
 *  This function is the X error handler that is registered with X to
 *  handle X errors resulting from window management activities.
 *
 *
 *  Inputs:
 *  ------
 *  display = display on which X error occurred
 *
 *  errorEvent = pointer to a block of information describing the error
 *
 * 
 *  Outputs:
 *  -------
 *  wmGD.errorFlag = set to True
 *
 *  Return = 0
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
int
WmXErrorHandler (display, errorEvent)
    Display *display;
    XErrorEvent *errorEvent;

#else /* _NO_PROTO */
int
WmXErrorHandler (Display *display, XErrorEvent *errorEvent)
#endif /* _NO_PROTO */
{
    ClientData *pCD;


    /*
     * Check for a BadWindow error for a managed window.  If this error
     * is detected indicate in the client data that the window no longer
     * exists.
     */

    if ((errorEvent->error_code == BadWindow) &&
	!XFindContext (DISPLAY, errorEvent->resourceid, wmGD.windowContextType,
	     (caddr_t *)&pCD))
    {
	if (errorEvent->resourceid == pCD->client)
	{
	    pCD->clientFlags |= CLIENT_DESTROYED;
	}
    }

    wmGD.errorFlag = True;

    return (0);

} /* END OF FUNCTION WmXErrorHandler */



/*************************************<->*************************************
 *
 *  WmXIOErrorHandler (display)
 *
 *
 *  Description:
 *  -----------
 *  This function is the X IO error handler that is registered with X to
 *  handle X IO errors.  This function exits the window manager.
 *
 *
 *  Inputs:
 *  ------
 *  display = X display on which the X IO error occurred
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
int
WmXIOErrorHandler (display)
    Display *display;

#else /* _NO_PROTO */
int
WmXIOErrorHandler (Display *display)
#endif /* _NO_PROTO */
{
  char  err[100];
 
  sprintf (err, "%s: %s\n", "I/O error on display:", XDisplayString(display));
  Warning(err);

  exit (WM_ERROR_EXIT_VALUE);

} /* END OF FUNCTIONS WmXIOErrorHandler */



/*************************************<->*************************************
 *
 *  WmXtErrorHandler (message)
 *
 *
 *  Description:
 *  -----------
 *  This function is registered as the X Toolkit error handler.
 *
 *
 *  Inputs:
 *  ------
 *  message = pointer to an error message
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void
WmXtErrorHandler (message)
    char * message;

#else /* _NO_PROTO */
void
WmXtErrorHandler (char *message)
#endif /* _NO_PROTO */
{
    Warning(message);
    exit (WM_ERROR_EXIT_VALUE);

} /* END OF FUNCTION WmXtErrorHandler */



/*************************************<->*************************************
 *
 *  WmXtWarningHandler (message)
 *
 *
 *  Description:
 *  -----------
 *  This function is registered as an X Toolkit warning handler.
 *
 *
 *  Inputs:
 *  ------
 *  message = pointer to a warning message
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void
WmXtWarningHandler (message)
    char * message;

#else /* _NO_PROTO */
void
WmXtWarningHandler (char *message)
#endif /* _NO_PROTO */
{


} /* END OF FUNCTIONS WmXtWarningHandler */


/*************************************<->*************************************
 *
 *  Warning (message)
 *
 *
 *  Description:
 *  -----------
 *  This function lists a message to stderr.
 *
 *
 *  Inputs:
 *  ------
 *  message = pointer to a message string
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void
Warning (message)
    char * message;

#else /* _NO_PROTO */
void
Warning (char *message)
#endif /* _NO_PROTO */
{
    fprintf (stderr, "%s: %s\n", wmGD.mwmName, message);
    fflush (stderr);

} /* END OF FUNCTION Warning */

