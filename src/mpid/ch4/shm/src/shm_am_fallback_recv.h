/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_AM_FALLBACK_RECV_H_INCLUDED
#define SHM_AM_FALLBACK_RECV_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_imrecv(void *buf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_Request_get_vci(message);
#endif
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_irecv(void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_cc_t * parent_cc_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    bool need_lock;
    int vci;
    if (*request) {
        need_lock = false;
        vci = MPIDI_Request_get_vci(*request);
    } else {
        need_lock = true;
        vci = 0;
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    }

    mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                 vci, request, 1, NULL);

    /* if no lock is needed then we come from the anysource code so it's correct to assign the parent_ptr anyway */
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
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_recv(MPIR_Request * rreq)
{
    return MPIDIG_mpi_cancel_recv(rreq);
}

#endif /* SHM_AM_FALLBACK_RECV_H_INCLUDED */
