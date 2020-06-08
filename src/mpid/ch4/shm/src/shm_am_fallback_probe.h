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
    return MPIDIG_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iprobe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int context_offset,
                                                  int *flag, MPI_Status * status)
{
    return MPIDIG_mpi_iprobe(source, tag, comm, context_offset, flag, status);
}

#endif /* SHM_AM_FALLBACK_PROBE_H_INCLUDED */
