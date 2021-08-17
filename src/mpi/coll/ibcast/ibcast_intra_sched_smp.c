/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"

#ifdef HAVE_ERROR_CHECKING
static int sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recv_size;
    struct MPII_Ibcast_state *ibcast_state = (struct MPII_Ibcast_state *) state;
    MPIR_Get_count_impl(&ibcast_state->status, MPI_BYTE, &recv_size);
    if (ibcast_state->n_bytes != recv_size || ibcast_state->status.MPI_ERROR != MPI_SUCCESS) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OTHER,
                                         "**collective_size_mismatch",
                                         "**collective_size_mismatch %d %d", ibcast_state->n_bytes,
                                         recv_size);
    }
    return mpi_errno;
}
#endif

/* This routine purely handles the hierarchical version of bcast, and does not
 * currently make any decision about which particular algorithm to use for any
 * subcommunicator. */
int MPIR_Ibcast_intra_sched_smp(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    struct MPII_Ibcast_state *ibcast_state;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));
#endif
    ibcast_state = MPIR_Sched_alloc_state(s, sizeof(struct MPII_Ibcast_state));
    MPIR_ERR_CHKANDJUMP(!ibcast_state, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Datatype_get_size_macro(datatype, type_size);

    ibcast_state->n_bytes = type_size * count;
    /* TODO insert packing here */

    /* send to intranode-rank 0 on the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) > 0) {   /* is not the node root (0) *//* and is on our node (!-1) */
        if (root == comm_ptr->rank) {
            mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (0 == comm_ptr->node_comm->rank) {
            mpi_errno =
                MPIR_Sched_recv_status(buffer, count, datatype,
                                       MPIR_Get_intranode_rank(comm_ptr, root), comm_ptr->node_comm,
                                       &ibcast_state->status, s);
            MPIR_ERR_CHECK(mpi_errno);
#ifdef HAVE_ERROR_CHECKING
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_cb(&sched_test_length, ibcast_state, s);
            MPIR_ERR_CHECK(mpi_errno);
#endif
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL) {
        mpi_errno = MPIR_Ibcast_intra_sched_auto(buffer, count, datatype,
                                                 MPIR_Get_internode_rank(comm_ptr, root),
                                                 comm_ptr->node_roots_comm, s);
        MPIR_ERR_CHECK(mpi_errno);

        /* don't allow the local ops for the intranode phase to start until this has completed */
        MPIR_SCHED_BARRIER(s);
    }
    /* perform the intranode broadcast on all except for the root's node */
    if (comm_ptr->node_comm != NULL) {
        mpi_errno =
            MPIR_Ibcast_intra_sched_auto(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
