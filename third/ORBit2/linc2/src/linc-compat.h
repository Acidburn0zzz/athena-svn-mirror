/*
 * Herein lies a set of private ugly portability
 * hacks for the mind-numbingly broken Unix like
 * things that exist out there.
 */
#ifndef LINK_HACKS_H
#define LINK_HACKS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_LINUX_IRDA_H
#include <asm/types.h>
#include <linux/irda.h>
#endif

#include <arpa/inet.h>
#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif

#if !defined (NI_MAXSERV) || !defined (NI_MAXHOST)
#  include <netdb.h>
#  include <sys/param.h>
#endif

#if !defined (NI_MAXHOST)
#  define NI_MAXHOST MAXHOSTNAMELEN
#endif

#if !defined (NI_MAXSERV)
#  define NI_MAXSERV 64
#endif

#if !defined (INADDR_NONE)
#  define INADDR_NONE (-1)
#endif

#if !defined (UNIX_PATH_MAX)
/* UNP: 14.2 - Posix.1g at least 100 bytes */
#  define LINK_UNIX_PATH_MAX 100
#else
#  define LINK_UNIX_PATH_MAX UNIX_PATH_MAX
#endif

#endif /* LINK_HACKS_H */
