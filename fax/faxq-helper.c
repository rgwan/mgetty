#ident "$Id: faxq-helper.c,v 4.1 2002/11/14 16:14:31 gert Exp $ Copyright (c) Gert Doering"

/* faxq-helper.c
 *
 * this is a suid-root helper process that is used for the unprivileged
 * fax queue client programs (faxspool, faxq, faxrm) to access the
 * /var/spool/fax/outgoing/... fax queue ($OUT).
 *
 * there are 4 commands:
 *
 * faxq-helper new
 *       return a new job ID (F000123) and create "$OUT/.inF000123/" directory
 *
 * faxq-helper activate $ID
 *       chown/chmod $OUT/.inF000134/ directory to user "fax"
 *       chown + check files in the directory (no symlinks)
 *       take prototype JOB file from stdin, 
 *	     check that "pages ..." does not reference any non-local 
 *	     or non-existing files
 *	     check that "user ..." contains the correct value
 *	 create JOB
 *       move $OUT/.inF000134/ to $OUT/F000134/ -> faxrunq will "see" it
 *
 * faxq-helper remove $ID
 *       check that $OUT/$ID/ exists and belongs to the calling user
 *       remove directory plus all files/subdirs in it
 *
 * faxq-helper requeue $ID
 *       check that $OUT/$ID/ exists and belongs to the calling user
 *       move $JOB.error to $JOB to reactivate job
 *       touch $queue_changed
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>

/* globals used by all the routines */
char * program_name;
int    real_user_id;		/* numeric user ID of caller */
char * real_user_name;		/* user name of caller */

#define FAX_SEQ_FILE	".Sequence"
#define FAX_SEQ_LOCK	"LCK..seq"

void error_out( char * string )
{
    fprintf( stderr, "%s: %s\n", program_name, string );
    exit(1);
}

/* validate format of job id: "Fnnnnnnn" 
 */
int validate_job_id( char * JobID )
{
    int ok = 1;
    char * p = JobID;

    if ( *p++ != 'F' ) ok = 0;
    while( *p != '\0' && ok )
    {
	if ( ! isdigit( *p ) ) ok = 0;
	p++;
    }
    if ( ! ok )
	fprintf( stderr, "%s: invalid JobID: '%s'\n", program_name, JobID );

    return ok;
}

/* create next sequence number
 *   - if sequence file doesn't exist -> create, and return "1"
 *   - if locks/ subdir doesn't exist, create
 *   - lock file by creating hard link
 *   - read current sequence number, add 1, write back
 *   - unlock
 */
long get_next_seq(void)
{
char buf[100];
int try = 0;
long s = -1;
int fd, l;

again:
    if ( link( FAX_SEQ_FILE, FAX_SEQ_LOCK ) < 0 )
    {
	if ( errno == EEXIST )		/* lock exists */
	{
	    if ( ++try < 3 )
	    {
		fprintf( stderr, "%s: sequence file locked (try %d)...\n",
			    program_name, try );
		sleep( rand()%3 +1 );
		goto again;
	    }
	    fprintf( stderr, "%s: can't lock sequence file, give up\n",
			program_name );
	    return -1;
	}

	if ( errno == ENOENT )		/* sequence file does not exist */
	{
	    fprintf( stderr, "%s: sequence file does not exist, creating...\n",
	    		program_name );
	    fd = creat( FAX_SEQ_FILE, 0644 );
	    if ( fd < 0 )
	    {
		fprintf( stderr, "%s: can't create sequence file '%s': %s\n",
			program_name, FAX_SEQ_FILE, strerror(errno) );
		return -1;
	    }
	    write( fd, "000000\n", 7 );
	    close( fd );
	    goto again;
	}

	fprintf( stderr, "%s: can't lock sequence file: %s\n",
			program_name, strerror(errno) );
	return -1;
    }

    /* sequence file is locked, now read current sequence */
    fd = open( FAX_SEQ_FILE, O_RDWR );
    if ( fd < 0 )
    {
	fprintf( stderr, "%s: can't open '%s' read/write: %s\n",
			program_name, FAX_SEQ_FILE, strerror(errno) );
	goto unlock_and_out;
    }

    l = read( fd, buf, sizeof(buf)-1 );
    if ( l >= 0 ) buf[l] = '\0';

    if ( l < 0 || l >= sizeof(buf)-1 || ! isdigit( buf[0] ) )
    {
	fprintf( stderr, "%s: sequence file '%s' corrupt\n",
			program_name, FAX_SEQ_FILE );
	goto close_and_out;
    }

    s = atol( buf ) + 1;
    sprintf( buf, "%0*ld\n", l-1, s );

    if ( lseek( fd, 0, SEEK_SET ) != 0 )
    {
	fprintf( stderr, "%s: can't rewind sequence file: %s\n",
			program_name, strerror(errno) );
	goto close_and_out;
    }

    l = strlen(buf);
    if ( write( fd, buf, l ) != l )
    {
	fprintf( stderr, "%s: can't write all %d bytes to %s: %s\n",
			program_name, l, FAX_SEQ_FILE, strerror(errno) );
    }

close_and_out:
    close(fd);

unlock_and_out:
    unlink( FAX_SEQ_LOCK );
    return s;
}

/* create a new job
 *   - get sequence number, write new sequence number file
 *   - create directory (prefixed with ".in")
 *   - chown directory to real user (euid)
 *   - print job ID to stdout
 */
int do_new( void )
{
long seq;
char dirbuf[100];

    /* get next sequence number (including locking) */
    seq = get_next_seq();

    if ( seq <= 0 ) return -1;

    sprintf( dirbuf, ".inF%06ld", seq );

    if ( mkdir(dirbuf, 0700) < 0 )
    {
	fprintf( stderr, "%s: can't create directory '%s': %s\n", 
		    program_name, dirbuf, strerror(errno) );
	return -1;
    }

    if ( chown(dirbuf, real_user_id, 0) < 0 )
    {
	fprintf( stderr, "%s: can't chown '%s' to uid %d: %s\n",
		    program_name, dirbuf, real_user_id, strerror(errno) );
	rmdir( dirbuf );
	return -1;
    }

    /* print file name (without ".in") to stdout */
    printf( "F%06ld\n", seq );
    return 0;
}

int do_activate( char * JID )
{
    error_out( "not implemented" );
}
int do_remove( char * JID )
{
    error_out( "not implemented" );
}
int do_requeue( char * JID )
{
    error_out( "not implemented" );
}

int main( int argc, char ** argv )
{
    struct passwd * pw; 		/* for user name */

    program_name = argv[0];

    if ( argc < 2 )
	{ error_out( "keyword missing" ); }

    /* common things to check and prepare */
    if ( chdir( FAX_SPOOL_OUT ) < 0 )
    {   
	fprintf( stderr, "%s: can't chdir to %s: %s\n",
		 program_name, FAX_SPOOL_OUT, strerror(errno) );
	exit(2);
    }

    /* effective user ID is root, real user ID is still the caller's */
    if ( geteuid() != 0 )
    {
	fprintf( stderr, "%s: must be set-uid root\n", program_name );
	exit(3);
    }
    real_user_id = getuid();
    pw = getpwuid( real_user_id );
    if ( pw == NULL || pw->pw_name == NULL )
    {
	fprintf( stderr, "%s: you don't exist, go away (uid=%d)!\n",
		 program_name, real_user_id );
	exit(3);
    }
    real_user_name = pw->pw_name;

    /* now parse arguments and go to specific functions */
    if ( argc == 2 && strcmp( argv[1], "new" ) == 0 ) 
    {
	exit( do_new() <0? 10: 0);
    }
    if ( argc == 3 )
    {
	/* second parameter is common for all commands: job ID */
	char * job_id = argv[2];
	if ( ! validate_job_id( job_id ) ) exit(1);

    	if( strcmp( argv[1], "activate" ) == 0 )
	{
	    exit( do_activate( job_id ) <0? 10: 0);
	}
	if ( strcmp( argv[1], "remove" ) == 0 )
	{
	    exit( do_remove( job_id ) <0? 10: 0);
	}
	if ( strcmp( argv[1], "requeue" ) == 0 )
	{
	    exit( do_requeue( job_id ) <0? 10: 0);
	}
    }

    error_out( "invalid keyword or wrong number of parameters" );
    return 0;
}
