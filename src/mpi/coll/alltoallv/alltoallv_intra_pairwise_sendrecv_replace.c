/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Pairwise sendrecv
 *
 * Restrictions: For MPI_IN_PLACE
 *
 * We use pair-wise sendrecv_replace in order to conserve memory usage,
 * which is keeping with the spirit of the MPI-2.2 Standard.  But
 * because of this approach all processes must agree on the global
 * schedule of sendrecv_replace operations to avoid deadlock.
 *
 * Note that this is not an especially efficient algorithm in terms of
 * time and there will be multiple repeated malloc/free's rather than
 * maintaining a single buffer across the whole loop.  Something like
 * MADRE is probably the best solution for the MPI_IN_PLACE scenario. */
int MPIR_Alltoallv_intra_pairwise_sendrecv_replace(const void *sendbuf, const MPI_Aint * sendcounts,
                                                   const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                                   void *recvbuf, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, int coll_attr)
{
    int comm_size, i, j;
    MPI_Aint recv_extent;
    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int rank;

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* Get extent of recv type, but send type is only valid if (sendbuf!=MPI_IN_PLACE) */
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf == MPI_IN_PLACE);
#endif

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
                mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[j] * recv_extent),
                                                  recvcounts[j], recvtype,
                                                  j, MPIR_ALLTOALLV_TAG,
                                                  j, MPIR_ALLTOALLV_TAG,
                                                  comm_ptr, &status, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);

            } else if (rank == j) {
                /* same as above with i/j args reversed */
                mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[i] * recv_extent),
                                                  recvcounts[i], recvtype,
                                                  i, MPIR_ALLTOALLV_TAG,
                                                  i, MPIR_ALLTOALLV_TAG,
                                                  comm_ptr, &status, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
