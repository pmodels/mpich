/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Blocked Alltoallw
 *
 * Since each process sends/receives different amounts of data to every other
 * process, we don't know the total message size for all processes without
 * additional communication. Therefore we simply use the "middle of the road"
 * isend/irecv algorithm that works reasonably well in all cases.
 *
 * We post all irecvs and isends and then do a waitall. We scatter the order of
 * sources and destinations among the processes, so that all processes don't
 * try to send/recv to/from the same process at the same time.
 *
 * *** Modification: We post only a small number of isends and irecvs at a time
 * and wait on them as suggested by Tony Ladd. ***
 */
int MPIR_Ialltoallw_intra_sched_blocked(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, i;
    int dst, rank;
    int ii, ss, bblock;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif /* HAVE_ERROR_CHECKING */

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = comm_size;

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (ii = 0; ii < comm_size; ii += bblock) {
        ss = comm_size - ii < bblock ? comm_size - ii : bblock;

        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            if (recvcounts[dst]) {
                MPI_Aint type_size;
                MPIR_Datatype_get_size_macro(recvtypes[dst], type_size);
                if (type_size) {
                    mpi_errno = MPIR_Sched_recv((char *) recvbuf + rdispls[dst],
                                                recvcounts[dst], recvtypes[dst], dst, comm_ptr, s);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            if (sendcounts[dst]) {
                MPI_Aint type_size;
                MPIR_Datatype_get_size_macro(sendtypes[dst], type_size);
                if (type_size) {
                    mpi_errno = MPIR_Sched_send((char *) sendbuf + sdispls[dst],
                                                sendcounts[dst], sendtypes[dst], dst, comm_ptr, s);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        MPIR_SCHED_BARRIER(s);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
