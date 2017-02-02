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
#ifndef POSIX_RECV_H_INCLUDED
#define POSIX_RECV_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_mpi_recv)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_recv(void *buf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int context_offset, MPI_Status * status,
                                                  MPIR_Request ** request)
{
    int mpi_errno =
        MPIDIG_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, status, request);
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK(*request, rank, comm);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_mpi_recv)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_recv_init(void *buf,
                                                       MPI_Aint count,
                                                       MPI_Datatype datatype,
                                                       int rank,
                                                       int tag,
                                                       MPIR_Comm * comm, int context_offset,
                                                       MPIR_Request ** request)
{
    return MPIDIG_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_imrecv(void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, MPIR_Request * message,
                                                    MPIR_Request ** rreqp)
{
    return MPIDIG_mpi_imrecv(buf, count, datatype, message, rreqp);
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_mpi_irecv)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_irecv(void *buf,
                                                   MPI_Aint count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIR_Request ** request)
{
    int mpi_errno =
        MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK(*request, rank, comm);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_mpi_cancel_recv)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_recv(MPIR_Request * rreq)
{
    return MPIDIG_mpi_cancel_recv(rreq);
}

#endif /* POSIX_RECV_H_INCLUDED */
