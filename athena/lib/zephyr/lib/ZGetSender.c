/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetSender.c function.
 *
 *	Created by:	Robert French
 *
 *	$Id: ZGetSender.c,v 1.14 1999-01-22 23:19:11 ghudson Exp $
 *
 *	Copyright (c) 1987, 1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <internal.h>

#ifndef lint
static const char rcsid_ZGetSender_c[] =
    "$Id: ZGetSender.c,v 1.14 1999-01-22 23:19:11 ghudson Exp $";
#endif

#include <pwd.h>

char *ZGetSender()
{
    struct passwd *pw;
#ifdef HAVE_KRB4
    char pname[ANAME_SZ], pinst[INST_SZ], prealm[REALM_SZ];
    static char sender[ANAME_SZ+INST_SZ+REALM_SZ+3] = "";
#else
    static char sender[128] = "";
#endif

    /* Return it if already cached */
    if (*sender)
	return (sender);

#ifdef HAVE_KRB4
    if (krb_get_tf_fullname((char *)TKT_FILE, pname, pinst, prealm) == KSUCCESS)
    {
	(void) sprintf(sender, "%s%s%s@%s", pname, (pinst[0]?".":""),
		       pinst, prealm);
	return (sender);
    }
#endif

    /* XXX a uid_t is a u_short (now),  but getpwuid
     * wants an int. AARGH! */
    pw = getpwuid((int) getuid());
    if (!pw)
	return ("unknown");
    (void) sprintf(sender, "%s@%s", pw->pw_name, __Zephyr_realm);
    return (sender);
}
