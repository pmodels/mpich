/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHMI_mpi_comm_connect(const char *port_name, MPIR_Info * info, int root, int timeout,
                                MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    int ret;

    ret = MPIDI_POSIX_mpi_comm_connect(port_name, info, root, timeout, comm, newcomm_ptr);

    return ret;
}

int MPIDI_SHMI_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int ret;

    ret = MPIDI_POSIX_mpi_comm_disconnect(comm_ptr);

    return ret;
}

int MPIDI_SHMI_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int ret;

    ret = MPIDI_POSIX_mpi_open_port(info_ptr, port_name);

    return ret;
}

int MPIDI_SHMI_mpi_close_port(const char *port_name)
{
    int ret;

    ret = MPIDI_POSIX_mpi_close_port(port_name);

    return ret;
}

int MPIDI_SHMI_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                               MPIR_Comm ** newcomm_ptr)
{
    int ret;

    ret = MPIDI_POSIX_mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    return ret;
}
