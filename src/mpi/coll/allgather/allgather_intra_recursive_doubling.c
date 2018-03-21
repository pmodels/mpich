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
#define FUNCNAME MPIR_Allgather_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_intra_recursive_doubling(const void *sendbuf,
                                            int sendcount,
                                            MPI_Datatype sendtype,
                                            void *recvbuf,
                                            int recvcount,
                                            MPI_Datatype recvtype,
                                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint recvtype_extent;
    int j, i;
    MPI_Aint curr_cnt, last_recv_cnt = 0;
    int dst;
    MPI_Status status;
    int mask, dst_tree_root, my_tree_root,
        send_offset, recv_offset, nprocs_completed, k, offset, tmp_mask, tree_root;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

#ifdef HAVE_ERROR_CHECKING
    /* Currently this algorithm can only handle power-of-2 comm_size.
     * Non power-of-2 comm_size is still experimental */
    MPIR_Assert(!(comm_size & (comm_size - 1)));
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     (comm_size * recvcount * recvtype_extent));


    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   ((char *) recvbuf +
                                    rank * recvcount * recvtype_extent), recvcount, recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    curr_cnt = recvcount;

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

        /* FIXME: saving an MPI_Aint into an int */
        send_offset = my_tree_root * recvcount * recvtype_extent;
        recv_offset = dst_tree_root * recvcount * recvtype_extent;

        if (dst < comm_size) {
            mpi_errno = MPIC_Sendrecv(((char *) recvbuf + send_offset),
                                      curr_cnt, recvtype, dst,
                                      MPIR_ALLGATHER_TAG,
                                      ((char *) recvbuf + recv_offset),
                                      (comm_size - dst_tree_root) * recvcount,
                                      recvtype, dst,
                                      MPIR_ALLGATHER_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                last_recv_cnt = 0;
            } else {
                MPIR_Get_count_impl(&status, recvtype, &last_recv_cnt);
            }
            curr_cnt += last_recv_cnt;
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

            /* FIXME: saving an MPI_Aint into an int */
            offset = recvcount * (my_tree_root + mask) * recvtype_extent;
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
                    mpi_errno = MPIC_Send(((char *) recvbuf + offset),
                                          last_recv_cnt,
                                          recvtype, dst, MPIR_ALLGATHER_TAG, comm_ptr, errflag);
                    /* last_recv_cnt was set in the previous
                     * receive. that's the amount of data to be
                     * sent now. */
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
                    mpi_errno = MPIC_Recv(((char *) recvbuf + offset),
                                          (comm_size - (my_tree_root + mask)) * recvcount,
                                          recvtype, dst,
                                          MPIR_ALLGATHER_TAG, comm_ptr, &status, errflag);
                    /* nprocs_completed is also equal to the
                     * no. of processes whose data we don't have */
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        last_recv_cnt = 0;
                    } else {
                        MPIR_Get_count_impl(&status, recvtype, &last_recv_cnt);
                    }
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

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
