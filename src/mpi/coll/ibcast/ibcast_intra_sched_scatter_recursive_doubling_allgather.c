/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"

/*
 * Broadcast based on a scatter followed by an allgather.

 * We first scatter the buffer using a binomial tree algorithm. This
 * costs lgp.alpha + n.((p-1)/p).beta
 *
 * If the datatype is contiguous, we treat the data as bytes and
 * divide (scatter) it among processes by using ceiling division.
 * For the noncontiguous, we first pack the data into a temporary
 * buffer by using MPI_Pack, scatter it as bytes, and unpack it
 * after the allgather.
 *
 * For the allgather, we use a recursive doubling algorithm for
 * medium-size messages and power-of-two number of processes. This
 * takes lgp steps. In each step pairs of processes exchange all the
 * data they have (we take care of non-power-of-two situations). This
 * costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
 * because it may be slightly more in the non-power-of-two case, but
 * it's still a logarithmic algorithm.) Therefore, for long messages
 *
 * Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta
 */
/* It would be nice to just call:
 * ----8<----
 * MPIR_Iscatter_intra_sched_auto(...);
 * MPIR_Iallgather_intra_sched_auto(...);
 * ----8<----
 *
 * But that results in inefficient additional memory allocation and copies
 * because the scatter doesn't know that we have the whole bcast buffer to use
 * as a temp buffer for forwarding.  Furthermore, there's no guarantee that
 * nbytes is a multiple of comm_size, and the regular scatter/allgather ops
 * can't cope with that case correctly.  We could use scatterv/allgatherv
 * instead, but that's not scalable in comm_size and still has the temporary
 * buffer problem.
 *
 * So we use a special-cased scatter (MPIR_Iscatter_for_bcast) that just handles
 * bytes and knows how to deal with a "ragged edge" vector length and we
 * implement the recursive doubling algorithm here.
 */
int MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(void *buffer, MPI_Aint count,
                                                                 MPI_Datatype datatype, int root,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, dst;
    int relative_rank, mask;
    int j, k, i, tmp_mask, is_contig;
    int relative_dst, dst_tree_root, my_tree_root;
    int tree_root, nprocs_completed;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf;
    struct MPII_Ibcast_state *ibcast_state;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

#ifdef HAVE_ERROR_CHECKING
    /* This algorithm can currently handle only power of 2 cases,
     * non-power of 2 is still experimental */
    MPIR_Assert(MPL_is_pof2(comm_size));
#endif /* HAVE_ERROR_CHECKING */

    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPI_Aint type_size, nbytes;
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
    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = MPIR_get_contig_ptr(buffer, true_lb);
    } else {
        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }


    mpi_errno = MPII_Iscatter_for_bcast_sched(tmp_buf, root, comm_ptr, nbytes, s);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint scatter_size, curr_size, incoming_count;
    /* this is the block size used for the scatter operation */
    scatter_size = (nbytes + comm_size - 1) / comm_size;        /* ceiling division */

    /* curr_size is the amount of data that this process now has stored in
     * buffer at byte offset (relative_rank*scatter_size) */
    curr_size = MPL_MIN(scatter_size, (nbytes - (relative_rank * scatter_size)));
    if (curr_size < 0)
        curr_size = 0;
    /* curr_size bytes already inplace */
    ibcast_state->initial_bytes = curr_size;
    mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_init_length, ibcast_state, s);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize because the compiler can't tell that it is always initialized when used */
    incoming_count = -1;

    mask = 0x1;
    i = 0;
    while (mask < comm_size) {
        relative_dst = relative_rank ^ mask;

        dst = (relative_dst + root) % comm_size;

        /* find offset into send and recv buffers.
         * zero out the least significant "i" bits of relative_rank and
         * relative_dst to find root of src and dst
         * subtrees. Use ranks of roots as index to send from
         * and recv into  buffer */

        dst_tree_root = relative_dst >> i;
        dst_tree_root <<= i;

        my_tree_root = relative_rank >> i;
        my_tree_root <<= i;

        MPI_Aint send_offset, recv_offset;
        send_offset = my_tree_root * scatter_size;
        recv_offset = dst_tree_root * scatter_size;

        if (relative_dst < comm_size) {
            /* calculate the exact amount of data to be received */
            /* alternative */
            if ((nbytes - recv_offset) > 0)
                incoming_count = MPL_MIN((nbytes - recv_offset), (mask * scatter_size));
            else
                incoming_count = 0;

            mpi_errno = MPIR_Sched_send(((char *) tmp_buf + send_offset),
                                        curr_size, MPI_BYTE, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            /* sendrecv, no barrier */
            mpi_errno = MPIR_Sched_recv_status(((char *) tmp_buf + recv_offset),
                                               incoming_count,
                                               MPI_BYTE, dst, comm_ptr, &ibcast_state->status, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_add_length, ibcast_state, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            curr_size += incoming_count;
        }

        /* if some processes in this process's subtree in this step
         * did not have any destination process to communicate with
         * because of non-power-of-two, we need to send them the
         * data that they would normally have received from those
         * processes. That is, the haves in this subtree must send to
         * the havenots. We use a logarithmic recursive-halfing algorithm
         * for this. */

        /* This part of the code will not currently be
         * executed because we are not using recursive
         * doubling for non power of two. Mark it as experimental
         * so that it doesn't show up as red in the coverage tests. */

        /* --BEGIN EXPERIMENTAL-- */
        if (dst_tree_root + mask > comm_size) {
            nprocs_completed = comm_size - my_tree_root - mask;
            /* nprocs_completed is the number of processes in this
             * subtree that have all the data. Send data to others
             * in a tree fashion. First find root of current tree
             * that is being divided into two. k is the number of
             * least-significant bits in this process's rank that
             * must be zeroed out to find the rank of the root */
            j = mask;
            k = 0;
            while (j) {
                j >>= 1;
                k++;
            }
            k--;

            MPI_Aint offset;
            offset = scatter_size * (my_tree_root + mask);
            tmp_mask = mask >> 1;

            while (tmp_mask) {
                relative_dst = relative_rank ^ tmp_mask;
                dst = (relative_dst + root) % comm_size;

                tree_root = relative_rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                 * doesn't have data. */

                if ((relative_dst > relative_rank) &&
                    (relative_rank < tree_root + nprocs_completed) &&
                    (relative_dst >= tree_root + nprocs_completed)) {
                    /* incoming_count was set in the previous
                     * receive. that's the amount of data to be
                     * sent now. */
                    mpi_errno = MPIR_Sched_send(((char *) tmp_buf + offset),
                                                incoming_count, MPI_BYTE, dst, comm_ptr, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                /* recv only if this proc. doesn't have data and sender
                 * has data */
                else if ((relative_dst < relative_rank) &&
                         (relative_dst < tree_root + nprocs_completed) &&
                         (relative_rank >= tree_root + nprocs_completed)) {
                    /* recalculate incoming_count, since not all processes will have
                     * this value */
                    if ((nbytes - offset) > 0)
                        incoming_count = MPL_MIN((nbytes - offset), (mask * scatter_size));
                    else
                        incoming_count = 0;

                    /* nprocs_completed is also equal to the no. of processes
                     * whose data we don't have */
                    mpi_errno = MPIR_Sched_recv_status(((char *) tmp_buf + offset),
                                                       incoming_count, MPI_BYTE, dst, comm_ptr,
                                                       &ibcast_state->status, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                    mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_add_length, ibcast_state, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    curr_size += incoming_count;
                }
                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }
    mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_test_curr_length, ibcast_state, s);
    MPIR_ERR_CHECK(mpi_errno);
    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
