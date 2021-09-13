/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_RECV_H_INCLUDED
#define POSIX_RECV_H_INCLUDED

#include "posix_impl.h"
#include "posix_eager.h"
#include "ch4_impl.h"

/* Active message only need local vsi since all messages go to the same per-vsi queue */
#define MPIDI_OFI_RECV_VSI(vsi_) \
    do { \
        /* NOTE: hashing is based on target rank */ \
        vsi_ = MPIDI_POSIX_get_vsi(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
    } while (0)

/* Hook triggered after posting a SHM receive request.
 * It hints the SHM/POSIX internal transport that the user is expecting
 * an incoming message from a specific rank, thus allowing the transport
 * to optimize progress polling (i.e., in POSIX/eager/fbox, the polling
 * will start from this rank at the next progress polling, see
 * MPIDI_POSIX_eager_recv_begin). */
MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_recv_posted_hook(MPIR_Request * request, int rank,
                                                           MPIR_Comm * comm)
{
    MPIDI_POSIX_EAGER_RECV_POSTED_HOOK(request, rank, comm);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_imrecv(void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, MPIR_Request * message)
{
    return MPIDIG_mpi_imrecv(buf, count, datatype, message);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_irecv(void *buf,
                                                   MPI_Aint count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIR_Request ** request)
{
    int vsi = MPIDI_POSIX_get_vsi(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag);
    int mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                     vsi, request, 1, NULL);
    MPIDI_POSIX_recv_posted_hook(*request, rank, comm);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_recv(MPIR_Request * rreq)
{
    return MPIDIG_mpi_cancel_recv(rreq);
}

#endif /* POSIX_RECV_H_INCLUDED */
