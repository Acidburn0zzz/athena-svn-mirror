/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  liboaf: A library for accessing oafd in a nice way.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000, 2001 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Elliot Lee <sopwith@redhat.com>
 *
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <config.h>
#include <string.h>
#include "liboaf-private.h"
#include "oaf-i18n.h"
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#undef OAF_DEBUG

/* Whacked from gnome-libs/libgnorba/orbitns.c */

#define IORBUFSIZE 2048

typedef struct
{
	GMainLoop *mloop;
	char iorbuf[IORBUFSIZE];
#ifdef OAF_DEBUG
	char *do_srv_output;
#endif
	FILE *fh;

        /* For list compares */
        const char *display;
        const char *act_iid;
        const char *exename;
        OAFForkReCheckFn re_check;
        gpointer         user_data;
}
EXEActivateInfo;

static gboolean
handle_exepipe (GIOChannel * source,
		GIOCondition condition, 
                EXEActivateInfo * data)
{
	gboolean retval = TRUE;

        /* The expected thing is to get this callback maybe twice,
         * once with G_IO_IN and once G_IO_HUP, of course we need to handle
         * other cases.
         */
        if (data->iorbuf[0] == '\0' &&
            (condition & (G_IO_IN | G_IO_PRI))) {
                if (!fgets (data->iorbuf, sizeof (data->iorbuf), data->fh)) {
                        g_snprintf (data->iorbuf, IORBUFSIZE,
                                    _("Failed to read from child process: %s\n"),
                                    strerror (errno));

                        retval = FALSE;
                } else {
                        retval = TRUE;
                }
        } else {                
                retval = FALSE;
        }

	if (retval && !strncmp (data->iorbuf, "IOR:", 4))
		retval = FALSE;

#ifdef OAF_DEBUG
	if (data->do_srv_output)
		g_message ("srv output[%d]: '%s'", retval, data->iorbuf);
#endif

	if (!retval)
		g_main_quit (data->mloop);

	return retval;
}

static CORBA_Object
exe_activate_info_to_retval (EXEActivateInfo *ai, CORBA_Environment *ev)
{
        CORBA_Object retval;

        g_strstrip (ai->iorbuf);
        if (!strncmp (ai->iorbuf, "IOR:", 4)) {
                retval = CORBA_ORB_string_to_object (oaf_orb_get (),
                                                     ai->iorbuf, ev);
                if (ev->_major != CORBA_NO_EXCEPTION)
                        retval = CORBA_OBJECT_NIL;
#ifdef OAF_DEBUG
                if (ai->do_srv_output)
                        g_message ("Did string_to_object on %s = '%p' (%s)",
                                   ai->iorbuf, retval,
                                   ev->_major == CORBA_NO_EXCEPTION?
                                   "no-exception" : ev->_repo_id);
#endif
        } else {
                OAF_GeneralError *errval;

#ifdef OAF_DEBUG
                if (ai->do_srv_output)
                        g_message ("string doesn't match IOR:");
#endif

                errval = OAF_GeneralError__alloc ();

                if (*ai->iorbuf == '\0')
                        errval->description =
                                CORBA_string_dup (_("Child process did not give an error message, unknown failure occurred"));
                else
                        errval->description = CORBA_string_dup (ai->iorbuf);
                CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                                     ex_OAF_GeneralError, errval);
                retval = CORBA_OBJECT_NIL;
        }

        return retval;
}

static CORBA_Object
scan_list (GSList *l, EXEActivateInfo *seek_ai, CORBA_Environment *ev)
{
        CORBA_Object retval = CORBA_OBJECT_NIL;

        for (; l; l = l->next) {
                EXEActivateInfo *ai = l->data;

                if (strcmp (seek_ai->exename, ai->exename))
                        continue;

                if (seek_ai->display && ai->display) {
                        if (strcmp (seek_ai->display, ai->display))
                                continue;

                } else if (seek_ai->display || ai->display)
                        continue;

                /* We run the loop too ... */
                g_main_run (ai->mloop);

                if (!strcmp (seek_ai->act_iid, ai->act_iid)) {
#ifdef OAF_DEBUG
                        g_warning ("Hit the jackpot '%s' '%s'",
                                   seek_ai->act_iid, ai->act_iid);
#endif
                        retval = exe_activate_info_to_retval (ai, ev);
                } else if (seek_ai->re_check) {
                        /* It might have just registered the IID */
#ifdef OAF_DEBUG
                        g_warning ("Re-check the thing ... '%s' '%s'",
                                   seek_ai->act_iid, ai->act_iid);
#endif
                        retval = seek_ai->re_check (
                                seek_ai->display, seek_ai->act_iid,
                                seek_ai->user_data, ev);
                }
        }

        return retval;
}

#ifdef OAF_DEBUG
static void
print_exit_status (int status)
{
	if (WIFEXITED (status))
		g_message ("Exit status was %d", WEXITSTATUS (status));

	if (WIFSIGNALED (status))
		g_message ("signal was %d", WTERMSIG (status));
}
#endif

static 
void oaf_setenv (const char *name, const char *value) 
{
#if HAVE_SETENV
        setenv (name, value, 1);
#else
        char *tmp;
                
        tmp = g_strconcat (name, "=", value, NULL);
        
        putenv (tmp);
#endif
}

CORBA_Object
oaf_server_by_forking (const char **cmd,
                       gboolean set_process_group,
                       int fd_arg, 
                       const char *display,
                       const char *od_iorstr,
                       const char *act_iid,
                       OAFForkReCheckFn re_check,
                       gpointer         user_data,
                       CORBA_Environment *ev)
{
	gint iopipes[2];
	CORBA_Object retval = CORBA_OBJECT_NIL;
	OAF_GeneralError *errval;
        FILE *iorfh;
        EXEActivateInfo ai;
        GIOChannel *gioc;
        int childpid;
        int status;
        guint watchid;
        struct sigaction sa;
        sigset_t mask, omask;
        int parent_pid;
        static GSList *running_activations = NULL;

        g_return_val_if_fail (cmd != NULL, CORBA_OBJECT_NIL);
        g_return_val_if_fail (cmd [0] != NULL, CORBA_OBJECT_NIL);
        g_return_val_if_fail (act_iid != NULL, CORBA_OBJECT_NIL);

        ai.display = display;
        ai.act_iid = act_iid;
        ai.exename = cmd [0];
        ai.re_check = re_check;
        ai.user_data = user_data;

        if ((retval = scan_list (running_activations, &ai, ev)) != CORBA_OBJECT_NIL)
                return retval;
        
     	pipe (iopipes);

        /* Block SIGCHLD so no one else can wait() on the child before us. */
        sigemptyset (&mask);
        sigaddset (&mask, SIGCHLD);
        sigprocmask (SIG_BLOCK, &mask, &omask);

        parent_pid = getpid ();
        
	/* fork & get the IOR from the magic pipe */
	childpid = fork ();

	if (childpid < 0) {
                sigprocmask (SIG_SETMASK, &omask, NULL);
		errval = OAF_GeneralError__alloc ();
		errval->description = CORBA_string_dup (_("Couldn't fork a new process"));

		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_OAF_GeneralError, errval);
		return CORBA_OBJECT_NIL;
	}

	if (childpid != 0) {
                /* de-zombify */
                while (waitpid (childpid, &status, 0) == -1 && errno == EINTR)
                        ;
                sigprocmask (SIG_SETMASK, &omask, NULL);
                
		if (!WIFEXITED (status)) {
			OAF_GeneralError *errval;
			char cbuf[512];
                        
			errval = OAF_GeneralError__alloc ();

			if (WIFSIGNALED (status))
				g_snprintf (cbuf, sizeof (cbuf),
					    _("Child received signal %u (%s)"),
					    WTERMSIG (status),
					    g_strsignal (WTERMSIG
                                                         (status)));
			else
				g_snprintf (cbuf, sizeof (cbuf),
					    _("Unknown non-exit error (status is %u)"),
					    status);
                        errval->description = CORBA_string_dup (cbuf);

			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_OAF_GeneralError, errval);
			return CORBA_OBJECT_NIL;
		}
#ifdef OAF_DEBUG
		ai.do_srv_output = getenv ("OAF_DEBUG_EXERUN");
                
		if (ai.do_srv_output)
			print_exit_status (status);
#endif
                
		close (iopipes[1]);
		ai.fh = iorfh = fdopen (iopipes[0], "r");
                
		ai.iorbuf[0] = '\0';
		ai.mloop = g_main_new (FALSE);

                running_activations = g_slist_prepend (running_activations, &ai);

		gioc = g_io_channel_unix_new (iopipes[0]);
		watchid = g_io_add_watch (gioc,
                                          G_IO_IN | G_IO_HUP | G_IO_NVAL |
                                          G_IO_ERR, (GIOFunc) & handle_exepipe,
                                          &ai);
		g_io_channel_unref (gioc);
		g_main_run (ai.mloop);
		g_main_destroy (ai.mloop);
		fclose (iorfh);

                running_activations = g_slist_remove (running_activations, &ai);

                retval = exe_activate_info_to_retval (&ai, ev);
	} else if ((childpid = fork ())) {
		_exit (0);	/* de-zombifier process, just exit */
	} else {
                if (display)
		  oaf_setenv ("DISPLAY", display);
		if (od_iorstr)
		  oaf_setenv ("OAF_OD_IOR", od_iorstr);
                
		sigprocmask (SIG_SETMASK, &omask, NULL);

		close (iopipes[0]);
                
                if (fd_arg != 0) {
                        cmd[fd_arg] = g_strdup_printf (cmd[fd_arg], iopipes[1]);
                }

		memset (&sa, 0, sizeof (sa));
		sa.sa_handler = SIG_IGN;
		sigaction (SIGPIPE, &sa, 0);

                if (set_process_group) {
                        if (setpgid (getpid (), parent_pid) < 0) {
                                g_print (_("OAF failed to set process group of %s: %s\n"),
                                         cmd[0], g_strerror (errno));
                                _exit (1);
                        }
                } else {
                        setsid ();
                }
                
		execvp (cmd[0], (char **) cmd);
		if (iopipes[1] != 1)
			dup2 (iopipes[1], 1);
		g_print (_("Failed to execute %s: %d (%s)\n"),
                         cmd[0],
                         errno, g_strerror (errno));
		_exit (1);
	}

	return retval;
}
