/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: commutil.c,v 1.80 2007/05/04 17:16:15 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */
#ifndef MPID_COMM_PREALLOC 
#define MPID_COMM_PREALLOC 8
#endif

/* Preallocated comm objects */
MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN] = { {0,0} };
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC] = { {0,0} };
MPIU_Object_alloc_t MPID_Comm_mem = { 0, 0, 0, 0, MPID_COMM, 
				      sizeof(MPID_Comm), MPID_Comm_direct,
                                      MPID_COMM_PREALLOC};

/* Support for threading */

#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context)
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context) \
		MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex);\
		MPID_Thread_yield();\
		MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex);

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL || \
      MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
/* There is a single, global lock, held only when needed */
#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context) \
    MPIU_THREAD_CHECKDEPTH(global_mutex,1);\
		MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex);\
		MPID_Thread_yield();\
		MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex);

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_LOCK_FREE
/* Updates to shared data and access to shared services is handled without 
   locks where ever possible. */
#error lock-free not yet implemented

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_SINGLE
/* No thread support, make all operations a no-op */

#else
#error Unrecognized thread granularity
#endif

/* FIXME :
   Reusing context ids can lead to a race condition if (as is desirable)
   MPI_Comm_free does not include a barrier.  Consider the following:
   Process A frees the communicator.
   Process A creates a new communicator, reusing the just released id
   Process B sends a message to A on the old communicator.
   Process A receives the message, and believes that it belongs to the
   new communicator.
   Process B then cancels the message, and frees the communicator.

   The likelyhood of this happening can be reduced by introducing a gap
   between when a context id is released and when it is reused.  An alternative
   is to use an explicit message (in the implementation of MPI_Comm_free)
   to indicate that a communicator is being freed; this will often require
   less communication than a barrier in MPI_Comm_free, and will ensure that 
   no messages are later sent to the same communicator (we may also want to
   have a similar check when building fault-tolerant versions of MPI).
 */

/* Create a new communicator with a context.  
   Do *not* initialize the other fields except for the reference count.
   See MPIR_Comm_copy for a function to produce a copy of part of a
   communicator 
*/


/*  
    Create a communicator structure and perform basic initialization 
    (mostly clearing fields and updating the reference count).  
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create
#undef FCNAME
#define FCNAME "MPIR_Comm_create"
int MPIR_Comm_create( MPID_Comm **newcomm_ptr )
{   
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *newptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE);

    newptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    /* --BEGIN ERROR HANDLING-- */
    if (!newptr) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
		   FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    *newcomm_ptr = newptr;
    MPIU_Object_set_ref( newptr, 1 );

    /* Clear many items (empty means to use the default; some of these
       may be overridden within the communicator initialization) */
    newptr->errhandler   = 0;
    newptr->attributes	 = 0;
    newptr->remote_group = 0;
    newptr->local_group	 = 0;
    newptr->coll_fns	 = 0;
    newptr->topo_fns	 = 0;
    newptr->name[0]	 = 0;

    /* Fields not set include context_id, remote and local size, and 
       kind, since different communicator construction routines need 
       different values */

    /* Insert this new communicator into the list of known communicators.
       Make this conditional on debugger support to match the test in 
       MPIR_Comm_release . */
    MPIR_COMML_REMEMBER( newptr );

 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE);

    return mpi_errno;
}

/* Create a local intra communicator from the local group of the 
   specified intercomm. */
/* FIXME : 
   For the context id, use the intercomm's context id + 2.  (?)
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Setup_intercomm_localcomm
#undef FCNAME
#define FCNAME "MPIR_Setup_intercomm_localcomm"
int MPIR_Setup_intercomm_localcomm( MPID_Comm *intercomm_ptr )
{
    MPID_Comm *localcomm_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    localcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    /* --BEGIN ERROR HANDLING-- */
    if (!localcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
					  FCNAME, __LINE__,
					  MPI_ERR_OTHER, "**nomem", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    MPIU_Object_set_ref( localcomm_ptr, 1 );
    /* Note that we must not free this context id since we are sharing it
       with the intercomm's context */
    /* FIXME: This was + 2 (in agreement with the docs) but that
       caused some errors with an apparent use of the same context id
       by operations in different communicators.  Switching this to +1
       seems to have fixed that problem, but this isn't the right answer. */
/*    printf( "intercomm context ids; %d %d\n",
      intercomm_ptr->context_id, intercomm_ptr->recvcontext_id ); */
    /* We use the recvcontext id for both contextids for the localcomm 
     because the localcomm is an intra (not inter) communicator */
    localcomm_ptr->context_id	  = intercomm_ptr->recvcontext_id + 1;
    localcomm_ptr->recvcontext_id = intercomm_ptr->recvcontext_id + 1;

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( intercomm_ptr->local_vcrt );
    localcomm_ptr->vcrt = intercomm_ptr->local_vcrt;
    localcomm_ptr->vcr  = intercomm_ptr->local_vcr;

    /* Save the kind of the communicator */
    localcomm_ptr->comm_kind   = MPID_INTRACOMM;
    
    /* Set the sizes and ranks */
    localcomm_ptr->remote_size = intercomm_ptr->local_size;
    localcomm_ptr->local_size  = intercomm_ptr->local_size;
    localcomm_ptr->rank        = intercomm_ptr->rank;

    /* More advanced version: if the group is available, dup it by 
       increasing the reference count */
    localcomm_ptr->local_group  = 0;
    localcomm_ptr->remote_group = 0;

    /* This is an internal communicator, so ignore */
    localcomm_ptr->errhandler = 0;
    
    /* FIXME  : No local functions for the collectives */
    localcomm_ptr->coll_fns = 0;

    /* FIXME  : No local functions for the topology routines */
    localcomm_ptr->topo_fns = 0;

    /* We do *not* inherit any name */
    localcomm_ptr->name[0] = 0;

    localcomm_ptr->attributes = 0;

    intercomm_ptr->local_comm = localcomm_ptr;

 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    return mpi_errno;;
}
/*
 * Here are the routines to find a new context id.  The algorithm is discussed 
 * in detail in the mpich2 coding document.  There are versions for
 * single threaded and multithreaded MPI.
 * 
 * These assume that int is 32 bits; they should use uint_32 instead,
 * and an MPI_UINT32 type (should be able to use MPI_INTEGER4). Code
 * in src/mpid/ch3/src/ch3u_port.c for creating the temporary
 * communicator assumes that the context_mask array is made up of
 * unsigned ints. If this is changed, that code will need to be
 * changed as well.
 *
 * Both the threaded and non-threaded routines use the same mask of
 * available context id values.
 */
static unsigned int context_mask[MPIR_MAX_CONTEXT_MASK];
static int initialize_context_mask = 1;

#ifdef USE_DBG_LOGGING
/* Create a string that contains the context mask.  This is
   used only with the logging interface, and must be used by one thread at 
   a time (should this be enforced by the logging interface?).
   Converts the mask to hex and returns a pointer to that string */
static char *MPIR_ContextMaskToStr( void )
{
    static char bufstr[MPIR_MAX_CONTEXT_MASK*8+1];
    int i;
    int maxset=0;

    for (maxset=MPIR_MAX_CONTEXT_MASK-1; maxset>=0; maxset--) {
	if (context_mask[maxset] != 0) break;
    }

    for (i=0; i<maxset; i++) {
	MPIU_Snprintf( &bufstr[i*8], 9, "%.8x", context_mask[i] );
    }
    return bufstr;
}
#endif

static void MPIR_Init_contextid(void)
{
    int i;

    for (i=1; i<MPIR_MAX_CONTEXT_MASK; i++) {
	context_mask[i] = 0xFFFFFFFF;
    }
    /* the first three values are already used (comm_world, comm_self,
       and the internal-only copy of comm_world) */
    context_mask[0] = 0xFFFFFFF8; 
    initialize_context_mask = 0;
}
/* Return the context id corresponding to the first set bit in the mask.
   Return 0 if no bit found */
static int MPIR_Find_context_bit( unsigned int local_mask[] ) {
    int i, j, context_id = 0;
    for (i=0; i<MPIR_MAX_CONTEXT_MASK; i++) {
	if (local_mask[i]) {
	    /* There is a bit set in this word. */
	    register unsigned int val, nval;
	    /* The following code finds the highest set bit by recursively
	       checking the top half of a subword for a bit, and incrementing
	       the bit location by the number of bit of the lower sub word if 
	       the high subword contains a set bit.  The assumption is that
	       full-word bitwise operations and compares against zero are 
	       fast */
	    val = local_mask[i];
	    j   = 0;
	    nval = val & 0xFFFF0000;
	    if (nval) {
		j += 16;
		val = nval;
	    }
	    nval = val & 0xFF00FF00;
	    if (nval) {
		j += 8;
		val = nval;
	    }
	    nval = val & 0xF0F0F0F0;
	    if (nval) {
		j += 4;
		val = nval;
	    }
	    nval = val & 0xCCCCCCCC;
	    if (nval) {
		j += 2;
		val = nval;
	    }
	    if (val & 0xAAAAAAAA) { 
		j += 1;
	    }
	    context_mask[i] &= ~(1<<j);
	    context_id = 4 * (32 * i + j);
	    MPIU_DBG_MSG_FMT(COMM,VERBOSE,(MPIU_DBG_FDEST,
                    "allocating contextid = %d, (mask[%d], bit %d)", 
		    context_id, i, j ) ); 
	    return context_id;
	}
    }
    return 0;
}
#ifndef MPICH_IS_THREADED
/* Unthreaded (only one MPI call active at any time) */

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid
#undef FCNAME
#define FCNAME "MPIR_Get_contextid"
int MPIR_Get_contextid( MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id )
{
    int mpi_errno = MPI_SUCCESS;
    unsigned int local_mask[MPIR_MAX_CONTEXT_MASK];
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    *context_id = 0;

    if (initialize_context_mask) {
	MPIR_Init_contextid();
    }
    memcpy( local_mask, context_mask, MPIR_MAX_CONTEXT_MASK * sizeof(int) );

    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();
    /* Comm must be an intracommunicator */
    mpi_errno = NMPI_Allreduce( MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK, 
				MPI_INT, MPI_BAND, comm_ptr->handle );
    MPIR_Nest_decr();
    /* FIXME: We should return the error code upward */
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *context_id = MPIR_Find_context_bit( local_mask );

fn_exit:
    MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#else /* MPICH_IS_THREADED is set and true */

/* Additional values needed to maintain thread safety */
static volatile int mask_in_use = 0;
/* lowestContextId is used to break ties when multiple threads
   are contending for the mask */
#define MPIR_MAXID (1 << 30)
static volatile int lowestContextId = MPIR_MAXID;

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid
#undef FCNAME
#define FCNAME "MPIR_Get_contextid"
int MPIR_Get_contextid( MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id )
{
    int          mpi_errno = MPI_SUCCESS;
    unsigned int local_mask[MPIR_MAX_CONTEXT_MASK];
    int          own_mask = 0;
    int          testCount = 10; /* if you change this value, you need to also change 
				    it below where it is reinitialized */

    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    MPIU_THREADPRIV_GET;

    *context_id = 0;

    /* We increment the nest level now because we need to know that we're
     within another MPI routine before calling the CS_ENTER macro */
    MPIR_Nest_incr();

    /* The SINGLE_CS_ENTER/EXIT macros are commented out because this
       routine shold always be called from within a routine that has 
       already entered the single critical section.  However, in a 
       finer-grained approach, these macros indicate where atomic updates
       to the shared data structures must be protected. */
    /* We lock only around access to the mask.  If another thread is
       using the mask, we take a mask of zero */
    MPIU_DBG_MSG_FMT( COMM, VERBOSE, (MPIU_DBG_FDEST,
         "Entering; shared state is %d:%d", mask_in_use, lowestContextId ) );
    /* We need a special test in this loop for the case where some process
     has exhausted its supply of context ids.  In the single threaded case, 
     this is simple, because the algorithm is deterministic (see above).  In 
     the multithreaded case, it is more complicated, because we may get a
     zero for the context mask because some other thread holds the mask.  
     In addition, we can't check for the case where this process did not
     select MPI_THREAD_MULTIPLE, because one of the other processes
     may have selected MPI_THREAD_MULTIPLE.  To handle this case, after a 
     fixed number of failures, we test to see if some process has exhausted 
     its supply of context ids.  If so, all processes can invoke the 
     out-of-context-id error.  That fixed number of tests is in testCount */
    while (*context_id == 0) {
	MPIU_THREAD_CS_ENTER(CONTEXTID,);
	if (initialize_context_mask) {
	    MPIR_Init_contextid();
	}
	if (mask_in_use || comm_ptr->context_id > lowestContextId) {
	    memset( local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int) );
	    own_mask        = 0;
	    if (comm_ptr->context_id < lowestContextId) {
		lowestContextId = comm_ptr->context_id;
	    }
	    MPIU_DBG_MSG_D( COMM, VERBOSE, 
	       "In in-use, set lowestContextId to %d", lowestContextId );
	}
	else {
	    memcpy( local_mask, context_mask, MPIR_MAX_CONTEXT_MASK * sizeof(int) );
	    mask_in_use     = 1;
	    own_mask        = 1;
	    lowestContextId = comm_ptr->context_id;
	    MPIU_DBG_MSG( COMM, VERBOSE, "Copied local_mask" );
	}
	MPIU_THREAD_CS_EXIT(CONTEXTID,);
	
	/* Now, try to get a context id */
        MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);
	mpi_errno = NMPI_Allreduce( MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK,
				    MPI_INT, MPI_BAND, comm_ptr->handle );
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	if (own_mask) {
	    /* There is a chance that we've found a context id */
	    MPIU_THREAD_CS_ENTER(CONTEXTID,);
	    /* Find_context_bit updates the context array if it finds a match */
	    *context_id = MPIR_Find_context_bit( local_mask );
	    MPIU_DBG_MSG_D( COMM, VERBOSE, 
			    "Context id is now %hd", *context_id );
	    if (*context_id > 0) {
		/* If we were the lowest context id, reset the value to
		   allow the other threads to compete for the mask */
		if (lowestContextId == comm_ptr->context_id) {
		    lowestContextId = MPIR_MAXID;
		    /* Else leave it alone; there is another thread waiting */
		}
	    }
	    else {
		/* else we did not find a context id. Give up the mask in case
		   there is another thread (with a lower context id) waiting for
		   it.
		   We need to ensure that any other threads have the 
		   opportunity to run.  We do this by releasing the single
		   mutex, yielding, and then reaquiring the mutex.
		   We might want to do something more sophisticated, such
		   as using a condition variable (if we know for sure that
		   there is another thread on this process that is waiting).
		*/
		MPIU_THREAD_CS_YIELD(CONTEXTID,);
#if 0
		/* The old code */
		MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex);
		MPID_Thread_yield();
		MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex);
#endif
	    }
	    mask_in_use = 0;
	    MPIU_THREAD_CS_EXIT(CONTEXTID,);
	}
	else {
	    /* As above, force this thread to yield */
	    /* FIXME: TEMP for current yield definition*/
	    MPIU_THREAD_CS_YIELD(CONTEXTID,);
#if 0
	    MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex);
	    MPID_Thread_yield();
	    MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex);
#endif
	}
	/* Here is the test for out-of-context ids */
	if ((testCount-- == 0) && (*context_id == 0)) {
	    int hasNoId, totalHasNoId;
	    /* We don't need to lock on this because we're just looking for
	       zero or nonzero */
	    hasNoId = MPIR_Find_context_bit( context_mask ) == 0;
	    mpi_errno = NMPI_Allreduce( &hasNoId, &totalHasNoId, 1, MPI_INT, 
			    MPI_MAX, comm_ptr->handle );
	    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    if (totalHasNoId == 1) {
		/* Release the mask for use by other threads */
		if (own_mask) {
		    mask_in_use = 0;
		}
		MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**toomanycomm" );
	    }
	    else { /* reinitialize testCount */
		testCount = 10;
	    }
	}
    }

fn_exit:
    MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());
    MPIR_Nest_decr();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;
fn_fail:
    /* Release the masks */
    if (own_mask) {
        mask_in_use = 0;
    }
    goto fn_exit;
}
#endif

/* Get a context for a new intercomm.  There are two approaches 
   here (for MPI-1 codes only)
   (a) Each local group gets a context; the groups exchange, and
       the low value is accepted and the high one returned.  This
       works because the context ids are taken from the same pool.
   (b) Form a temporary intracomm over all processes and use that
       with the regular algorithm.
   
   In some ways, (a) is the better approach because it is the one that
   extends to MPI-2 (where the last step, returning the context, is 
   not used and instead separate send and receive context id value 
   are kept).  For this reason, we'll use (a).

   Even better is to separate the local and remote context ids.  Then
   each group of processes can manage their context ids separately.
*/
/* 
 * This uses the thread-safe (if necessary) routine to get a context id
 * and does not need its own thread-safe version.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_intercomm_contextid
#undef FCNAME
#define FCNAME "MPIR_Get_intercomm_contextid"
int MPIR_Get_intercomm_contextid( MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, 
				  MPIR_Context_id_t *recvcontext_id )
{
    MPIR_Context_id_t mycontext_id, remote_context_id;
    int mpi_errno = MPI_SUCCESS;
    int tag = 31567; /* FIXME  - we need an internal tag or 
		        communication channel.  Can we use a different
		        context instead?.  Or can we use the tag 
		        provided in the intercomm routine? (not on a dup, 
			but in that case it can use the collective context) */
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Get_contextid( comm_ptr->local_comm, &mycontext_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(mycontext_id != 0);

    MPIU_THREADPRIV_GET;

    /* MPIC routine uses an internal context id.  The local leads (process 0)
       exchange data */
    remote_context_id = -1;
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv( &mycontext_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag,
                                   &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag, 
                                   comm_ptr->handle, MPI_STATUS_IGNORE );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* Make sure that all of the local processes now have this
       id */
    MPIR_Nest_incr();
    mpi_errno = NMPI_Bcast( &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 
                            0, comm_ptr->local_comm->handle );
    MPIR_Nest_decr();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *context_id     = remote_context_id;
    *recvcontext_id = mycontext_id;
 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Free_contextid
#undef FCNAME
#define FCNAME "MPIR_Free_contextid"
void MPIR_Free_contextid( MPIR_Context_id_t context_id )
{
    int idx, bitpos;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_FREE_CONTEXTID);
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_FREE_CONTEXTID);

    /* Convert the context id to the bit position */
    /* FIXME why do we shift right by 2?  What exactly are those bottom two bits
       used for? */
    /* Note: The use of the context_id is covered in the design document.  They
       are used to provide private context ids for different communication 
       operations, such as collective communication, without requiring a
       separate communicator.  4 ids were originally allocated to ensure
       separation between pt-2-pt, collective, RMA, and intercomm pt-2-pt */
    idx    = (context_id >> 2) / 32;
    bitpos = (context_id >> 2) % 32;

    /* --BEGIN ERROR HANDLING-- */
    if (idx < 0 || idx >= MPIR_MAX_CONTEXT_MASK) {
	MPID_Abort( 0, MPI_ERR_INTERN, 1, 
		    "In MPIR_Free_contextid, idx is out of range" );
    }

    /* Check that this context id has been allocated */
    if ( (context_mask[idx] & (0x1 << bitpos)) != 0) {
        MPIU_DBG_MSG_D(COMM,VERBOSE,"context_id=%d", context_id);
        MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());
        /* FIXME This abort cannot be enabled at this time.  The local and
           remote communicators in an intercommunicator (always?) share a
           context_id prefix (bits 15..2) and free will be called on both of
           them by the higher level code.  This should probably be fixed but we
           can't until we understand the context_id code better.  One possible
           solution is to only free when (context_id&0x3)!=0.
           [goodell@ 2008-08-18]

	MPID_Abort( 0, MPI_ERR_INTERN, 1, 
		    "In MPIR_Free_contextid, the context id is not in use" );
         */
    }
    /* --END ERROR HANDLING-- */

    MPIU_THREAD_CS_ENTER(CONTEXTID,);
    /* MT: Note that this update must be done atomically in the multithreaded
       case.  In the "one, single lock" implementation, that lock is indeed
       held when this operation is called. */
    context_mask[idx] |= (0x1 << bitpos);
    MPIU_THREAD_CS_EXIT(CONTEXTID,);

    MPIU_DBG_MSG_FMT(COMM,VERBOSE,(MPIU_DBG_FDEST,
			"Freed context %d, mask[%d] bit %d", 
			context_id, idx, bitpos ) );
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_FREE_CONTEXTID);
}

/*
 * Copy a communicator, including creating a new context and copying the
 * virtual connection tables and clearing the various fields.
 * Does *not* copy attributes.  If size is < the size of the local group
 * in the input communicator, copy only the first size elements.
 * If this process is not a member, return a null pointer in outcomm_ptr.
 * This is only supported in the case where the communicator is in 
 * Intracomm (not an Intercomm).  Note that this is all that is required
 * for cart_create and graph_create.
 *
 * Used by cart_create, graph_create, and dup_create 
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_copy
#undef FCNAME
#define FCNAME "MPIR_Comm_copy"
int MPIR_Comm_copy( MPID_Comm *comm_ptr, int size, MPID_Comm **outcomm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id, new_recvcontext_id;
    MPID_Comm *newcomm_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_COPY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_COPY);

    /* Get a new context first.  We need this to be collective over the
       input communicator */
    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id.  Be careful
       of intercomms here */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	mpi_errno = 
	    MPIR_Get_intercomm_contextid( 
		 comm_ptr, &new_context_id, &new_recvcontext_id );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
	mpi_errno = MPIR_Get_contextid( comm_ptr, &new_context_id );
	new_recvcontext_id = new_context_id;
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_Assert(new_context_id != 0);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (new_context_id == 0) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**toomanycomm" );
    }
    /* --END ERROR HANDLING-- */

    /* This is the local size, not the remote size, in the case of
       an intercomm */
    if (comm_ptr->rank >= size) {
	*outcomm_ptr = 0;
	goto fn_exit;
    }

    /* We're left with the processes that will have a non-null communicator.
       Create the object, initialize the data, and return the result */

    mpi_errno = MPIR_Comm_create( &newcomm_ptr );
    if (mpi_errno) goto fn_fail;

    newcomm_ptr->context_id     = new_context_id;
    newcomm_ptr->recvcontext_id = new_recvcontext_id;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind   = comm_ptr->comm_kind;
    newcomm_ptr->local_comm  = 0;

    /* There are two cases here - size is the same as the old communicator,
       or it is smaller.  If the size is the same, we can just add a reference.
       Otherwise, we need to create a new VCRT.  Note that this is the
       test that matches the test on rank above. */
    if (size == comm_ptr->local_size) {
	/* Duplicate the VCRT references */
	MPID_VCRT_Add_ref( comm_ptr->vcrt );
	newcomm_ptr->vcrt = comm_ptr->vcrt;
	newcomm_ptr->vcr  = comm_ptr->vcr;
    }
    else {
	int i;
	/* The "remote" vcr gets the shortened vcrt */
	MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
	MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, 
			   &newcomm_ptr->vcr );
	for (i=0; i<size; i++) {
	    /* For rank i in the new communicator, find the corresponding
	       rank in the input communicator */
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[i] );
	}
    }

    /* If it is an intercomm, duplicate the local vcrt references */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	MPID_VCRT_Add_ref( comm_ptr->local_vcrt );
	newcomm_ptr->local_vcrt = comm_ptr->local_vcrt;
	newcomm_ptr->local_vcr  = comm_ptr->local_vcr;
    }

    /* Set the sizes and ranks */
    newcomm_ptr->rank        = comm_ptr->rank;
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	newcomm_ptr->local_size   = comm_ptr->local_size;
	newcomm_ptr->remote_size  = comm_ptr->remote_size;
	newcomm_ptr->is_low_group = comm_ptr->is_low_group;
    }
    else {
	newcomm_ptr->local_size  = size;
	newcomm_ptr->remote_size = size;
    }

    /* Inherit the error handler (if any) */
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
	MPIR_Errhandler_add_ref( comm_ptr->errhandler );
    }

    /* Notify the device of the new communicator */
    MPID_Dev_comm_create_hook(newcomm_ptr);
	    
    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;
    *outcomm_ptr = newcomm_ptr;

 fn_fail:
 fn_exit:

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_COPY);

    return mpi_errno;
}

/* Release a reference to a communicator.  If there are no pending
   references, delete the communicator and recover all storage and 
   context ids */
#undef FUNCNAME 
#define FUNCNAME MPIR_Comm_release
#undef FCNAME
#define FCNAME "MPIR_Comm_release"
int MPIR_Comm_release(MPID_Comm * comm_ptr, int isDisconnect)
{
    int mpi_errno = MPI_SUCCESS;
    int inuse;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_RELEASE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_RELEASE);
    
    MPIR_Comm_release_ref( comm_ptr, &inuse );
    if (!inuse) {
	/* Remove the attributes, executing the attribute delete routine.  
           Do this only if the attribute functions are defined. 
	   This must be done first, because if freeing the attributes
	   returns an error, the communicator is not freed */
	if (MPIR_Process.attr_free && comm_ptr->attributes) {
	    /* Temporarily add a reference to this communicator because 
               the attr_free code requires a valid communicator */ 
	    MPIU_Object_add_ref( comm_ptr ); 
	    mpi_errno = MPIR_Process.attr_free( comm_ptr->handle, 
						comm_ptr->attributes );
	    /* Release the temporary reference added before the call to 
               attr_free */ 
	    MPIU_Object_release_ref( comm_ptr, &inuse); 
	}

	if (mpi_errno == MPI_SUCCESS) {
	    /* If this communicator is our parent, and we're disconnecting
	       from the parent, mark that fact */
	    if (MPIR_Process.comm_parent == comm_ptr)
		MPIR_Process.comm_parent = NULL;

	    /* Notify the device that the communicator is about to be 
	       destroyed */
	    MPID_Dev_comm_destroy_hook(comm_ptr);
	    
	    /* Free the VCRT */
	    mpi_errno = MPID_VCRT_Release(comm_ptr->vcrt, isDisconnect);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
            if (comm_ptr->comm_kind == MPID_INTERCOMM) {
                mpi_errno = MPID_VCRT_Release(
		    comm_ptr->local_vcrt, isDisconnect);
		if (mpi_errno != MPI_SUCCESS) {
		    MPIU_ERR_POP(mpi_errno);
		}
                if (comm_ptr->local_comm) 
                    MPIR_Comm_release(comm_ptr->local_comm, isDisconnect );
            }

	    /* Free the context value */
	    MPIR_Free_contextid( comm_ptr->recvcontext_id );

	    /* Free the local and remote groups, if they exist */
            if (comm_ptr->local_group)
                MPIR_Group_release(comm_ptr->local_group);
            if (comm_ptr->remote_group)
                MPIR_Group_release(comm_ptr->remote_group);

  	    MPIU_Handle_obj_free( &MPID_Comm_mem, comm_ptr );  
	    
	    /* Remove from the list of active communicators if 
	       we are supporting message-queue debugging.  We make this
	       conditional on having debugger support since the
	       operation is not constant-time */
	    MPIR_COMML_FORGET( comm_ptr );
	}
	else {
	    /* If the user attribute free function returns an error,
	       then do not free the communicator */
	    MPIR_Comm_add_ref( comm_ptr );
	}
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_RELEASE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
