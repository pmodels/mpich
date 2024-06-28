/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Intercommunicator Allgather
 *
 * Each group does a gather to local root with the local
 * intracommunicator, and then does an intercommunicator
 * broadcast.
 */

int MPIR_Allgather_inter_local_gather_remote_bcast(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr)
{
    int rank, local_size, remote_size, mpi_errno = MPI_SUCCESS, root;
    MPI_Aint sendtype_sz;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;

    MPIR_CHKLMEM_DECL(1);

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    if ((rank == 0) && (sendcount != 0)) {
        /* In each group, rank 0 allocates temp. buffer for local
         * gather */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_sz);
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, sendcount * sendtype_sz * local_size, mpi_errno,
                            "tmp_buf", MPL_MEM_BUFFER);
    } else {
        /* silence -Wmaybe-uninitialized due to MPIR_{Gather,Bcast} calls by non-zero ranks */
        sendtype_sz = 0;
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        mpi_errno = MPIR_Gather(sendbuf, sendcount, sendtype, tmp_buf, sendcount * sendtype_sz,
                                MPI_BYTE, 0, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPIR_Bcast(tmp_buf, sendcount * sendtype_sz * local_size,
                                   MPI_BYTE, root, comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPIR_Bcast(recvbuf, recvcount * remote_size, recvtype, root, comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPIR_Bcast(recvbuf, recvcount * remote_size, recvtype, root, comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPIR_Bcast(tmp_buf, sendcount * sendtype_sz * local_size,
                                   MPI_BYTE, root, comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
