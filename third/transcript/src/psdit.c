#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987,1990,1991,1992 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: /afs/dev.mit.edu/source/repository/third/transcript/src/psdit.c,v 1.3 1999-11-04 17:31:03 tb Exp $";
#endif
/* psdit.c
 *
 * Copyright (C) 1984,1985,1987,1990,1991,1992 Adobe Systems Incorporated.
 * All rights reserved. 
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * ditroff intermediate file to PostScript translator
 *
 * Original Version: Barry Hayes spring/summer 1984
 *
 * RCSLOG:
 * $Log: not supported by cvs2svn $
 * Revision 1.2  1997/10/30 01:24:23  ghudson
 * From mhpower: close buffer overruns.
 *
 * Revision 1.1.1.1  1996/10/07 20:25:51  ghudson
 * Import of Transcript 4.1
 *
 * Revision 3.6  1993/12/21  23:43:24  snichols
 * missed one of those bogus escapes.
 *
 * Revision 3.5  1992/08/21  16:26:32  snichols
 * Release 4.0
 *
 * Revision 3.4  1992/07/14  22:45:07  snichols
 * Updated copyright.
 *
 * Revision 3.3  1992/06/01  22:36:53  snichols
 *  get rid of unneeded declaration of getpwuid.
 *
 * Revision 3.2  1992/05/26  21:52:05  snichols
 * comments after endif
 *
 * Revision 3.1  1992/05/18  20:52:12  snichols
 * Now tries to match page sizes from DESC file to known page sizes,
 * so it can put out the appropriate IncludeFeature comment.  Also,
 * some changes related to prolog changes, where problems of page
 * independence were cleaned up.
 *
 * Revision 3.0  1991/06/17  16:50:45  snichols
 * Release 3.0
 *
 * Revision 2.14  1991/03/06  22:21:31  snichols
 * needed to include string.h
 *
 * Revision 2.13  1991/03/06  22:07:54  snichols
 * missing declaration in imported code.
 *
 * Revision 2.12  1991/03/06  21:51:54  snichols
 * imported code missing arg to perror.
 *
 * Revision 2.11  1991/03/06  21:45:10  snichols
 * really fix the index problem.
 *
 * Revision 2.10  1991/03/06  21:30:07  snichols
 * imported psfig code used index instead of strchr.
 *
 * Revision 2.9  1991/02/08  11:01:39  snichols
 * support for psfig's use of the \X feature in DWB 2.0.
 *
 * Revision 2.8  91/01/04  16:21:55  snichols
 * remove bogus malloc declaration.
 * 
 * Revision 2.7  90/12/12  10:23:33  snichols
 * new configuration stuff.
 * 
 * Revision 2.6  90/11/21  17:22:18  snichols
 * fixed bug in handling of input from stdin.
 * 
 * Revision 2.5  90/11/21  15:44:39  snichols
 * increased path size limit on temp files, and made it symbolic.
 * added usage string.
 * now handles multiple files correctly.
 * doesn't send prolog if there's nothing in a file.
 * 
 * Revision 2.4  90/11/16  14:16:38  snichols
 * brought up to DSC 3.0
 * 
 * Revision 2.3  90/10/11  15:07:41  snichols
 * Updated to match version 3.0 DSC.
 * 
 * Revision 2.2  87/11/17  16:51:53  byron
 * *** empty log message ***
 * 
 * Revision 2.2  87/11/17  16:51:53  byron
 * Release 2.1
 * 
 * Revision 2.1.1.9  87/11/12  13:41:36  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.8  87/07/08  16:03:11  byron
 * Fixed char overprinting.  Wasn't overprinting at all before. This is
 * basically a kludge -- the char output stuff should be rewritten.
 * 
 * Revision 2.1.1.7  87/06/19  17:07:40  byron
 * Added checks for EOF in code that skips to end-of-line.  Fixed problem
 * where ending input prematurely gave infinite loop. Thanx to Kirk D. Smith.
 * 
 * Revision 2.1.1.6  87/06/04  11:24:30  byron
 * Type problems caught by customer compiler (courtesy Brian Reid).
 * 
 * Revision 2.1.1.5  87/04/29  11:34:53  byron
 * David Yost's support for the DWB 2.0 "x X ..\n" command.
 * 
 * Revision 2.1.1.4  87/04/23  10:26:27  byron
 * Copyright notice.
 * 
 * Revision 2.1.1.3  87/04/23  10:03:29  byron
 * Changed fprintf() to debugp() in draw routine.  Was printing out
 * random coordinates in some cases when drawing.
 * 
 * Revision 2.1.1.2  87/04/13  16:40:37  byron
 * Added FlushShow for 'x' commands.  Was overprinting chars just before
 * fonts switching -- didn't have previous horizontal move correct.
 * 
 * Revision 2.1.1.1  86/06/01  17:47:34  shore
 * fixed drawing location bugs
 * complains on missing .aux file
 * 
 * Revision 2.1  85/11/24  11:50:41  shore
 * Product Release 2.0
 * 
 * Revision 1.8  85/11/23  20:09:44  shore
 * test for termination of included PostScript was bad
 * 
 * Revision 1.7  85/11/21  14:23:56  shore
 * added envget check for PSLIBDIR
 * 
 * Revision 1.6  85/11/20  00:43:43  shore
 * support for included PostScript
 * big rework on FlushShow, word "breaks"
 * removed FlushFont and made them instant
 * Still no Gremlin support yet
 * 
 * Revision 1.5  85/10/03  10:48:09  shore
 * added FlushShow to xf fix !
 * 
 * Revision 1.4  85/10/02  16:20:32  shore
 * fixed xf bug 
 * mounting a font causes a font switch!
 * 
 * Revision 1.3  85/07/09  13:10:20  shore
 * added fclose on map file
 * 
 * Revision 1.2  85/05/14  11:24:23  shore
 * added flush to trailer
 * fixed read bug when mounting fonts
 * 
 * 
 */

/* 
output language from troff:
all numbers are character strings

sn	size in points
fn	font as number from 1-n
cx	ascii character x
Cxyz	funny char xyz. terminated by white space
Hn	go to absolute horizontal position n
Vn	go to absolute vertical position n (down is positive)
hn	go n units horizontally (relative)
vn	ditto vertically
nnc	move right nn, then print c (exactly 2 digits!)
		(this wart is an optimization that shrinks output file size
		 about 35% and run-time about 15% while preserving ascii-ness)
Dt ...\n	draw operation 't':
	Dl dx dy	line from here by dx,dy
	Dc d		circle of diameter d with left side here
	De x y		ellipse of axes x,y with left side here
	Da cdx cdy fdx fdy	arc counter-clockwise
				center by cdx,cdy
				end by fdx,fdy
	D~ x y x y ...	wiggly line by x,y then x,y ...
nb a	end of line (information only -- no action needed)
	a = space before line, a = after
w	paddable word space -- no action needed
pn	new page begins -- set v to 0
{	push current environment (font info & location)
}	pop a saved environment
txxxx	print string xxxx using natural widths
#...\n	comment
x ...\n	device control functions:
	x i[nit]	init
	x T s		name of device is s
	x r[es] n h v	resolution is n/inch
			h = min horizontal motion, v = min vert
	x p[ause]	pause (can restart)
	x s[top]	stop -- done for ever
	x t[railer]	generate trailer
	x f[ont] n s	font position n contains font s
	x H[eight] n	set character height to n
	x S[lant] n	set slant to N
  	x X ...\n       pass ... directly to postscript

Adobe Extension for included PostScript:
%
(raw postscript...)
.\n

*/

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <string.h>
#include "transcript.h"

#include "dev.h"
#include "config.h"


/* forwards */
private outlist();
private conv();
private preface();
private devcntrl();
private fileinit();
private loadpswidths();
private loadfont();
private sayload();
private VOID done();
private t_init();
private t_push();
private t_pop();
private t_page();
private t_size();
private t_charht();
private t_slant();
private t_font();
private t_text();
private t_reset();
private t_res();
private t_trailer();
private put1();
private put1s();
private putnf();
private t_fp();
private setfont();
private drawline();
private drawwig();
private drawcirc();
private drawarc();
private drawellip();
private hmot();
private hgoto();
private vmot();
private vgoto();
private showspecial();
private showchar();
private MoveTo();
private DoMove();
private FlushShow();


#define	NFONT	100

/* DIT state consists of: */
private int	hpos;		/* current horizontal position */
private int	vpos;		/* current vertical position */
private int	fontsize;	/* current font size */
private int	fontheight;	/* current character height */
private int	fontslant;	/* current font slant */
private int	font;		/* current font */
private int	resolution;	/* device resolution */
private int	minhoriz;	/* minimum horizontal motion */
private int	minvert;	/* minimum vertical motion */

private int	onspecial;
private int	specfont;
private int	prevfont;
private int 	pfont;

/* {} push/pop stack */
#define DSTACK 10
private struct ditstack {
    int hpos, vpos, fontsize, fontheight, fontslant, font;
} ditstack[DSTACK];
private int dlevel = 0;

#define ErrorTolerance 48
#define PSWID 0x00000FFF
#define ISPSPROC 0x000FF000


/* PSscale is equivalent to (x * PSmag / 72000) + 0.5 */
#define PSmag 16
#define PSscale(x) (((x)+2250)/4500)

/* we maintain PS coords with PSmag times the precision */
/* current PS state is: */

private int	PSx;		/* current horizontal position */
private int	PSy;		/* current vertical position */
private int	savex, savey;	/* position of start of current show string */

/* ps move types -- note that XMOVE|YMOVE == XYMOVE ! */
#define NONE 0
#define XMOVE 1
#define YMOVE 2
#define XYMOVE 3

private int movepending = NONE;

/* buffer string for show -- save up adjacent chars */
#define SHOWSIZE 400
private char showbuf[SHOWSIZE + 3]; /* extras are for quoting */
private int showind = 0;	/* index into string of next available byte */
private int PSshowlen = 0;	/* size in big units of buffered string */
private int nshow = 0;		/* actual number of show chars in showbuf */
private int startx;		/* troff starting pos of current string */
private int thisw;

/* #define NONE 0 */
#define HMOT 1
#define VMOT 2
#define CPUT 4
#define BRK  8
#define FNT  16
private int lastcmd;

private int	output	= 0;	/* do we do output at all? */
private int	nolist	= 0;	/* output page list if > 0 */
private int	olist[20];	/* pairs of page numbers */
private int	spage	= 9999;	/* stop every spage pages */
private int	scount	= 0;
private int	stopped = 0;
private int	pageno = 0;
private int	firstpage = TRUE;

private struct	dev	dev;
private struct font *fontbase[NFONT+1];
private char    fontlist[NFONT+1][60];
private int     nfontlist = 0;
private short	*pstab;
private int	dres;	/* resolution from DESC */
private int	nsizes; /* number of point sizes from DESC */
private int	nfonts; /* number of fonts from DESC */
private int	smnt;	/* index of first special font */
private int	nchtab;
private char	*chname;
private short	*chtab;
private char	*fitab[NFONT+1];
private char	*widthtab[NFONT+1];	/* widtab would be a better name */
private char	*codetab[NFONT+1];	/* device codes */

private int     *pswidths[NFONT+1]; /* ps width tables */
private int	fontdelta[NFONT+1]; /* nonzero if xf overwrites font i */

/* font position info: */
private struct {
	char *name;
	int number;
} fontname[NFONT+1];

#define	FATAL	1
#define	BMASK	0377

#ifdef DEBUG
int	dbg	= 0;
int	fdbg = 0;
#define debugp(xxx) {if(dbg != 0){dbg--; printf xxx ; VOIDC fflush(stdout);}}
#else
#define debugp(x)
#endif

/* page sizes */
#define NSIZES 11
struct dimensions {
    char name[25];
    int width;
    int height;
} pagesizes[NSIZES] = {
    { "Letter", 612, 792 },
    { "Legal", 612, 1008 },
    { "A4", 595, 842 },
    { "B5", 516, 729 },
    { "Tabloid", 792, 1224 },
    { "A3", 842, 1191 },
    { "Ledger", 1224, 792 },
    { "A5", 420, 595 }, 
    { "SuperA", 643, 1009 },
    { "SuperB", 864, 1380 },
    { "Executive", 522, 756 }
};


private FILE	*tf;	/* output file */
private char devname[20] = "psc";

private char	*infilename = "stdin"; /* input file name */
private char	*prologfile = PSDITPRO;
private char	*ditdir;
static char *printer = NULL;

private char	*prog;		/* argv[0] - program name */

#define ARGS "f:F:p:o:d:bws:"

static void Usage()
{
    fprintf(stderr,"Usage: psdit [-P printer] [-F fontdir] [-p prolog] [-o list] [file]\n");
    exit(0);
}

main(argc, argv)
int argc;
char *argv[];
{
    FILE *fp;
    VOID done();
    int c;
    int i;

    tf = stdout;

    ditdir = DitDir;
    prog = argv[0];
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
	switch (c) {
	case 'f':
	case 'F':
	    ditdir = optarg;
	    break;
	case 'p':
	    prologfile = optarg;
	    break;
	case 'o':
	    outlist(optarg);
	    break;
	case 'P':
	    printer = optarg;
	    break;
	case 'd':
#ifdef DEBUG
	    dbg = atoi(optarg);
	    if (dbg == 0)
		dbg = 1;
	    tf = stdout;
#endif /* DEBUG */
	    break;
	case 'b':		/* ignore busy */
	    break;
	case 'w':		/* ignore wait */
	    break;
	case 's':
	    spage = atoi(optarg);
	    if (spage <= 0)
		spage = 9999;
	    break;
	default:
	    Usage();
	    break;
	}
    }

    if (signal(SIGINT, done) == SIG_IGN) {
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
    }
    else {
	signal(SIGQUIT, done);
	signal(SIGHUP, done);
    }
    signal(SIGTERM, done);

    if (optind == argc) 
	conv(stdin);
    else
	for (i = optind; i < argc; i++) {
	    if (*argv[i] == '-')
		fp = stdin;
	    else if ((fp = fopen(argv[i],"r")) == NULL) {
		fprintf(stderr, "%s: can't open %s\n", prog, argv[i]);
		pexit(prog, 2);
	    }
	    infilename = argv[i];
	    conv(fp);
	    VOIDC fclose(fp);
	}
    done();
}

private outlist(s)	/* process list of page numbers to be printed */
char *s;
{
    int     n1, n2, i;

    nolist = 0;
    while (*s) {
	n1 = 0;
	if (isdigit (*s)) {
	    do {
		n1 = 10 * n1 + *s++ - '0';
	    }
	    while (isdigit (*s));
	}
	else {
	    n1 = -9999;
	}
	n2 = n1;
	if (*s == '-') {
	    s++;
	    n2 = 0;
	    if (isdigit (*s)) {
		do {
		    n2 = 10 * n2 + *s++ - '0';
		}
		while (isdigit (*s));
	    }
	    else {
		n2 = 9999;
	    }
	}
	olist[nolist++] = n1;
	olist[nolist++] = n2;
	if (*s != '\0') {
	    s++;
	}
    }
    olist[nolist] = 0;
#ifdef DEBUG
    if (dbg)
	for (i = 0; i < nolist; i += 2)
	    printf ("%3d %3d\n", olist[i], olist[i + 1]);
#endif	    
}

private conv(fp)	/* convert a file */
register FILE *fp;
{
    register int    c, k;
    int     m, n, n1, m1;
    char    str[100], buf[300];
    int first = TRUE;
    
    while ((c = getc(fp)) != EOF) {
	if (first) {
	    preface();
	    first = FALSE;
	}
	switch (c) {
	    case '\n': case ' ': case '\0':
		break;
	    case '{': 		/* push down current environment */
		t_push();
		break;
	    case '}': 
		t_pop();
		break;
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9': 
	    /* two motion digits plus a character */
		hmot((c - '0') * 10 + getc (fp) - '0');
		lastcmd = HMOT;
		put1(getc(fp), (char *) 0);
		lastcmd = CPUT;
		break;
	    case 'c': 		/* single ascii character */
		put1(getc(fp), (char *) 0);
		lastcmd = CPUT;
		break;
	    case 'C': 
		fscanf(fp, "%99s", str);
		put1s(str);
		lastcmd = CPUT;
		break;
	    case 't': 		/* straight text */
		fgets(buf, sizeof (buf), fp);
		t_text (buf);
		lastcmd = CPUT;
		break;
	    case 'D': 		/* draw function */
		fgets(buf, sizeof (buf), fp);
		switch (buf[0]) {
		    case 'l': 	/* draw a line */
			sscanf (buf + 1, "%d %d", &n, &m);
			drawline (n, m);
			break;
		    case 'c': 	/* circle */
			sscanf (buf + 1, "%d", &n);
			drawcirc (n);
			break;
		    case 'e': 	/* ellipse */
			sscanf (buf + 1, "%d %d", &m, &n);
			drawellip (m, n);
			break;
		    case 'a': 	/* arc */
			sscanf (buf + 1, "%d %d %d %d", &n, &m, &n1, &m1);
			drawarc (n, m, n1, m1);
			break;
		    case '~': 	/* wiggly line */
			drawwig (buf + 1);
			break;
		    default:
			fprintf(stderr,"%s: unknown drawing function %s\n",
				prog,buf);
			exit(2);
			break;
		}
		break;
	    case 's': 
		fscanf (fp, "%d", &n);
		t_size (n);
		lastcmd = FNT;
		break;
	    case 'f': 
		fscanf (fp, "%99s", str);
		setfont (t_font (str));
		lastcmd = FNT;
		break;
	    case 'H': 		/* absolute horizontal motion */
		while ((c = getc (fp)) == ' ');
		k = 0;
		do {
		    k = 10 * k + c - '0';
		} while (isdigit (c = getc (fp)));
		ungetc (c, fp);
		hgoto (k);
		lastcmd = HMOT;
		break;
	    case 'h': 		/* relative horizontal motion */
		while ((c = getc (fp)) == ' ');
		k = 0;
		do {
		    k = 10 * k + c - '0';
		} while (isdigit (c = getc (fp)));
		ungetc (c, fp);
		hmot (k);
		lastcmd = HMOT;
		break;
	    case 'w':
		FlushShow(1);
		lastcmd = BRK;
		break;
	    case 'V': 
		fscanf (fp, "%d", &n);
		vgoto (n);
		lastcmd = VMOT;
		break;
	    case 'v': 
		fscanf (fp, "%d", &n);
		vmot (n);
		lastcmd = VMOT;
		break;
	    case 'p': 		/* new page */
		fscanf (fp, "%d", &n);
		t_page (n);
		lastcmd = NONE;
		break;
	    case 'n': 		/* end of line -- ignore */
		while ((c=getc (fp)) != '\n') {if (c==EOF) break;};
		FlushShow(1);
		lastcmd = BRK;
		break;
	    case '#': 		/* comment */
	    /* maybe should pass through as a PS comment */
		while ((c=getc (fp)) != '\n') {if (c==EOF) break;};
		break;
	    case 'x': 		/* device control */
		FlushShow(1);
		devcntrl (fp);
		break;
	    case '%':		/* imbedded PostScript */
	    /* copy everything up to but NOT including a line */
	    /* with at single "." */
	    FlushShow(0);MoveTo();DoMove();
	    printf("\n%% included PostScript\n");
	    while (fgets(buf, sizeof buf, fp) != NULL) {
		if (strcmp(".\n",buf) == 0) break;
		fputs(buf,stdout);
	    }
	    break;
	    default:
		fprintf(stderr,"%s: bad input char \\%03o (%c)\n",prog,c,c);
		exit(2);
		done ();
	}
    }
    if (first) {
	fprintf(stderr,"%s: empty input.\n",prog);
	exit(1);
    }
}


/* put in PostScript prolog */
private preface()
{
    register    FILE *prolog;
    char    hostname[256];
    char tempfile[512];
    struct passwd  *pswd;
    long    clock;
    char *libdir;
    
    fprintf (tf, "%%!%s\n", COMMENTVERSION);
    pswd = getpwuid (getuid ());
    VOIDC gethostname (hostname, sizeof hostname);
    fprintf(tf, "%%%%Creator: psdit\n");
    fprintf (tf, "%%%%For: %s:%s (%s)\n", hostname,
	    pswd->pw_name, pswd->pw_gecos);
    fprintf (tf, "%%%%Title: %s (ditroff)\n", infilename);
    fprintf (tf, "%%%%CreationDate: %s",
		(time (&clock), ctime (&clock)));
    fprintf(tf, "%%%%DocumentNeededResources: (atend)\n");
    fprintf(tf,"%%%%DocumentSuppliedResources: DIThacks\n");
    fprintf(tf,"%%%%Pages: (atend)\n");
    fprintf (tf, "%%%%EndComments\n");

    if ((libdir = envget("PSLIBDIR")) == NULL) libdir = PSLibDir;
    mstrcat(tempfile, libdir, prologfile, sizeof tempfile);
    if ((copyfile(tempfile, stdout)) != 0) {
	fprintf(stderr,"%s: can't copy prolog file %s\n",prog, tempfile);
	exit(2);
    }
    fprintf(tf,"%%%%EndProlog\n");
    fprintf(tf,"%%%%BeginSetup\n");
    printf ("ditstart\n");
}

private devcntrl(fp)	/* interpret device control functions */
FILE *fp;
{
    char    str[20], str1[50], buf[50];
    int     c, n, res, minh, minv;
    int last;
    char largebuf[128];
    char *nl;

    fscanf (fp, "%19s", str);
    switch (str[0]) {		/* crude for now */
	case 'i': 		/* initialize */
	    fileinit ();
	    t_init ();
	    lastcmd = NONE;
	    break;
	case 'T': 		/* device name */
	    fscanf (fp, "%19s", devname);
	    if (strcmp (devname, "psc")) {
		fprintf(stderr,"%s: device not psc\n",prog);
		exit(2);
	    }
	    printf ("(%s)xT\n", devname);
	    lastcmd = NONE;
	    break;
	case 't': 		/* trailer */
	    t_trailer ();
	    lastcmd = NONE;
	    break;
	case 'p': 		/* pause -- can restart */
	    t_reset ('p');
	    lastcmd = NONE;
	    break;
	case 's': 		/* stop */
	    t_reset ('s');
	    lastcmd = NONE;
	    break;
	case 'X': 		/* \X command from ditroff */
	    fscanf (fp, "%1s", str);
	    switch (str[0]) {
		case 'p' :
		    FlushShow(0);MoveTo();DoMove();
		    fgets(largebuf, sizeof(largebuf), fp);
		    last = strlen(largebuf) - 1;
		    if (last >= 0 && largebuf[last] == '\n') {
			ungetc('\n', fp);
			largebuf[last] = ' ';
		    }
		    fputs(largebuf, tf);
		    putc('\n', tf);
		    break;
		case 'f' :
		    FlushShow(0);MoveTo();DoMove();
		    if (fscanf(fp, "%127s", largebuf) == 1) {
			nl = strchr(largebuf, '\n');
			if (nl) *nl = '\0';
			includefile(largebuf);
		    } else
			fprintf(stderr, "warning - include cmd w/o path.\n");
		    break;
		default:	/* pass through arbitrary string */
		    puts(str);
		    if( (c = getc (fp)) != EOF )
			if( c != ' ' ) putchar (c);
		    while ((c = getc (fp)) != EOF) {
			if (c == '\n') {
			    ungetc ('\n', fp);
			    break;
			}
			putchar (c);
		    }
		    putchar ('\n');
		    lastcmd = NONE;
		    break;
		}
	    break;
	case 'r': 		/* resolution assumed when prepared */
	    fscanf (fp, "%d %d %d", &res, &minh, &minv);
	    t_res (res, minh, minv);
	    lastcmd = NONE;
	    break;
	case 'f': 		/* font used */
	    fscanf (fp, "%d %19s", &n, str);
	    fgets (buf, sizeof buf, fp);/* in case theres a filename */
	    ungetc ('\n', fp);	/* fgets goes too far */
	    str1[0] = 0;	/* in case there is nothing to come in */
	    sscanf (buf, "%s", str1);
	    loadfont (n, str, str1);
	    lastcmd = FNT;
	    break;
	case 'H': 		/* char height */
	    fscanf (fp, "%d", &n);
	    t_charht (n);
	    lastcmd = FNT;
	    break;
	case 'S': 		/* slant */
	    fscanf (fp, "%d", &n);
	    t_slant (n);
	    lastcmd = FNT;
	    break;
    }
    /* skip rest of input line */
    while ((c = getc (fp)) != '\n') {if (c == EOF) break;};
}

includefile(filenm)
char *filenm; {

	FILE *inf;
	int ch, c1, c2, firstch = 0;

	if (!(inf = fopen(filenm, "r"))) {
		fprintf(stderr, "psdit: fopen(%s): ", filenm);
		perror("");
		exit(1);
	}
	c1 = fgetc(inf); c2 = fgetc(inf);
	if (c1 != '%' || c2 != '!')
		fprintf(stderr, "psdit: %s not a postscript file.\n", filenm),
		exit(1);

	fputs("%!", tf);
	while ((ch = fgetc(inf)) != EOF) {
		fputc(ch, tf);
		if (firstch && ch == '%') {
			/* we have to double leading '%'s */
			fputc('%', tf);
		}
		firstch = (ch == '\n');
	}			
	fclose(inf);
}


private fileinit()	/* read in font and code files, etc. */
{
    int     i, fin, nw;
    char    *filebase, *p;
    char    temp[TSPATHSIZE];
    unsigned msize;

    /* open table for device,
     * read in resolution, size info, font info,   etc. and set params */

    sprintf (temp, "%s/dev%s/DESC.out", ditdir, devname);
    if ((fin = open (temp, 0)) < 0) {
	fprintf (stderr, "%s: can't open %s - %s\n", prog, devname, temp);
	pexit(prog,2);
    }
    if (read(fin,(char *)&dev, (int) sizeof(struct dev)) != sizeof(struct dev)) {
	fprintf (stderr, "%s: can't read %s\n", prog, temp);
	pexit(prog,2);
    }
    dres = dev.res;
    nfonts = dev.nfonts;
    nsizes = dev.nsizes;
    nchtab = dev.nchtab;
    /* enough room for whole file */
    filebase = malloc ((unsigned) dev.filesize);
    if (read (fin, filebase, dev.filesize) != dev.filesize) {
	fprintf (stderr, "%s: trouble reading %s\n", prog, temp);
	pexit(prog,2);
    }
    pstab = (short *) filebase;		/* point size table */
    chtab = pstab + nsizes + 1;		/* char index table */
    chname = (char *) (chtab + dev.nchtab);	/* char name table */
    p = chname + dev.lchname;		/* end of char name table */
    /* parse the preloaded font tables */
    for (i = 1; i <= nfonts; i++) {
	fontdelta[i] = 0;
	fontbase[i] = (struct font *) p;
	nw = *p & BMASK;	/* number of width entries */
	if ((smnt == 0) && (fontbase[i]->specfont == 1))
	    smnt = i;		/* first special font */
	p += sizeof (struct font); /* skip header */
	widthtab[i] = p;		/* width table */
					/* kern table is next */
	codetab[i] = p + 2 * nw;	/* device codes */
	fitab[i] = p + 3 * nw;		/* font index table */

	p += 3 * nw + dev.nchtab + (128 - 32);	/* next font */
	t_fp (i, fontbase[i]->namefont, fontbase[i]->intname);
	loadpswidths (i, fontbase[i]->namefont);
	sayload (i, fontbase[i]->namefont, (char *) 0);
#ifdef DEBUG
	if (fdbg > 1)
	    fontprint (i);
#endif
    }
    fontdelta[0] = 0;
    msize = 3*255 + dev.nchtab + (128-32) + sizeof (struct font);
    fontbase[0] = (struct font *) malloc(msize);
    widthtab[0] = (char *) fontbase[0] + sizeof (struct font);
    fontbase[0]->nwfont = 255;
    close (fin);
}

private loadpswidths(i,name)
int i;
char *name;
{
    char temp[TSPATHSIZE];
    register FILE *auxin;
    register int j;
    int cc, wid, funny;

    sprintf(temp, "%s/dev%s/%s.aux", ditdir, devname, name);
    if ((auxin = fopen(temp, "r")) == NULL) {
	fprintf(stderr,"%s: can't open font table %s\n", prog, temp);
	pexit(prog,2);
    }
    /* allocate table */
    if (pswidths[i] == NULL) {
	pswidths[i] = (int *) malloc(256 * (sizeof (int)));
    }
    /* initialize to not-there */
    for (j = 0; j <= 255; pswidths[i][j++] = -1);
    /* read them in */
    while (fscanf(auxin, "%d %d %d", &cc, &wid, &funny) != EOF) {
	pswidths[i][cc] = wid | (funny << 12);
    }
    VOIDC fclose(auxin);
}

#ifdef DEBUG
private fontprint(i)	/* debugging print of font i (0,...) */
int i;
{
    int     j, n;
    char   *p;

    printf ("font %d:\n", i);
    p = (char *) fontbase[i];
    n = fontbase[i]->nwfont & BMASK;
    printf ("base=0%o, nchars=%d, spec=%d, name=%s, widtab=0%o, fitab=0%o\n",
	    p, n, fontbase[i]->specfont,
	    fontbase[i]->namefont, widthtab[i], fitab[i]);
    printf ("widths:\n");
    for (j = 0; j <= n; j++) {
	printf (" %2d", widthtab[i][j] & BMASK);
	if (j % 20 == 19)
	    printf ("\n");
    }
    printf ("\ncodetab:\n");
    for (j = 0; j <= n; j++) {
	printf (" %2d", codetab[i][j] & BMASK);
	if (j % 20 == 19)
	    printf ("\n");
    }
    printf ("\nfitab:\n");
    for (j = 0; j <= dev.nchtab + 128 - 32; j++) {
	printf (" %2d", fitab[i][j] & BMASK);
	if (j % 20 == 19)
	    printf ("\n");
    }
    printf ("\n");
}
#endif

private loadfont(n, s, s1) /* load font info for font s on position n */
int n;
char *s, *s1;
{
    char    temp[TSPATHSIZE];
    int     fin, nw, norig;
    int     bcount;

    if (n < 0 || n > NFONT) {
	fprintf(stderr,"%s: illegal fp command %d %s\n", prog, n, s);
	exit(2);
    }
    if (strcmp(s, fontbase[n]->namefont) == 0) return;
    if (fontbase[n]->namefont != (char *)0) {
	fontdelta[n] = 1;
    }
    if (s1 == NULL || s1[0] == '\0') {
	sprintf (temp, "%s/dev%s/%s.out", ditdir, devname, s);
    }
    else {
	sprintf (temp, "%s/%s.out", s1, s);
    }
    if ((fin = open (temp, 0)) < 0) {
	fprintf(stderr,"%s: can't open font table %s\n", prog, temp);
	pexit(prog,2);
    }
    norig = fontbase[n]->nwfont & BMASK;
    bcount = 3 * norig + nchtab + 128 - 32 + sizeof (struct font);
    VOIDC read (fin, (char *)fontbase[n], bcount);
    if ((fontbase[n]->nwfont & BMASK) > norig) {
	fprintf(stderr,"%s: Font %s too big for position %d\n", prog, s, n);
	exit(2);
    }
    close (fin);
    nw = fontbase[n]->nwfont & BMASK;
    widthtab[n] = (char *) fontbase[n] + sizeof (struct font);
    codetab[n] = (char *) widthtab[n] + 2 * nw;
    fitab[n] = (char *) widthtab[n] + 3 * nw;
    t_fp (n, fontbase[n]->namefont, fontbase[n]->intname);
    loadpswidths (n, fontbase[n]->namefont);
    sayload (n, s, s1);
    fontbase[n]->nwfont = norig; /* so can later use full original size */
#ifdef DEBUG
    if (fdbg > 1)
	fontprint (n);
#endif
}

private sayload(n, s, s1)	/* position n contains font s (internal s1) */
int n;
char *s, *s1;
{
    char pass[TSPATHSIZE];
    FILE *ptrfile;
    char Adobefont[256];
    int i;

    if (s1 == NULL || s1[0] == '\0') {
	sprintf(pass, "%s/dev%s/%s.map", ditdir, devname, s);
    }
    else {
	sprintf(pass, "%s/%s.map", s1, s);
    }

    if ((ptrfile = fopen(pass, "r")) == NULL) {
	fprintf(stderr, "%s: can't open font map file %s\n", prog, pass);
	pexit(prog, 2);
    }

    fscanf(ptrfile, "%255s", Adobefont);
    FlushShow(0);
    if (!strcmp(Adobefont, "DIThacks"))
	printf("%d(%s)xf %d f\n", n, Adobefont, n);
    else {
	printf("%%%%IncludeResource: font %s\n", Adobefont);
	printf("%d(%s)xf %d f\n", n, Adobefont, n);
	for (i = 0; i < nfontlist; i++)
	    if (!strcmp(Adobefont, fontlist[i]))
		break;
	if (i == nfontlist)
	    strcpy(fontlist[nfontlist++], Adobefont);
    }
    font = n;
    VOIDC fclose(ptrfile);
}

private VOID done()
{
    if (tf == NULL)
	exit (1);
    t_reset ('s');
    exit (0);
}

private t_init()	/* "x i" - initialize device */
{
    int i;
    int tmpl;
    int tmpw;

    movepending = NONE;
    savex = savey = 0;

    t_size (10);		/* start somewhere */
    t_slant (0);
    setfont (1);		/* set font */
    tmpl = ((float) dev.paperlength/(float) dev.res) *72;
    tmpw = ((float) dev.paperwidth/(float) dev.res) *72;
    for (i = 0; i < NSIZES; i++) {
	if (abs(tmpl - pagesizes[i].height) < 10 &&
	    abs(tmpw - pagesizes[i].width) < 10 ) {
	    fprintf(tf, "%%%%IncludeFeature: *PageSize %s\n",
		    pagesizes[i].name);
	    break;
	}
	/* might be rotated */
	if (abs(tmpl - pagesizes[i].width) < 10 &&
	    abs(tmpw - pagesizes[i].height) < 10) {
	    fprintf(tf, "%%%%IncludeFeature: *PageSize %s\n",
		    pagesizes[i].name);
	    break;
	}
    }
    printf("xi\n");
    printf("%%%%EndSetup\n");
}

private t_push()	/* begin a new block */
{
    FlushShow(1);MoveTo();DoMove();
    if (dlevel == DSTACK) {
	fprintf(stderr,"%s: ditroff push/pop overflow!\n",prog);
	exit(2);
    }
    ditstack[dlevel].hpos = hpos;
    ditstack[dlevel].vpos = vpos;
    ditstack[dlevel].fontsize = fontsize;
    ditstack[dlevel].fontheight = fontheight;
    ditstack[dlevel].fontslant = fontslant;
    ditstack[dlevel].font = font;
    dlevel++;
    printf ("\nditpush\n");
}

private t_pop()	/* pop to previous state */
{
    FlushShow(1);MoveTo();DoMove();
    if (dlevel == 0) {
	fprintf(stderr,"%s: ditroff push/pop underflow!\n",prog);
	exit(2);
    }
    dlevel--;
    hpos = ditstack[dlevel].hpos;
    vpos = ditstack[dlevel].vpos;
    fontsize = ditstack[dlevel].fontsize;
    fontheight = ditstack[dlevel].fontheight;
    fontslant = ditstack[dlevel].fontslant;
    font = ditstack[dlevel].font;
    printf ("%d s %d xH %d xS %d f\n",fontsize,fontheight,fontslant,font);
    startx = savex = hpos;
    savey = vpos;
    PSx = hpos * PSmag;
    PSy = vpos * PSmag;
    printf("%d %d MXY\n",savex,savey);
    movepending = NONE;
    printf("\nditpop\n");
}

private t_page(n)	/* do whatever new page functions */
{
    int     i;

    if (output) {
	if (++scount >= spage) {
	    t_reset ('p');
	    scount = 0;
	}
    }
    output = 1;
    FlushShow(0);
    if (!firstpage) {
	printf("\n%d p",n);
    }
    firstpage = FALSE;
    printf ("\n%%%%Page: %d %d\n", n, ++pageno, n);
    printf("%5.2f ps\n", ((float)dev.paperlength)/ (float)dev.res);
    for (i = 0; i <= nfonts; i++) {
	if (fontdelta[i] != 0) {
	    sayload (i, fontname[i].name, (char *) 0);
	}
    }
    vpos = 0;
    PSy = 0;
    printf ("%d s %d xH %d xS %d f\n",fontsize,fontheight,fontslant,font);
    if (nolist == 0)
	return;
    output = 0;
    for (i = 0; i < nolist; i += 2)
	if (n >= olist[i] && n <= olist[i + 1]) {
	    output = 1;
	    break;
	}
}

private t_size(n)	/* convert integer to internal size number*/
int n;
{
    FlushShow(1);
    if (fontsize != n) {
	fontsize = n;
	fontheight = n;
	printf("%d s\n",fontsize);
    }
}

private t_charht(n)	/* set character height to n */
int n;
{
    FlushShow(1);
    if (fontheight != n) {
	fontheight = n;
	printf("%d xH\n",fontheight);
    }
}

private t_slant(n)	/* set slant to n */
int n;
{
    FlushShow(1);
    if (fontslant != n) {
	fontslant = n;
	printf("%d xS\n",fontslant);
    }
}

private t_font(s)	/* convert string to internal font number */
char *s;
{
    int     n;

    n = atoi (s);
    if (n < 0 || n > nfonts) n = 1;
    return (n);
}

private t_text(s)	/* print string s as text??? */
char *s;
{
	fprintf(stderr,"%s: ditroff t <%s> unimplemented!\n",prog,s);
}

private t_reset(c)
{
    output = 1;			/* by God */
    if (c == 'p') {
	printf ("\nxp\n");
    }
    else {
	if (!stopped)
	    printf ("\nxs\n");
	stopped = 1;
    }
    fflush (tf);
}

private t_res(res, minh, minv)
int res, minh, minv;
{
    resolution = res;
    minhoriz = minh;
    minvert = minv;
    printf ("%d %d %d xr\n", res, minh, minv);
}

private t_trailer()
{
    int i;
    int cnt;

    FlushShow(0);
    printf("\n%d p",pageno);
    printf("\n%%%%Trailer\n");
    printf("xt\n");
    printf("%%%%Pages: %d\n", pageno);
    printf("%%%%DocumentNeededResources: font ");
    cnt = 32;
    for (i = 0; i < nfontlist; i++) {
	if (!strcmp(fontlist[i],"DIThacks"))
	    continue;
	cnt += strlen(fontlist[i]) + 1;
	if (cnt > 80) {	/* looks better this way */
	    printf("\n%%%%+ ");
	    cnt = 4;
	}
	printf("%s ", fontlist[i]);
    }
    printf("\n");
}

private put1s(s)	/* s is a funny char name */
char *s;
{
    int     i;

    if (!output) return;
    debugp(("%s ", s));

    /* search for s in the funny char name table */
    for (i = 0; i < nchtab; i++) {
	if (strcmp(&chname[chtab[i]], s) == 0) break;
    }

    if (i < nchtab) {
	put1(i + 128, s);
    }
    else {
	debugp(("not found "));
	putnf (0, s);
    }
}

#define needsescape(c) ((c=='\\') || (c=='(') || (c==')'))

private put1(c, s)	/* output char c */
int c;
char *s;
{
    char *pw;
    register char *p;
    register int i, k;
    register int cc;
    int ofont, code;
    int psinfo, pswid, tw;

    if (!output) return;
    if (c == 32) {
	thisw = 0;
	FlushShow(0);
	return;
    }
    if (c < 32) {
	debugp(("non-exist 0%o\n",c));
	return;
    }

    c -= 32;	/* offset char code */
    k = ofont = pfont = font;
    if (onspecial) pfont = prevfont;

    if ((i = (fitab[pfont][c] & BMASK)) != 0) {/* char on this font */
	p = codetab[pfont];
	pw = widthtab[pfont];
	if (onspecial) {
	    setfont(prevfont);
	    thisw = 0;
	    onspecial = 0;
	}
    }
    else if (smnt > 0) {	/* on special (we hope) */
	for (k = smnt; k <= nfonts; k += 1)
	    if ((i = (fitab[k][c] & BMASK)) != 0) {
		p = codetab[k];
		pw = widthtab[k];
		prevfont = pfont;
		if (onspecial && (k == specfont)) break;
		setfont (k);
		thisw = 0;
		onspecial = 1;
		specfont = k;
		break;
	    }
    }
    if ((i == 0) || (k > nfonts) || ((code = p[i] & BMASK) == 0)) {
	debugp(("not found 0%o\n", c+32));
	putnf (c + 32, s);
	return;
    }
    /* when we get here,
     *  c == biased character code
     *	k == font number
     *  i == index into codetab and widthtab for this character
     *  p == codetab for this font
     *  pw == width tab for this font
     *  code == character code for this char
     */

    cc = c + 32;
    debugp(((isascii(cc) && isprint(cc)) ? "%c %d\n":"%03o %d\n",
    		cc, code));
    psinfo = pswidths[font][code];	/* PS specific char info */
    pswid = psinfo & PSWID;		/* PS character width */
    thisw = pw[i] & BMASK;		/* troff char width */
    tw = thisw = (thisw * fontsize + dev.unitwidth/2) / dev.unitwidth;

    if ((psinfo & ISPSPROC) && (psinfo != -1)) {
	/* character is implemented by a PostScript proc */
	showspecial(s, code, pswid);
	if (pswid > 0) {
	    PSx += PSscale(pswid * fontsize * dres);
	}
	thisw = 0;
    }
    else {
	showchar(code);
	if (pswid > 0) {
	    PSshowlen += PSscale(pswid * fontsize * dres);
	}
    }

/*
    if (font != ofont) {
	setfont(ofont);
	startx = hpos + tw;
	thisw = 0;
	lastcmd = FNT;
    }
*/    
    debugp(("...width (%d)\n", pw[i]&BMASK));
}


private putnf(c, s)	/* note that a character wasnt found */
int c;
char *s;
{

    FlushShow(0);
    thisw = 0;
    if ((s == NULL) || (*s == '\0')) printf("(%3o)cb\n", c);
    else if ((strcmp(s, "\\|") == 0) || (strcmp(s, "\\^") == 0)
    || (strcmp (s, "\\&") == 0))
	return;
    else
	printf("(%s)cb\n", s);
}


private t_fp(n, s, si)	/* font position n now contains font s, intname si */
int n;		/* position */
char *s;	/* font (ditname) */
char *si;	/* font (intname = number) */
{
    fontname[n].name = s;
    fontname[n].number = atoi (si);
}

private setfont(n)	/* set font to n */
int n;
{
    FlushShow(1);

    if (n < 0 || n > NFONT) {
	fprintf(stderr,"%s: illegal font %d\n", prog,n);
    }
    if (font != n) {
	font = n;
	printf("%d f\n",font);
    }
    onspecial = 0;
}

private drawline(dx, dy)	/* draw line from here to dx, dy */
int dx, dy;
{
    FlushShow(0); MoveTo(); DoMove();
    if (!output) return;
    printf("%d %d Dl\n", dx, dy);
    hpos += dx;
    vpos += dy;
    PSx = hpos * PSmag;
    PSy = vpos * PSmag;
}

private drawwig(s)	/* draw wiggly line */
char *s;
{
    register char *p;

    FlushShow(0); MoveTo(); DoMove();
    if (!output) return;
    printf("D~ %s D~~\n",s);
    /* update position */
    p = s;
    while (1) {
	while (*p == ' ') p++;
	hpos += atoi(p);
	while (isdigit(*++p));
	while (*++p == ' ');
	vpos += atoi(p);
	debugp((stderr,"%d %d\n",hpos,vpos));
	while (isdigit(*++p));
	while (*p == ' ') p++;
	if (*p == '\n') break;
    }
    PSx = hpos * PSmag;
    PSy = vpos * PSmag;
}

private drawcirc(d)
int d;
{
    FlushShow(0); MoveTo(); DoMove();
    if (!output) return;
    printf("%d Dc\n",d);
    hpos += d;
    PSx = hpos * PSmag;
 }

private drawarc(dx1, dy1, dx2, dy2)
int dx1, dy1, dx2, dy2;
{
    FlushShow(0); MoveTo(); DoMove();
    if (!output) return;
    printf("%d %d %d %d Da\n", dx1, dy1, dx2, dy2);
    hpos += dx1 + dx2;
    vpos += dy1 + dy2;
    PSx = hpos * PSmag;
    PSy = vpos * PSmag;
}

private drawellip(a, b)
int a, b;
{
    FlushShow(0);MoveTo();DoMove();
    printf("%d %d De\n",a,b);
    hpos += a;
    PSx = hpos * PSmag;
}

private hmot(a)	/* relative horizontal motion */
int a;
{
    register int aa;
    aa = abs(a);
    if ((aa < 8) || (aa > (10 * thisw)) || (a >= 100)
    || ((thisw != 0) && (abs(thisw - a) > 4))) {
	FlushShow(1);
    }
    hpos += a;
    if (lastcmd != CPUT) startx = hpos;
}

private hgoto(a) /* absolute horizontal motion */
int a;
{
    FlushShow(1);
    startx = hpos = a;
    thisw = 0;
}

private vmot(a) /* relative vertical motion */
int a;
{
    FlushShow(1);
    vpos += a;
    thisw = 0;
}

private vgoto(a) /* absolute vertical motion */
int a;
{
    FlushShow(1);
    vpos = a;
    thisw = 0;
}

private showspecial(s,cc,wid)
char *s;
int cc;
int wid;
{
    char *sp;

    FlushShow(0);
    MoveTo();
    DoMove();
    putchar('(');
    for (sp = s; *sp != '\0'; sp++) {
	if (needsescape(*sp)) {
	    putchar('\\');
	}
	putchar(*sp);
    }
    printf(")%d %d oc\n",cc,wid);
}

private showchar(c)
int c;
{
    int savethisw;          /* Save current thisw value */
    static int lasthpos;    /* Saved value of hpos from last char */
    static int lastthisw;   /* Saved value of thisw from last char */

    if (showind == 0) {MoveTo();}
    else if ((vpos * PSmag) != PSy || lasthpos == hpos) {
	savethisw = thisw;
	thisw = lastthisw;
	FlushShow(0);
	MoveTo();
	thisw = savethisw;
    }
    if (showind >= SHOWSIZE) FlushShow(0);
    if (isascii(c) && isprint(c)) {
	switch (c) {
	    case '\\': case '(': case ')':
	        showbuf[showind++] = '\\';
		/* fall through */

	    default:
		showbuf[showind++] = c;
	}
    }
    else {
	showbuf[showind++] = '\\';
	showbuf[showind++] = ((c>>6)&03) + '0';
	showbuf[showind++] = ((c>>3)&07) + '0';
	showbuf[showind++] = (c&07) + '0';
    }
    showbuf[showind] = '\0';
    nshow++;
    lasthpos = hpos;
    lastthisw = thisw;
}

private MoveTo() {
    int x, y;
    x = hpos * PSmag;
    y = vpos * PSmag;

    if (x != PSx) {
	startx = savex = hpos;
	PSx = x;
	movepending |= XMOVE;
    }
    if (y != PSy) {
	savey = vpos;
	PSy = y;
	movepending |= YMOVE;
    }
}

private FlushMove() {
    switch (movepending) {
	case NONE:
	    break;
	case XMOVE:
	    printf("%d",savex);
	    break;
	case YMOVE:
	    printf("%d",savey);
	    break;
	case XYMOVE:
	    printf("%d %d",savex,savey);
	    break;
	default:
	    fprintf(stderr,"%s: invalid move code %d\n",prog, movepending);
	    exit(2);
    }
}

private char *movecmds[] = {
    "MX","MY","MXY"
};
    
private DoMove() {
    FlushMove();
    if (movepending != NONE) {
	printf(" %s\n",movecmds[movepending-1]);
	movepending = NONE;
    }
}

private char showops[] = "SXYN";

private FlushShow(t) int t; {
    long err, tlen;
    float cerror;
    
    if (showind == 0) {thisw = 0; return;}
    if (movepending != NONE) {
	FlushMove();
    }
    tlen = hpos - startx;
    if (lastcmd == CPUT) tlen += thisw;
    err = tlen * PSmag - PSshowlen;
    if ((nshow != 1) && (abs(err) > ErrorTolerance)) {
	cerror = ((float) err) / ((nshow - 1) * PSmag);
#ifdef DEBUG
	fprintf(stderr,"F%d lc %d thisw %d ",t,lastcmd,thisw);
        fprintf(stderr,"x %ld h %ld tn %ld %ld ",
		startx, hpos, tlen*PSmag,PSshowlen);
	fprintf(stderr,"error %d %.4f %s\n",nshow,cerror,showbuf);
	fflush(stderr);
#endif
	printf(" %.4f(%s)A%c\n", cerror, showbuf, showops[movepending]);
    }
    else {
	printf("(%s)%c\n", showbuf, showops[movepending]);
    }

    showind = 0;
    nshow = 0;
    showbuf[showind] = '\0';
    PSx += PSshowlen;
    PSshowlen = 0;
    startx = hpos;
    if (lastcmd == CPUT) startx += thisw;
    thisw = 0;
    movepending = NONE;
}
