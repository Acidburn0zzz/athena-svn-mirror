/* $Header: /afs/dev.mit.edu/source/repository/athena/etc/xdm/xlogin/verify.c,v 1.85 1997-12-13 01:38:54 cfields Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#ifdef SYSV
#include <shadow.h>
#include <unistd.h>
#include <limits.h>
#include <utmpx.h>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/sysmacros.h>
#endif
#include <grp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#ifdef SYSV
#include <utime.h>
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <utmp.h>
#include <netdb.h>
#ifndef SYSV
#include <ttyent.h>
#endif
#ifdef sgi
#include <sys/statfs.h>
#endif
#include <errno.h>
#include <syslog.h>

#include <krb.h>
#include <hesiod.h>
#include <al.h>

#ifdef KRB5
#include <krb5.h>
#endif

#ifdef XDM
#include "dm.h"
#endif

#include "environment.h"

#ifndef TRUE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#define LOGIN_TKT_DEFAULT_LIFETIME DEFAULT_TKT_LIFE /* from krb.h */
#define PASSWORD_LEN 14
#define MAXENVIRON 32

#define MOTD "/etc/motd"
#ifndef SYSV
#define UTMP "/etc/utmp"
#define WTMP "/usr/adm/wtmp"
#endif
#ifdef SOLARIS
char *defaultpath = "/srvd/patch:/usr/athena/bin:/bin/athena:/usr/openwin/bin:/bin:/usr/ucb:/usr/sbin:/usr/andrew/bin:.";
#else
#ifdef sgi
char *defaultpath = "/srvd/patch:/usr/athena/bin:/bin/athena:/usr/sbin:/usr/bsd:/usr/bin:/bin:/etc:/usr/etc:/usr/bin/X11:/usr/andrew/bin:.";
#else
char *defaultpath = "/srvd/patch:/usr/athena/bin:/bin/athena:/usr/bin/X11:/usr/new:/usr/ucb:/bin:/usr/bin:/usr/ibm:/usr/andrew/bin:.";
#endif
#endif

#ifdef sgi
extern FILE *xdmstream;
#endif

pid_t fork_and_store(pid_t *var);
extern char *crypt(), *lose(), *getenv();
extern char *krb_get_phost(); /* should be in <krb.h> */
char *get_tickets(), *strsave();
int abort_verify();
extern pid_t attach_pid, attachhelp_pid, quota_pid;
extern int attach_state, attachhelp_state, errno;
extern sigset_t sig_zero;

#ifdef SOLARIS
struct passwd *
 get_pwnam(usr)
 char *usr;
 {
   struct passwd *pwd;
   struct spwd *sp;
   pwd = getpwnam (usr);
   sp = getspnam(usr);
   if ((sp != NULL) && (pwd != NULL))
     pwd->pw_passwd = sp->sp_pwdp;
   return(pwd);
 }
#else
#define get_pwnam(x) getpwnam(x)
#endif

#ifdef XDM
char *dologin(user, passwd, option, script, tty, session, display, verify)
struct verify_info *verify;
#else /* XDM */
char *dologin(user, passwd, option, script, tty, session, display)
#endif /* XDM */
char *user;
char *passwd;
int option;
char *script;
char *tty;
char *session;
char *display;
{
    static char errbuf[5120];
    char tkt_file[128], *msg, wgfile[16];
#ifdef KRB5
    char tkt5_file[128];
#endif
    struct passwd *pwd;
    struct group *gr;
#ifdef SYSV
    struct utimbuf times;
#else
    struct timeval times[2];
#endif
    long salt;
    char saltc[2], c;
    char encrypt[PASSWORD_LEN+1];
    char **environment;
    char fixed_tty[16], *p;
#ifdef sgi
    char *newargv[4];
#endif
    int i;
    /* state variables: */
    int local_passwd = FALSE;	/* user is in local passwd file */
    int local_ok = FALSE;	/* verified from local password file */
#ifdef sgi
    int f;
#endif
    char *altext = NULL, *alerrmem;
    int status, *warnings, *warning;
    int tmp_homedir = 0;

    /* 4.2 vs 4.3 style syslog */
#ifndef  LOG_ODELAY
    openlog("login", LOG_NOTICE);
#else
    openlog("login", LOG_ODELAY, LOG_AUTH);
#endif

    /* Check to make sure a username was entered. */
    if (!strcmp(user, ""))
      {
	return("No username entered.  Please enter a username and "
	       "password to try again.");
      }

    /* Check that the user is allowed to log in. */
    status = al_login_allowed(user, 0, &altext);
    if (status != AL_SUCCESS)
      {
	memset(passwd, 0, strlen(passwd));	/* zap ASAP */
	switch(status)
	  {
	  case AL_ENOUSER:
	    sprintf(errbuf,
		    "Unknown user name entered (no hesiod information "
		    "for \"%s\")", user);
	    break;
	  case AL_ENOLOGIN:
	    strcpy(errbuf,
		   "Logins are currently disabled on this workstation.  ");
	    break;
	  case AL_ENOCREATE:
	    strcpy(errbuf,
		   "You are not allowed to log into this workstation.  "
		   "Contact the workstation's administrator or a consultant "
		   "for further information.  ");
	    break;
	  case AL_EBADHES:
	    strcpy(errbuf, "This account conflicts with a locally defined "
		   "account... aborting.");
	    break;
	  case AL_ENOMEM:
	    strcpy(errbuf, "Out of memory.");
	    break;
	  default:
	    strcpy(errbuf, al_strerror(status, &alerrmem));
	    al_free_errmem(alerrmem);
	    break;
	  }

	if (altext)
	  {
	    strncat(errbuf, altext, sizeof(errbuf) - strlen(errbuf) - 1);
	    free(altext);
	  }

	return errbuf;
      }

    /* Test to see if the user can be authenticated locally. If not,
     * grab their password information from Hesiod, since the uid is
     * potentially needed for mail-check login and the ticket file
     * name, before we want to call al_acct_create().
     */
    if ((pwd = get_pwnam(user)) != NULL) {
	local_passwd = TRUE;
	if (strcmp(crypt(passwd, pwd->pw_passwd), pwd->pw_passwd)) {
	    if (pwd->pw_uid == ROOT)
	      return("Incorrect root password");
	} else
	  local_ok = TRUE;
    } else {
	pwd = hes_getpwnam(user);
	if (pwd == NULL) /* "can't" happen */
	    return "Strange failure in Hesiod lookup.";
    }

    /* Terminal names may be something like pts/0; we don't want any /'s
     * in the path name; replace them with _'s.
     */
    if (tty != NULL)
      {
	strcpy(fixed_tty,tty);
	while (p = strchr(fixed_tty,'/'))
	  *p = '_';
      }
    else
      {
	sprintf(fixed_tty, "%d", pwd->pw_uid);
      }
    sprintf(tkt_file, "/tmp/tkt_%s", fixed_tty);
    psetenv("KRBTKFILE", tkt_file, 1);

    /* we set the ticket file here because a previous dest_tkt() might
     * have cached the wrong ticket file.
     */
    krb_set_tkt_string(tkt_file);

#ifdef KRB5
    sprintf(tkt5_file, "/tmp/krb5cc_%s", fixed_tty);
    psetenv("KRB5CCNAME", tkt5_file, 1);
#endif

    /* Save encrypted password to put in local password file. We do
     * this ahead of time so that we can be sure of zeroing the
     * password below.
     */
    salt = 9 * getpid();
    saltc[0] = salt & 077;
    saltc[1] = (salt>>6) & 077;
    for (i=0;i<2;i++) {
	c = saltc[i] + '.';
	if (c > '9')
	  c += 7;
	if (c > 'Z')
	  c += 6;
	saltc[i] = c;
    }
    strcpy(encrypt,crypt(passwd, saltc));	

    msg = get_tickets(user, passwd);
    memset(passwd, 0, strlen(passwd));

    if (msg) {
	if (!local_ok) {
	    return(msg);
	} else {
	    if (pwd->pw_uid != ROOT)
		prompt_user("Unable to get full authentication, you will "
			    "have local access only during this login "
			    "session (failed to get kerberos tickets).  "
			    "Continue anyway?", abort_verify, NULL);
	}
    }

    chown(tkt_file, pwd->pw_uid, pwd->pw_gid);
#ifdef KRB5
    chown(tkt5_file, pwd->pw_uid, pwd->pw_gid);
#endif

    /* Code for verifying a secure tty used to be here. */

    /* if mail-check login selected, do that now. */
    if (option == 4) {
	attach_state = -1;
	switch(fork_and_store(&attach_pid)) {
	case -1:
	    fprintf(stderr, "Unable to fork to check your mail.\n");
	    break;
	case 0:
	    if (setuid(pwd->pw_uid) != 0) {
		fprintf(stderr, "Unable to set user ID to check your mail.\n");
		_exit(-1);
	    }
	    printf("Electronic mail status:\n");
	    execlp("from", "from", "-r", user, NULL);
	    fprintf(stderr, "Unable to run mailcheck program.\n");
	    _exit(-1);
	default:
	    while (attach_state == -1)
	      sigsuspend(&sig_zero);
	    printf("\n");
	    prompt_user("A summary of your waiting email is displayed in "
			"the console window.  Continue with full login "
			"session or logout now?", abort_verify, NULL);
	}
    }
#if defined(SETPAG) && !defined(sgi) /* not appropriate for SGI system */
    setpag();
#endif

    status = al_acct_create(user, encrypt, getpid(), !msg, 1, &warnings);
    if (status != AL_SUCCESS)
      {
	switch(status)
	  {
	  case AL_EPASSWD:
	    strcpy(errbuf, "An unexpected error occured while entering you in "
		   "the local password file.");
	    return errbuf;
	    break;
	  case AL_WARNINGS:
	    warning = warnings;
	    while (*warning != AL_SUCCESS)
	      {
		switch(*warning)
		  {
		  case AL_WGROUP:
		    prompt_user("Unable to set your group access list.  "
				"You may have insufficient permission to "
				"access some files.  Continue with this "
				"login session anyway?", abort_verify, user);
		    break;
		  case AL_WXTMPDIR:
		    tmp_homedir = 1;
		    prompt_user("You are currently logged in with a "
				"temporary home directory, so this login "
				"session will use that directory. Continue "
				"with this login session anyway?",
				abort_verify, user);
		    break;
		  case AL_WTMPDIR:
		    tmp_homedir = 1;
		    prompt_user("Your home directory is unavailable.  A "
				"temporary directory will be created for "
				"you.  However, it will be DELETED when you "
				"logout.  Any mail that you incorporate "
				"during this session WILL BE LOST when you "
				"logout.  Continue with this login session "
				"anyway?", abort_verify, user);
		    break;
		  case AL_WNOHOMEDIR:
		    prompt_user("No home directory is available.  Continue "
				"with this login session anyway?",
				abort_verify, user);
		    break;
		  case AL_WNOATTACH:
		    prompt_user("This workstation is configured not to "
				"attach remote filesystems.  Continue with "
				"your local home directory?", abort_verify,
				user);
		    break;
		  case AL_WBADSESSION:
		  default:
		    break;
		  }
		warning++;
	      }
	    free(warnings);
	    break;
	  default:
	    strcpy(errbuf, al_strerror(status, &alerrmem));
	    al_free_errmem(alerrmem);
	    return errbuf;
	    break;
	  }
      }

    /* Get the password entry again. We need a new copy because it
     * may have been edited by al_acct_create().
     */
    pwd = get_pwnam(user);
    if (pwd == NULL) /* "can't" happen */
	return(lose("Unable to get your password entry.\n"));

    switch(fork_and_store(&quota_pid)) {
    case -1:
	fprintf(stderr, "Unable to fork to check your filesystem quota.\n");
	break;
    case 0:
	if (setuid(pwd->pw_uid) != 0) {
	    fprintf(stderr,
		    "Unable to set user ID to check your filesystem quota.\n");
	    _exit(-1);
	}
	execlp("quota", "quota", NULL);
	fprintf(stderr, "Unable to run quota command %s\n", "quota");
	_exit(-1);
    default:
	  ;
    }

    /* show message of the day */
    sprintf(errbuf, "%s/.hushlogin", pwd->pw_dir);
    if (!file_exists(errbuf)) {
	int f, count;
	f = open(MOTD, O_RDONLY, 0);
	if (f > 0) {
	    count = read(f, errbuf, sizeof(errbuf) - 1);
	    write(1, errbuf, count);
	    close(f);
	}
    }

    /*
     * Set up the user's environment.
     *
     *   By default, none of xlogin's environment is passed to
     *   users who log in.
     *
     *   The PASSENV macro is defined to make it trivial to pass
     *   an element of xlogin's environment on to the user.
     *
     *   Note that the environment for pre-login options is set
     *   up in xlogin.c: it is NOT RELATED to this environment
     *   setup. If you add a new environment variable here,
     *   consider whether or not it also needs to be added there.
     *   Note that variables that need to be PASSENVed here do not
     *   need similar treatment in the pre-login area, since there
     *   all variables as passed by default.
     */
#define PASSENV(envvar)					\
    msg = getenv(envvar);				\
    if (msg) {						\
	sprintf(errbuf, "%s=%s", envvar, msg);		\
	environment[i++] = strsave(errbuf);		\
    }

    environment = (char **) malloc(MAXENVIRON * sizeof(char *));
    if (environment == NULL)
      return("Out of memory while trying to initialize user environment "
	     "variables.");

    i = 0;
    sprintf(errbuf, "HOME=%s", pwd->pw_dir);
    environment[i++] = strsave(errbuf);
    sprintf(errbuf, "PATH=%s", defaultpath);
    environment[i++] = strsave(errbuf);
    sprintf(errbuf, "USER=%s", pwd->pw_name);
    environment[i++] = strsave(errbuf);
    sprintf(errbuf, "SHELL=%s", pwd->pw_shell);
    environment[i++] = strsave(errbuf);
    sprintf(errbuf, "DISPLAY=%s", display);
    environment[i++] = strsave(errbuf);
    sprintf(errbuf, "KRBTKFILE=%s", tkt_file);
    environment[i++] = strsave(errbuf);
#ifdef KRB5
    sprintf(errbuf, "KRB5CCNAME=%s", tkt5_file);
    environment[i++] = strsave(errbuf);
#endif
#ifdef HOSTTYPE
     sprintf(errbuf, "hosttype=%s", HOSTTYPE); /* environment.h */
     environment[i++] = strsave(errbuf);
#endif

#ifdef SOLARIS
#ifdef XDM
     sprintf(errbuf, "LD_LIBRARY_PATH=%s", "/usr/openwin/lib");
     environment[i++] = strsave(errbuf);
     sprintf(errbuf, "OPENWINHOME=%s", "/usr/openwin");
     environment[i++] = strsave(errbuf);
#else
    PASSENV("LD_LIBRARY_PATH");
    PASSENV("OPENWINHOME");
#endif
#endif

    if (tmp_homedir) {
	environment[i++] = "TMPHOME=1";
    }
    strcpy(wgfile, "/tmp/wg.XXXXXX");
    mktemp(wgfile);
    sprintf(errbuf, "WGFILE=%s", wgfile);
    environment[i++] = strsave(errbuf);
    PASSENV("TZ");

#ifdef sgi
    PASSENV("XAUTHORITY");
#endif

    environment[i++] = NULL;

#ifndef sgi /* nanny handles this on SGI */
    add_utmp(user, tty, display);
#endif
    if (pwd->pw_uid == ROOT)
      syslog(LOG_CRIT, "ROOT LOGIN on tty %s", tty ? tty : "X");

#ifndef sgi /* nanny/xdm does all this on SGI too... */
    /* Set the owner and modtime on the tty */
    sprintf(errbuf, "/dev/%s", tty);
    gr = getgrnam("tty");
    chown(errbuf, pwd->pw_uid, gr ? gr->gr_gid : pwd->pw_gid);
    chmod(errbuf, 0620);

#ifdef SYSV
    times.actime = times.modtime = time(NULL);
    utime(errbuf, &times);
#else
    gettimeofday(&times[0], NULL);
    times[1].tv_sec = times[0].tv_sec;
    times[1].tv_usec = times[0].tv_usec;
    utimes(errbuf, times);
#endif

#ifdef XDM
    {
	static char *newargv[4];

	verify->uid = pwd->pw_uid;
	getGroups(pwd->pw_name, verify, pwd->pw_gid);
	verify->userEnviron = environment;
	newargv[0] = script;
	sprintf(errbuf, "%d", option);
	newargv[1] = errbuf;
	newargv[2] = session;
	newargv[3] = NULL;
	verify->argv = newargv;
	return(0);
    }
#endif /* XDM */

    i = setgid(pwd->pw_gid);
    if (i) 
	return(lose("Unable to set your primary GID.\n"));

        
    if (initgroups(user, pwd->pw_gid) < 0)
	prompt_user("Unable to set your group access list.  You may have "
		    "insufficient permission to access some files.  "
		    "Continue with this login session anyway?",
		    abort_verify, user);

#ifdef SOLARIS_MAE
    /* If the login fails, lose() is called, setting a global flag
       to indicate that xlogin will exit as soon as the user has
       been notified of the error. xlogin will then restart, and
       at the beginning of xlogin we chown netdev back to root. */
    if (netspy)
      chown(NETDEV, pwd->pw_uid, SYS);
#endif

    i = setuid(pwd->pw_uid);
    if (i)
      return(lose("Unable to set your user ID.\n"));
#endif /* not sgi */

    if (chdir(pwd->pw_dir))
      fprintf(stderr, "Unable to connect to your home directory.\n");

    /* Stuff first arg for xsession into a string. */
    sprintf(errbuf, "%d", option);

#ifdef sgi
    /* Output username and environment information and let xdm log us in. */
    fprintf(xdmstream, "%s", pwd->pw_name);
    fputc(0, xdmstream);

    newargv[0] = errbuf;
    newargv[1] = script;
    newargv[2] = NULL;
    if (nanny_setupUser(pwd->pw_name, !local_passwd, environment, newargv))
      return(lose("failed to setup for login"));

    exit(0);
#else
    execle(session, "sh", errbuf, script, NULL, environment);
#endif /* sgi */

    return(lose("Failed to start session."));
}


char *get_tickets(username, password)
char *username;
char *password;
{
    char inst[INST_SZ], realm[REALM_SZ];
    char hostname[MAXHOSTNAMELEN], phost[INST_SZ];
    char key[8], *rcmd;
    static char errbuf[1024];
    int error;
    struct hostent *hp;
    KTEXT_ST ticket;
    AUTH_DAT authdata;
    unsigned long addr;

    rcmd = "rcmd";

    /* inst has to be a buffer instead of the constant "" because
     * krb_get_pw_in_tkt() will write a zero at inst[INST_SZ] to
     * truncate it.
     */
    inst[0] = 0;
    dest_tkt();
#ifdef KRB5
    do_v5_kdestroy(0);
#endif

    if (krb_get_lrealm(realm, 1) != KSUCCESS)
      strcpy(realm, KRB_REALM);

    error = krb_get_pw_in_tkt(username, inst, realm, "krbtgt", realm,
			      LOGIN_TKT_DEFAULT_LIFETIME, password);
    switch (error) {
    case KSUCCESS:
	break;
    case INTK_BADPW:
	return("Incorrect password entered.");
    case KDC_PR_UNKNOWN:
	return("Unknown username entered.");
    default:
	sprintf(errbuf, "Unable to authenticate you, kerberos failure "
		"%d: %s.  Try again here or on another workstation.",
		error, krb_err_txt[error]);
	return(errbuf);
    }

#ifdef KRB5
    {
	krb5_error_code krb5_ret;
	char *etext;

	krb5_ret = do_v5_kinit(username, inst, realm,
			       LOGIN_TKT_DEFAULT_LIFETIME, password,
			       0, &etext);
	if (krb5_ret && krb5_ret != KRB5KRB_AP_ERR_BAD_INTEGRITY) {
	    com_err("xlogin", krb5_ret, etext);
	}
    }
#endif

    if (gethostname(hostname, sizeof(hostname)) == -1) {
	fprintf(stderr, "Warning: cannot retrieve local hostname");
	return(NULL);
    }
    strncpy (phost, krb_get_phost (hostname), sizeof (phost));
    phost[sizeof(phost)-1] = '\0';

    /* without srvtab, cannot verify tickets */
    if (read_service_key(rcmd, phost, realm, 0, KEYFILE, key) == KFAILURE)
      return (NULL);

    hp = gethostbyname (hostname);
    if (!hp) {
	fprintf(stderr, "Warning: cannot get address for host %s\n", hostname);
	return(NULL);
    }
    memmove(&addr, hp->h_addr, sizeof (addr));

    error = krb_mk_req(&ticket, rcmd, phost, realm, 0);
    if (error == KDC_PR_UNKNOWN) return(NULL);
    if (error != KSUCCESS) {
	sprintf(errbuf, "Unable to authenticate you, kerberos failure %d: %s",
		error, krb_err_txt[error]);
	return(errbuf);
    }
    error = krb_rd_req(&ticket, rcmd, phost, addr, &authdata, "");
    if (error != KSUCCESS) {
	memset(&ticket, 0, sizeof(ticket));
	sprintf(errbuf, "Unable to authenticate you, kerberos failure %d: %s",
		error, krb_err_txt[error]);
	return(errbuf);
    }
    memset(&ticket, 0, sizeof(ticket));
    memset(&authdata, 0, sizeof(authdata));
    return(NULL);
}


cleanup(user)
char *user;
{
    /* must also detach homedir, clean passwd file */
    dest_tkt();
#ifdef KRB5
    do_v5_kdestroy(0);
#endif

    if (user)
      al_acct_revert(user, getpid());

    /* Set real uid to zero.  If this is impossible, exit.  The
       current implementation of lose() will not print a message
       so xlogin will just exit silently.  This call "can't fail",
       so this is not a serious problem. */
    if (setuid(0) == -1)
      lose ("Unable to reset real uid to root");
}

abort_verify(user)
char *user;
{
    cleanup(user);
    _exit(1);
}

char *strsave(s)
char *s;
{
    char *ret = malloc(strlen(s) + 1);
    strcpy(ret, s);
    return(ret);
}

add_utmp(user, tty, display)
char *user;
char *tty;
char *display;
{
    struct utmp ut_entry;
#ifndef SYSV
    struct utmp ut_tmp;
#else
    struct utmp *ut_tmp;
    struct utmpx utx_entry;
    struct utmpx *utx_tmp;
#endif /* SYSV */
    int f;

#ifdef SYSV
    memset(&utx_entry, 0, sizeof(utx_entry));

    strncpy(utx_entry.ut_line, tty, sizeof utx_entry.ut_line);
    strncpy(utx_entry.ut_name, user, sizeof utx_entry.ut_name);

    /* Be sure the host string is null terminated. */
    strncpy(utx_entry.ut_host, display, sizeof utx_entry.ut_host);
    utx_entry.ut_host[(sizeof utx_entry.ut_host) - 1] = '\0';

    gettimeofday(&utx_entry.ut_tv, NULL);
    utx_entry.ut_pid = getppid();
    utx_entry.ut_type = USER_PROCESS;
    strncpy(utx_entry.ut_id, "XLOG", sizeof utx_entry.ut_id);

    getutmp(&utx_entry, &ut_entry);

    setutent();
    while (ut_tmp = getutline(&ut_entry))
      if (!strncmp(ut_tmp->ut_id, "XLOG", sizeof (ut_tmp->ut_id)))
	break;
    pututline(&ut_entry);

    setutxent();
    while (utx_tmp = getutxline(&utx_entry))
      if (!strncmp(utx_tmp->ut_id, "XLOG", sizeof (utx_tmp->ut_id)))
	break;
    pututxline(&utx_entry);

    if ( (f = open( WTMP_FILE, O_WRONLY|O_APPEND)) >= 0) {
        write(f, (char *) &ut_entry, sizeof(ut_entry));
        close(f);
    }
    if ( (f = open( WTMPX_FILE, O_WRONLY|O_APPEND)) >= 0) {
        write(f, (char *) &utx_entry, sizeof(utx_entry));
        close(f);
}
#else /* !SYSV */

    memset(&ut_entry, 0, sizeof(ut_entry));

    strncpy(ut_entry.ut_line, tty, sizeof ut_entry.ut_line);
    strncpy(ut_entry.ut_name, user, sizeof ut_entry.ut_name);

    /* Be sure the host string is null terminated. */
    strncpy(ut_entry.ut_host, display, sizeof ut_entry.ut_host);
    ut_entry.ut_host[(sizeof ut_entry.ut_host) - 1] = '\0';

    time(&(ut_entry.ut_time));

    if ((f = open(UTMP, O_RDWR )) >= 0) {
	while (read(f, (char *) &ut_tmp, sizeof(ut_tmp)) == sizeof(ut_tmp))
	    if (ut_tmp.ut_pid == ut_entry.ut_pid) {
		strncpy(ut_entry.ut_id, ut_tmp.ut_id, sizeof(ut_tmp.ut_id));
		lseek(f, -(long) sizeof(ut_tmp), 1);
		break;
	    }
	write(f, (char *) &ut_entry, sizeof(ut_entry));
	close(f);
    }
    if ( (f = open( WTMP, O_WRONLY|O_APPEND)) >= 0) {
	write(f, (char *) &ut_entry, sizeof(ut_entry));
	close(f);
    }
#endif /* SYSV */
}

#define MAXGNAMELENGTH	32

/* Fork, storing the pid in a variable var and returning the pid.  Make sure
 * that the pid is stored before any SIGCHLD can be delivered. */
pid_t fork_and_store(pid_t *var)
{
    sigset_t mask, omask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &omask);
    *var = fork();
    sigprocmask(SIG_SETMASK, &omask, NULL);
    return *var;
}

/* Emulate setenv() with the more portable (these days) putenv(). */
int psetenv(const char *name, const char *value, int overwrite)
{
    char *var;

    if (!overwrite && getenv(name) != NULL)
	return 0;
    var = malloc(strlen(name) + strlen(value) + 2);
    if (!var) {
	errno = ENOMEM;
	return -1;
    }
    sprintf(var, "%s=%s", name, value);
    putenv(var);
    return 0;
}

/* Emulate unsetenv() by fiddling with the environment. */
int punsetenv(const char *name)
{
    extern char **environ;
    char **p, **q;
    int len = strlen(name);

    q = environ;
    for (p = environ; *p; p++) {
	if (strncmp(*p, name, len) != 0 || (*p)[len] != '=')
	    *q++ = *p;
    }
    *q = NULL;
    return 0;
}

#ifdef KRB5
/*
 * This routine takes v4 kinit parameters and performs a V5 kinit.
 * 
 * name, instance, realm is the v4 principal information
 *
 * lifetime is the v4 lifetime (i.e., in units of 5 minutes)
 * 
 * password is the password
 *
 * ret_cache_name is an optional output argument in case the caller
 * wants to know the name of the actual V5 credentials cache (to put
 * into the KRB5_ENV_CCNAME environment variable)
 *
 * etext is a mandatory output variable which is filled in with
 * additional explanatory text in case of an error.
 * 
 */
krb5_error_code do_v5_kinit(name, instance, realm, lifetime, password,
			    ret_cache_name, etext)
	char	*name;
	char	*instance;
	char	*realm;
	int	lifetime;
	char	*password;
	char	**ret_cache_name;
	char	**etext;
{
	krb5_context context;
	krb5_error_code retval;
	krb5_principal me = 0, server = 0;
	krb5_ccache ccache = NULL;
	krb5_creds my_creds;
	krb5_timestamp now;
	krb5_flags options = KDC_OPT_FORWARDABLE | KDC_OPT_PROXIABLE;

	char *cache_name;

	*etext = 0;
	if (ret_cache_name)
		*ret_cache_name = 0;
	memset(&my_creds, 0, sizeof(my_creds));

	retval = krb5_init_context(&context);
	if (retval)
		return retval;

	cache_name = krb5_cc_default_name(context);
	krb5_init_ets(context);

	retval = krb5_425_conv_principal(context, name, instance, realm, &me);
	if (retval) {
		*etext = "while converting V4 principal";
		goto cleanup;
	}

	retval = krb5_cc_resolve(context, cache_name, &ccache);
	if (retval) {
		*etext = "while resolving ccache";
		goto cleanup;
	}

	retval = krb5_cc_initialize(context, ccache, me);
	if (retval) {
		*etext = "while initializing cache";
		goto cleanup;
	}

	retval = krb5_build_principal_ext(context, &server,
					  krb5_princ_realm(context,
							   me)->length,
					  krb5_princ_realm(context, me)->data,
					  KRB5_TGS_NAME_SIZE, KRB5_TGS_NAME,
					  krb5_princ_realm(context,
							   me)->length,
					  krb5_princ_realm(context, me)->data,
					  0);
	if (retval)  {
		*etext = "while building server name";
		goto cleanup;
	}

	retval = krb5_timeofday(context, &now);
	if (retval) {
		*etext = "while getting time of day";
		goto cleanup;
	}

	my_creds.client = me;
	my_creds.server = server;
	my_creds.times.starttime = 0;
	my_creds.times.endtime = now + lifetime*5*60;
	my_creds.times.renew_till = 0;

	retval = krb5_get_in_tkt_with_password(context, options, NULL, NULL,
					       NULL, password, ccache,
					       &my_creds, NULL);
	if (retval) {
		*etext = "while calling krb5_get_in_tkt_with_password";
		goto cleanup;
	}

	if (ret_cache_name) {
		*ret_cache_name = malloc(strlen(cache_name)+1);
		if (!*ret_cache_name) {
			retval = ENOMEM;
			goto cleanup;
		}
		strcpy(*ret_cache_name, cache_name);
	}

cleanup:
	if (me)
		krb5_free_principal(context, me);
	if (server)
		krb5_free_principal(context, server);
	if (ccache)
		krb5_cc_close(context, ccache);
	my_creds.client = 0;
	my_creds.server = 0;
	krb5_free_cred_contents(context, &my_creds);
	krb5_free_context(context);
	return retval;
}

krb5_error_code do_v5_kdestroy(cachename)
	char	*cachename;
{
	krb5_context context;
	krb5_error_code retval;
	krb5_ccache cache;

	retval = krb5_init_context(&context);
	if (retval)
		return retval;

	if (!cachename)
		cachename = krb5_cc_default_name(context);

	krb5_init_ets(context);

	retval = krb5_cc_resolve (context, cachename, &cache);
	if (retval) {
		krb5_free_context(context);
		return retval;
	}

	retval = krb5_cc_destroy(context, cache);

	krb5_free_context(context);
	return retval;
}
#endif /* KRB5 */
