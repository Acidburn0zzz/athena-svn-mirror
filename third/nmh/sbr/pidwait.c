
/*
 * pidwait.c -- wait for child to exit
 *
 * $Id: pidwait.c,v 1.2 1999-05-11 18:10:58 danw Exp $
 */

#include <h/mh.h>
#include <h/signals.h>
#include <signal.h>

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

int
pidwait (pid_t id, int sigsok)
{
    pid_t pid;
    sigset_t set, oset;
    SIGNAL_HANDLER istat, qstat;

#ifdef WAITINT
    int status;
#else
    union wait status;
#endif

    if (sigsok == -1) {
	/* ignore a couple of signals */
	istat = SIGNAL (SIGINT, SIG_IGN);
	qstat = SIGNAL (SIGQUIT, SIG_IGN);
    }

#ifdef HAVE_WAITPID
    pid = waitpid(id, &status, 0);
#else
    while ((pid = wait(&status)) != -1 && pid != id)
	continue;
#endif

    if (sigsok == -1) {
	/* reset the signal handlers */
	SIGNAL (SIGINT, istat);
	SIGNAL (SIGQUIT, qstat);
    }

#ifdef WAITINT
    return (pid == -1 ? -1 : status);
#else
    return (pid == -1 ? -1 : status.w_status);
#endif
}
