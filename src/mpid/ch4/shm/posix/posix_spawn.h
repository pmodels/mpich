/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_SPAWN_H_INCLUDED
#define POSIX_SPAWN_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_POSIX_mpi_comm_connect(const char *port_name,
                                             MPIR_Info * info,
                                             int root, int timeout,
                                             MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_CONNECT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_CONNECT);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_DISCONNECT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_DISCONNECT);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_OPEN_PORT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_OPEN_PORT);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_mpi_close_port(const char *port_name)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_CLOSE_PORT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_CLOSE_PORT);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_mpi_comm_accept(const char *port_name,
                                            MPIR_Info * info,
                                            int root, MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_ACCEPT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_ACCEPT);
    return MPI_SUCCESS;
}

#endif /* POSIX_SPAWN_H_INCLUDED */
