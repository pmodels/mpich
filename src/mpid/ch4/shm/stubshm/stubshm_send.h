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
#ifndef STUBSHM_SEND_H_INCLUDED
#define STUBSHM_SEND_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_STUBSHM_mpi_send(const void *buf,
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_irsend(const void *buf,
                                       MPI_Aint count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIR_Request ** request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ssend(const void *buf,
                                          MPI_Aint count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_isend(const void *buf,
                                          MPI_Aint count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_issend(const void *buf,
                                           MPI_Aint count,
                                           MPI_Datatype datatype,
                                           int rank,
                                           int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_cancel_send(MPIR_Request * sreq)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

#endif /* STUBSHM_SEND_H_INCLUDED */
