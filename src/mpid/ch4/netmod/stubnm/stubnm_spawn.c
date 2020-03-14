/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stubnm_impl.h"

int MPIDI_STUBNM_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                                  MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_close_port(const char *port_name)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root,
                                 MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
