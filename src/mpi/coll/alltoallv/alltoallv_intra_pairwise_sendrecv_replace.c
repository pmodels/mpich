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
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    int comm_size;
    comm_size = comm_ptr->local_size;

    /* Get extent of recv type, but send type is only valid if (sendbuf!=MPI_IN_PLACE) */
    MPI_Aint recv_extent;
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

#if 0
    MPIR_Assert(comm_ptr->intranode_table);
    /* -- 1st exchange within intranode -- */
    for (int i = 0; i < comm_size; ++i) {
        if (comm_ptr->intranode_table[i] == -1)
            continue;
        mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[i] * recv_extent),
                                          recvcounts[i], recvtype, i, MPIR_ALLTOALLV_TAG,
                                          i, MPIR_ALLTOALLV_TAG,
                                          comm_ptr, MPI_STATUS_IGNORE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* -- 2nd exchange over internode -- */
    for (int i = 0; i < comm_size; ++i) {
        if (comm_ptr->intranode_table[i] > -1)
            continue;
        mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[i] * recv_extent),
                                          recvcounts[i], recvtype, i, MPIR_ALLTOALLV_TAG,
                                          i, MPIR_ALLTOALLV_TAG,
                                          comm_ptr, MPI_STATUS_IGNORE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }
#else
    /* Ordering the pairs by flipping bits */
    int max_mask = 1;
    while (max_mask < comm_size) {
        max_mask *= 2;
    }
    for (int mask = 0; mask < max_mask; mask++) {
        int r = comm_ptr->rank ^ mask;
        if (r >= comm_size) {
            continue;
        }
        mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[r] * recv_extent),
                                          recvcounts[r], recvtype, r, MPIR_ALLTOALLV_TAG,
                                          r, MPIR_ALLTOALLV_TAG,
                                          comm_ptr, MPI_STATUS_IGNORE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
