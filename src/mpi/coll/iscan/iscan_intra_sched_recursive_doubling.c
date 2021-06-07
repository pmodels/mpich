/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Iscan_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    int is_commutative;
    int mask, dst, rank, comm_size;
    void *partial_scan = NULL;
    void *tmp_buf = NULL;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store partial scan */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    partial_scan = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!partial_scan, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *) ((char *) partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data */
    tmp_buf = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* Since this is an inclusive scan, copy local contribution into
     * recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, partial_scan, count, datatype, s);
    else
        mpi_errno = MPIR_Sched_copy(recvbuf, count, datatype, partial_scan, count, datatype, s);
    MPIR_ERR_CHECK(mpi_errno);

    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIR_Sched_send(partial_scan, count, datatype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            if (rank > dst) {
                mpi_errno = MPIR_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            } else {
                if (is_commutative) {
                    mpi_errno = MPIR_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                } else {
                    mpi_errno = MPIR_Sched_reduce(partial_scan, tmp_buf, count, datatype, op, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    mpi_errno = MPIR_Sched_copy(tmp_buf, count, datatype,
                                                partial_scan, count, datatype, s);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
            }
        }
        mask <<= 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
