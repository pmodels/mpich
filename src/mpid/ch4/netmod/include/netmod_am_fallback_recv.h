/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_AM_FALLBACK_RECV_H_INCLUDED
#define NETMOD_AM_FALLBACK_RECV_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 MPI_Aint count, MPI_Datatype datatype,
                                                 MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_Request_get_vci(message);
#endif
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(vci));
    mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(vci));

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                MPIR_Request * partner)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    /* For anysource recv, we may be called while holding the vci lock of shm request (to
     * prevent shm progress). */
    bool need_cs;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    need_cs = true;
#else
    need_cs = (rank != MPI_ANY_SOURCE);
#endif
    if (need_cs) {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(0));
    } else {
#ifdef MPICH_DEBUG_MUTEX
        MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    }

    mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, 0,
                                 request, 0, partner);
    if (need_cs) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(0));
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq, bool is_blocking)
{
    return MPIDIG_mpi_cancel_recv(rreq);
}

#endif /* NETMOD_AM_FALLBACK_RECV_H_INCLUDED */
