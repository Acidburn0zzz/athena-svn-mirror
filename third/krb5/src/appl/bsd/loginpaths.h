/* here are actual path values from each operating system supported. */
/* LPATH is from rlogin, for login.c; RPATH is from rsh, for rshd.c */

#define APATH "/usr/athena/bin:/bin/athena:"

#ifdef sun
#ifdef __SVR4
#define RPATH APATH "/usr/bin:/usr/sbin:/usr/openwin/bin:/usr/ccs/bin:/usr/ucb"
#define LPATH APATH "/usr/bin:/usr/sbin:/usr/openwin/bin:/usr/ccs/bin:/usr/ucb"
#else
/* sun3 and sun4 */
#define LPATH "/usr/ucb:/bin:/usr/bin"
#define RPATH "/usr/ucb:/bin:/usr/bin"
#endif
#endif

#ifdef __ultrix
#define LPATH "/usr/ucb:/bin:/usr/bin"
#define RPATH "/usr/ucb:/bin:/usr/bin"
#endif

#ifdef hpux
/* hpux 8, both hppa and s300 */
#define LPATH "/bin:/usr/bin:/usr/contrib/bin:/usr/local/bin"
#define RPATH "/bin:/usr/bin:/usr/contrib/bin:/usr/local/bin"
#else
#ifdef __hpux /* 9.04 */
#define LPATH_root ":/bin:/usr/bin:/etc"
#define LPATH "/bin:/usr/bin"
#define RPATH "/bin:/usr/bin:/usr/contrib/bin:/usr/local/bin"
#endif
#endif

#ifdef NeXT
#define LPATH "/usr/ucb:/bin:/usr/bin:/usr/local/bin"
#define RPATH "/bin:/usr/ucb:/usr/bin"
#endif

#ifdef _IBMR2
/* 3.2.0 */ 
#define LPATH "/usr/bin:/usr/ucb:/usr/bin/X11"
#define RPATH "/usr/bin:/usr/ucb:/usr/bin/X11"
#endif

#ifdef __SCO__
#define LPATH "/bin:/usr/bin:/usr/dbin:/usr/ldbin"
#define RPATH "/bin:/usr/bin:/usr/local/bin"
#endif

#ifdef sgi
#define LPATH APATH "/usr/sbin:/usr/bsd:/sbin:/usr/bin:/bin:/usr/bin/X11"
#define RPATH APATH "/usr/sbin:/usr/bsd:/sbin:/usr/bin:/bin:/usr/bin/X11"
#endif

#ifdef linux
#define LPATH APATH "/local/bin:/usr/bin:/bin:/usr/local/bin:/usr/bin/X11:."
#define RPATH APATH "/local/bin:/usr/bin:/bin:/usr/local/bin:/usr/bin/X11:."
#endif

#ifdef __386BSD__
#define LPATH "/usr/bin:/bin"
#define RPATH "/usr/bin:/bin"
#endif

#ifdef __alpha
#ifdef __osf__
#define LPATH "/usr/bin:."
#define RPATH "/usr/bin:/bin"
#endif
#endif

#ifdef __pyrsoft
#ifdef MIPSEB
#define RPATH "/bin:/usr/bin"
#define LPATH "/usr/bin:/usr/ccs/bin:/usr/ucb:."
#endif
#endif

#ifdef __DGUX
#ifdef __m88k__
#define RPATH "/usr/bin"
#define LPATH "/usr/bin"
#endif
#endif

#ifndef LPATH
#ifdef __svr4__
/* taken from unixware, sirius... */
#define RPATH "/bin:/usr/bin:/usr/X/bin"
#define LPATH "/usr/bin:/usr/dbin:/usr/dbin"
#endif
#endif

#ifndef LPATH
#ifdef __NetBSD__
#define LPATH APATH "/usr/bin:/bin"
#define RPATH APATH "/usr/bin:/bin"
#endif
#endif

#ifdef _PATH_DEFPATH
#undef LPATH
#undef RPATH
#define LPATH APATH _PATH_DEFPATH
#define RPATH APATH _PATH_DEFPATH
#endif

/* catch-all entries for operating systems we haven't looked up
   hardcoded paths for */
#ifndef LPATH
#define LPATH APATH "/usr/bin:/bin"
#endif

#ifndef RPATH
#define RPATH APATH "/usr/bin:/bin"
#endif
