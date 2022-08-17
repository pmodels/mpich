/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
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
MPIR_TSP_Iallgatherv_sched_intra_brucks(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                        int k, MPIR_TSP_sched_t sched)
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

    int tag;
    int *recv_id = NULL;
    MPI_Aint *recv_index = NULL;
    int *scount_lookup = NULL;
    MPIR_CHKLMEM_DECL(3);
    void *tmp_recvbuf = NULL;
    int **s_counts = NULL;
    int **r_counts = NULL;
    int tmp_sum = 0;
    int idx = 0, vtx_id;
    int prev_delta = 0;
    int count_length, top_count, bottom_count, left_count;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    rank = MPIR_Comm_rank(comm);
    size = MPIR_Comm_size(comm);
    max = size - 1;

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

    while (max) {
        nphases++;
        max /= k;
    }

    /* Check if size is power of k */
    if (MPL_ipow(k, nphases) == size)
        p_of_k = 1;

    /* if nphases=0 then no recv_id needed */
    MPIR_CHKLMEM_MALLOC(recv_id, int *, sizeof(int) * nphases * (k - 1), mpi_errno, "recv_id",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(scount_lookup, int *, sizeof(int) * nphases, mpi_errno, "scount_lookup",
                        MPL_MEM_COLL);

    /* To store the index to receive in various phases and steps within */
    MPIR_CHKLMEM_MALLOC(recv_index, MPI_Aint *, sizeof(MPI_Aint) * nphases * (k - 1), mpi_errno,
                        "recv_index", MPL_MEM_COLL);

    for (i = 0; i < size; i++)
        total_recvcount += recvcounts[i];

    if (rank == 0)
        tmp_recvbuf = recvbuf;
    else
        tmp_recvbuf = MPIR_TSP_sched_malloc(total_recvcount * recvtype_extent, sched);
    MPIR_ERR_CHKANDJUMP(!tmp_recvbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");


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

    MPI_Aint index_sum;
    index_sum = recvcounts[rank];       /* because in initially you copy your own data to the top of recv_buf */
    if (nphases > 0)
        recv_index[idx++] = index_sum;

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

        }
        delta *= k;
    }

    /* Step1: copy own data from sendbuf to top of recvbuf */
    if (is_inplace && rank != 0)
        mpi_errno = MPIR_TSP_sched_localcopy((char *) recvbuf + displs[rank] * recvtype_extent,
                                             recvcounts[rank], recvtype, tmp_recvbuf,
                                             recvcounts[rank], recvtype, sched, 0, NULL, &vtx_id);
    else if (!is_inplace)
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype, tmp_recvbuf,
                                             recvcounts[rank], recvtype, sched, 0, NULL, &vtx_id);

    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    idx = 0;
    delta = 1;
    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {
            /* If the first location exceeds comm size, nothing is to be sent */
            if (MPL_ipow(k, i) * j >= size)
                break;

            dst = (int) (size + (rank - delta * j)) % size;
            src = (int) (rank + delta * j) % size;
            /* Recv at the exact location */
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) tmp_recvbuf + recv_index[idx] * recvtype_extent,
                                     r_counts[i][j - 1], recvtype, src, tag, comm, sched, 0, NULL,
                                     &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            recv_id[idx] = vtx_id;
            idx++;

            /* Send from the start of recv till the count amount of data */
            mpi_errno =
                MPIR_TSP_sched_isend(tmp_recvbuf, s_counts[i][j - 1], recvtype, dst, tag, comm,
                                     sched, n_invtcs, recv_id, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        n_invtcs += (k - 1);
        delta *= k;
    }
    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    /* No shift required for rank 0 */
    if (rank != 0) {
        idx = 0;
        if (MPII_Iallgatherv_is_displs_ordered(size, recvcounts, displs)) {
            /* Calculate idx(same as count) till 0th rank's data */
            for (i = 0; i < (size - rank); i++)
                idx += recvcounts[(rank + i) % size];

            bottom_count = idx;
            top_count = total_recvcount - idx;
            mpi_errno =
                MPIR_TSP_sched_localcopy((char *) tmp_recvbuf + bottom_count * recvtype_extent,
                                         top_count, recvtype, recvbuf, top_count, recvtype, sched,
                                         0, NULL, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            mpi_errno = MPIR_TSP_sched_localcopy(tmp_recvbuf, bottom_count, recvtype,
                                                 (char *) recvbuf + top_count * recvtype_extent,
                                                 bottom_count, recvtype, sched, 0, NULL, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        } else {
            for (i = 0; i < size; i++) {
                src = (rank + i) % size;        /* Rank whose data it is copying */
                idx += (i == 0) ? 0 : recvcounts[(rank + i - 1) % size];
                mpi_errno = MPIR_TSP_sched_localcopy((char *) tmp_recvbuf + idx * recvtype_extent,
                                                     recvcounts[src], recvtype,
                                                     (char *) recvbuf +
                                                     displs[src] * recvtype_extent, recvcounts[src],
                                                     recvtype, sched, 0, NULL, &vtx_id);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
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
    MPIR_FUNC_ENTER;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
