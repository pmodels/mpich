/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Pairwise Exchange
 *
 * This pairwise exchange algorithm takes p-1 steps.
 *
 * We calculate the pairs by using an exclusive-or algorithm:
 *         for (i=1; i<comm_size; i++)
 *             dest = rank ^ i;
 * This algorithm doesn't work if the number of processes is not a power of
 * two. For a non-power-of-two number of processes, we use an
 * algorithm in which, in step i, each process  receives from (rank-i)
 * and sends to (rank+i).
 *
 * Cost = (p-1).alpha + n.beta
 *
 * where n is the total amount of data a process needs to send to all
 * other processes.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_intra_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_intra_pairwise(const void *sendbuf,
                                 int sendcount,
                                 MPI_Datatype sendtype,
                                 void *recvbuf,
                                 int recvcount,
                                 MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, i, pof2;
    MPI_Aint sendtype_extent, recvtype_extent;
    int mpi_errno = MPI_SUCCESS, src, dst, rank;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;

    if (recvcount == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif /* HAVE_ERROR_CHECKING */

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);

    /* Make local copy first */
    mpi_errno = MPIR_Localcopy(((char *) sendbuf +
                                rank * sendcount * sendtype_extent),
                               sendcount, sendtype,
                               ((char *) recvbuf +
                                rank * recvcount * recvtype_extent), recvcount, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* Is comm_size a power-of-two? */
    i = 1;
    while (i < comm_size)
        i *= 2;
    if (i == comm_size)
        pof2 = 1;
    else
        pof2 = 0;

    /* Do the pairwise exchanges */
    for (i = 1; i < comm_size; i++) {
        if (pof2 == 1) {
            /* use exclusive-or algorithm */
            src = dst = rank ^ i;
        } else {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
        }

        mpi_errno = MPIC_Sendrecv(((char *) sendbuf +
                                   dst * sendcount * sendtype_extent),
                                  sendcount, sendtype, dst,
                                  MPIR_ALLTOALL_TAG,
                                  ((char *) recvbuf +
                                   src * recvcount * recvtype_extent),
                                  recvcount, recvtype, src,
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

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
