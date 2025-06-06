/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* A recursive doubling algorithm for MPI_Ireduce_scatter_block, suitable for
 * noncommutative and (non-pof2 or block irregular). */
int MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                              MPI_Aint recvcount,
                                                              MPI_Datatype datatype, MPI_Op op,
                                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    void *tmp_recvbuf, *tmp_results;
    int dst;
    int mask, dst_tree_root, my_tree_root, j, k;
    int received;
    MPI_Datatype sendtype, recvtype;
    int nprocs_completed, tmp_mask, tree_root, is_commutative;

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    MPI_Aint *disps;
    disps = MPIR_Sched_alloc_state(s, comm_size * sizeof(MPI_Aint));
    MPIR_ERR_CHKANDJUMP(!disps, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPI_Aint total_count;
    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcount;
    }

    /* need to allocate temporary buffer to receive incoming data */
    tmp_recvbuf = MPIR_Sched_alloc_state(s, total_count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!tmp_recvbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
     * results */
    tmp_results = MPIR_Sched_alloc_state(s, total_count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!tmp_recvbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *) ((char *) tmp_results - true_lb);

    /* copy sendbuf into tmp_results */
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Sched_copy(sendbuf, total_count, datatype,
                                    tmp_results, total_count, datatype, s);
    else
        mpi_errno = MPIR_Sched_copy(recvbuf, total_count, datatype,
                                    tmp_results, total_count, datatype, s);

    MPIR_ERR_CHECK(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    mask = 0x1;
    i = 0;
    while (mask < comm_size) {
        dst = rank ^ mask;

        dst_tree_root = dst >> i;
        dst_tree_root <<= i;

        my_tree_root = rank >> i;
        my_tree_root <<= i;

        /* At step 1, processes exchange (n-n/p) amount of
         * data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
         * amount of data, and so forth. We use derived datatypes for this.
         *
         * At each step, a process does not need to send data
         * indexed from my_tree_root to
         * my_tree_root+mask-1. Similarly, a process won't receive
         * data indexed from dst_tree_root to dst_tree_root+mask-1. */

        /* calculate sendtype */
        MPI_Aint blklens[2];
        MPI_Aint dis[2];
        blklens[0] = my_tree_root * recvcount;
        blklens[1] = (comm_size - (my_tree_root + mask)) * recvcount;
        if (blklens[1] < 0)
            blklens[1] = 0;

        dis[0] = 0;
        dis[1] =
            blklens[0] + recvcount * (MPL_MIN((my_tree_root + mask), comm_size) - my_tree_root);

        mpi_errno = MPIR_Type_indexed_large_impl(2, blklens, dis, datatype, &sendtype);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&sendtype);
        MPIR_ERR_CHECK(mpi_errno);

        /* calculate recvtype */
        blklens[0] = recvcount * MPL_MIN(dst_tree_root, comm_size);
        blklens[1] = recvcount * (comm_size - (dst_tree_root + mask));
        if (blklens[1] < 0)
            blklens[1] = 0;

        dis[0] = 0;
        dis[1] =
            blklens[0] + recvcount * (MPL_MIN((dst_tree_root + mask), comm_size) - dst_tree_root);

        mpi_errno = MPIR_Type_indexed_large_impl(2, blklens, dis, datatype, &recvtype);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&recvtype);
        MPIR_ERR_CHECK(mpi_errno);

        received = 0;
        if (dst < comm_size) {
            /* tmp_results contains data to be sent in each step. Data is
             * received in tmp_recvbuf and then accumulated into
             * tmp_results. accumulation is done later below.   */
            mpi_errno = MPIR_Sched_send(tmp_results, 1, sendtype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Sched_recv(tmp_recvbuf, 1, recvtype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            received = 1;
        }

        /* if some processes in this process's subtree in this step
         * did not have any destination process to communicate with
         * because of non-power-of-two, we need to send them the
         * result. We use a logarithmic recursive-halfing algorithm
         * for this. */

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

            tmp_mask = mask >> 1;
            while (tmp_mask) {
                dst = rank ^ tmp_mask;

                tree_root = rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                 * doesn't have data. at any step, multiple processes
                 * can send if they have the data */
                if ((dst > rank) && (rank < tree_root + nprocs_completed)
                    && (dst >= tree_root + nprocs_completed)) {
                    /* send the current result */
                    mpi_errno = MPIR_Sched_send(tmp_recvbuf, 1, recvtype, dst, comm_ptr, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                /* recv only if this proc. doesn't have data and sender
                 * has data */
                else if ((dst < rank) &&
                         (dst < tree_root + nprocs_completed) &&
                         (rank >= tree_root + nprocs_completed)) {
                    mpi_errno = MPIR_Sched_recv(tmp_recvbuf, 1, recvtype, dst, comm_ptr, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                    received = 1;
                }
                tmp_mask >>= 1;
                k--;
            }
        }

        /* N.B. The following comment comes from the FT version of
         * MPI_Reduce_scatter.  It does not currently apply to this code, but
         * will in the future when we update the NBC code to be fault-tolerant
         * in roughly the same fashion. [goodell@ 2011-03-03] */
        /* The following reduction is done here instead of after
         * the MPIC_Sendrecv or MPIC_Recv above. This is
         * because to do it above, in the noncommutative
         * case, we would need an extra temp buffer so as not to
         * overwrite temp_recvbuf, because temp_recvbuf may have
         * to be communicated to other processes in the
         * non-power-of-two case. To avoid that extra allocation,
         * we do the reduce here. */
        if (received) {
            if (is_commutative || (dst_tree_root < my_tree_root)) {
                mpi_errno =
                    MPIR_Sched_reduce(tmp_recvbuf, tmp_results, blklens[0], datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Sched_reduce(((char *) tmp_recvbuf + dis[1] * extent),
                                              ((char *) tmp_results + dis[1] * extent),
                                              blklens[1], datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            } else {
                mpi_errno =
                    MPIR_Sched_reduce(tmp_results, tmp_recvbuf, blklens[0], datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Sched_reduce(((char *) tmp_results + dis[1] * extent),
                                              ((char *) tmp_recvbuf + dis[1] * extent),
                                              blklens[1], datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* copy result back into tmp_results */
                mpi_errno = MPIR_Sched_copy(tmp_recvbuf, 1, recvtype, tmp_results, 1, recvtype, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
        }

        MPIR_Type_free_impl(&sendtype);
        MPIR_Type_free_impl(&recvtype);

        mask <<= 1;
        i++;
    }

    /* now copy final results from tmp_results to recvbuf */
    mpi_errno = MPIR_Sched_copy(((char *) tmp_results + disps[rank] * extent),
                                recvcount, datatype, recvbuf, recvcount, datatype, s);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_SCHED_BARRIER(s);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
