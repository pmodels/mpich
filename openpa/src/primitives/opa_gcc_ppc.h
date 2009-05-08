/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_GCC_PPC_H_INCLUDED
#define OPA_GCC_PPC_H_INCLUDED

/* these need to be aligned on an 8-byte boundary to work on a BG/P */
typedef struct { volatile int v;    ATTRIBUTE((aligned (8))) } OPA_int_t;
typedef struct { void * volatile v; ATTRIBUTE((aligned (8))) } OPA_ptr_t;

/*
   load-link/store-conditional (LL/SC) primitives.  We LL/SC implement
   these here, which are arch-specific, then use the generic
   implementations from opa_emulated.h */

static _opa_inline int OPA_LL_int(OPA_int_t *ptr)
{
    int val;
    __asm__ __volatile__ ("lwarx %[val],0,%[ptr]"
                          : [val] "=r" (val)
                          : [ptr] "r" (&ptr->v)
                          : "cc");

    return val;
}

/* Returns non-zero if the store was successful, zero otherwise. */
static _opa_inline int OPA_SC_int(OPA_int_t *ptr, int val)
{
    int ret;
    __asm__ __volatile__ ("stwcx. %[val],0,%[ptr];\n"
                          "beq 1f;\n"
                          "li %[ret], 0;\n"
                          "1: ;\n"
                          : [ret] "=r" (ret)
                          : [ptr] "r" (&ptr->v), [val] "r" (val), "0" (ret)
                          : "cc", "memory");
    return ret;
}


/*
   Pointer versions of LL/SC.  For now we implement them in terms of the integer
   versions with some casting.  If/when we encounter a platform with LL/SC and
   differing word and pointer widths we can write separate versions.
*/

static _opa_inline void *OPA_LL_ptr(OPA_ptr_t *ptr)
{
    /* need to implement a separate ptr-sized version, although it's not needed
       on the BG/P it might be needed on a different PPC impl */
    OPA_COMPILE_TIME_ASSERT(sizeof(int) == sizeof(void *)); 
    OPA_COMPILE_TIME_ASSERT(sizeof(OPA_int_t) == sizeof(OPA_ptr_t));
    return (void *) OPA_LL_int((OPA_int_t *)ptr);
}

/* Returns non-zero if the store was successful, zero otherwise. */
static _opa_inline int OPA_SC_ptr(OPA_ptr_t *ptr, void *val)
{
    /* need to implement a separate ptr-sized version, although it's not needed
       on the BG/P it might be needed on a different PPC impl */
    OPA_COMPILE_TIME_ASSERT(sizeof(int) == sizeof(void *)); 
    OPA_COMPILE_TIME_ASSERT(sizeof(OPA_int_t) == sizeof(OPA_ptr_t));
    return OPA_SC_int((OPA_int_t *)ptr, (int)val);
}

/* necessary to enable LL/SC emulation support */
#define OPA_LL_SC_SUPPORTED 1

/* Implement all function using LL/SC */
#define OPA_add_by_llsc            OPA_add
#define OPA_incr_by_llsc           OPA_incr
#define OPA_decr_by_llsc           OPA_decr
#define OPA_decr_and_test_by_llsc  OPA_decr_and_test
#define OPA_fetch_and_add_by_llsc  OPA_fetch_and_add
#define OPA_fetch_and_decr_by_llsc OPA_fetch_and_decr
#define OPA_fetch_and_incr_by_llsc OPA_fetch_and_incr
#define OPA_cas_ptr_by_llsc        OPA_cas_ptr
#define OPA_cas_int_by_llsc        OPA_cas_int
#define OPA_swap_ptr_by_llsc       OPA_swap_ptr
#define OPA_swap_int_by_llsc       OPA_swap_int

/* FIXME: "sync" is a full memory barrier, we should look into using
   less costly barriers (like lwsync or eieio) where appropriate
   whenever they're available */
#define OPA_write_barrier()      __asm__ __volatile__  ("sync" ::: "memory" )
#define OPA_read_barrier()       __asm__ __volatile__  ("sync" ::: "memory" )
#define OPA_read_write_barrier() __asm__ __volatile__  ("sync" ::: "memory" )


#include "opa_emulated.h"

#endif /* OPA_GCC_PPC_H_INCLUDED */
