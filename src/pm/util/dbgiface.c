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
#include <stdio.h>
#include "mpibase.h"
#include "mpimem.h"
#include "process.h"

/* This structure is defined by the debugger interface */
typedef struct MPIR_PROCDESC {
  const char *host_name;
  const char *executable_name;
  long  pid;
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
volatile int MPIR_debug_state = 0;
char *MPIR_debug_abort_string = 0;

int MPIE_InitForDebugger( ProcessWorld *pWorld )
{
    int np    = pWorld->nProcess;
    int i, j;
    ProcessApp *apps = pWorld->apps;

    MPIR_proctable = (struct MPIR_PROCDESC *)
	MPIU_Malloc( np * sizeof(struct MPIR_PROCDESC) );

    i = 0;
    while (apps && i < np) {
	for (j=0; j<apps->nProcess && i < np; j++) {
	    MPIR_proctable[i].host_name       = apps->hostname;
	    MPIR_proctable[i].executable_name = apps->exename;
	    MPIR_proctable[i].pid             = apps->pState[j].pid;
	    i++;
	}
	apps = apps->nextApp;
    }
    /* Sanity check */
    if (i != np || apps) {
	return -1;
    }

    /* Set the size at the very end in case the debugger is watching too 
       closely */
    MPIR_proctable_size = np;
    
    return 0;
}

/* This routine is provided to free memory allocated in this routine */
int MPIE_FreeFromDebugger( void )
{
    if (MPIR_proctable) { MPIU_Free( MPIR_proctable ); }
    return 0;
}
