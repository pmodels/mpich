/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* 
 * This file is a first cut at providing the information needed by the 
 * MPI debugging interface to the debugger through the mpiexec process.
 * This primarily consists of making the process location and pid information
 * available in a standard format (separate from that in the data structures 
 * defined in process.h)
 */
#include "pmutilconf.h"
#include "pmutil.h"
#include <stdio.h>
#include "mpibase.h"
#include "process.h"

#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME 1024
#endif

/* This structure is defined by the debugger interface */
typedef struct MPIR_PROCDESC {
  const char *host_name;
  const char *executable_name;
  int  pid;
} MPIR_PROCDESC;

/* An array of processes */
struct MPIR_PROCDESC *MPIR_proctable = 0;
/* This global variable defines the number of MPIR_PROCDESC entries for the
   debugger */
int MPIR_proctable_size = 0;

/* The presence of this variable tells the debugger that this process starts
   MPI jobs and isn't part of the MPI_COMM_WORLD */
int MPIR_i_am_starter = 0;

/* The presence of this variable tells the debugger that it need not attach to
   all processes to get them running */
int MPIR_partial_attach_ok = 0;

/* Two global variables which a debugger can use for
   1) finding out the state of the program
   2) informing the process that it has been attached to
*/
enum 
{
  MPIR_NULL          = 0,
  MPIR_DEBUG_SPAWNED = 1,
  MPIR_DEBUG_ABORTING= 2
};

volatile int MPIR_debug_state = 0;
char *MPIR_debug_abort_string = 0;

/* Set by the debugger when it attaches to this process. */
volatile int MPIR_being_debugged = 0;

int MPIR_Breakpoint( void );
/* We make this a global pointer to prohibit the compiler from inlining 
   the breakpoint functions (without this, gcc often removes the call)
   Neither the attribute __attribute__((noinline)) nor the use of 
   asm(""), recommended by the GCC manual, prevented the inlining of
   this call.  Rather than place it in a separate file (and still
   risk whole-program analysis removal), we use a globally visable
   function pointer.
*/
int (*MPIR_breakpointFn)(void) = MPIR_Breakpoint;

int MPIE_InitForDebugger( ProcessWorld *pWorld )
{
    int np    = pWorld->nProcess;
    int i, j;
    ProcessApp *apps = pWorld->apps;
    static int hostnameSet=0;
    static char myhostname[MAX_HOST_NAME+1];

    MPIR_proctable = (struct MPIR_PROCDESC *)
	MPIU_Malloc( np * sizeof(struct MPIR_PROCDESC) );

    i = 0;
    while (apps && i < np) {
	for (j=0; j<apps->nProcess && i < np; j++) {
	    const char *hostname;
	    /* Some process managers may not fill in the hostname
	       field (e.g., gforker) */
	    if (!apps->hostname) {
		if (!hostnameSet) {		    
		    myhostname[0] = 0;
		    MPIE_GetMyHostName( myhostname, MAX_HOST_NAME );
		    hostnameSet = 1;
		}
		hostname = (const char *)myhostname;
	    }
	    else
		hostname = apps->hostname;
	    MPIR_proctable[i].host_name       = hostname;
	    MPIR_proctable[i].executable_name = apps->exename;
	    MPIR_proctable[i].pid             = (int)apps->pState[j].pid;
	    i++;
	}
	apps = apps->nextApp;
    }
    /* Sanity check */
    if (i != np || apps) {
	/* This can happen only if the apps list describes more (or fewer) 
	   processes than expected. */
	printf( "Error!\n" ); 
	return -1;
    }

    /* Set the size at the very end in case the debugger is watching too 
       closely */
    MPIR_proctable_size = np;
    MPIR_debug_state    = MPIR_DEBUG_SPAWNED;
    (*MPIR_breakpointFn)();
    /*MPIR_Breakpoint(); */
    
    return 0;
}

/* This routine is provided to free memory allocated in this routine */
int MPIE_FreeFromDebugger( void )
{
    if (MPIR_proctable) { MPIU_Free( MPIR_proctable ); }
    return 0;
}

/*
 * MPIR_Breakpoint - Provide a routine that a debugger can intercept
 *                   at interesting times.
 *		     Note that before calling this you should set up
 *		     MPIR_debug_state, so that the debugger can see
 *		     what is going on.
 */
int MPIR_Breakpoint(void)
{
    /*    printf( "calling breakpoint\n" ); */
    return 0;
} /* MPIR_Breakpoint */

/* For debugging purposes */
int MPIE_PrintDebuggerInfo( FILE *fp )
{
    int i;
    
    for (i=0; i<MPIR_proctable_size; i++) {
	fprintf( fp, "rank %d: exec = %s, host = %s, pid = %d\n", 
		 i, 
		 MPIR_proctable[i].executable_name,
		 MPIR_proctable[i].host_name,
		 MPIR_proctable[i].pid );
    }
    fflush( fp );
    return 0;
}

