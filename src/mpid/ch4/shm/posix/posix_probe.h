/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_PROBE_H_INCLUDED
#define POSIX_PROBE_H_INCLUDED

#include "mpidch4r.h"
#include "posix_impl.h"

#define MPIDI_POSIX_PROBE_VSI(vci_) \
    do { \
        int vci_src_tmp; \
        MPIDI_EXPLICIT_VCIS(comm, attr, source, comm->rank, vci_src_tmp, vci_); \
        if (vci_src_tmp == 0 && vci_ == 0) { \
            vci_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
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

    int vci;
    MPIDI_POSIX_PROBE_VSI(vci);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci);
    mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, vci, flag, message, status);
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_iprobe(int source,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int attr, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    int vci;
    MPIDI_POSIX_PROBE_VSI(vci);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci);
    mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, vci, flag, status);
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci);

    return mpi_errno;
}

#endif /* POSIX_PROBE_H_INCLUDED */
