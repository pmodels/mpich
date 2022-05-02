/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_PROBE_H_INCLUDED
#define POSIX_PROBE_H_INCLUDED

#include "mpidch4r.h"
#include "posix_impl.h"

#define MPIDI_POSIX_PROBE_VSI(vsi_) \
    do { \
        int vsi_src_tmp; \
        MPIDI_EXPLICIT_VCIS(comm, attr, source, comm->rank, vsi_src_tmp, vsi_); \
        if (vsi_src_tmp == 0 && vsi_ == 0) { \
            vsi_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
        } \
    } while (0)


MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_improbe(int source,
                                                     int tag,
                                                     MPIR_Comm * comm,
                                                     int attr,
                                                     int *flag, MPIR_Request ** message,
                                                     MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    int vsi;
    MPIDI_POSIX_PROBE_VSI(vsi);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vsi);
    mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, vsi, flag, message, status);
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vsi);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_iprobe(int source,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int attr, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    int vsi;
    MPIDI_POSIX_PROBE_VSI(vsi);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vsi);
    mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, vsi, flag, status);
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vsi);

    return mpi_errno;
}

#endif /* POSIX_PROBE_H_INCLUDED */
