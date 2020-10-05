/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBSHM_PROBE_H_INCLUDED
#define STUBSHM_PROBE_H_INCLUDED

#include "stubshm_impl.h"


MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_improbe(int source,
                                                       int tag,
                                                       MPIR_Comm * comm,
                                                       int context_offset,
                                                       int *flag, MPIR_Request ** message,
                                                       MPI_Status * status)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IMPROBE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IMPROBE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iprobe(int source,
                                                      int tag,
                                                      MPIR_Comm * comm,
                                                      int context_offset, int *flag,
                                                      MPI_Status * status)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IPROBE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IPROBE);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_PROBE_H_INCLUDED */
