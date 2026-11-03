/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Iscan_intra_sched_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.scan.sendbuf;
    void *recvbuf = coll_sig->u.scan.recvbuf;
    MPI_Aint count = coll_sig->u.scan.count;
    MPI_Datatype datatype = coll_sig->u.scan.datatype;
    MPI_Op op = coll_sig->u.scan.op;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    MPIR_Comm *node_comm;
    MPIR_Comm *roots_comm;
    void *tempbuf = NULL;
    void *prefulldata = NULL;
    void *localfulldata = NULL;
    unsigned saved_flags = coll_sig->flags;

    node_comm = comm_ptr->node_comm;
    roots_comm = comm_ptr->node_roots_comm;

    MPI_Aint buf_shift, buf_size;
    MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
    tempbuf = MPIR_Sched_alloc_state(s, buf_size);
    MPIR_ERR_CHKANDJUMP(!tempbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    tempbuf = (void *) ((char *) tempbuf - buf_shift);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        prefulldata = MPIR_Sched_alloc_state(s, buf_size);
        MPIR_ERR_CHKANDJUMP(!prefulldata, mpi_errno, MPI_ERR_OTHER, "**nomem");
        prefulldata = (void *) ((char *) prefulldata - buf_shift);

        if (node_comm != NULL) {
            localfulldata = MPIR_Sched_alloc_state(s, buf_size);
            MPIR_ERR_CHKANDJUMP(!localfulldata, mpi_errno, MPI_ERR_OTHER, "**nomem");
            localfulldata = (void *) ((char *) localfulldata - buf_shift);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (node_comm != NULL) {
        MPIR_COLL_SCHED_INNER(Iscan, coll_sig, sendbuf, recvbuf, count, datatype, op, node_comm);
        MPIR_SCHED_BARRIER(s);
    } else if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (node_comm != NULL) {
        if (roots_comm != NULL) {
            mpi_errno = MPIR_Sched_recv(localfulldata, count, datatype,
                                        (node_comm->local_size - 1), node_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (node_comm->rank == node_comm->local_size - 1) {
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, 0, node_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else if (roots_comm != NULL) {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the
     * main process of node 3. */
    if (roots_comm != NULL) {
        /* FIXME just use roots_comm->rank instead */
        int roots_rank = MPIR_Get_internode_rank(comm_ptr, rank);
        MPIR_Assert(roots_rank == roots_comm->rank);

        /* localfulldata is either recvbuf or host memory, depending on the size of the node */
        coll_sig->flags &= ~MPIR_COLL_SIG_FLAG__GPU_COLL;
        MPIR_COLL_SCHED_INNER(Iscan, coll_sig, localfulldata, prefulldata, count, datatype, op,
                              roots_comm);
        MPIR_SCHED_BARRIER(s);

        if (roots_rank != roots_comm->local_size - 1) {
            mpi_errno = MPIR_Sched_send(prefulldata, count, datatype, (roots_rank + 1), roots_comm,
                                        s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        if (roots_rank != 0) {
            mpi_errno = MPIR_Sched_recv(tempbuf, count, datatype, (roots_rank - 1), roots_comm, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    /* tempbuf is host memory on all the ranks of the bcast */
    coll_sig->flags &= ~(MPIR_COLL_SIG_FLAG__GPU_LOCAL | MPIR_COLL_SIG_FLAG__GPU_COLL);
    if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
        /* we aren't on "node 0", so our node leader (possibly us) received
         * "prefulldata" from another leader into "tempbuf" */

        if (node_comm != NULL) {
            MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, tempbuf, count, datatype, 0, node_comm);
            MPIR_SCHED_BARRIER(s);
        }

        /* do reduce on tempbuf and recvbuf, finish scan. */
        mpi_errno = MPIR_Sched_reduce(tempbuf, recvbuf, count, datatype, op, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    coll_sig->flags = saved_flags;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
