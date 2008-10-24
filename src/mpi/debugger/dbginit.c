/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* We prototype this routine here because it is only used in this file.  It 
   is not static so that the debugger can find it (the debugger will set a 
   breakpoint at this routine */
void *MPIR_Breakpoint(void);

/*
 * This file contains information and routines used to simplify the interface 
 * to a debugger.  This follows the description in "A Standard Interface 
 * for Debugger Access to Message Queue Information in MPI", by Jim Cownie
 * and William Gropp.
 *
 * This file should be compiled with debug information (-g)
 */

/*
 * In addition to the discussion in the paper "A Standard Interface for Debugger 
 * Access to Message Queue Inforation in MPI" and the more recent paper "An 
 * Interface to Support the Identification of Dynamic {MPI} 2 Processes for 
 * Scalable Parallel Debugging", there are a few features that have become
 * defacto standard.  These include the "proctable" (a relic of the way 
 * that p4 represented processes that was used in the ch_p4 device in 
 * MPICH1), a debugger state (has MPI started or exited), and a routine that
 * has the sole purpose of serving as a break point for a debugger.
 * Specifically, these extensions are:
 *
 *  void * MPIR_Breakpoint( void )
 *
 * This routine should be called at any point where control should be
 * offered to the debugger.  Typical spots are in MPI_Init/MPI_Init_thread
 * after initialization is completed and in MPI_Abort before exiting.
 *
 * MPIR_DebuggerSetAborting( const char *msg )
 *
 * This routine should be called when MPI is exiting (either in finalize
 * or abort.  If a message is provided, it will call MPIR_Breakpoint.
 * This routine sets the variables MPIR_debug_state and MPIR_debug_abort_string.
 *
 * In MPICH1, the variables MPIR_debug_state, MPIR_debug_abort_string, 
 * MPIR_being_debugged, and MPIR_debug_gate where exported globally.  
 * In MPICH2, while these are global variables (so that the debugger can
 * find them easily), they are not explicitly exported or referenced outside
 * of a few routines.  In particular, MPID_Abort uses MPIR_DebuggerSetAborting
 * instead of directly accessing these variables.
 */

/* The following is used to tell a debugger the location of the shared
   library that the debugger can load in order to access information about
   the parallel program, such as message queues */
#ifdef HAVE_DEBUGGER_SUPPORT
#ifdef MPICH_INFODLL_LOC
char MPIR_dll_name[] = MPICH_INFODLL_LOC;
#endif
#endif

/* 
 * The following variables are used to interact with the debugger.
 *
 * MPIR_debug_state 
 *    Values are 0 (before MPI_Init), 1 (after MPI_init), and 2 (Aborting).
 * MPIR_debug_gate
 *    The debugger will set this to 1 when the debugger attaches 
 *    to the process to tell the process to proceed.
 * MPIR_being_debugged
 *    Set to 1 if the process is started or attached under the debugger 
 * MPIR_debug_abort_string
 *    String that the debugger can display on an abort.
 */
volatile int MPIR_debug_state    = 0;
volatile int MPIR_debug_gate     = 0;
volatile int MPIR_being_debugged = 0;
char * MPIR_debug_abort_string   = 0;

/* Values for the debug_state, this seems to be all we need at the moment
 * but that may change... 
 */
#define MPIR_DEBUG_SPAWNED   1
#define MPIR_DEBUG_ABORTING  2

#if 0
/*
 * MPIR_PROCDESC is used to pass information to the debugger about 
 * all of the processes.
 */
typedef struct {
    char *host_name;         /* Valid name for inet_addr */
    char *executable_name;   /* The name of the image */
    int  pid;                /* The process id */
} MPIR_PROCDESC;
MPIR_PROCDESC *MPIR_proctable    = 0;
int MPIR_proctable_size          = 1;
#endif /* MPIR_proctable definition */

/* Other symbols:
 * MPIR_i_am_starter - Indicates that this process is not an MPI process
 *   (for example, the forker mpiexec?)
 * MPIR_acquired_pre_main - 
 * MPIR_partial_attach_ok -
*/

/*
 * If MPICH2 is built with the --enable-debugger option, MPI_Init and 
 * MPI_Init_thread will call MPIR_WaitForDebugger.  This ensures both that
 * the debugger can gather information on the MPI job before the MPI_Init
 * returns to the user and that the necessary symbols for providing 
 * information such as message queues is available.
 *
 * In addition, the environment variable MPIEXEC_DEBUG, if set, will cause
 * all MPI processes to wait in this routine until the variable 
 * MPIR_debug_gate is set to 1.
 */
void MPIR_WaitForDebugger( void )
{
    int rank = MPIR_Process.comm_world->rank;
    int size = MPIR_Process.comm_world->local_size;

#if 0
    /* FIXME: In MPICH2, the executables may not have the information
       on the other processes; this is part of the Process Manager Interface
       (PMI).  We need another way to provide this information to 
       a debugger */
    if (rank == 0) {
	MPIR_proctable    = (MPIR_PROCDESC *)MPIU_Malloc( size * sizeof(MPIR_PROCDESC) );
	/* Temporary to see if we can get totalview's attention */
	MPIR_proctable[0].host_name       = 0;
	MPIR_proctable[0].executable_name = 0;
	MPIR_proctable[0].pid             = getpid();

	MPIR_proctable_size               = 1;
    }

    /* Put the breakpoint after setting up the proctable */
    MPIR_debug_state    = MPIR_DEBUG_SPAWNED;
    (void)MPIR_Breakpoint();
    /* After we exit the MPIR_Breakpoint routine, the debugger may have
       set variables such as MPIR_being_debugged */

    /* Check to see if we're not the master,
     * and wait for the debugger to attach if we're 
     * a slave. The debugger will reset the debug_gate.
     * There is no code in the library which will do it !
     * 
     * THIS IS OLD CODE FROM MPICH1  FIXME
     */
    if (MPIR_being_debugged && rank != 0) {
	while (MPIR_debug_gate == 0) {
	    /* Wait to be attached to, select avoids 
	     * signaling and allows a smaller timeout than 
	     * sleep(1)
	     */
	    struct timeval timeout;
	    timeout.tv_sec  = 0;
	    timeout.tv_usec = 250000;
	    select( 0, (void *)0, (void *)0, (void *)0,
		    &timeout );
	}
    }
#endif /* MPIR_proctable definition */

    if (getenv("MPIEXEC_DEBUG")) {
	while (!MPIR_debug_gate) ; 
    }
}

/* 
 * This routine is a special dummy routine that is used to provide a
 * location for a debugger to set a breakpoint on, allowing a user (and the
 * debugger) to attach to MPI processes after MPI_Init succeeds but before
 * MPI_Init returns control to the user.  It may also be called when MPI aborts, 
 * also to allow a debugger to regain control of an application.
 *
 * This routine can also initialize any datastructures that are required
 * 
 */
void * MPIR_Breakpoint( void )
{
    return 0;
}

/* 
 * Call this routine to signal to the debugger that the application is aborting.
 * If there is an abort message, call the MPIR_Breakpoint routine (which allows a tool 
 * such as a debugger to gain control.
 */
void MPIR_DebuggerSetAborting( const char *msg )
{
    MPIR_debug_abort_string = msg;
    MPIR_debug_state        = MPIR_DEBUG_ABORTING;
    if (msg) 
	MPIR_Breakpoint();
}

/* ------------------------------------------------------------------------- */
/* 
 * Manage the send queue.
 *
 * The send queue is needed only by the debugger.  The communication
 * device has a separate notion of send queue, which are the operations
 * that it needs to complete, independent of whether the user has called
 * MPI_Wait/Test/etc on the request.
 * 
 * This implementation uses a simple linked list of user-visible requests
 * (more specifically, requests created with MPI_Isend, MPI_Issend, or 
 * MPI_Irsend).
 *
 * FIXME: We need to add MPI_Ibsend and the persistent send requests to
 * the known send requests.
 * FIXME: We need to register a Finalize call back to free memory.
 * FIXME: We should exploit this to allow Finalize to report on 
 * send requests that were never completed.
 */

/* We need to save the tag and rank since this information may not 
   be included in the request.  Saving the context_id also simplifies
   matching these entries with a communicator */
typedef struct MPIR_Sendq {
    MPID_Request *sreq;
    int tag, rank, context_id;
    struct MPIR_Sendq *next;
} MPIR_Sendq;

MPIR_Sendq *MPIR_Sendq_head = 0;
/* Keep a pool of previous sendq elements to speed allocation of queue 
   elements */
static MPIR_Sendq *pool = 0;

void MPIR_Sendq_remember( MPID_Request *req, 
			  int rank, int tag, int context_id )
{
    MPIR_Sendq *p;
    if (pool) {
	p = pool;
	pool = p->next;
    }
    else {
	p = (MPIR_Sendq *)MPIU_Malloc( sizeof(MPIR_Sendq) );
	if (!p) {
	    /* Just ignore it */
	    return;
	}
    }
    p->sreq       = req;
    p->tag        = tag;
    p->rank       = rank;
    p->context_id = context_id;
    p->next       = MPIR_Sendq_head;
    MPIR_Sendq_head = p;
}

void MPIR_Sendq_forget( MPID_Request *req )
{
    MPIR_Sendq *p, *prev;

    p    = MPIR_Sendq_head;
    prev = 0;

    /* FIXME: Make this thread-safe */
    while (p) {
	if (p->sreq == req) {
	    if (prev) prev->next = p->next;
	    else MPIR_Sendq_head = p->next;
	    /* Return this element to the pool */
	    p->next = pool;
	    pool    = p;
	    break;
	}
	prev = p;
	p    = p->next;
    }
    /* If we don't find the request, just ignore it */
}

/* Manage the known communicators */
/* Provide a list of all active communicators.  This is used only by the
   debugger message queue interface */
typedef struct MPIR_Comm_list {
    int sequence_number;   /* Used to detect changes in the list */
    MPID_Comm *head;       /* Head of the list */
} MPIR_Comm_list;

MPIR_Comm_list MPIR_All_communicators = { 0, 0 };

void MPIR_CommL_remember( MPID_Comm *comm_ptr )
{   
    MPIU_DBG_MSG_P(COMM,VERBOSE,
		   "Adding communicator %p to remember list",comm_ptr);
    /* FIXME: (MT) Ensure thread-safe */
    if (comm_ptr == MPIR_All_communicators.head) {
	MPIU_Internal_error_printf( "Internal error: communicator is already on free list\n" );
	return;
    }
    comm_ptr->comm_next = MPIR_All_communicators.head;
    MPIR_All_communicators.head = comm_ptr;
    MPIR_All_communicators.sequence_number++;
}

void MPIR_CommL_forget( MPID_Comm *comm_ptr )
{
    MPID_Comm *p, *prev;

    MPIU_DBG_MSG_P(COMM,VERBOSE,
		   "Forgetting communicator %p from remember list",comm_ptr);
    /* FIXME: (MT) Ensure thread-safe */
    p = MPIR_All_communicators.head;
    prev = 0;
    while (p) {
	if (p == comm_ptr) {
	    if (prev) prev->comm_next = p->comm_next;
	    else MPIR_All_communicators.head = p->comm_next;
	    break;
	}
	if (p == p->comm_next) {
	    MPIU_Internal_error_printf( "Mangled pointers to communicators - next is itself for %p\n", p );
	    break;
	}
	prev = p;
	p = p->comm_next;
    }
    /* Record a change to the list */
    MPIR_All_communicators.sequence_number++;
}

