/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_GCC_INTRINSICS_H_INCLUDED
#define MPIDU_ATOMICS_GCC_INTRINSICS_H_INCLUDED

/* gcc atomic intrinsics accept an optional list of variables to be
   protected by a memory barrier.  These variables are labeled
   below by "protected variables :". */

static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val)
{
    return __sync_fetch_and_add(ptr, val, /* protected variables: */ ptr);
}

static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr)
{
    return __sync_sub_and_fetch(ptr, 1, /* protected variables: */ ptr) == 0;
}

#define MPIDU_Atomic_fetch_and_incr_by_faa MPIDU_Atomic_fetch_and_incr 
#define MPIDU_Atomic_fetch_and_decr_by_faa MPIDU_Atomic_fetch_and_decr 
#define MPIDU_Atomic_add_by_faa MPIDU_Atomic_add 
#define MPIDU_Atomic_incr_by_fai MPIDU_Atomic_incr 
#define MPIDU_Atomic_decr_by_fad MPIDU_Atomic_decr 


static inline int *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv)
{
    return __sync_val_compare_and_swap(ptr, oldv, newv, /* protected variables: */ ptr);
}

static inline int MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv)
{
    return __sync_val_compare_and_swap(ptr, oldv, newv, /* protected variables: */ ptr);
}

static inline MPI_Aint MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
    return __sync_val_compare_and_swap(ptr, oldv, newv, /* protected variables: */ ptr);
}

#ifdef SYNC_LOCK_TEST_AND_SET_IS_SWAP
static inline int *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val)
{
    return __sync_lock_test_and_set(ptr, val, /* protected variables: */ ptr);
}

static inline int MPIDU_Atomic_swap_int(volatile int *ptr, int val)
{
    return __sync_lock_test_and_set(ptr, val, /* protected variables: */ ptr);
}

static inline MPI_Aint MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val)
{
    return __sync_lock_test_and_set(ptr, val, /* protected variables: */ ptr);
}
#else
#define MPIDU_Atomic_swap_int_ptr_by_cas MPIDU_Atomic_swap_int_ptr 
#define MPIDU_Atomic_swap_int_by_cas MPIDU_Atomic_swap_int 
#define MPIDU_Atomic_swap_aint_by_cas MPIDU_Atomic_swap_aint 
#endif

#include"mpidu_atomic_emulated.h"

#endif /* MPIDU_ATOMICS_GCC_INTRINSICS_H_INCLUDED */
