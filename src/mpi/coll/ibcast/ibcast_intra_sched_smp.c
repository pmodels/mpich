/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "ibcast.h"

static int sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recv_size;
    struct MPII_Ibcast_state *ibcast_state = (struct MPII_Ibcast_state *) state;
    MPIR_Get_count_impl(&ibcast_state->status, MPIR_BYTE_INTERNAL, &recv_size);
    if (ibcast_state->n_bytes != recv_size || ibcast_state->status.MPI_ERROR != MPI_SUCCESS) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OTHER,
                                         "**collective_size_mismatch",
                                         "**collective_size_mismatch %d %d", ibcast_state->n_bytes,
                                         recv_size);
    }
    return mpi_errno;
}

/* This routine purely handles the hierarchical version of bcast, and does not
 * currently make any decision about which particular algorithm to use for any
 * subcommunicator. */
int MPIR_Ibcast_intra_sched_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    void *buffer = coll_sig->u.bcast.buffer;
    MPI_Aint count = coll_sig->u.bcast.count;
    MPI_Datatype datatype = coll_sig->u.bcast.datatype;
    int root = coll_sig->u.bcast.root;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));
#endif

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* TODO insert packing here */

    /* send to intranode-rank 0 on the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) > 0) {   /* is not the node root (0) *//* and is on our node (!-1) */
        if (root == comm_ptr->rank) {
            mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (0 == comm_ptr->node_comm->rank && MPIR_ERROR_CHECKS_ENABLED) {
            /* checking the size received, only when error checking is on */
            struct MPII_Ibcast_state *ibcast_state;
            ibcast_state = MPIR_Sched_alloc_state(s, sizeof(struct MPII_Ibcast_state));
            MPIR_ERR_CHKANDJUMP(!ibcast_state, mpi_errno, MPI_ERR_OTHER, "**nomem");
            ibcast_state->n_bytes = type_size * count;

            mpi_errno =
                MPIR_Sched_recv_status(buffer, count, datatype,
                                       MPIR_Get_intranode_rank(comm_ptr, root), comm_ptr->node_comm,
                                       &ibcast_state->status, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_cb(&sched_test_length, ibcast_state, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (0 == comm_ptr->node_comm->rank) {
            mpi_errno = MPIR_Sched_recv(buffer, count, datatype,
                                        MPIR_Get_intranode_rank(comm_ptr, root),
                                        comm_ptr->node_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, buffer, count, datatype,
                              MPIR_Get_internode_rank(comm_ptr, root), comm_ptr->node_roots_comm);

        /* don't allow the local ops for the intranode phase to start until this has completed */
        MPIR_SCHED_BARRIER(s);
    }
    /* perform the intranode broadcast on all except for the root's node */
    if (comm_ptr->node_comm != NULL) {
        MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, buffer, count, datatype, 0, comm_ptr->node_comm);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
