/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_AM_FALLBACK_PROBE_H_INCLUDED
#define NETMOD_AM_FALLBACK_PROBE_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int attr, MPIDI_av_entry_t * addr,
                                                  int *flag, MPIR_Request ** message,
                                                  MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, 0, flag, message, status);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int attr, MPIDI_av_entry_t * addr,
                                                 int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, 0, flag, status);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

#endif /* NETMOD_AM_FALLBACK_PROBE_H_INCLUDED */
