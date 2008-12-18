/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_GCC_INTEL_32_64_H_INCLUDED
#define MPIDU_ATOMICS_GCC_INTEL_32_64_H_INCLUDED

static inline void MPIDU_Atomic_add(volatile int *ptr, int val)
{
    __asm__ __volatile__ ("lock ; add %1,%0"
                          :"=m" (*ptr)
                          :"ir" (val), "m" (*ptr));
}

static inline void MPIDU_Atomic_incr(volatile int *ptr)
{
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; incl %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; incq %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return;
}

static inline void MPIDU_Atomic_decr(volatile int *ptr)
{
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return;
}


static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr)
{
    int result;
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0; setz %1"
                              :"=m" (*ptr), "=q" (result)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0; setz %1"
                              :"=m" (*ptr), "=q" (result)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return result;
}

static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val)
{
    __asm__ __volatile__ ("lock ; xadd %0,%1"
                          : "=r" (val), "=m" (*ptr)
                          :  "0" (val),  "m" (*ptr));
    return val;
}

#define MPIDU_Atomic_fetch_and_decr_by_faa MPIDU_Atomic_fetch_and_decr 
#define MPIDU_Atomic_fetch_and_incr_by_faa MPIDU_Atomic_fetch_and_incr 


static inline int *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv)
{
    int *prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv));
    return prev;
}

static inline int MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv)
{
    int prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv)
                          : "memory");
    return prev;
}

static inline MPI_Aint MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
    MPI_Aint prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv)
                          : "memory");
    return prev;   
}


static inline int *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val)
{
    __asm__ __volatile__ ("xchg %0,%1"
                          :"=r" (val), "=m" (*ptr)
                          : "0" (val),  "m" (*ptr));
    return val;
}

static inline int MPIDU_Atomic_swap_int(volatile int *ptr, int val)
{
    __asm__ __volatile__ ("xchg %0,%1"
                          :"=r" (val), "=m" (*ptr)
                          : "0" (val),  "m" (*ptr));
    return val;
}

static inline MPI_Aint MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val)
{
    __asm__ __volatile__ ("xchg %0,%1"
                          :"=r" (val), "=m" (*ptr)
                          : "0" (val),  "m" (*ptr));
    return val;
}

#include"mpidu_atomic_emulated.h"

#endif /* MPIDU_ATOMICS_GCC_INTEL_32_64_H_INCLUDED */
