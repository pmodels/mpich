/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Ring
 *
 * In the first step, each process i sends its contribution to process
 * i+1 and receives the contribution from process i-1 (with
 * wrap-around).  From the second step onwards, each process i
 * forwards to process i+1 the data it received from process i-1 in
 * the previous step.  This takes a total of p-1 steps.
 *
 * Cost = (p-1).alpha + n.((p-1)/p).beta
 *
 * This algorithm is preferred to recursive doubling for long messages
 * because we find that this communication pattern (nearest neighbor)
 * performs twice as fast as recursive doubling for long messages (on
 * Myrinet and IBM SP).
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_intra_ring(const void *sendbuf,
                              int sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              int recvcount,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint recvtype_extent;
    int j, i;
    int left, right, jnext;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     (comm_size * recvcount * recvtype_extent));

    /* First, load the "local" version in the recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   ((char *) recvbuf +
                                    rank * recvcount * recvtype_extent), recvcount, recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    /*
     * Now, send left to right.  This fills in the receive area in
     * reverse order.
     */
    left = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    j = rank;
    jnext = left;
    for (i = 1; i < comm_size; i++) {
        mpi_errno = MPIC_Sendrecv(((char *) recvbuf +
                                   j * recvcount * recvtype_extent),
                                  recvcount, recvtype, right,
                                  MPIR_ALLGATHER_TAG,
                                  ((char *) recvbuf +
                                   jnext * recvcount * recvtype_extent),
                                  recvcount, recvtype, left,
                                  MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        j = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
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
