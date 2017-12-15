/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ibarrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibarrier = PMPI_Ibarrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibarrier  MPI_Ibarrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibarrier as PMPI_Ibarrier
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request) __attribute__((weak,alias("PMPI_Ibarrier")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibarrier
#define MPI_Ibarrier PMPI_Ibarrier

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of the barrier operation.  The
   algorithm is:

   Algorithm: MPI_Ibarrier

   We use the dissemination algorithm described in:
   Debra Hensgen, Raphael Finkel, and Udi Manber, "Two Algorithms for
   Barrier Synchronization," International Journal of Parallel
   Programming, 17(1):1-17, 1988.

   It uses ceiling(lgp) steps. In step k, 0 <= k <= (ceiling(lgp)-1),
   process i sends to process (i + 2^k) % p and receives from process
   (i - 2^k + p) % p.

   Possible improvements:

   End Algorithm: MPI_Ibarrier

   This is an intracommunicator barrier only!
*/
/* Provides a generic "flat" barrier that doesn't know anything about hierarchy.
 * It will choose between several different algorithms based on the given
 * parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_intra_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_intra_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibarrier_rec_dbl_sched(comm_ptr, s);

    return mpi_errno;
}

/* It will choose between several different algorithms based on the given
 * parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_inter_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_inter_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier_bcast_sched(comm_ptr, s);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_sched(MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ibarrier_intra_sched(comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ibarrier_inter_sched(comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier(MPIR_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *reqp = NULL;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    if (comm_ptr->local_size != 1 || comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_create(&s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPID_Ibarrier_sched(comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, &reqp);
        if (reqp)
            *request = reqp->handle;
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ibarrier - Notifies the process that it has reached the barrier and returns
               immediately

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

Notes:
MPI_Ibarrier is a nonblocking version of MPI_barrier. By calling MPI_Ibarrier,
a process notifies that it has reached the barrier. The call returns
immediately, independent of whether other processes have called MPI_Ibarrier.
The usual barrier semantics are enforced at the corresponding completion
operation (test or wait), which in the intra-communicator case will complete
only after all other processes in the communicator have called MPI_Ibarrier. In
the intercommunicator case, it will complete when all processes in the remote
group have called MPI_Ibarrier.

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBARRIER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IBARRIER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Ibarrier(comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IBARRIER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ibarrier", "**mpi_ibarrier %C %p", comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
