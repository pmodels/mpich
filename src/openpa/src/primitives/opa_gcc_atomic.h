/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_GCC_ATOMIC_H_INCLUDED
#define OPA_GCC_ATOMIC_H_INCLUDED

typedef struct {
    volatile int v;
} OPA_int_t;
typedef struct {
    void *volatile v;
} OPA_ptr_t;

#define OPA_INT_T_INITIALIZER(val_) { (val_) }
#define OPA_PTR_T_INITIALIZER(val_) { (val_) }

static _opa_inline int OPA_load_int(_opa_const OPA_int_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_RELAXED);
}

static _opa_inline void OPA_store_int(OPA_int_t * ptr, int val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELAXED);
}

static _opa_inline void *OPA_load_ptr(_opa_const OPA_ptr_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_RELAXED);
}

static _opa_inline void OPA_store_ptr(OPA_ptr_t * ptr, void *val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELAXED);
}

static _opa_inline int OPA_load_acquire_int(_opa_const OPA_int_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_ACQUIRE);
}

static _opa_inline void OPA_store_release_int(OPA_int_t * ptr, int val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELEASE);
}

static _opa_inline void *OPA_load_acquire_ptr(_opa_const OPA_ptr_t * ptr)
{
    return __atomic_load_n(&ptr->v, __ATOMIC_ACQUIRE);
}

static _opa_inline void OPA_store_release_ptr(OPA_ptr_t * ptr, void *val)
{
    __atomic_store_n(&ptr->v, val, __ATOMIC_RELEASE);
}

static _opa_inline int OPA_fetch_and_add_int(OPA_int_t * ptr, int val)
{
    return __atomic_fetch_add(&ptr->v, val, __ATOMIC_ACQ_REL);
}

static _opa_inline int OPA_decr_and_test_int(OPA_int_t * ptr)
{
    return __atomic_sub_fetch(&ptr->v, 1, __ATOMIC_ACQ_REL) == 0;
}

#define OPA_fetch_and_incr_int_by_faa OPA_fetch_and_incr_int
#define OPA_fetch_and_decr_int_by_faa OPA_fetch_and_decr_int
#define OPA_add_int_by_faa OPA_add_int
#define OPA_incr_int_by_fai OPA_incr_int
#define OPA_decr_int_by_fad OPA_decr_int

static _opa_inline void *OPA_cas_ptr(OPA_ptr_t * ptr, void *oldv, void *newv)
{
    __atomic_compare_exchange_n(&ptr->v, &oldv, newv, 0, __ATOMIC_ACQ_REL,
                                __ATOMIC_ACQUIRE);
    return oldv;
}

static _opa_inline int OPA_cas_int(OPA_int_t * ptr, int oldv, int newv)
{
    __atomic_compare_exchange_n(&ptr->v, &oldv, newv, 0, __ATOMIC_ACQ_REL,
                                __ATOMIC_ACQUIRE);
    return oldv;
}

static _opa_inline void *OPA_swap_ptr(OPA_ptr_t * ptr, void *val)
{
    return __atomic_exchange_n(&ptr->v, val, __ATOMIC_ACQ_REL);
}

static _opa_inline int OPA_swap_int(OPA_int_t * ptr, int val)
{
    return __atomic_exchange_n(&ptr->v, val, __ATOMIC_ACQ_REL);
}

#define OPA_write_barrier()      __atomic_thread_fence(__ATOMIC_RELEASE)
#define OPA_read_barrier()       __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define OPA_read_write_barrier() __atomic_thread_fence(__ATOMIC_ACQ_REL)
/* __atomic_signal_fence performs a compiler barrier without any overhead */
#define OPA_compiler_barrier()   __atomic_signal_fence(__ATOMIC_ACQ_REL)

#include"opa_emulated.h"

#endif /* OPA_GCC_ATOMIC_H_INCLUDED */
