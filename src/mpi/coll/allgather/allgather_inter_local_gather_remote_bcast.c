/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"

/* Intercommunicator Allgather
 *
 * Each group does a gather to local root with the local
 * intracommunicator, and then does an intercommunicator
 * broadcast.
 */

int MPIR_Allgather_inter_local_gather_remote_bcast(MPIR_Csel_coll_sig_s * coll_sig,
                                                   MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.allgather.sendbuf;
    MPI_Aint sendcount = coll_sig->u.allgather.sendcount;
    MPI_Datatype sendtype = coll_sig->u.allgather.sendtype;
    void *recvbuf = coll_sig->u.allgather.recvbuf;
    MPI_Aint recvcount = coll_sig->u.allgather.recvcount;
    MPI_Datatype recvtype = coll_sig->u.allgather.recvtype;
    int rank, local_size, remote_size, mpi_errno = MPI_SUCCESS, root;
    MPI_Aint sendtype_sz;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;

    MPIR_CHKLMEM_DECL();

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    if ((rank == 0) && (sendcount != 0)) {
        /* In each group, rank 0 allocates temp. buffer for local
         * gather */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_sz);
        MPIR_CHKLMEM_MALLOC(tmp_buf, sendcount * sendtype_sz * local_size);
    } else {
        /* silence -Wmaybe-uninitialized due to MPIR_{Gather,Bcast} calls by non-zero ranks */
        sendtype_sz = 0;
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        MPIR_COLL_INNER(Gather, coll_sig, sendbuf, sendcount, sendtype, tmp_buf,
                        sendcount * sendtype_sz, MPIR_BYTE_INTERNAL, 0, newcomm_ptr);
    }

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            MPIR_COLL_INNER(Bcast, coll_sig, tmp_buf, sendcount * sendtype_sz * local_size,
                            MPIR_BYTE_INTERNAL, root, comm_ptr);
        }

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            MPIR_COLL_INNER(Bcast, coll_sig, recvbuf, recvcount * remote_size, recvtype, root,
                            comm_ptr);
        }
    } else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            MPIR_COLL_INNER(Bcast, coll_sig, recvbuf, recvcount * remote_size, recvtype, root,
                            comm_ptr);
        }

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            MPIR_COLL_INNER(Bcast, coll_sig, tmp_buf, sendcount * sendtype_sz * local_size,
                            MPIR_BYTE_INTERNAL, root, comm_ptr);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
