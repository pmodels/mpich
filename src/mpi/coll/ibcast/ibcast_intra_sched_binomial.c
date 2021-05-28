/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"

/* A binomial tree broadcast algorithm.  Good for short messages,
   Cost = lgp.alpha + n.lgp.beta */
/* Adds operations to the given schedule that correspond to the specified
 * binomial broadcast.  It does _not_ start the schedule.  This permits callers
 * to build up a larger hierarchical broadcast from multiple invocations of this
 * function. */
int MPIR_Ibcast_intra_sched_binomial(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int mask;
    int comm_size, rank;
    int is_contig;
    MPI_Aint nbytes, type_size;
    int relative_rank;
    int src, dst;
    struct MPII_Ibcast_state *ibcast_state;
    void *tmp_buf = NULL;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_is_contig(datatype, &is_contig);
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* we'll allocate tmp_buf along with ibcast_state.
     * Alternatively, we can add init callback to allocate the tmp_buf.
     */
    MPI_Aint tmp_size = 0;
    if (!is_contig) {
        tmp_size = nbytes;
    }

    ibcast_state = MPIR_Sched_alloc_state(s, sizeof(struct MPII_Ibcast_state) + tmp_size);
    MPIR_ERR_CHKANDJUMP(!ibcast_state, mpi_errno, MPI_ERR_OTHER, "**nomem");
    if (!is_contig) {
        tmp_buf = ibcast_state + 1;
    }

    ibcast_state->n_bytes = nbytes;

    if (!is_contig) {
        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Use short message algorithm, namely, binomial tree */

    /* Algorithm:
     * This uses a fairly basic recursive subdivision algorithm.
     * The root sends to the process comm_size/2 away; the receiver becomes
     * a root for a subtree and applies the same process.
     *
     * So that the new root can easily identify the size of its
     * subtree, the (subtree) roots are all powers of two (relative
     * to the root) If m = the first power of 2 such that 2^m >= the
     * size of the communicator, then the subtree at root at 2^(m-k)
     * has size 2^k (with special handling for subtrees that aren't
     * a power of two in size).
     *
     * Do subdivision.  There are two phases:
     * 1. Wait for arrival of data.  Because of the power of two nature
     * of the subtree roots, the source of this message is always the
     * process whose relative rank has the least significant 1 bit CLEARED.
     * That is, process 4 (100) receives from process 0, process 7 (111)
     * from process 6 (110), etc.
     * 2. Forward to my subtree
     *
     * Note that the process that is the tree root is handled automatically
     * by this code, since it has no bits set.  */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;
            if (!is_contig)
                mpi_errno = MPIR_Sched_recv_status(tmp_buf, nbytes, MPI_BYTE, src,
                                                   comm_ptr, &ibcast_state->status, s);
            else
                mpi_errno = MPIR_Sched_recv_status(buffer, count, datatype, src,
                                                   comm_ptr, &ibcast_state->status, s);
            MPIR_ERR_CHECK(mpi_errno);

            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_test_length, ibcast_state, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB up to (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down one.
     *
     * We can easily change to a different algorithm at any power of two
     * by changing the test (mask > 1) to (mask > block_size)
     *
     * One such version would use non-blocking operations for the last 2-4
     * steps (this also bounds the number of MPI_Requests that would
     * be needed).  */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size)
                dst -= comm_size;
            if (!is_contig)
                mpi_errno = MPIR_Sched_send(tmp_buf, nbytes, MPI_BYTE, dst, comm_ptr, s);
            else
                mpi_errno = MPIR_Sched_send(buffer, count, datatype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);

            /* NOTE: This is departure from MPIR_Bcast_intra_binomial.  A true analog
             * would put an MPIR_Sched_barrier here after every send. */
        }
        mask >>= 1;
    }

    if (!is_contig) {
        if (rank != root) {
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
