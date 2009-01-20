/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Barrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Barrier = PMPI_Barrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Barrier  MPI_Barrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Barrier as PMPI_Barrier
#endif
/* -- End Profiling Symbol Block */

PMPI_LOCAL inline int MPIR_Barrier_or_coll_fn(MPID_Comm *comm_ptr );

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Barrier
#define MPI_Barrier PMPI_Barrier


/* This is the default implementation of the barrier operation.  The
   algorithm is:
   
   Algorithm: MPI_Barrier

   We use the dissemination algorithm described in:
   Debra Hensgen, Raphael Finkel, and Udi Manbet, "Two Algorithms for
   Barrier Synchronization," International Journal of Parallel
   Programming, 17(1):1-17, 1988.  

   It uses ceiling(lgp) steps. In step k, 0 <= k <= (ceiling(lgp)-1),
   process i sends to process (i + 2^k) % p and receives from process 
   (i - 2^k + p) % p.

   Possible improvements: 

   End Algorithm: MPI_Barrier

   This is an intracommunicator barrier only!
*/

/* not declared static because it is called in ch3_comm_connect/accept */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Barrier( MPID_Comm *comm_ptr )
{
    int size, rank, src, dst, mask, mpi_errno=MPI_SUCCESS;
    MPI_Comm comm;

    size = comm_ptr->local_size;
    /* Trivial barriers return immediately */
    if (size == 1) return MPI_SUCCESS;

    rank = comm_ptr->rank;
    comm = comm_ptr->handle;

    /* Only one collective operation per communicator can be active at any
       time */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        mpi_errno = MPIC_Sendrecv(NULL, 0, MPI_BYTE, dst,
                                  MPIR_BARRIER_TAG, NULL, 0, MPI_BYTE,
                                  src, MPIR_BARRIER_TAG, comm,
                                  MPI_STATUS_IGNORE);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
        mask <<= 1;
    }

    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

    return mpi_errno;
}


#if 0

/* This is the default implementation of the barrier operation.  The
   algorithm is:
   
   Algorithm: MPI_Barrier

   Find the largest power of two that is less than or equal to the size of 
   the communicator.  Call tbis twon_within.

   Divide the communicator by rank into two groups: those with 
   rank < twon_within and those with greater rank.  The barrier
   executes in three steps.  First, the group with rank >= twon_within
   sends to the first (size-twon_within) ranks of the first group.
   That group then executes a recursive doubling algorithm for the barrier.
   For the third step, the first (size-twon_within) ranks send to the top
   group.  This is the same algorithm used in MPICH-1.

   Possible improvements: 
   The upper group could apply recursively this approach to reduce the 
   total number of messages sent (in the case of of a size of 2^n-1, there 
   are 2^(n-1) messages sent in the first and third steps).

   End Algorithm: MPI_Barrier

   This is an intracommunicator barrier only!
*/
int MPIR_Barrier( MPID_Comm *comm_ptr )
{
    int size, rank;
    int twon_within, n2, remaining, gap, partner;
    MPID_Request *request_ptr;
    int mpi_errno = MPI_SUCCESS;
    
    size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Trivial barriers return immediately */
    if (size == 1) return MPI_SUCCESS;

    /* Only one collective operation per communicator can be active at any
       time */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
    
    /* Find the twon_within (this could be cached if more routines
     need it) */
    twon_within = 1;
    n2          = 2;
    while (n2 <= size) { twon_within = n2; n2 <<= 1; }
    remaining = size - twon_within;

    if (rank < twon_within) {
	/* First step: receive from the upper group */
	if (rank < remaining) {
	    MPID_Recv( 0, 0, MPI_BYTE, twon_within + rank, MPIR_BARRIER_TAG, 
		       comm_ptr, MPID_CONTEXT_INTRA_COLL, MPI_STATUS_IGNORE,
		       &request_ptr );
	    if (request_ptr) {
		mpi_errno = MPIC_Wait(request_ptr);
		MPID_Request_release(request_ptr);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	}
	/* Second step: recursive doubling exchange */
	for (gap=1; gap<twon_within; gap <<= 1) {
	    partner = (rank ^ gap);
	    MPIC_Sendrecv( 0, 0, MPI_BYTE, partner, MPIR_BARRIER_TAG,
			   0, 0, MPI_BYTE, partner, MPIR_BARRIER_TAG,
			   comm_ptr->handle, MPI_STATUS_IGNORE );
	}

	/* Third step: send to the upper group */
	if (rank < remaining) {
	    MPID_Send( 0, 0, MPI_BYTE, rank + twon_within, MPIR_BARRIER_TAG,
		       comm_ptr, MPID_CONTEXT_INTRA_COLL, &request_ptr );
	    if (request_ptr) {
		mpi_errno = MPIC_Wait(request_ptr);
		MPID_Request_release(request_ptr);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	}
    }
    else {
	/* For the upper group, step one is a send */
	MPID_Send( 0, 0, MPI_BYTE, rank - twon_within, MPIR_BARRIER_TAG,
		   comm_ptr, MPID_CONTEXT_INTRA_COLL, &request_ptr );
	if (request_ptr) {
	    mpi_errno = MPIC_Wait(request_ptr);
	    MPID_Request_release(request_ptr);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	/* There is no second step; for the third step, recv */
	MPID_Recv( 0, 0, MPI_BYTE, rank - twon_within, MPIR_BARRIER_TAG, 
		   comm_ptr, MPID_CONTEXT_INTRA_COLL, MPI_STATUS_IGNORE,
		   &request_ptr );
	if (request_ptr) {
	    mpi_errno = MPIC_Wait(request_ptr);
	    MPID_Request_release(request_ptr);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
    }

  fn_exit:
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

    return mpi_errno;
}
#endif


/* A simple utility function to that calls the comm_ptr->coll_fns->Barrier
override if it exists or else it calls MPIR_Barrier with the same arguments. */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier_or_coll_fn
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
PMPI_LOCAL inline int MPIR_Barrier_or_coll_fn(MPID_Comm *comm_ptr )
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Barrier != NULL)
    {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->node_roots_comm->coll_fns->Barrier(comm_ptr);
        /* --END USEREXTENSION-- */
    }
    else {
        mpi_errno = MPIR_Barrier(comm_ptr);
    }

    return mpi_errno;
}


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Barrier_inter( MPID_Comm *comm_ptr )
{
    int rank, mpi_errno, i, root;
    MPID_Comm *newcomm_ptr = NULL;

    rank = comm_ptr->rank;

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
	MPIR_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    /* do a barrier on the local intracommunicator */
    mpi_errno = MPIR_Barrier(newcomm_ptr);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* rank 0 on each group does an intercommunicator broadcast to the
       remote group to indicate that all processes in the local group
       have reached the barrier. We do a 1-byte bcast because a 0-byte
       bcast will just return without doing anything. */
    
    /* first broadcast from left to right group, then from right to
       left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr); 
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
        /* receive bcast from right */
        root = 0;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr); 
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
    }
    else {
        /* receive bcast from left */
        root = 0;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr); 
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr);  
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
    }

    return mpi_errno;
}

#endif


#undef FUNCNAME
#define FUNCNAME MPI_Barrier
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)

/*@

MPI_Barrier - Blocks until all processes in the communicator have
reached this routine.  

Input Parameter:
. comm - communicator (handle) 

Notes:
Blocks the caller until all processes in the communicator have called it; 
that is, the call returns at any process only after all members of the
communicator have entered the call.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Barrier( MPI_Comm comm )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BARRIER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_BARRIER);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Barrier != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Barrier(comm_ptr);
    }
    else
    {
	MPIU_THREADPRIV_DECL;
	MPIU_THREADPRIV_GET;
        MPIR_Nest_incr();
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
#if defined(USE_SMP_COLLECTIVES)
            if (MPIR_Comm_is_node_aware(comm_ptr)) {

                /* do the intranode barrier on all nodes */
                if (comm_ptr->node_comm != NULL)
                {
                    mpi_errno = MPIR_Barrier_or_coll_fn(comm_ptr->node_comm);
                    if (mpi_errno) {
                        MPIR_Nest_decr();
                        goto fn_fail;
                    }
                }

                /* do the barrier across roots of all nodes */
                if (comm_ptr->node_roots_comm != NULL) {
                    mpi_errno = MPIR_Barrier_or_coll_fn(comm_ptr->node_roots_comm);
                    if (mpi_errno) {
                        MPIR_Nest_decr();
                        goto fn_fail;
                    }
                }

                /* release the local processes on each node with a 1-byte broadcast
                   (0-byte broadcast just returns without doing anything) */
                if (comm_ptr->node_comm != NULL)
                {  
		    int i=0;
                    mpi_errno = MPIR_Bcast_or_coll_fn(&i, 1, MPI_BYTE, 0, 
						      comm_ptr->node_comm);
                }
            }
            else {
                mpi_errno = MPIR_Barrier( comm_ptr );
            }
#else
            mpi_errno = MPIR_Barrier( comm_ptr );
#endif
        }
        else {
            /* intercommunicator */ 
            mpi_errno = MPIR_Barrier_inter( comm_ptr );
	}
        MPIR_Nest_decr();
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    /* --END ERROR HANDLING-- */

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_barrier", "**mpi_barrier %C", comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
