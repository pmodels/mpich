/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PROGRESS_H_INCLUDED
#define UCX_PROGRESS_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int blocking)
{
    int mpi_errno = MPI_SUCCESS;

    int vni = MPIDI_UCX_vci_to_vni(vci);

    MPIR_FUNC_ENTER;

    if (vni < 0) {
        /* skip if the vci is not for us */
        return MPI_SUCCESS;
    }

    ucp_worker_progress(MPIDI_UCX_global.ctx[vni].worker);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* UCX_PROGRESS_H_INCLUDED */
