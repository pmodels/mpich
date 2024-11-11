/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Pairwise sendrecv replace
 *
 * Restrictions: Only for MPI_IN_PLACE
 *
 * We use pair-wise sendrecv_replace in order to conserve memory usage,
 * which is keeping with the spirit of the MPI-2.2 Standard.  But
 * because of this approach all processes must agree on the global
 * schedule of sendrecv_replace operations to avoid deadlock.
 *
 * Note that this is not an especially efficient algorithm in terms of
 * time and there will be multiple repeated malloc/free's rather than
 * maintaining a single buffer across the whole loop.  Something like
 * MADRE is probably the best solution for the MPI_IN_PLACE scenario.
 */
int MPIR_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf,
                                                  MPI_Aint sendcount,
                                                  MPI_Datatype sendtype,
                                                  void *recvbuf,
                                                  MPI_Aint recvcount,
                                                  MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, int coll_group,
                                                  MPIR_Errflag_t errflag)
{
    int comm_size, i, j;
    MPI_Aint recvtype_extent;
    int mpi_errno = MPI_SUCCESS, rank;
    MPI_Status status;

    MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* We use pair-wise sendrecv_replace in order to conserve memory usage,
     * which is keeping with the spirit of the MPI-2.2 Standard.  But
     * because of this approach all processes must agree on the global
     * schedule of sendrecv_replace operations to avoid deadlock.
     *
     * Note that this is not an especially efficient algorithm in terms of
     * time and there will be multiple repeated malloc/free's rather than
     * maintaining a single buffer across the whole loop.  Something like
     * MADRE is probably the best solution for the MPI_IN_PLACE scenario. */
    for (i = 0; i < comm_size; ++i) {
        /* start inner loop at i to avoid re-exchanging data */
        for (j = i; j < comm_size; ++j) {
            if (rank == i) {
                /* also covers the (rank == i && rank == j) case */
                mpi_errno =
                    MPIC_Sendrecv_replace(((char *) recvbuf + j * recvcount * recvtype_extent),
                                          recvcount, recvtype, j, MPIR_ALLTOALL_TAG, j,
                                          MPIR_ALLTOALL_TAG, comm_ptr, coll_group, &status,
                                          errflag);
                MPIR_ERR_CHECK(mpi_errno);
            } else if (rank == j) {
                /* same as above with i/j args reversed */
                mpi_errno =
                    MPIC_Sendrecv_replace(((char *) recvbuf + i * recvcount * recvtype_extent),
                                          recvcount, recvtype, i, MPIR_ALLTOALL_TAG, i,
                                          MPIR_ALLTOALL_TAG, comm_ptr, coll_group, &status,
                                          errflag);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
