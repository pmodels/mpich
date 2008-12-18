/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_GCC_IA64_H_INCLUDED
#define MPIDU_ATOMICS_GCC_IA64_H_INCLUDED


#define MPIDU_Atomic_add_by_faa MPIDU_Atomic_add 
#define MPIDU_Atomic_incr_by_fai MPIDU_Atomic_incr 
#define MPIDU_Atomic_decr_by_fad MPIDU_Atomic_decr 
#define MPIDU_Atomic_fetch_and_decr_by_faa MPIDU_Atomic_fetch_and_decr
#define MPIDU_Atomic_fetch_and_incr_by_faa MPIDU_Atomic_fetch_and_incr


static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr)
{
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m"(*ptr)
                          : "r"(ptr), "i"(-1));
    return val == 1;
}

/* IA64 has a fetch-and-add instruction that only accepts immediate
   values of -16, -8, -4, -1, 1, 4, 8, and 16.  So we check for these
   values before falling back to the CAS implementation. */
#define IA64_FAA_CASE_MACRO(ptr, val) case val: {       \
    int prev;                                           \
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"    \
                          : "=r"(prev), "=m"(*ptr)      \
                          : "r"(ptr), "i"(val));        \
    return prev;                                        \
    }                                                   \
    break


static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val)
{
    switch (val)
    {
        IA64_FAA_CASE_MACRO(ptr, -16);
        IA64_FAA_CASE_MACRO(ptr,  -8);
        IA64_FAA_CASE_MACRO(ptr,  -4);
        IA64_FAA_CASE_MACRO(ptr,  -1);
        IA64_FAA_CASE_MACRO(ptr,   1);
        IA64_FAA_CASE_MACRO(ptr,   4);
        IA64_FAA_CASE_MACRO(ptr,   8);
        IA64_FAA_CASE_MACRO(ptr,  16);
    default:
        return MPIDU_Atomic_fetch_and_add_by_cas(ptr, val);
    }
}
#undef IA64_FAA_CASE_MACRO


static inline int *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv)
{
    void *prev;
    __asm__ __volatile__ ("mov ar.ccv=%1;;"
                          "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                          : "=r"(prev), "=m"(*ptr)
                          : "rO"(oldv), "r"(ptr), "r"(newv));
    return prev;   
}

static inline int MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv)
{
    int prev;

    switch (sizeof(int)) /* this switch statement should be optimized out */
    {
    case 8:
        __asm__ __volatile__ ("mov ar.ccv=%1;;"
                              "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
                              : "rO"(oldv), "r"(ptr), "r"(newv)
                              : "memory");
        break;
    case 4:
        __asm__ __volatile__ ("zxt4 %1=%1;;" /* don't want oldv sign-extended to 64 bits */
			      "mov ar.ccv=%1;;"
			      "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
			      : "r0"(oldv), "r"(ptr), "r"(newv)
                              : "memory");
        break;
    default:
        MPIU_Assertp (0);
    }
    
    return prev;   
}

static inline MPI_Aint MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
    int prev;

    switch (sizeof(MPI_Aint)) /* this switch statement should get optimized out */
    {
        case 8:
            __asm__ __volatile__ ("mov ar.ccv=%1;;"
                                  "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                                  : "=r"(prev), "=m"(*ptr)
                                  : "rO"(oldv), "r"(ptr), "r"(newv)
                                  : "memory");
            break;
        case 4:
            __asm__ __volatile__ ("zxt4 %1=%1;;" /* don't want oldv sign-extended to 64 bits */
                                  "mov ar.ccv=%1;;"
                                  "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                                  : "=r"(prev), "=m"(*ptr)
                                  : "r0"(oldv), "r"(ptr), "r"(newv)
                                  : "memory");
            break;
        default:
            MPIU_Assertp (0);
            break;
    }
    
    return prev;   
}


static inline int *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val)
{
    /* is pointer swizzling necessary here? */
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*val)
                          : "r" (ptr), "0" (val));
    return val;
}


static inline int MPIDU_Atomic_swap_int(volatile int *ptr, int val)
{
    /* is pointer swizzling necessary here? */
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*val)
                          : "r" (ptr), "0" (val));
    return val;
}

static inline MPI_Aint MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val)
{
    /* is pointer swizzling necessary here? */
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*val)
                          : "r" (ptr), "0" (val));
    return val;
}

#include"mpidu_atomic_emulated.h"

#endif /* MPIDU_ATOMICS_GCC_IA64_H_INCLUDED */
