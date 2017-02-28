/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "lock/zm_ticket.h"

int zm_ticket_init(zm_ticket_t *lock)
{
    zm_atomic_store(&lock->next_ticket, 0, zm_memord_release);
    zm_atomic_store(&lock->now_serving, 0, zm_memord_release);
    return 0;
}

/* Atomically increment the nex_ticket counter and get my ticket.
   Then spin on now_serving until it equals my ticket. */
int zm_ticket_acquire(zm_ticket_t* lock) {
    unsigned my_ticket = zm_atomic_fetch_add(&lock->next_ticket, 1, zm_memord_acq_rel);
    while(zm_atomic_load(&lock->now_serving, zm_memord_acquire) != my_ticket)
            ; /* SPIN */
    return 0;
}

/* Release the lock */
int zm_ticket_release(zm_ticket_t* lock) {
    zm_atomic_fetch_add(&lock->now_serving, 1, zm_memord_release);
    return 0;
}

