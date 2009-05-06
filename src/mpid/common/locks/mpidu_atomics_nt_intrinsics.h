/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED
#define MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<intrin.h>
#include "mpi.h"

typedef struct {
    volatile long v;
} MPIDU_Atomic_t;

typedef struct {
    void * volatile v;
}MPIDU_Atomic_ptr_t;

static inline int MPIDU_Atomic_load(MPIDU_Atomic_t *ptr)
{
    return ((int )ptr->v);
}

static inline void MPIDU_Atomic_store(MPIDU_Atomic_t *ptr, int val)
{
    ptr->v = (long )val;
}

static inline void *MPIDU_Atomic_load_ptr(MPIDU_Atomic_ptr_t *ptr)
{
    return ((void *)ptr->v);
}

static inline void MPIDU_Atomic_store_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    ptr->v = val;
}

static inline void MPIDU_Atomic_add(MPIDU_Atomic_t *ptr, int val)
{
    _InterlockedExchangeAdd(&(ptr->v), (long )val);
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
    return ((int )(_InterlockedDecrement(&(ptr->v)) == 0));
}

static inline int MPIDU_Atomic_fetch_and_add(MPIDU_Atomic_t *ptr, int val)
{
    return ((int )_InterlockedExchangeAdd(&(ptr->v), (long )val));
}

static inline int *MPIDU_Atomic_cas_ptr(MPIDU_Atomic_ptr_t *ptr, void *oldv, void *newv)
{
/* InterlockedExchangePointer() is only available > win2k */
#if (SIZEOF_VOID_P == 4)
    return ((int *)(LONG_PTR )_InterlockedCompareExchange((LONG volatile *)&(ptr->v), (LONG )(LONG_PTR )newv, (LONG ) (LONG_PTR )oldv));
#elif (SIZEOF_VOID_P == 8)
    return ((int *)(LONG_PTR )_InterlockedCompareExchange64((INT64 volatile *)&(ptr->v), (INT64 )(LONG_PTR )newv, (INT64 ) (LONG_PTR )oldv));
#else
#error  "SIZEOF_VOID_P not valid"
#endif
}

static inline int MPIDU_Atomic_cas_int(MPIDU_Atomic_t *ptr, int oldv, int newv)
{
    return ((int )_InterlockedCompareExchange(&(ptr->v), (long )newv, (long )oldv));
}

static inline int *MPIDU_Atomic_swap_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
#if (SIZEOF_VOID_P == 4)
    return ((int * )(LONG_PTR )_InterlockedExchange((LONG volatile * )&(ptr->v), (LONG )(LONG_PTR )val));
#elif (SIZEOF_VOID_P == 8)
    return ((int * )(LONG_PTR )_InterlockedExchange64((INT64 volatile * )&(ptr->v), (INT64 )(LONG_PTR )val));
#else
#error  "SIZEOF_VOID_P not valid"
#endif
}

static inline int MPIDU_Atomic_swap_int(MPIDU_Atomic_t *ptr, int val)
{
    return ((int )_InterlockedExchange(&(ptr->v), (long )val));
}

/* Implement fetch_and_incr/decr using fetch_and_add (*_faa) */
#define MPIDU_Atomic_fetch_and_incr MPIDU_Atomic_fetch_and_incr_by_faa
#define MPIDU_Atomic_fetch_and_decr MPIDU_Atomic_fetch_and_decr_by_faa

#define MPIDU_Shm_write_barrier()      _WriteBarrier()
#define MPIDU_Shm_read_barrier()       _ReadBarrier()
#define MPIDU_Shm_read_write_barrier() _ReadWriteBarrier()

#include "mpidu_atomic_emulated.h"

#endif /* defined(MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED) */
