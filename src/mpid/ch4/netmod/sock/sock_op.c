/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"

int MPIDI_SOCK_mpi_op_commit_hook(MPIR_Op * op_p)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_mpi_op_free_hook(MPIR_Op * op_p)
{
    return MPI_SUCCESS;
}
