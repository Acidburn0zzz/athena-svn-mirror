#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <hesiod.h>

#define CVIEW_FALLBACK_PORT 3704

int net(progname, num, names)
     char *progname;
     int num;
     char **names;
{
  static char *host;		/* name of host running cview daemon. */
  static char **serv;		/* cview service hesiod info. */
  static struct hostent *hp;	/* hostent struct for cview server. */
  static struct servent *sp;	/* servent struct for cview service. */
  static struct sockaddr_in sin; /* Socket address. */
  static int init = 0;		/* Have we been here before? */
  int s;			/* Socket to connect to cview daemon. */
  int j;			/* Counter. */
  char buf[BUFSIZ];		/* Temporary buffer. */

  if (!init)
    {
#ifdef TEST
      host = "fries.mit.edu";
#else
      serv = hes_resolve("cview","sloc"); 

      if (serv == NULL)
	host = "doghouse.mit.edu";	/* fall back if hesiod is broken... */
      else
	host = *serv;
#endif

      hp = gethostbyname(host);

      if (hp == NULL) 
	{
	  fprintf(stderr, "%s: Unable to resolve hostname '%s'.\n",
		  progname, host);
	  return(-1);
	}
  
      if (hp->h_length != sizeof(sin.sin_addr))
	{
	  fprintf(stderr, "%s: Unexpected h_length value for '%s'.\n",
		  progname, host);
	  return(-1);
	}

      sp = getservbyname("cview", "tcp");

      memset(&sin, 0, sizeof (sin));
      memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
      sin.sin_family = hp->h_addrtype;
      sin.sin_port = (sp) ? sp->s_port : htons(CVIEW_FALLBACK_PORT);

      init = 1;
    }
  
  s = socket(hp->h_addrtype, SOCK_STREAM, 0);
  if (s < 0) 
    {
      perror("socket"); 
      return(-1);
    }

  if (connect(s, (struct sockaddr *)&sin, sizeof (sin)) < 0) 
    {
      perror("connect");
      close(s);
      return(-1);
    }

  for (j=0; j<num; j++)
    {
      strncpy(buf, *names, sizeof(buf) - 2);
      buf[sizeof(buf) - 2] = '\0';
      strcat(buf, " ");
      if (write(s, buf, strlen(buf)) == -1)
	{
	  perror("write");
	  close(s);
	  return(-1);
	}
      names++;
    }

  if (write(s, "\n", 1) == -1)
    {
      perror("write");
      close(s);
      return(-1);
    }

  return(s);
}
