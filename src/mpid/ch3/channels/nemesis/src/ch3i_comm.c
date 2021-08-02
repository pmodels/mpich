/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"
#undef utarray_oom
#define utarray_oom() do { goto fn_oom; } while (0)
#include "utarray.h"

#define NULL_CONTEXT_ID -1

int MPIDI_CH3I_comm_create(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CH3I_comm_destroy(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int nem_coll_finalize(void *param ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}


int MPID_nem_coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Add_finalize(nem_coll_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);
    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
