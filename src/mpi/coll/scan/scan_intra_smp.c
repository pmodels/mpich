/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Scan_intra_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.scan.sendbuf;
    void *recvbuf = coll_sig->u.scan.recvbuf;
    MPI_Aint count = coll_sig->u.scan.count;
    MPI_Datatype datatype = coll_sig->u.scan.datatype;
    MPI_Op op = coll_sig->u.scan.op;
    int coll_attr = 0;
    int mpi_errno = MPI_SUCCESS;
    unsigned saved_flags = coll_sig->flags;
    MPIR_CHKLMEM_DECL();
    int rank;
    int size ATTRIBUTE((unused));
    MPIR_COMM_RANK_SIZE(comm_ptr, rank, size);
    MPI_Status status;
    void *tempbuf = NULL, *localfulldata = NULL, *prefulldata = NULL;
    int noneed = 1;             /* noneed=1 means no need to bcast tempbuf and
                                 * reduce tempbuf & recvbuf */

    MPI_Aint buf_shift, buf_size;
    MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
    MPIR_CHKLMEM_MALLOC(tempbuf, buf_size);
    tempbuf = (void *) ((char *) tempbuf - buf_shift);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_CHKLMEM_MALLOC(prefulldata, buf_size);
        prefulldata = (void *) ((char *) prefulldata - buf_shift);

        if (comm_ptr->node_comm != NULL) {
            MPIR_CHKLMEM_MALLOC(localfulldata, buf_size);
            localfulldata = (void *) ((char *) localfulldata - buf_shift);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (comm_ptr->node_comm != NULL) {
        MPIR_COLL_INNER(Scan, coll_sig, sendbuf, recvbuf, count, datatype, op, comm_ptr->node_comm);
    } else if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL) {
        mpi_errno = MPIC_Recv(localfulldata, count, datatype,
                              comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG,
                              comm_ptr->node_comm, &status);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (comm_ptr->node_roots_comm == NULL &&
               comm_ptr->node_comm != NULL &&
               MPIR_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1) {
        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                              0, MPIR_SCAN_TAG, comm_ptr->node_comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (comm_ptr->node_roots_comm != NULL) {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the
     * main process of node 3. */
    if (comm_ptr->node_roots_comm != NULL) {
        /* localfulldata is either recvbuf or host memory, depending on the size of the node, and
         * prefulldata is host memory. The inner calls below no longer have gpu_coll. */
        coll_sig->flags &= ~MPIR_COLL_SIG_FLAG__GPU_COLL;
        MPIR_COLL_INNER(Scan, coll_sig, localfulldata, prefulldata, count, datatype, op,
                        comm_ptr->node_roots_comm);

        if (MPIR_Get_internode_rank(comm_ptr, rank) != comm_ptr->node_roots_comm->local_size - 1) {
            mpi_errno = MPIC_Send(prefulldata, count, datatype,
                                  MPIR_Get_internode_rank(comm_ptr, rank) + 1,
                                  MPIR_SCAN_TAG, comm_ptr->node_roots_comm, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
            mpi_errno = MPIC_Recv(tempbuf, count, datatype,
                                  MPIR_Get_internode_rank(comm_ptr, rank) - 1,
                                  MPIR_SCAN_TAG, comm_ptr->node_roots_comm, &status);
            noneed = 0;
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    /* the bcasts pass host buffers only */
    coll_sig->flags &= ~(MPIR_COLL_SIG_FLAG__GPU_LOCAL | MPIR_COLL_SIG_FLAG__GPU_COLL);
    if (comm_ptr->node_comm != NULL) {
        MPIR_COLL_INNER(Bcast, coll_sig, &noneed, 1, MPIR_INT_INTERNAL, 0, comm_ptr->node_comm);
    }

    if (noneed == 0) {
        if (comm_ptr->node_comm != NULL) {
            MPIR_COLL_INNER(Bcast, coll_sig, tempbuf, count, datatype, 0, comm_ptr->node_comm);
        }

        mpi_errno = MPIR_Reduce_local(tempbuf, recvbuf, count, datatype, op);
    }

  fn_exit:
    coll_sig->flags = saved_flags;
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
