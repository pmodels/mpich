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
                                                     int attr,
                                                     int *flag, MPIR_Request ** message,
                                                     MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vsi = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vsi).lock);
    mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, vsi, flag, message, status);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vsi).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_iprobe(int source,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int attr, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vsi = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vsi).lock);
    mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, vsi, flag, status);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vsi).lock);

    return mpi_errno;
}

#endif /* POSIX_PROBE_H_INCLUDED */
