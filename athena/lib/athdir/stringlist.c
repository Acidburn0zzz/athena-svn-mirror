/* Copyright 1998 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

/* This file implements basic string array functions and structures
 * to be used by software using the athdir library.
 */

static const char rcsid[] = "$Id: stringlist.c,v 1.2 1999-10-23 19:28:46 danw Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringlist.h"

#define CHUNKSIZE 10

/* athdir__add_string
 *   Add a copy of "string" to "list," and to the front of "list" if
 *   "front" is false. If "list" is NULL, initialize it as well.
 */
int athdir__add_string(string_list **list, char *string, int front)
{
  char *newstring;
  int index;

  /* To add nothing, do nothing. */
  if (string == NULL)
    return 0;

  if (list == NULL)
    return -1;

 /* Initialize the structure if it's NULL. */
  if (*list == NULL)
    {
      *list = malloc(sizeof(string_list));

      if (*list == NULL)
	return -1;

      (*list)->strings = malloc(CHUNKSIZE * sizeof(char *));
      if ((*list)->strings == NULL)
	{
	  free(*list);
	  *list = NULL;
	  return -1;
	}

      (*list)->alloced = CHUNKSIZE;
      (*list)->length = 0;
      (*list)->strings[0] = NULL;
    }

  /* Allocate more list space if the list is full. */
  if ((*list)->alloced == (*list)->length + 1) /* must be NULL terminated */
    {
      (*list)->strings = realloc((*list)->strings,
			      ((*list)->alloced + CHUNKSIZE) * sizeof(char *));
      if ((*list)->strings == NULL)
	{
	  free(*list);
	  return -1;
	}
      (*list)->alloced += CHUNKSIZE;
    }

  /* Make our own copy of the string. */
  newstring = malloc(strlen(string) + 1);
  if (newstring == NULL)
    return -1;
  strcpy(newstring, string);

  /* Add the string to the list. */
  if (front)
    {
      (*list)->length++;
      for (index = (*list)->length; index > 0; index--)
	(*list)->strings[index] = (*list)->strings[index - 1];
      (*list)->strings[0] = newstring;
    }
  else
    {
      (*list)->strings[(*list)->length] = newstring;
      (*list)->length++;
      (*list)->strings[(*list)->length] = NULL;
    }

  return 0;
}

/* athdir__make_string_array
 *   Returns a NULL terminated array of strings (or NULL if the list
 *   is empty) while freeing the surrounding structure. free_string_array
 *   the array when finished.
 */
char **athdir__make_string_array(string_list **list)
{
  char **strings = NULL;

  if (list != NULL && *list != NULL)
    {
      if ((*list)->strings[0] != NULL)
	strings = (*list)->strings;
      else
	free((*list)->strings);

      free(*list);
      *list = NULL;
    }

  return strings;
}

/* athdir__free_string_array
 *   Frees a string array generated by athdir__make_string_array.
 */
void athdir__free_string_array(char **array)
{
  char **ptr;

  if (array != NULL)
    {
      for (ptr = array; *ptr != NULL; ptr++)
	free(*ptr);

      free(array);
    }
}

/* athdir__parse_string
 *   Parse "string" using "sep" as a separator into separate strings, adding
 *   adding each substring to the end of "list." Nondestructive to string,
 *   doesn't point to string.
 */
int athdir__parse_string(string_list **list, char *string, char sep)
{
  char *ptr, *sep_ptr;
  char *value;
  int length;

  if (string != NULL)
    {
      /* Allocate a buffer big enough for any substring of string. */
      value = malloc(strlen(string) + 1);
      if (value == NULL)
	return -1;

      ptr = string;
      while (*ptr != '\0')
	{
	  /* Figure out the length of this substring. */
	  sep_ptr = strchr(ptr, sep);
	  if (sep_ptr == NULL)
	    length = strlen(ptr);
	  else
	    length = sep_ptr - ptr;

	  /* Make a copy to be copied by athdir__add_string. */
	  strncpy(value, ptr, length);
	  value[length] = '\0';

	  if (athdir__add_string(list, value, 0))
	    {
	      free(value);
	      return -1;
	    }

	  ptr += length;
	  if (*ptr == sep)
	    ptr++;
	}

      free(value);
    }

  return 0;
}
