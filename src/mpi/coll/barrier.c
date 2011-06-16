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

PMPI_LOCAL int MPIR_Barrier_or_coll_fn(MPID_Comm *comm_ptr, int *errflag );

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
#define FUNCNAME MPIR_Barrier_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Barrier_intra( MPID_Comm *comm_ptr, int *errflag )
{
    int size, rank, src, dst, mask, mpi_errno=MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Comm comm;

    /* Only one collective operation per communicator can be active at any
       time */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    size = comm_ptr->local_size;
    /* Trivial barriers return immediately */
    if (size == 1) goto fn_exit;

    rank = comm_ptr->rank;
    comm = comm_ptr->handle;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        mpi_errno = MPIC_Sendrecv_ft(NULL, 0, MPI_BYTE, dst,
                                     MPIR_BARRIER_TAG, NULL, 0, MPI_BYTE,
                                     src, MPIR_BARRIER_TAG, comm,
                                     MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = TRUE;
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
            MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        mask <<= 1;
    }

 fn_exit:
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* A simple utility function to that calls the comm_ptr->coll_fns->Barrier
override if it exists or else it calls MPIR_Barrier_intra with the same arguments. */
/* Note that this function must *not* be inline - if weak symbols are not 
   available, this function must be a global symbol. */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier_or_coll_fn
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
PMPI_LOCAL int MPIR_Barrier_or_coll_fn(MPID_Comm *comm_ptr, int *errflag )
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Barrier != NULL)
    {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->node_roots_comm->coll_fns->Barrier(comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        /* --END USEREXTENSION-- */
    }
    else {
        mpi_errno = MPIR_Barrier_intra(comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Barrier_inter( MPID_Comm *comm_ptr, int *errflag )
{
    int rank, mpi_errno = MPI_SUCCESS, root;
    int mpi_errno_ret = MPI_SUCCESS;
    int i = 0;
    MPID_Comm *newcomm_ptr = NULL;
    
    rank = comm_ptr->rank;

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
	mpi_errno = MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    /* do a barrier on the local intracommunicator */
    mpi_errno = MPIR_Barrier_intra(newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = TRUE;
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
        MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* rank 0 on each group does an intercommunicator broadcast to the
       remote group to indicate that all processes in the local group
       have reached the barrier. We do a 1-byte bcast because a 0-byte
       bcast will just return without doing anything. */
    
    /* first broadcast from left to right group, then from right to
       left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = TRUE;
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
            MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* receive bcast from right */
        root = 0;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = TRUE;
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
            MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else {
        /* receive bcast from left */
        root = 0;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = TRUE;
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
            MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast_inter(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = TRUE;
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
            MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
 fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPIR_Barrier performs an barrier using poin        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
t-to-point messages.
   This is intended to be used by device-specific implementations of
   barrier.  In all other cases MPIR_Barrier_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Barrier(MPID_Comm *comm_ptr, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Barrier_intra( comm_ptr, errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Barrier_inter( comm_ptr, errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* MPIR_Barrier_impl should be called by any internal component that
   would otherwise call MPI_Barrier.  This differs from MPIR_Barrier
   in that this is SMP aware and will call the coll_fns version if it
   exists.  This is a replacement for NMPI_Barrier. */
#undef FUNCNAME
#define FUNCNAME MPIR_Barrier_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Barrier_impl(MPID_Comm *comm_ptr, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Barrier != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Barrier(comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
#if defined(USE_SMP_COLLECTIVES)
            if (MPIR_Comm_is_node_aware(comm_ptr)) {

                /* do the intranode barrier on all nodes */
                if (comm_ptr->node_comm != NULL)
                {
                    mpi_errno = MPIR_Barrier_or_coll_fn(comm_ptr->node_comm, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = TRUE;
                        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                        MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }

                /* do the barrier across roots of all nodes */
                if (comm_ptr->node_roots_comm != NULL) {
                    mpi_errno = MPIR_Barrier_or_coll_fn(comm_ptr->node_roots_comm, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = TRUE;
                        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                        MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }

                /* release the local processes on each node with a 1-byte broadcast
                   (0-byte broadcast just returns without doing anything) */
                if (comm_ptr->node_comm != NULL)
                {
		    int i=0;
                    mpi_errno = MPIR_Bcast_impl(&i, 1, MPI_BYTE, 0, comm_ptr->node_comm, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = TRUE;
                        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                        MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
            else {
                mpi_errno = MPIR_Barrier_intra( comm_ptr, errflag );
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
#else
            mpi_errno = MPIR_Barrier_intra( comm_ptr, errflag );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
#endif
        }
        else {
            /* intercommunicator */ 
            mpi_errno = MPIR_Barrier_inter( comm_ptr, errflag );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	}
    }
        
 fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
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
    int errflag = FALSE;
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

    mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;
    
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
