/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Scatterv_inter_generic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scatterv_inter_generic(const void *sendbuf,
                                const int *sendcounts,
                                const int *displs,
                                MPI_Datatype sendtype,
                                void *recvbuf,
                                int recvcount,
                                MPI_Datatype recvtype,
                                int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int rank = -1;
    int comm_size = 0;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint extent;
    int i = 0;
    int reqs = 0;
    MPIR_Request **reqarray = NULL;
    MPI_Status *starray = NULL;

    MPIR_CHKLMEM_DECL(2);

    rank = comm_ptr->rank;

    /* If I'm the root, then scatter */
    if (root == MPI_ROOT) {
        comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(sendtype, extent);
        /* We need a check to ensure extent will fit in a
         * pointer. That needs extent * (max count) but we can't get
         * that without looping over the input data. This is at least
         * a minimal sanity check. Maybe add a global var since we do
         * loop over sendcount[] in MPI_Scatterv before calling
         * this? */
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf + extent);

        MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, comm_size * sizeof(MPIR_Request *),
                            mpi_errno, "reqarray", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, comm_size * sizeof(MPI_Status),
                            mpi_errno, "starray", MPL_MEM_BUFFER);

        reqs = 0;
        for (i = 0; i < comm_size; i++) {
            if (sendcounts[i]) {
                {
                    mpi_errno = MPIC_Isend(((char *) sendbuf + displs[i] * extent),
                                           sendcounts[i], sendtype, i,
                                           MPIR_SCATTERV_TAG, comm_ptr, &reqarray[reqs++], errflag);
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
    else if (root != MPI_PROC_NULL) { /* non-root nodes on remote side */
        if (recvcount) {
            mpi_errno = MPIC_Recv(recvbuf, recvcount, recvtype, root,
                                  MPIR_SCATTERV_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
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
