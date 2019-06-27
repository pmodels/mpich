/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLGATHERV_TSP_BRUCKS_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"
#include "iallgatherv.h"

/* XXX The code has work around to deal with a known failure with a ICC version
 * 2019.0.105 build with the following configuration:
 * ./configure -C --disable-perftest --disable-ft-tests --with-fwrapname=mpigf \
   --with-filesystem=ufs+nfs --enable-timer-type=linux86_cycle --enable-romio  \
   --with-mpe=no --with-smpcoll=yes --with-assert-level=0 --enable-shared \
   --enable-static --enable-error-messages=yes --enable-visibility --enable-large-tests \
   --enable-strict --enable-collalgo-tests --with-ch4-posix-eager-modules=fbox \
   --enable-g=none --enable-error-checking=no --enable-fast=all,O3 --disable-debuginfo \
   --with-device=ch4:ofi:sockets --enable-handle-allocation=default --enable-threads=multiple \
   --without-valgrind --enable-timing=none --enable-ch4-direct=auto \
   --with-ch4-netmod-ofi-args= --enable-thread-cs=global --with-libfabric=embedded \
   MPICHLIB_CFLAGS="-O3 -Wall -ggdb  -ggdb -mtune=generic -std=gnu99 -Wcheck -Wall -w3 \
   -wd869 -wd280 -wd593 -wd2259 -wd981" MPICHLIB_CXXFLAGS="-O3 -Wall -ggdb  -ggdb \
   -mtune=generic -Wcheck -Wall -w3 -wd869 -wd280 -wd593 -wd2259 -wd981" \
   MPICHLIB_FCFLAGS="-O3 -Wall -ggdb  -ggdb -mtune=generic -w" MPICHLIB_F77FLAGS="-O3 \
   -Wall -ggdb  -ggdb -mtune=generic -w" MPICHLIB_LDFLAGS="-O3 -L/usr/lib64 -O0" \
   CC=icc LDFLAGS=" -Wl,-z,muldefs -Wl,-z,now" CXX=icpc FC=ifort F77=ifort --disable-checkerrors
 */

int
MPIR_TSP_Iallgatherv_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                        MPIR_TSP_sched_t * sched, int k)
{
    int i, j, l;
    int nphases = 0;
    int n_invtcs = 0;
    int src, dst, p_of_k = 0;   /* largest power of k that is (strictly) smaller than 'size' */
    int total_recvcount = 0;
    int delta = 1;

    int is_inplace, rank, size, max;
    MPI_Aint sendtype_extent, sendtype_lb;
    MPI_Aint recvtype_extent, recvtype_lb;
    MPI_Aint sendtype_true_extent, recvtype_true_extent;

#ifdef MPL_USE_DBG_LOGGING
    size_t sendtype_size;
#endif

    int tag;
    int *recv_id = NULL;
    int *recv_index = NULL;
    int *scount_lookup = NULL;
    MPIR_CHKLMEM_DECL(3);
    void *tmp_recvbuf = NULL;
    int **s_counts = NULL;
    int **r_counts = NULL;
    int tmp_sum = 0;
    int idx = 0;
    int index_sum = 0;
    int prev_delta = 0;
    int count_length, top_count, bottom_count, left_count;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_BRUCKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_BRUCKS);

    int mpi_errno = MPI_SUCCESS;
    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    rank = MPIR_Comm_rank(comm);
    size = MPIR_Comm_size(comm);
    max = size - 1;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "allgatherv_brucks: num_ranks: %d, k: %d", size, k));

    if (is_inplace) {
        sendcount = recvcounts[rank];
        sendtype = recvtype;
    }

    /* Get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

#ifdef MPL_USE_DBG_LOGGING
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "send_type_size: %lu, sendtype_extent: %lu, send_count: %d",
                     sendtype_size, sendtype_extent, sendcount));
#endif

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

    /* if nphases=0 then no recv_id needed */
    MPIR_CHKLMEM_MALLOC(recv_id, int *, sizeof(int) * nphases * (k - 1), mpi_errno, "recv_id",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(scount_lookup, int *, sizeof(int) * nphases, mpi_errno, "scount_lookup",
                        MPL_MEM_COLL);

    /* To store the index to receive in various phases and steps within */
    MPIR_CHKLMEM_MALLOC(recv_index, int *, sizeof(int) * nphases * (k - 1), mpi_errno, "recv_index",
                        MPL_MEM_COLL);

    for (i = 0; i < size; i++)
        total_recvcount += recvcounts[i];

    if (rank == 0)
        tmp_recvbuf = recvbuf;
    else
        tmp_recvbuf = MPIR_TSP_sched_malloc(total_recvcount * recvtype_extent, sched);

    r_counts = (int **) MPL_malloc(sizeof(int *) * nphases, MPL_MEM_COLL);
    if (nphases > 0)
        MPIR_ERR_CHKANDJUMP(!r_counts, mpi_errno, MPI_ERR_OTHER, "**nomem");

    s_counts = (int **) MPL_malloc(sizeof(int *) * nphases, MPL_MEM_COLL);
    if (nphases > 0)
        MPIR_ERR_CHKANDJUMP(!s_counts, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (i = 0; i < nphases; i++) {
        r_counts[i] = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!r_counts[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
        s_counts[i] = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!s_counts[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    index_sum = recvcounts[rank];       /* because in initially you copy your own data to the top of recv_buf */
    if (nphases > 0)
        recv_index[idx++] = index_sum;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "addresses of all allocated memory:\n recv_index:%p, recv_id:%p, r_counts:%p, s_counts:%p, tmp_recvbuf:%p",
                     recv_index, recv_id, r_counts, s_counts, tmp_recvbuf));

    delta = 1;
    for (i = 0; i < nphases; i++) {
        scount_lookup[i] = (i == 0) ? 0 : scount_lookup[i - 1];
        for (l = prev_delta; l < delta; l++) {
            scount_lookup[i] += recvcounts[(rank + l) % size];
        }
        prev_delta = delta;
        delta *= k;
    }

    delta = 1;
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
            tmp_sum = 0;
            /* XXX We use a temporary variable to hold the sum instead of directly
             * accumulating in r_counts[i][j-1]. Using r_counts[i][j-1] does not
             * compile properly for a certain build. The same issue happens with
             * s_counts[i][j - 1] after we use the tmp_sum for r_counts[i][j - 1].
             * Details of the configuration are at the top of the file. */
            for (l = 0; l < count_length; l++) {
                tmp_sum += recvcounts[(src + l) % size];
            }
            r_counts[i][j - 1] = tmp_sum;

            if (count_length != delta) {
                tmp_sum = 0;
                for (l = 0; l < count_length; l++) {
                    tmp_sum += recvcounts[(rank + l) % size];
                }
                s_counts[i][j - 1] = tmp_sum;
            } else {
                s_counts[i][j - 1] = scount_lookup[i];
            }

            /* Helps point to correct recv location in tmp_recvbuf */
            index_sum += r_counts[i][j - 1];
            if (idx < nphases * (k - 1))
                recv_index[idx++] = index_sum;

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

    idx = 0;
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
            recv_id[idx] =
                MPIR_TSP_sched_irecv((char *) tmp_recvbuf + recv_index[idx] * recvtype_extent,
                                     r_counts[i][j - 1], recvtype, src, tag, comm, sched, 0, NULL);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "Phase#%d:, k:%d with recv_index[idx:%d]:%d at Recv at:%p from src:%d for recv_count:%d",
                             i, k, idx, recv_index[idx],
                             ((char *) tmp_recvbuf + recv_index[idx] * recvtype_extent), src,
                             r_counts[i][j - 1]));
            idx++;

            /* Send from the start of recv till the count amount of data */
            MPIR_TSP_sched_isend(tmp_recvbuf, s_counts[i][j - 1], recvtype, dst, tag, comm, sched,
                                 n_invtcs, recv_id);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                                     "Phase#%d:, k:%d Send from:%p to dst:%d for count:%d",
                                                     i, k, tmp_recvbuf, dst, s_counts[i][j - 1]));
        }
        n_invtcs += (k - 1);
        delta *= k;
    }
    MPIR_TSP_sched_fence(sched);

    /* No shift required for rank 0 */
    if (rank != 0) {
        idx = 0;
        if (MPII_Iallgatherv_is_displs_ordered(size, recvcounts, displs)) {
            /* Calculate idx(same as count) till 0th rank's data */
            for (i = 0; i < (size - rank); i++)
                idx += recvcounts[(rank + i) % size];

            bottom_count = idx;
            top_count = total_recvcount - idx;
            MPIR_TSP_sched_localcopy((char *) tmp_recvbuf + bottom_count * recvtype_extent,
                                     top_count, recvtype, recvbuf, top_count, recvtype, sched, 0,
                                     NULL);
            MPIR_TSP_sched_localcopy(tmp_recvbuf, bottom_count, recvtype,
                                     (char *) recvbuf + top_count * recvtype_extent, bottom_count,
                                     recvtype, sched, 0, NULL);
        } else {
            for (i = 0; i < size; i++) {
                src = (rank + i) % size;        /* Rank whose data it is copying */
                idx += (i == 0) ? 0 : recvcounts[(rank + i - 1) % size];
                MPIR_TSP_sched_localcopy((char *) tmp_recvbuf + idx * recvtype_extent,
                                         recvcounts[src], recvtype,
                                         (char *) recvbuf + displs[src] * recvtype_extent,
                                         recvcounts[src], recvtype, sched, 0, NULL);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "Copied rank %d's data for recvcounts:%d from idx:%d at displs:%d",
                                 src, recvcounts[src], idx, displs[src]));
            }
        }
    }

    for (i = 0; i < nphases; i++) {
        MPL_free(r_counts[i]);
        MPL_free(s_counts[i]);
    }
    MPL_free(r_counts);
    MPL_free(s_counts);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_SCHED_INTRA_BRUCKS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Non-blocking brucks based Allgatherv */
int MPIR_TSP_Iallgatherv_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                      MPIR_Request ** req, int k)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_BRUCKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_BRUCKS);

    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched);

    mpi_errno = MPIR_TSP_Iallgatherv_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm_ptr,
                                                        sched, k);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLGATHERV_INTRA_BRUCKS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
