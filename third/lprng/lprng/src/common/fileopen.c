/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-1999, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************/

 static char *const _id =
"$Id: fileopen.c,v 1.2 2001-03-07 01:19:22 ghudson Exp $";


#include "lp.h"
#include "fileopen.h"
#include "errorcodes.h"
#include "child.h"
/**** ENDINCLUDE ****/

/***************************************************************************
Commentary:
Patrick Powell Mon May  1 05:37:02 PDT 1995
 
These routines were created in order to centralize all file open
and checking.  Hopefully,  if there are portability problems, these
routines will be the only ones to change.
 
 ***************************************************************************/

/***************************************************************************
 * int Checkread( char *file, struct stat *statb )
 * open a file for reading,  and check its permissions
 * Returns: fd of open file, -1 if error.
 ***************************************************************************/

int Checkread( const char *file, struct stat *statb )
{
	int fd = -1;
	int status = 0;
	int err = 0;

	/* open the file */
	DEBUG3("Checkread: file '%s'", file );

	if( (fd = open( file, O_RDONLY|O_NOCTTY, Spool_file_perms_DYN ) )< 0 ){
		status = -1;
		err = errno;
		DEBUG3( "Checkread: cannot open '%s', %s", file, Errormsg(err) );
	}

    if( status >= 0 && fstat( fd, statb ) < 0 ) {
		err = errno;
        logerr(LOG_ERR,
		"Checkread: fstat of '%s' failed, possible security problem", file);
        status = -1;
    }

	/* check for a security loophole: not a file */
	if( status >= 0 && !(S_ISREG(statb->st_mode))){
		/* AHA!  not a regular file! */
		DEBUG3( "Checkread: '%s' not regular file, mode = 0%o",
			file, statb->st_mode );
		status = -1;
	}

	if( status < 0 ){
		close( fd );
		fd = -1;
	}
	DEBUG3("Checkread: '%s' fd %d", file, fd );
	errno = err;
	return( fd );
}


/***************************************************************************
 * int Checkwrite( char *file, struct stat *statb, int rw, int create,
 *  int nodelay )
 *  - if rw != 0, open for both read and write
 *  - if create != 0, create if it does not exist
 *  - if nodelay != 0, use nonblocking open
 * open a file or device for writing,  and check its permissions
 * Returns: fd of open file, -1 if error.
 *     status in *statb
 ***************************************************************************/
int Checkwrite( const char *file, struct stat *statb, int rw, int create,
	int nodelay )
{
	int fd = -1;
	int status = 0;
	int options = O_NOCTTY|O_APPEND;
	int mask, oldumask;
	int err = errno;

	/* open the file */
	DEBUG3("Checkwrite: file '%s', rw %d, create %d, nodelay %d",
		file, rw, create, nodelay );

	memset( statb, 0, sizeof( statb[0] ) );
	if( nodelay ){
		options |= NONBLOCK;
	}
	if( rw ){
		options |= rw;
	} else {
		options |= O_WRONLY;
	}
	if( create ){
		options |= O_CREAT;
	}
	/* turn off umask */
	oldumask = umask( 0 ); 
	fd = open( file, options, Spool_file_perms_DYN );
	err = errno;
	umask( oldumask );
	if( fd < 0 ){
		status = -1;
		DEBUG3( "Checkwrite: cannot open '%s', %s", file, Errormsg(err) );
	} else if( nodelay ){
		/* turn off nonblocking */
		mask = fcntl( fd, F_GETFL, 0 );
		if( mask == -1 ){
			logerr(LOG_ERR, "Checkwrite: fcntl F_GETFL of '%s' failed", file);
			status = -1;
		} else {
			DEBUG3( "Checkwrite: F_GETFL value '0x%x', BLOCK 0x%x",
				mask, NONBLOCK );
			mask &= ~NONBLOCK;
			mask = fcntl( fd, F_SETFL, mask );
			if( mask == -1 ){
				logerr(LOG_ERR, "Checkwrite: fcntl F_SETFL of '%s' failed",
					file );
				status = -1;
			}
			DEBUG3( "Checkwrite: after F_SETFL value now '0x%x'",
				fcntl( fd, F_GETFL, 0 ) );
		}
	}

    if( status >= 0 && fstat( fd, statb ) < 0 ) {
		err = errno;
        logerr_die(LOG_ERR, "Checkwrite: fstat of '%s' failed, possible security problem", file);
        status = -1;
    }

	/* check for a security loophole: not a file */
	if( status >= 0 && (S_ISDIR(statb->st_mode))){
		/* AHA!  Directory! */
		DEBUG3( "Checkwrite: '%s' directory, mode 0%o",
			file, statb->st_mode );
		status = -1;
	}
	if( fd == 0 ){
		int tfd;
		tfd = dup(fd);
		err = errno;
		if( tfd < 0 ){
			logerr(LOG_ERR, "Checkwrite: dup of '%s' failed", file);
			status = -1;
		} else {
			close(fd);
			fd = tfd;
		}
    }
	if( status < 0 ){
		close( fd );
		fd = -1;
	}
	DEBUG2("Checkwrite: file '%s' fd %d, inode 0x%x, perms 0%o",
		file, fd, statb->st_ino, statb->st_mode );
	errno = err;
	return( fd );
}

/***************************************************************************
 * int Checkwrite_timeout(int timeout, ... )
 *  Tries to do Checkwrite() with a timeout 
 ***************************************************************************/
int Checkwrite_timeout(int timeout,
	const char *file, struct stat *statb, int rw, int create, int nodelay )
{
	int fd;
	if( Set_timeout() ){
		Set_timeout_alarm( timeout );
		fd = Checkwrite( file, statb, rw, create, nodelay );
	} else {
		fd = -1;
	}
	Clear_timeout();
	return(fd);
}
