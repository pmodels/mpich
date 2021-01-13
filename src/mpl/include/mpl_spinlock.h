/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_SPINLOCK_H_INCLUDED
#define MPL_SPINLOCK_H_INCLUDED

#include <assert.h>
#include "mplconfig.h"
#include "mpl_atomic.h"

#ifndef MPL_USE_NO_ATOMIC_PRIMITIVES

/* Let's use atomic-based spinlock. */
typedef struct MPL_spinlock_t {
    MPL_atomic_int_t val;
} MPL_spinlock_t;

#define MPL_SPINLOCK_INITIALIZER { MPL_ATOMIC_INT_T_INITIALIZER(0) }

static void MPL_spinlock_lock(MPL_spinlock_t * lock)
{
    while (MPL_atomic_cas_int(&lock->val, 0, 1));
}

static int MPL_spinlock_trylock(MPL_spinlock_t * lock)
{
    /* Return 0 if the lock is successfully taken.  Otherwise, return non-zero.
     * This trylock is strong since MPL_atomic_cas_int is atomically strong. */
    return MPL_atomic_cas_int(&lock->val, 0, 1);
}

static void MPL_spinlock_unlock(MPL_spinlock_t * lock)
{
    assert(MPL_atomic_relaxed_load_int(&lock->val) == 1);
    MPL_atomic_release_store_int(&lock->val, 0);
}

#elif defined(MPL_HAVE_PTHREAD_H)

/* If pthread can be used, let's use pthread_mutex. Note that we cannot use
 * pthread's spinlock because it does not have a static initializer. */
#include <pthread.h>

#define MPL_spinlock_t pthread_mutex_t

#define MPL_SPINLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static void MPL_spinlock_lock(MPL_spinlock_t * lock)
{
    int ret = pthread_mutex_lock(lock);
    assert(ret == 0);
}

static int MPL_spinlock_trylock(MPL_spinlock_t * lock)
{
    int ret = pthread_mutex_trylock(lock);
    return ret == 0 ? 0 : 1;
}

static void MPL_spinlock_unlock(MPL_spinlock_t * lock)
{
    int ret = pthread_mutex_unlock(lock);
    assert(ret == 0);
}

#else /* MPL_HAVE_PTHREAD_H */

#error "Neither atomic primitives nor POSIX thread is supported."

#endif /* !MPL_HAVE_PTHREAD_H */

#endif /* MPL_SPINLOCK_H_INCLUDED */
