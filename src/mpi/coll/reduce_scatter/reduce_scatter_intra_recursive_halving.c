/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* Algorithm: Recursive Halving
 *
 * If the operation is commutative, for short and medium-size messages, we use
 * a recursive-halving algorithm in which the first p/2 processes send the
 * second n/2 data to their counterparts in the other half and receive the
 * first n/2 data from them. This procedure continues recursively, halving the
 * data communicated at each step, for a total of lgp steps. If the number of
 * processes is not a power-of-two, we convert it to the nearest lower
 * power-of-two by having the first few even-numbered processes send their data
 * to the neighboring odd-numbered process at (rank+1). Those odd-numbered
 * processes compute the result for their left neighbor as well in the
 * recursive halving algorithm, and then at  the end send the result back to
 * the processes that didn't participate.
 *
 * Therefore, if p is a power-of-two:
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
 *
 * If p is not a power-of-two:
 *
 * Cost = (floor(lgp)+2).alpha + n.(1+(p-1+n)/p).beta + n.(1+(p-1)/p).gamma
 *
 * The above cost in the non power-of-two case is approximate because there is
 * some imbalance in the amount of work each process does because some
 * processes do the work of their neighbors as well.
 *
 * Cost = (p-1).alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
 */
int MPIR_Reduce_scatter_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t errflag)
{
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int *disps;
    void *tmp_recvbuf, *tmp_results;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int total_count, dst;
    int mask;
    int rem, newdst, send_idx, recv_idx, last_idx, send_cnt, recv_cnt;
    int pof2, old_i, newrank;
    MPIR_CHKLMEM_DECL(5);

    MPIR_THREADCOMM_RANK_SIZE(comm_ptr, rank, comm_size);

#ifdef HAVE_ERROR_CHECKING
    {
        int is_commutative;
        is_commutative = MPIR_Op_is_commutative(op);
        MPIR_Assertp(is_commutative);
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_CHKLMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps", MPL_MEM_BUFFER);

    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* commutative and short. use recursive halving algorithm */

    /* allocate temp. buffer to receive incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, total_count * (MPL_MAX(true_extent, extent)),
                        mpi_errno, "tmp_recvbuf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
     * results because recvbuf may not be big enough */
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

    MPIR_ERR_CHECK(mpi_errno);

    pof2 = MPL_pof2(comm_size);

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */
            mpi_errno = MPIC_Send(tmp_results, total_count,
                                  datatype, rank + 1, MPIR_REDUCE_SCATTER_TAG, comm_ptr, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = -1;
        } else {        /* odd */
            mpi_errno = MPIC_Recv(tmp_recvbuf, total_count,
                                  datatype, rank - 1,
                                  MPIR_REDUCE_SCATTER_TAG, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            mpi_errno = MPIR_Reduce_local(tmp_recvbuf, tmp_results, total_count, datatype, op);
            MPIR_ERR_CHECK(mpi_errno);

            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        /* recalculate the recvcounts and disps arrays because the
         * even-numbered processes who no longer participate will
         * have their result calculated by the process to their
         * right (rank+1). */
        MPI_Aint *newcnts, *newdisps;
        MPIR_CHKLMEM_MALLOC(newcnts, MPI_Aint *, pof2 * sizeof(MPI_Aint), mpi_errno, "newcnts",
                            MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(newdisps, MPI_Aint *, pof2 * sizeof(MPI_Aint), mpi_errno, "newdisps",
                            MPL_MEM_BUFFER);

        for (i = 0; i < pof2; i++) {
            /* what does i map to in the old ranking? */
            old_i = (i < rem) ? i * 2 + 1 : i + rem;
            if (old_i < 2 * rem) {
                /* This process has to also do its left neighbor's
                 * work */
                newcnts[i] = recvcounts[old_i] + recvcounts[old_i - 1];
            } else
                newcnts[i] = recvcounts[old_i];
        }

        newdisps[0] = 0;
        for (i = 1; i < pof2; i++)
            newdisps[i] = newdisps[i - 1] + newcnts[i - 1];

        mask = pof2 >> 1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask > 0) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + mask;
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += newcnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += newcnts[i];
            } else {
                recv_idx = send_idx + mask;
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += newcnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += newcnts[i];
            }

/*                    printf("Rank %d, send_idx %d, recv_idx %d, send_cnt %d, recv_cnt %d, last_idx %d\n", newrank, send_idx, recv_idx,
                  send_cnt, recv_cnt, last_idx);
*/
            /* Send data from tmp_results. Recv into tmp_recvbuf */
            if ((send_cnt != 0) && (recv_cnt != 0))
                mpi_errno = MPIC_Sendrecv((char *) tmp_results +
                                          newdisps[send_idx] * extent,
                                          send_cnt, datatype,
                                          dst, MPIR_REDUCE_SCATTER_TAG,
                                          (char *) tmp_recvbuf +
                                          newdisps[recv_idx] * extent,
                                          recv_cnt, datatype, dst,
                                          MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                          MPI_STATUS_IGNORE, errflag);
            else if ((send_cnt == 0) && (recv_cnt != 0))
                mpi_errno = MPIC_Recv((char *) tmp_recvbuf +
                                      newdisps[recv_idx] * extent,
                                      recv_cnt, datatype, dst,
                                      MPIR_REDUCE_SCATTER_TAG, comm_ptr, MPI_STATUS_IGNORE);
            else if ((recv_cnt == 0) && (send_cnt != 0))
                mpi_errno = MPIC_Send((char *) tmp_results +
                                      newdisps[send_idx] * extent,
                                      send_cnt, datatype,
                                      dst, MPIR_REDUCE_SCATTER_TAG, comm_ptr, errflag);

            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

            /* tmp_recvbuf contains data received in this step.
             * tmp_results contains data accumulated so far */

            if (recv_cnt) {
                mpi_errno = MPIR_Reduce_local((char *) tmp_recvbuf + newdisps[recv_idx] * extent,
                                              (char *) tmp_results + newdisps[recv_idx] * extent,
                                              recv_cnt, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);
            }

            /* update send_idx for next iteration */
            send_idx = recv_idx;
            last_idx = recv_idx + mask;
            mask >>= 1;
        }

        /* copy this process's result from tmp_results to recvbuf */
        if (recvcounts[rank]) {
            mpi_errno = MPIR_Localcopy((char *) tmp_results +
                                       disps[rank] * extent,
                                       recvcounts[rank], datatype, recvbuf, recvcounts[rank],
                                       datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }

    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send to (rank-1) the result they
     * calculated for that process */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            if (recvcounts[rank - 1]) {
                mpi_errno = MPIC_Send((char *) tmp_results +
                                      disps[rank - 1] * extent, recvcounts[rank - 1],
                                      datatype, rank - 1,
                                      MPIR_REDUCE_SCATTER_TAG, comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            }
        } else {        /* even */
            if (recvcounts[rank]) {
                mpi_errno = MPIC_Recv(recvbuf, recvcounts[rank],
                                      datatype, rank + 1,
                                      MPIR_REDUCE_SCATTER_TAG, comm_ptr, MPI_STATUS_IGNORE);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            }
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;
}
