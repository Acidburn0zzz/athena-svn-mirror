
/*
 * check_charset.c -- routines for character sets
 *
 * $Id: check_charset.c,v 1.1.1.1 1999-02-07 18:14:07 danw Exp $
 */

#include <h/mh.h>

/*
 * Check if we can display a given character set natively.
 * We are passed the length of the initial part of the
 * string to check, since we want to allow the name of the
 * character set to be a substring of a larger string.
 */

int
check_charset (char *str, int len) 
{
    static char *mm_charset = NULL;
    static char *alt_charset = NULL;
    static int mm_len;
    static int alt_len;

    /* Cache the name of our default character set */
    if (!mm_charset) {
	if (!(mm_charset = getenv ("MM_CHARSET")))
	    mm_charset = "US-ASCII";
	mm_len = strlen (mm_charset);

	/* US-ASCII is a subset of the ISO-8859-X character sets */
	if (!strncasecmp("ISO-8859-", mm_charset, 9)) {
	    alt_charset = "US-ASCII";
	    alt_len = strlen (alt_charset);
	}
    }

    /* Check if character set is OK */
    if ((len == mm_len) && !strncasecmp(str, mm_charset, mm_len))
	return 1;
    if (alt_charset && (len == alt_len) && !strncasecmp(str, alt_charset, alt_len))
	return 1;

    return 0;
}


/*
 * Return the name of the character set we are
 * using for 8bit text.
 */
char *
write_charset_8bit (void)
{
    static char *mm_charset = NULL;

    /*
     * Cache the name of the character set to
     * use for 8bit text.
     */
    if (!mm_charset && !(mm_charset = getenv ("MM_CHARSET")))
	    mm_charset = "x-unknown";

    return mm_charset;
}
