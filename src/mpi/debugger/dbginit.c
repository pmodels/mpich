/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* For getpid */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* There are two versions of the debugger startup:
   1. The debugger starts mpiexec - then mpiexec provides the MPIR_proctable
      information
   2. The debugger attaches to an MPI process which contains the 
      MPIR_proctable and related variables

   This file is intended to provide both as an option.  The macros that 
   control the code for these are

   MPICH2_STARTER_MPIEXEC
   MPICH2_STARTER_RANK0
 */
#define MPICH2_STARTER_MPIEXEC
/* #define MPICH2_STARTER_RANK0 */

#ifdef MPICH2_STARTER_RANK0
#define MPIU_PROCTABLE_NEEDED 1
#define MPIU_BREAKPOINT_NEEDED 1
#endif

/* If MPIR_Breakpoint is not defined and called, the message queue information
   will not be properly displayed by the debugger. */
/* I believe this was caused by a poor choice in the dll_mpich2.c file */
/* #define MPIU_BREAKPOINT_NEEDED 1 */

#ifdef MPIU_BREAKPOINT_NEEDED
/* We prototype this routine here because it is only used in this file.  It 
   is not static so that the debugger can find it (the debugger will set a 
   breakpoint at this routine */
void *MPIR_Breakpoint(void);
#endif

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
const char * MPIR_debug_abort_string = 0;

/* Values for the debug_state, this seems to be all we need at the moment
 * but that may change... 
 */
#define MPIR_DEBUG_SPAWNED   1
#define MPIR_DEBUG_ABORTING  2

#ifdef MPIU_PROCTABLE_NEEDED
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
static int MPIR_FreeProctable( void * );

#endif /* MPIR_proctable definition */

/* Other symbols:
 * MPIR_i_am_starter - Indicates that this process is not an MPI process
 *   (for example, the forker mpiexec?)
 * MPIR_acquired_pre_main - 
 * MPIR_partial_attach_ok -
*/

/* Forward references */
static void SendqInit( void );
static int SendqFreePool( void * );

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
#ifdef MPIU_PROCTABLE_NEEDED
    int rank = MPIR_Process.comm_world->rank;
    int size = MPIR_Process.comm_world->local_size;
    int i, maxsize;

    /* FIXME: In MPICH2, the executables may not have the information
       on the other processes; this is part of the Process Manager Interface
       (PMI).  We need another way to provide this information to 
       a debugger */
    /* The process manager probably has all of this data - the MPI2 
       debugger interface API provides (at least originally) a way 
       to access this. */
    /* Also, to avoid scaling problems, we only populate the first 64
       entries (default) */
    maxsize = MPIR_PARAM_PROCTABLE_SIZE;
    if (maxsize > size) maxsize = size;

    if (rank == 0) {
	char hostname[MPI_MAX_PROCESSOR_NAME+1];
	int  hostlen;
	int  val;

	MPIR_proctable    = (MPIR_PROCDESC *)MPIU_Malloc( 
					 size * sizeof(MPIR_PROCDESC) );
	for (i=0; i<size; i++) {
	    /* Initialize the proctable */
	    MPIR_proctable[i].host_name       = 0;
	    MPIR_proctable[i].executable_name = 0;
	    MPIR_proctable[i].pid             = -1;
	}

	PMPI_Get_processor_name( hostname, &hostlen );
	MPIR_proctable[0].host_name       = (char *)MPIU_Strdup( hostname );
	MPIR_proctable[0].executable_name = 0;
	MPIR_proctable[0].pid             = getpid();

	for (i=1; i<maxsize; i++) {
	    int msg[2];
	    PMPI_Recv( msg, 2, MPI_INT, i, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	    MPIR_proctable[i].pid = msg[1];
	    MPIR_proctable[i].host_name = (char *)MPIU_Malloc( msg[0] + 1 );
	    PMPI_Recv( MPIR_proctable[i].host_name, msg[0]+1, MPI_CHAR, 
		       i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
	    MPIR_proctable[i].host_name[msg[0]] = 0;
	}

	MPIR_proctable_size               = size;
#if 0
	/* Debugging hook */
	if (MPIR_PARAM_PROCTABLE_PRINT) {
	    for (i=0; i<maxsize; i++) {
		printf( "PT[%d].pid = %d, .host_name = %s\n", 
			i, MPIR_proctable[i].pid, MPIR_proctable[i].host_name );
	    }
	    fflush( stdout );
	}
#endif
	MPIR_Add_finalize( MPIR_FreeProctable, MPIR_proctable, 0 );
    }
    else {
	char hostname[MPI_MAX_PROCESSOR_NAME+1];
	int  hostlen;
	int  mypid = getpid();
	int  msg[2];
	if (rank < maxsize) {
	    PMPI_Get_processor_name( hostname, &hostlen );
	    msg[0] = hostlen;
	    msg[1] = mypid;
	    
	    /* Deliver to the root process the proctable information */
	    PMPI_Ssend( msg, 2, MPI_INT, 0, 0, MPI_COMM_WORLD );
	    PMPI_Ssend( hostname, hostlen, MPI_CHAR, 0, 0, MPI_COMM_WORLD );
	}
    }
#endif /* MPIU_PROCTABLE_NEEDED */

    /* Put the breakpoint after setting up the proctable */
    MPIR_debug_state    = MPIR_DEBUG_SPAWNED;
#ifdef MPIU_BREAKPOINT_NEEDED
    (void)MPIR_Breakpoint();
#endif
    /* After we exit the MPIR_Breakpoint routine, the debugger may have
       set variables such as MPIR_being_debugged */

#if 0
    /* Check to see if we're not the master,
     * and wait for the debugger to attach if we're 
     * a slave. The debugger will reset the debug_gate.
     * There is no code in the library which will do it !
     * 
     * THIS IS OLD CODE FROM MPICH1.  It is no longer needed for the 
     * MPIEXEC is starter process mode.
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
#endif
    /* Initialize the sendq support */
    SendqInit();

    if (getenv("MPIEXEC_DEBUG")) {
	while (!MPIR_debug_gate) ; 
    }

    
}

#ifdef MPIU_BREAKPOINT_NEEDED
/* 
 * This routine is a special dummy routine that is used to provide a
 * location for a debugger to set a breakpoint on, allowing a user (and the
 * debugger) to attach to MPI processes after MPI_Init succeeds but before
 * MPI_Init returns control to the user. It may also be called when MPI aborts, 
 * also to allow a debugger to regain control of an application.
 *
 * This routine can also initialize any datastructures that are required
 * 
 */
void * MPIR_Breakpoint( void )
{
    MPIU_DBG_MSG(OTHER,VERBOSE,"In MPIR_Breakpoint");
    return 0;
}
#endif

/* 
 * Call this routine to signal to the debugger that the application is aborting.
 * If there is an abort message, call the MPIR_Breakpoint routine (which 
 * allows a tool such as a debugger to gain control.
 */
void MPIR_DebuggerSetAborting( const char *msg )
{
    MPIR_debug_abort_string = (char *)msg;
    MPIR_debug_state        = MPIR_DEBUG_ABORTING;
#ifdef MPIU_BREAKPOINT_NEEDED
    if (msg) 
	MPIR_Breakpoint();
#endif
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

/* This routine is used to establish a queue of send requests to allow the
   debugger easier access to the active requests.  Some devices may be able
   to provide this information without requiring this separate queue. */
void MPIR_Sendq_remember( MPID_Request *req, 
			  int rank, int tag, int context_id )
{
    MPIR_Sendq *p;

    MPIU_THREAD_CS_ENTER(HANDLE,req);
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
    MPIU_THREAD_CS_EXIT(HANDLE,req);
}

void MPIR_Sendq_forget( MPID_Request *req )
{
    MPIR_Sendq *p, *prev;

    MPIU_THREAD_CS_ENTER(HANDLE,req);
    p    = MPIR_Sendq_head;
    prev = 0;

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
    MPIU_THREAD_CS_EXIT(HANDLE,req);
}

static int SendqFreePool( void *d )
{
    MPIR_Sendq *p;

    /* Free the pool */
    p = pool;
    while (p) {
	pool = p->next;
	MPIU_Free( p );
	p = pool;
    }
    /* Free the list of pending sends */
    p    = MPIR_Sendq_head;
    while (p) {
	MPIR_Sendq_head = p->next;
	MPIU_Free( p );
	p = MPIR_Sendq_head;
    }
    return 0;
}
static void SendqInit( void )
{
    int i;
    MPIR_Sendq *p;

    /* Preallocated a few send requests */
    for (i=0; i<10; i++) {
	p = (MPIR_Sendq *)MPIU_Malloc( sizeof(MPIR_Sendq) );
	if (!p) {
	    /* Just ignore it */
	    break;
	}
	p->next = pool;
	pool    = p;
    }

    /* Make sure the pool is deleted */
    MPIR_Add_finalize( SendqFreePool, 0, 0 );
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
    MPIU_THREAD_CS_ENTER(HANDLE,comm_ptr);
    if (comm_ptr == MPIR_All_communicators.head) {
	MPIU_Internal_error_printf( "Internal error: communicator is already on free list\n" );
	return;
    }
    comm_ptr->comm_next = MPIR_All_communicators.head;
    MPIR_All_communicators.head = comm_ptr;
    MPIR_All_communicators.sequence_number++;
    MPIU_THREAD_CS_EXIT(HANDLE,comm_ptr);
}

void MPIR_CommL_forget( MPID_Comm *comm_ptr )
{
    MPID_Comm *p, *prev;

    MPIU_DBG_MSG_P(COMM,VERBOSE,
		   "Forgetting communicator %p from remember list",comm_ptr);
    MPIU_THREAD_CS_ENTER(HANDLE,comm_ptr);
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
    MPIU_THREAD_CS_EXIT(HANDLE,comm_ptr);
}

#ifdef MPIU_PROCTABLE_NEEDED
/* This routine is the finalize callback used to free the procable */
static int MPIR_FreeProctable( void *ptable )
{
    int i;
    MPIR_PROCDESC *proctable = (MPIR_PROCDESC *)ptable;
    for (i=0; i<MPIR_proctable_size; i++) {
	if (proctable[i].host_name) { MPIU_Free( proctable[i].host_name ); }
    }
    MPIU_Free( proctable );

    return 0;
}
#endif /* MPIU_PROCTABLE_NEEDED */

/* 
 * There is an MPI-2 process table interface which has been defined; this
 * provides a more scalable, distributed description of the process table.
 * 
 * 
 */
