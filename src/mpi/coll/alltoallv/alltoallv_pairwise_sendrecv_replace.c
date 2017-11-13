/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_pairwise_sendrecv_replace (const void *sendbuf, const int *sendcounts, const int *sdispls,
                         MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                         const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                         MPIR_Errflag_t *errflag)
{
    int        comm_size, i, j;
    MPI_Aint   recv_extent;
    int        mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int rank;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get extent of recv type, but send type is only valid if (sendbuf!=MPI_IN_PLACE) */
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);

    MPIR_Assert (sendbuf == MPI_IN_PLACE);
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
                mpi_errno = MPIC_Sendrecv_replace(((char *)recvbuf + rdispls[j]*recv_extent),
                                                     recvcounts[j], recvtype,
                                                     j, MPIR_ALLTOALLV_TAG,
                                                     j, MPIR_ALLTOALLV_TAG,
                                                     comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

            }
            else if (rank == j) {
                /* same as above with i/j args reversed */
                mpi_errno = MPIC_Sendrecv_replace(((char *)recvbuf + rdispls[i]*recv_extent),
                                                     recvcounts[i], recvtype,
                                                     i, MPIR_ALLTOALLV_TAG,
                                                     i, MPIR_ALLTOALLV_TAG,
                                                     comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
    }

fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

fn_fail:
    goto fn_exit;
}
