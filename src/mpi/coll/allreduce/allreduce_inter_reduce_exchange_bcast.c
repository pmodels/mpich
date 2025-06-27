/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Intercommunicator Allreduce
 *
 * We first do intracommunicator reduces to rank 0 on left and right
 * groups, then an exchange between left and right rank 0, and finally
 * intracommunicator broadcasts from rank 0 on left and right
 * group.
 */

int MPIR_Allreduce_inter_reduce_exchange_bcast(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL();

    if (comm_ptr->rank == 0) {
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);
        MPIR_CHKLMEM_MALLOC(tmp_buf, count * (MPL_MAX(extent, true_extent)));
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    /* Do a local reduce on this intracommunicator */
    mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, newcomm_ptr, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Do a exchange between local and remote rank 0 on this intercommunicator */
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(tmp_buf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  recvbuf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Do a local broadcast on this intracommunicator */
    mpi_errno = MPIR_Bcast(recvbuf, count, datatype, 0, newcomm_ptr, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
