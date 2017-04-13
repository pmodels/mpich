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
#ifndef STUBNM_RECV_H_INCLUDED
#define STUBNM_RECV_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_mpi_recv(void *buf,
                                    int count,
                                    MPI_Datatype datatype,
                                    int rank,
                                    int tag,
                                    MPIR_Comm * comm,
                                    int context_offset,
                                    MPIDI_av_entry_t *addr,
                                    MPI_Status * status,
                                    MPIR_Request ** request)
{
    int err = MPI_SUCCESS;
    MPIR_Assert(0);
    return err;
}

static inline int MPIDI_NM_mpi_recv_init(void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIDI_av_entry_t *addr,
                                         MPIR_Request ** request)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_imrecv(void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      MPIR_Request * message, MPIR_Request ** rreqp)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_irecv(void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset,
                                     MPIDI_av_entry_t *addr,
                                     MPIR_Request ** request)
{
    int err = MPI_SUCCESS;
    MPIR_Assert(0);
    return err;
}

static inline int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_RECV_H_INCLUDED */
