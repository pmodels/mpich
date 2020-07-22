/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

int MPIDI_UCX_progress(int vci, int blocking)
{
    int mpi_errno = MPI_SUCCESS;

    int vni = MPIDI_UCX_vci_to_vni(vci);
    if (vni < 0) {
        /* skip if the vci is not for us */
        return MPI_SUCCESS;
    }

    ucp_worker_progress(MPIDI_UCX_global.ctx[vni].worker);
    MPIDI_UCX_am_progress(vni);

    return mpi_errno;
}
