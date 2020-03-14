/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* FIXME needs to be converted to new style functions with OPA_int_t/OPA_ptr_t-types */
#ifndef OPA_GCC_ATOMIC_H_INCLUDED
#define OPA_GCC_ATOMIC_H_INCLUDED

/* FIXME do we need to align these? */
typedef struct {
    volatile int v;
} OPA_int_t;
typedef struct {
    void *volatile v;
} OPA_ptr_t;

#define OPA_INT_T_INITIALIZER(val_) { (val_) }
#define OPA_PTR_T_INITIALIZER(val_) { (val_) }

/* Oracle Developer Studio (suncc) supports gcc sync intrinsics, but it is very noisy.
 * Suppress the error_messages here and reset at the end of this file.
 */
#ifdef __SUNPRO_C
#pragma error_messages (off, E_ARG_INCOMPATIBLE_WITH_ARG_L)
#endif
/* Intel compiler won't accept the extra "protected" var list, suppress by pragma */
#ifdef __ICC
#pragma warning disable 2206
#endif

/* Assume that loads/stores are atomic on the current platform, even though this
   may not be true at all. */
static inline int OPA_load_int(const OPA_int_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_RELAXED);
}

static inline void OPA_store_int(OPA_int_t * ptr, int val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELAXED);
}

static inline void *OPA_load_ptr(const OPA_ptr_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_RELAXED);
}

static inline void OPA_store_ptr(OPA_ptr_t * ptr, void *val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELAXED);
}

static inline int OPA_load_acquire_int(const OPA_int_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_ACQUIRE);
}

static inline void OPA_store_release_int(OPA_int_t * ptr, int val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELEASE);
}

static inline void *OPA_load_acquire_ptr(const OPA_ptr_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_ACQUIRE);
}

static inline void OPA_store_release_ptr(OPA_ptr_t * ptr, void *val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELEASE);
}


/* gcc atomic intrinsics accept an optional list of variables to be
   protected by a memory barrier.  These variables are labeled
   below by "protected variables :". */

static inline int OPA_fetch_and_add_int(OPA_int_t * ptr, int val)
{
    return __atomic_fetch_add(&ptr->v, val, __ATOMIC_SEQ_CST);
}

static inline int OPA_decr_and_test_int(OPA_int_t * ptr)
{
    return __atomic_sub_fetch(&ptr->v, 1, __ATOMIC_SEQ_CST) == 0;
}

#define OPA_fetch_and_incr_int_by_faa OPA_fetch_and_incr_int
#define OPA_fetch_and_decr_int_by_faa OPA_fetch_and_decr_int
#define OPA_add_int_by_faa OPA_add_int
#define OPA_incr_int_by_fai OPA_incr_int
#define OPA_decr_int_by_fad OPA_decr_int


static inline void *OPA_cas_ptr(OPA_ptr_t * ptr, void *oldv, void *newv)
{
    void *expected = oldv;
    __atomic_compare_exchange_n(&ptr->v, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}

static inline int OPA_cas_int(OPA_int_t * ptr, int oldv, int newv)
{
    int expected = oldv;
    __atomic_compare_exchange_n(&ptr->v, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}

static inline void *OPA_swap_ptr(OPA_ptr_t * ptr, void *val)
{
    return __atomic_exchange_n(&ptr->v, val, __ATOMIC_SEQ_CST);
}

static inline int OPA_swap_int(OPA_int_t * ptr, int val)
{
    return __atomic_exchange_n(&ptr->v, val, __ATOMIC_SEQ_CST);
}

#define OPA_write_barrier()      __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define OPA_read_barrier()       __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define OPA_read_write_barrier() __atomic_thread_fence(__ATOMIC_SEQ_CST)

#define OPA_compiler_barrier()   __asm__ __volatile__  (""  ::: "memory")

#ifdef __SUNPRO_C
#pragma error_messages (default, E_ARG_INCOMPATIBLE_WITH_ARG_L)
#endif
#ifdef __ICC
#pragma warning enable 2206
#endif

#include"opa_emulated.h"

#endif /* OPA_GCC_ATOMIC_H_INCLUDED */
