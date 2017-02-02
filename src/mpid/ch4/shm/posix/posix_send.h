/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_SEND_H_INCLUDED
#define POSIX_SEND_H_INCLUDED

#include "posix_impl.h"

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_SEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_send(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_SSEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_ssend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_SEND_INIT)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_send_init(const void *buf, MPI_Aint count,
                                                       MPI_Datatype datatype, int rank, int tag,
                                                       MPIR_Comm * comm, int context_offset,
                                                       MPIR_Request ** request)
{
    return MPIDIG_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_SSEND_INIT)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_ssend_init(const void *buf, MPI_Aint count,
                                                        MPI_Datatype datatype, int rank, int tag,
                                                        MPIR_Comm * comm, int context_offset,
                                                        MPIR_Request ** request)
{
    return MPIDIG_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_BSEND_INIT)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_bsend_init(const void *buf, MPI_Aint count,
                                                        MPI_Datatype datatype, int rank, int tag,
                                                        MPIR_Comm * comm, int context_offset,
                                                        MPIR_Request ** request)
{
    return MPIDIG_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_RSEND_INIT)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rsend_init(const void *buf, MPI_Aint count,
                                                        MPI_Datatype datatype, int rank, int tag,
                                                        MPIR_Comm * comm, int context_offset,
                                                        MPIR_Request ** request)
{
    return MPIDIG_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_ISEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_isend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_ISSEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_issend(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * addr,
                                                    MPIR_Request ** request)
{
    return MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_CANCEL_SEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* POSIX_SEND_H_INCLUDED */
