/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMIC_EMULATED_H_INCLUDED
#define MPIDU_ATOMIC_EMULATED_H_INCLUDED

/* Functions emulated using other atomics

   This header should be included at the bottom of any atomic
   primitives header that needs to implement an atomic op in terms of
   another atomic.
*/

static inline int MPIDU_Atomic_fetch_and_add_by_cas(int *ptr, int val)
{
    int cmp;
    int prev = *ptr;

    do {
        cmp = prev;
        prev = MPIDU_Atomic_cas_int(ptr, cmp, prev + val);
    } while (prev != cmp);

    return prev;
}


static inline int MPIDU_Atomic_fetch_and_incr_by_faa(volatile int *ptr)
{
    return MPIDU_Atomic_fetch_and_add(ptr, 1);
}

static inline int MPIDU_Atomic_fetch_and_decr_by_faa(volatile int *ptr)
{
    return MPIDU_Atomic_fetch_and_add(ptr, -1);
}

static inline int MPIDU_Atomic_decr_and_test_by_fad(volatile int *ptr)
{
    return MPIDU_Atomic_fetch_and_decr(ptr) == 1;
}

static inline void MPIDU_Atomic_add_by_faa(volatile int *ptr, int val)
{
    MPIDU_Atomic_fetch_and_add(ptr, val);
}

static inline void MPIDU_Atomic_incr_by_fai(volatile int *ptr)
{
    MPIDU_Atomic_fetch_and_incr(ptr);
}

static inline void MPIDU_Atomic_decr_by_fad(volatile int *ptr)
{
    MPIDU_Atomic_fetch_and_decr(ptr);
}


/* Swap using CAS */

static inline int *MPIDU_Atomic_swap_int_ptr_by_cas(int * volatile *ptr, int *val)
{
    int *cmp;
    int *prev = *ptr;

    do {
        cmp = prev;
        prev = MPIDU_Atomic_cas_int_ptr(ptr, cmp, val);
    } while (prev != cmp);

    return prev;
}

static inline int MPIDU_Atomic_swap_int_by_cas(volatile int *ptr, int val)
{
    int cmp;
    int prev = *ptr;

    do {
        cmp = prev;
        prev = MPIDU_Atomic_cas_int(ptr, cmp, val);
    } while (prev != cmp);

    return prev;
}

static inline MPI_Aint MPIDU_Atomic_swap_aint_by_cas(volatile MPI_Aint *ptr, MPI_Aint val)
{
    MPI_Aint cmp;
    MPI_Aint prev = *ptr;

    do {
        cmp = prev;
        prev = MPIDU_Atomic_cas_aint(ptr, cmp, val);
    } while (prev != cmp);

    return prev;
}


/* Emulating using LL/SC */

#ifdef ATOMIC_LL_SC_SUPPORTED
static inline int MPIDU_Atomic_fetch_and_add_by_llsc(volatile int *ptr, int val)
{
    int prev;
    do {
        prev = MPIDU_Atomic_LL_int(ptr);
    } while (!MPIDU_Atomic_SC_int(ptr, prev + val));
    return prev;
}


static inline void MPIDU_Atomic_add_by_llsc(int *ptr, int val)
{
    MPIDU_Atomic_fetch_and_add_by_llsc(ptr, val);
}

static inline void MPIDU_Atomic_incr_by_llsc(volatile int *ptr)
{
    MPIDU_Atomic_add_by_llsc(ptr, 1);
}

static inline void MPIDU_Atomic_decr_by_llsc(volatile int *ptr)
{
    MPIDU_Atomic_add_by_llsc(ptr, -1);
}


static inline int MPIDU_Atomic_fetch_and_decr_by_llsc(volatile int *ptr)
{
    return MPIDU_Atomic_fetch_and_add_by_llsc(ptr, -1);
}

static inline int MPIDU_Atomic_fetch_and_incr_by_llsc(volatile int *ptr)
{
    return MPIDU_Atomic_fetch_and_add_by_llsc(ptr, 1);
}

static inline int MPIDU_Atomic_decr_and_test_by_llsc(volatile int *ptr)
{
    int prev = MPIDU_Atomic_fetch_and_decr_by_llsc(ptr);
    return prev == 1;
}



static inline int *MPIDU_Atomic_cas_int_ptr_by_llsc(int * volatile *ptr, int *oldv, int *newv)
{
    int *prev;
    do {
        prev = MPIDU_Atomic_LL_int_ptr(ptr);
    } while (prev == oldv && !MPIDU_Atomic_SC_int_ptr(ptr, newv));
    return prev;
}

static inline int MPIDU_Atomic_cas_int_by_llsc(volatile int *ptr, int oldv, int newv)
{
    int prev;
    do {
        prev = MPIDU_Atomic_LL_int(ptr);
    } while (prev == oldv && !MPIDU_Atomic_SC_int(ptr, newv));
    return prev;
}

static inline MPI_Aint MPIDU_Atomic_cas_aint_by_llsc(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
    aint prev;
    do {
        prev = MPIDU_Atomic_LL_aint(ptr);
    } while (prev == oldv && !MPIDU_Atomic_SC_aint(ptr, newv));
    return prev;
}



static inline int *MPIDU_Atomic_swap_int_ptr_by_llsc(int * volatile *ptr, int *val)
{
    int *prev;
    do {
        prev = MPIDU_Atomic_LL_int_ptr(ptr);
    } while (!MPIDU_Atomic_SC_int_ptr(ptr, val));
    return prev;
}

static inline int MPIDU_Atomic_swap_int_by_llsc(volatile int *ptr, int val)
{
    int prev;
    do {
        prev = MPIDU_Atomic_LL_int(ptr);
    } while (!MPIDU_Atomic_SC_int(ptr, val));
    return prev;
}

static inline MPI_Aint MPIDU_Atomic_swap_aint_by_llsc(volatile MPI_Aint *ptr, MPI_Aint val)
{
    aint prev;
    do {
        prev = MPIDU_Atomic_LL_aint(ptr);
    } while (!MPIDU_Atomic_SC_aint(ptr, val));
    return prev;
}

#endif /* ATOMIC_LL_SC_SUPPORTED */


#endif /* MPIDU_ATOMIC_EMULATED_H_INCLUDED */
