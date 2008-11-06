/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "pmutilconf.h"
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
/* To get the prototype for getsid in gcc environments, 
   define both _XOPEN_SOURCE and _XOPEN_SOURCE_EXTENDED  or
   define _XOPEN_SOURCE n for n >= 500 */
#include <unistd.h>
#endif
#include <errno.h>
#include "pmutil.h"

/* ------------------------------------------------------------------------ */
/* On some systems (SGI IRIX 6), process exit sometimes kills all processes
 * in the process GROUP.  This code attempts to fix that.  
 * We DON'T do it if stdin (0) is connected to a terminal, because that
 * disconnects the process from the terminal.
 * ------------------------------------------------------------------------ */
void MPIE_CreateNewSession( void )
{
#if defined(HAVE_SETSID) && defined(HAVE_ISATTY) && \
    defined(USE_NEW_SESSION) && defined(HAVE_GETSID)
#ifdef NEEDS_GETSID_PROTOTYPE
pid_t getsid(pid_t);
#endif
if (!isatty(0) && !isatty(1) && !isatty(2) && getsid(0) != getpid()) {
    pid_t rc;
    /* printf( "Creating a new session\n" ); */
/*    printf( "Session id = %d and process id = %d\n", getsid(0), getpid() );*/
    MPIE_SYSCALL(rc,setsid,());
    if (rc < 0) {
	MPIU_Internal_sys_error_printf( "setsid", errno, 
				"Could not create new process group\n" );
	}
    }
#endif
}
