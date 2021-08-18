/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidrma.h"


int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t * win_fns)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* Sock doesn't override any of the default Window functions */

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDI_CH3_Win_hooks_init(MPIDI_CH3U_Win_hooks_t * win_hooks)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* Sock doesn't implement any of the Window hooks */

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDI_CH3_Win_pkt_orderings_init(MPIDI_CH3U_Win_pkt_ordering_t * win_pkt_orderings)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* Guarantees ordered AM flush. */
    win_pkt_orderings->am_flush_ordered = 1;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
