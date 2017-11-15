/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_util.h"
#include "ibcast.h"

/* A binomial tree broadcast algorithm.  Good for short messages,
   Cost = lgp.alpha + n.lgp.beta */
/* Adds operations to the given schedule that correspond to the specified
 * binomial broadcast.  It does _not_ start the schedule.  This permits callers
 * to build up a larger hierarchical broadcast from multiple invocations of this
 * function. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_binomial_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_binomial_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int mask;
    int comm_size, rank;
    int is_contig, is_homogeneous;
    MPI_Aint nbytes, type_size;
    int relative_rank;
    int src, dst;
    struct MPII_Ibcast_status *status;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (comm_size == 1) {
        /* nothing to add, this is a useless broadcast */
        goto fn_exit;
    }

    MPIR_Datatype_is_contig(datatype, &is_contig);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_SCHED_CHKPMEM_MALLOC(status, struct MPII_Ibcast_status *,
                              sizeof(struct MPII_Ibcast_status), mpi_errno, "MPI_Stauts", MPL_MEM_BUFFER);


    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPIR_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

    nbytes = type_size * count;

    status->n_bytes = nbytes;

    if (!is_contig || !is_homogeneous)
    {
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_PACKED, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Use short message algorithm, namely, binomial tree */

    /* Algorithm:
       This uses a fairly basic recursive subdivision algorithm.
       The root sends to the process comm_size/2 away; the receiver becomes
       a root for a subtree and applies the same process. 

       So that the new root can easily identify the size of its
       subtree, the (subtree) roots are all powers of two (relative
       to the root) If m = the first power of 2 such that 2^m >= the
       size of the communicator, then the subtree at root at 2^(m-k)
       has size 2^k (with special handling for subtrees that aren't
       a power of two in size).

       Do subdivision.  There are two phases:
       1. Wait for arrival of data.  Because of the power of two nature
       of the subtree roots, the source of this message is always the
       process whose relative rank has the least significant 1 bit CLEARED.
       That is, process 4 (100) receives from process 0, process 7 (111)
       from process 6 (110), etc.
       2. Forward to my subtree

       Note that the process that is the tree root is handled automatically
       by this code, since it has no bits set.  */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask; 
            if (src < 0) src += comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPIR_Sched_recv_status(tmp_buf, nbytes, MPI_BYTE, src,
                                                    comm_ptr, &status->status, s);
            else
                mpi_errno = MPIR_Sched_recv_status(buffer, count, datatype, src,
                                                   comm_ptr, &status->status, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            MPIR_SCHED_BARRIER(s);
            if(is_homogeneous){
                mpi_errno = MPIR_Sched_cb(&MPII_sched_test_length, status, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
       set from the LSB upto (but not including) mask.  Because of
       the "not including", we start by shifting mask back down one.

       We can easily change to a different algorithm at any power of two
       by changing the test (mask > 1) to (mask > block_size) 

       One such version would use non-blocking operations for the last 2-4
       steps (this also bounds the number of MPI_Requests that would
       be needed).  */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size) dst -= comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPIR_Sched_send(tmp_buf, nbytes, MPI_BYTE, dst, comm_ptr, s);
            else
                mpi_errno = MPIR_Sched_send(buffer, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* NOTE: This is departure from MPIR_Bcast_binomial.  A true analog
             * would put an MPIR_Sched_barrier here after every send. */
        }
        mask >>= 1;
    }

    if (!is_contig || !is_homogeneous) {
        if (rank != root) {
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_PACKED, buffer, count, datatype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
