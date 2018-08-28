/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALL_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLTOALL_SCATTERED_OUTSTANDING_TASKS
      category    : COLLECTIVE
      type        : int
      default     : 64
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum number of outstanding sends and recvs posted at a time

    - name        : MPIR_CVAR_IALLTOALL_SCATTERED_BATCH_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of send/receive tasks that scattered algorithm waits for
        completion before posting another batch of send/receives of that size

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "tsp_namespace_def.h"

/* Routine to schedule a scattered based alltoall */
int MPIR_TSP_Ialltoall_sched_intra_scattered(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, int batch_size, int bblock,
                                             MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int src, dst;
    int i, j, ww;
    int invtcs;
    void *data_buf;
    int *vtcs, *recv_id, *send_id;
    size_t recvtype_extent;
    size_t sendtype_extent;
    MPI_Aint recvtype_lb, sendtype_lb, sendtype_true_extent, recvtype_true_extent;
    int size, rank;
    int is_inplace;
    int tag = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_SCATTERED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_SCATTERED);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    is_inplace = (sendbuf == MPI_IN_PLACE);

    /* vtcs is twice the batch size to store both send and recv ids */
    vtcs = (int *) MPL_malloc(sizeof(int) * 2 * batch_size, MPL_MEM_COLL);
    recv_id = (int *) MPL_malloc(sizeof(int) * bblock, MPL_MEM_COLL);
    send_id = (int *) MPL_malloc(sizeof(int) * bblock, MPL_MEM_COLL);
    MPIR_Assert(vtcs);
    MPIR_Assert(recv_id);
    MPIR_Assert(send_id);

    if (bblock > size)
        bblock = size;

    /* Get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    if (!is_inplace) {
        MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
        MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
        sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);
    } else {
        sendcount = recvcount;
        sendtype = recvtype;
        sendtype_extent = recvtype_extent;
    }

    if (is_inplace) {
        data_buf = (void *) MPIR_TSP_sched_malloc(size * recvcount * recvtype_extent, sched);
        MPIR_Assert(data_buf != NULL);
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
        int idx = 0;            /* Store vtcs array from the start */
        /* Get the send and recv ids from previous sends/recvs.
         * ((i + j) % bblock) would ensure extracting and storing
         * dependencies in order from '0, 1,...,bblock' */
        for (j = 0; j < ww; j++) {
            vtcs[idx++] = recv_id[(i + j) % bblock];
            vtcs[idx++] = send_id[(i + j) % bblock];
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_SCATTERED);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Scattered sliding window based Alltoall */
int MPIR_TSP_Ialltoall_intra_scattered(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm, int batch_size, int bblock,
                                       MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_SCATTERED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_SCATTERED);

    /* Generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched, false);

    mpi_errno =
        MPIR_TSP_Ialltoall_sched_intra_scattered(sendbuf, sendcount, sendtype,
                                                 recvbuf, recvcount, recvtype, comm, batch_size,
                                                 bblock, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_SCATTERED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
