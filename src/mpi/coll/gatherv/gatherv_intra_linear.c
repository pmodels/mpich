
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

#undef FUNCNAME
#define FUNCNAME MPIR_Gatherv_intra_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gatherv_intra_linear(const void *sendbuf,
                              int sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              const int *recvcounts,
                              const int *displs,
                              MPI_Datatype recvtype,
                              int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size = 0;
    int rank = -1;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint extent = 0;
    int i = 0;
    int reqs = 0;
    int min_procs = 0;
    MPIR_Request **reqarray = NULL;
    MPI_Status *starray = NULL;

    MPIR_CHKLMEM_DECL(2);

    rank = comm_ptr->rank;

    /* If rank == root, then I recv lots, otherwise I send */
    if (root == rank) {

        comm_size = comm_ptr->local_size;

        MPIR_Datatype_get_extent_macro(recvtype, extent);
        /* each node can make sure it is not going to overflow aint */
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         displs[rank] * extent);

        MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **,
                            comm_size * sizeof(MPIR_Request *), mpi_errno,
                            "reqarray", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(starray, MPI_Status *,
                            comm_size * sizeof(MPI_Status), mpi_errno, "starray", MPL_MEM_BUFFER);

        reqs = 0;
        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if (i == rank) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                                   ((char *) recvbuf + displs[rank] * extent),
                                                   recvcounts[rank], recvtype);
                        if (mpi_errno) {
                            MPIR_ERR_POP(mpi_errno);
                        }
                    }
                } else {
                    mpi_errno = MPIC_Irecv(((char *) recvbuf + displs[i] * extent),
                                           recvcounts[i], recvtype, i,
                                           MPIR_GATHERV_TAG, comm_ptr, &reqarray[reqs++]);
                    if (mpi_errno) {
                        MPIR_ERR_POP(mpi_errno);
                    }
                }
            }
        }
        /* ... then wait for *all* of them to finish: */
        mpi_errno = MPIC_Waitall(reqs, reqarray, starray, errflag);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS) {
            MPIR_ERR_POP(mpi_errno);
        }

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < reqs; i++) {
                if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = starray[i].MPI_ERROR;
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

    else if (root != MPI_PROC_NULL) {   /* non-root nodes */
        if (sendcount) {
            /* we want local size in the intracomm
             * because the size of the root's group (group A in the standard) is
             * irrelevant here. */
            comm_size = comm_ptr->local_size;

            mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, root,
                                  MPIR_GATHERV_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);

            }
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret) {
        mpi_errno = mpi_errno_ret;
    } else if (*errflag != MPIR_ERR_NONE) {
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
