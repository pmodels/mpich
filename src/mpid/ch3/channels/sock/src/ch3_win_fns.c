/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidrma.h"


int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t * win_fns)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    /* Sock doesn't override any of the default Window functions */

    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    return mpi_errno;
}

int MPIDI_CH3_Win_hooks_init(MPIDI_CH3U_Win_hooks_t * win_hooks)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_HOOKS_INIT);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3_WIN_HOOKS_INIT);

    /* Sock doesn't implement any of the Window hooks */

    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3_WIN_HOOKS_INIT);

    return mpi_errno;
}

int MPIDI_CH3_Win_pkt_orderings_init(MPIDI_CH3U_Win_pkt_ordering_t * win_pkt_orderings)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_PKT_ORDERINGS_INIT);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3_WIN_PKT_ORDERINGS_INIT);

    /* Guarantees ordered AM flush. */
    win_pkt_orderings->am_flush_ordered = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_WIN_PKT_ORDERINGS_INIT);
    return mpi_errno;
}
