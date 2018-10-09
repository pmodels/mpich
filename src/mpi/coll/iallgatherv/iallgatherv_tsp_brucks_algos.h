/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * (C) 2006 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., IALLGATHERV_TSP_BRUCKS_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

int
MPIR_TSP_Iallgatherv_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                        MPIR_TSP_sched_t * sched, int k)
{
    int i, j, l;
    int dtcopy_id = 0;
    int nphases = 0;
    int n_invtcs = 0;
    int count, src, dst, p_of_k = 0;    /* largest power of k that is (strictly) smaller than 'size' */
    int total_recvcount = 0;
    int delta = 1;

    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int rank = MPIR_Comm_rank(comm);
    int size = MPIR_Comm_size(comm);
    int max = size - 1;

    size_t sendtype_size, sendtype_extent, sendtype_lb;
    size_t recvtype_size, recvtype_extent, recvtype_lb;
    size_t sendtype_true_extent, recvtype_true_extent;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    int tag;
    int *recv_id = NULL;
    int *recv_index = NULL;
    void *tmp_recvbuf = NULL;
    int **r_counts = NULL;
    int **s_counts = NULL;
    int index = 0;
    int index_sum = 0;
    int count_length, top_count, bottom_count, left_count;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_BRUCKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_BRUCKS);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "allgatherv_brucks: num_ranks: %d, k: %d", size, k));

    if (is_inplace) {
        sendcount = recvcounts[rank];
        sendtype = recvtype;
    }

    /* Get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "send_type_size: %d, sendtype_extent: %d, send_count: %d",
                     sendtype_size, sendtype_extent, sendcount));

    while (max) {
        nphases++;
        max /= k;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "comm_size:%d, nphases:%d, recvcounts[rank:%d]:%d", size,
                     nphases, rank, recvcounts[rank]));

    /* Check if size is power of k */
    if (MPL_ipow(k, nphases) == size)
        p_of_k = 1;

    recv_id = (int *) MPL_malloc(sizeof(int) * nphases * (k - 1), MPL_MEM_COLL);        /* if nphases=0 then no recv_id needed */

    for (i = 0; i < size; i++)
        total_recvcount += recvcounts[i];

    if (rank == 0)
        tmp_recvbuf = recvbuf;
    else
        tmp_recvbuf = MPIR_TSP_sched_malloc(total_recvcount * recvtype_extent, sched);

    r_counts = (int **) MPL_malloc(sizeof(int *) * nphases, MPL_MEM_COLL);
    s_counts = (int **) MPL_malloc(sizeof(int *) * nphases, MPL_MEM_COLL);
    for (i = 0; i < nphases; i++) {
        r_counts[i] = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
        s_counts[i] = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
    }

    /* To store the index to receive in various phases and steps within */
    recv_index = (int *) MPL_malloc(sizeof(int) * nphases * (k - 1), MPL_MEM_COLL);

    index_sum = recvcounts[rank];       /* because in initially you copy your own data to the top of recv_buf */
    if (nphases > 0)
        recv_index[index++] = index_sum;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "addresses of all allocated memory:\n recv_index:%04x, recv_id:%04x, r_counts:%04x, s_counts:%04x, tmp_recvbuf:%04x",
                     recv_index, recv_id, r_counts, s_counts, tmp_recvbuf));

    /* The least s_count for any send will be their own sendcount(=recvcounts[rank])
     * We add all the r_counts received from neighbours during the previous phase */
    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {
            r_counts[i][j - 1] = 0;
            s_counts[i][j - 1] = 0;
            src = (int) (rank + delta * j) % size;
            if ((i == nphases - 1) && (!p_of_k)) {
                left_count = size - delta * j;
                count_length = MPL_MIN(delta, left_count);
            } else {
                count_length = delta;
            }
            /* Calculate r_counts and s_counts for the data you'll receive and send in
             * the next steps(phase, [0, 1, ..,(k-1)]) */
            for (l = 0; l < count_length; l++) {
                r_counts[i][j - 1] += recvcounts[(size + src + l) % size];
                s_counts[i][j - 1] += recvcounts[(rank + l) % size];
            }

            /* Helps point to correct recv location in tmp_recvbuf */
            index_sum += r_counts[i][j - 1];
            if (index < nphases * (k - 1))
                recv_index[index++] = index_sum;

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "r_counts[%d][%d]=%d, s_counts[%d][%d]=%d", i, j - 1,
                             r_counts[i][j - 1], i, j - 1, s_counts[i][j - 1]));
        }
        delta *= k;
    }

    /* Step1: copy own data from sendbuf to top of recvbuf */
    if (is_inplace && rank != 0)
        MPIR_TSP_sched_localcopy((char *) recvbuf + displs[rank] * recvtype_extent,
                                 recvcounts[rank], recvtype, tmp_recvbuf,
                                 recvcounts[rank], recvtype, sched, 0, NULL);
    else if (!is_inplace)
        MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype, tmp_recvbuf,
                                 recvcounts[rank], recvtype, sched, 0, NULL);

    MPIR_TSP_sched_fence(sched);

    index = 0;
    delta = 1;
    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {
            /* If the first location exceeds comm size, nothing is to be sent */
            if (MPL_ipow(k, i) * j >= size)
                break;

            dst = (int) (size + (rank - delta * j)) % size;
            src = (int) (rank + delta * j) % size;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Phase#%d/%d:j:%d: src:%d, dst:%d",
                             i, nphases, j, src, dst));
            /* Recv at the exact location */
            recv_id[index] =
                MPIR_TSP_sched_irecv((char *) tmp_recvbuf + recv_index[index] * recvtype_extent,
                                     r_counts[i][j - 1], recvtype, src, tag, comm, sched, 0, NULL);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "Phase#%d:, k:%d with recv_index[index:%d]:%d at Recv at:%04x from src:%d for recv_count:%d",
                             i, k, index, recv_index[index],
                             (tmp_recvbuf + recv_index[index] * recvtype_extent), src,
                             r_counts[i][j - 1]));
            index++;

            /* Send from the start of recv till the count amount of data */
            MPIR_TSP_sched_isend(tmp_recvbuf, s_counts[i][j - 1], recvtype, dst, tag, comm, sched,
                                 n_invtcs, recv_id);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                                     "Phase#%d:, k:%d Send from:%04x to dst:%d for count:%d",
                                                     i, k, tmp_recvbuf, dst, s_counts[i][j - 1]));
        }
        n_invtcs += (k - 1);
        delta *= k;
    }
    MPIR_TSP_sched_fence(sched);

    /* No shift required for rank 0 */
    if (rank != 0) {
        index = 0;
        /* Calculate index(same as count) till 0th rank's data */
        for (i = 0; i < (size - rank); i++)
            index += recvcounts[(rank + i) % size];

        bottom_count = index;
        top_count = total_recvcount - index;

        MPIR_TSP_sched_localcopy((char *) tmp_recvbuf + bottom_count * recvtype_extent,
                                 top_count, recvtype, recvbuf, top_count, recvtype, sched, 0, NULL);
        MPIR_TSP_sched_localcopy(tmp_recvbuf, bottom_count, recvtype,
                                 (char *) recvbuf + top_count * recvtype_extent,
                                 bottom_count, recvtype, sched, 0, NULL);
    }

    for (i = 0; i < nphases; i++) {
        MPL_free(r_counts[i]);
        MPL_free(s_counts[i]);
    }

    MPL_free(r_counts);
    MPL_free(s_counts);
    MPL_free(recv_id);
    MPL_free(recv_index);

  fn_exit:
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_BRUCKS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Non-blocking brucks based Allgatherv */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallgatherv_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallgatherv_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                      MPIR_Request ** req, int k)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_BRUCKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_BRUCKS);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    mpi_errno = MPIR_TSP_Iallgatherv_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm_ptr,
                                                        sched, k);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm_ptr, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_BRUCKS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
