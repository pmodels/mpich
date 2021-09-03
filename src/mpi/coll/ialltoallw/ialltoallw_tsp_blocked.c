/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a recursive exchange based alltoallv */
int MPIR_TSP_Ialltoallw_sched_intra_blocked(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint sdispls[],
                                            const MPI_Datatype sendtypes[], void *recvbuf,
                                            const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                            const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                            int bblock, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int tag, vtx_id;
    size_t sendtype_size, recvtype_size;
    int nranks, rank;
    int i, j, comm_block, dst;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    MPIR_Assert(sendbuf != MPI_IN_PLACE);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    if (bblock == 0)
        bblock = nranks;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (i = 0; i < nranks; i += bblock) {
        comm_block = nranks - i < bblock ? nranks - i : bblock;

        /* do the communication -- post ss sends and receives: */
        for (j = 0; j < comm_block; j++) {
            dst = (rank + j + i) % nranks;
            if (recvcounts[dst]) {
                MPIR_Datatype_get_size_macro(recvtypes[dst], recvtype_size);
                if (recvtype_size) {
                    mpi_errno = MPIR_TSP_sched_irecv((char *) recvbuf + rdispls[dst],
                                                     recvcounts[dst], recvtypes[dst], dst, tag,
                                                     comm, sched, 0, NULL, &vtx_id);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
                }
            }
        }

        for (j = 0; j < comm_block; j++) {
            dst = (rank - j - i + nranks) % nranks;
            if (sendcounts[dst]) {
                MPIR_Datatype_get_size_macro(sendtypes[dst], sendtype_size);
                if (sendtype_size) {
                    mpi_errno = MPIR_TSP_sched_isend((char *) sendbuf + sdispls[dst],
                                                     sendcounts[dst], sendtypes[dst], dst, tag,
                                                     comm, sched, 0, NULL, &vtx_id);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
                }
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        mpi_errno = MPIR_TSP_sched_fence(sched);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
