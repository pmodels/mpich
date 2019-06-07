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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_COMM_CONNECT);

    ret = MPIDI_POSIX_mpi_comm_connect(port_name, info, root, timeout, comm, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_COMM_CONNECT);
    return ret;
}

int MPIDI_SHMI_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_COMM_DISCONNECT);

    ret = MPIDI_POSIX_mpi_comm_disconnect(comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_COMM_DISCONNECT);
    return ret;
}

int MPIDI_SHMI_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_OPEN_PORT);

    ret = MPIDI_POSIX_mpi_open_port(info_ptr, port_name);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_OPEN_PORT);
    return ret;
}

int MPIDI_SHMI_mpi_close_port(const char *port_name)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_CLOSE_PORT);

    ret = MPIDI_POSIX_mpi_close_port(port_name);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_CLOSE_PORT);
    return ret;
}

int MPIDI_SHMI_mpi_comm_accept(const char *port_name, MPIR_Info * info, int root, MPIR_Comm * comm,
                               MPIR_Comm ** newcomm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_COMM_ACCEPT);

    ret = MPIDI_POSIX_mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_COMM_ACCEPT);
    return ret;
}
