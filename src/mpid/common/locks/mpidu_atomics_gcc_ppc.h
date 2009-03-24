/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_GCC_PPC_H_INCLUDED
#define MPIDU_ATOMICS_GCC_PPC_H_INCLUDED

/*
   load-link/store-conditional (LL/SC) primitives.  We LL/SC implement
   these here, which are arch-specific, then use the generic
   implementations from _atomic_emulated.h */

static inline int MPIDU_Atomic_LL_int(volatile int *ptr)
{
    int val;
    __asm__ __volatile__ ("lwarx %[val],0,%[ptr]"
                          : [val] "=r" (val)
                          : [ptr] "r" (ptr)
                          : "cc");

    return val;
}

/* Returns non-zero if the store was successful, zero otherwise. */
static inline int MPIDU_Atomic_SC_int(volatile int *ptr, int val)
{
    int ret;
    __asm__ __volatile__ ("stwcx. %[val],0,%[ptr];\n"
                          "beq 1f;\n"
                          "li %[ret], 0;\n"
                          "1: ;\n"
                          : [ret] "=r" (ret)
                          : [ptr] "r" (ptr), [val] "r" (val), "0" (ret)
                          : "cc", "memory");
    return ret;
}


/*
   Pointer versions of LL/SC.  For now we implement them in terms of the integer
   versions with some casting.  If/when we encounter a platform with LL/SC and
   differing word and pointer widths we can write separate versions.
*/

static inline int *MPIDU_Atomic_LL_int_ptr(volatile int **ptr)
{
    switch (sizeof(int *)) { /* should get optimized away */
        case sizeof(int):
            return MPIDU_Atomic_LL_int((volatile int)ptr);
            break;
        default:
            MPIU_Assertp(0); /* need to implement a separate ptr-sized version */
            return NULL; /* placate the compiler */
    }
}

/* Returns non-zero if the store was successful, zero otherwise. */
static inline int MPIDU_Atomic_SC_int_ptr(volatile int **ptr, int *val)
{
    switch (sizeof(int *)) { /* should get optimized away */
        case sizeof(int):
            return MPIDU_Atomic_SC_int((volatile int *)ptr, (int)val);
            break;
        default:
            MPIU_Assertp(0); /* need to implement a separate ptr-sized version */
            return NULL; /* placate the compiler */
    }
}

/* Implement all function using LL/SC */

#define MPIDU_Atomic_add_by_llsc            MPIDU_Atomic_add
#define MPIDU_Atomic_incr_by_llsc           MPIDU_Atomic_incr
#define MPIDU_Atomic_decr_by_llsc           MPIDU_Atomic_decr
#define MPIDU_Atomic_decr_and_test_by_llsc  MPIDU_Atomic_decr_and_test
#define MPIDU_Atomic_fetch_and_add_by_llsc  MPIDU_Atomic_fetch_and_add
#define MPIDU_Atomic_fetch_and_decr_by_llsc MPIDU_Atomic_fetch_and_decr
#define MPIDU_Atomic_fetch_and_incr_by_llsc MPIDU_Atomic_fetch_and_incr
#define MPIDU_Atomic_cas_int_ptr_by_llsc    MPIDU_Atomic_cas_int_ptr
#define MPIDU_Atomic_cas_int_by_llsc        MPIDU_Atomic_cas_int
#define MPIDU_Atomic_cas_aint_by_llsc       MPIDU_Atomic_cas_aint
#define MPIDU_Atomic_swap_int_ptr_by_llsc   MPIDU_Atomic_swap_int_ptr
#define MPIDU_Atomic_swap_int_by_llsc       MPIDU_Atomic_swap_int
#define MPIDU_Atomic_swap_aint_by_llsc      MPIDU_Atomic_swap_aint

/* FIXME: "sync" is a full memory barrier, we should look into using
   less costly barriers (like lwsync or eieio) where appropriate
   whenever they're available */
#define MPIDU_Shm_write_barrier()      __asm__ __volatile__  ("sync" ::: "memory" )
#define MPIDU_Shm_read_barrier()       __asm__ __volatile__  ("sync" ::: "memory" )
#define MPIDU_Shm_read_write_barrier() __asm__ __volatile__  ("sync" ::: "memory" )

#include"mpidu_atomic_emulated.h"

#endif /* MPIDU_ATOMICS_GCC_PPC_H_INCLUDED */
