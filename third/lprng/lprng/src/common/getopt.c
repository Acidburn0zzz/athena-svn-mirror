/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-1999, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************/

 static char *const _id =
"$Id: getopt.c,v 1.6 2001-03-07 01:19:22 ghudson Exp $";


#include "lp.h"
/**** ENDINCLUDE ****/

int Optind;                 /* next argv to process */
int Opterr = 1;                 /* Zero disables errors msgs */
char *Optarg;               /* Pointer to option argument */
char *next_opt;			    /* pointer to next option char */
char *Name;					/* Name of program */
char **Argv_p;
int Argc_p;

int Getopt (int argc, char *argv[], char *optstring)
{
	int  option;               /* current option found */
	char *match;                /* matched option in optstring */

	if( argv == 0 ){
		/* reset parsing */
		next_opt = 0;
		Optind = 0;
		return(0);
	}

	if (Optind == 0 ) {
		char *basename;
		/*
		 * set up the Name variable for error messages
		 * setproctitle will change this, so
		 * make a copy.
		 */
		if( Name == 0 ){
			if( argv[0] ){
				if( (basename = strrchr( argv[0], '/' )) ){
					++basename;
				} else {
					basename = argv[0];
				}
				Name = basename;
			} else {
				Name = "???";
			}
		}
		Argv_p = argv;
		Argc_p = argc;
		Optind = 1;
	}

	while( next_opt == 0 || *next_opt == '\0' ){
		/* No more arguments left in current or initial string */
		if (Optind >= argc){
		    return (EOF);
		}
		next_opt = argv[Optind++];
	}

	/* check for start of option string AND no initial '-'  */
	if( (next_opt == argv[Optind-1]) ){
		if( next_opt[0] != '-' || next_opt[1] == '\0' ){
			--Optind;
			return( EOF );
		} else {
			++next_opt;
			if( next_opt[0] == 0 ){
				return( EOF );
			}
		}
	}
	option = *next_opt++;
	/*
	 * Case of '--',  Force end of options
	 */
	if (option == '-') {
		return ( EOF );
	}
	/*
	 * See if option is in optstring
	 */
	if ((match = (char *) strchr (optstring, option)) == 0 ){
		if( Opterr ){
		    (void) fprintf (stderr, "%s: Illegal option '%c'\n", Name, option);
		}
		return( '?' );
	}
	/*
	 * Argument?
	 */
	if (match[1] == ':') {
		/*
		 * Set Optarg to proper value
		 */
		Optarg = 0;
		if (*next_opt != '\0') {
		    Optarg = next_opt;
		} else if (Optind < argc) {
		    Optarg = argv[Optind++];
		    if (Optarg != 0 && *Optarg == '-') {
				Optarg = 0;
			}
		}
		if( Optarg == 0 && Opterr ) {
			(void) fprintf (stderr,
				"%s: missing argument for '%c'\n", Name, option);
			option = '?';
		}
		next_opt = 0;
	} else if (match[1] == '?') {
		/*
		 * Set Optarg to proper value
		 */
		if (*next_opt != '\0') {
		    Optarg = next_opt;
		} else {
		    Optarg = 0;
		}
		next_opt = 0;
	}
	return (option);
}
