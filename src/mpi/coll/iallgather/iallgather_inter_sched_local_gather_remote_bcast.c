/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Local gather remote bcast
 *
 * Each group does a gather to local root with the local
 * intracommunicator, and then does an intercommunicator broadcast.
 */

int MPIR_Iallgather_inter_sched_local_gather_remote_bcast(const void *sendbuf, MPI_Aint sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          MPI_Aint recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size, root;
    MPI_Aint sendtype_sz;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    if ((rank == 0) && (sendcount != 0)) {
        /* In each group, rank 0 allocates temp. buffer for local
         * gather */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_sz);
        tmp_buf = MPIR_Sched_alloc_state(s, sendcount * local_size * sendtype_sz);
        MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    } else {
        /* silence -Wmaybe-uninitialized due to MPIR_{Igather,Ibcast}_sched by non-zero ranks */
        sendtype_sz = 0;
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        mpi_errno = MPIR_Igather_intra_sched_auto(sendbuf, sendcount, sendtype,
                                                  tmp_buf, sendcount * sendtype_sz, MPI_BYTE, 0,
                                                  newcomm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPIR_Ibcast_inter_sched_auto(tmp_buf, sendcount * local_size * sendtype_sz,
                                                     MPI_BYTE, root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPIR_Ibcast_inter_sched_auto(recvbuf, recvcount * remote_size,
                                                     recvtype, root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPIR_Ibcast_inter_sched_auto(recvbuf, recvcount * remote_size,
                                                     recvtype, root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPIR_Ibcast_inter_sched_auto(tmp_buf, sendcount * local_size * sendtype_sz,
                                                     MPI_BYTE, root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
