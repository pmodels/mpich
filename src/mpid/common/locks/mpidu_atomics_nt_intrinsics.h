/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED
#define MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED

#include<intrin.h>

static inline void MPIDU_Atomic_add(volatile int *ptr, int val)
{
    _InterlockedExchangeAdd(ptr, val);
}

static inline void MPIDU_Atomic_incr(volatile int *ptr)
{
    _InterlockedIncrement(ptr);
}

static inline void MPIDU_Atomic_decr(volatile int *ptr)
{
    _InterlockedDecrement(ptr);
}

static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr)
{
    return (_InterlockedDecrement(ptr) == 0);
}

static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val)
{
    return _InterlockedExchangeAdd(ptr, val);
}

static inline int *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv)
{
#if (SIZEOF_VOID_P == 4)
    return _InterlockedCompareExchange(ptr, newv, oldv);
#elif (SIZEOF_VOID_P == 8)
    return _InterlockedCompareExchange64(ptr, newv, oldv);
#else
#error  "SIZEOF_VOID_P not valid"
#endif
}

static inline int MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv)
{
    return _InterlockedCompareExchange(ptr, newv, oldv);
}

static inline MPI_Aint MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
#if defined(SIZEOF_INT_IS_AINT)
    return _InterlockedCompareExchange(ptr, newv, oldv);
#else
    return _InterlockedCompareExchange64(ptr, newv, oldv);
#endif
}

static inline int *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val)
{
#if (SIZEOF_VOID_P == 4)
    return _InterlockedExchange(ptr, val);
#elif (SIZEOF_VOID_P == 8)
    return _InterlockedExchange64(ptr, val);
#else
#error  "SIZEOF_VOID_P not valid"
#endif
}

static inline int MPIDU_Atomic_swap_int(volatile int *ptr, int val)
{
    return _InterlockedExchange(ptr, val);
}

static inline MPI_Aint MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val)
{
#if defined(SIZEOF_INT_IS_AINT)
    return _InterlockedExchange(ptr, val);
#else
    return _InterlockedExchange64(ptr, val);
#endif
}

/* Implement fetch_and_incr/decr using fetch_and_add (*_faa) */
#define MPIDU_Atomic_fetch_and_incr MPIDU_Atomic_fetch_and_incr_by_faa
#define MPIDU_Atomic_fetch_and_decr MPIDU_Atomic_fetch_and_decr_by_faa
#include "mpidu_atomic_emulated.h"

#endif /* defined(MPIDU_ATOMICS_NT_INTRINSICS_H_INCLUDED) */
