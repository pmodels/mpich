/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

int
MPIR_TSP_Iallgather_sched_intra_brucks(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       MPI_Aint recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, j;
    int nphases = 0;
    int n_invtcs;
    int tag;
    int src, dst, p_of_k = 0;   /* Largest power of k that is (strictly) smaller than 'size' */
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    int rank = MPIR_Comm_rank(comm);
    int size = MPIR_Comm_size(comm);
    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int max = size - 1;
    int vtx_id;

    MPI_Aint sendtype_extent, sendtype_lb;
    MPI_Aint recvtype_extent, recvtype_lb;
    MPI_Aint sendtype_true_extent, recvtype_true_extent;

    int delta = 1;
    int i_recv = 0;
    int *recv_id = NULL;
    void *tmp_recvbuf = NULL;
    MPIR_CHKLMEM_DECL(1);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_ENTER;

    if (is_inplace) {
        sendcount = recvcount;
        sendtype = recvtype;
    }

    /* get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    while (max) {
        nphases++;
        max /= k;
    }

    /* Check if size is power of k */
    if (MPL_ipow(k, nphases) == size)
        p_of_k = 1;

    MPIR_CHKLMEM_MALLOC(recv_id, int *, sizeof(int) * nphases * (k - 1),
                        mpi_errno, "recv_id buffer", MPL_MEM_COLL);

    if (rank == 0)
        tmp_recvbuf = recvbuf;
    else
        tmp_recvbuf = MPIR_TSP_sched_malloc(size * recvcount * recvtype_extent, sched);

    /* Step1: copy own data from sendbuf to top of recvbuf. */
    if (is_inplace && rank != 0) {
        mpi_errno = MPIR_TSP_sched_localcopy((char *) recvbuf + rank * recvcount * recvtype_extent,
                                             recvcount, recvtype, tmp_recvbuf, recvcount, recvtype,
                                             sched, 0, NULL, &vtx_id);
    } else if (!is_inplace) {
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype, tmp_recvbuf,
                                             recvcount, recvtype, sched, 0, NULL, &vtx_id);
    }

    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    /* All following sends/recvs and copies depend on this dtcopy */
    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    n_invtcs = 0;
    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {
            if (MPL_ipow(k, i) * j >= size)     /* If the first location exceeds comm size, nothing is to be sent. */
                break;

            dst = (int) (size + (rank - delta * j)) % size;
            src = (int) (rank + delta * j) % size;

            /* Amount of data sent in each cycle = k^i, where i = phase_number.
             * if (size != MPL_ipow(k, power_of_k) send less data in the last phase.
             * This might differ for the different values of j in the last phase. */
            MPI_Aint count;
            if ((i == (nphases - 1)) && (!p_of_k)) {
                count = recvcount * delta;
                MPI_Aint left_count = recvcount * (size - delta * j);
                if (j == k - 1)
                    count = left_count;
                else
                    count = MPL_MIN(count, left_count);
            } else {
                count = recvcount * delta;
            }

            /* Receive at the exact location. */
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) tmp_recvbuf + j * recvcount * delta * recvtype_extent,
                                     count, recvtype, src, tag, comm, sched, 0, NULL, &vtx_id);

            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            recv_id[i_recv++] = vtx_id;
            /* Send from the start of recv till `count` amount of data. */
            if (i == 0)
                mpi_errno =
                    MPIR_TSP_sched_isend(tmp_recvbuf, count, recvtype, dst, tag, comm, sched, 0,
                                         NULL, &vtx_id);
            else
                mpi_errno = MPIR_TSP_sched_isend(tmp_recvbuf, count, recvtype, dst, tag,
                                                 comm, sched, n_invtcs, recv_id, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        n_invtcs += (k - 1);
        delta *= k;
    }
    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    if (rank != 0) {    /* No shift required for rank 0 */
        mpi_errno =
            MPIR_TSP_sched_localcopy((char *) tmp_recvbuf +
                                     (size - rank) * recvcount * recvtype_extent, rank * recvcount,
                                     recvtype, (char *) recvbuf, rank * recvcount, recvtype, sched,
                                     0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        mpi_errno =
            MPIR_TSP_sched_localcopy((char *) tmp_recvbuf, (size - rank) * recvcount, recvtype,
                                     (char *) recvbuf + rank * recvcount * recvtype_extent,
                                     (size - rank) * recvcount, recvtype, sched, 0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
