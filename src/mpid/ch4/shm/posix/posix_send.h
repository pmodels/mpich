/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * The POSIX shared memory module uses the CH4-fallback active message implementation for tracking
 * requests and performing message matching. This is because these functions are always implemented
 * in software, therefore duplicating the code to perform the matching is a bad idea. In all of
 * these functions, we call back up to the fallback code to start the process of sending the
 * message.
 */

#ifndef POSIX_SEND_H_INCLUDED
#define POSIX_SEND_H_INCLUDED

#include "posix_impl.h"
#include "shm.h"

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_am_eager_limit(void);

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_send(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;

    protocol = MPIDI_SHM_am_choose_protocol(buf, count, datatype, 0, MPIDIG_SEND);
    return MPIDIG_mpi_send_new(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                               protocol);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_send_coll(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr,
                                                   MPIR_Request ** request,
                                                   MPIR_Errflag_t * errflag)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;

    protocol = MPIDI_SHM_am_choose_protocol(buf, count, datatype, 0, MPIDIG_SEND);
    return MPIDIG_send_coll_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, errflag, protocol);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_ssend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;

    protocol = MPIDI_SHM_am_choose_protocol(buf, count, datatype, 0, MPIDIG_SSEND_REQ);
    return MPIDIG_mpi_ssend_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, protocol);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_isend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;

    protocol = MPIDI_SHM_am_choose_protocol(buf, count, datatype, 0, MPIDIG_SEND);
    return MPIDIG_mpi_isend_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, protocol);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_isend_coll(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * addr,
                                                    MPIR_Request ** request,
                                                    MPIR_Errflag_t * errflag)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;

    protocol = MPIDI_SHM_am_choose_protocol(buf, count, datatype, 0, MPIDIG_SEND);
    return MPIDIG_isend_coll_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                 request, errflag, protocol);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_issend(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * addr,
                                                    MPIR_Request ** request)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;

    protocol = MPIDI_SHM_am_choose_protocol(buf, count, datatype, 0, MPIDIG_SSEND_REQ);
    return MPIDIG_mpi_issend_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                 request, protocol);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* POSIX_SEND_H_INCLUDED */
