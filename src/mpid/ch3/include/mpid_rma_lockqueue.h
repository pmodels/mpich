/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_LOCKQUEUE_H_INCLUDED)
#define MPID_RMA_LOCKQUEUE_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_lockqueue_alloc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_winlock_getlocallock);

/* MPIDI_CH3I_Win_lock_entry_alloc(): return a new lock queue entry and
 * initialize it. If we cannot get one, return NULL. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_lock_entry_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_lock_entry_alloc(MPID_Win * win_ptr,
                                                  MPIDI_CH3_Pkt_t *pkt,
                                                  MPIDI_Win_lock_queue **lock_entry)
{
    MPIDI_Win_lock_queue *new_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* FIXME: we should use a lock entry queue to manage all this. */

    /* allocate lock queue entry */
    MPIR_T_PVAR_TIMER_START(RMA, rma_lockqueue_alloc);
    new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
    MPIR_T_PVAR_TIMER_END(RMA, rma_lockqueue_alloc);
    if (!new_ptr) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIDI_Win_lock_queue");
    }

    new_ptr->next = NULL;
    new_ptr->pkt = (*pkt);
    new_ptr->data = NULL;
    new_ptr->all_data_recved = 0;

    (*lock_entry) = new_ptr;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif  /* MPID_RMA_ISSUE_H_INCLUDED */
