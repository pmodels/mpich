/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPIR_Allgather_intra_ring(const void *sendbuf,
                              MPI_Aint sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              MPI_Aint recvcount,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int coll_group,
                              MPIR_Errflag_t errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recvtype_extent;
    int j, i;
    int left, right, jnext;

    MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* First, load the "local" version in the recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   ((char *) recvbuf +
                                    rank * recvcount * recvtype_extent), recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
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
                                  MPIR_ALLGATHER_TAG, comm_ptr, coll_group, MPI_STATUS_IGNORE,
                                  errflag);
        MPIR_ERR_CHECK(mpi_errno);
        j = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
