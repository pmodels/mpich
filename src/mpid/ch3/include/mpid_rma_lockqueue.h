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
static inline MPIDI_RMA_Lock_entry_t *MPIDI_CH3I_Win_lock_entry_alloc(MPID_Win * win_ptr,
                                                                      MPIDI_CH3_Pkt_t *pkt)
{
    MPIDI_RMA_Lock_entry_t *new_ptr = NULL;

    if (win_ptr->lock_entry_pool != NULL) {
        new_ptr = win_ptr->lock_entry_pool;
        MPL_LL_DELETE(win_ptr->lock_entry_pool, win_ptr->lock_entry_pool_tail, new_ptr);
    }

    if (new_ptr != NULL) {
        new_ptr->next = NULL;
        MPIU_Memcpy(&(new_ptr->pkt), pkt, sizeof(*pkt));
        new_ptr->vc = NULL;
        new_ptr->data = NULL;
        new_ptr->data_size = 0;
        new_ptr->all_data_recved = 0;
    }

    return new_ptr;
}

/* MPIDI_CH3I_Win_lock_entry_free(): put a lock queue entry back to
 * the global pool. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_lock_entry_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_lock_entry_free(MPID_Win * win_ptr,
                                                 MPIDI_RMA_Lock_entry_t *lock_entry)
{
    int mpi_errno = MPI_SUCCESS;

    if (lock_entry->data != NULL) {
        win_ptr->current_lock_data_bytes -= lock_entry->data_size;
        MPIU_Free(lock_entry->data);
    }

    /* use PREPEND when return objects back to the pool
       in order to improve cache performance */
    MPL_LL_PREPEND(win_ptr->lock_entry_pool, win_ptr->lock_entry_pool_tail, lock_entry);

    return mpi_errno;
}

#endif  /* MPID_RMA_ISSUE_H_INCLUDED */
