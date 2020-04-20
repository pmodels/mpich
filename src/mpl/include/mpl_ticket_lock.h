/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * Ticket lock based on MPL atomics
 */

#ifndef MPL_TICKET_LOCK_H_INCLUDED
#define MPL_TICKET_LOCK_H_INCLUDED

#include "mpl_atomic.h"
#include "mpl_yield.h"

typedef struct {
    MPL_atomic_int_t num;
    MPL_atomic_int_t next;
} MPL_ticket_lock;

#define MPL_ticket_lock_init(lock_) \
    do { \
        MPL_atomic_store_int(&(lock_)->num, 0); \
        MPL_atomic_store_int(&(lock_)->next, 0); \
    } while (0)

#define MPL_ticket_lock_lock(lock_) \
    do { \
        int ticket = MPL_atomic_fetch_add_int(&(lock_)->num, 1); \
        while (MPL_atomic_load_int(&(lock_)->next) != ticket) { \
            MPL_sched_yield(); \
        } \
    } while (0)

#define MPL_ticket_lock_unlock(lock_) \
    do { \
        MPL_atomic_fetch_add_int(&(lock_)->next, 1); \
    } while (0)

#endif /* MPL_TICKET_LOCK_H_INCLUDED */
