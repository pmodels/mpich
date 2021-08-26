/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ch4r_init.h"

int MPIDIG_init_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, subcomm_type, is_localcomm;

    MPIR_FUNC_ENTER;

    MPIR_Assert(MPIDI_global.is_ch4u_initialized);

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

    MPIDIG_COMM(comm, window_instance) = 0;
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDIG_destroy_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, subcomm_type, is_localcomm;
    MPIR_FUNC_ENTER;

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

void *MPIDIG_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_ENTER;
    void *p;

    p = MPL_malloc(size, MPL_MEM_USER);

    MPIR_FUNC_EXIT;
    return p;
}

int MPIDIG_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPL_free(ptr);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
