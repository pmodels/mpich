/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "ibcast.h"

#undef FUNCNAME
#define FUNCNAME sched_test_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recv_size;
    struct MPII_Ibcast_state *ibcast_state = (struct MPII_Ibcast_state *) state;
    MPIR_Get_count_impl(&ibcast_state->status, MPI_BYTE, &recv_size);
    if (ibcast_state->n_bytes != recv_size || ibcast_state->status.MPI_ERROR != MPI_SUCCESS) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OTHER,
                                         "**collective_size_mismatch",
                                         "**collective_size_mismatch %d %d", ibcast_state->n_bytes,
                                         recv_size);
    }
    return mpi_errno;
}

/* This routine purely handles the hierarchical version of bcast, and does not
 * currently make any decision about which particular algorithm to use for any
 * subcommunicator. */
int MPIR_Ibcast_sched_intra_smp(void *buffer, int count, MPI_Datatype datatype, int root,
                                MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    struct MPII_Ibcast_state *ibcast_state;
    MPIR_SCHED_CHKPMEM_DECL(1);

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));
#endif
    MPIR_SCHED_CHKPMEM_MALLOC(ibcast_state, struct MPII_Ibcast_state *,
                              sizeof(struct MPII_Ibcast_state), mpi_errno, "MPI_Status",
                              MPL_MEM_BUFFER);

    MPIR_Datatype_get_size_macro(datatype, type_size);

    ibcast_state->n_bytes = type_size * count;
    /* TODO insert packing here */

    /* send to intranode-rank 0 on the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) > 0) {   /* is not the node root (0) *//* and is on our node (!-1) */
        if (root == comm_ptr->rank) {
            mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        } else if (0 == comm_ptr->node_comm->rank) {
            mpi_errno =
                MPIR_Sched_recv_status(buffer, count, datatype,
                                       MPIR_Get_intranode_rank(comm_ptr, root), comm_ptr->node_comm,
                                       &ibcast_state->status, s);
        }
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        mpi_errno = MPIR_Sched_cb(&sched_test_length, ibcast_state, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL) {
        mpi_errno = MPIR_Ibcast_sched(buffer, count, datatype,
                                      MPIR_Get_internode_rank(comm_ptr, root),
                                      comm_ptr->node_roots_comm, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* don't allow the local ops for the intranode phase to start until this has completed */
        MPIR_SCHED_BARRIER(s);
    }
    /* perform the intranode broadcast on all except for the root's node */
    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPIR_Ibcast_sched(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
