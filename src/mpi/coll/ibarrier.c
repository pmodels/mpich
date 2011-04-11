/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Ibarrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Ibarrier = PMPIX_Ibarrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Ibarrier  MPIX_Ibarrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Ibarrier as PMPIX_Ibarrier
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Ibarrier
#define MPIX_Ibarrier PMPIX_Ibarrier

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
#define FUNCNAME MPIR_Ibarrier_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibarrier_intra(MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int size, rank, src, dst, mask;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Trivial barriers return immediately */
    if (size == 1) goto fn_exit;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;

        mpi_errno = MPID_Sched_send(NULL, 0, MPI_BYTE, dst, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPID_Sched_recv(NULL, 0, MPI_BYTE, src, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = MPID_Sched_barrier(s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mask <<= 1;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Provides a generic "flat" barrier that doesn't know anything about hierarchy.
 * It will choose between several different algorithms based on the given
 * parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibarrier_inter(MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int size, rank, root;
    MPIR_SCHED_CHKPMEM_DECL(1);
    char *buf = NULL;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

    size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPIR_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* do a barrier on the local intracommunicator */
    MPIU_Assert(comm_ptr->local_comm->coll_fns && comm_ptr->local_comm->coll_fns->Ibarrier);
    mpi_errno = comm_ptr->local_comm->coll_fns->Ibarrier(comm_ptr->local_comm, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPID_SCHED_BARRIER(s);

    /* rank 0 on each group does an intercommunicator broadcast to the
       remote group to indicate that all processes in the local group
       have reached the barrier. We do a 1-byte bcast because a 0-byte
       bcast will just return without doing anything. */

    MPIR_SCHED_CHKPMEM_MALLOC(buf, char *, 1, mpi_errno, "bcast buf");
    buf[0] = 'D'; /* avoid valgrind warnings */

    /* first broadcast from left to right group, then from right to
       left group */
    MPIU_Assert(comm_ptr->coll_fns && comm_ptr->coll_fns->Ibcast);
    if (comm_ptr->is_low_group) {
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = comm_ptr->coll_fns->Ibcast(buf, 1, MPI_BYTE, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPID_SCHED_BARRIER(s);

        /* receive bcast from right */
        root = 0;
        mpi_errno = comm_ptr->coll_fns->Ibcast(buf, 1, MPI_BYTE, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        /* receive bcast from left */
        root = 0;
        mpi_errno = comm_ptr->coll_fns->Ibcast(buf, 1, MPI_BYTE, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPID_SCHED_BARRIER(s);

        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = comm_ptr->coll_fns->Ibcast(buf, 1, MPI_BYTE, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibarrier_impl(MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPID_Request *reqp = NULL;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Ibarrier != NULL);
    mpi_errno = comm_ptr->coll_fns->Ibarrier(comm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, &reqp);
    if (reqp)
        *request = reqp->handle;
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_Ibarrier
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_Ibarrier - XXX description here

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_IBARRIER);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_IBARRIER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibarrier_impl(comm_ptr, request);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_IBARRIER);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_ibarrier", "**mpix_ibarrier %C %p", comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
