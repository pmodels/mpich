/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED
#define MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED

#include<intrin.h>
#include "mpi.h"

typedef struct { volatile int v;  } MPIDU_Atomic_t;
typedef struct { int * volatile v; } MPIDU_Atomic_ptr_t;

static inline int MPIDU_Atomic_load(MPIDU_Atomic_t *ptr)
{
    return ptr->v;
}

static inline void MPIDU_Atomic_store(MPIDU_Atomic_t *ptr, int val)
{
    ptr->v = val;
}

static inline void *MPIDU_Atomic_load_ptr(MPIDU_Atomic_ptr_t *ptr)
{
    return ptr->v;
}

static inline void MPIDU_Atomic_store_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    ptr->v = val;
}

static inline void MPIDU_Atomic_add(MPIDU_Atomic_t *ptr, int val)
{
    _InterlockedExchangeAdd(&(ptr->v), val);
}

static inline void MPIDU_Atomic_incr(MPIDU_Atomic_t *ptr)
{
    _InterlockedIncrement(&(ptr->v));
}

static inline void MPIDU_Atomic_decr(MPIDU_Atomic_t *ptr)
{
    _InterlockedDecrement(&(ptr->v));
}

static inline int MPIDU_Atomic_decr_and_test(MPIDU_Atomic_t *ptr)
{
    return (_InterlockedDecrement(&(ptr->v)) == 0);
}

static inline int MPIDU_Atomic_fetch_and_add(MPIDU_Atomic_t *ptr, int val)
{
    return _InterlockedExchangeAdd(&(ptr->v), val);
}

static inline int *MPIDU_Atomic_cas_ptr(MPIDU_Atomic_ptr_t *ptr, void *oldv, void *newv)
{
#if (SIZEOF_VOID_P == 4)
    return _InterlockedCompareExchange(&(ptr->v), newv, oldv);
#elif (SIZEOF_VOID_P == 8)
    return _InterlockedCompareExchange64(&(ptr->v), newv, oldv);
#else
#error  "SIZEOF_VOID_P not valid"
#endif
}

static inline int MPIDU_Atomic_cas_int(MPIDU_Atomic_t *ptr, int oldv, int newv)
{
    return _InterlockedCompareExchange(&(ptr->v), newv, oldv);
}

static inline int *MPIDU_Atomic_swap_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
#if (SIZEOF_VOID_P == 4)
    return _InterlockedExchange(&(ptr->v), val);
#elif (SIZEOF_VOID_P == 8)
    return _InterlockedExchange64(&(ptr->v), val);
#else
#error  "SIZEOF_VOID_P not valid"
#endif
}

static inline int MPIDU_Atomic_swap_int(MPIDU_Atomic_t *ptr, int val)
{
    return _InterlockedExchange(&(ptr->v), val);
}

/* Implement fetch_and_incr/decr using fetch_and_add (*_faa) */
#define MPIDU_Atomic_fetch_and_incr MPIDU_Atomic_fetch_and_incr_by_faa
#define MPIDU_Atomic_fetch_and_decr MPIDU_Atomic_fetch_and_decr_by_faa
#include "mpidu_atomic_emulated.h"

#endif /* defined(MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED) */
