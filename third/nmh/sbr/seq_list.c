
/*
 * seq_list.c -- Get all messages in a sequence and return them
 *            -- as a space separated list of message ranges.
 *
 * $Id: seq_list.c,v 1.1.1.1 1999-02-07 18:14:10 danw Exp $
 */

#include <h/mh.h>

/* allocate this much buffer space at a time */
#define MAXBUFFER 1024

/* static buffer to collect the sequence line */
static char *buffer = NULL;
static int len = 0;


char *
seq_list(struct msgs *mp, char *seqname)
{
    int i, j, seqnum;
    char *bp;

    /* On first invocation, allocate initial buffer space */
    if (!buffer) {
	len = MAXBUFFER;
	if (!(buffer = malloc ((size_t) len)))
	    adios (NULL, "unable to malloc storage in seq_list");
    }

    /*
     * Special processing for "cur" sequence.  We assume that the
     * "cur" sequence and mp->curmsg are in sync (see seq_add.c).
     * This is returned, even if message doesn't exist or the
     * folder is empty.
     */
    if (!strcmp (current, seqname)) {
	if (mp->curmsg) {	
	    sprintf(buffer, "%s", m_name(mp->curmsg));
	    return (buffer);
	} else
	    return (NULL);
    }

    /* If the folder is empty, just return NULL */
    if (mp->nummsg == 0)
	return NULL;

    /* Get the index of the sequence */
    if ((seqnum = seq_getnum (mp, seqname)) == -1)
	return NULL;

    bp = buffer;

    for (i = mp->lowmsg; i <= mp->hghmsg; ++i) {
	/*
	 * If message doesn't exist, or isn't in
	 * the sequence, then continue.
	 */
	if (!does_exist(mp, i) || !in_sequence(mp, seqnum, i))
	    continue;

	/*
	 * See if we need to enlarge buffer.  Since we don't know
	 * exactly how many character this particular message range
	 * will need, we enlarge the buffer if we are within
	 * 50 characters of the end.
	 */
	if (bp - buffer > len - 50) {
	    char *newbuf;

	    len += MAXBUFFER;
	    if (!(newbuf = realloc (buffer, (size_t) len)))
		adios (NULL, "unable to realloc storage in seq_list");
	    bp = newbuf + (bp - buffer);
	    buffer = newbuf;
	}

	/*
	 * If this is not the first message range in
	 * the list, first add a space.
	 */
	if (bp > buffer)
	    *bp++ = ' ';

	sprintf(bp, "%s", m_name(i));
	bp += strlen(bp);
	j = i;			/* Remember beginning of message range */

	/*
	 * Scan to the end of this message range
	 */
	for (++i; i <= mp->hghmsg && does_exist(mp, i) && in_sequence(mp, seqnum, i);
	     ++i)
	    ;

	if (i - j > 1) {
	    sprintf(bp, "-%s", m_name(i - 1));
	    bp += strlen(bp);
	}
    }
    return (bp > buffer? buffer : NULL);
}
