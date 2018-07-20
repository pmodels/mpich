/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_SEND_H_INCLUDED
#define PTL_SEND_H_INCLUDED

#include "ptl_impl.h"

static inline int MPIDI_NM_mpi_send(const void *buf,
                                    int count,
                                    MPI_Datatype datatype,
                                    int rank,
                                    int tag,
                                    MPIR_Comm * comm, int context_offset,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

static inline int MPIDI_NM_mpi_ssend(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset,
                                     MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

static inline int MPIDI_NM_mpi_send_init(const void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

static inline int MPIDI_NM_mpi_ssend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

static inline int MPIDI_NM_mpi_bsend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

static inline int MPIDI_NM_mpi_rsend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

static inline int MPIDI_NM_mpi_isend(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset,
                                     MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

static inline int MPIDI_NM_mpi_issend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset,
                                      MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

static inline int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* PTL_SEND_H_INCLUDED */
