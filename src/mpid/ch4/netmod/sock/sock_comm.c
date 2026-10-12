/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"

int MPIDI_SOCK_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    /* TODO: address exchange for bootstrap comms */
    return MPI_SUCCESS;
}

int MPIDI_SOCK_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_mpi_comm_free_hook(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    return MPI_SUCCESS;
}

int MPIDI_SOCK_comm_set_vcis(MPIR_Comm * comm, int num_implicit, int num_reserved,
                             MPIDI_num_vci_t * all_num_vcis)
{
    /* TODO: not implemented yet */
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
