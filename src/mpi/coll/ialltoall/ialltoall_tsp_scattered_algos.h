/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., IALLTOALL_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

/* Routine to schedule a scattered based alltoall */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ialltoall_sched_intra_scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ialltoall_sched_intra_scattered(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype,
                                             int tag, MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_contig, src, dst, copy_dst;
    int i, j, ww, index = 0;
    int invtcs;
    void *data_buf;
    int nvtcs;

    int size = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);
    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int batch_size = MPIR_CVAR_IALLTOALL_SCATTERED_BATCH_SIZE;
    int bblock = MPIR_CVAR_IALLTOALL_SCATTERED_OUTSTANDING_TASKS;

    /* vtcs is twice the batch size to store both send and recv ids */
    int *vtcs = (int *) MPL_malloc(sizeof(int) * 2 * batch_size, MPL_MEM_COLL);
    int *recv_id = (int *) MPL_malloc(sizeof(int) * bblock, MPL_MEM_COLL);
    int *send_id = (int *) MPL_malloc(sizeof(int) * bblock, MPL_MEM_COLL);

    size_t recvtype_lb, recvtype_extent, recvtype_size;
    size_t sendtype_lb, sendtype_extent, sendtype_size;
    size_t sendtype_true_extent, recvtype_true_extent;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_SCATTERED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_SCATTERED);

    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (bblock > size)
        bblock = size;

    if (is_inplace) {
        sendcount = recvcount;
        sendtype = recvtype;
        sendtype_extent = recvtype_extent;
    }

    /* Get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    if (is_inplace) {
        data_buf = (void *) MPIR_TSP_sched_malloc(size * recvcount * recvtype_extent, sched);
        MPIR_TSP_sched_localcopy((char *) recvbuf, size * recvcount, recvtype, (char *) data_buf,
                                 size * recvcount, recvtype, sched, 0, NULL);
        MPIR_TSP_sched_fence(sched);
    } else {
        data_buf = (void *) sendbuf;
    }

    /* First, post bblock number of sends/recvs */
    for (i = 0; i < bblock; i++) {
        src = (rank + i) % size;
        recv_id[i] =
            MPIR_TSP_sched_irecv((char *) recvbuf + src * recvcount * recvtype_extent,
                                 recvcount, recvtype, src, tag, comm, sched, 0, NULL);
        dst = (rank - i + size) % size;
        send_id[i] =
            MPIR_TSP_sched_isend((char *) data_buf + dst * sendcount * sendtype_extent,
                                 sendcount, sendtype, dst, tag, comm, sched, 0, NULL);
    }

    /* Post more send/recv pairs as the previous ones finish */
    for (i = bblock; i < size; i += batch_size) {
        ww = MPL_MIN(size - i, batch_size);
        index = 0;      /* Store vtcs array from the start */
        /* Get the send and recv ids from previous sends/recvs.
         * ((i + j) % bblock) would ensure extracting and storing
         * dependencies in order from '0, 1,...,bblock' */
        for (j = 0; j < ww; j++) {
            vtcs[index++] = recv_id[(i + j) % bblock];
            vtcs[index++] = send_id[(i + j) % bblock];
        }
        invtcs = MPIR_TSP_sched_selective_sink(sched, 2 * ww, vtcs);
        for (j = 0; j < ww; j++) {
            src = (rank + i + j) % size;
            recv_id[(i + j) % bblock] =
                MPIR_TSP_sched_irecv((char *) recvbuf + src * recvcount * recvtype_extent,
                                     recvcount, recvtype, src, tag, comm, sched, 1, &invtcs);
            dst = (rank - i - j + size) % size;
            send_id[(i + j) % bblock] =
                MPIR_TSP_sched_isend((char *) data_buf + dst * sendcount * sendtype_extent,
                                     sendcount, sendtype, dst, tag, comm, sched, 1, &invtcs);
        }
    }

    MPL_free(vtcs);
    MPL_free(recv_id);
    MPL_free(send_id);

  fn_exit:
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_SCATTERED);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Scattered sliding window based Alltoall */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Ialltoall_intra_scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Ialltoall_intra_scattered(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_SCATTERED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_SCATTERED);

    /* Generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_TSP_Ialltoall_sched_intra_scattered(sendbuf, sendcount, sendtype,
                                                 recvbuf, recvcount, recvtype, tag, comm, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_SCATTERED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
