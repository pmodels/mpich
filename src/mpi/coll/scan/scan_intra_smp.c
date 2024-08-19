/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


int MPIR_Scan_intra_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, int coll_group,
                        MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(3);
    MPI_Status status;
    void *tempbuf = NULL, *localfulldata = NULL, *prefulldata = NULL;
    MPI_Aint true_lb, true_extent, extent;
    int noneed = 1;             /* noneed=1 means no need to bcast tempbuf and
                                 * reduce tempbuf & recvbuf */

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr, coll_group));
    int local_rank = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].rank;
    int local_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].size;

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_CHKLMEM_MALLOC(tempbuf, void *, count * (MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
    tempbuf = (void *) ((char *) tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (local_rank == 0) {
        MPIR_CHKLMEM_MALLOC(prefulldata, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan", MPL_MEM_BUFFER);
        prefulldata = (void *) ((char *) prefulldata - true_lb);

        if (local_size > 1) {
            MPIR_CHKLMEM_MALLOC(localfulldata, void *, count * (MPL_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan", MPL_MEM_BUFFER);
            localfulldata = (void *) ((char *) localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (local_size > 1) {
        mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype, op,
                              comm_ptr, MPIR_SUBGROUP_NODE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (local_rank == 0 && local_size > 1) {
        mpi_errno = MPIC_Recv(localfulldata, count, datatype,
                              local_size - 1, MPIR_SCAN_TAG, comm_ptr, MPIR_SUBGROUP_NODE, &status);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (local_rank > 0 && local_size > 1 && local_rank == local_size - 1) {
        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                              0, MPIR_SCAN_TAG, comm_ptr, MPIR_SUBGROUP_NODE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (local_rank == 0) {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the
     * main process of node 3. */
    if (local_rank == 0) {
        mpi_errno = MPIR_Scan(localfulldata, prefulldata, count, datatype,
                              op, comm_ptr, MPIR_SUBGROUP_NODE_CROSS, errflag);
        MPIR_ERR_CHECK(mpi_errno);

        int inter_rank = comm_ptr->subgroups[MPIR_SUBGROUP_NODE_CROSS].rank;
        int inter_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE_CROSS].size;
        if (inter_rank != inter_size - 1) {
            mpi_errno = MPIC_Send(prefulldata, count, datatype,
                                  inter_rank + 1,
                                  MPIR_SCAN_TAG, comm_ptr, MPIR_SUBGROUP_NODE_CROSS, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (inter_rank != 0) {
            mpi_errno = MPIC_Recv(tempbuf, count, datatype,
                                  inter_rank - 1,
                                  MPIR_SCAN_TAG, comm_ptr, MPIR_SUBGROUP_NODE_CROSS, &status);
            noneed = 0;
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    if (local_size > 1) {
        mpi_errno = MPIR_Bcast(&noneed, 1, MPI_INT, 0, comm_ptr, MPIR_SUBGROUP_NODE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (noneed == 0) {
        if (local_size > 1) {
            mpi_errno = MPIR_Bcast(tempbuf, count, datatype, 0,
                                   comm_ptr, MPIR_SUBGROUP_NODE, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = MPIR_Reduce_local(tempbuf, recvbuf, count, datatype, op);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
