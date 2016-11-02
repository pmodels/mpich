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
#ifndef SHM_SHMAM_SEND_H_INCLUDED
#define SHM_SHMAM_SEND_H_INCLUDED

#include "shmam_impl.h"

static inline int MPIDI_SHM_mpi_send(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_send(buf, count, datatype, rank, tag, comm, context_offset,
                               request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_rsend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rsend(buf, count, datatype, rank, tag, comm, context_offset,
                                request, MPIDI_SHM);
}



static inline int MPIDI_SHM_mpi_irsend(const void *buf,
                                       int count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_irsend(buf, count, datatype, rank, tag, comm, context_offset,
                                 request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_ssend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset,
                                request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_startall(int count, MPIR_Request * requests[])
{
    return MPIDI_CH4U_mpi_startall(count, requests);
}

static inline int MPIDI_SHM_mpi_send_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm,
                                          int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset,
                                    request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_ssend_init(const void *buf,
                                           int count,
                                           MPI_Datatype datatype,
                                           int rank,
                                           int tag,
                                           MPIR_Comm * comm,
                                           int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                     request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_bsend_init(const void *buf,
                                           int count,
                                           MPI_Datatype datatype,
                                           int rank,
                                           int tag,
                                           MPIR_Comm * comm,
                                           int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                     request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_rsend_init(const void *buf,
                                           int count,
                                           MPI_Datatype datatype,
                                           int rank,
                                           int tag,
                                           MPIR_Comm * comm,
                                           int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                     request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_isend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset,
                                request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_issend(const void *buf,
                                       int count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset,
                                 request, MPIDI_SHM);
}

static inline int MPIDI_SHM_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDI_CH4U_mpi_cancel_send(sreq, MPIDI_SHM);
}

#endif /* SHM_SHMAM_SEND_H_INCLUDED */
