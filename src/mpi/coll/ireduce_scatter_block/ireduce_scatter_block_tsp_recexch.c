/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "recexchalgo.h"

/* Routine to schedule a recursive exchange based reduce_scatter with distance halving in each phase */
int MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch(const void *sendbuf, void *recvbuf,
                                                       MPI_Aint recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm, int k,
                                                       MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int is_inplace;
    MPI_Aint extent, lb, true_extent;
    int step1_sendto = -1, step2_nphases = 0, step1_nrecvs = 0;
    int in_step2;
    int *step1_recvfrom = NULL;
    int **step2_nbrs = NULL;
    int nranks, rank, p_of_k, T, dst;
    int i, phase, offset;
    int dtcopy_id = -1, send_id = -1, recv_id = -1, reduce_id = -1, step1_id = -1;
    int nvtcs, vtcs[2];
    void *tmp_recvbuf = NULL, *tmp_results = NULL;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    MPIR_Assert(MPIR_Op_is_commutative(op) == 1);

    MPI_Aint total_count;
    total_count = nranks * recvcount;

    /* get the neighbors, the function allocates the required memory */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    in_step2 = (step1_sendto == -1);    /* whether this rank participates in Step 2 */
    tmp_results = MPIR_TSP_sched_malloc(total_count * extent, sched);
    tmp_recvbuf = MPIR_TSP_sched_malloc(total_count * extent, sched);

    if (in_step2) {
        if (!is_inplace)
            mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, total_count, datatype,
                                                 tmp_results, total_count, datatype, sched, 0,
                                                 NULL, &dtcopy_id);
        else
            mpi_errno = MPIR_TSP_sched_localcopy(recvbuf, total_count, datatype,
                                                 tmp_results, total_count, datatype, sched, 0,
                                                 NULL, &dtcopy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    /* Step 1 */
    if (!in_step2) {
        /* non-participating rank sends the data to a participating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        mpi_errno =
            MPIR_TSP_sched_isend(buf_to_send, total_count, datatype, step1_sendto, tag, comm, sched,
                                 0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {    /* Step 2 participating rank */
        for (i = 0; i < step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            /* recv dependencies */
            nvtcs = 1;
            vtcs[0] = (i == 0) ? dtcopy_id : reduce_id;
            mpi_errno = MPIR_TSP_sched_irecv(tmp_recvbuf, total_count, datatype,
                                             step1_recvfrom[i], tag, comm, sched, nvtcs, vtcs,
                                             &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            nvtcs++;
            vtcs[1] = recv_id;
            mpi_errno = MPIR_TSP_sched_reduce_local(tmp_recvbuf, tmp_results, total_count,
                                                    datatype, op, sched, nvtcs, vtcs, &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    mpi_errno = MPIR_TSP_sched_sink(sched, &step1_id);  /* sink for all the tasks up to end of Step 1 */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Step 2 */
    for (phase = step2_nphases - 1; phase >= 0 && step1_sendto == -1; phase--) {
        for (i = 0; i < k - 1; i++) {
            dst = step2_nbrs[phase][i];
            int send_cnt = 0, recv_cnt = 0;

            /* Both send and recv have similar dependencies */
            nvtcs = 1;
            if (phase == step2_nphases - 1 && i == 0) {
                vtcs[0] = step1_id;
            } else {
                vtcs[0] = reduce_id;
            }

            MPII_Recexchalgo_get_count_and_offset(dst, phase, k, nranks, &send_cnt, &offset);
            MPI_Aint send_offset = offset * extent * recvcount;

            mpi_errno =
                MPIR_TSP_sched_isend((char *) tmp_results + send_offset, send_cnt * recvcount,
                                     datatype, dst, tag, comm, sched, nvtcs, vtcs, &send_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            MPII_Recexchalgo_get_count_and_offset(rank, phase, k, nranks, &recv_cnt, &offset);
            MPI_Aint recv_offset = offset * extent * recvcount;

            mpi_errno =
                MPIR_TSP_sched_irecv(tmp_recvbuf, recv_cnt * recvcount,
                                     datatype, dst, tag, comm, sched, nvtcs, vtcs, &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            mpi_errno =
                MPIR_TSP_sched_reduce_local(tmp_recvbuf,
                                            (char *) tmp_results + recv_offset,
                                            recv_cnt * recvcount, datatype, op, sched, nvtcs, vtcs,
                                            &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }
    if (in_step2) {
        nvtcs = 1;
        vtcs[0] = reduce_id;
        /* copy data from tmp_results buffer correct position into recvbuf for all participating ranks */
        mpi_errno =
            MPIR_TSP_sched_localcopy((char *) tmp_results + rank * recvcount * extent, recvcount,
                                     datatype, recvbuf, recvcount, datatype, sched, nvtcs, vtcs,
                                     &dtcopy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        mpi_errno =
            MPIR_TSP_sched_irecv(recvbuf, recvcount, datatype, step1_sendto, tag, comm, sched, 1,
                                 &step1_id, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }
    for (i = 0; i < step1_nrecvs; i++) {
        nvtcs = 1;
        vtcs[0] = reduce_id;
        mpi_errno =
            MPIR_TSP_sched_isend((char *) tmp_results + recvcount * step1_recvfrom[i] * extent,
                                 recvcount, datatype, step1_recvfrom[i], tag, comm, sched, nvtcs,
                                 vtcs, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

  fn_exit:
    /* free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_free(step1_recvfrom);

    MPIR_FUNC_EXIT;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
