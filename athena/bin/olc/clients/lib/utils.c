/*
 * This file is part of the OLC On-Line Consulting System.
 * It contains miscellaneous utilties for the olc and olcr programs.
 *
 *      Win Treese
 *      Dan Morgan
 *      Bill Saphir
 *      MIT Project Athena
 *
 *      Ken Raeburn
 *      MIT Information Systems
 *
 *      Tom Coppeto
 *      MIT Project Athena
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology
 *
 *      $Source: /afs/dev.mit.edu/source/repository/athena/bin/olc/clients/lib/utils.c,v $
 *      $Author: tjcoppet $
 */

#ifndef lint
static char rcsid[]="$Header: /afs/dev.mit.edu/source/repository/athena/bin/olc/clients/lib/utils.c,v 1.1 1989-07-06 21:48:02 tjcoppet Exp $";
#endif

#include <olc/olc.h>
#include <zephyr/zephyr.h>
#include <signal.h>
#include <ctype.h>

/*
 * Function:	call_program() executes the named program by forking the
 *			olc process.
 * Arguments:	program:	Name of the program to execute.
 *		argument:	Argument to be passed to the program.
 *	Note: Currently we support only a single argument, though it
 *	may be necessary to extend this later.
 * Returns:	An error code.
 * Notes:
 *	First, we fork a new process.  If the fork is unsuccessful, this
 *	fact is logged, and we return an error.  Otherwise, the child
 *	process (pid = 0) exec's the desired program, while the parent
 *	program waits for it to finish.
 */

ERRCODE
call_program(program, argument)
     char *program;		/* Name of program to be called. */
     char *argument;		/* Argument to be passed to program. */
{
  int pid;		/* Process id for forking. */
  int (*func)();

  if ((pid = fork()) == -1) 
    {
      perror("call_program");
      return(ERROR);
    }
  else 
    if (pid == 0) 
      {
	execlp(program, program, argument, 0);
	perror("call_program");
	return(ERROR);
      }
    else 
      {
	func = signal(SIGINT, SIG_IGN);
	while (wait(0) != pid) 
	  {
			/* ho hum ... (yawn) */
            /* tap tap */
	  };
	signal(SIGINT, func);
	return(SUCCESS);
      }
}




/*
 * Function:    expand_hostname()
 * Arguments:   char *hostname:    
 *              char *instance:  return
 *              char *realm:     return
 *
 * Returns:     Nothing
 * Notes:       Parses hostname for instance & realm.
 *              Snarfed from kerberos document
 */

void
expand_hostname(hostname, instance, realm)
     char *hostname;
     char *instance;
     char *realm;
{
  char *p;
  int i;

  realm[0] = '\0';
  p = index(hostname, '.');
  
  if(p == NULL)
    {
      (void) strcpy(instance, hostname);

#ifdef KERBEROS
      get_krbrlm(realm,1);
#endif KERBEROS

    }
  else
    {
      i = p-hostname;
      (void) strncpy(instance,hostname,i);
      instance[i] = '\0';
      (void) strcpy(realm, p+1);
    }

#ifdef REALM
  if(strlen(realm) == 0)
    (void) strcpy(realm, LOCAL_REALM);
#endif REALM

  for(i=0; instance[i] != '\0'; i++)
    if(isupper(instance[i]))
      instance[i] = tolower(instance[i]);

  for(i=0; realm[i] != '\0'; i++)
    if(islower(realm[i]))
      realm[i] = toupper(realm[i]);
  
  for(i=0; strlen(LOCAL_REALMS[i]) !=0; i++)
    if(strcmp(realm, LOCAL_REALMS[i]) == 0)
      (void) strcpy(realm, LOCAL_REALM);
  
  return;
}


/*
 * Function:	sendmail() forks a sendmail process to send mail to someone.
 * Arguments:	username:	Name of user receving the mail.
 *		machine:	His machine.
 * Returns:	A file descriptor of a pipe to the sendmail process.
 * Notes:
 *	First, create a pipe so we can fork a sendmail child process.
 *	Then execute the fork, logging an error if we are unable to do.
 *	As usual in a situtation like this, check the process ID returned
 *	by fork().  If it is zero, we are in the child, so we execl
 *	sendmail with the appropriate arguments.  Otherwise, close
 *	the zeroth file descriptor, which is for reading.  Then construct
 *	the mail address and write it to sendmail.  Finally, return the
 *	file descriptor so the message can be sent.
 */

int
sendmail()
{
  int fildes[2];	/* File descriptor array. */
  
  (void) pipe(fildes);
  switch (fork()) 
    {
    case -1:		/* error */
      perror("mail");
      printf("sendmail: error starting process.\n");
      return(-1);
    case 0:		/* child */
      (void) close(fildes[1]);
      (void) close(0);
      (void) dup2(fildes[0], 0);
      execl("/usr/lib/sendmail", "sendmail", "-t", 0);
      perror("sendmail: exec");
      exit(1);
    default:
      (void) close(fildes[0]);
      return(fildes[1]);
    }
}



#ifdef ZEPHYR
char *
zephyr_get_opcode(class, instance)
     char *class;
     char *instance;
{
  ZNotice_t notice;
  struct sockaddr_in from;
  Code_t retval;
  char *msg;

  while (1) 
    {
      if ((retval = ZReceiveNotice(&notice, &from)) != ZERR_NONE)
	{
	  com_err("olc", retval, "while receiving notice");
	  return((char *) NULL);
	}
      
      if ((strcmp(notice.z_class, class) != 0) ||
	  (strcmp(notice.z_class_inst, instance) != 0))
	continue;
      
      msg =  strcpy(malloc(strlen((notice.z_opcode))+1), (notice.z_opcode));
      ZFreeNotice(&notice);
      return(msg);
    }
}


zephyr_subscribe(class, instance, recipient)
     char *class;
     char *instance;
     char *recipient;
{
  ZSubscription_t sub;
  Code_t retval;

  sub.class = class;
  sub.classinst = instance;
  sub.recipient = recipient;

  if ((retval = ZSubscribeTo(&sub, 1, 0)) != ZERR_NONE)
    {
      com_err("olc", retval, "while subscribing");
      return(ERROR);
    }

  return(SUCCESS);
}

#endif ZEPHYR
