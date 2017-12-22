/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_util.h"
#include "ibcast.h"

struct ibcast_status{
    int curr_bytes;
    int n_bytes;
    MPI_Status status;
};

#undef FUNCNAME
#define FUNCNAME sched_test_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    int recv_size;
    struct  ibcast_status *status = (struct ibcast_status*) state;
    MPIR_Get_count_impl(&status->status, MPI_BYTE, &recv_size);
    if(status->n_bytes != recv_size || status->status.MPI_ERROR != MPI_SUCCESS) {
        mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE,
                FCNAME, __LINE__, MPI_ERR_OTHER,
                "**collective_size_mismatch",
                "**collective_size_mismatch %d %d", status->n_bytes, recv_size);
    }
    return mpi_errno;
}

/* This routine purely handles the hierarchical version of bcast, and does not
 * currently make any decision about which particular algorithm to use for any
 * subcommunicator. */
int MPIR_Ibcast_sched_intra_smp(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_homogeneous;
    MPI_Aint type_size;
    struct ibcast_status *status;
    MPIR_SCHED_CHKPMEM_DECL(1);

    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));
    MPIR_SCHED_CHKPMEM_MALLOC(status, struct ibcast_status*,
                              sizeof(struct ibcast_status), mpi_errno, "MPI_Status", MPL_MEM_BUFFER);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case yet */

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPIR_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

     status->n_bytes = type_size * count;
    /* TODO insert packing here */

    /* send to intranode-rank 0 on the root's node */
    if (comm_ptr->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
    {                                                /* and is on our node (!-1) */
        if (root == comm_ptr->rank) {
            mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        }
        else if (0 == comm_ptr->node_comm->rank) {
            mpi_errno = MPIR_Sched_recv_status(buffer, count, datatype, MPIR_Get_intranode_rank(comm_ptr, root),
                                        comm_ptr->node_comm, &status->status, s);
        }
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        mpi_errno = MPIR_Sched_cb(&sched_test_length, status, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL)
    {
        mpi_errno = MPID_Ibcast_sched(buffer, count, datatype,
                                      MPIR_Get_internode_rank(comm_ptr, root),
                                      comm_ptr->node_roots_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* don't allow the local ops for the intranode phase to start until this has completed */
        MPIR_SCHED_BARRIER(s);
    }
    /* perform the intranode broadcast on all except for the root's node */
    if (comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPID_Ibcast_sched(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
