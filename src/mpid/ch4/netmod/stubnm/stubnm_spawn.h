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
#ifndef STUBNM_SPAWN_H_INCLUDED
#define STUBNM_SPAWN_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_mpi_comm_connect(const char *port_name,
                                            MPIR_Info * info,
                                            int root, int timeout,
                                            MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_close_port(const char *port_name)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_comm_accept(const char *port_name,
                                           MPIR_Info * info,
                                           int root, MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_SPAWN_H_INCLUDED */
