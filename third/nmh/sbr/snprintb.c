
/*
 * snprintb.c -- snprintf a %b string
 *
 * $Id: snprintb.c,v 1.1.1.1 1999-02-07 18:14:10 danw Exp $
 */

#include <h/mh.h>


char *
snprintb (char *buffer, size_t n, unsigned v, char *bits)
{
    register int i, j;
    register char c, *bp;

    snprintf (buffer, n, bits && *bits == 010 ? "0%o" : "0x%x", v);
    bp = buffer + strlen(buffer);

    if (bits && *++bits) {
	j = 0;
	*bp++ = '<';
	while ((i = *bits++))
	    if (v & (1 << (i - 1))) {
		if (j++)
		    *bp++ = ',';
		for (; (c = *bits) > 32; bits++)
		    *bp++ = c;
	    }
	    else
		for (; *bits > 32; bits++)
		    continue;
	*bp++ = '>';
	*bp = 0;
    }

    return buffer;
}
