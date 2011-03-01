/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Ireduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Ireduce = PMPIX_Ireduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Ireduce  MPIX_Ireduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Ireduce as PMPIX_Ireduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Ireduce
#define MPIX_Ireduce PMPIX_Ireduce

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_binomial
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ireduce_binomial(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, is_commutative;
    int mask, relrank, source, lroot;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    MPIR_SCHED_CHKPMEM_DECL(2);
    MPIU_THREADPRIV_DECL;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    if (count == 0) return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    MPIU_THREADPRIV_GET;
    MPIU_THREADPRIV_FIELD(op_errno) = 0;

    /* Create a temporary buffer */

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(datatype, extent);

    is_commutative = MPIR_Op_is_commutative(op);

    /* I think this is the worse case, so we can avoid an assert()
     * inside the for loop */
    /* should be buf+{this}? */
    MPID_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)),
                        mpi_errno, "temporary buffer");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
        MPIR_SCHED_CHKPMEM_MALLOC(recvbuf, void *,
                            count*(MPIR_MAX(extent,true_extent)),
                            mpi_errno, "receive buffer");
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        /* could do this up front as an MPIR_Localcopy instead, but we'll defer
         * it to the progress engine */
        mpi_errno = MPID_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPID_Sched_barrier(s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* This code is from MPICH-1. */

    /* Here's the algorithm.  Relative to the root, look at the bit pattern in
       my rank.  Starting from the right (lsb), if the bit is 1, send to
       the node with that bit zero and exit; if the bit is 0, receive from the
       node with that bit set and combine (as long as that node is within the
       group)

       Note that by receiving with source selection, we guarentee that we get
       the same bits with the same input.  If we allowed the parent to receive
       the children in any order, then timing differences could cause different
       results (roundoff error, over/underflows in some cases, etc).

       Because of the way these are ordered, if root is 0, then this is correct
       for both commutative and non-commutitive operations.  If root is not
       0, then for non-commutitive, we use a root of zero and then send
       the result to the root.  To see this, note that the ordering is
       mask = 1: (ab)(cd)(ef)(gh)            (odds send to evens)
       mask = 2: ((ab)(cd))((ef)(gh))        (3,6 send to 0,4)
       mask = 4: (((ab)(cd))((ef)(gh)))      (4 sends to 0)

       Comments on buffering.
       If the datatype is not contiguous, we still need to pass contiguous
       data to the user routine.
       In this case, we should make a copy of the data in some format,
       and send/operate on that.

       In general, we can't use MPI_PACK, because the alignment of that
       is rather vague, and the data may not be re-usable.  What we actually
       need is a "squeeze" operation that removes the skips.
    */
    mask = 0x1;
    if (is_commutative)
        lroot = root;
    else
        lroot = 0;
    relrank = (rank - lroot + comm_size) % comm_size;

    while (mask < comm_size) {
        /* Receive */
        if ((mask & relrank) == 0) {
            source = (relrank | mask);
            if (source < comm_size) {
                source = (source + lroot) % comm_size;
                mpi_errno = MPID_Sched_recv(tmp_buf, count, datatype, source, comm_ptr, s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                mpi_errno = MPID_Sched_barrier(s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                /* The sender is above us, so the received buffer must be
                   the second argument (in the noncommutative case). */
                if (is_commutative) {
                    mpi_errno = MPID_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
                else {
                    mpi_errno = MPID_Sched_reduce(recvbuf, tmp_buf, count, datatype, op, s);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    mpi_errno = MPID_Sched_barrier(s);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    mpi_errno = MPID_Sched_copy(tmp_buf, count, datatype, recvbuf, count, datatype, s);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
                mpi_errno = MPID_Sched_barrier(s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        }
        else {
            /* I've received all that I'm going to.  Send my result to
               my parent */
            source = ((relrank & (~ mask)) + lroot) % comm_size;
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype, source, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            break;
        }
        mask <<= 1;
    }

    if (!is_commutative && (root != 0))
    {
        if (rank == 0)
        {
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else if (rank == root)
        {
            mpi_errno = MPID_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ireduce_intra(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    /* TODO need to implement additional algorithms */
    mpi_errno = MPIR_Ireduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ireduce_inter(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

/*  Intercommunicator reduce.
    Remote group does a local intracommunicator
    reduce to rank 0. Rank 0 then sends data to root.
*/

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPID_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPID_Sched_barrier(s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        /* remote group. Rank 0 allocates temporary buffer, does
           local intracommunicator reduce, and then sends the data
           to root. */
        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            MPID_Datatype_get_extent_macro(datatype, extent);
            /* I think this is the worse case, so we can avoid an assert()
             * inside the for loop */
            /* Should MPIR_SCHED_CHKPMEM_MALLOC do this? */
            MPID_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);
        }

        if (!comm_ptr->local_comm) {
            mpi_errno = MPIR_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIR_Ireduce_intra(sendbuf, tmp_buf, count, datatype, op, 0, comm_ptr->local_comm, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPID_Sched_barrier(s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (rank == 0) {
            mpi_errno = MPID_Sched_send(tmp_buf, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ireduce_impl(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPID_Comm *comm_ptr, MPI_Request *request)
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
    MPIU_Assert(comm_ptr->coll_fns->Ireduce != NULL);
    mpi_errno = comm_ptr->coll_fns->Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
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
#define FUNCNAME MPIX_Ireduce
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_Ireduce - XXX description here

Input Parameters:
+ sendbuf - address of the send buffer (choice)
. count - number of elements in send buffer (non-negative integer)
. datatype - data type of elements of send buffer (handle)
. op - reduce operation (handle)
. root - rank of root process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Ireduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_IREDUCE);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_IREDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
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
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op *op_ptr = NULL;
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr(op_ptr, mpi_errno);
            }
            else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = ( * MPIR_Op_check_dtype_table[op%16 - 1] )(datatype);
            }

            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, request);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_IREDUCE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_ireduce", "**mpix_ireduce %p %p %d %D %O %d %C %p", sendbuf, recvbuf, count, datatype, op, root, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
