/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * TrilobiteRootHelper is an object that allows a user process to ask for
 * certain tasks to be handled by the superuser, using the usermode
 * library (man userhelper) to run eazel-helper.  A signal is emitted
 * when the root password is needed, so the caller can ask for it in a
 * special way.
 *
 * Copyright (C) 2000 Eazel, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Robey Pointer <robey@eazel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <gtk/gtk.h>
#include "trilobite-core-utils.h"
#include "trilobite-root-helper.h"

#define USERHELPER_PATH		"/usr/sbin/userhelper"
#define EAZEL_HELPER		"eazel-helper"


static GtkObject *parent_class;

/* signals that a TrilobiteRootHelper can emit */
/* LAST_SIGNAL is just a sentinel marking the end of the list */
enum {
	NEED_PASSWORD,
	TRY_AGAIN,
	LAST_SIGNAL
};
static guint root_helper_signals[LAST_SIGNAL] = { 0 };


/* have to make my own signal marshaller for STRING__NONE (grumble) */
typedef gchar *(*GtkSignal_STRING__NONE) (GtkObject *object, gpointer user_data);
static void
gtk_marshal_STRING__NONE (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	GtkSignal_STRING__NONE rfunc;
	gchar **return_value;

	return_value = GTK_RETLOC_STRING (args[0]);
	rfunc = (GtkSignal_STRING__NONE)func;
	*return_value = (*rfunc) (object, func_data);
}


/* destroy callback, and accessible from the outside world */
void
trilobite_root_helper_destroy (GtkObject *object)
{
	TrilobiteRootHelper *root_helper;
	GList *iter;

	g_return_if_fail (object != NULL);
	g_return_if_fail (TRILOBITE_IS_ROOT_HELPER (object));

	root_helper = TRILOBITE_ROOT_HELPER (object);

	/* anything that needs to be freed? */
	for (iter = g_list_first (root_helper->old_fd_list); iter; iter = g_list_next (iter)) {
		trilobite_debug ("roothelper: tossing old fd %d", (int)(iter->data));
		close ((int)(iter->data));
	}
	g_list_free (root_helper->old_fd_list);

	close (root_helper->pipe_stdin);
	close (root_helper->pipe_stdout);
	if (root_helper->pid) {
		waitpid (root_helper->pid, NULL, 0);
		root_helper->pid = 0;
	}

	/* call parent destructor */
	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		GTK_OBJECT_CLASS (parent_class)->destroy (object);
	}
}

/* class initializer */
static void
trilobite_root_helper_class_initialize (TrilobiteRootHelperClass *klass)
{
	GtkObjectClass *object_class;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	object_class = (GtkObjectClass *)klass;
	object_class->destroy = trilobite_root_helper_destroy;

	root_helper_signals[NEED_PASSWORD] =
		gtk_signal_new ("need_password", 0, object_class->type,
				GTK_SIGNAL_OFFSET (TrilobiteRootHelperClass, need_password),
				gtk_marshal_STRING__NONE, GTK_TYPE_STRING, 0);
	root_helper_signals[TRY_AGAIN] =
		gtk_signal_new ("try_again", 0, object_class->type,
				GTK_SIGNAL_OFFSET (TrilobiteRootHelperClass, try_again),
				gtk_marshal_BOOL__NONE, GTK_TYPE_BOOL, 0);
	gtk_object_class_add_signals (object_class, root_helper_signals, LAST_SIGNAL);

	klass->need_password = NULL;
	klass->try_again = NULL;
}

/* object initializer */
static void
trilobite_root_helper_initialize (TrilobiteRootHelper *object)
{
	TrilobiteRootHelper *root_helper;

	g_assert (object != NULL);
	g_assert (TRILOBITE_IS_ROOT_HELPER (object));

	root_helper = TRILOBITE_ROOT_HELPER (object);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_NEW;
}

/* generate the GtkType for TrilobiteRootHelper */
GtkType
trilobite_root_helper_get_type (void)
{
	static GtkType trilobite_root_helper_type = 0;

	/* First time it's called ? */
	if (! trilobite_root_helper_type) {
		static const GtkTypeInfo root_helper_info = {
			"TrilobiteRootHelper",
			sizeof (TrilobiteRootHelper),
			sizeof (TrilobiteRootHelperClass),
			(GtkClassInitFunc) trilobite_root_helper_class_initialize,
			(GtkObjectInitFunc) trilobite_root_helper_initialize,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		/* Get a unique GtkType */
		trilobite_root_helper_type = gtk_type_unique (GTK_TYPE_OBJECT, &root_helper_info);
	}

	return trilobite_root_helper_type;
}

TrilobiteRootHelper *
trilobite_root_helper_new (void)
{
	return TRILOBITE_ROOT_HELPER (gtk_object_new (TRILOBITE_TYPE_ROOT_HELPER, NULL));
}


/**********   functions that actually implement the root helper   **********/


static int
make_nonblocking (int fd)
{
	int flags;

	flags = fcntl (fd, F_GETFL, 0);
	fcntl (fd, F_SETFL, flags | O_NONBLOCK);
	return 0;
}

static int
make_blocking (int fd)
{
	int flags;

	flags = fcntl (fd, F_GETFL, 0);
	fcntl (fd, F_SETFL, flags &~ O_NONBLOCK);
	return 0;
}


/* wait for and read one character from the eazel-helper */
static char
eazel_helper_response (int pipe_stdout)
{
	struct pollfd pollfd;
	char buffer[4];
	int bytes;

	/* wait for something to come in */
	memset (&pollfd, 0, sizeof (pollfd));
	pollfd.fd = pipe_stdout;
	pollfd.events = POLLIN;
	pollfd.revents = 0;
	poll (&pollfd, 1, 3000);
	bytes = read (pipe_stdout, buffer, 1);
	if (bytes <= 0) {
		return '\0';
	} else {
		return buffer[0];
	}
}

/* read chars from eazel-helper (and discard them) until we reach a linefeed
 * (or there's nothing else in the buffer -- older buggy versions of userhelper
 * won't end msg-type "5" with a linefeed correctly.)
 */
static void
discard_line (int pipe_stdout)
{
	char in;

	do {
		in = 0;
		read (pipe_stdout, &in, 1);
	} while ((in != 0) && (in != '\n'));
}


/* /usr/sbin/userhelper
 * userhelper
 * -w
 * eazel-helper
 */
static TrilobiteRootHelperStatus
eazel_helper_start (int *pipe_stdin, int *pipe_stdout, int *child_pid)
{
	char *pipe_argv[4];
	int useless_stderr;
	char buffer[256];
	TrilobiteRootHelperStatus err;

	pipe_argv[0] = "userhelper";
	pipe_argv[1] = "-w";
	pipe_argv[2] = EAZEL_HELPER;
	pipe_argv[3] = NULL;
	*child_pid = trilobite_pexec (USERHELPER_PATH, pipe_argv, pipe_stdin, pipe_stdout, &useless_stderr);
	if (*child_pid == 0) {
		trilobite_debug ("roothelper: system userhelper utility not found");
		return TRILOBITE_ROOT_HELPER_NO_USERHELPER;
	}
	close (useless_stderr);

	make_nonblocking (*pipe_stdout);

	while (1) {
		switch (buffer[0] = eazel_helper_response (*pipe_stdout)) {
		case '\0':
			trilobite_debug ("roothelper: userhelper died (stdin: %d, stdout: %d, pid: %d",
					 *pipe_stdin, *pipe_stdout, *child_pid);
			err = TRILOBITE_ROOT_HELPER_NO_USERHELPER;
			goto give_up;
		case '2':
			/* need root password.  but first, clear the buffer of crap. */
			discard_line (*pipe_stdout);
			err = TRILOBITE_ROOT_HELPER_NEED_PASSWORD;
			goto done;
		case '3':
		case '5':
		case '6':
		case '7':
		case '8':
			/* informational messages from userhelper -- don't care */
			/* trilobite_debug ("roothelper: info msg '%c'", buffer[0]); */
			discard_line (*pipe_stdout);
			continue;
		case '*':
			/* the winner! */
			discard_line (*pipe_stdout);
			err = TRILOBITE_ROOT_HELPER_SUCCESS;
			goto done;
		default:
			/* ?! */
			g_warning ("TrilobiteRootHelper: unknown userhelper result %02X", buffer[0]);
			err = TRILOBITE_ROOT_HELPER_INTERNAL_ERROR;
			goto give_up;
		}
	}

give_up:
	close (*pipe_stdin);
	close (*pipe_stdout);

done:
	return err;
}

static TrilobiteRootHelperStatus
eazel_helper_password (int pipe_stdin, int pipe_stdout, const char *password)
{
	char buffer[256];
	int bytes;
	TrilobiteRootHelperStatus err;

	/* clear out anything lingering in the input buffer */
	read (pipe_stdout, buffer, 256);

	/* send password followed by linefeed */
	memset (buffer, 0, 256);
	strncpy (buffer, password, 200);
	strcat (buffer, "\n");
	bytes = write (pipe_stdin, buffer, strlen(buffer));

	/* now check for response */
	while (1) {
		switch (buffer[0] = eazel_helper_response (pipe_stdout)) {
		case '\0':
			trilobite_debug ("roothelper: userhelper closed pipe");
			err = TRILOBITE_ROOT_HELPER_LOST_PIPE;
			goto give_up;
		case '2':
			trilobite_debug ("roothelper: bad password");
			err = TRILOBITE_ROOT_HELPER_BAD_PASSWORD;
			/* don't close the pipe, let them try again if they want */
			discard_line (pipe_stdout);
			goto done;
		case '3':
		case '5':
		case '6':
		case '7':
		case '8':
			/* useless info messages */
			trilobite_debug ("roothelper: info msg '%c'", buffer[0]);
			discard_line (pipe_stdout);
			continue;
		case '*':
			/* clear buffer */
			discard_line (pipe_stdout);
			err = TRILOBITE_ROOT_HELPER_SUCCESS;
			goto done;
		default:
			/* ?! */
			g_warning ("TrilobiteRootHelper: unknown userhelper 2nd result %02X", buffer[0]);
			err = TRILOBITE_ROOT_HELPER_INTERNAL_ERROR;
			goto give_up;
		}
	}

give_up:
	close (pipe_stdin);
	close (pipe_stdout);

done:
	return err;
}


/**********   actual API functions to make the root helper work   **********/


/* returns ROOT_HELPER_STATUS_SUCCESS (0) on success */
TrilobiteRootHelperStatus
trilobite_root_helper_start (TrilobiteRootHelper *root_helper)
{
	TrilobiteRootHelperStatus err;
	char *password;
	gboolean try_again;
	int child_pid;

	g_return_val_if_fail (root_helper != NULL, FALSE);
	g_return_val_if_fail (TRILOBITE_IS_ROOT_HELPER (root_helper), FALSE);

	password = NULL;
	if (root_helper->state == TRILOBITE_ROOT_HELPER_STATE_CONNECTED) {
		/* already connected, dude. */
		return TRILOBITE_ROOT_HELPER_SUCCESS;
	}
	if (root_helper->state == TRILOBITE_ROOT_HELPER_STATE_PIPE) {
		/* starting over from a previous operation */
		/* won't hurt to error on close, just don't want to leak fd's in case the
		 * caller forgot to close stdout */
		close (root_helper->pipe_stdout);
		root_helper->pipe_stdin = root_helper->pipe_stdout = -1;
		root_helper->state = TRILOBITE_ROOT_HELPER_STATE_NEW;
	}

	while (1) {
		err = eazel_helper_start (&root_helper->pipe_stdin, &root_helper->pipe_stdout, &child_pid);
		switch (err) {
		case TRILOBITE_ROOT_HELPER_SUCCESS:
			/* already done -- nice! */
			goto connected;
		case TRILOBITE_ROOT_HELPER_NEED_PASSWORD:
			/* the expected case -- continue */
			break;
		default:
			/* anything else is an error */
			goto failed;
		}

		/* now emit a signal and get the password */
		trilobite_debug ("roothelper: asking for password");
		gtk_signal_emit (GTK_OBJECT (root_helper), root_helper_signals[NEED_PASSWORD], &password);
		if (password == NULL) {
			/* bummer.  nobody caught the signal. */
			err = TRILOBITE_ROOT_HELPER_NEED_PASSWORD;
			goto failed;
		}

		err = eazel_helper_password (root_helper->pipe_stdin, root_helper->pipe_stdout, password);
		switch (err) {
		case TRILOBITE_ROOT_HELPER_SUCCESS:
			/* yeah!  set! */
			goto connected;
		case TRILOBITE_ROOT_HELPER_BAD_PASSWORD:
			try_again = FALSE;
			gtk_signal_emit (GTK_OBJECT (root_helper), root_helper_signals[TRY_AGAIN], &try_again);
			if (! try_again) {
				trilobite_debug ("roothelper: not going to try again.");
				/* then we fail. */
				goto failed;
			} else {
				g_free (password);
				/* for some reason, userhelper will spaz out if we close stdin. */
				/* so we have this awful hack where we push stdin onto a stack to close in the
				 * destructor.  pathetic! :( 
				 */
				close (root_helper->pipe_stdout);
				root_helper->old_fd_list = g_list_append(root_helper->old_fd_list,
									 (void *)root_helper->pipe_stdin);
				root_helper->pipe_stdin = root_helper->pipe_stdout = -1;
			}
			break;
		default:
			/* all else is error */
			goto failed;
		}
	}

connected:
	g_free (password);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_CONNECTED;
	root_helper->pid = child_pid;
	return TRILOBITE_ROOT_HELPER_SUCCESS;

failed:
	g_free (password);
	close (root_helper->pipe_stdout);
	close (root_helper->pipe_stdin);
	root_helper->pipe_stdin = root_helper->pipe_stdout = -1;
	waitpid (child_pid, NULL, 0);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_NEW;
	return err;
}

/* on an already-connected root helper, run rpm with the given arguments.
 * argv is a GList of (char *) containing the arguments
 * on success, returns an fd containing stdout/stderr from rpm.
 * on failure, returns -1.
 */
static TrilobiteRootHelperStatus
trilobite_root_helper_run_program (TrilobiteRootHelper *root_helper, const char *program, GList *argv, int *fd)
{
	GList *item;
	char *p, *out;
	int len;

	/* any argument containing an unprintable is an immediate error */
	for (item = g_list_first (argv); item; item = g_list_next (item)) {
		if (item->data == NULL) {
			g_warning ("TrilobiteRootHelper: run %s: argument %d was (null)",
				   program, g_list_position (argv, item));
			continue;
		}
		for (p = (char *)item->data; *p; p++) {
			if ((*p < ' ') || (*p == 127)) {
				g_warning ("TrilobiteRootHelper: run %s: argument %d contained an "
					   "unprintable character: %02X",
					   program, g_list_position (argv, item), *p);
				return TRILOBITE_ROOT_HELPER_BAD_ARGS;
			}
		}
	}

	out = g_strdup_printf ("%s %u\n", program, g_list_length (argv));
	len = strlen (out);
	if (write (root_helper->pipe_stdin, out, len) < len) {
		g_free (out);
		goto bail;
	}
	g_free (out);

	for (item = g_list_first (argv); item; item = g_list_next (item)) {
		len = strlen ((char *)item->data);
		if ((write (root_helper->pipe_stdin, (char *)item->data, len) < len) ||
		    (write (root_helper->pipe_stdin, "\n", 1) < 1)) {
			trilobite_debug ("roothelper: eazel-helper closed pipe before i was done");
			goto bail;
		}
	}

	/* the service will probably call fdopen on this fd, so the pipe needs to be made
	 * blocking, or else the buffered reads and writes will barf everywhere.
	 */
	make_blocking (root_helper->pipe_stdout);
	*fd = root_helper->pipe_stdout;
	close (root_helper->pipe_stdin);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_PIPE;
	return TRILOBITE_ROOT_HELPER_SUCCESS;

bail:
	/* unable to write everything to the pipe */
	close (root_helper->pipe_stdin);
	close (root_helper->pipe_stdout);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_NEW;
	return TRILOBITE_ROOT_HELPER_LOST_PIPE;
}

/* this is horrible.  you should never set your time this way.  use NTP.  please. */
/* ie, this is for demonstration purposes only. */
static TrilobiteRootHelperStatus
trilobite_root_helper_set_time (TrilobiteRootHelper *root_helper, GList *argv, int *fd)
{
	GList *item;
	char *out;
	char ch;
	int len;
	
	if (fd) {
		*fd = -1;
	}

	item = g_list_first (argv);
	if (!item || g_list_next (argv)) {
		g_warning ("TrilobiteRootHelper: set time: needs exactly 1 arg");
		return TRILOBITE_ROOT_HELPER_BAD_ARGS;
	}

	out = g_strdup_printf ("set-time %s\n", (char *)item->data);
	len = strlen (out);
	if (write (root_helper->pipe_stdin, out, len) < len) {
		g_free (out);
		goto bail;
	}
	g_free (out);

	ch = eazel_helper_response (root_helper->pipe_stdout);
	if (ch != '*') {
		goto bail;
	}

	/* success, but pipe isn't needed anymore */
	close (root_helper->pipe_stdin);
	close (root_helper->pipe_stdout);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_NEW;
	return TRILOBITE_ROOT_HELPER_SUCCESS;

bail:
	/* unable to write anything to the pipe */
	close (root_helper->pipe_stdin);
	close (root_helper->pipe_stdout);
	root_helper->state = TRILOBITE_ROOT_HELPER_STATE_NEW;
	return TRILOBITE_ROOT_HELPER_LOST_PIPE;
}


TrilobiteRootHelperStatus
trilobite_root_helper_run (TrilobiteRootHelper *root_helper, TrilobiteRootHelperCommand command, GList *argv, int *fd)
{
	g_return_val_if_fail (root_helper != NULL, TRILOBITE_ROOT_HELPER_BAD_ARGS);
	g_return_val_if_fail (argv != NULL, TRILOBITE_ROOT_HELPER_BAD_ARGS);
	g_return_val_if_fail (TRILOBITE_IS_ROOT_HELPER (root_helper), TRILOBITE_ROOT_HELPER_BAD_ARGS);
	g_return_val_if_fail (TRILOBITE_ROOT_HELPER_IS_CONNECTED (root_helper), TRILOBITE_ROOT_HELPER_BAD_ARGS);

	switch (command) {
	case TRILOBITE_ROOT_HELPER_RUN_RPM:
		return trilobite_root_helper_run_program (root_helper, "rpm", argv, fd);
	case TRILOBITE_ROOT_HELPER_RUN_SET_TIME:
		return trilobite_root_helper_set_time (root_helper, argv, fd);
	case TRILOBITE_ROOT_HELPER_RUN_LS:
		return trilobite_root_helper_run_program (root_helper, "ls", argv, fd);
	default:
		g_warning ("unknown TrilobiteRootHelper command: 0x%02X", command);
		return TRILOBITE_ROOT_HELPER_BAD_COMMAND;
	}
}

int
trilobite_root_helper_get_exit_code (TrilobiteRootHelper *root_helper)
{
	int status = 0;

	g_return_val_if_fail (root_helper != NULL, -1);
	g_return_val_if_fail (TRILOBITE_IS_ROOT_HELPER (root_helper), -1);

	if (root_helper->pid) {
		trilobite_debug (">>> waiting on pid %d", root_helper->pid);
		while ((waitpid (root_helper->pid, &status, 0) == -1) && (errno == EINTR))
			;
		root_helper->pid = 0;
		trilobite_debug (">>> status = %d", status);
		if (WIFEXITED (status)) {
			root_helper->child_status = WEXITSTATUS (status);
		} else {
			root_helper->child_status = -1;
		}
	}

	trilobite_debug (">>> returning %d", root_helper->child_status);
	return root_helper->child_status;
}
