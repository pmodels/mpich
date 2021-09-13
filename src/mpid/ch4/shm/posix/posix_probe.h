/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_PROBE_H_INCLUDED
#define POSIX_PROBE_H_INCLUDED

#include "mpidch4r.h"
#include "posix_impl.h"


MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_improbe(int source,
                                                     int tag,
                                                     MPIR_Comm * comm,
                                                     int context_offset,
                                                     int *flag, MPIR_Request ** message,
                                                     MPI_Status * status)
{
    int vsi = MPIDI_POSIX_get_vsi(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag);
    return MPIDIG_mpi_improbe(source, tag, comm, context_offset, vsi, flag, message, status);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_iprobe(int source,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int context_offset, int *flag,
                                                    MPI_Status * status)
{
    int vsi = MPIDI_POSIX_get_vsi(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag);
    return MPIDIG_mpi_iprobe(source, tag, comm, context_offset, vsi, flag, status);
}

#endif /* POSIX_PROBE_H_INCLUDED */
