/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-1997, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************
 * MODULE: sendjob.c
 * PURPOSE: Send a print job to the remote host
 *
 **************************************************************************/

static char *const _id =
"sendjob.c,v 3.24 1998/03/29 18:32:54 papowell Exp";

#include "lp.h"
#include "printcap.h"
#include "errorcodes.h"
#include "fileopen.h"
#include "fixcontrol.h"
#include "jobcontrol.h"
#include "killchild.h"
#include "linksupport.h"
#include "malloclist.h"
#include "pr_support.h"
#include "printjob.h"
#include "readstatus.h"
#include "sendauth.h"
#include "sendjob.h"
#include "setstatus.h"
#include "setup_filter.h"
/**** ENDINCLUDE ****/

static int Bounce_filter( struct control_file *cfp,
	struct data_file *data_file, struct printcap_entry *printcap_entry,
	int fd, int *size, struct stat *statb, int transfer_timeout );

/***************************************************************************
Commentary:
The protocol used to send a job to a remote host consists of the
following:

Client                                   Server
\2printername\n - receive a job
                                         \0  (ack)
\2count controlfilename\n
<count bytes>
\0
                                         \0
\3count datafilename\n
<count bytes>
\0
                                         \0
\3count datafilename\n
<count bytes>
\0
                                         \0
<close connection>

1. In order to abort the job transfer,  client sends \1
2. Anything but a 0 ACK is an error indication

NB: some spoolers require that the data files be sent first.
The same transfer protocol is followed, but the data files are
send first,  followed by the control file.

The Send_job() routine will try to transfer a control file
to the remote host.  It does so using the following algorithm.

1.  makes a connection (connection timeout)
2.  sends the \2printer and gets ACK (transfer timeout)
3.  sends the control file (transfer timeout)
4.  sends the data files (transfer timeout)

int Send_job(
	char *printer,	- name of printer
	char *host,		- remote host name
	struct dpathname *p - pathname to spool directory
	struct control_file *cfp,	- control file
	int connect_timeout,	- timeout on making connection
	int connect_interval,	- interval between retries
	int max_connect_interval - maximum connection interval
	int transfer_timeout )	- timeout on sending a file

	RETURNS: 0 if successful, non-zero if not
 **************************************************************************/
extern int Send_files( char *host, char *printer,
	struct dpathname *dpath,
	int *sock, struct control_file *cfp, int transfer_timeout,
	struct printcap_entry *printcap_entry, int block_fd );

extern int Send_block( char *host, char *printer,
	struct dpathname *dpath,
	int *sock, struct control_file *cfp, int transfer_timeout,
	struct printcap_entry *printcap_entry );

extern int Send_secure_block( char *host, char *printer,
	struct dpathname *dpath,
	int *sock, struct control_file *cfp, int transfer_timeout,
	struct printcap_entry *printcap_entry );

int Send_job( char *printer, char *host, struct control_file *cfp,
	struct dpathname *dpath,
	int connect_timeout, int connect_interval, int max_connect_interval,
	int transfer_timeout, struct printcap_entry *printcap_entry )
{
	int sock = -1;		/* socket to use */
	int i, n, errcount = 0;	/* AJAX Integers (ACME on strike) */
	int status = 0;	/* status of transfer */
	char msg[LINEBUFFER];
	char bannerpath[MAXPATHLEN];
	int err;
	int tempfd;
	char *id;
	char *s;
	struct data_file *df, *dfp;	/* data file entry */
 
	/* make sure only temp files are removed on exit */
	Cfp_static = cfp;

	/* now we have a banner, is it at start or end? */
	if( Generate_banner && (Is_server || Lpr_bounce )
		&& (Always_banner || (!Suppress_header && cfp->BNRNAME)) ){
		DEBUG2( "Send_job: Banner_start '%s', Banner_printer '%s'",
			Banner_start, Banner_printer );
		if( Always_banner && cfp->BNRNAME == 0 ){
			cfp->BNRNAME = cfp->LOGNAME;
			if( cfp->BNRNAME == 0 ){
				cfp->BNRNAME = "ANONYMOUS";
			}
		}
		s = Banner_printer;
		if( s == 0 ) s = Banner_start;
		if( s ){
			tempfd = Make_temp_fd( bannerpath, sizeof(bannerpath) );

			DEBUG2( "Send_job: banner path '%s'", bannerpath );

			/* fix up the control file */
			if( cfp->data_file_list.count+1 >= cfp->data_file_list.max ){
				extend_malloc_list(&cfp->data_file_list, sizeof(df[0]),
					1,__FILE__,__LINE__);
			}
			dfp = (void *)cfp->data_file_list.list;
			if( Banner_last ){
				df = &dfp[cfp->data_file_list.count];
			} else {
				for(i=cfp->data_file_list.count; i > 0; --i ){
					dfp[i] = dfp[i-1];
				}
				df = &dfp[0];
			}
			++cfp->data_file_list.count;
			memset(df,0,sizeof(df[0]));

			/* add control file line */
			safestrncpy( df->Ninfo, "N(banner)" );
			plp_snprintf(df->cfline, sizeof(df->cfline),
				"fdfA%0*d%s", cfp->number_len, cfp->number, cfp->filehostname );
			safestrncpy( df->openname, bannerpath );
			if( Print_banner( s, cfp, 0, tempfd, 0, 0, printcap_entry ) ){
				Errorcode = JABORT;
				fatal( LOG_INFO, "Send_job: banner generation failed" );
			}
			if( fstat( tempfd, &df->statb ) < 0 ){
				logerr_die( LOG_INFO,
					"Send_job: fstat of temp file '%s' failed",
					df->openname );
			}
			close(tempfd);
		}
	}

	/* fix up the control file */
	if( Fix_control( cfp, printcap_entry ) ){
		Errorcode = JABORT;
		if( cfp->error[0] ){
			fatal( LOG_INFO, "Send_job: %s", cfp->error );
		} else {
			fatal( LOG_INFO, "Send_job: Fix_control failed" );
		}
	}

	/*
	 * do accounting at start
	 */
	id = cfp->identifier;
	if( id[0] == 0 ){
		id = cfp->transfername;
	} else {
		++id;
	}
	if( Is_server && printcap_entry && Accounting_remote ){
		Setup_accounting( cfp, printcap_entry );
		if( Accounting_start ){
			i = Do_accounting( 0, Accounting_start, cfp, transfer_timeout,
				printcap_entry, 0 );
			if( i ){
				Errorcode = i;
				cleanup( 0 );
			}
		}
	}

	DEBUG3("Send_job: '%s'->'%s@%s',connect(timeout %d,interval %d)",
		cfp->transfername, printer, host, connect_timeout, connect_interval );

	setstatus( cfp, "start job '%s' transfer to %s@%s", id, printer, host );

	/* we check if we need to do authentication */
	Fix_auth();

	setstatus( cfp,
	"sending job '%s' to '%s@%s'", id, printer, host );
	/* we pause on later tries */
retry_connect:
	setstatus( cfp, "connecting to '%s', attempt %d",
		host, errcount+1 );
	if( Network_connect_grace > 0 ){
		plp_sleep( Network_connect_grace );
	}

	errno = 0;
	sock = Link_open( host, connect_timeout );
	err = errno;
	DEBUG4("Send_job: socket %d", sock );
	if( sock < 0 ){
		++errcount;
		status = LINK_OPEN_FAIL;
		plp_snprintf(msg,sizeof(msg)-2,
			"connection to '%s' failed - %s", host, Errormsg(err) );
		setstatus( cfp, msg );
		if( Interactive ){
			/* send it directly to the error output */
			strcat(msg,"\n");
			if( Write_fd_str( 2, msg ) < 0 ) cleanup(0);
		} else if( Is_server && Retry_NOLINK ){
			if( connect_interval > 0 ){
				n = (connect_interval * (1 << (errcount - 1)));
				if( max_connect_interval && n > max_connect_interval ){
					n = max_connect_interval;
				}
				if( n > 0 ){
					setstatus( cfp, _("sleeping %d secs before retry, starting sleep"),n );
					plp_sleep( n );
				}
			}
			goto retry_connect;
		}
	} else {
		/* send job to the LPD server for the printer */
		DEBUG2("Send_job: Is_server %d, Forward_auth '%s', auth '%s'",
			Is_server, Forward_auth, cfp->auth_id );
		if( (Is_server && Forward_auth && cfp->auth_id[0])
			|| ( !Is_server && ( Use_auth || Use_auth_flag) ) ){
			status = Send_secure_block( host, printer,
			dpath, &sock, cfp, transfer_timeout, printcap_entry );
		} else if( Send_block_format ){
			status = Send_block( host, printer,
			dpath, &sock, cfp, transfer_timeout, printcap_entry );
		} else {
			status = Send_files( host, printer,
			dpath, &sock, cfp, transfer_timeout, printcap_entry, 0 );
		}
		/* we read any other information that is sent */
		DEBUG2("Send_job: sending status %d, sock %d", status, sock );
		if( sock > 0 ){
			Read_status_info( printer, 0, sock, host, 2,
			transfer_timeout );
		}
	}

	if( status ){
		plp_snprintf(msg, sizeof(msg)-2, "job '%s' transfer to %s@%s failed",
			id, printer, host);
		if( Interactive ){
			strcat(msg,"\n");
			if( Write_fd_str( 2, msg ) < 0 )cleanup(0);
		} else {
			setstatus( cfp, msg );
		}
	} else {
		DEBUG4("Send_job: printcap_entry 0x%x, end %d, file %s, remote %d",
			printcap_entry, Accounting_end, Accounting_file, Accounting_remote );
		if( Is_server && printcap_entry && Accounting_remote ){
			i = Do_accounting( 1, Accounting_end, cfp,
				transfer_timeout, printcap_entry, 0 );
			if( i ){
				Errorcode = i;
				cleanup( 0 );
			}
		}
		setstatus( cfp, "done job '%s' transfer to %s@%s",
			id, printer, host );
		if( Is_server == 0 && LP_mode && !Silent ){
			id = cfp->identifier;
			if( id[0] ){
				plp_snprintf( msg,sizeof(msg)-1,"request id is %s\n", id+1 );
			} else {
				plp_snprintf( msg,sizeof(msg)-1,"request id is %d\n", cfp->number );
			}
			Write_fd_str(1, msg );
		}
	}
	/* remove temp files */
	Remove_tempfiles();
	return( status );
}

/***************************************************************************
int Send_files(
	char *host,				- host name
	char *printer,			- printer name
	char *dpathname *dpath  - spool directory pathname
	int *sock,					- socket to use
	struct control_file *cfp,	- control file
	int transfer_timeout,		- transfer timeout
	struct printcap_entry 		- printcap entry
	)						- acknowlegement status

 1. send the \2printer\n string to the remote host, wait for an ACK

 
 2. if control file first, send the control file: 
        send \3count cfname\n
        get back <0> ack
        send 'count' file bytes
        send <0> term
        get back <0> ack
 3. for each data file
        send the \4count dfname\n
            Note: count is 0 if file is filter
        get back <0> ack
        send 'count' file bytes
            Close socket and finish if filter
        send <0> term
        get back <0> ack
  4. If control file last, send the control file as in step 2.
      

If the block_fd parameter is non-zero, we write out the
various control and data functions to a file instead.

 ***************************************************************************/

int Send_files( char *host, char *printer,
	struct dpathname *dpath,
	int *sock, struct control_file *cfp, int transfer_timeout,
	struct printcap_entry *printcap_entry,
	int block_fd )
{
	int status = 0;			/* status of transfer command */
	int size;			/* size of file */
	char line[LINEBUFFER];	/* buffer for making up command to send */
	int count, order;	/* ACME, the very finest in integer variables */
	struct data_file *df;	/* data file entry */
	struct stat statb;	/* status buffer */
	int fd;				/* file descriptor */
	int ack;			/* ack byte */
	int err;			/* saved errno */
	int sent_data = 0;		/* flag to handle sending data files first */
	char *cf_copy;			/* control file copy being sent */
	char *write_file;		/* file being written */
	char *id;

	DEBUG3("Send_files: send_data_first %d", Send_data_first );
	/* ugly ugly hack to send data files first to printers that
	 *	need it.  Also elegant way to test that you can handle data files
	 *	first. :-)
	 */
	id = cfp->identifier;
	if( id[0] == 0 ){
		id = cfp->transfername;
	} else {
		++id;
	}
	write_file = cfp->transfername;
	if( block_fd == 0 ){
		setstatus( cfp, "sending job '%s' to %s@%s",
			id, printer, host );
		plp_snprintf( line, sizeof(line), "%c%s\n",
			REQ_RECV, printer );
		status = Link_send( host, sock, transfer_timeout,
			line, strlen(line), &ack );
		if( status ){
			setstatus( cfp, "error '%s' sending '\\%d%s\\n' to %s@%s",
				Link_err_str(status), REQ_RECV, printer, printer, host );
			return(status);
		}
	}


	if( block_fd == 0 &&  Send_data_first ) goto send_data;
send_control:
	/*
	 * get the total length of the control file
	 */

	size = 0;
	DEBUG3("Send_files: line_count %d", cfp->control_file_lines.count );

	if( block_fd == 0 ){
		setstatus( cfp, "sending file '%s' to %s@%s",
		cfp->transfername, printer, host );
	}


	cf_copy = (char *)cfp->control_file_copy.list;
	size = strlen( cf_copy );
	DEBUG3( "Send_files: control file %d bytes '%s'", size, cf_copy );

	/*
	 * send the command file name line
	 */
	write_file = cfp->transfername;
	ack = 0;
	errno = 0;
	plp_snprintf( line, sizeof(line), "%c%d %s\n",
		CONTROL_FILE, size, write_file);
	if( block_fd == 0 ){
		status = Link_send( host, sock, transfer_timeout,
			line, strlen(line), &ack );
		if( status ){
			if(!Interactive)logerr( LOG_INFO,
				"Send_files: error '%s' ack %d sending '%s' to %s@%s",
				Link_err_str(status), ack, line, printer, host );
			switch( ack ){
			case ACK_STOP_Q: status = JABORT; break;
			default:         status = JFAIL; break;
			}
			return(status);
		}
	} else {
		if( Write_fd_str( block_fd, line ) < 0 ){
			goto write_error;
		}
	}
	/*
	 * send the control file
	 */
	errno = 0;
	if( block_fd == 0 ){
		/* we include the 0 at the end */
		status = Link_send( host, sock, transfer_timeout,
			cf_copy,strlen(cf_copy)+1,&ack );
		if( status != 0 ){
			if(!Interactive)logerr( LOG_INFO,
			"Send_files: error '%s' ack %d sending control file '%s' to %s@%s",
				Link_err_str(status), ack, cfp->transfername, printer, host );
			switch( ack ){
			case ACK_STOP_Q: status = JABORT; break;
			default:         status = JFAIL; break;
			}
			return( status );
		}
		DEBUG3( "Send_files: control file sent, ack '%d'", ack );
		setstatus( cfp, "completed '%s' to %s@%s",
			cfp->transfername, printer, host );
	} else {
		if( Write_fd_str( block_fd, cf_copy ) < 0 ){
			goto write_error;
		}
	}

	DEBUG3( "Send_files: data file count '%d'", cfp->data_file_list.count );

	/*
	 * now send the data files
	 */

send_data:
	order = 0;
	if( cfp->flags & LAST_DATA_FILE_FIRST ){
		order = cfp->data_file_list.count - 1;
	}
	for( count = 0; sent_data == 0 && status == 0 && count < cfp->data_file_list.count;
		++count, ++order ){
		df = (void *)cfp->data_file_list.list;
		if( order >= cfp->data_file_list.count ){
			order = 0;
		}
		df = &df[order];
		DEBUG3(
	"Send_files: [%d] cfline '%s', openname '%s', fd %d, size %d, is_a_copy %d",
		order, df->cfline, df->openname, df->fd, df->statb.st_size,
		df->is_a_copy );
		if( df->is_a_copy ) continue;	/* we do not send this file */
		if( (DbgTest & 0x01) && count == 0 ){
			DEBUG0("Send_files: DgbTest flag 0x01 set! skip first file" );
			continue;
		}
		fd = df->fd;
		if( df->d_flags & PIPE_FLAG ){
			DEBUG3( "Send_files: file '%s' open already, fd %d",
				df->openname, fd );
			/* ignore the status on LSEEK - we may have a pipe or fd */
			size = -1;
		} else if( df->openname[0] ){
			DEBUG3("Send_files: opening file '%s'", df->openname );
			/*
			 * open file as user; we should be running as user
			 */
			fd = Checkread( df->openname, &statb );
			err = errno;
			size = statb.st_size;
			DEBUG3( "Send_files: openname '%s', fd %d, size %d",
				df->openname, fd, size );
			if( fd < 0 ){
				char *s = strrchr( df->openname, '/' );
				if( s == 0 ) s = df->openname;
				/* we cannot open the data file! */
				plp_snprintf( cfp->error, sizeof(cfp->error),
					"cannot open '%s' - '%s'", s, Errormsg(err) );
				if( !Interactive ){
					Set_job_control( cfp, (void *)0 );
				} else {
					/* send it directly to the error output */
					safestrncpy(line,cfp->error);
					strcat(line,"\n");
					if( Write_fd_str( 2, line ) < 0 ) cleanup(0);
				}
				setstatus( cfp, "%s", cfp->error );
				Link_close( sock );
				return(JREMOVE);
			}
			if( statb.st_dev != df->statb.st_dev ){
				plp_snprintf( cfp->error, sizeof(cfp->error),
					"file '%s', st_dev %d, not %d, possible security problem",
					df->openname, statb.st_dev, df->statb.st_dev );
				log( LOG_ERR, "Send_files: %s", cfp->error );
				if( !Interactive ){
					Set_job_control( cfp, (void *)0 );
				}
				return(JREMOVE);
			}
			if( statb.st_ino != df->statb.st_ino ){
				plp_snprintf( cfp->error, sizeof(cfp->error),
				"file '%s', st_ino %d, not %d, possible security problem",
					df->openname, statb.st_ino, df->statb.st_ino );
				log( LOG_ERR, "Send_files: %s", cfp->error );
				if( !Interactive ){
					Set_job_control( cfp, (void *)0 );
				}
				return(JREMOVE);
			}
			if( statb.st_size != df->statb.st_size ){
				plp_snprintf( cfp->error, sizeof(cfp->error),
					"file '%s', st_size %d, not %d, possible security problem",
					df->openname, statb.st_size, df->statb.st_size );
				log( LOG_ERR, "Send_files: %s", cfp->error );
				if( !Interactive ){
					Set_job_control( cfp, (void *)0 );
				}
				return(JREMOVE);
			}
			/* first we have to rewind the file */
			if( lseek( fd, 0, SEEK_SET ) == (off_t)(-1) ){
				err = errno;
				plp_snprintf( cfp->error, sizeof(cfp->error),
					"file '%s', fd %d, lseek failed - %s",
					df->openname, fd, Errormsg(err) );
				log( LOG_ERR, "Send_files: %s", cfp->error );
				if( !Interactive ){
					Set_job_control( cfp, (void *)0 );
				}
				return(JREMOVE);
			}
		} else {
			fatal( LOG_ERR, "Send_files: no job file name" );
		}

		/*
		 * Now for the infamous 'use the filters on a bounce queue'
		 * problem.  If we have a bounce queue,  we may want to
		 * run the data files through the filters before sending them
		 * to their destinations.  This is done if the :bq=destination:
		 * is set in the printcap (Bounce_queue_dest variable)
		 * is set.  We do filtering if:
		 *   1. Bounce_queue_dest is set
		 *   2. There is a filter for this format
		 *	 3. and we are not a client (Interactive = 0);
		 * To make filtering work,  we first create a temporary file in
		 * the spool directory.  We redirect the filter output to the
		 * temporary file,  and then use this as the real file.  When done,
		 * we remove the temporary file.  To ensure that we do not
		 * use too much space,  we will create only one temp file.
		 *
		 * Note: the 'p' format is supposed to be processed by the /bin/pr
		 * program. However, it may also get reprocessed at the destination
		 * by another filter.  We will assume that the user will provide a
		 * 'p' format filter or not have it filtered in a bounce queue.
		 */

		if( (Is_server && Bounce_queue_dest) || (!Is_server && Lpr_bounce) ){
			if( (status = Bounce_filter( cfp, df, printcap_entry, fd,
					&size, &statb, transfer_timeout )) ){
				return( status );
			}
		}

		/*
		 * send the data file name line
		 */
		plp_snprintf( line, sizeof(line), "%c%d %s\n",
				DATA_FILE, size == -1? 0 : size, df->cfline+1 );
		if( block_fd == 0 ){
			setstatus( cfp, "sending file '%s' to %s@%s", df->cfline+1,
				printer, host );
			DEBUG3("Send_files: data file line '%s'", line );
			errno = 0;
			status = Link_send( host, sock, transfer_timeout,
				line, strlen(line), &ack );
			if( status ){
				logerr( LOG_INFO,
				"Send_files: error '%s' ack %d sending '%s' to %s@%s",
					Link_err_str( status ),ack, line, printer, host );
				return(status);
			}

			/*
			 * send the data files content
			 */

			DEBUG3("Send_files: doing transfer of '%s'", df->openname );
			if( df->d_flags & PIPE_FLAG ){
				size = -1;
			}
			/* no timeout on reading, we may be reading from pipe */
			errno = 0;
			status = Link_copy( host, sock, 0, transfer_timeout,
					df->openname, fd, size );

			/*
			 * get the ACK only if you did not have an error
			 */
			if( size > 0 && status == 0 ){
				status = Link_ack( host,sock,transfer_timeout,0x100,&ack );
			}
			if( status ){
				log( LOG_INFO,
					"Send_files: error '%s' sending data file '%s' to %s@%s",
					Link_err_str( status ), df->openname, printer, host );
				setstatus( cfp, "error sending '%s' to %s@%s",
					df->cfline+1, printer, host );
			} else {
				setstatus( cfp, "completed '%s' to %s@%s",
					df->cfline+1, printer, host );
			}
		} else {
			int total;
			int len;

			write_file = df->cfline+1;
			if( Write_fd_str( block_fd, line ) < 0 ){
				goto write_error;
			}
			/* now we need to read the file and transfer it */
			total = 0;
			while( total < size && (len = read(fd, line, sizeof(line)) ) > 0 ){
				if( Write_fd_len( block_fd, line, len ) < 0 ){
					goto write_error;
				}
				total += len;
			}
			if( total != size ){
				plp_snprintf(cfp->error, sizeof(cfp->error),
					"job '%s' did not read all of '%s'",
					id, write_file );
				setstatus( cfp, "%s", cfp->error );
				return( JFAIL );
			}
		}
		close(fd);
	}

	/* the remaining part of the ugly hack */
	if( block_fd == 0 ){
		if( Send_data_first && sent_data == 0 ){
			sent_data = 1;
			goto send_control;
		}
		Link_close( sock );
	}
	return(status);

write_error:
	err = errno;
	plp_snprintf(cfp->error, sizeof(cfp->error),
		"job '%s' write to temporary file failed '%s'",
		write_file, Errormsg( err ) );
	setstatus( cfp, "%s", cfp->error );
	return( JFAIL );
}

/*
 * Bounce filter - 
 * 1. check to see if there is a filter for this format
 * 2. check to see if the temp file has already been created
 *    if not, create and open the temp file
 * 3. run the filter,  with its output directed to the temp file
 * 4. close the original input file,  and dup the temp file to it.
 * 5. return the temp file size
 */


static int Bounce_filter( struct control_file *cfp,
	struct data_file *data_file, struct printcap_entry *printcap_entry,
	int fd, int *size, struct stat *statb, int transfer_timeout )
{
	int c, in, err;
	char *filter;
	static int tempfd;
	char tempbuf[LARGEBUFFER];

	/* get the temp file */
	/* now find the filter - similar to the printer support code */
	filter = 0;
	/*
	 * check for PR to be used
	 */
	c = data_file->format;
	switch( c ){
	case 'f': case 'l':
		filter = IF_Filter;
		break;
	default:
		filter = Find_filter( c, printcap_entry );
	}
	DEBUG3("Bounce_filter: filter '%s'", filter );

	/* hook filter up to the device file descriptor */
	if( filter ){
		if( tempfd <= 0 ){
			tempfd = Make_temp_fd( 0, 0 );
		}
		if( ftruncate( tempfd, 0 ) < 0 ){
			err = errno;
			Errorcode = JFAIL;
			logerr_die( LOG_INFO,
				"Bounce_filter: error 'truncating bounce queue temp fd" );
		}
		if( Make_filter( c, cfp, &XF_fd_info, filter,
			0, /* no extra */
			0,	/* RW pipe */
			tempfd, /* dup to fd 1 */
			printcap_entry, /* printcap information */
			data_file, Accounting_port, Logger_destination != 0,
				Direct_read?fd:0 ) ){
			setstatus( cfp, "%s", cfp->error );
			cleanup(0);
		}
		/* at this point you have a filter, which is taking input
			from XF_fd_info.input; pass input file through it */
		if( Direct_read == 0 ){
			while( (in = read(fd, tempbuf, sizeof(tempbuf)) ) > 0 ){
				Write_fd_len( XF_fd_info.input, tempbuf, in );
			}
		}
		Errorcode = Close_filter( 0, &XF_fd_info,
			transfer_timeout, "bounce queue" );
		if( Errorcode ){
			logerr_die( LOG_INFO,
				"Bounce_filter: error closing bounce queue filter" );
		}
		/* now we close the input file and dup the filter output to it */
		if( dup2( tempfd, fd ) < 0 ){
			Errorcode = JFAIL;
			logerr_die( LOG_INFO, "Bounce_filter: dup2 failed" );
		}
		if( lseek( fd, 0, SEEK_SET ) < 0 ){
			Errorcode = JFAIL;
			logerr_die( LOG_INFO, "Bounce_filter: lseek failed" );
		}
		if( fstat( fd, statb ) < 0 ){
			Errorcode = JFAIL;
			logerr_die( LOG_INFO, "Bounce_filter: fstat failed" );
		}
		*size = statb->st_size;
		DEBUG3("Bounce_filter: fd '%d', size %d", fd, *size );
	}
	return( 0 );
}

/***************************************************************************
int Send_block(
	char *host,				- host name
	char *printer,			- printer name
	char *dpathname *dpath  - spool directory pathname
	int *sock,					- socket to use
	struct control_file *cfp,	- control file
	int transfer_timeout,		- transfer timeout
	)						- acknowlegement status

 1. Get a temporary file
 2. Generate the compressed data files - this has the format
      \3count cfname\n
      [count control file bytes]
      \4count dfname\n
      [count data file bytes]

 3. send the \6printer user@host size\n
    string to the remote host, wait for an ACK
 
 4. send the compressed data files - this has the format
      wait for an ACK

 ***************************************************************************/

int Send_block( char *host, char *printer,
	struct dpathname *dpath,
	int *sock, struct control_file *cfp, int transfer_timeout,
	struct printcap_entry *printcap_entry )
{
	int tempfd;			/* temp file for data transfer */
	char tempbuf[SMALLBUFFER];	/* buffer */
	struct stat statb;
	int size, err;				/* ACME! The best... */
	int status = 0;				/* job status */
	int ack;
	char *id;

	id = cfp->identifier;
	if( id[0] == 0 ){
		id = cfp->transfername;
	} else {
		++id;
	}
	tempfd = Make_temp_fd( 0, 0 );
	if( tempfd < 0 ){
		err = errno;
		Errorcode = JFAIL;
		logerr_die( LOG_INFO,
			"Send_block: error truncating temp fd" );
	}
	if( ftruncate( tempfd, 0 ) < 0 ){
		err = errno;
		Errorcode = JFAIL;
		logerr_die( LOG_INFO,
			"Send_block: error truncating temp fd" );
	}

	status = Send_files( host, printer, dpath, sock, cfp, transfer_timeout,
		printcap_entry, tempfd );

	if( status ) return( status );

	/* rewind the file */
	if( lseek( tempfd, 0, SEEK_SET ) == -1 ){
		Errorcode = JFAIL;
		logerr_die( LOG_INFO, "Send_files: lseek tempfd failed" );
	}
	/* now we have the copy, we need to send the control message */
	if( fstat( tempfd, &statb ) ){
		Errorcode = JFAIL;
		logerr_die( LOG_INFO, "Send_files: fstat tempfd failed" );
	}
	size = statb.st_size;

	/* now we know the size */
	DEBUG3("Send_block: size %d", size );
	setstatus( cfp, "sending job '%s' to %s@%s",
		id, printer, host );
	plp_snprintf( tempbuf, sizeof(tempbuf), "%c%s %s %d\n",
		REQ_BLOCK, printer, cfp->transfername, size );
	DEBUG3("Send_block: sending '%s'", tempbuf );
	status = Link_send( host, sock, transfer_timeout,
		tempbuf, strlen(tempbuf), &ack );
	DEBUG3("Send_block: status '%s'", Link_err_str(status) );
	if( status ){
		setstatus( cfp, "error '%s' sending '%s' to %s@%s",
			Link_err_str(status), tempbuf, printer, host );
		return(status);
	}

	/* now we send the data file, followed by a 0 */
	DEBUG3("Send_block: sending data" );
	status = Link_copy( host, sock, 0, transfer_timeout,
		cfp->transfername, tempfd, size );
	DEBUG3("Send_block: status '%s'", Link_err_str(status) );
	if( size > 0 && status == 0 ){
		status = Link_ack( host,sock,transfer_timeout,0x100,&ack );
		DEBUG3("Send_block: ack status '%s'", Link_err_str(status) );
	}
	if( status ){
		log( LOG_INFO,
			"Send_files: error '%s' sending data file '%s' to %s@%s",
			Link_err_str( status ), cfp->transfername, printer, host );
		setstatus( cfp, "error sending '%s' to %s@%s",
			id, printer, host );
	} else {
		setstatus( cfp, "completed '%s' to %s@%s",
			id, printer, host );
	}
	close( tempfd );
	return( status );
}

/***************************************************************************
 *int Send_secure_block(
 *	char *host,				- host name
 *	char *printer,			- printer name
 *	char *dpathname *dpath  - spool directory pathname
 *	int *sock,					- socket to use
 *	struct control_file *cfp,	- control file
 *	int transfer_timeout,		- transfer timeout
 *	)						- acknowlegement status
 *
 * 1. Get a temporary file
 * 2. Generate the compressed data files - this has the format
 *      \REQ_BLOCKprinter user@host filename
 *      \3count cfname\n
 *      [count control file bytes]
 *      \4count dfname\n
 *      [count data file bytes]
 *
 * 3. send the \REQ_SECprinter user@host file size\n
 *    string to the remote host, wait for an ACK
 * 
 * 4. send the compressed data files - this has the format
 *      wait for an ACK
 *
 ***************************************************************************/

int Send_secure_block( char *host, char *printer,
	struct dpathname *dpath,
	int *sock, struct control_file *cfp, int transfer_timeout,
	struct printcap_entry *printcap_entry )
{
	int tempfd;			/* temp file for data transfer */
	int err;			/* ACME! The best... */
	int status = 0;		/* job status */
	char tempfilename[MAXPATHLEN];

	DEBUG3("Send_secure_block: printer '%s'", printer );
	if( Is_server && cfp->auth_id[0] == 0 ){
		fatal( LOG_ERR,
		"Send_secure_block: missing job authentication");
	}

	tempfd = Make_temp_fd( tempfilename, sizeof(tempfilename) );
	if( tempfd <= 0 ){
		err = errno;
		Errorcode = JFAIL;
		logerr_die( LOG_INFO,
			"Send_secure_block: error opening temp fd" );
	}
	if( ftruncate( tempfd, 0 ) < 0 ){
		err = errno;
		Errorcode = JFAIL;
		logerr_die( LOG_INFO,
			"Send_secure_block: error truncating temp fd" );
	}

	status = Send_files( host, printer, dpath, sock, cfp,
		transfer_timeout, printcap_entry, tempfd );

	if( status ) return( status );

	status = Send_auth_transfer( 1, printer, host, sock, transfer_timeout,
		printcap_entry, tempfd, tempfilename, 2, cfp->transfername );

	DEBUG3("Send_secure_block: status %d", status );
	return( status );
}
