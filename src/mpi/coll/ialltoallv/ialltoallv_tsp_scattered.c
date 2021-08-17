/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Routine to schedule a scattered based alltoallv */
/* Alltoallv doesn't support MPI_IN_PLACE */
int MPIR_TSP_Ialltoallv_sched_intra_scattered(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const MPI_Aint recvcounts[],
                                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm, int batch_size, int bblock,
                                              MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int src, dst;
    int i, j, ww;
    int invtcs;
    int tag;
    int *vtcs, *recv_id, *send_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(3);

    MPIR_Assert(!(sendbuf == MPI_IN_PLACE));

    int size = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);

    MPI_Aint recvtype_lb, recvtype_extent;
    MPI_Aint sendtype_lb, sendtype_extent;
    MPI_Aint sendtype_true_extent, recvtype_true_extent;

    MPIR_FUNC_ENTER;

    /* Alltoall: each process need send #size msgs and recv #size msgs
     * This algorithm does #bblock send/recv first
     * followed by addition batchs of #batch_size each.
     */
    if (bblock > size)
        bblock = size;

    /* vtcs is twice the batch size to store both send and recv ids */
    MPIR_CHKLMEM_MALLOC(vtcs, int *, 2 * batch_size * sizeof(int), mpi_errno, "vtcs", MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(recv_id, int *, bblock * sizeof(int), mpi_errno, "recv_id", MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(send_id, int *, bblock * sizeof(int), mpi_errno, "send_id", MPL_MEM_COLL);

    /* Get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* First, post bblock number of sends/recvs */
    for (i = 0; i < bblock; i++) {
        src = (rank + i) % size;
        mpi_errno =
            MPIR_TSP_sched_irecv((char *) recvbuf + rdispls[src] * recvtype_extent,
                                 recvcounts[src], recvtype, src, tag, comm, sched, 0, NULL,
                                 &recv_id[i]);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        dst = (rank - i + size) % size;
        mpi_errno =
            MPIR_TSP_sched_isend((char *) sendbuf + sdispls[dst] * sendtype_extent,
                                 sendcounts[dst], sendtype, dst, tag, comm, sched, 0, NULL,
                                 &send_id[i]);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    /* Post more send/recv pairs as the previous ones finish */
    for (i = bblock; i < size; i += batch_size) {
        int i_vtcs = 0;
        ww = MPL_MIN(size - i, batch_size);
        /* Add dependency to ensure not to run until previous block the batch portion
         * is finished -- effectively limiting the on-going tasks to bblock
         */
        for (j = 0; j < ww; j++) {
            vtcs[i_vtcs++] = recv_id[(i + j) % bblock];
            vtcs[i_vtcs++] = send_id[(i + j) % bblock];
        }
        mpi_errno = MPIR_TSP_sched_selective_sink(sched, 2 * ww, vtcs, &invtcs);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        for (j = 0; j < ww; j++) {
            src = (rank + i + j) % size;
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) recvbuf + rdispls[src] * recvtype_extent,
                                     recvcounts[src], recvtype, src, tag, comm, sched, 1, &invtcs,
                                     &recv_id[(i + j) % bblock]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            dst = (rank - i - j + size) % size;
            mpi_errno =
                MPIR_TSP_sched_isend((char *) sendbuf + sdispls[dst] * sendtype_extent,
                                     sendcounts[dst], sendtype, dst, tag, comm, sched, 1, &invtcs,
                                     &send_id[(i + j) % bblock]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_ENTER;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
