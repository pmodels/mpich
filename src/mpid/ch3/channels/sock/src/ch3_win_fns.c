/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t * win_fns)
{
    int mpi_errno = MPI_SUCCESS;

    /* Sock doesn't override any of the default Window functions */

    return mpi_errno;
}

int MPIDI_CH3_Win_hooks_init(MPIDI_CH3U_Win_hooks_t * win_hooks)
{
    int mpi_errno = MPI_SUCCESS;

    /* Sock doesn't implement any of the Window hooks */

    return mpi_errno;
}

int MPIDI_CH3_Win_pkt_orderings_init(MPIDI_CH3U_Win_pkt_ordering_t * win_pkt_orderings)
{
    int mpi_errno = MPI_SUCCESS;

    /* Guarantees ordered AM flush. */
    win_pkt_orderings->am_flush_ordered = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
