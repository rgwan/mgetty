#ident "$Id: locks.c,v 1.1 1993/02/24 17:21:41 gert Exp $ Gert Doering / Paul Sutcliffe Jr."

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#include "mgetty.h"

char	*lock;				/* name of the lockfile */

/*
**	makelock() - attempt to create a lockfile
**
**	Returns FAIL if lock could not be made (line in use).
*/

int
makelock(char *name)
{
	int fd, pid;
	char *temp, buf[MAXLINE+1];
	char apid[16];

	lprintf(L_NOISE, "makelock(%s) called", name);

	/* first make a temp file */

	(void) sprintf(buf, LOCK, "TM.XXXXXX");
	if ((fd = creat((temp=mktemp(buf)), 0444)) == FAIL) {
		lprintf(L_ERROR, "cannot create tempfile (%s)", temp);
		return(FAIL);
	}

	/* put my pid in it */

	(void) sprintf(apid, "%10d\n", getpid());
	(void) write(fd, apid, strlen(apid));
	(void) close(fd);

	/* link it to the lock file */

	while (link(temp, name) == FAIL) {
		lprintf(L_ERROR, "link(temp,name) failed" );

		if (errno == EEXIST) {		/* lock file already there */
			if ((pid = readlock(name)) == FAIL)
				continue;
			if (pid == getpid())	/* huh? WE locked the line!*/
			{
				lprintf( L_WARN, "we *have* the line!" );
				unlink(temp);
				return SUCCESS;
			}

			if ((kill(pid, 0) == FAIL) && errno == ESRCH) {
				/* pid that created lockfile is gone */
				(void) unlink(name);
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
**	checklock() - test for presense of valid lock file
**
**	Returns TRUE if lockfile found, FALSE if not.
*/

boolean
checklock(char * name)
{
	int pid;
	struct stat st;

	if ((stat(name, &st) == FAIL) && errno == ENOENT) {
		lprintf(L_NOISE, "checklock: stat failed, no file");
		return(FALSE);
	}

	if ((pid = readlock(name)) == FAIL) {
		lprintf(L_MESG, "checklock: couldn't read lockfile");
		return(FALSE);
	}

	if ((kill(pid, 0) == FAIL) && errno == ESRCH) {
		lprintf(L_NOISE, "checklock: no active process has lock, will remove");
		(void) unlink(name);
		return(FALSE);
	}

	return(TRUE);
}

/*
**	readlock() - read contents of lockfile
**
**	Returns pid read or FAIL on error.
*/

int readlock( char * name )
{
	int fd, pid;
	char apid[16];

	if ((fd = open(name, O_RDONLY)) == FAIL)
		return(FAIL);

	(void) read(fd, apid, sizeof(apid));
	(void) sscanf(apid, "%d", &pid);

	(void) close(fd);
	return(pid);
}

/*
**	rmlocks() - remove lockfile(s)
*/

sig_t
rmlocks()
{
	(void) unlink(lock);
}
