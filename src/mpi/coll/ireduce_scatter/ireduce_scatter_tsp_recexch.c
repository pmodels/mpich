/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "recexchalgo.h"

/* Routine to schedule a recursive exchange based reduce_scatter with distance halving in each phase */
/*
    MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2 - recursive exchange between pofk partners

Input Parameters:
+ tmp_results - buffer to store intermediate results (choice)
. tmp_recvbuf - buffer to receive intermediate data from partners (choice)
. displs - array to specify the displacements of counts for each process (integer array)
. datatype - datatype of each buffer element (handle)
. op - Type of allreduce operation (handle)
. extent - extent of datatype (integer)
. tag - message tag (integer)
. comm - communicator (handle)
. k - number of processes exchanging data in each step (integer)
. is_dist_halving - variable to denote whether we exchange data starting with
                    the farthest partner different receive buffers are (integer)
. step2_nphases - number of phases to exchange data in step2 (integer)
. step2_nbrs - partners to receive data from in each phase of step2
               each phase would have k-1 partners (2D integer array)
. rank - rank of the process in the communicator (integer)
. nranks - total ranks in the communicator (integer)
. sink_id - id of previous sink from scheduler (integer)
. recv_id - array to store ids of the receives from the scheduler (integer array)
. is_out_vtcs - variable to denote if there is going to be an outgoing vertices
                in the scheduler from this function (integer)
. reduce_id - array with ids of reduces from the scheduler (integer array)
. vtcs - array to specify dependencies of a call to the scheduler (integer array)
- sched - scheduler (handle)

*/
int MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2(void *tmp_results, void *tmp_recvbuf,
                                                       const MPI_Aint * recvcounts,
                                                       MPI_Aint * displs, MPI_Datatype datatype,
                                                       MPI_Op op, size_t extent, int tag,
                                                       MPIR_Comm * comm, int k, int is_dist_halving,
                                                       int step2_nphases, int **step2_nbrs,
                                                       int rank, int nranks, int sink_id,
                                                       int is_out_vtcs, int *reduce_id_,
                                                       MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int x, i, j, phase;
    int current_cnt, offset, rank_for_offset, dst;
    MPI_Aint send_cnt, recv_cnt, send_offset, recv_offset;
    int nvtcs, vtcs[2];
    int send_id, recv_id, reduce_id = -1;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    for (x = 0, phase = step2_nphases - 1; phase >= 0; phase--, x++) {
        for (i = 0; i < k - 1; i++) {
            if (is_dist_halving)
                dst = step2_nbrs[x][i];
            else
                dst = step2_nbrs[phase][i];

            /* Both send and recv have similar dependencies */
            nvtcs = 1;
            if (phase == step2_nphases - 1 && i == 0) {
                vtcs[0] = sink_id;
            } else {
                vtcs[0] = reduce_id;
            }

            rank_for_offset =
                (is_dist_halving) ? MPII_Recexchalgo_reverse_digits_step2(dst, nranks, k) : dst;
            MPII_Recexchalgo_get_count_and_offset(rank_for_offset, phase, k, nranks, &current_cnt,
                                                  &offset);
            send_offset = displs[offset] * extent;
            send_cnt = 0;
            for (j = 0; j < current_cnt; j++)
                send_cnt += recvcounts[offset + j];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "phase %d sending to %d send_offset %d send_count %d", phase, dst,
                             send_offset, send_cnt));
            mpi_errno =
                MPIR_TSP_sched_isend((char *) tmp_results + send_offset, send_cnt,
                                     datatype, dst, tag, comm, sched, nvtcs, vtcs, &send_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            rank_for_offset =
                (is_dist_halving) ? MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k) : rank;
            MPII_Recexchalgo_get_count_and_offset(rank_for_offset, phase, k, nranks, &current_cnt,
                                                  &offset);
            recv_offset = displs[offset] * extent;
            recv_cnt = 0;
            for (j = 0; j < current_cnt; j++)
                recv_cnt += recvcounts[offset + j];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "phase %d recving from %d recv_offset %d recv_count %d", phase, dst,
                             recv_offset, recv_cnt));
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) tmp_recvbuf + recv_offset, recv_cnt,
                                     datatype, dst, tag, comm, sched, nvtcs, vtcs, &recv_id);

            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            mpi_errno =
                MPIR_TSP_sched_reduce_local((char *) tmp_recvbuf + recv_offset,
                                            (char *) tmp_results + recv_offset,
                                            recv_cnt, datatype, op, sched, nvtcs, vtcs, &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    if (is_out_vtcs)
        *reduce_id_ = reduce_id;

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


/* Routine to schedule a recursive exchange based reduce_scatter with distance halving in each phase */
int MPIR_TSP_Ireduce_scatter_sched_intra_recexch(const void *sendbuf, void *recvbuf,
                                                 const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm, int is_dist_halving,
                                                 int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int is_inplace;
    size_t extent;
    MPI_Aint lb, true_extent;
    int step1_sendto = -1, step2_nphases = 0, step1_nrecvs = 0;
    int in_step2;
    int *step1_recvfrom = NULL;
    int **step2_nbrs = NULL;
    int nranks, rank, p_of_k, T;
    int total_count, i;
    int dtcopy_id = -1, recv_id = -1, reduce_id = -1, sink_id = -1;
    int nvtcs, vtcs[2];
    void *tmp_recvbuf = NULL, *tmp_results = NULL;
    MPI_Aint *displs;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(1);

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

    total_count = 0;
    for (i = 0; i < nranks; i++) {
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        return mpi_errno;
    }

    MPIR_CHKLMEM_MALLOC(displs, MPI_Aint *, nranks * sizeof(MPI_Aint),
                        mpi_errno, "displs buffer", MPL_MEM_COLL);
    displs[0] = 0;
    for (i = 1; i < nranks; i++) {
        displs[i] = displs[i - 1] + recvcounts[i - 1];
    }

    /* get the neighbors, the function allocates the required memory */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    in_step2 = (step1_sendto == -1);    /* whether this rank participates in Step 2 */
    tmp_results = MPIR_TSP_sched_malloc(total_count * extent, sched);
    tmp_recvbuf = MPIR_TSP_sched_malloc(total_count * extent, sched);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Beforeinitial dt copy"));

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
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy"));

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

    mpi_errno = MPIR_TSP_sched_sink(sched, &sink_id);   /* sink for all the tasks up to end of Step 1 */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Step 2 */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Start Step2"));
    if (in_step2) {
        MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2(tmp_results, tmp_recvbuf,
                                                           recvcounts, displs, datatype, op, extent,
                                                           tag, comm, k, is_dist_halving,
                                                           step2_nphases, step2_nbrs, rank, nranks,
                                                           sink_id, 1, &reduce_id, sched);
        /* copy data from tmp_results buffer correct position into recvbuf for all participating ranks */
        nvtcs = 1;
        vtcs[0] = reduce_id;    /* This assignment will also be used in step3 sends */
        mpi_errno = MPIR_TSP_sched_localcopy((char *) tmp_results + displs[rank] * extent,
                                             recvcounts[rank], datatype, recvbuf, recvcounts[rank],
                                             datatype, sched, nvtcs, vtcs, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After Step 2"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        mpi_errno =
            MPIR_TSP_sched_irecv(recvbuf, recvcounts[rank], datatype, step1_sendto, tag, comm,
                                 sched, 1, &sink_id, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }
    for (i = 0; i < step1_nrecvs; i++) {
        nvtcs = 1;
        /* vtcs will be assigned to last reduce_id in step2 function */
        mpi_errno = MPIR_TSP_sched_isend((char *) tmp_results + displs[step1_recvfrom[i]] * extent,
                                         recvcounts[step1_recvfrom[i]], datatype, step1_recvfrom[i],
                                         tag, comm, sched, nvtcs, vtcs, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Done Step 3"));

  fn_exit:
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_free(step1_recvfrom);
    MPIR_CHKLMEM_FREEALL();

    MPIR_FUNC_EXIT;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
