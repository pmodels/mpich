/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_intra_pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf,
                                                  int sendcount,
                                                  MPI_Datatype sendtype,
                                                  void *recvbuf,
                                                  int recvcount,
                                                  MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, i, j;
    MPI_Aint recvtype_extent;
    int mpi_errno = MPI_SUCCESS, rank;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;

    if (recvcount == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

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
                                          MPIR_ALLTOALL_TAG, comm_ptr, &status, errflag);
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
                mpi_errno =
                    MPIC_Sendrecv_replace(((char *) recvbuf + i * recvcount * recvtype_extent),
                                          recvcount, recvtype, i, MPIR_ALLTOALL_TAG, i,
                                          MPIR_ALLTOALL_TAG, comm_ptr, &status, errflag);
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
