
/*
 * rmm.c -- remove a message(s)
 *
 * $Id: rmm.c,v 1.1.1.1 1999-02-07 18:14:16 danw Exp $
 */

#include <h/mh.h>

/*
 * We allocate space for message names and ranges
 * (msgs array) this number of elements at a time.
 */
#define MAXMSGS  256

static struct swit switches[] = {
#define UNLINKSW      0
    { "unlink", 0 },
#define NUNLINKSW    1
    { "nounlink", 0 },
#define VERSIONSW     2
    { "version", 0 },
#define	HELPSW        3
    { "help", 4 },
    { NULL, 0 }
};


int
main (int argc, char **argv)
{
    int nummsgs, maxmsgs, msgnum, unlink_msgs = 0;
    char *cp, *maildir, *folder = NULL;
    char buf[BUFSIZ], **argp;
    char **arguments, **msgs;
    struct msgs *mp;

#ifdef LOCALE
    setlocale(LC_ALL, "");
#endif
    invo_name = r1bindex (argv[0], '/');

    /* read user profile/context */
    context_read();

    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    /*
     * Allocate the initial space to record message
     * names and ranges.
     */
    nummsgs = 0;
    maxmsgs = MAXMSGS;
    if (!(msgs = (char **) malloc ((size_t) (maxmsgs * sizeof(*msgs)))))
	adios (NULL, "unable to allocate storage");

    /* parse arguments */
    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
	    case AMBIGSW: 
		ambigsw (cp, switches);
		done (1);
	    case UNKWNSW: 
		adios (NULL, "-%s unknown\n", cp);

	    case HELPSW: 
		snprintf (buf, sizeof(buf), "%s [+folder] [msgs] [switches]",
			  invo_name);
		print_help (buf, switches, 1);
		done (1);
	    case VERSIONSW:
		print_version(invo_name);
		done (1);

	    case UNLINKSW:
		unlink_msgs++;
		continue;
	    case NUNLINKSW:
		unlink_msgs = 0;
		continue;
	    }
	}
	if (*cp == '+' || *cp == '@') {
	    if (folder)
		adios (NULL, "only one folder at a time!");
	    else
		folder = path (cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	} else {
	    /*
	     * Check if we need to allocate more space
	     * for message names/ranges.
	     */
	    if (nummsgs >= maxmsgs){
		maxmsgs += MAXMSGS;
		if (!(msgs = (char **) realloc (msgs,
			     (size_t) (maxmsgs * sizeof(*msgs)))))
		    adios (NULL, "unable to reallocate msgs storage");
	    }
	    msgs[nummsgs++] = cp;
	}
    }

    if (!context_find ("path"))
	free (path ("./", TFOLDER));
    if (!nummsgs)
	msgs[nummsgs++] = "cur";
    if (!folder)
	folder = getfolder (1);
    maildir = m_maildir (folder);

    if (chdir (maildir) == NOTOK)
	adios (maildir, "unable to change directory to");

    /* read folder and create message structure */
    if (!(mp = folder_read (folder)))
	adios (NULL, "unable to read folder %s", folder);

    /* check for empty folder */
    if (mp->nummsg == 0)
	adios (NULL, "no messages in %s", folder);

    /* parse all the message ranges/sequences and set SELECTED */
    for (msgnum = 0; msgnum < nummsgs; msgnum++)
	if (!m_convert (mp, msgs[msgnum]))
	    done (1);
    seq_setprev (mp);		/* set the previous-sequence      */

    /*
     * This is hackish.  If we are using a external rmmproc,
     * then we need to update the current folder in the
     * context so the external rmmproc will remove files
     * from the correct directory.  This should be moved to
     * folder_delmsgs().
     */
    if (rmmproc) {
	context_replace (pfolder, folder);
	context_save ();
	fflush (stdout);
    }

    /* "remove" the SELECTED messages */
    folder_delmsgs (mp, unlink_msgs);

    seq_save (mp);		/* synchronize message sequences  */
    context_replace (pfolder, folder);	/* update current folder   */
    context_save ();			/* save the context file   */
    folder_free (mp);			/* free folder structure   */
    done (0);
}
