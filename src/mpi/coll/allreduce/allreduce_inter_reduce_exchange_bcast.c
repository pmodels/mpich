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

int MPIR_Allreduce_inter_reduce_exchange_bcast(MPIR_Csel_coll_sig_s * coll_sig,
                                               MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.allreduce.sendbuf;
    void *recvbuf = coll_sig->u.allreduce.recvbuf;
    MPI_Aint count = coll_sig->u.allreduce.count;
    MPI_Datatype datatype = coll_sig->u.allreduce.datatype;
    MPI_Op op = coll_sig->u.allreduce.op;
    int coll_attr = 0;
    int mpi_errno = MPI_SUCCESS;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL();

    if (comm_ptr->rank == 0) {
        MPI_Aint buf_shift, buf_size;
        MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
        MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - buf_shift);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    /* Do a local reduce on this intracommunicator */
    MPIR_COLL_INNER(Reduce, coll_sig, sendbuf, tmp_buf, count, datatype, op, 0, newcomm_ptr);

    /* Do a exchange between local and remote rank 0 on this intercommunicator */
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(tmp_buf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  recvbuf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Do a local broadcast on this intracommunicator */
    MPIR_COLL_INNER(Bcast, coll_sig, recvbuf, count, datatype, 0, newcomm_ptr);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
