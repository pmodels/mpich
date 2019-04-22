/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_C11_H_INCLUDED
#define OPA_C11_H_INCLUDED

#include <stdint.h>
#include <stdatomic.h>

typedef struct {
    atomic_int v;
} OPA_int_t;
typedef struct {
    atomic_intptr_t v;
} OPA_ptr_t;

#if __STDC_VERSION__ >= 201710L
// C17 obsoletes ATOMIC_VAR_INIT.
#define OPA_INT_T_INITIALIZER(val_) { (val_) }
#define OPA_PTR_T_INITIALIZER(val_) { (val_) }
#else
#define OPA_INT_T_INITIALIZER(val_) { ATOMIC_VAR_INIT(val_) }
#define OPA_PTR_T_INITIALIZER(val_) { ATOMIC_VAR_INIT(val_) }
#endif

static _opa_inline int OPA_load_int(_opa_const OPA_int_t * ptr)
{
    return atomic_load_explicit(&ptr->v, memory_order_relaxed);
}

static _opa_inline void OPA_store_int(OPA_int_t * ptr, int val)
{
    atomic_store_explicit(&ptr->v, val, memory_order_relaxed);
}

static _opa_inline void *OPA_load_ptr(_opa_const OPA_ptr_t * ptr)
{
    return (void *)atomic_load_explicit(&ptr->v, memory_order_relaxed);
}

static _opa_inline void OPA_store_ptr(OPA_ptr_t * ptr, void *val)
{
    atomic_store_explicit(&ptr->v, (intptr_t)val, memory_order_relaxed);
}

static _opa_inline int OPA_load_acquire_int(_opa_const OPA_int_t * ptr)
{
    return atomic_load_explicit(&ptr->v, memory_order_acquire);
}

static _opa_inline void OPA_store_release_int(OPA_int_t * ptr, int val)
{
    atomic_store_explicit(&ptr->v, (intptr_t)val, memory_order_release);
}

static _opa_inline void *OPA_load_acquire_ptr(_opa_const OPA_ptr_t * ptr)
{
    return (void *)atomic_load_explicit(&ptr->v, memory_order_acquire);
}

static _opa_inline void OPA_store_release_ptr(OPA_ptr_t * ptr, void *val)
{
    atomic_store_explicit(&ptr->v, (intptr_t)val, memory_order_release);
}

static _opa_inline int OPA_fetch_and_add_int(OPA_int_t * ptr, int val)
{
    return atomic_fetch_add_explicit(&ptr->v, val, memory_order_acq_rel);
}

static _opa_inline int OPA_decr_and_test_int(OPA_int_t * ptr)
{
    return atomic_fetch_sub_explicit(&ptr->v, 1, memory_order_acq_rel) == 1;
}

#define OPA_fetch_and_incr_int_by_faa OPA_fetch_and_incr_int
#define OPA_fetch_and_decr_int_by_faa OPA_fetch_and_decr_int
#define OPA_add_int_by_faa OPA_add_int
#define OPA_incr_int_by_fai OPA_incr_int
#define OPA_decr_int_by_fad OPA_decr_int

static _opa_inline void *OPA_cas_ptr(OPA_ptr_t * ptr, void *oldv, void *newv)
{
    atomic_compare_exchange_strong_explicit(&ptr->v, (intptr_t *)&oldv,
                                            (intptr_t)newv,
                                            memory_order_acq_rel,
                                            memory_order_acquire);
    return oldv;
}

static _opa_inline int OPA_cas_int(OPA_int_t * ptr, int oldv, int newv)
{
    atomic_compare_exchange_strong_explicit(&ptr->v, &oldv, newv,
                                            memory_order_acq_rel,
                                            memory_order_acquire);
    return oldv;
}

static _opa_inline void *OPA_swap_ptr(OPA_ptr_t * ptr, void *val)
{
    return (void *)atomic_exchange_explicit(&ptr->v, (intptr_t)val,
                                            memory_order_acq_rel);
}

static _opa_inline int OPA_swap_int(OPA_int_t * ptr, int val)
{
    return atomic_exchange_explicit(&ptr->v, val, memory_order_acq_rel);
}

#define OPA_write_barrier()      atomic_thread_fence(memory_order_release)
#define OPA_read_barrier()       atomic_thread_fence(memory_order_acquire)
#define OPA_read_write_barrier() atomic_thread_fence(memory_order_acq_rel)
/* atomic_signal_fence performs a compiler barrier without any overhead */
#define OPA_compiler_barrier()   atomic_signal_fence(memory_order_acq_rel)

#include"opa_emulated.h"

#endif /* OPA_C11_H_INCLUDED */
