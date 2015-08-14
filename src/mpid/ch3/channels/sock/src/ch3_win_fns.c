/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_fns_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t *win_fns)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    /* Sock doesn't override any of the default Window functions */

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_FNS_INIT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_hooks_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_hooks_init(MPIDI_CH3U_Win_hooks_t *win_hooks)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_HOOKS_INIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_HOOKS_INIT);

    /* Sock doesn't implement any of the Window hooks */

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_HOOKS_INIT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_pkt_orderings_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_pkt_orderings_init(MPIDI_CH3U_Win_pkt_ordering_t * win_pkt_orderings)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_PKT_ORDERINGS_INIT);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_PKT_ORDERINGS_INIT);

    /* Guarantees ordered AM flush. */
    win_pkt_orderings->am_flush_ordered = 1;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_PKT_ORDERINGS_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
