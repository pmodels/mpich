/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSU_ATOMICS_H_INCLUDED
#define YAKSU_ATOMICS_H_INCLUDED

#include <yaksa_config.h>

#ifdef HAVE_C11_ATOMICS

#include <stdatomic.h>

#define YAKSU_ATOMIC_VAR_INIT ATOMIC_VAR_INIT
typedef atomic_int yaksu_atomic_int;

static inline int yaksu_atomic_incr(yaksu_atomic_int * val)
{
    return atomic_fetch_add(val, 1);
}

static inline int yaksu_atomic_decr(yaksu_atomic_int * val)
{
    return atomic_fetch_sub(val, 1);
}

static inline int yaksu_atomic_load(yaksu_atomic_int * val)
{
    return atomic_load_explicit(val, memory_order_acquire);
}

static inline void yaksu_atomic_store(yaksu_atomic_int * val, int x)
{
    atomic_store_explicit(val, x, memory_order_release);
}

#else

#include <pthread.h>

#define YAKSU_ATOMIC_VAR_INIT(x) x
extern pthread_mutex_t yaksui_atomic_mutex;
typedef int yaksu_atomic_int;

static inline int yaksu_atomic_incr(yaksu_atomic_int * val)
{
    pthread_mutex_lock(&yaksui_atomic_mutex);
    int ret = (*val)++;
    pthread_mutex_unlock(&yaksui_atomic_mutex);

    return ret;
}

static inline int yaksu_atomic_decr(yaksu_atomic_int * val)
{
    pthread_mutex_lock(&yaksui_atomic_mutex);
    int ret = (*val)--;
    pthread_mutex_unlock(&yaksui_atomic_mutex);

    return ret;
}

static inline int yaksu_atomic_load(yaksu_atomic_int * val)
{
    pthread_mutex_lock(&yaksui_atomic_mutex);
    int ret = (*val);
    pthread_mutex_unlock(&yaksui_atomic_mutex);

    return ret;
}

static inline void yaksu_atomic_store(yaksu_atomic_int * val, int x)
{
    pthread_mutex_lock(&yaksui_atomic_mutex);
    *val = x;
    pthread_mutex_unlock(&yaksui_atomic_mutex);
}

#endif

#endif /* YAKSU_THREADS_H_INCLUDED */
