/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a ring exchange based allreduce.
 * The implementation is based on Baidu's ring algorithm
 * for Machine Learning/Deep Learning. The algorithm is
 * explained here: http://andrew.gibiansky.com/ */
int MPIR_TSP_Iallreduce_sched_intra_ring(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                         MPI_Datatype datatype, MPI_Op op,
                                         MPIR_Comm * comm, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, src, dst;
    int nranks, is_inplace, rank;
    size_t extent;
    MPI_Aint lb, true_extent;
    MPI_Aint *cnts, *displs;
    int recv_id, *reduce_id, nvtcs, vtcs;
    int send_rank, recv_rank, total_count;
    void *tmpbuf;
    int tag, vtx_id;
    int coll_attr ATTRIBUTE((unused)) = 0;

    MPIR_FUNC_ENTER;
    MPIR_CHKLMEM_DECL();

    is_inplace = (sendbuf == MPI_IN_PLACE);
    MPIR_COMM_RANK_SIZE(comm, rank, nranks);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_CHKLMEM_MALLOC(cnts, nranks * sizeof(MPI_Aint));
    MPIR_CHKLMEM_MALLOC(displs, nranks * sizeof(MPI_Aint));

    for (i = 0; i < nranks; i++)
        cnts[i] = 0;

    total_count = 0;
    for (i = 0; i < nranks; i++) {
        cnts[i] = (count + nranks - 1) / nranks;
        if (total_count + cnts[i] > count) {
            cnts[i] = count - total_count;
            break;
        } else
            total_count += cnts[i];
    }

    displs[0] = 0;
    for (i = 1; i < nranks; i++)
        displs[i] = displs[i - 1] + cnts[i - 1];

    /* Phase 1: copy to tmp buf */
    if (!is_inplace) {
        mpi_errno =
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                     NULL, &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_TSP_sched_fence(sched);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Phase 2: Ring based send recv reduce scatter */
    /* Need only 2 spaces for current and previous reduce_id(s) */
    MPIR_CHKLMEM_MALLOC(reduce_id, 2 * sizeof(int));
    tmpbuf = MPIR_TSP_sched_malloc(count * extent, sched);

    src = (nranks + rank - 1) % nranks;
    dst = (rank + 1) % nranks;

    for (i = 0; i < nranks - 1; i++) {
        recv_rank = (nranks + rank - 2 - i) % nranks;
        send_rank = (nranks + rank - 1 - i) % nranks;

        /* get a new tag to prevent out of order messages */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        nvtcs = (i == 0) ? 0 : 1;
        vtcs = (i == 0) ? 0 : reduce_id[(i - 1) % 2];
        mpi_errno =
            MPIR_TSP_sched_irecv(tmpbuf, cnts[recv_rank], datatype, src, tag, comm, sched, nvtcs,
                                 &vtcs, &recv_id);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno =
            MPIR_TSP_sched_reduce_local(tmpbuf, (char *) recvbuf + displs[recv_rank] * extent,
                                        cnts[recv_rank], datatype, op, sched, 1, &recv_id,
                                        &reduce_id[i % 2]);

        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno =
            MPIR_TSP_sched_isend((char *) recvbuf + displs[send_rank] * extent, cnts[send_rank],
                                 datatype, dst, tag, comm, sched, nvtcs, &vtcs, &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPIR_CHKLMEM_MALLOC(reduce_id, 2 * sizeof(int));

    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Phase 3: Allgatherv ring, so everyone has the reduced data */
    MPIR_TSP_Iallgatherv_sched_intra_ring(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, recvbuf, cnts,
                                          displs, datatype, comm, sched);

    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
