/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* This is the machine-independent implementation of scatterv. The algorithm is:

   Algorithm: Linear

   Since the array of sendcounts is valid only on the root, we cannot
   do a tree algorithm without first communicating the sendcounts to
   other processes. Therefore, we simply use a linear algorithm for the
   scatter, which takes (p-1) steps versus lgp steps for the tree
   algorithm. The bandwidth requirement is the same for both algorithms.

   Cost = (p-1).alpha + n.((p-1)/p).beta
*/
int MPIR_Scatterv_allcomm_linear(const void *sendbuf, const MPI_Aint * sendcounts,
                                 const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                                 MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int rank, comm_size, mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint extent;
    int i, reqs;
    MPIR_Request **reqarray;
    MPI_Status *starray;
    MPIR_CHKLMEM_DECL(2);

    rank = comm_ptr->rank;

    /* If I'm the root, then scatter */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(sendtype, extent);

        MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, comm_size * sizeof(MPIR_Request *),
                            mpi_errno, "reqarray", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, comm_size * sizeof(MPI_Status), mpi_errno,
                            "starray", MPL_MEM_BUFFER);

        reqs = 0;
        for (i = 0; i < comm_size; i++) {
            if (sendcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (recvbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Localcopy(((char *) sendbuf + displs[rank] * extent),
                                                   sendcounts[rank], sendtype,
                                                   recvbuf, recvcount, recvtype);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                } else {
                    mpi_errno = MPIC_Isend(((char *) sendbuf + displs[i] * extent),
                                           sendcounts[i], sendtype, i,
                                           MPIR_SCATTERV_TAG, comm_ptr, &reqarray[reqs++], errflag);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
        /* ... then wait for *all* of them to finish: */
        mpi_errno = MPIC_Waitall(reqs, reqarray, starray, errflag);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIR_ERR_POP(mpi_errno);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < reqs; i++) {
                if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = starray[i].MPI_ERROR;
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

    else if (root != MPI_PROC_NULL) {   /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (recvcount) {
            mpi_errno = MPIC_Recv(recvbuf, recvcount, recvtype, root,
                                  MPIR_SCATTERV_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }


  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
