/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "lock/zm_mcs.h"

int zm_mcs_init(zm_mcs_t *L)
{
    zm_atomic_store(L, (zm_ptr_t)ZM_NULL, zm_memord_release);
    return 0;
}

int zm_mcs_acquire(zm_mcs_t *L, zm_mcs_qnode_t* I) {
    zm_atomic_store(&I->next, ZM_NULL, zm_memord_release);
    zm_mcs_qnode_t* pred = (zm_mcs_qnode_t*)zm_atomic_exchange(L, (zm_ptr_t)I, zm_memord_acq_rel);
    if((zm_ptr_t)pred != ZM_NULL) {
        zm_atomic_store(&I->status, ZM_LOCKED, zm_memord_release);
        zm_atomic_store(&pred->next, (zm_ptr_t)I, zm_memord_release);
        while(zm_atomic_load(&I->status, zm_memord_acquire) != ZM_UNLOCKED)
            ; /* SPIN */
    }
    return 0;
}

/* Release the lock */
int zm_mcs_release(zm_mcs_t *L, zm_mcs_qnode_t *I) {
    if (zm_atomic_load(&I->next, zm_memord_acquire) == ZM_NULL) {
        zm_mcs_qnode_t *tmp = I;
        if(zm_atomic_compare_exchange_weak(L,
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
