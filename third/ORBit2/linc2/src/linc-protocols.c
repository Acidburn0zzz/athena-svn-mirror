/*
 * linc-protocols.c: This file is part of the linc library.
 *
 * Authors:
 *    Elliot Lee     (sopwith@redhat.com)
 *    Michael Meeks  (michael@ximian.com)
 *    Mark McLouglin (mark@skynet.ie) & others
 *
 * Copyright 2001, Red Hat, Inc., Ximian, Inc.,
 *                 Sun Microsystems, Inc.
 */
#include <config.h>
#include "linc-compat.h"
#include <dirent.h>
#include <linc/linc-protocol.h>
#include <linc/linc-connection.h>

#include "linc-private.h"

#undef LOCAL_DEBUG

static char *link_tmpdir = NULL;

/*
 * make_local_tmpdir:
 * @dirname: directory name.
 *
 * Create a directory with the name in @dirname. Also, clear the
 * access and modification times of @dirname.
 *
 * If the directory already exists and is not owned by the current 
 * user, or is not solely readable by the current user, then linc
 * will error out.
 */
static void
make_local_tmpdir (const char *dirname)
{
	struct stat statbuf;
		
	if (mkdir (dirname, 0700) != 0) {
		int e = errno;
			
		switch (e) {
		case 0:
		case EEXIST:
			if (stat (dirname, &statbuf) != 0)
				g_error ("Can not stat %s\n", dirname);

#ifndef __CYGWIN__
			if (statbuf.st_uid != getuid ())
				g_error ("Owner of %s is not the current user\n", dirname);

			if ((statbuf.st_mode & (S_IRWXG|S_IRWXO)) ||
			    !S_ISDIR (statbuf.st_mode))
				g_error ("Wrong permissions for %s\n", dirname);
#endif

			break;
				
		default:
			g_error("Unknown error on directory creation of %s (%s)\n",
				dirname, g_strerror (e));
		}
	}

	{ /* Hide some information ( apparently ) */
		struct utimbuf utb;
		memset (&utb, 0, sizeof (utb));
		utime (dirname, &utb);
	}
}

/**
 * link_set_tmpdir:
 * @dir: directory name.
 *
 * Set the temporary directory used by linc to @dir. 
 *
 * This directory is used for the creation of UNIX sockets.
 * @dir must have the correct permissions, 0700, user owned
 * otherwise this method will g_error.
 **/
void
link_set_tmpdir (const char *dir)
{
	g_free (link_tmpdir);
	link_tmpdir = g_strdup (dir);

	make_local_tmpdir (link_tmpdir);
}

/**
 * link_get_tmpdir:
 * @void: 
 * 
 * Fetches the directory name used by linc to whack
 * Unix Domain sockets into.
 * 
 * Return value: the g_allocated socket name.
 **/
char *
link_get_tmpdir (void)
{
	return g_strdup (link_tmpdir ? link_tmpdir : "");
}

#ifdef HAVE_SOCKADDR_SA_LEN
#define LINK_SET_SOCKADDR_LEN(saddr, len)                     \
		((struct sockaddr *)(saddr))->sa_len = (len)
#else 
#define LINK_SET_SOCKADDR_LEN(saddr, len)
#endif

#if defined(HAVE_RESOLV_H) && defined(AF_INET6) && defined(RES_USE_INET6)
#define LINK_RESOLV_SET_IPV6     _res.options |= RES_USE_INET6
#define LINK_RESOLV_UNSET_IPV6   _res.options &= ~RES_USE_INET6
#else
#define LINK_RESOLV_SET_IPV6
#define LINK_RESOLV_UNSET_IPV6
#endif



#if defined(AF_INET) || defined(AF_INET6) || defined (AF_UNIX)
const char *
link_get_local_hostname (void)
{
	static char local_host[NI_MAXHOST] = { 0 };

	if (local_host [0])
		return local_host;

	if (gethostname (local_host, NI_MAXHOST) == -1)
		return NULL;

	return local_host;
}

/*
 * True if succeeded in mapping, else false.
 */
static gboolean
ipv4_addr_from_addr (struct in_addr *dest_addr,
		     guint8         *src_addr,
		     int             src_length)
{
	if (src_length == 4)
		memcpy (dest_addr, src_addr, 4);

	else if (src_length == 16) {
		int i;

#ifdef LOCAL_DEBUG
		g_warning ("Doing conversion ...");
#endif

		/* An ipv6 address, might be an IPv4 mapped though */
		for (i = 0; i < 10; i++)
			if (src_addr [i] != 0)
				return FALSE;

		if (src_addr [10] != 0xff ||
		    src_addr [11] != 0xff)
			return FALSE;

		memcpy (dest_addr, &src_addr[12], 4);
	} else
		return FALSE;

	return TRUE;
}

static gboolean
link_protocol_is_local_ipv46 (const LinkProtocolInfo *proto,
			      const struct sockaddr   *saddr,
			      LinkSockLen              saddr_len)
{
	static int warned = 0;
#if defined (AF_INET6) && defined (HAVE_GETADDRINFO)
	struct addrinfo hints, *result = NULL;
	static struct addrinfo *local_addr = NULL;
#else
	int i;
	static struct hostent *local_hostent;
#endif

	g_assert (saddr->sa_family == proto->family);

#if defined (AF_INET6) && defined (HAVE_GETADDRINFO) 
	if (!local_addr) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_CANONNAME;

		if (getaddrinfo(link_get_local_hostname(), NULL, &hints, &local_addr) != 0) {
			if (!warned++)
				g_warning ("can't getaddrinfo on '%s'",
					   link_get_local_hostname ());
			return FALSE;
		}
	}

	if (!local_addr->ai_addr) 
		g_error ("No address for local host");

	if (proto->family != AF_INET) {
		if (proto->family == AF_INET6 &&
		    local_addr->ai_family != AF_INET6)
			return FALSE; /* can connect via IPv4 */
		
		if (proto->family == AF_INET6)
			return FALSE;
	}

	for (result = local_addr; result; result = result->ai_next) {
		int af = result->ai_family;

		if ((af != AF_INET6) && (af != AF_INET))
			continue;

		if (proto->family == AF_INET) {
			if (af == AF_INET) {
				if (!memcmp ((struct sockaddr_in *)result->ai_addr,
					     (struct sockaddr_in *)saddr,
					     result->ai_addrlen)) {
#ifdef LOCAL_DEBUG
					g_warning ("local ipv4 address");
#endif
					return TRUE;
				}
			}
		}
		else {
			if (af == AF_INET6) {
				if (!memcmp ((struct sockaddr_in6 *)result->ai_addr,
					     (struct sockaddr_in6 *)saddr,
					     result->ai_addrlen)) {
#ifdef LOCAL_DEBUG
					g_warning ("local ipv6 address");
#endif
					return TRUE;
				}
			}
		}
	}
#ifdef LOCAL_DEBUG
	g_warning ("No match over all");
#endif
	return FALSE;
#else   /*HAVE_GETADDRINFO*/	
	if (!local_hostent) {
		LINK_RESOLV_SET_IPV6;
		local_hostent = gethostbyname (link_get_local_hostname ());
	}

	if (!local_hostent) {
		if (!warned++)
			g_warning ("can't gethostbyname on '%s'",
				   link_get_local_hostname ());
		return FALSE;
	}

	if (!local_hostent->h_addr_list)
		g_error ("No address for local host");

	if (proto->family != AF_INET) {
#ifdef AF_INET6
		if (proto->family == AF_INET6 &&
		    local_hostent->h_addrtype != AF_INET6)
			return FALSE; /* can connect via IPv4 */

		if (proto->family != AF_INET6)
			return FALSE;
#else
		return FALSE;
#endif
	}

	for (i = 0; local_hostent->h_addr_list [i]; i++) {

		if (proto->family == AF_INET) {
			struct in_addr ipv4_addr;
			
			if (!ipv4_addr_from_addr (&ipv4_addr,
						  (guint8 *)local_hostent->h_addr_list [i],
						  local_hostent->h_length))
				continue;

			if (!memcmp (&ipv4_addr, 
				     &((struct sockaddr_in *)saddr)->sin_addr.s_addr, 4)) {
#ifdef LOCAL_DEBUG
				g_warning ("local ipv4 address");
#endif
				return TRUE;
			}

		}
#ifdef AF_INET6
		else if (!memcmp (local_hostent->h_addr_list [i],
				  &((struct sockaddr_in6 *)saddr)->sin6_addr.s6_addr,
				  local_hostent->h_length)) {
#ifdef LOCAL_DEBUG
			g_warning ("local ipv6 address");
#endif
			return TRUE;
		}
#endif
	}

#ifdef LOCAL_DEBUG
	g_warning ("No match over all");
#endif

	return FALSE;
#endif /*HAVE_GETADDRINFO*/
}

#endif

/*
 * link_protocol_get_sockaddr_ipv4:
 * @proto: the #LinkProtocolInfo structure for the IPv4 protocol.
 * @hostname: the hostname.
 * @portnum: the port number.
 * @saddr_len: location in which to store the returned structure's length.
 *
 * Allocates and fills a #sockaddr_in with with the IPv4 address 
 * information.
 *
 * Return Value: a pointer to a valid #sockaddr_in structure if the call 
 *               succeeds, NULL otherwise.
 */
#ifdef AF_INET
static struct sockaddr *
link_protocol_get_sockaddr_ipv4 (const LinkProtocolInfo *proto,
				 const char             *hostname,
				 const char             *portnum,
				 LinkSockLen            *saddr_len)
{
	struct sockaddr_in *saddr;
	struct hostent     *host;

	g_assert (proto->family == AF_INET);
	g_assert (hostname);

	if (!portnum)
		portnum = "0";

	saddr = g_new0 (struct sockaddr_in, 1);

	*saddr_len = sizeof (struct sockaddr_in);

	LINK_SET_SOCKADDR_LEN (saddr, sizeof (struct sockaddr_in));

	saddr->sin_family = AF_INET;
	saddr->sin_port   = htons (atoi (portnum));

	if ((saddr->sin_addr.s_addr = inet_addr (hostname)) == INADDR_NONE) {
	        int i;

		LINK_RESOLV_UNSET_IPV6;
#ifdef HAVE_RESOLV_H
		if (!(_res.options & RES_INIT))
			res_init();
#endif
		
		host = gethostbyname (hostname);
		if (!host) {
		  g_free (saddr);
		  return NULL;
		}

		for(i = 0; host->h_addr_list[i]; i++)
		    if(ipv4_addr_from_addr (&saddr->sin_addr,
					    (guint8 *)host->h_addr_list [i],
					    host->h_length))
		      break;

		if(!host->h_addr_list[i]) {
		  g_free (saddr);
		  return NULL;
		}
	}

	return (struct sockaddr *) saddr;
}
#endif /* AF_INET */

/*
 * link_protocol_get_sockaddr_ipv6:
 * @proto: the #LinkProtocolInfo structure for the IPv6 protocol.
 * @hostname: the hostname.
 * @portnum: the port number
 * @saddr_len: location in which to store the returned structure's length.
 *
 * Allocates and fills a #sockaddr_in6 with with the IPv6 address 
 * information.
 *
 * NOTE: This function is untested.
 *
 * Return Value: a pointer to a valid #sockaddr_in6 structure if the call 
 *               succeeds, NULL otherwise.
 */
#ifdef AF_INET6
static struct sockaddr *
link_protocol_get_sockaddr_ipv6 (const LinkProtocolInfo *proto,
				 const char             *hostname,
				 const char             *portnum,
				 LinkSockLen            *saddr_len)
{
	struct sockaddr_in6 *saddr;
#ifdef HAVE_GETADDRINFO
	struct addrinfo     *host, hints, *result = NULL;
#else
	struct hostent	    *host;

#endif
	g_assert (proto->family == AF_INET6);
	g_assert (hostname);

	if (!portnum)
		portnum = "0";

	saddr = g_new0 (struct sockaddr_in6, 1);

	*saddr_len = sizeof (struct sockaddr_in6);

	LINK_SET_SOCKADDR_LEN (saddr, sizeof (struct sockaddr_in6));

	saddr->sin6_family = AF_INET6;
	saddr->sin6_port = htons (atoi (portnum));
#ifdef HAVE_INET_PTON
	if (inet_pton (AF_INET6, hostname, &saddr->sin6_addr) > 0)
		return (struct sockaddr *)saddr;
#endif
#ifdef HAVE_GETADDRINFO
	memset (&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo (hostname, NULL, &hints, &result))
		return NULL;

	for (host = result; host; host = host->ai_next) {
		if (host->ai_family == AF_INET6) 
			break;
	}
	if (!host) {
		g_free (saddr);
		freeaddrinfo (result);
		return NULL;
	}
	memcpy (&saddr->sin6_addr, &((struct sockaddr_in6 *)host->ai_addr)->sin6_addr, sizeof (struct in6_addr));
	freeaddrinfo (result);
	
	return (struct sockaddr *)saddr;
#else

#ifdef HAVE_RESOLV_H
	if (!(_res.options & RES_INIT))
		res_init();
#endif

	LINK_RESOLV_SET_IPV6;
	host = gethostbyname (hostname);
	if (!host || host->h_addrtype != AF_INET6) {
		g_free (saddr);
		return NULL;
	}

	memcpy (&saddr->sin6_addr, host->h_addr_list[0], sizeof (struct in6_addr));

	return (struct sockaddr *)saddr;
#endif /* HAVE_GETADDRINFO */
}
#endif /* AF_INET6 */

#ifdef AF_UNIX
/*
 * link_protocol_get_sockaddr_unix:
 * @proto: the #LinkProtocolInfo structure for the UNIX sockets protocol.
 * @dummy: not used.
 * @path: the path name of the UNIX socket.
 * @saddr_len: location in which to store the returned structure's length.
 *
 * Allocates and fills a #sockaddr_un with with the UNIX socket address 
 * information.
 *
 * If @path is NULL, a new, unique path name will be generated.
 *
 * Return Value: a pointer to a valid #sockaddr_un structure if the call 
 *               succeeds, NULL otherwise.
 */
static struct sockaddr *
link_protocol_get_sockaddr_unix (const LinkProtocolInfo *proto,
				 const char             *dummy,
				 const char             *path,
				 LinkSockLen            *saddr_len)
{
	struct sockaddr_un *saddr;
	int                 pathlen;
	char                buf[LINK_UNIX_PATH_MAX], *actual_path;

	g_assert (proto->family == AF_UNIX);

	if (!path) {
		struct timeval t;
		static guint pid = 0, idx = 0;

		if (!pid)
			pid = getpid ();

		gettimeofday (&t, NULL);
		g_snprintf (buf, sizeof (buf),
			    "%s/linc-%x-%x-%x%x",
			    link_tmpdir ? link_tmpdir : "",
			    pid, idx,
			    (guint) (rand() ^ t.tv_sec),
			    (guint) (idx ^ t.tv_usec));
		idx++;
#ifdef CONNECTION_DEBUG
		if (g_file_test (buf, G_FILE_TEST_EXISTS))
			g_warning ("'%s' already exists !", buf);
#endif
		actual_path = buf;
	} else 
		actual_path = (char *)path;

	pathlen = strlen (actual_path) + 1;

	if (pathlen > sizeof (saddr->sun_path))
		return NULL;

	saddr = g_new0 (struct sockaddr_un, 1);

	*saddr_len = sizeof (struct sockaddr_un) - sizeof (saddr->sun_path) + pathlen;

	LINK_SET_SOCKADDR_LEN (saddr, *saddr_len);

	saddr->sun_family =  AF_UNIX;
	strncpy (saddr->sun_path, actual_path, sizeof (saddr->sun_path) - 1);
	saddr->sun_path[sizeof (saddr->sun_path) - 1] = '\0';

	return (struct sockaddr *)saddr;
}
#endif /* AF_UNIX */

/*
 * link_protocol_get_sockaddr:
 * @proto: a #LinkProtocolInfo structure.
 * @hostname: protocol dependant host information.
 * @service: protocol dependant service information.
 * @saddr_len: location in which to store the returned structure's length.
 *
 * Allocates, fills in and returns the #sockaddr structure appropriate
 * for the supplied protocol, @proto.
 *
 * Return Value: a pointer to a valid #sockaddr structure if the call 
 *               succeeds, NULL otherwise.
 */
struct sockaddr *
link_protocol_get_sockaddr (const LinkProtocolInfo *proto,
			    const char             *hostname,
			    const char             *service,
			    LinkSockLen            *saddr_len)		   
{
	if (proto && proto->get_sockaddr)
		return proto->get_sockaddr (proto, hostname, service, saddr_len);

	return NULL;
}

/*
 * link_protocol_get_sockinfo_ipv46:
 * @host: char pointer describing the hostname.
 * @port: the portnumber.
 * @hostname: pointer by which the hostname string is returned.
 * @portnum: pointer by which the port number string is returned.
 *
 * Generates two strings, returned through @hostname and @portnum, corresponding
 * to @host and @port. On return @hostname should contain the canonical hostname 
 * of the host and @portnum should contain the port number string.
 *
 * If @host is NULL, the local host name is used.
 *
 * Note: both @hostname and @service are allocated on the heap and should be
 *       freed using g_free().
 *
 * Return Value: #TRUE if the function succeeds, #FALSE otherwise.
 */
static gboolean
link_protocol_get_sockinfo_ipv46 (const char      *host,
				  guint            port,
				  gchar          **hostname,
				  char           **portnum)
{
	if (!host)  
		if (!(host = link_get_local_hostname ()))
			return FALSE;
	
	if (hostname)
		*hostname = g_strdup (host);

	if (portnum) {
		gchar tmpport[NI_MAXSERV];

		g_snprintf (tmpport, sizeof (tmpport), "%d", ntohs (port));

		*portnum = g_strdup (tmpport);
	}

	return TRUE;
}

/*
 * link_protocol_get_sockinfo_ipv4:
 * @proto: the #LinkProtocolInfo structure for the IPv4 protocol.
 * @sockaddr: a #sockaddr_in structure desribing the socket.
 * @hostname: pointer by which the hostname string is returned.
 * @portnum: pointer by which the port number string is returned.
 *
 * Generates two strings, returned through @hostname and @portnum, describing
 * the socket address, @sockaddr. On return @hostname should contain the 
 * canonical hostname of the host described in @sockaddr and @portnum should
 * contain the port number of the socket described in @sockaddr.
 *
 * Note: both @hostname and @service are allocated on the heap and should be
 *       freed using g_free().
 *
 * Return Value: #TRUE if the function succeeds, #FALSE otherwise.
 */
#ifdef AF_INET
static gboolean
link_protocol_get_sockinfo_ipv4 (const LinkProtocolInfo  *proto,
				 const struct sockaddr   *saddr,
				 gchar                  **hostname,
				 gchar                  **portnum)
{
	struct sockaddr_in *sa_in = (struct sockaddr_in  *)saddr;
	struct hostent     *host = NULL;
	char *hname = NULL;

	g_assert (proto && saddr && saddr->sa_family == AF_INET);

	if (sa_in->sin_addr.s_addr != INADDR_ANY) {
		host = gethostbyaddr ((char *)&sa_in->sin_addr, 
                                      sizeof (struct in_addr), AF_INET);
		if (!host)
			return FALSE;
		else
			hname = host->h_name;
	}

	return link_protocol_get_sockinfo_ipv46 (hname, sa_in->sin_port, 
						 hostname, portnum);
}
#endif /* AF_INET */

/*
 * link_protocol_get_sockinfo_ipv6:
 * @proto: the #LinkProtocolInfo structure for the IPv6 protocol.
 * @sockaddr: a #sockaddr_in structure desribing the socket.
 * @hostname: pointer by which the hostname string is returned.
 * @portnum: pointer by which the port number string is returned.
 *
 * Generates two strings, returned through @hostname and @portnum, describing
 * the socket address, @sockaddr. On return @hostname should contain the 
 * canonical hostname of the host described in @sockaddr and @portnum should
 * contain the port number of the socket described in @sockaddr.
 *
 * Note: both @hostname and @service are allocated on the heap and should be
 *       freed using g_free().
 *
 * Return Value: #TRUE if the function succeeds, #FALSE otherwise.
 */
#ifdef AF_INET6

/*
 *   We need some explicit check for Macs here - OSF1 does this
 * right, and does not use a #define; so the Mac gets to break for
 * now, until someone sends me a patch.
 */
#ifdef MAC_OS_X_IS_SO_BROKEN
/* FIXME: is IN6ADDR_ANY_INIT exported on Mac OS X ? */
/* on Mac OS X 10.1 inaddr6_any isn't exported by libc */
#  ifndef in6addr_any
	static const struct in6_addr in6addr_any = { { { 0 } } };
#  endif
#endif

static gboolean
link_protocol_get_sockinfo_ipv6 (const LinkProtocolInfo  *proto,
				 const struct sockaddr   *saddr,
				 gchar                  **hostname,
				 gchar                  **portnum)
{
	struct sockaddr_in6 *sa_in6 = (struct sockaddr_in6 *)saddr;
#ifdef HAVE_GETNAMEINFO
	char hbuf[NI_MAXHOST];
#else
	struct hostent      *host = NULL;
#endif
	char *hname = NULL;

	g_assert (proto && saddr && saddr->sa_family == AF_INET6);

	if (memcmp (&sa_in6->sin6_addr, &in6addr_any, sizeof (struct in6_addr))) {

#ifdef HAVE_GETNAMEINFO
                if (getnameinfo((struct sockaddr *)sa_in6, sizeof(*sa_in6), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD))
			return FALSE;
		else
			hname = hbuf;
	}
#else
		host = gethostbyaddr ((char *)&sa_in6->sin6_addr,
				      sizeof (struct in6_addr), AF_INET6);
		if (!host)
			return FALSE;
		if (host)
			hname = host->h_name;
	}
#endif /* HAVE_GETNAMEINFO */

	return link_protocol_get_sockinfo_ipv46 (hname, sa_in6->sin6_port, 
						 hostname, portnum);
}

#endif /* AF_INET6 */

/*
 * link_protocol_get_sockinfo_unix:
 * @proto: a #LinkProtocolInfo structure.
 * @sockaddr: a #sockaddr_un structure desribing the socket.
 * @hostname: pointer by which the hostname string is returned.
 * @service: pointer by which the sockets pathname string is returned.
 *
 * Generates two strings, returned through @hostname and @sock_path, describing
 * the socket address, @sockaddr. On return @hostname should contain the 
 * canonical hostname of the local host and @sock_path should contain the 
 * path name of the unix socket described in @sockaddr.
 *
 * Note: both @hostname and @sock_path are allocated on the heap and should 
 *       be freed using g_free().
 *
 * Return Value: #TRUE if the function succeeds, #FALSE otherwise.
 */
#ifdef AF_UNIX
static gboolean
link_protocol_get_sockinfo_unix (const LinkProtocolInfo  *proto,
				 const struct sockaddr   *saddr,
				 gchar                  **hostname,
				 gchar                  **sock_path)
{
	struct sockaddr_un *sa_un = (struct sockaddr_un *)saddr;

	g_assert (proto && saddr && saddr->sa_family == AF_UNIX);

	if (hostname) {
		const char *local_host;

		if (!(local_host = link_get_local_hostname ()))
			return FALSE;

		*hostname = g_strdup (local_host);
	}

	if (sock_path)
		*sock_path = g_strdup (sa_un->sun_path);

	return TRUE;
}
#endif /* AF_UNIX */

/*
 * link_protocol_get_sockinfo:
 * @proto: a #LinkProtocolInfo structure.
 * @sockaddr: a #sockadrr structure desribing the socket.
 * @hostname: pointer by which the hostname string is returned.
 * @service: pointer by which the service string is returned.
 *
 * Generates two strings, returned through @hostname and @service, describing
 * the socket address, @sockaddr. On return @hostname should contain the 
 * canonical hostname of the host described in @sockaddr and @service should
 * contain the service descriptor(e.g. port number) of the socket described in 
 * @sockaddr
 *
 * Note: both @hostname and @service are allocated on the heap and should be
 *       freed using g_free().
 *
 * Return Value: #TRUE if the function succeeds, #FALSE otherwise.
 */
gboolean
link_protocol_get_sockinfo (const LinkProtocolInfo  *proto,
			    const struct sockaddr   *saddr,
			    gchar                  **hostname,
			    gchar                  **service)
{
	if (proto && proto->get_sockinfo)
		return proto->get_sockinfo (proto, saddr, hostname, service);

	return FALSE;
}

/**
 * link_protocol_is_local:
 * @proto: the protocol
 * @saddr: the socket address of a connecting client.
 * 
 *   This method determines if the client is from the same
 * machine or not - per protocol.
 * 
 * Return value: TRUE if the connection is local, else FALSE
 **/
gboolean
link_protocol_is_local (const LinkProtocolInfo  *proto,
			const struct sockaddr   *saddr,
			LinkSockLen              saddr_len)
{
	if (proto && proto->is_local)
		return proto->is_local (proto, saddr, saddr_len);

	return FALSE;
}

/*
 * af_unix_destroy:
 * @fd: file descriptor of the socket.
 * @dummy: not used.
 * @pathname: path name of the UNIX socket
 *
 * Removes the UNIX socket file.
 */
#ifdef AF_UNIX
static void
link_protocol_unix_destroy (int         fd,
			    const char *dummy,
			    const char *pathname)
{
	unlink (pathname);
}

static gboolean
link_protocol_unix_is_local (const LinkProtocolInfo *proto,
			     const struct sockaddr   *saddr,
			     LinkSockLen              saddr_len)
{
	return TRUE;
}
#endif /* AF_UNIX */

/*
 * link_protocol_tcp_setup:
 * @fd: file descriptor of the socket.
 * @cnx_flags: a #LinkConnectionOptions value.
 *
 * Sets the TCP_NODELAY option on the TCP socket.
 *
 * Note: this is not applied to SSL TCP sockets.
 */
#if defined(AF_INET) || defined(AF_INET6)
static void
link_protocol_tcp_setup (int                   fd,
			 LinkConnectionOptions cnx_flags)
{
#ifdef TCP_NODELAY
	if (!(cnx_flags & LINK_CONNECTION_SSL)) {
		struct protoent *proto;
		int              on = 1;

		proto = getprotobyname ("tcp");
		if (!proto)
			return;

		setsockopt (fd, proto->p_proto, TCP_NODELAY, 
		            &on, sizeof (on));
	}
#endif
}
#endif /* defined(AF_INET) || defined(AF_INET6) */

static LinkProtocolInfo static_link_protocols[] = {
#if defined(AF_INET)
	{
	"IPv4", 			/* name */
	AF_INET, 			/* family */
	sizeof (struct sockaddr_in), 	/* addr_len */
	IPPROTO_TCP, 			/* stream_proto_num */
	0, 				/* flags */
	link_protocol_tcp_setup, 	/* setup */
	NULL, 				/* destroy */
	link_protocol_get_sockaddr_ipv4,/* get_sockaddr */
	link_protocol_get_sockinfo_ipv4,/* get_sockinfo */
	link_protocol_is_local_ipv46    /* is_local */
	},
#endif
#if defined(AF_INET6)
	{ 
	"IPv6", 			/* name */
	AF_INET6, 			/* family */
	sizeof (struct sockaddr_in6), 	/* addr_len */
	IPPROTO_TCP, 			/* stream_proto_num */
	0, 				/* flags */
	link_protocol_tcp_setup, 	/* setup */
	NULL, 				/* destroy */
	link_protocol_get_sockaddr_ipv6,/* get_sockaddr */
	link_protocol_get_sockinfo_ipv6,/* get_sockinfo */
	link_protocol_is_local_ipv46    /* is_local */
	},
#endif
#ifdef AF_UNIX
	{
	"UNIX", 					/* name */
	AF_UNIX, 					/* family */
	sizeof (struct sockaddr_un), 			/* addr_len */
	0, 						/* stream_proto_num */
	LINK_PROTOCOL_SECURE|LINK_PROTOCOL_NEEDS_BIND, 	/* flags */
	NULL,  						/* setup */
	link_protocol_unix_destroy,  			/* destroy */
	link_protocol_get_sockaddr_unix, 		/* get_sockaddr */
	link_protocol_get_sockinfo_unix, 		/* get_sockinfo */
	link_protocol_unix_is_local                     /* is_local */
	},
#endif
	{ NULL /* name */ }
};

void
link_protocol_destroy_cnx (const LinkProtocolInfo *proto,
			   int                     fd,
			   const char             *host,
			   const char             *service)
{
	g_return_if_fail (proto != NULL);

	if (fd >= 0) {
		if (proto->destroy)
			proto->destroy (fd, host, service);
		
		LINK_CLOSE (fd);
	}
}


void
link_protocol_destroy_addr (const LinkProtocolInfo *proto,
			    int                     fd,
			    struct sockaddr        *saddr)
{
	g_return_if_fail (proto != NULL);

	if (fd >= 0) {
#ifdef AF_UNIX
		if (proto->family == AF_UNIX && proto->destroy) {
			/* We are AF_UNIX - we need the path to unlink */
			struct sockaddr_un *addr_un =
				(struct sockaddr_un *) saddr;
			proto->destroy (fd, NULL, addr_un->sun_path);
		}
#endif
		LINK_CLOSE (fd);
		g_free (saddr);
	}

}

/*
 * link_protocol_all:
 *
 * Returns a list of protocols supported by linc.
 *
 * Note: the list is terminated by a #LinkProtocolInfo with a
 *       NULL name pointer.
 *
 * Return Value: an array of #LinkProtocolInfo structures.
 */
LinkProtocolInfo * const
link_protocol_all (void)
{
	return static_link_protocols;
}

/*
 * link_protocol_find:
 * @name: name of the protocol.
 *
 * Find a protocol identified by @name.
 *
 * Return Value: a pointer to a valid #LinkProtocolInfo structure if 
 *               the protocol is supported by linc, NULL otherwise.
 */
LinkProtocolInfo * const
link_protocol_find (const char *name)
{
	int i;

	for (i = 0; static_link_protocols [i].name; i++) {
		if (!strcmp (name, static_link_protocols [i].name))
			return &static_link_protocols [i];
	}

	return NULL;
}

/*
 * link_protocol_find_num:
 * @family: the family identifier of the protocol - i.e. AF_*
 *
 * Find a protocol identified by @family.
 *
 * Return Value: a pointer to a valid #LinkProtocolInfo structure if
 *               the protocol is supported by linc, NULL otherwise.
 */
LinkProtocolInfo * const
link_protocol_find_num (const int family)
{
	int i;

	for (i = 0; static_link_protocols [i].name; i++) {
		if (family == static_link_protocols [i].family)
			return &static_link_protocols [i];
	}

	return NULL;
}
