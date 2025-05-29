/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: MPI_Gatherv
 *
 * Since the array of recvcounts is valid only on the root, we cannot do a tree
 * algorithm without first communicating the recvcounts to other processes.
 * Therefore, we simply use a linear algorithm for the gather, which takes
 * (p-1) steps versus lgp steps for the tree algorithm. The bandwidth
 * requirement is the same for both algorithms.
 *
 * Cost = (p-1).alpha + n.((p-1)/p).beta
*/
int MPIR_Gatherv_allcomm_linear(const void *sendbuf,
                                MPI_Aint sendcount,
                                MPI_Datatype sendtype,
                                void *recvbuf,
                                const MPI_Aint * recvcounts,
                                const MPI_Aint * displs,
                                MPI_Datatype recvtype,
                                int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent;
    int i, reqs;
    MPIR_Request **reqarray;
    MPI_Status *starray;
    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(recvtype, extent);

        MPIR_CHKLMEM_MALLOC(reqarray, comm_size * sizeof(MPIR_Request *));
        MPIR_CHKLMEM_MALLOC(starray, comm_size * sizeof(MPI_Status));

        reqs = 0;
        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                                   ((char *) recvbuf + displs[rank] * extent),
                                                   recvcounts[rank], recvtype);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                } else {
                    mpi_errno = MPIC_Irecv(((char *) recvbuf + displs[i] * extent),
                                           recvcounts[i], recvtype, i,
                                           MPIR_GATHERV_TAG, comm_ptr, &reqarray[reqs++]);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
        /* ... then wait for *all* of them to finish: */
        mpi_errno = MPIC_Waitall(reqs, reqarray, starray);
        MPIR_ERR_CHECK(mpi_errno);
    }

    else if (root != MPI_PROC_NULL) {   /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (sendcount) {
            mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, root,
                                  MPIR_GATHERV_TAG, comm_ptr, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }


  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
