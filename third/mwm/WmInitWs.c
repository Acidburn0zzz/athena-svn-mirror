/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: WmInitWs.c,v $ $Revision: 1.1.1.1 $ $Date: 1997-03-25 09:12:21 $"
#endif
#endif
/*
 * (c) Copyright 1987, 1988, 1989, 1990 HEWLETT-PACKARD COMPANY */

/*
 * Included Files:
 */

#include "WmGlobal.h"
#include "WmResNames.h"
#include "WmICCC.h"
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>     /* just for XmRegisterConverters decl */
#include <X11/Shell.h>
#include <X11/Core.h>

/*
 * include extern functions
 */
#include "WmCDInfo.h"
#include "WmColormap.h"
#include "WmError.h"
#include "WmEvent.h"
#include "WmFeedback.h"
#include "WmFunction.h"
#include "WmIDecor.h"
#include "WmIPlace.h"
#include "WmIconBox.h"
#include "WmKeyFocus.h"
#include "WmManage.h"
#include "WmMenu.h"
#include "WmProperty.h"
#include "WmResCvt.h"
#include "WmResource.h"
#include "WmSignal.h"
#include "WmProtocol.h"
#include "WmCDecor.h"
#include "stdio.h"
#include "WmResParse.h"

/*
 * Function Declarations:
 */

#ifdef _NO_PROTO
void CopyArgv ();
void InitWmDisplayEnv ();
void InitWmGlobal ();
void InitWmScreen ();
void InitScreenNames ();
void InitWmWorkspace ();
void ProcessMotifWmInfo ();
void SetupWmWorkspaceWindows ();
void MakeWmFunctionResources ();
void MakeWorkspaceCursors ();
void MakeXorGC ();
#else /* _NO_PROTO */
void InitWmDisplayEnv (void);
void InitWmGlobal (int argc, char *argv [], char *environ []);
void InitWmScreen (WmScreenData *pSD, int sNum);
void InitWmWorkspace (WmWorkspaceData *pWS, WmScreenData *pSD);
void ProcessMotifWmInfo (Window rootWindowOfScreen);
void SetupWmWorkspaceWindows (void);
void MakeWorkspaceCursors (void);
void MakeWmFunctionResources (WmScreenData *pSD);
void MakeXorGC (WmScreenData *pSD);
void CopyArgv (int argc, char *argv []);
void InitScreenNames (void);
#endif /* _NO_PROTO */

/*
 * Global Variables:
 */
extern int firstTime;


/******************************<->*************************************
 *
 *  InitWmGlobal (argc, argv, environ)
 *
 *
 *  Description:
 *  -----------
 *  This function initializes the workspace manager.
 *
 *
 *  Inputs:
 *  ------
 *  argc = number of command line arguments (+1)
 *
 *  argv = window manager command line arguments
 *
 *  environ = window manager environment
 *
 * 
 *  Outputs:
 *  -------
 *  wmGD = (initialize the global data structure)
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void InitWmGlobal (argc, argv, environ)
    int argc;
    char *argv[];
    char *environ[];

#else /* _NO_PROTO */
void InitWmGlobal (int argc, char *argv [], char *environ [])
#endif /* _NO_PROTO */
{
    XSetWindowAttributes sAttributes;
    int scr;
    int managed = 0;
    char pch[80];
    Boolean activeSet = False;
    Boolean processedGlobalResources = False;
    WmScreenData *pSD;
    Arg args[11];
    int argnum;
    char *res_class;
    wmGD.errorFlag = False;

#ifdef _NO_PROTO
    SetupWmSignalHandlers ();
#else
    SetupWmSignalHandlers (0); /* dummy paramater */
#endif


    /*
     * Do (pre-toolkit) initialization:
     */

    wmGD.windowContextType = XUniqueContext ();
    wmGD.screenContextType = XUniqueContext ();

    /* copy argv (the XtInititalize changes the original) for use in restart */
    CopyArgv (argc, argv);

    wmGD.environ = environ;




    /* set our name */
    if ((wmGD.mwmName = (char*)strrchr (wmGD.argv[0], '/')) != NULL)
    {
        wmGD.mwmName++;
    }
    else
    {
        wmGD.mwmName = wmGD.argv[0];
    }
    res_class = WM_RESOURCE_CLASS;

    wmGD.display = NULL;


    /*
     * Do X Tookit initialization:
     *
     */   

    XtToolkitInitialize();

    XmRegisterConverters ();

    wmGD.mwmAppContext = XtCreateApplicationContext();
    wmGD.display = XtOpenDisplay (wmGD.mwmAppContext,
				  NULL,
				  wmGD.mwmName,
				  res_class,
				  NULL,
				  0,
				  &argc, /* R5 changed from Cardinal to int*/
				  argv);
    
    if (!wmGD.display)
    {
	Warning("Could not open display.");
	exit (WM_ERROR_EXIT_VALUE);
    }

    /*
     * Setup error handling:
     */

    WmInitErrorHandler(wmGD.display);

    /*
     * Initialize cursor size info and 
     * display the startup cursor.
     */
    
    InitCursorInfo ();
    ShowWaitState (TRUE);

    /*
     * Set up the _MOTIF_BINDINGS property on the root window
     * of screen 0.  Must do this before we create shells.
     */
    
    ProcessMotifBindings ();
    
    argnum = 0;
    XtSetArg (args[argnum], XtNgeometry, NULL);	argnum++;
    XtSetArg (args[argnum], XtNx, 10000);	argnum++;
    XtSetArg (args[argnum], XtNy, 0);		argnum++;
    XtSetArg (args[argnum], XtNwidth, 10);	argnum++;
    XtSetArg (args[argnum], XtNheight, 10);	argnum++;

    /* create topmost shell (application shell) */
    wmGD.topLevelW = XtAppCreateShell (NULL, 
			      res_class,
			      applicationShellWidgetClass,
			      DISPLAY,
			      args,
			      argnum);
    
    /* allocate namespace for screens */
    InitScreenNames();
    
    /* 
     * Determine the screen management policy (all or none)
     * Process command line arguments that we handle 
     * This could change the number of screens we manage 
     */
    ProcessGlobalScreenResources ();
    ProcessCommandLine (argc, argv);


    /*
     * Allocate data and initialize for screens we manage:
     */

    if (!(wmGD.Screens = (WmScreenData *) 
	    XtMalloc (wmGD.numScreens * sizeof(WmScreenData))))
    {
	ShowWaitState (FALSE);
	Warning ("Insufficient memory for Screen data");
	exit (WM_ERROR_EXIT_VALUE);
    }
    else 
    {

	sAttributes.event_mask = SubstructureRedirectMask;

	for (scr=0; scr<wmGD.numScreens; scr++) 
	{
	    int sNum;
	    
	    /* 
	     * Gain control of the root windows of each screen:
	     */

	    sNum = (wmGD.numScreens == 1) ? DefaultScreen(DISPLAY) : scr;
	    wmGD.errorFlag = False;

	    XChangeWindowAttributes (DISPLAY, RootWindow (DISPLAY, sNum), 
		CWEventMask, &sAttributes);
	    /*
	     * Do XSync to force server action and catch errors
	     * immediately.
	     */
	    XSync (DISPLAY, False /* do not discard events */);

	    if (wmGD.errorFlag)
	    {
		sprintf(pch, 
		    "Another window manager is running on screen %d", sNum);
		Warning ((char *) &pch[0]);
		wmGD.Screens[scr].managed = False;
	    }
	    else 
	    {
		if (!processedGlobalResources)
		{

#ifndef NO_SHAPE
		    wmGD.hasShape = XShapeQueryExtension (DISPLAY,
							  &wmGD.shapeEventBase,
							  &wmGD.shapeErrorBase);
#endif /*  NO_SHAPE  */

		    wmGD.menuActive = NULL;
		    wmGD.menuUnpostKeySpec = NULL;
		    wmGD.F_NextKeySpec = NULL;
		    wmGD.F_PrevKeySpec = NULL;
		    wmGD.passKeysActive = False;
		    wmGD.passKeysKeySpec = NULL;
		    wmGD.checkHotspot = False;
		    wmGD.configAction = NO_ACTION;
		    wmGD.configPart = FRAME_NONE;
		    wmGD.configSet = False;
		    wmGD.preMove = False;
		    wmGD.gadgetClient = NULL;
		    wmGD.wmTimers = NULL;
		    wmGD.clientDefaultTitle = 
			XmStringCreateLtoR(DEFAULT_CLIENT_TITLE,
					   XmFONTLIST_DEFAULT_TAG);
		    wmGD.iconDefaultTitle = 
			XmStringCreateLtoR(DEFAULT_ICON_TITLE,
					   XmFONTLIST_DEFAULT_TAG);
		    wmGD.attributesWindow = (Window)NULL;
		    wmGD.clickData.pCD = NULL;
		    wmGD.clickData.clickPending = False;
		    wmGD.clickData.doubleClickPending = False;
		    wmGD.systemModalActive = False;
		    wmGD.activeIconTextDisplayed = False;
		    wmGD.movingIcon = False;
		    wmGD.queryScreen = True;
		    wmGD.dataType = GLOBAL_DATA_TYPE;
		    

		    /* 
		     * if this is the first screen we can manage, 
		     * process global.
		     */
		    
		    processedGlobalResources = True;

		    /*
		     * Get the _MOTIF_WM_INFO property and determine 
		     * the startup / restart state.
		     */
		    
		    ProcessMotifWmInfo (RootWindow (DISPLAY, sNum));
		    
		    /*
		     * Process global window manager resources:
		     */
		    
		    AddWmResourceConverters ();
		    ProcessWmResources ();

		}
		
		InitWmScreen (&(wmGD.Screens[scr]), sNum);
		wmGD.Screens[scr].managed = True;
		managed++;

		if (!activeSet) 
		{
		    activeSet = True;
		    ACTIVE_PSD = &wmGD.Screens[scr];
		}
	    }
	}

	if (managed == 0) 
	{
	    /*
	     * No screens for me to manage, give up.
	     */
	    ShowWaitState (FALSE);
	    Warning ("Unable to manage any screens on display.");
	    exit (WM_ERROR_EXIT_VALUE);
	}
    }
    

    /*
     * Prepare to have child processes (e.g., exec'ed commands).
     * The X connection should not be passed on to child processes
     * (it should be automatically closed when a fork is done).
     */

    if (fcntl (ConnectionNumber (DISPLAY), F_SETFD, 1) == -1)
    {
	ShowWaitState (FALSE);
	Warning ("Cannot configure X connection");
	exit (WM_ERROR_EXIT_VALUE);
    }


    /*
     * Make the window manager workspace window.
     * Setup the _MOTIF_WM_INFO property on the root window.
     */

    SetupWmWorkspaceWindows ();


    /* make the cursors that the window manager uses */
    MakeWorkspaceCursors ();


    /* Sync the table used by Mwm's modifier parser to actual modMasks used */
    SyncModifierStrings();

    /*
     * Setup screen data and resources (after processing Wm resources.
     */
    for (scr = 0; scr < wmGD.numScreens; scr++)
    {
	pSD = &(wmGD.Screens[scr]);

	if (pSD->managed)
	{
	    /*
	     * Initialize workspace colormap data.
	     */

	    InitWorkspaceColormap (pSD);

	    /*
	     * Process the window manager resource description file (.mwmrc):
	     */

	    ProcessWmFile (pSD);


	    /*
	     * Setup default resources for the system menu and key bindings:
	     */

	    SetupDefaultResources (pSD);


	    /*
	     * Make global window manager facilities:
	     */

	    if(pSD->iconDecoration & ICON_ACTIVE_LABEL_PART)
	    {
		/* create active icon window */
		CreateActiveIconTextWindow(pSD); 
	    }


	    /*
	     * Make menus and other resources that are used by window manager
	     * functions that are activated by menus, buttons and keys.
	     */

	    MakeWmFunctionResources (pSD);
	}


    }

    /*
     * Realize the top level widget, make the window override
     * redirect so we don't manage it, and then move it out of the way
     */

    XtRealizeWidget (wmGD.topLevelW);

    sAttributes.override_redirect = True;
    XChangeWindowAttributes (DISPLAY, XtWindow (wmGD.topLevelW),
		CWOverrideRedirect, &sAttributes);


    /* setup window manager inter-client communications conventions handling */
    SetupWmICCC ();


    /*
     * Initialize window manager event handling:
     */

    InitEventHandling ();



    /*
     * Initialize frame component graphics
     */
    {
	for (scr = 0; scr < wmGD.numScreens; scr++)
	{
	    pSD = &(wmGD.Screens[scr]);

	    if (pSD->managed)
	    {
		InitClientDecoration (pSD);

		/*
		 * Make an icon box if specificed:
		 */
		if (pSD->useIconBox)
		{
		    InitIconBox (pSD);
		}

		/*
		 * Adopt client windows that exist before wm startup:
		 */

		AdoptInitialClients (pSD);

		/*
		 * Setup initial keyboard focus and colormap focus:
		 */

		InitColormapFocus (pSD);

	    }
	}

        for (scr = 0; scr < wmGD.numScreens; scr++)
        {
            pSD = &(wmGD.Screens[scr]);
	    
            if (pSD->managed)
            {
                ACTIVE_PSD = &wmGD.Screens[scr];
                MapIconBoxes (pSD->pActiveWS);
            }
        }
        firstTime = 0;
    }
    
    InitKeyboardFocus ();

    InitWmDisplayEnv ();
    ShowWaitState (FALSE);

#ifdef AUTOMATION
	AutoInitByteOrderChar();
#endif


} /* END OF FUNCTION InitWmGlobal */



/******************************<->*************************************
 *
 *  InitWmScreen
 *
 *
 *  Description:
 *  -----------
 *  This function initializes a screen data block.
 *
 *  Inputs:
 *  -------
 *  pSD = pointer to preallocated screen data block
 *  sNum = screen number for this screen
 *
 *  Outputs:
 *  -------
 *************************************<->***********************************/

#ifdef _NO_PROTO
void
InitWmScreen (pSD, sNum)
    WmScreenData *pSD;
    int sNum;
#else /* _NO_PROTO */
void
InitWmScreen (WmScreenData *pSD, int sNum)
#endif /* _NO_PROTO */
{
    Arg args[12];
    int argnum;


    char *pDisplayName;
    char buffer[256];
    char displayName[256];
    char *token1, *token2;


   /*
    * Set screen data values
    */

    pSD->rootWindow = RootWindow (DISPLAY, sNum);
    pSD->clientCounter = 0;
    pSD->defaultSystemMenuUseBuiltin = TRUE;
    pSD->displayString = NULL;
    pSD->acceleratorMenuCount = 0;
    pSD->activeIconTextWin = (Window)NULL;
    pSD->focusPriority = 0;
    pSD->inputScreenWindow = (Window)NULL;
    pSD->colormapFocus = NULL;
    pSD->keySpecs = NULL;
    pSD->screen = sNum;
    pSD->confirmboxW[DEFAULT_BEHAVIOR_ACTION] = NULL;
    pSD->confirmboxW[CUSTOM_BEHAVIOR_ACTION] = NULL;
    pSD->confirmboxW[RESTART_ACTION] = NULL;
    pSD->confirmboxW[QUIT_MWM_ACTION] = NULL;
    pSD->feedbackWin = (Window)NULL;
    pSD->fbStyle = FB_OFF;
    pSD->fbWinWidth = 0;
    pSD->fbWinHeight = 0;
    pSD->fbLocation[0] = '\0';
    pSD->fbSize[0] = '\0';
    pSD->fbLocX = 0;
    pSD->fbLocY = 0;
    pSD->fbSizeX = 0;
    pSD->fbSizeY = 0;
    pSD->fbLastX = -1;
    pSD->fbLastY = -1;
    pSD->fbLastWidth = -1;
    pSD->fbLastHeight = -1;
    pSD->fbTop = NULL;
    pSD->fbBottom = NULL;
    pSD->actionNbr = -1;
    pSD->clientList = NULL;
    pSD->lastClient = NULL;
    pSD->lastInstalledColormap = (Colormap)NULL;
    pSD->shrinkWrapGC = NULL;
    pSD->bitmapCache = NULL;
    pSD->bitmapCacheSize = 0;
    pSD->bitmapCacheCount = 0;
    pSD->dataType = SCREEN_DATA_TYPE;
    /*
     * Save screen context
     */
    XSaveContext (DISPLAY, pSD->rootWindow, wmGD.screenContextType,
	(caddr_t)pSD);
    /*
     * Create shell widget for screen resource hierarchy
     */

    argnum = 0;
    XtSetArg (args[argnum], XtNgeometry, NULL);	argnum++;
    XtSetArg (args[argnum], XtNx, 10000);	argnum++;
    XtSetArg (args[argnum], XtNy, 10000);	argnum++;
    XtSetArg (args[argnum], XtNwidth, 10);	argnum++;
    XtSetArg (args[argnum], XtNheight, 10);	argnum++;
    XtSetArg (args[argnum], XtNoverrideRedirect, True);	argnum++;

    XtSetArg (args[argnum], XtNdepth, 
	    DefaultDepth(DISPLAY, sNum));	argnum++;
    XtSetArg (args[argnum], XtNscreen, 
	    ScreenOfDisplay(DISPLAY, sNum)); 	argnum++;
    XtSetArg (args[argnum], XtNcolormap, 
	    DefaultColormap(DISPLAY, sNum)); 	argnum++;

    pSD->screenTopLevelW = XtCreatePopupShell ((String) wmGD.screenNames[sNum],
					       vendorShellWidgetClass,
					       wmGD.topLevelW,
					       args,
					       argnum);
    /*
     * Fetch screen based resources
     */
    ProcessScreenResources (pSD, wmGD.screenNames[sNum]);

    /*
     * Initialize other screen resources and parameters
     */
    MakeXorGC (pSD);
    InitIconSize(pSD);

    /*
     *  Allocate and initialize a workspace structure
     */
    
    if (!(pSD->pWS = (WmWorkspaceData *) XtMalloc (sizeof(WmWorkspaceData))))
    {
	ShowWaitState (FALSE);
	Warning ("Insufficient memory for Workspace data");
	exit (WM_ERROR_EXIT_VALUE);
    }

    /*
     * Set up workspace for this screen
     */
    InitWmWorkspace (pSD->pWS, pSD);
    pSD->pActiveWS = pSD->pWS;


    pDisplayName = DisplayString (DISPLAY);

    /*
     * Construct displayString for this string.  
     */

    strcpy(displayName, pDisplayName);

    token1 = (char*)strtok(displayName, ":");		/* parse of hostname */

    if((token2 = (char*)strtok(NULL, ".")) ||		/* parse off dpy & scr # */
       (token2 = (char*)strtok(NULL, "")) ||
       (displayName[0] == ':'))
    {
	if (displayName[0] == ':')		/* local dpy (special case) */
	{
	    if ((token2 = (char*)strtok(token1, ".")) != NULL)	/* parse dpy# */
		sprintf(buffer, "DISPLAY=:%s.%d",
			token2,	sNum);
	} else {				/* otherwise process normally */
	    sprintf(buffer, "DISPLAY=%s:%s.%d",
		    token1, token2, sNum);
	}

	/*		
	 * Allocate space for the display string
	 */
    
	if ((pSD->displayString =
	     (String)XtMalloc ((unsigned int) (strlen(buffer) + 1))) == NULL)
	{
	    Warning ("Insufficient memory for displayString");
	}
	else
	{
	    strcpy(pSD->displayString, buffer);
	}

    }



} /* END OF FUNCTION  InitWmScreen */


/*************************************<->*************************************
 *
 *  InitWmWorkspace
 *
 *
 *  Description:
 *  -----------
 *  This function initializes a workspace data block.
 *
 *  Inputs:
 *  -------
 *  pWS = pointer to preallocated workspace data block
 *  pSD = ptr to parent screen data block
 *
 *  Outputs:
 *  -------
 *************************************<->***********************************/

#ifdef _NO_PROTO
void InitWmWorkspace (pWS, pSD)
    WmWorkspaceData *pWS;
    WmScreenData *pSD;

#else /* _NO_PROTO */
void InitWmWorkspace (WmWorkspaceData *pWS, WmScreenData *pSD)
#endif /* _NO_PROTO */
{
    Arg args[10];
    int argnum;

#define DEFAULT_WS_NAME "workspace"

    pWS->pSD = pSD;
    pWS->pIconBox = NULL;
    pWS->dataType = WORKSPACE_DATA_TYPE;

    if ((pWS->name = (char *) 
	    XtMalloc ((1+strlen(DEFAULT_WS_NAME)) * sizeof (char))) == NULL)
    {
	ShowWaitState (FALSE);
	exit (WM_ERROR_EXIT_VALUE);
    }
    strcpy (pWS->name, DEFAULT_WS_NAME);

    /*
     * Create widget for workspace resource hierarchy
     */
    argnum = 0;
    XtSetArg (args[argnum], XtNdepth, 
	    DefaultDepth(DISPLAY, pSD->screen));	argnum++;
    XtSetArg (args[argnum], XtNscreen, 
	    ScreenOfDisplay(DISPLAY, pSD->screen)); 	argnum++;
    XtSetArg (args[argnum], XtNcolormap, 
	    DefaultColormap(DISPLAY, pSD->screen)); 	argnum++;
    XtSetArg (args[argnum], XtNwidth,  5);		argnum++;
    XtSetArg (args[argnum], XtNheight,  5);		argnum++;

    pWS->workspaceTopLevelW = XtCreateWidget (	pWS->name,
						xmPrimitiveWidgetClass,
    						pSD->screenTopLevelW,
					   	args,
						argnum);


    /*
     * Process workspace based resources
     */
    ProcessWorkspaceResources (pWS);	

    /* setup icon placement */
    if (wmGD.iconAutoPlace)
    {
	InitIconPlacement (pWS); 
    }

} /* END OF FUNCTION  InitWmWorkspace */



/*************************************<->*************************************
 *
 *  ProcessMotifWmInfo (rootWindowOfScreen)
 *
 *
 *  Description:
 *  -----------
 *  This function is used retrieve and save the information in the 
 *  _MOTIF_WM_INFO property.  If the property does not exist then
 *  the start / restart state is set to initial startup with the
 *  user specified (not standard) configuration.
 *
 *
 *  Outputs:
 *  -------
 *  wmGD.useStandardBehavior = True if set indicated in property
 *
 *  wmGD.wmRestarted = True if the window manager was restarted
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void ProcessMotifWmInfo (rootWindowOfScreen)
    Window rootWindowOfScreen;
#else /* _NO_PROTO */
void ProcessMotifWmInfo (Window rootWindowOfScreen)
#endif /* _NO_PROTO */
{
    MwmInfo *pMwmInfo;

    wmGD.xa_MWM_INFO = XInternAtom (DISPLAY, _XA_MWM_INFO, False);
    if ((pMwmInfo = (MotifWmInfo *)GetMwmInfo (rootWindowOfScreen)) != NULL)
    {
	wmGD.useStandardBehavior =
		(pMwmInfo->flags & MWM_INFO_STARTUP_STANDARD) ? True : False;
	wmGD.wmRestarted = True;
	XFree ((char *)pMwmInfo);
    }
    else
    {
	wmGD.useStandardBehavior = False;
	wmGD.wmRestarted = False;
    }

} /* END OF FUNCTION ProcessMotifWmInfo */



/*************************************<->*************************************
 *
 *  SetupWmWorkspaceWindows ()
 *
 *
 *  Description:
 *  -----------
 *  This function is used to setup a window that can be used in doing window
 *  management functions.  This window is not visible on the screen.
 *
 *
 *  Outputs:
 *  -------
 *  pSD->wmWorkspaceWin = window that is used to hold wm properties
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void SetupWmWorkspaceWindows ()

#else /* _NO_PROTO */
void SetupWmWorkspaceWindows (void)
#endif /* _NO_PROTO */
{
    int scr;
    WmScreenData *pSD;
    XSetWindowAttributes sAttributes;

    for (scr = 0; scr < wmGD.numScreens; scr++)
    {
	pSD = &(wmGD.Screens[scr]);
	if (pSD->managed)
	{
	    sAttributes.override_redirect = True;
	    sAttributes.event_mask = FocusChangeMask;
	    pSD->wmWorkspaceWin = XCreateWindow (DISPLAY, pSD->rootWindow, 
				      -100, -100, 10, 10, 0, 0, 
				      InputOnly, CopyFromParent,
				      (CWOverrideRedirect |CWEventMask),
				      &sAttributes);

	    XMapWindow (DISPLAY, pSD->wmWorkspaceWin);

	    SetMwmInfo (pSD->rootWindow, 
			(long) ((wmGD.useStandardBehavior) ?
                        MWM_INFO_STARTUP_STANDARD : MWM_INFO_STARTUP_CUSTOM), 
			pSD->wmWorkspaceWin);
	}
    }

} /* END OF FUNCTION SetupWmWorkspaceWindow */



/*************************************<->*************************************
 *
 *  MakeWorkspaceCursors ()
 *
 *
 *  Description:
 *  -----------
 *  This function makes the cursors that the window manager uses.
 *
 *
 *  Inputs:
 *  ------
 *  XXinput = ...
 *
 *  XXinput = ...
 *
 * 
 *  Outputs:
 *  -------
 *  wmGD = (stretchCursors ...)
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeWorkspaceCursors ()

#else /* _NO_PROTO */
void MakeWorkspaceCursors (void)
#endif /* _NO_PROTO */
{
    wmGD.workspaceCursor = XCreateFontCursor (DISPLAY, XC_left_ptr);

    wmGD.stretchCursors[STRETCH_NORTH_WEST] =
	XCreateFontCursor (DISPLAY, XC_top_left_corner);
    wmGD.stretchCursors[STRETCH_NORTH] =
	XCreateFontCursor (DISPLAY, XC_top_side);
    wmGD.stretchCursors[STRETCH_NORTH_EAST] =
	XCreateFontCursor (DISPLAY, XC_top_right_corner);
    wmGD.stretchCursors[STRETCH_EAST] =
	XCreateFontCursor (DISPLAY, XC_right_side);
    wmGD.stretchCursors[STRETCH_SOUTH_EAST] =
	XCreateFontCursor (DISPLAY, XC_bottom_right_corner);
    wmGD.stretchCursors[STRETCH_SOUTH] =
	XCreateFontCursor (DISPLAY, XC_bottom_side);
    wmGD.stretchCursors[STRETCH_SOUTH_WEST] =
	XCreateFontCursor (DISPLAY, XC_bottom_left_corner);
    wmGD.stretchCursors[STRETCH_WEST] =
	XCreateFontCursor (DISPLAY, XC_left_side);

    wmGD.configCursor = XCreateFontCursor (DISPLAY, XC_fleur);

    wmGD.movePlacementCursor = XCreateFontCursor (DISPLAY, XC_ul_angle);
    wmGD.sizePlacementCursor = XCreateFontCursor (DISPLAY, XC_lr_angle);


} /* END OF FUNCTION MakeWorkspaceCursors */



/*************************************<->*************************************
 *
 *  MakeWmFunctionResources (pSD)
 *
 *
 *  Description:
 *  -----------
 *  This function makes menus and other resources that are used by window
 *  manager functions.
 *
 *
 *  Inputs:
 *  ------
 *  wmGD  = (menuSpecs, keySpecs, buttonSpecs)
 *
 * 
 *  Outputs:
 *  -------
 *  wmGD (menuSpecs) = new menu panes, protocol atoms
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeWmFunctionResources (pSD)

    WmScreenData *pSD;

#else /* _NO_PROTO */
void MakeWmFunctionResources (WmScreenData *pSD)
#endif /* _NO_PROTO */
{
    ButtonSpec *buttonSpec;
    KeySpec    *keySpec;
    MenuSpec   *menuSpec;
    Context     menuContext;


    /*
     * Scan through the menu specifications and make wm protocol atoms.
     */


    /*
     * Scan through the button binding specifications making menus if the
     * f.menu function is invoked.
     */

    buttonSpec = pSD->buttonSpecs;
    while (buttonSpec)
    {
	if (buttonSpec->wmFunction == F_Menu)
	{
	    if (buttonSpec->context & F_CONTEXT_WINDOW)
	    {
		menuContext = F_CONTEXT_WINDOW;
	    }
	    else if (buttonSpec->context & F_CONTEXT_ICON)
	    {
		menuContext = F_CONTEXT_ICON;
	    }
	    else
	    {
		menuContext = F_CONTEXT_ROOT;
	    }

	    menuSpec = MakeMenu (pSD, buttonSpec->wmFuncArgs, menuContext,
	                         buttonSpec->context, 
				 (MenuItem *) NULL, FALSE);

	    if (menuSpec)
	    /* 
	     * If successful, save in pSD->acceleratorMenuSpecs 
	     * Note: these accelerators have nonzero contexts.
	     */
	    {
		SaveMenuAccelerators (pSD, menuSpec);
	    }
	    else
	    {
		buttonSpec->wmFunction = F_Nop;
	    }
	}
	buttonSpec = buttonSpec->nextButtonSpec;
    }


    /*
     * Scan through the key binding specifications making menus if the
     * f.menu function is invoked.
     */

    keySpec = pSD->keySpecs;
    while (keySpec)
    {
	if (keySpec->wmFunction == F_Menu)
	{
	    if (keySpec->context & F_CONTEXT_WINDOW)
	    {
		menuContext = F_CONTEXT_WINDOW;
	    }
	    else if (keySpec->context & F_CONTEXT_ICON)
	    {
		menuContext = F_CONTEXT_ICON;
	    }
	    else
	    {
		menuContext = F_CONTEXT_ROOT;
	    }

	    menuSpec = MakeMenu (pSD, keySpec->wmFuncArgs, menuContext,
	                         keySpec->context, 
				 (MenuItem *) NULL, FALSE);

	    if (menuSpec)
	    /* 
	     * If successful, save in pSD->acceleratorMenuSpecs 
	     * Note: these accelerators have nonzero contexts.
	     */
	    {
		SaveMenuAccelerators (pSD, menuSpec);
	    }
	    else
	    {
		keySpec->wmFunction = F_Nop;
	    }
	}
	keySpec = keySpec->nextKeySpec;
    }


} /* END OF FUNCTION MakeWmFunctionResources */



/*************************************<->*************************************
 *
 *  MakeXorGC (pSD)
 *
 *
 *  Description:
 *  -----------
 *  Make an XOR graphic context for resizing and moving
 *
 *
 *  Inputs:
 *  ------
 *  pSD = pointer to screen data
 * 
 *  Outputs:
 *  -------
 *  Modifies global data
 *
 *  Comments:
 *  --------
 *  
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeXorGC (pSD)

    WmScreenData *pSD;
#else /* _NO_PROTO */
void MakeXorGC (WmScreenData *pSD)
#endif /* _NO_PROTO */
{
    XGCValues gcv;
    XtGCMask  mask;

    mask = GCFunction | GCLineWidth | GCSubwindowMode | GCCapStyle;
    gcv.function = GXinvert;
    gcv.line_width = 0;
    gcv.cap_style = CapNotLast;
    gcv.subwindow_mode = IncludeInferiors;

    /* Fix so that the rubberbanding for resize and move will
     *  have more contrasting colors.
     */

    gcv.plane_mask = BlackPixelOfScreen( DefaultScreenOfDisplay( DISPLAY )) ^ 
                     WhitePixelOfScreen( DefaultScreenOfDisplay( DISPLAY ));
    mask = mask | GCPlaneMask;

    pSD->xorGC = XCreateGC (DISPLAY, pSD->rootWindow, mask, &gcv);


} /* END OF FUNCTION MakeXorGC */



/*************************************<->*************************************
 *
 *  CopyArgv (argc, argv)
 *
 *
 *  Description:
 *  -----------
 *  This function makes a copy of the window manager's argv for use by
 *  the f.restart function.  A copy must be kept because XtInitialize
 *  changes argv.
 *
 *
 *  Inputs:
 *  ------
 *  argc = the number of strings in argv
 *
 *  argv = window manager parameters
 *
 * 
 *  Outputs:
 *  -------
 *  Return = a copy of argv
 *
 *************************************<->***********************************/


#ifdef _NO_PROTO
void CopyArgv (argc, argv)
    int argc;
    char *argv[];

#else /* _NO_PROTO */
void CopyArgv (int argc, char *argv [])
#endif /* _NO_PROTO */
{
    int i;


    if ((wmGD.argv = (char **)XtMalloc ((argc + 1) * sizeof (char *))) == NULL)
    {
	Warning ("Insufficient memory for window manager data");
	wmGD.argv = argv;
    }
    else
    {
	for (i = 0; i < argc; i++)
	{
	    wmGD.argv[i] = argv[i];
	}
	wmGD.argv[i] = NULL;
    }
    
} /* END OF FUNCTION CopyArgv */


/*************************************<->*************************************
 *
 *  InitScreenNames ()
 *
 *
 *  Description:
 *  -----------
 *  Initializes the name space for screen names
 *
 *  Outputs:
 *  -------
 *  Modifies global data
 *    + screenNames
 *
 *  Comments:
 *  --------
 *  Initializes screenNames to contain a numeric name for each screen
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void InitScreenNames ()

#else /* _NO_PROTO */
void InitScreenNames (void)
#endif /* _NO_PROTO */
{
    int num, numScreens;
    
    numScreens = ScreenCount (wmGD.display);
    
    if (!(wmGD.screenNames = 
	  (unsigned char **) XtMalloc (numScreens * sizeof(char *))))
    {
	ShowWaitState (FALSE);
	Warning ("Insufficient memory for screen names");
	exit (WM_ERROR_EXIT_VALUE);
    }
    
    for (num=0; num<numScreens; num++)
    {
	if (!(wmGD.screenNames[num] = 
	      (unsigned char *) XtMalloc (4*sizeof(char))))
	{
	    ShowWaitState (FALSE);
	    Warning ("Insufficient memory for screen names");
	    exit (WM_ERROR_EXIT_VALUE);
	}
	/* default name is left justified, 3-chars max, zero terminated */
	sprintf((char *)wmGD.screenNames[num],"%d",num%1000);
    }
}



/******************************<->*************************************
 *
 *  InitWmDisplayEnv
 *
 *
 *  Description:
 *  -----------
 *  This function saves the display string for putenv in F_Exec.
 *
 *  Inputs:
 *  -------
 *
 *  Outputs:
 *  -------
 *************************************<->***********************************/

#ifdef _NO_PROTO
void
InitWmDisplayEnv ()
#else /* _NO_PROTO */
void
InitWmDisplayEnv (void)
#endif /* _NO_PROTO */
{
    char *pDisplayName;
    char buffer[256];
    char displayName[256];

    pDisplayName = DisplayString (DISPLAY);
    
    /*
     * Construct displayString for this string.  
     */
    strcpy(displayName, pDisplayName);
    sprintf(buffer, "DISPLAY=%s",displayName);
    
    /*		
     * Allocate space for the display string
     */
    if ((wmGD.displayString =
	 (String)XtMalloc ((unsigned int) (strlen(buffer) + 1))) == NULL)
    {
	wmGD.displayString = NULL;
	Warning ("Insufficient memory for displayString");
    }
    else
    {
	strcpy(wmGD.displayString, buffer);
    }
    
} /* END OF FUNCTION  InitWmDisplayEnv */
