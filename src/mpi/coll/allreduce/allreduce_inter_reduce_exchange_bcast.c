/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Intercommunicator Allreduce
 *
 * We first do intracommunicator reduces to rank 0 on left and right
 * groups, then an exchange between left and right rank 0, and finally
 * intracommunicator broadcasts from rank 0 on left and right
 * group.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_inter_reduce_exchange_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_inter_reduce_exchange_bcast(const void *sendbuf, void *recvbuf, int
                                               count, MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (comm_ptr->rank == 0) {
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);
        /* I think this is the worse case, so we can avoid an assert()
         * inside the for loop */
        /* Should MPIR_CHKLMEM_MALLOC do this? */
        MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)), mpi_errno,
                            "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    /* Do a local reduce on this intracommunicator */
    mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* Do a exchange between local and remote rank 0 on this intercommunicator */
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(tmp_buf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  recvbuf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Do a local broadcast on this intracommunicator */
    mpi_errno = MPIR_Bcast(recvbuf, count, datatype, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
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
