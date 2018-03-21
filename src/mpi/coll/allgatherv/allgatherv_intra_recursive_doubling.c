/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * Recursive Doubling Algorithm:
 *
 * Restrictions: power-of-two no. of processes
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 *
 * TODO: On TCP, we may want to use recursive doubling instead of the
 * Bruck's algorithm in all cases because of the pairwise-exchange
 * property of recursive doubling (see Benson et al paper in Euro
 * PVM/MPI 2003).
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Allgatherv_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgatherv_intra_recursive_doubling(const void *sendbuf,
                                             int sendcount,
                                             MPI_Datatype sendtype,
                                             void *recvbuf,
                                             const int *recvcounts,
                                             const int *displs,
                                             MPI_Datatype recvtype,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, rank, j, i;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint recvtype_extent, recvtype_true_extent, recvtype_true_lb;
    MPI_Aint curr_cnt, last_recv_cnt;
    int dst, total_count;
    void *tmp_buf;
    int mask, dst_tree_root, my_tree_root, position,
        send_offset, recv_offset, nprocs_completed, k, offset, tmp_mask, tree_root;
    MPIR_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

#ifdef HAVE_ERROR_CHECKING
    /* Currently this algorithm can only handle power-of-2 comm_size.
     * Non power-of-2 comm_size is still experimental */
    MPIR_Assert(!(comm_size & (comm_size - 1)));
#endif /* HAVE_ERROR_CHECKING */

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);


    /* need to receive contiguously into tmp_buf because
     * displs could make the recvbuf noncontiguous */

    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    MPIR_Ensure_Aint_fits_in_pointer(total_count *
                                     (MPL_MAX(recvtype_true_extent, recvtype_extent)));
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *,
                        total_count * (MPL_MAX(recvtype_true_extent, recvtype_extent)),
                        mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - recvtype_true_lb);

    /* copy local data into right location in tmp_buf */
    position = 0;
    for (i = 0; i < rank; i++)
        position += recvcounts[i];
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   ((char *) tmp_buf + position *
                                    recvtype_extent), recvcounts[rank], recvtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* if in_place specified, local data is found in recvbuf */
        mpi_errno = MPIR_Localcopy(((char *) recvbuf +
                                    displs[rank] * recvtype_extent),
                                   recvcounts[rank], recvtype,
                                   ((char *) tmp_buf + position *
                                    recvtype_extent), recvcounts[rank], recvtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    curr_cnt = recvcounts[rank];

    mask = 0x1;
    i = 0;
    while (mask < comm_size) {
        dst = rank ^ mask;

        /* find offset into send and recv buffers. zero out
         * the least significant "i" bits of rank and dst to
         * find root of src and dst subtrees. Use ranks of
         * roots as index to send from and recv into buffer */

        dst_tree_root = dst >> i;
        dst_tree_root <<= i;

        my_tree_root = rank >> i;
        my_tree_root <<= i;

        if (dst < comm_size) {
            send_offset = 0;
            for (j = 0; j < my_tree_root; j++)
                send_offset += recvcounts[j];

            recv_offset = 0;
            for (j = 0; j < dst_tree_root; j++)
                recv_offset += recvcounts[j];

            mpi_errno = MPIC_Sendrecv(((char *) tmp_buf + send_offset * recvtype_extent),
                                      curr_cnt, recvtype, dst,
                                      MPIR_ALLGATHERV_TAG,
                                      ((char *) tmp_buf + recv_offset * recvtype_extent),
                                      total_count - recv_offset, recvtype, dst,
                                      MPIR_ALLGATHERV_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                last_recv_cnt = 0;
            } else
                /* for convenience, recv is posted for a bigger amount
                 * than will be sent */
                MPIR_Get_count_impl(&status, recvtype, &last_recv_cnt);
            curr_cnt += last_recv_cnt;
        }

        /* if some processes in this process's subtree in this step
         * did not have any destination process to communicate with
         * because of non-power-of-two, we need to send them the
         * data that they would normally have received from those
         * processes. That is, the haves in this subtree must send to
         * the havenots. We use a logarithmic
         * recursive-halfing algorithm for this. */

        /* This part of the code will not currently be
         * executed because we are not using recursive
         * doubling for non power of two. Mark it as experimental
         * so that it doesn't show up as red in the coverage
         * tests. */

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

                    offset = 0;
                    for (j = 0; j < (my_tree_root + mask); j++)
                        offset += recvcounts[j];
                    offset *= recvtype_extent;

                    mpi_errno = MPIC_Send(((char *) tmp_buf + offset),
                                          last_recv_cnt,
                                          recvtype, dst, MPIR_ALLGATHERV_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                    /* last_recv_cnt was set in the previous
                     * receive. that's the amount of data to be
                     * sent now. */
                }
                /* recv only if this proc. doesn't have data and sender
                 * has data */
                else if ((dst < rank) &&
                         (dst < tree_root + nprocs_completed) &&
                         (rank >= tree_root + nprocs_completed)) {

                    offset = 0;
                    for (j = 0; j < (my_tree_root + mask); j++)
                        offset += recvcounts[j];

                    mpi_errno = MPIC_Recv(((char *) tmp_buf + offset * recvtype_extent),
                                          total_count - offset, recvtype,
                                          dst, MPIR_ALLGATHERV_TAG, comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        last_recv_cnt = 0;
                    } else
                        /* for convenience, recv is posted for a
                         * bigger amount than will be sent */
                        MPIR_Get_count_impl(&status, recvtype, &last_recv_cnt);
                    curr_cnt += last_recv_cnt;
                }
                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }

    /* copy data from tmp_buf to recvbuf */
    position = 0;
    for (j = 0; j < comm_size; j++) {
        if ((sendbuf != MPI_IN_PLACE) || (j != rank)) {
            /* not necessary to copy if in_place and
             * j==rank. otherwise copy. */
            mpi_errno = MPIR_Localcopy(((char *) tmp_buf + position * recvtype_extent),
                                       recvcounts[j], recvtype,
                                       ((char *) recvbuf + displs[j] * recvtype_extent),
                                       recvcounts[j], recvtype);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        position += recvcounts[j];
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
