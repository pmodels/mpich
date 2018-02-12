/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Inplace Alltoallw
 *
 * We use pair-wise sendrecv_replace in order to conserve memory usage, which
 * is keeping with the spirit of the MPI-2.2 Standard.  But because of this
 * approach all processes must agree on the global schedule of sendrecv_replace
 * operations to avoid deadlock.
 *
 * Note that this is not an especially efficient algorithm in terms of time and
 * there will be multiple repeated malloc/free's rather than maintaining a
 * single buffer across the whole loop.  Something like MADRE is probably the
 * best solution for the MPI_IN_PLACE scenario.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallw_intra_pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallw_intra_pairwise_sendrecv_replace(const void *sendbuf, const int sendcounts[],
                                                   const int sdispls[],
                                                   const MPI_Datatype sendtypes[], void *recvbuf,
                                                   const int recvcounts[], const int rdispls[],
                                                   const MPI_Datatype recvtypes[],
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, i, j;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int rank;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

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
                mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[j]),
                                                  recvcounts[j], recvtypes[j],
                                                  j, MPIR_ALLTOALLW_TAG,
                                                  j, MPIR_ALLTOALLW_TAG,
                                                  comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else if (rank == j) {
                /* same as above with i/j args reversed */
                mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[i]),
                                                  recvcounts[i], recvtypes[i],
                                                  i, MPIR_ALLTOALLW_TAG,
                                                  i, MPIR_ALLTOALLW_TAG,
                                                  comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}
