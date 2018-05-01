/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* Algorithm: Recursive Doubling
 *
 * Restrictions: Suitible for noncommutative and non-pof2 or block irregular.
 *
 * We use a recursive doubling algorithm, which
 * takes lgp steps. At step 1, processes exchange (n-n/p) amount of
 * data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
 * amount of data, and so forth.
 *
 * Cost = lgp.alpha + n.(lgp-(p-1)/p).beta + n.(lgp-(p-1)/p).gamma
 */
int MPIR_Reduce_scatter_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                 const int recvcounts[], MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag)
{
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int *disps;
    void *tmp_recvbuf, *tmp_results;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int dis[2], blklens[2], total_count, dst;
    int mask, dst_tree_root, my_tree_root, j, k;
    int pof2, received;
    MPI_Datatype sendtype, recvtype;
    int nprocs_completed, tmp_mask, tree_root, is_commutative;
    MPIR_CHKLMEM_DECL(5);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_CHKLMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps", MPL_MEM_BUFFER);

    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* total_count*extent eventually gets malloced. it isn't added to
     * a user-passed in buffer */
    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(true_extent, extent));

    /* slightly retask pof2 to mean pof2 equal or greater, not always greater as it is above */
    pof2 = 1;
    while (pof2 < comm_size)
        pof2 <<= 1;

    /* need to allocate temporary buffer to receive incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, total_count * (MPL_MAX(true_extent, extent)),
                        mpi_errno, "tmp_recvbuf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
     * results */
    MPIR_CHKLMEM_MALLOC(tmp_results, void *, total_count * (MPL_MAX(true_extent, extent)),
                        mpi_errno, "tmp_results", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *) ((char *) tmp_results - true_lb);

    /* copy sendbuf into tmp_results */
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, total_count, datatype,
                                   tmp_results, total_count, datatype);
    else
        mpi_errno = MPIR_Localcopy(recvbuf, total_count, datatype,
                                   tmp_results, total_count, datatype);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
        blklens[0] = blklens[1] = 0;
        for (j = 0; j < my_tree_root; j++)
            blklens[0] += recvcounts[j];
        for (j = my_tree_root + mask; j < comm_size; j++)
            blklens[1] += recvcounts[j];

        dis[0] = 0;
        dis[1] = blklens[0];
        for (j = my_tree_root; (j < my_tree_root + mask) && (j < comm_size); j++)
            dis[1] += recvcounts[j];

        mpi_errno = MPIR_Type_indexed_impl(2, blklens, dis, datatype, &sendtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&sendtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* calculate recvtype */
        blklens[0] = blklens[1] = 0;
        for (j = 0; j < dst_tree_root && j < comm_size; j++)
            blklens[0] += recvcounts[j];
        for (j = dst_tree_root + mask; j < comm_size; j++)
            blklens[1] += recvcounts[j];

        dis[0] = 0;
        dis[1] = blklens[0];
        for (j = dst_tree_root; (j < dst_tree_root + mask) && (j < comm_size); j++)
            dis[1] += recvcounts[j];

        mpi_errno = MPIR_Type_indexed_impl(2, blklens, dis, datatype, &recvtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&recvtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        received = 0;
        if (dst < comm_size) {
            /* tmp_results contains data to be sent in each step. Data is
             * received in tmp_recvbuf and then accumulated into
             * tmp_results. accumulation is done later below.   */

            mpi_errno = MPIC_Sendrecv(tmp_results, 1, sendtype, dst,
                                      MPIR_REDUCE_SCATTER_TAG,
                                      tmp_recvbuf, 1, recvtype, dst,
                                      MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);
            received = 1;
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
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
                    mpi_errno = MPIC_Send(tmp_recvbuf, 1, recvtype,
                                          dst, MPIR_REDUCE_SCATTER_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                /* recv only if this proc. doesn't have data and sender
                 * has data */
                else if ((dst < rank) &&
                         (dst < tree_root + nprocs_completed) &&
                         (rank >= tree_root + nprocs_completed)) {
                    mpi_errno = MPIC_Recv(tmp_recvbuf, 1, recvtype, dst,
                                          MPIR_REDUCE_SCATTER_TAG,
                                          comm_ptr, MPI_STATUS_IGNORE, errflag);
                    received = 1;
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                tmp_mask >>= 1;
                k--;
            }
        }

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
                {
                    mpi_errno = MPIR_Reduce_local(tmp_recvbuf, tmp_results, blklens[0],
                                                  datatype, op);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);

                    mpi_errno = MPIR_Reduce_local(((char *) tmp_recvbuf + dis[1] * extent),
                                                  ((char *) tmp_results + dis[1] * extent),
                                                  blklens[1], datatype, op);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
            } else {
                {
                    mpi_errno = MPIR_Reduce_local(tmp_results, tmp_recvbuf, blklens[0],
                                                  datatype, op);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);

                    mpi_errno = MPIR_Reduce_local(((char *) tmp_results + dis[1] * extent),
                                                  ((char *) tmp_recvbuf + dis[1] * extent),
                                                  blklens[1], datatype, op);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
                /* copy result back into tmp_results */
                mpi_errno = MPIR_Localcopy(tmp_recvbuf, 1, recvtype, tmp_results, 1, recvtype);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
        }

        MPIR_Type_free_impl(&sendtype);
        MPIR_Type_free_impl(&recvtype);

        mask <<= 1;
        i++;
    }

    /* now copy final results from tmp_results to recvbuf */
    mpi_errno = MPIR_Localcopy(((char *) tmp_results + disps[rank] * extent),
                               recvcounts[rank], datatype, recvbuf, recvcounts[rank], datatype);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno)
            mpi_errno = per_thread->op_errno;
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
