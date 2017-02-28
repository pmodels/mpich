/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "lock/zm_csvmcs.h"

int zm_csvmcs_init(zm_csvmcs_t *L)
{
    zm_atomic_store(&L->lock, ZM_NULL, zm_memord_release);
    L->cur_ctx = (zm_mcs_qnode_t*)ZM_NULL;
    return 0;
}

int zm_csvmcs_acquire(zm_csvmcs_t *L, zm_mcs_qnode_t* I) {
    if((zm_ptr_t)I == ZM_NULL)
        I = L->cur_ctx;
    zm_atomic_store(&I->next, ZM_NULL, zm_memord_release);
    zm_mcs_qnode_t* pred = (zm_mcs_qnode_t*)zm_atomic_exchange(&L->lock, (zm_ptr_t)I, zm_memord_acq_rel);
    if((zm_ptr_t)pred != ZM_NULL) {
        zm_atomic_store(&I->status, ZM_LOCKED, zm_memord_release);
        zm_atomic_store(&pred->next, (zm_ptr_t)I, zm_memord_release);
        while(zm_atomic_load(&I->status, zm_memord_acquire) != ZM_UNLOCKED)
            ; /* SPIN */
    }
    L->cur_ctx = I; /* save current local context*/
    return 0;
}

/* Release the lock */
int zm_csvmcs_release(zm_csvmcs_t *L) {
    zm_mcs_qnode_t* I = L->cur_ctx; /* get current local context */
    L->cur_ctx = (zm_mcs_qnode_t*)ZM_NULL;
    if (zm_atomic_load(&I->next, zm_memord_acquire) == ZM_NULL) {
        zm_mcs_qnode_t *tmp = I;
        if(zm_atomic_compare_exchange_weak(&L->lock,
                                          (zm_ptr_t*)&tmp,
                                          ZM_NULL,
                                          zm_memord_release,
                                          zm_memord_acquire))
            return 0;
        while(zm_atomic_load(&I->next, zm_memord_acquire) == ZM_NULL)
            ; /* SPIN */
    }
    zm_atomic_store(&((zm_mcs_qnode_t*)zm_atomic_load(&I->next, zm_memord_acquire))->status, ZM_UNLOCKED, zm_memord_release);
    return 0;
}
