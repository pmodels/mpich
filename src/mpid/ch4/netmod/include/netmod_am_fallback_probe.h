/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_AM_FALLBACK_PROBE_H_INCLUDED
#define NETMOD_AM_FALLBACK_PROBE_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int context_offset, MPIDI_av_entry_t * addr,
                                                  int *flag, MPIR_Request ** message,
                                                  MPI_Status * status)
{
    return MPIDIG_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t * addr,
                                                 int *flag, MPI_Status * status)
{
    return MPIDIG_mpi_iprobe(source, tag, comm, context_offset, flag, status);
}

#endif /* NETMOD_AM_FALLBACK_PROBE_H_INCLUDED */
