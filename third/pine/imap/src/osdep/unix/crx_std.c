/*
 * Program:	Exclusive create of a file
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	17 December 1999
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Exclusive create of a file
 * Accepts: file name
 * Return: T if success, NIL if failed, -1 if retry
 */

long crexcl (char *name)
{
  int i;
				/* try to get the lock */
  if ((i = open (name,O_WRONLY|O_CREAT|O_EXCL,(int) lock_protection)) < 0)
    return (errno == EEXIST) ? -1 : NIL;
  close (i);			/* make the file, now close it */
  return LONGT;			/* success */
}
