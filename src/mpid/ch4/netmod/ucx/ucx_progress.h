/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PROGRESS_H_INCLUDED
#define UCX_PROGRESS_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Assert(vci < MPIDI_UCX_global.num_vcis);
    if (vci < 0) {
        /* skip if the vci is not for us */
        return MPI_SUCCESS;
    }

    ucp_worker_progress(MPIDI_UCX_global.ctx[vci].worker);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* UCX_PROGRESS_H_INCLUDED */
