#ident "$Id: locks.c,v 1.18 1993/11/05 20:22:38 gert Exp $ Copyright (c) Gert Doering / Paul Sutcliffe Jr."

/* large parts of the code in this module are taken from the
 * "getty kit 2.0" by Paul Sutcliffe, Jr., paul@devon.lns.pa.us,
 * and are used with permission here.
 * SVR4 style locking by Bodo Bauer, bodo@hal.nbg.sub.org.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

/* SVR4 uses a different locking mechanism. This is why we need this... */
#ifdef SVR4 
#include <sys/mkdev.h>
 
#define LCK_NODEV    -1
#define LCK_OPNFAIL  -2
#endif

/* some OSes do include this in stdio.h, others don't... */
#ifndef EEXIST
#include <errno.h>
#endif

#include "mgetty.h"
#include "policy.h"

static char	lock[MAXLINE+1];	/* name of the lockfile */

static int readlock _PROTO(( char * name ));
static char *  get_lock_name _PROTO(( char * lock_name, char * device ));

/*
 *	makelock() - attempt to create a lockfile
 *
 *	Returns FAIL if lock could not be made (line in use).
 */

int makelock _P1( (device),
		  char *device)
{
	int fd, pid;
	char *temp, buf[MAXLINE+1];
#if LOCKS_BINARY
	int  bpid;		/* must be 4 byte long! */
#else
	char apid[16];
#endif

	lprintf(L_NOISE, "makelock(%s) called", device);

	if ( get_lock_name( lock, device ) == NULL )
	{
	    lprintf( L_ERROR, "cannot get lock name" );
	    return FAIL;
	}
	lprintf( L_NOISE, "lock='%s'", lock );

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
	(void) write(fd, &bpid, sizeof( bpid ) );
#else
	(void) sprintf(apid, "%10d\n", getpid());
	(void) write(fd, apid, strlen(apid));
#endif
	(void) close(fd);

	/* link it to the lock file */

	while (link(temp, lock) == FAIL) {
		lprintf(L_ERROR, "link(temp,lock) failed" );

		if (errno == EEXIST) {		/* lock file already there */
			if ((pid = readlock(lock)) == FAIL)
			{
			    lprintf( L_NOISE, "cannot read lockfile" );
			    if ( errno == ENOENT )	/* disappeared */
				continue;
			    else
				return FAIL;
			}

			if (pid == getpid())	/* huh? WE locked the line!*/
			{
				lprintf( L_WARN, "we *have* the line!" );
				unlink(temp);
				return SUCCESS;
			}

			if ((kill(pid, 0) == FAIL) && errno == ESRCH) {
				/* pid that created lockfile is gone */
				lprintf( L_NOISE, "stale lockfile, created by process %d, ignoring", pid );
				(void) unlink(lock);
				continue;
			}
		}
		lputs(L_MESG, " - lock NOT made");
		(void) unlink(temp);
		return(FAIL);
	}
	lprintf(L_NOISE, "lock made");
	(void) unlink(temp);
	return(SUCCESS);
}

/*
 *	checklock() - test for presense of valid lock file
 *
 *	Returns TRUE if lockfile found, FALSE if not.
 */

boolean checklock _P1( (device),
		       char * device)
{
	int pid;
	struct stat st;
	char name[MAXLINE+1];

	if ( get_lock_name( name, device ) == NULL )
	{
	    lprintf( L_ERROR, "cannot get lock name" );
	    return FALSE;
	}

	if ((stat(name, &st) == FAIL) && errno == ENOENT) {
		lprintf(L_NOISE, "checklock: stat failed, no file");
		return(FALSE);
	}

	if ((pid = readlock(name)) == FAIL) {
		lprintf(L_MESG, "checklock: couldn't read lockfile");
		return(FALSE);
	}

        if (pid == getpid())
	{
	        lprintf(L_WARN, "huh? It's *our* lock file!" );
		return(FALSE);
	}
		
	if ((kill(pid, 0) == FAIL) && errno == ESRCH) {
		lprintf(L_NOISE, "checklock: no active process has lock, will remove");
		(void) unlink(name);
		return(FALSE);
	}
	lprintf(L_NOISE, "lockfile found" );

	return(TRUE);
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
	}

	(void) close(fd);
	return(pid);
}

/*
 *	rmlocks() - remove lockfile
 */

RETSIGTYPE rmlocks()
{
	lprintf( L_NOISE, "removing lock file" );
	(void) unlink(lock);
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

  lprintf(L_NOISE, "get_lock_name(%s,%s) called", lock, fax_tty);

  sprintf(ttyname, "/dev/%s", fax_tty);
  
  lprintf(L_NOISE, "ttyname %s", ttyname);

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
    (void) sprintf(lock_name, LOCK, device);
    return lock_name;
}
	
#endif /* SVR4 */
