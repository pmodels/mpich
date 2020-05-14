/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Ialltoallv_intra_sched_inplace(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                        const int recvcounts[], const int rdispls[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int max_count;
    void *tmp_buf = NULL;
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    int i, j;
    MPI_Aint recvtype_extent, recvtype_sz;
    int dst, rank;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get extent and size of recvtype, don't look at sendtype for MPI_IN_PLACE */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);

    /* The regular MPI_Alltoallv handles MPI_IN_PLACE using pairwise
     * sendrecv_replace calls.  We don't have a sendrecv_replace, so just
     * malloc the maximum of the counts array entries and then perform the
     * pairwise exchanges manually with schedule barriers instead.
     *
     * Because of this approach all processes must agree on the global
     * schedule of "sendrecv_replace" operations to avoid deadlock.
     *
     * This keeps with the spirit of the MPI-2.2 standard, which is to
     * conserve memory when using MPI_IN_PLACE for these routines.
     * Something like MADRE would probably generate a more optimal
     * algorithm. */
    max_count = 0;
    for (i = 0; i < comm_size; ++i) {
        max_count = MPL_MAX(max_count, recvcounts[i]);
    }

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, max_count * recvtype_sz, mpi_errno,
                              "Ialltoallv tmp_buf", MPL_MEM_BUFFER);

    for (i = 0; i < comm_size; ++i) {
        /* start inner loop at i to avoid re-exchanging data */
        for (j = i; j < comm_size; ++j) {
            if (rank == i && rank == j) {
                /* no need to "sendrecv_replace" for ourselves */
            } else if (rank == i || rank == j) {
                if (rank == i)
                    dst = j;
                else
                    dst = i;

                mpi_errno = MPIR_Sched_send(((char *) recvbuf + rdispls[dst] * recvtype_extent),
                                            recvcounts[dst], recvtype, dst, comm_ptr, s);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Sched_recv(tmp_buf, recvcounts[dst] * recvtype_sz, MPI_BYTE,
                                            dst, comm_ptr, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                mpi_errno = MPIR_Sched_copy(tmp_buf, recvcounts[dst] * recvtype_sz, MPI_BYTE,
                                            ((char *) recvbuf + rdispls[dst] * recvtype_extent),
                                            recvcounts[dst], recvtype, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
        }
    }

    MPIR_SCHED_BARRIER(s);

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
