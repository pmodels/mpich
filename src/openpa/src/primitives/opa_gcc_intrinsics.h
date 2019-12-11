/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME needs to be converted to new style functions with OPA_int_t/OPA_ptr_t-types */
#ifndef OPA_GCC_INTRINSICS_H_INCLUDED
#define OPA_GCC_INTRINSICS_H_INCLUDED

/* FIXME do we need to align these? */
typedef struct {
    volatile int v;
} OPA_int_t;
typedef struct {
    void *volatile v;
} OPA_ptr_t;

#define OPA_INT_T_INITIALIZER(val_) { (val_) }
#define OPA_PTR_T_INITIALIZER(val_) { (val_) }

/* Use __atomic builtins if GCC >= 4.7.4 */
#if __GNUC__ > 4 || \
    (__GNUC__ == 4 && (__GNUC_MINOR__ > 7 || \
                       (__GNUC_MINOR__ == 7 && \
                        __GNUC_PATCHLEVEL__ >= 4)))
#define USE_GCC_ATOMIC
#endif

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
#ifdef USE_GCC_ATOMIC
    return __atomic_load_n(&ptr->v, __ATOMIC_RELAXED);
#else
    return ptr->v;
#endif
}

static inline void OPA_store_int(OPA_int_t * ptr, int val)
{
#ifdef USE_GCC_ATOMIC
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELAXED);
#else
    ptr->v = val;
#endif
}

static inline void *OPA_load_ptr(const OPA_ptr_t * ptr)
{
#ifdef USE_GCC_ATOMIC
    return __atomic_load_n(&ptr->v, __ATOMIC_RELAXED);
#else
    return ptr->v;
#endif
}

static inline void OPA_store_ptr(OPA_ptr_t * ptr, void *val)
{
#ifdef USE_GCC_ATOMIC
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELAXED);
#else
    ptr->v = val;
#endif
}

static inline int OPA_load_acquire_int(const OPA_int_t * ptr)
{
#ifdef USE_GCC_ATOMIC
    return __atomic_load_n(&ptr->v, __ATOMIC_ACQUIRE);
#else
    int tmp = ptr->v;
    /* __sync does not have a true load-acquire barrier builtin,
     * so the best that can be done is use a normal barrier to
     * ensure the read of ptr->v happens before the ensuing block
     * of memory references we are ordering.
     */
    __sync_synchronize();
    return tmp;
#endif
}

static inline void OPA_store_release_int(OPA_int_t * ptr, int val)
{
#ifdef USE_GCC_ATOMIC
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELEASE);
#else
    /* __sync does not a true store-release barrier builtin,
     * so the best that can be done is to use a normal barrier
     * to ensure previous memory operations are ordered-before
     * the following store.
     */
    __sync_synchronize();
    ptr->v = val;
#endif
}

static inline void *OPA_load_acquire_ptr(const OPA_ptr_t * ptr)
{
#ifdef USE_GCC_ATOMIC
    return __atomic_load_n(&ptr->v, __ATOMIC_ACQUIRE);
#else
    void *tmp = ptr->v;
    __sync_synchronize();
    return tmp;
#endif
}

static inline void OPA_store_release_ptr(OPA_ptr_t * ptr, void *val)
{
#ifdef USE_GCC_ATOMIC
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELEASE);
#else
    __sync_synchronize();
    ptr->v = val;
#endif
}


/* gcc atomic intrinsics accept an optional list of variables to be
   protected by a memory barrier.  These variables are labeled
   below by "protected variables :". */

static inline int OPA_fetch_and_add_int(OPA_int_t * ptr, int val)
{
#ifdef USE_GCC_ATOMIC
    return __atomic_fetch_add(&ptr->v, val, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_add(&ptr->v, val, /* protected variables: */ &ptr->v);
#endif
}

static inline int OPA_decr_and_test_int(OPA_int_t * ptr)
{
#ifdef USE_GCC_ATOMIC
    return __atomic_sub_fetch(&ptr->v, 1, __ATOMIC_SEQ_CST) == 0;
#else
    return __sync_sub_and_fetch(&ptr->v, 1, /* protected variables: */ &ptr->v) == 0;
#endif
}

#define OPA_fetch_and_incr_int_by_faa OPA_fetch_and_incr_int
#define OPA_fetch_and_decr_int_by_faa OPA_fetch_and_decr_int
#define OPA_add_int_by_faa OPA_add_int
#define OPA_incr_int_by_fai OPA_incr_int
#define OPA_decr_int_by_fad OPA_decr_int


static inline void *OPA_cas_ptr(OPA_ptr_t * ptr, void *oldv, void *newv)
{

#ifdef USE_GCC_ATOMIC
    void *expected = oldv;
    __atomic_compare_exchange_n(&ptr->v, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
#else
    return __sync_val_compare_and_swap(&ptr->v, oldv, newv, /* protected variables: */ &ptr->v);
#endif
}

static inline int OPA_cas_int(OPA_int_t * ptr, int oldv, int newv)
{

#ifdef USE_GCC_ATOMIC
    int expected = oldv;
    __atomic_compare_exchange_n(&ptr->v, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
#else
    return __sync_val_compare_and_swap(&ptr->v, oldv, newv, /* protected variables: */ &ptr->v);
#endif
}



#ifdef USE_GCC_ATOMIC
static inline void *OPA_swap_ptr(OPA_ptr_t * ptr, void *val)
{
    return __atomic_exchange_n(&ptr->v, val, __ATOMIC_SEQ_CST);
}

static inline int OPA_swap_int(OPA_int_t * ptr, int val)
{
    return __atomic_exchange_n(&ptr->v, val, __ATOMIC_SEQ_CST);
}
#else
#ifdef SYNC_LOCK_TEST_AND_SET_IS_SWAP
static inline void *OPA_swap_ptr(OPA_ptr_t * ptr, void *val)
{
    return __sync_lock_test_and_set(&ptr->v, val, /* protected variables: */ &ptr->v);
}

static inline int OPA_swap_int(OPA_int_t * ptr, int val)
{
    return __sync_lock_test_and_set(&ptr->v, val, /* protected variables: */ &ptr->v);
}
#else
#define OPA_swap_ptr_by_cas OPA_swap_ptr
#define OPA_swap_int_by_cas OPA_swap_int
#endif /* SYNC_LOCK_TEST_AND_SET_IS_SWAP */
#endif /* USE_GCC_ATOMIC */

#ifdef USE_GCC_ATOMIC
#define OPA_write_barrier()      __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define OPA_read_barrier()       __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define OPA_read_write_barrier() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#else
#define OPA_write_barrier()      __sync_synchronize()
#define OPA_read_barrier()       __sync_synchronize()
#define OPA_read_write_barrier() __sync_synchronize()
#endif

#define OPA_compiler_barrier()   __asm__ __volatile__  (""  ::: "memory")

#ifdef __SUNPRO_C
#pragma error_messages (default, E_ARG_INCOMPATIBLE_WITH_ARG_L)
#endif
#ifdef __ICC
#pragma warning enable 2206
#endif

#include"opa_emulated.h"

#endif /* OPA_GCC_INTRINSICS_H_INCLUDED */
