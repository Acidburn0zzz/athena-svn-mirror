/*
 * $Source: /afs/dev.mit.edu/source/repository/athena/bin/delete/lsdel.h,v $
 * $Author: jik $
 * $Header: /afs/dev.mit.edu/source/repository/athena/bin/delete/lsdel.h,v 1.2 1989-02-01 03:43:00 jik Exp $
 * 
 * This file is part of a package including delete, undelete,
 * lsdel, expunge and purge.  The software suite is meant as a
 * replacement for rm which allows for file recovery.
 * 
 * Copyright (c) 1989 by the Massachusetts Institute of Technology.
 * For copying and distribution information, see the file "mit-copyright.h."
 */

#define ERROR_MASK 1
#define NO_DELETE_MASK 2

char **get_the_files();
