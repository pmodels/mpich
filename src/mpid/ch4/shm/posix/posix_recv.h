/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_RECV_H_INCLUDED
#define POSIX_RECV_H_INCLUDED

#include "posix_impl.h"
#include "posix_eager.h"
#include "ch4_impl.h"

/* Active message only need local vci since all messages go to the same per-vci queue */
#define MPIDI_POSIX_RECV_VSI(vci_) \
    do { \
        int vci_src_tmp; \
        MPIDI_EXPLICIT_VCIS(comm, attr, rank, comm->rank, vci_src_tmp, vci_); \
        if (vci_src_tmp == 0 && vci_ == 0) { \
            vci_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
        } \
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
    int mpi_errno = MPI_SUCCESS;

    int vci = MPIDI_Request_get_vci(message);
    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci);
    mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_irecv(void *buf,
                                                   MPI_Aint count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag,
                                                   MPIR_Comm * comm, int attr,
                                                   MPIR_cc_t * parent_cc_ptr,
                                                   MPIR_Request ** request)
{
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    bool need_lock;
    int vci;
    if (*request) {
        need_lock = false;
        vci = MPIDI_Request_get_vci(*request);
    } else {
        need_lock = true;
        MPIDI_POSIX_RECV_VSI(vci);
        MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci);
    }
    int mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                     vci, request, 1, NULL);

    /* if the lock is not set, we come from anysource and it's ok to set the counter here */
    if (parent_cc_ptr) {
        if (MPIR_Request_is_complete(*request)) {
            /* if the request is already completed, decrement the parent counter */
            MPIR_cc_dec(parent_cc_ptr);
        } else {
            /* if the request is not done yet, assign the completion pointer to the parent one and it will be decremented later */
            (*request)->dev.completion_notification = parent_cc_ptr;
        }
    }

    if (need_lock) {
        MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci);
    }

    MPIDI_POSIX_recv_posted_hook(*request, rank, comm);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_recv(MPIR_Request * rreq)
{
    return MPIDIG_mpi_cancel_recv(rreq);
}

#endif /* POSIX_RECV_H_INCLUDED */
