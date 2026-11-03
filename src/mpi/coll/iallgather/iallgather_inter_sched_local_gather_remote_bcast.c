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

int MPIR_Iallgather_inter_sched_local_gather_remote_bcast(MPIR_Csel_coll_sig_s * coll_sig,
                                                          MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.allgather.sendbuf;
    MPI_Aint sendcount = coll_sig->u.allgather.sendcount;
    MPI_Datatype sendtype = coll_sig->u.allgather.sendtype;
    void *recvbuf = coll_sig->u.allgather.recvbuf;
    MPI_Aint recvcount = coll_sig->u.allgather.recvcount;
    MPI_Datatype recvtype = coll_sig->u.allgather.recvtype;
    MPIR_Sched_t s = coll_sig->sched;
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
        /* silence -Wmaybe-uninitialized by non-zero ranks */
        sendtype_sz = 0;
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        MPIR_COLL_SCHED_INNER(Igather, coll_sig, sendbuf, sendcount, sendtype, tmp_buf,
                              sendcount * sendtype_sz, MPIR_BYTE_INTERNAL, 0, newcomm_ptr);
        MPIR_SCHED_BARRIER(s);
    }

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, tmp_buf, sendcount * local_size * sendtype_sz,
                                  MPIR_BYTE_INTERNAL, root, comm_ptr);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, recvbuf, recvcount * remote_size, recvtype,
                                  root, comm_ptr);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, recvbuf, recvcount * remote_size, recvtype,
                                  root, comm_ptr);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, tmp_buf, sendcount * local_size * sendtype_sz,
                                  MPIR_BYTE_INTERNAL, root, comm_ptr);
        }
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
