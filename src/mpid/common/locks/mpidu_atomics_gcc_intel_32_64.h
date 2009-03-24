/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_GCC_INTEL_32_64_H_INCLUDED
#define MPIDU_ATOMICS_GCC_INTEL_32_64_H_INCLUDED

#define MPIDU_ATOMIC_UNIVERSAL_PRIMITIVE MPIDU_ATOMIC_CAS

/* XXX DJG FIXME do we need to align these? */
typedef struct { volatile int v;  } MPIDU_Atomic_t;
typedef struct { int * volatile v; } MPIDU_Atomic_ptr_t;

/* Aligned loads and stores are atomic on x86(-64). */
static inline int MPIDU_Atomic_load(MPIDU_Atomic_t *ptr)
{
    return ptr->v;
}

/* Aligned loads and stores are atomic on x86(-64). */
static inline void MPIDU_Atomic_store(MPIDU_Atomic_t *ptr, int val)
{
    ptr->v = val;
}

/* Aligned loads and stores are atomic on x86(-64). */
static inline void *MPIDU_Atomic_load_ptr(MPIDU_Atomic_ptr_t *ptr)
{
    return ptr->v;
}

/* Aligned loads and stores are atomic on x86(-64). */
static inline void MPIDU_Atomic_store_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    ptr->v = val;
}

static inline void MPIDU_Atomic_add(MPIDU_Atomic_t *ptr, int val)
{
    __asm__ __volatile__ ("lock ; add %1,%0"
                          :"=m" (ptr->v)
                          :"ir" (val), "m" (ptr->v));
}

static inline void MPIDU_Atomic_incr(MPIDU_Atomic_t *ptr)
{
    switch(sizeof(ptr->v))
    {
    case 4:
        __asm__ __volatile__ ("lock ; incl %0"
                              :"=m" (ptr->v)
                              :"m" (ptr->v));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; incq %0"
                              :"=m" (ptr->v)
                              :"m" (ptr->v));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIDU_Atomic_assert(0);
    }
    return;
}

static inline void MPIDU_Atomic_decr(MPIDU_Atomic_t *ptr)
{
    switch(sizeof(ptr->v))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0"
                              :"=m" (ptr->v)
                              :"m" (ptr->v));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0"
                              :"=m" (ptr->v)
                              :"m" (ptr->v));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIDU_Atomic_assert(0);
    }
    return;
}


static inline int MPIDU_Atomic_decr_and_test(MPIDU_Atomic_t *ptr)
{
    int result;
    switch(sizeof(ptr->v))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0; setz %1"
                              :"=m" (ptr->v), "=q" (result)
                              :"m" (ptr->v));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0; setz %1"
                              :"=m" (ptr->v), "=q" (result)
                              :"m" (ptr->v));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIDU_Atomic_assert(0);
    }
    return result;
}

static inline int MPIDU_Atomic_fetch_and_add(MPIDU_Atomic_t *ptr, int val)
{
    __asm__ __volatile__ ("lock ; xadd %0,%1"
                          : "=r" (val), "=m" (ptr->v)
                          :  "0" (val),  "m" (ptr->v));
    return val;
}

#define MPIDU_Atomic_fetch_and_decr_by_faa MPIDU_Atomic_fetch_and_decr 
#define MPIDU_Atomic_fetch_and_incr_by_faa MPIDU_Atomic_fetch_and_incr 


static inline int *MPIDU_Atomic_cas_ptr(MPIDU_Atomic_ptr_t *ptr, void *oldv, void *newv)
{
    int *prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (ptr->v)
                          : "q" (newv), "m" (ptr->v), "0" (oldv));
    return prev;
}

static inline int MPIDU_Atomic_cas_int(MPIDU_Atomic_t *ptr, int oldv, int newv)
{
    int prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (ptr->v)
                          : "q" (newv), "m" (ptr->v), "0" (oldv)
                          : "memory");
    return prev;
}

static inline int *MPIDU_Atomic_swap_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    __asm__ __volatile__ ("xchg %0,%1"
                          :"=r" (val), "=m" (ptr->v)
                          : "0" (val),  "m" (ptr->v));
    return val;
}

static inline int MPIDU_Atomic_swap_int(MPIDU_Atomic_t *ptr, int val)
{
    __asm__ __volatile__ ("xchg %0,%1"
                          :"=r" (val), "=m" (ptr->v)
                          : "0" (val),  "m" (ptr->v));
    return val;
}

#define MPIDU_Shm_write_barrier()      __asm__ __volatile__  ( "sfence" ::: "memory" )
#define MPIDU_Shm_read_barrier()       __asm__ __volatile__  ( "lfence" ::: "memory" )
#define MPIDU_Shm_read_write_barrier() __asm__ __volatile__  ( "mfence" ::: "memory" )

#include"mpidu_atomic_emulated.h"

#endif /* MPIDU_ATOMICS_GCC_INTEL_32_64_H_INCLUDED */
