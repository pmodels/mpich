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

#define MPIDI_POSIX_SEND_VSIS(vsi_src_, vsi_dst_) \
    do { \
        vsi_src_ = MPIDI_POSIX_get_vsi(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
        vsi_dst_ = MPIDI_POSIX_get_vsi(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_isend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int vsi_src, vsi_dst;
    MPIDI_POSIX_SEND_VSIS(vsi_src, vsi_dst);

    return MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                            vsi_src, vsi_dst, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_isend_coll(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * addr,
                                                    MPIR_Request ** request,
                                                    MPIR_Errflag_t * errflag)
{
    int vsi_src, vsi_dst;
    MPIDI_POSIX_SEND_VSIS(vsi_src, vsi_dst);

    return MPIDIG_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, addr,
                             vsi_src, vsi_dst, request, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_issend(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * addr,
                                                    MPIR_Request ** request)
{
    int vsi_src, vsi_dst;
    MPIDI_POSIX_SEND_VSIS(vsi_src, vsi_dst);

    return MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                             vsi_src, vsi_dst, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* POSIX_SEND_H_INCLUDED */
