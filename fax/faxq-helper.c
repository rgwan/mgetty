#ident "$Id: faxq-helper.c,v 4.2 2002/11/14 22:51:24 gert Exp $ Copyright (c) Gert Doering"

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
 *       TODO: user permission check
 *       TODO: check for existance of $FAX_SPOOL_OUT
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
 *
 * Note: right now, this needs an ANSI C compiler, and might not be 
 *       as portable as the remaining mgetty code.  Send diffs :-)
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdarg.h>

/* globals used by all the routines */
char * program_name;
int    real_user_id;		/* numeric user ID of caller */
char * real_user_name;		/* user name of caller */

int    fax_out_uid = 5;		/* TODO!!! */

#define FAX_SEQ_FILE	".Sequence"
#define FAX_SEQ_LOCK	"LCK..seq"

#define MAXJIDLEN	20	/* maximum length of acceptable job ID */

void error_and_exit( char * string )
{
    fprintf( stderr, "%s: %s\n", program_name, string );
    exit(1);
}

/* generic error messager - just to increase readability of the code below
 */
void eout(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf( stderr, "%s: ", program_name );
    vfprintf( stderr, fmt, ap);
    va_end(ap);
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
    if ( strlen( JobID ) > MAXJIDLEN ) ok = 0;

    if ( ! ok ) eout( "invalid JobID: '%s'\n", JobID );

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
		eout( "sequence file locked (try %d)...\n", try );
		sleep( rand()%3 +1 );
		goto again;
	    }
	    eout( "can't lock sequence file, give up\n" );
	    return -1;
	}

	if ( errno == ENOENT )		/* sequence file does not exist */
	{
	    eout( "sequence file does not exist, creating...\n" );
	    fd = creat( FAX_SEQ_FILE, 0644 );
	    if ( fd < 0 )
	    {
		eout( "can't create sequence file '%s': %s\n",
		      FAX_SEQ_FILE, strerror(errno) );
		return -1;
	    }
	    write( fd, "000000\n", 7 );
	    close( fd );
	    goto again;
	}

	eout( "can't lock sequence file: %s\n", strerror(errno) );
	return -1;
    }

    /* sequence file is locked, now read current sequence */
    fd = open( FAX_SEQ_FILE, O_RDWR );
    if ( fd < 0 )
    {
	eout( "can't open '%s' read/write: %s\n", 
              FAX_SEQ_FILE, strerror(errno) );
	goto unlock_and_out;
    }

    l = read( fd, buf, sizeof(buf)-1 );
    if ( l >= 0 ) buf[l] = '\0';

    if ( l < 0 || l >= sizeof(buf)-1 || ! isdigit( buf[0] ) )
    {
	eout( "sequence file '%s' corrupt\n", FAX_SEQ_FILE );
	goto close_and_out;
    }

    s = atol( buf ) + 1;
    sprintf( buf, "%0*ld\n", l-1, s );

    if ( lseek( fd, 0, SEEK_SET ) != 0 )
    {
	eout( "can't rewind sequence file: %s\n", strerror(errno) );
	goto close_and_out;
    }

    l = strlen(buf);
    if ( write( fd, buf, l ) != l )
    {
	eout( "can't write all %d bytes to %s: %s\n", 
              l, FAX_SEQ_FILE, strerror(errno) );
    }

close_and_out:
    close(fd);

unlock_and_out:
    unlink( FAX_SEQ_LOCK );
    return s;
}

/* create a new job
 *   - get next sequence number
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
	eout( "can't create directory '%s': %s\n", dirbuf, strerror(errno) );
	return -1;
    }

    if ( chown(dirbuf, real_user_id, 0) < 0 )
    {
	eout( "can't chown '%s' to uid %d: %s\n", 
              dirbuf, real_user_id, strerror(errno) );
	rmdir( dirbuf );
	return -1;
    }

    /* print file name (without ".in") to stdout */
    printf( "F%06ld\n", seq );
    return 0;
}

/* remove a complete directory hierarchy
 */
void remove_broken_dir( char * directory )
{
    error_and_exit( "not implemented" );
}

/* make sure that all path names in the following list (separated by
 * whitespace) are local to this directory, exist, and are regular files
 */
int do_sanitize_page_files( char * dir, char * filelist )
{
char * p, tmp[300];
struct stat stb;
int n=0;

    p = strtok( filelist, " \t\n" );

    while( p != NULL )
    {
	if ( strchr( p, '/' ) != NULL )
	{
	    eout( "non-local file name: '%s', abort\n", p ); return -1;
	}

	if ( strlen( dir ) + strlen( p ) + 3 >= sizeof(tmp) )
	{
	    eout( "file name '%s' too long, abort\n", p );
	}
	sprintf( tmp, "%s/%s", dir, p );

	if ( lstat( tmp, &stb ) < 0 )
	{
	    eout( "can't stat file '%s': %s\n", tmp, strerror(errno) );
	    return -1;
	}

	if ( !S_ISREG( stb.st_mode ) )
	{
	    eout( "'%s' is not a regular file, abort\n", tmp ); return -1;
	}

	n++;
	p = strtok( NULL, " \t\n" );
    }

    return n;
}

/* Activate "pending" fax job
 *
 */
int do_activate( char * JID )
{
struct stat stb;
char dir1[MAXJIDLEN+10];
char buf[1000], *p;
int fd;

    sprintf( dir1, ".in%s", JID );
    if ( stat( dir1, &stb ) < 0 )
    {
	eout( "can't stat '%s': %s\n", dir1, strerror(errno) );
	return -1;
    }
    if ( !S_ISDIR( stb.st_mode ) )
    {
	eout( "%s is no directory!\n", dir1 );
	return -1;
    }
    if ( stb.st_uid != real_user_id )
    {
	eout( "job directory '%s' belongs to user %d\n", dir1, stb.st_uid );
	return -1;
    }

    if ( chown( dir1, fax_out_uid, 0 ) < 0 ||
         chmod( dir1, 0755 ) < 0 )
    {
	eout( "chown/chmod '%s' failed: %s\n", dir1, strerror(errno) );
	return -1;
    }

    sprintf( buf, "%s/JOB", dir1 );

/* TODO: check if this catches symlinks to non-existant files! */
    if ( ( fd = open( buf, O_WRONLY | O_CREAT | O_EXCL, 0644 ) ) < 0 )
    {
	eout( "can't create JOB file '%s': %s\n", buf, strerror(errno) );
	remove_broken_dir(dir1);
	return -1;
    }

    /* new JOB file has to belong to fax user, too */
    if ( chown( buf, fax_out_uid, 0 ) < 0 )
    {
	eout( "can't chown '%s' to uid %d: %s\n", 
	      buf, fax_out_uid, strerror(errno) );
	close(fd);
	remove_broken_dir(dir1);
	return -1;
    }

    /* read queue metadata from stdin, sanitize, write to JOB fd */
    while( ( p = fgets( buf, sizeof(buf)-1, stdin ) ) != NULL )
    {
	int l = strlen(buf);

	if ( l >= sizeof(buf)-2 )
	{
	    eout( "input line too long\n" ); break;
	}

	if ( write( fd, buf, l ) != l )
	{
	    eout( "can't write line to JOB file: %s\n", strerror(errno) );
	    break;
	}

	if ( l>0 && buf[l-1] == '\n' ) buf[--l]='\0';

	if ( strncmp(buf, "user ", 5) == 0 )
	{
	    if ( strcmp( buf+5, real_user_name ) != 0 )
	    {
		eout( "user name mismatch: %s <-> %s\n", buf+5, real_user_name );
		break;
	    }
	}
	if ( strncmp(buf, "pages", 5 ) == 0 &&
	     do_sanitize_page_files( dir1, buf+5 ) < 0 )
	{
	    eout( "bad input files specified\n" );
	    break;
	}
    }

    close(fd);
    if ( p != NULL )	/* loop aborted */
    {
	remove_broken_dir(dir1); return -1;
    }

    /* now move directory in final place */
    if ( rename( dir1, JID ) < 0 )
    {
	eout( "can't rename '%s' to '%s': %s\n", dir1, JID, strerror(errno));
	remove_broken_dir(dir1); return -1;
    }

    return 0;
}


int do_remove( char * JID )
{
    error_and_exit( "not implemented" );
}
int do_requeue( char * JID )
{
    error_and_exit( "not implemented" );
}

int main( int argc, char ** argv )
{
    struct passwd * pw; 		/* for user name */

    program_name = strrchr( argv[0], '/' );
    if ( program_name == NULL ) program_name = argv[0];

    if ( argc < 2 )
	{ error_and_exit( "keyword missing" ); }

    /* common things to check and prepare */
    if ( chdir( FAX_SPOOL_OUT ) < 0 )
    {   
	eout( "can't chdir to %s: %s\n", FAX_SPOOL_OUT, strerror(errno) );
	exit(2);
    }

    /* effective user ID is root, real user ID is still the caller's */
    if ( geteuid() != 0 )
    {
	eout( "must be set-uid root\n" );
	exit(3);
    }
    real_user_id = getuid();
    pw = getpwuid( real_user_id );
    if ( pw == NULL || pw->pw_name == NULL )
    {
	eout( "you don't exist, go away (uid=%d)!\n", real_user_id );
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

    error_and_exit( "invalid keyword or wrong number of parameters" );
    return 0;
}
