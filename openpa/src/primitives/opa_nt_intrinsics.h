/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_NT_INTRINSICS_H_INCLUDED
#define OPA_NT_INTRINSICS_H_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#include<intrin.h>
#include "mpi.h"

/* OPA_int_t uses a long because the compiler intrinsics operate on
 * longs instead of ints. */
typedef struct { volatile long v;  } OPA_int_t;
typedef struct { void * volatile v; } OPA_ptr_t;

static inline int OPA_load(OPA_int_t *ptr)
{
    return ((int)ptr->v);
}

static inline void OPA_store(OPA_int_t *ptr, int val)
{
    ptr->v = (long)val;
}

static inline void *OPA_load_ptr(OPA_ptr_t *ptr)
{
    return ((void *)ptr->v);
}

static inline void OPA_store_ptr(OPA_ptr_t *ptr, void *val)
{
    ptr->v = val;
}

static inline void OPA_add(OPA_int_t *ptr, int val)
{
    _InterlockedExchangeAdd(&(ptr->v), val);
}

static inline void OPA_incr(OPA_int_t *ptr)
{
    _InterlockedIncrement(&(ptr->v));
}

static inline void OPA_decr(OPA_int_t *ptr)
{
    _InterlockedDecrement(&(ptr->v));
}

static inline int OPA_decr_and_test(OPA_int_t *ptr)
{
    return (_InterlockedDecrement(&(ptr->v)) == 0);
}

static inline int OPA_fetch_and_add(OPA_int_t *ptr, int val)
{
    return ((int)_InterlockedExchangeAdd(&(ptr->v), (long)val));
}

static inline void *OPA_cas_ptr(OPA_ptr_t *ptr, void *oldv, void *newv)
{
#if (OPA_SIZEOF_VOID_P == 4)
    return ((LONG_PTR) _InterlockedCompareExchange((LONG volatile *)&(ptr->v),
                                                   (LONG)(LONG_PTR)newv,
                                                   (LONG)(LONG_PTR)oldv)
           );
#elif (OPA_SIZEOF_VOID_P == 8)
    return ((LONG_PTR)_InterlockedCompareExchange64((INT64 volatile *)&(ptr->v),
                                                    (INT64)(LONG_PTR)newv,
                                                    (INT64)(LONG_PTR)oldv)
           );
#else
#error  "OPA_SIZEOF_VOID_P not valid"
#endif
}

static inline void *OPA_swap_ptr(OPA_ptr_t *ptr, void *val)
{
#if (OPA_SIZEOF_VOID_P == 4)
    return _InterlockedExchange(&(ptr->v), val);
#elif (OPA_SIZEOF_VOID_P == 8)
    return _InterlockedExchange64(&(ptr->v), val);
#else
#error  "OPA_SIZEOF_VOID_P not valid"
#endif
}

static inline int OPA_cas_int(OPA_int_t *ptr, int oldv, int newv)
{
    return _InterlockedCompareExchange(&(ptr->v), newv, oldv);
}

static inline int OPA_swap_int(OPA_int_t *ptr, int val)
{
    return _InterlockedExchange(&(ptr->v), val);
}

/* Implement fetch_and_incr/decr using fetch_and_add (*_faa) */
#define OPA_fetch_and_incr OPA_fetch_and_incr_by_faa
#define OPA_fetch_and_decr OPA_fetch_and_decr_by_faa

#define OPA_write_barrier()      _WriteBarrier()
#define OPA_read_barrier()       _ReadBarrier()
#define OPA_read_write_barrier() _ReadWriteBarrier()

#include "opa_emulated.h"

#endif /* defined(OPA_NT_INTRINSICS_H_INCLUDED) */
