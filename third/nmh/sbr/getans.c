
/*
 * getans.c -- get an answer from the user and return a string array
 *
 * $Id: getans.c,v 1.1.1.1 1999-02-07 18:14:08 danw Exp $
 */

#include <h/mh.h>
#include <h/signals.h>
#include <setjmp.h>
#include <signal.h>

static char ansbuf[BUFSIZ];
static jmp_buf sigenv;

/*
 * static prototypes
 */
static RETSIGTYPE intrser (int);


char **
getans (char *prompt, struct swit *ansp)
{
    int i;
    SIGNAL_HANDLER istat;
    char *cp, **cpp;

    if (!(setjmp (sigenv))) {
	istat = SIGNAL (SIGINT, intrser);
    } else {
	SIGNAL (SIGINT, istat);
	return NULL;
    }

    for (;;) {
	printf ("%s", prompt);
	fflush (stdout);
	cp = ansbuf;
	while ((i = getchar ()) != '\n') {
	    if (i == EOF)
		longjmp (sigenv, 1);
	    if (cp < &ansbuf[sizeof ansbuf - 1])
		*cp++ = i;
	}
	*cp = '\0';
	if (ansbuf[0] == '?' || cp == ansbuf) {
	    printf ("Options are:\n");
	    print_sw (ALL, ansp, "");
	    continue;
	}
	cpp = brkstring (ansbuf, " ", NULL);
	switch (smatch (*cpp, ansp)) {
	    case AMBIGSW: 
		ambigsw (*cpp, ansp);
		continue;
	    case UNKWNSW: 
		printf (" -%s unknown. Hit <CR> for help.\n", *cpp);
		continue;
	    default: 
		SIGNAL (SIGINT, istat);
		return cpp;
	}
    }
}


static RETSIGTYPE
intrser (int i)
{
    /*
     * should this be siglongjmp?
     */
    longjmp (sigenv, 1);
}
