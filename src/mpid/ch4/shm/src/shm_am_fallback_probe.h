/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_AM_FALLBACK_PROBE_H_INCLUDED
#define SHM_AM_FALLBACK_PROBE_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_improbe(int source,
                                                   int tag,
                                                   MPIR_Comm * comm,
                                                   int context_offset,
                                                   int *flag, MPIR_Request ** message,
                                                   MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, 0, flag, message, status);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iprobe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int context_offset,
                                                  int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, 0, flag, status);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

#endif /* SHM_AM_FALLBACK_PROBE_H_INCLUDED */
