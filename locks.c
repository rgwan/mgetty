#ident "$Id: locks.c,v 1.29 1994/08/10 12:54:11 gert Exp $ Copyright (c) Gert Doering / Paul Sutcliffe Jr."

/* large parts of the code in this module are taken from the
 * "getty kit 2.0" by Paul Sutcliffe, Jr., paul@devon.lns.pa.us,
 * and are used with permission here.
 * SVR4 style locking by Bodo Bauer, bodo@hal.nbg.sub.org.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

/* some OSes do include this in stdio.h, others don't... */
#ifndef EEXIST
#include <errno.h>
#endif

#include "mgetty.h"
#include "policy.h"

/* SVR4 uses a different locking mechanism. This is why we need this... */
#ifdef SVR4 
#include <sys/mkdev.h>
 
#define LCK_NODEV    -1
#define LCK_OPNFAIL  -2
#endif

static char	lock[MAXLINE+1];	/* name of the lockfile */

static int readlock _PROTO(( char * name ));
static char *  get_lock_name _PROTO(( char * lock_name, char * device ));

/*
 *	do_makelock() - attempt to create a lockfile
 *
 *	Returns FAIL if lock could not be made (line in use).
 */

int do_makelock _P0( void )
{
	int fd, pid;
	char *temp, buf[MAXLINE+1];
#if LOCKS_BINARY
	int  bpid;		/* must be 4 byte long! */
#else
	char apid[16];
#endif

	lprintf( L_NOISE, "do_makelock: lock='%s'", lock );

	/* first make a temp file */

	(void) sprintf(buf, LOCK, "TM.XXXXXX");
	if ((fd = creat((temp=mktemp(buf)), 0644)) == FAIL) {
		lprintf(L_ERROR, "cannot create tempfile (%s)", temp);
		return(FAIL);
	}

	/* just in case some "umask" is set (errors are ignored) */
	chmod( temp, 0644 );

	/* put my pid in it */

#if LOCKS_BINARY
	bpid = getpid();
	if ( write(fd, &bpid, sizeof(bpid) ) != sizeof(bpid) )
#else
	sprintf( apid, "%10d\n", getpid() );
	if ( write(fd, apid, strlen(apid)) != strlen(apid) )
#endif
	{
	    lprintf( L_FATAL, "cannot write PID to (temp) lock file" );
	    close(fd);
	    unlink(temp);
	    return(FAIL);
	}
	
	close(fd);

	/* link it to the lock file */

	while (link(temp, lock) == FAIL)
	{
	        if (errno != EEXIST )
		{
		    lprintf(L_ERROR, "lock not made: link(temp,lock) failed" );
		}

		if (errno == EEXIST)		/* lock file already there */
		{
		    if ((pid = readlock(lock)) == FAIL)
		    {
			if ( errno == ENOENT )	/* disappeared */
			    continue;
			else
			{
			    lprintf( L_NOISE, "cannot read lockfile" );
			    unlink(temp);
			    return FAIL;
			}
		    }

		    if (pid == getpid())	/* huh? WE locked the line!*/
		    {
			lprintf( L_WARN, "we *have* the line!" );
			unlink(temp);
			return SUCCESS;
		    }

		    if ((kill(pid, 0) == FAIL) && errno == ESRCH)
		    {
			/* pid that created lockfile is gone */
			lprintf( L_NOISE, "stale lockfile, created by process %d, ignoring", pid );
			(void) unlink(lock);
			continue;
		    }
		    
		    lprintf(L_MESG, "lock not made: lock file exists");
		}				/* if (errno == EEXIST) */
		
		(void) unlink(temp);
		return(FAIL);
	}
	
	lprintf(L_NOISE, "lock made");
	(void) unlink(temp);
	return(SUCCESS);
}

/* makelock( Device )
 *
 * lock a device,
 * using the LOCK directory from mgetty.c resp. get_lock_name()
 */

int makelock _P1( (device),
		  char *device)
{
    lprintf(L_NOISE, "makelock(%s) called", device);

    if ( get_lock_name( lock, device ) == NULL )
    {
	lprintf( L_ERROR, "cannot get lock name" );
	return FAIL;
    }

    return do_makelock();
}

/* makelock_file( lock file )
 *
 * make a lock file with a given name (uses for locking of other files
 * than device nodes)
 */

int makelock_file _P1( (file), char * file )
{
    lprintf(L_NOISE, "makelock_file(%s) called", file);

    strcpy( lock, file );
    
    return do_makelock();
}
   
/*
 *	checklock() - test for presence of valid lock file
 *
 *	if lockfile found, return PID of process holding it, 0 otherwise
 */

int checklock _P1( (device), char * device)
{
    int pid;
    struct stat st;
    char name[MAXLINE+1];
    
    if ( get_lock_name( name, device ) == NULL )
    {
	lprintf( L_ERROR, "cannot get lock name" );
	return NO_LOCK;
    }

    if ((stat(name, &st) == FAIL) && errno == ENOENT)
    {
	lprintf(L_NOISE, "checklock: stat failed, no file");
	return NO_LOCK;
    }
    
    if ((pid = readlock(name)) == FAIL)
    {
	lprintf(L_MESG, "checklock: couldn't read lockfile");
	return NO_LOCK;
    }

    if (pid == getpid())
    {
	lprintf(L_WARN, "huh? It's *our* lock file!" );
	return NO_LOCK;
    }
		
    if ((kill(pid, 0) == FAIL) && errno == ESRCH)
    {
	lprintf(L_NOISE, "checklock: no active process has lock, will remove");
	(void) unlink(name);
	return NO_LOCK;
    }
    
    lprintf(L_NOISE, "lockfile found, pid=%d", pid );
    
    return pid;
}

/*
 *	readlock() - read contents of lockfile
 *
 *	Returns pid read or FAIL on error.
 *
 *      private function
 */

static int readlock _P1( (name),
			 char * name )
{
	int fd, pid;
	char apid[16];
	int  length;

	if ((fd = open(name, O_RDONLY)) == FAIL)
		return(FAIL);

	length = read(fd, apid, sizeof(apid));

	pid = 0;
	if ( length == sizeof( pid ) || sscanf(apid, "%d", &pid) != 1 ||
	     pid == 0 )
	{
	    pid = * ( (int *) apid );
#if LOCKS_BINARY == 0
	    lprintf( L_WARN, "compiled with ascii locks, found binary lock file (length=%d, pid=%d)!", length, pid );
#endif
	}
#if LOCKS_BINARY == 1
	else
	{
	    lprintf( L_WARN, "compiled with binary locks, found ascii lock file (length=%d, pid=%d)!", length, pid );
	}
#endif

	(void) close(fd);
	return(pid);
}

/*
 *	rmlocks() - remove lockfile
 */

RETSIGTYPE rmlocks()
{
    if ( lock[0] != 0 )
    {
	lprintf( L_NOISE, "removing lock file" );
	(void) unlink(lock);
    }
    /* mark lock file as 'not set' */
    lock[0] = 0;
}

/* get_lock_name()
 *
 * determine full path + name of the lock file for a given device
 */

#ifdef SVR4

/*
 * get_lock_name() - create SVR4 lock file name (Bodo Bauer)
 */

static char *get_lock_name _P2( (lock, fax_tty),
			 char* lock, char* fax_tty )
{
  struct stat tbuf;
  char ttyname[FILENAME_MAX];

  lprintf(L_NOISE, "get_lock_name(%s) called", fax_tty);

  if ( strncmp( fax_tty, "/dev/", 5 ) == 0 )
      strcpy( ttyname, fax_tty );
  else
      sprintf(ttyname, "/dev/%s", fax_tty);
  
  lprintf(L_NOISE, "-> ttyname %s", ttyname);

  if (stat(ttyname, &tbuf) < 0) {
    if(errno == ENOENT) {
      lprintf(L_NOISE, "device does not exist: %s", ttyname);
      return(NULL);		
    } else {
      lprintf(L_NOISE, "could not access line: %s", ttyname);
      return(NULL);		
    }
  }

  sprintf(lock,"%s/LK.%03u.%03u.%03u",
	  LOCK_PATH,
	  major(tbuf.st_dev),
	  tbuf.st_rdev >> 18, 
	  minor(tbuf.st_rdev));

  lprintf(L_NOISE, "lock file: %s", lock);
  return(lock);
}

#else	/* not SVR4 */ 

static char * get_lock_name _P2( (lock_name, device),
			  char * lock_name, char * device )
{
#ifdef LOCKS_LOWERCASE
    /* sco locking convention -> change all device names to lowercase */

    char p[MAXLINE+1];
    int i;
    if ( ( i = strlen( device ) ) > sizeof(p) )
    {
	lprintf( L_FATAL, "get_lock_name: device name too long" );
	exit(5);
    }
    while ( i >= 0 )
    {
	p[i] = tolower( device[i] ); i--;
    }

    device = p;
#endif	/* LOCKS_LOWERCASE */

    /* throw out all directory prefixes */
    if ( strchr( device, '/' ) != NULL )
        device = strrchr( device, '/' ) +1;
    
    sprintf( lock_name, LOCK, device);

    return lock_name;
}
	
#endif /* !SVR4 */
