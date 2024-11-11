/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a recursive exchange based scan */
int MPIR_TSP_Iscan_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, int coll_group,
                                                  MPIR_TSP_sched_t sched)
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
    int tag = 0, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, coll_group, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_COLL_RANK_SIZE(comm, coll_group, rank, nranks);

    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    partial_scan = MPIR_TSP_sched_malloc(count * extent, sched);

    int dtcopy_id;
    if (sendbuf != MPI_IN_PLACE) {
        /* Since this is an inclusive scan, copy local contribution into
         * recvbuf. */
        mpi_errno =
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                     NULL, &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno =
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, partial_scan, count, datatype, sched,
                                     0, NULL, &dtcopy_id);
    } else
        mpi_errno =
            MPIR_TSP_sched_localcopy(recvbuf, count, datatype, partial_scan, count, datatype, sched,
                                     0, NULL, &dtcopy_id);

    MPIR_ERR_CHECK(mpi_errno);

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
            mpi_errno =
                MPIR_TSP_sched_isend(partial_scan, count, datatype, dst, tag, comm, coll_group,
                                     sched, nvtcs, vtcs, &send_id);

            MPIR_ERR_CHECK(mpi_errno);

            if (recv_reduce != -1) {
                nvtcs++;
                vtcs[1] = recv_reduce;
            }
            mpi_errno =
                MPIR_TSP_sched_irecv(tmp_buf, count, datatype, dst, tag, comm, coll_group, sched,
                                     nvtcs, vtcs, &recv_id);

            MPIR_ERR_CHECK(mpi_errno);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            if (rank > dst) {
                mpi_errno = MPIR_TSP_sched_reduce_local(tmp_buf, partial_scan, count,
                                                        datatype, op, sched, nvtcs, vtcs,
                                                        &reduce_id);
                MPIR_ERR_CHECK(mpi_errno);

                mpi_errno = MPIR_TSP_sched_reduce_local(tmp_buf, recvbuf, count,
                                                        datatype, op, sched, nvtcs, vtcs,
                                                        &recv_reduce);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                if (is_commutative) {
                    mpi_errno = MPIR_TSP_sched_reduce_local(tmp_buf, partial_scan, count,
                                                            datatype, op, sched, nvtcs, vtcs,
                                                            &reduce_id);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    mpi_errno = MPIR_TSP_sched_reduce_local(partial_scan, tmp_buf, count,
                                                            datatype, op, sched, nvtcs, vtcs,
                                                            &reduce_id);

                    MPIR_ERR_CHECK(mpi_errno);

                    mpi_errno = MPIR_TSP_sched_localcopy(tmp_buf, count, datatype,
                                                         partial_scan, count, datatype, sched, 1,
                                                         &reduce_id, &vtx_id);
                    reduce_id = vtx_id;
                    MPIR_ERR_CHECK(mpi_errno);

                }
                recv_reduce = -1;
            }
            loop_count++;
        }
        mask <<= 1;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
