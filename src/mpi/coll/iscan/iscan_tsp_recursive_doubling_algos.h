/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., ISCAN_TSP_RECURSIVE_DOUBLING_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a recursive exchange based scan */
int MPIR_TSP_Iscan_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent, true_extent;
    MPI_Aint lb;
    int nranks, rank;
    int is_commutative;
    int mask, recv_reduce = -1;
    int dst, loop_count;
    void *partial_scan = NULL;
    void *tmp_buf = NULL;
    int tag = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_ISCAN_SCHED_INTRA_RECURSIVE_DOUBLING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_ISCAN_SCHED_INTRA_RECURSIVE_DOUBLING);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    if (count == 0)
        goto fn_exit;

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    partial_scan = MPIR_TSP_sched_malloc(count * extent, sched);

    int dtcopy_id;
    if (sendbuf != MPI_IN_PLACE) {
        /* Since this is an inclusive scan, copy local contribution into
         * recvbuf. */
        MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                 NULL);
        dtcopy_id =
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, partial_scan, count, datatype, sched,
                                     0, NULL);
    } else
        dtcopy_id =
            MPIR_TSP_sched_localcopy(recvbuf, count, datatype, partial_scan, count, datatype, sched,
                                     0, NULL);

    tmp_buf = MPIR_TSP_sched_malloc(count * extent, sched);
    mask = 0x1;
    loop_count = 0;

    int send_id, recv_id;
    int reduce_id = 0;          /* warning fix: icc: maybe used before set */
    while (mask < nranks) {
        dst = rank ^ mask;
        if (dst < nranks) {
            int nvtcs, vtcs[2];

            /* Send partial_scan to dst. Recv into tmp_buf */
            nvtcs = 1;
            vtcs[0] = (loop_count == 0) ? dtcopy_id : reduce_id;
            send_id =
                MPIR_TSP_sched_isend(partial_scan, count, datatype, dst, tag, comm, sched, nvtcs,
                                     vtcs);

            if (recv_reduce != -1) {
                nvtcs++;
                vtcs[1] = recv_reduce;
            }
            recv_id =
                MPIR_TSP_sched_irecv(tmp_buf, count, datatype, dst, tag, comm, sched, nvtcs, vtcs);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            if (rank > dst) {
                reduce_id = MPIR_TSP_sched_reduce_local(tmp_buf, partial_scan, count,
                                                        datatype, op, sched, nvtcs, vtcs);
                recv_reduce = MPIR_TSP_sched_reduce_local(tmp_buf, recvbuf, count,
                                                          datatype, op, sched, nvtcs, vtcs);
            } else {
                if (is_commutative) {
                    reduce_id = MPIR_TSP_sched_reduce_local(tmp_buf, partial_scan, count,
                                                            datatype, op, sched, nvtcs, vtcs);
                } else {
                    reduce_id = MPIR_TSP_sched_reduce_local(partial_scan, tmp_buf, count,
                                                            datatype, op, sched, nvtcs, vtcs);

                    reduce_id = MPIR_TSP_sched_localcopy(tmp_buf, count, datatype,
                                                         partial_scan, count, datatype, sched, 1,
                                                         &reduce_id);
                }
                recv_reduce = -1;
            }
            loop_count++;
        }
        mask <<= 1;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_ISCAN_SCHED_INTRA_RECURSIVE_DOUBLING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking recursive_doubling based SCAN */
int MPIR_TSP_Iscan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op,
                                            MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_ISCAN_INTRA_RECURSIVE_DOUBLING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_ISCAN_INTRA_RECURSIVE_DOUBLING);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched, false);

    mpi_errno =
        MPIR_TSP_Iscan_sched_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                      op, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_ISCAN_INTRA_RECURSIVE_DOUBLING);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
