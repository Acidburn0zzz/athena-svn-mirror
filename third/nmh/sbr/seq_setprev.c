
/*
 * seq_setprev.c -- set the Previous-Sequence
 *
 * $Id: seq_setprev.c,v 1.1.1.1 1999-02-07 18:14:10 danw Exp $
 */

#include <h/mh.h>

/*
 * Add all the messages currently SELECTED to
 * the Previous-Sequence.  This way, when the next
 * command is given, there is a convenient way to
 * selected all the messages used in the previous
 * command.
 */

void
seq_setprev (struct msgs *mp)
{
    char **ap, *cp, *dp;

    /*
     * Get the list of sequences for Previous-Sequence
     * and split them.
     */
    if ((cp = context_find (psequence))) {
	dp = getcpy (cp);
	if (!(ap = brkstring (dp, " ", "\n")) || !*ap) {
	    free (dp);
	    return;
	}
    } else {
	return;
    }

    /* Now add all SELECTED messages to each sequence */
    for (; *ap; ap++)
	seq_addsel (mp, *ap, -1, 1);

    free (dp);
}
