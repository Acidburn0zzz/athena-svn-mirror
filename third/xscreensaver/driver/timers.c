/* timers.c --- detecting when the user is idle, and other timer-related tasks.
 * xscreensaver, Copyright (c) 1991-1997, 1998
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* #define DEBUG_TIMERS */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#ifdef HAVE_XMU
# ifndef VMS
#  include <X11/Xmu/Error.h>
# else /* VMS */
#  include <Xmu/Error.h>
# endif /* VMS */
# else /* !HAVE_XMU */
# include "xmu.h"
#endif /* !HAVE_XMU */

#ifdef HAVE_XIDLE_EXTENSION
#include <X11/extensions/xidle.h>
#endif /* HAVE_XIDLE_EXTENSION */

#ifdef HAVE_MIT_SAVER_EXTENSION
#include <X11/extensions/scrnsaver.h>
#endif /* HAVE_MIT_SAVER_EXTENSION */

#ifdef HAVE_SGI_SAVER_EXTENSION
#include <X11/extensions/XScreenSaver.h>
#endif /* HAVE_SGI_SAVER_EXTENSION */

#ifdef HAVE_XHPDISABLERESET
# include <X11/XHPlib.h>
  extern Bool hp_locked_p;	/* from windows.c */
#endif /* HAVE_XHPDISABLERESET */

#include "xscreensaver.h"

#ifdef HAVE_PROC_INTERRUPTS
static Bool proc_interrupts_activity_p (saver_info *si);
#endif /* HAVE_PROC_INTERRUPTS */


void
idle_timer (XtPointer closure, XtIntervalId *id)
{
  saver_info *si = (saver_info *) closure;

  /* What an amazingly shitty design.  Not only does Xt execute timeout
     events from XtAppNextEvent() instead of from XtDispatchEvent(), but
     there is no way to tell Xt to block until there is an X event OR a
     timeout happens.  Once your timeout proc is called, XtAppNextEvent()
     still won't return until a "real" X event comes in.

     So this function pushes a stupid, gratuitous, unnecessary event back
     on the event queue to force XtAppNextEvent to return Right Fucking Now.
     When the code in sleep_until_idle() sees an event of type XAnyEvent,
     which the server never generates, it knows that a timeout has occurred.
   */
  XEvent fake_event;
  fake_event.type = 0;	/* XAnyEvent type, ignored. */
  fake_event.xany.display = si->dpy;
  fake_event.xany.window  = 0;
  XPutBackEvent (si->dpy, &fake_event);
}


static void
schedule_wakeup_event (saver_info *si, Time when, Bool verbose_p)
{
  /* Wake up periodically to ask the server if we are idle. */
  si->timer_id = XtAppAddTimeOut (si->app, when, idle_timer,
                                  (XtPointer) si);

#ifdef DEBUG_TIMERS
  if (verbose_p)
    fprintf (stderr, "%s: starting idle_timer (%ld, %ld)\n",
             blurb(), when, si->timer_id);
#endif /* DEBUG_TIMERS */
}


static void
notice_events (saver_info *si, Window window, Bool top_p)
{
  saver_preferences *p = &si->prefs;
  XWindowAttributes attrs;
  unsigned long events;
  Window root, parent, *kids;
  unsigned int nkids;

  if (XtWindowToWidget (si->dpy, window))
    /* If it's one of ours, don't mess up its event mask. */
    return;

  if (!XQueryTree (si->dpy, window, &root, &parent, &kids, &nkids))
    return;
  if (window == root)
    top_p = False;

  XGetWindowAttributes (si->dpy, window, &attrs);
  events = ((attrs.all_event_masks | attrs.do_not_propagate_mask)
	    & KeyPressMask);

  /* Select for SubstructureNotify on all windows.
     Select for KeyPress on all windows that already have it selected.
     Do we need to select for ButtonRelease?  I don't think so.
   */
  XSelectInput (si->dpy, window, SubstructureNotifyMask | events);

  if (top_p && p->verbose_p && (events & KeyPressMask))
    {
      /* Only mention one window per tree (hack hack). */
      fprintf (stderr, "%s: selected KeyPress on 0x%lX\n", blurb(),
	       (unsigned long) window);
      top_p = False;
    }

  if (kids)
    {
      while (nkids)
	notice_events (si, kids [--nkids], top_p);
      XFree ((char *) kids);
    }
}


int
BadWindow_ehandler (Display *dpy, XErrorEvent *error)
{
  /* When we notice a window being created, we spawn a timer that waits
     30 seconds or so, and then selects events on that window.  This error
     handler is used so that we can cope with the fact that the window
     may have been destroyed <30 seconds after it was created.
   */
  if (error->error_code == BadWindow ||
      error->error_code == BadMatch ||
      error->error_code == BadDrawable)
    return 0;
  else
    return saver_ehandler (dpy, error);
}


struct notice_events_timer_arg {
  saver_info *si;
  Window w;
};

static void
notice_events_timer (XtPointer closure, XtIntervalId *id)
{
  struct notice_events_timer_arg *arg =
    (struct notice_events_timer_arg *) closure;

  XErrorHandler old_handler = XSetErrorHandler (BadWindow_ehandler);

  saver_info *si = arg->si;
  Window window = arg->w;

  free(arg);
  notice_events (si, window, True);
  XSync (si->dpy, False);
  XSetErrorHandler (old_handler);
}

void
start_notice_events_timer (saver_info *si, Window w, Bool verbose_p)
{
  saver_preferences *p = &si->prefs;
  struct notice_events_timer_arg *arg =
    (struct notice_events_timer_arg *) malloc(sizeof(*arg));
  arg->si = si;
  arg->w = w;
  XtAppAddTimeOut (si->app, p->notice_events_timeout, notice_events_timer,
		   (XtPointer) arg);

  if (verbose_p)
    fprintf (stderr, "%s: starting notice_events_timer for 0x%X (%lu)\n",
             blurb(), (unsigned int) w, p->notice_events_timeout);
}


/* When the screensaver is active, this timer will periodically change
   the running program.
 */
void
cycle_timer (XtPointer closure, XtIntervalId *id)
{
  saver_info *si = (saver_info *) closure;
  saver_preferences *p = &si->prefs;
  Time how_long = p->cycle;
  if (si->dbox_up_p)
    {
      if (p->verbose_p)
	fprintf (stderr, "%s: dialog box up; delaying hack change.\n",
		 blurb());
      how_long = 30000; /* 30 secs */
    }
  else
    {
      maybe_reload_init_file (si);
      if (p->verbose_p)
	fprintf (stderr, "%s: changing graphics hacks.\n", blurb());
      kill_screenhack (si);
      spawn_screenhack (si, False);
    }
  si->cycle_id = XtAppAddTimeOut (si->app, how_long, cycle_timer,
				  (XtPointer) si);

#ifdef DEBUG_TIMERS
  if (p->verbose_p)
    fprintf (stderr, "%s: starting cycle_timer (%ld, %ld)\n",
	    blurb(), how_long, si->cycle_id);
#endif /* DEBUG_TIMERS */
}


void
activate_lock_timer (XtPointer closure, XtIntervalId *id)
{
  saver_info *si = (saver_info *) closure;
  saver_preferences *p = &si->prefs;

  if (p->verbose_p)
    fprintf (stderr, "%s: timed out; activating lock\n", blurb());
  si->locked_p = True;
  si->locked_due_to_idle_p = True;

#ifdef HAVE_XHPDISABLERESET
  if (!hp_locked_p)
    {
      XHPDisableReset (si->dpy);	/* turn off C-Sh-Reset */
      hp_locked_p = True;
    }
#endif
}


/* Call this when user activity (or "simulated" activity) has been noticed.
 */
static void
reset_timers (saver_info *si)
{
  saver_preferences *p = &si->prefs;
  if (si->using_mit_saver_extension || si->using_sgi_saver_extension)
    return;

  if (si->timer_id)
    {
#ifdef DEBUG_TIMERS
      if (p->verbose_p)
        fprintf (stderr, "%s: killing idle_timer  (%ld, %ld)\n",
                 blurb(), p->timeout, si->timer_id);
#endif /* DEBUG_TIMERS */
      XtRemoveTimeOut (si->timer_id);
    }

  schedule_wakeup_event (si, p->timeout, p->verbose_p); /* sets si->timer_id */

  if (si->cycle_id) abort ();	/* no cycle timer when inactive */

  si->last_activity_time = time ((time_t *) 0);
}


/* When we aren't using a server extension, this timer is used to periodically
   wake up and poll the mouse position, which is possibly more reliable than
   selecting motion events on every window.
 */
static void
check_pointer_timer (XtPointer closure, XtIntervalId *id)
{
  int i;
  saver_info *si = (saver_info *) closure;
  saver_preferences *p = &si->prefs;
  Bool active_p = False;

  if (!si->using_proc_interrupts &&
      (si->using_xidle_extension ||
       si->using_mit_saver_extension ||
       si->using_sgi_saver_extension))
    /* If an extension is in use, we should not be polling the mouse.
       Unless we're also checking /proc/interrupts, in which case, we should.
     */
    abort ();

  si->check_pointer_timer_id =
    XtAppAddTimeOut (si->app, p->pointer_timeout, check_pointer_timer,
		     (XtPointer) si);

  for (i = 0; i < si->nscreens; i++)
    {
      saver_screen_info *ssi = &si->screens[i];
      Window root, child;
      int root_x, root_y, x, y;
      unsigned int mask;

      XQueryPointer (si->dpy, ssi->screensaver_window, &root, &child,
		     &root_x, &root_y, &x, &y, &mask);

      if (root_x == ssi->poll_mouse_last_root_x &&
	  root_y == ssi->poll_mouse_last_root_y &&
	  child  == ssi->poll_mouse_last_child &&
	  mask   == ssi->poll_mouse_last_mask)
	continue;

      active_p = True;

#ifdef DEBUG_TIMERS
      if (p->verbose_p)
	if (root_x == ssi->poll_mouse_last_root_x &&
	    root_y == ssi->poll_mouse_last_root_y &&
	    child  == ssi->poll_mouse_last_child)
	  fprintf (stderr, "%s: modifiers changed at %s on screen %d.\n",
		   blurb(), timestring(), i);
	else
	  fprintf (stderr, "%s: pointer moved at %s on screen %d.\n",
		   blurb(), timestring(), i);
#endif /* DEBUG_TIMERS */

      si->last_activity_screen    = ssi;
      ssi->poll_mouse_last_root_x = root_x;
      ssi->poll_mouse_last_root_y = root_y;
      ssi->poll_mouse_last_child  = child;
      ssi->poll_mouse_last_mask   = mask;
    }

#ifdef HAVE_PROC_INTERRUPTS
  if (!active_p &&
      si->using_proc_interrupts &&
      proc_interrupts_activity_p (si))
    {
# ifdef DEBUG_TIMERS
      if (p->verbose_p)
        fprintf (stderr, "%s: /proc/interrupts activity at %s.\n",
                 blurb(), timestring());
# endif /* DEBUG_TIMERS */
      active_p = True;
    }
#endif /* HAVE_PROC_INTERRUPTS */

  if (active_p)
    reset_timers (si);
}


static void
dispatch_event (saver_info *si, XEvent *event)
{
  /* If this is for the splash dialog, pass it along.
     Note that the password dialog is handled with its own event loop,
     so events for that window will never come through here.
   */
  if (si->splash_dialog && event->xany.window == si->splash_dialog)
    handle_splash_event (si, event);

  XtDispatchEvent (event);
}


/* methods of detecting idleness:

      explicitly informed by SGI SCREEN_SAVER server event;
      explicitly informed by MIT-SCREEN-SAVER server event;
      poll server idle time with XIDLE extension;
      select events on all windows, and note absence of recent events;
      note that /proc/interrupts has not changed in a while;
      activated by clientmessage.

   methods of detecting non-idleness:

      read events on the xscreensaver window;
      explicitly informed by SGI SCREEN_SAVER server event;
      explicitly informed by MIT-SCREEN-SAVER server event;
      select events on all windows, and note events on any of them;
      note that /proc/interrupts has changed;
      deactivated by clientmessage.

   I trust that explains why this function is a big hairy mess.
 */
void
sleep_until_idle (saver_info *si, Bool until_idle_p)
{
  saver_preferences *p = &si->prefs;
  XEvent event;

  /* We need to select events on all windows if we're not using any extensions.
     Otherwise, we don't need to. */
  Bool scanning_all_windows = !(si->using_xidle_extension ||
                                si->using_mit_saver_extension ||
                                si->using_sgi_saver_extension);

  /* We need to periodically wake up and check for idleness if we're not using
     any extensions, or if we're using the XIDLE extension.  The other two
     extensions explicitly deliver events when we go idle/non-idle, so we
     don't need to poll. */
  Bool polling_for_idleness = !(si->using_mit_saver_extension ||
                                si->using_sgi_saver_extension);

  /* Whether we need to periodically wake up and check to see if the mouse has
     moved.  We only need to do this when not using any extensions.  The reason
     this isn't the same as `polling_for_idleness' is that the "idleness" poll
     can happen (for example) 5 minutes from now, whereas the mouse-position
     poll should happen with low periodicity.  We don't need to poll the mouse
     position with the XIDLE extension, but we do need to periodically wake up
     and query the server with that extension.  For our purposes, polling
     /proc/interrupts is just like polling the mouse position.  It has to
     happen on the same kind of schedule. */
  Bool polling_mouse_position = (si->using_proc_interrupts ||
                                 !(si->using_xidle_extension ||
                                   si->using_mit_saver_extension ||
                                   si->using_sgi_saver_extension));

  if (until_idle_p)
    {
      if (polling_for_idleness)
        /* This causes a no-op event to be delivered to us in a while, so that
           we come back around through the event loop again.  Use of this timer
           is economical: for example, if the screensaver should come on in 5
           minutes, and the user has been idle for 2 minutes, then this
           timeout will go off no sooner than 3 minutes from now.  */
        schedule_wakeup_event (si, p->timeout, p->verbose_p);

      if (polling_mouse_position)
        /* Check to see if the mouse has moved, and set up a repeating timer
           to do so periodically (typically, every 5 seconds.) */
	check_pointer_timer ((XtPointer) si, 0);
    }

  while (1)
    {
      XtAppNextEvent (si->app, &event);

      switch (event.xany.type) {
      case 0:		/* our synthetic "timeout" event has been signalled */
	if (until_idle_p)
	  {
	    Time idle;
#ifdef HAVE_XIDLE_EXTENSION
	    if (si->using_xidle_extension)
	      {
                /* The XIDLE extension uses the synthetic event to prod us into
                   re-asking the server how long the user has been idle. */
		if (! XGetIdleTime (si->dpy, &idle))
		  {
		    fprintf (stderr, "%s: XGetIdleTime() failed.\n", blurb());
		    saver_exit (si, 1, 0);
		  }
	      }
	    else
#endif /* HAVE_XIDLE_EXTENSION */
#ifdef HAVE_MIT_SAVER_EXTENSION
	      if (si->using_mit_saver_extension)
		{
		  /* We don't need to do anything in this case - the synthetic
		     event isn't necessary, as we get sent specific events
		     to wake us up.  In fact, this event generally shouldn't
                     be being delivered when the MIT extension is in use. */
		  idle = 0;
		}
	    else
#endif /* HAVE_MIT_SAVER_EXTENSION */
#ifdef HAVE_SGI_SAVER_EXTENSION
	      if (si->using_sgi_saver_extension)
		{
		  /* We don't need to do anything in this case - the synthetic
		     event isn't necessary, as we get sent specific events
		     to wake us up.  In fact, this event generally shouldn't
                     be being delivered when the SGI extension is in use. */
		  idle = 0;
		}
	    else
#endif /* HAVE_SGI_SAVER_EXTENSION */
	      {
                /* Otherwise, no server extension is in use.  The synthetic
                   event was to tell us to wake up and see if the user is now
                   idle.  Compute the amount of idle time by comparing the
                   `last_activity_time' to the wall clock.  The l_a_t was set
                   by calling `reset_timers()', which is called only in only
                   two situations: when polling the mouse position has revealed
                   the the mouse has moved (user activity) or when we have read
                   an event (again, user activity.)
                 */
		idle = 1000 * (si->last_activity_time - time ((time_t *) 0));
	      }

	    if (idle >= p->timeout)
              {
                /* Look, we've been idle long enough.  We're done. */
                goto DONE;
              }
            else
              {
                /* The event went off, but it turns out that the user has not
                   yet been idle for long enough.  So re-signal the event.
                   */
                if (polling_for_idleness)
                  schedule_wakeup_event (si, p->timeout - idle, p->verbose_p);
              }
	  }
	break;

      case ClientMessage:
	if (handle_clientmessage (si, &event, until_idle_p))
	  goto DONE;
	break;

      case CreateNotify:
        /* A window has been created on the screen somewhere.  If we're
           supposed to scan all windows for events, prepare this window. */
	if (scanning_all_windows)
	  {
            Window w = event.xcreatewindow.window;
#ifdef DEBUG_TIMERS
	    start_notice_events_timer (si, w, p->verbose_p);
#else  /* !DEBUG_TIMERS */
	    start_notice_events_timer (si, w, False);
#endif /* !DEBUG_TIMERS */
	  }
	break;

      case KeyPress:
      case KeyRelease:
      case ButtonPress:
      case ButtonRelease:
      case MotionNotify:

#ifdef DEBUG_TIMERS
	if (p->verbose_p)
	  {
	    if (event.xany.type == MotionNotify)
	      fprintf (stderr,"%s: MotionNotify at %s\n",blurb(),timestring());
	    else if (event.xany.type == KeyPress)
	      fprintf (stderr, "%s: KeyPress seen on 0x%X at %s\n", blurb(),
		       (unsigned int) event.xkey.window, timestring ());
	    else if (event.xany.type == ButtonPress)
	      fprintf (stderr, "%s: ButtonPress seen on 0x%X at %s\n", blurb(),
		       (unsigned int) event.xbutton.window, timestring ());
	  }
#endif /* DEBUG_TIMERS */

	/* If any widgets want to handle this event, let them. */
	dispatch_event (si, &event);

	/* We got a user event.
	   If we're waiting for the user to become active, this is it.
	   If we're waiting until the user becomes idle, reset the timers
	   (since now we have longer to wait.)
	 */
	if (!until_idle_p)
	  {
	    if (si->demoing_p &&
		(event.xany.type == MotionNotify ||
		 event.xany.type == KeyRelease))
	      /* When we're demoing a single hack, mouse motion doesn't
		 cause deactivation.  Only clicks and keypresses do. */
	      ;
	    else
	      /* If we're not demoing, then any activity causes deactivation.
	       */
	      goto DONE;
	  }
	else
	  reset_timers (si);

	break;

      case ConfigureNotify:
      case VisibilityNotify:
        if (!until_idle_p)
	{
	  int i;

	  /* Something happened, and we're no longer the topmost window.
	   * Forcibly raise ourselves to solve this problem. */
	  for (i = 0; i < si->nscreens; i++)
	    XRaiseWindow(si->dpy, si->screens[i].screensaver_window);
	}
	break;

      default:

#ifdef HAVE_MIT_SAVER_EXTENSION
	if (event.type == si->mit_saver_ext_event_number)
	  {
            /* This event's number is that of the MIT-SCREEN-SAVER server
               extension.  This extension has one event number, and the event
               itself contains sub-codes that say what kind of event it was
               (an "idle" or "not-idle" event.)
             */
	    XScreenSaverNotifyEvent *sevent =
	      (XScreenSaverNotifyEvent *) &event;
	    if (sevent->state == ScreenSaverOn)
	      {
		int i = 0;
		if (p->verbose_p)
		  fprintf (stderr, "%s: MIT ScreenSaverOn event received.\n",
			   blurb());

		/* Get the "real" server window(s) out of the way as soon
		   as possible. */
		for (i = 0; i < si->nscreens; i++)
		  {
		    saver_screen_info *ssi = &si->screens[i];
		    if (ssi->server_mit_saver_window &&
			window_exists_p (si->dpy,
					 ssi->server_mit_saver_window))
		      XUnmapWindow (si->dpy, ssi->server_mit_saver_window);
		  }

		if (sevent->kind != ScreenSaverExternal)
		  {
		    fprintf (stderr,
			 "%s: ScreenSaverOn event wasn't of type External!\n",
			     blurb());
		  }

		if (until_idle_p)
		  goto DONE;
	      }
	    else if (sevent->state == ScreenSaverOff)
	      {
		if (p->verbose_p)
		  fprintf (stderr, "%s: MIT ScreenSaverOff event received.\n",
			   blurb());
		if (!until_idle_p)
		  goto DONE;
	      }
	    else
	      fprintf (stderr,
		       "%s: unknown MIT-SCREEN-SAVER event %d received!\n",
		       blurb(), sevent->state);
	  }
	else

#endif /* HAVE_MIT_SAVER_EXTENSION */


#ifdef HAVE_SGI_SAVER_EXTENSION
	if (event.type == (si->sgi_saver_ext_event_number + ScreenSaverStart))
	  {
            /* The SGI SCREEN_SAVER server extension has two event numbers,
               and this event matches the "idle" event. */
	    if (p->verbose_p)
	      fprintf (stderr, "%s: SGI ScreenSaverStart event received.\n",
		       blurb());

	    if (until_idle_p)
	      goto DONE;
	  }
	else if (event.type == (si->sgi_saver_ext_event_number +
				ScreenSaverEnd))
	  {
            /* The SGI SCREEN_SAVER server extension has two event numbers,
               and this event matches the "idle" event. */
	    if (p->verbose_p)
	      fprintf (stderr, "%s: SGI ScreenSaverEnd event received.\n",
		       blurb());
	    if (!until_idle_p)
	      goto DONE;
	  }
	else
#endif /* HAVE_SGI_SAVER_EXTENSION */

          /* Just some random event.  Let the Widgets handle it, if desired. */
	  dispatch_event (si, &event);
      }
    }
 DONE:


  /* If there's a user event on the queue, swallow it.
     If we're using a server extension, and the user becomes active, we
     get the extension event before the user event -- so the keypress or
     motion or whatever is still on the queue.  This makes "unfade" not
     work, because it sees that event, and bugs out.  (This problem
     doesn't exhibit itself without an extension, because in that case,
     there's only one event generated by user activity, not two.)
   */
  XCheckMaskEvent (si->dpy, (KeyPressMask|ButtonPressMask|PointerMotionMask),
		   &event);


  if (si->check_pointer_timer_id)
    {
      XtRemoveTimeOut (si->check_pointer_timer_id);
      si->check_pointer_timer_id = 0;
    }
  if (si->timer_id)
    {
      XtRemoveTimeOut (si->timer_id);
      si->timer_id = 0;
    }

  if (until_idle_p && si->cycle_id)	/* no cycle timer when inactive */
    abort ();

  return;
}



/* Some crap for dealing with /proc/interrupts.

   On Linux systems, it's possible to see the hardware interrupt count
   associated with the keyboard.  We can therefore use that as another method
   of detecting idleness.

   Why is it a good idea to do this?  Because it lets us detect keyboard
   activity that is not associated with X events.  For example, if the user
   has switched to another virtual console, it's good for xscreensaver to not
   be running graphics hacks on the (non-visible) X display.  The common
   complaint that checking /proc/interrupts addresses is that the user is
   playing Quake on a non-X console, and the GL hacks are perceptibly slowing
   the game...

   This is tricky for a number of reasons.

     * First, we must be sure to only do this when running on an X server that
       is on the local machine (because otherwise, we'd be reacting to the
       wrong keyboard.)  The way we do this is by noting that the $DISPLAY is
       pointing to display 0 on the local machine.  It *could* be that display
       1 is also on the local machine (e.g., two X servers, each on a different
       virtual-terminal) but it's also possible that screen 1 is an X terminal,
       using this machine as the host.  So we can't take that chance.

     * Second, one can only access these interrupt numbers in a completely
       and utterly brain-damaged way.  You would think that one would use an
       ioctl for this.  But no.  The ONLY way to get this information is to
       open the pseudo-file /proc/interrupts AS A FILE, and read the numbers
       out of it TEXTUALLY.  Because this is Unix, and all the world's a file,
       and the only real data type is the short-line sequence of ASCII bytes.

       Now it's all well and good that the /proc/interrupts pseudo-file
       exists; that's a clever idea, and a useful API for things that are
       already textually oriented, like shell scripts, and users doing
       interactive debugging sessions.  But to make a *C PROGRAM* open a file
       and parse the textual representation of integers out of it is just
       insane.

     * Third, you can't just hold the file open, and fseek() back to the
       beginning to get updated data!  If you do that, the data never changes.
       And I don't want to call open() every five seconds, because I don't want
       to risk going to disk for any inodes.  It turns out that if you dup()
       it early, then each copy gets fresh data, so we can get around that in
       this way (but for how many releases, one might wonder?)

     * Fourth, the format of the output of the /proc/interrupts file is
       undocumented, and has changed several times already!  In Linux 2.0.33,
       even on a multiprocessor machine, it looks like this:

          0:  309453991   timer
          1:    4771729   keyboard
   
       but on later kernels with MP machines, it looks like this:

                   CPU0       CPU1
          0:    1671450    1672618    IO-APIC-edge  timer
          1:      13037      13495    IO-APIC-edge  keyboard

       Joy!  So how are we expected to parse that?  Well, this code doesn't
       parse it: it saves the last line with the string "keyboard" in it, and
       does a string-comparison to note when it has changed.

   Thanks to Nat Friedman <nat@nat.org> for figuring out all of this crap.

   Note that this only checks for lines with "keyboard" in them.  Perhaps we
   should also be checking for lines with "PS/2 Mouse" in them.  But that
   would obviously fail to work for regular serial mice, and obviously just
   using COM1 would be bad news (turn off the screensaver because the modem
   is active, yum.)
 */


#ifdef HAVE_PROC_INTERRUPTS

#define PROC_INTERRUPTS "/proc/interrupts"

Bool
query_proc_interrupts_available (saver_info *si, const char **why)
{
  /* We can use /proc/interrupts if $DISPLAY points to :0, and if the
     "/proc/interrupts" file exists and is readable.
   */
  FILE *f;
  if (why) *why = 0;

  if (!display_is_on_console_p (si))
    {
      if (why) *why = "not on primary console";
      return False;
    }

  f = fopen (PROC_INTERRUPTS, "r");
  if (!f)
    return False;

  fclose (f);
  return True;
}


static Bool
proc_interrupts_activity_p (saver_info *si)
{
  static FILE *f0 = 0;
  FILE *f1 = 0;
  int fd;
  static char last_line[255] = { 0, };
  char new_line[sizeof(last_line)];

  if (!f0)
    {
      /* First time -- open the file. */
      f0 = fopen (PROC_INTERRUPTS, "r");
      if (!f0)
        {
          char buf[255];
          sprintf(buf, "%s: error opening %s", blurb(), PROC_INTERRUPTS);
          perror (buf);
          goto FAIL;
        }
    }

  if (f0 == (FILE *) -1)	    /* means we got an error initializing. */
    return False;

  fd = dup (fileno (f0));
  if (fd < 0)
    {
      char buf[255];
      sprintf(buf, "%s: could not dup() the %s fd", blurb(), PROC_INTERRUPTS);
      perror (buf);
      goto FAIL;
    }

  f1 = fdopen (fd, "r");
  if (!f1)
    {
      char buf[255];
      sprintf(buf, "%s: could not fdopen() the %s fd", blurb(),
              PROC_INTERRUPTS);
      perror (buf);
      goto FAIL;
    }

  /* Actually, I'm unclear on why this fseek() is necessary, given the timing
     of the dup() above, but it is. */
  if (fseek (f1, 0, SEEK_SET) != 0)
    {
      char buf[255];
      sprintf(buf, "%s: error rewinding %s", blurb(), PROC_INTERRUPTS);
      perror (buf);
      goto FAIL;
    }

  /* Now read through the pseudo-file until we find the "keyboard" line. */

  while (fgets (new_line, sizeof(new_line)-1, f1))
    if (strstr (new_line, "keyboard"))
      {
        Bool diff = (*last_line &&
                     !!strcmp (new_line, last_line));
        strcpy (last_line, new_line);	/* save this line for next time */
        fclose (f1);
        return diff;
      }

  /* If we got here, we didn't find a "keyboard" line in the file at all. */
  fprintf (stderr, "%s: no keyboard data in %s?\n", blurb(), PROC_INTERRUPTS);

 FAIL:
  if (f1)
    fclose (f1);

  if (f0 && f0 != (FILE *) -1)
    fclose (f0);

  f0 = (FILE *) -1;
  return False;
}

#endif /* HAVE_PROC_INTERRUPTS */


/* This timer goes off every few minutes, whether the user is idle or not,
   to try and clean up anything that has gone wrong.

   It calls disable_builtin_screensaver() so that if xset has been used,
   or some other program (like xlock) has messed with the XSetScreenSaver()
   settings, they will be set back to sensible values (if a server extension
   is in use, messing with xlock can cause xscreensaver to never get a wakeup
   event, and could cause monitor power-saving to occur, and all manner of
   heinousness.)

   If the screen is currently blanked, it raises the window, in case some
   other window has been mapped on top of it.

   If the screen is currently blanked, and there is no hack running, it
   clears the window, in case there is an error message printed on it (we
   don't want the error message to burn in.)
 */

static void
watchdog_timer (XtPointer closure, XtIntervalId *id)
{
  saver_info *si = (saver_info *) closure;

  disable_builtin_screensaver (si, False);

  if (si->screen_blanked_p)
    {
      Bool running_p = screenhack_running_p(si);

#ifdef DEBUG_TIMERS
      if (si->prefs.verbose_p)
	fprintf (stderr, "%s: watchdog timer raising %sscreen.\n",
		 blurb(), (running_p ? "" : "and clearing "));
#endif /* DEBUG_TIMERS */

      raise_window (si, True, True, running_p);

      if (!monitor_powered_on_p (si))
	{
	  if (si->prefs.verbose_p)
	    fprintf (stderr,
		     "%s: server reports that monitor has powered down; "
		     "killing running hacks.\n", blurb());
	  kill_screenhack (si);
	}
    }
}


void
reset_watchdog_timer (saver_info *si, Bool on_p)
{
  saver_preferences *p = &si->prefs;

  if (si->watchdog_id)
    {
      XtRemoveTimeOut (si->watchdog_id);
      si->watchdog_id = 0;
    }

  if (on_p && p->watchdog_timeout)
    {
      si->watchdog_id = XtAppAddTimeOut (si->app, p->watchdog_timeout,
					 watchdog_timer, (XtPointer) si);

#ifdef DEBUG_TIMERS
      if (p->verbose_p)
	fprintf (stderr, "%s: restarting watchdog_timer (%ld, %ld)\n",
		 blurb(), p->watchdog_timeout, si->watchdog_id);
#endif /* DEBUG_TIMERS */

    }
}
