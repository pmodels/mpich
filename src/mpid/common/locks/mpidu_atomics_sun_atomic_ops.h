/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_SUN_ATOMIC_OPS_H_INCLUDED
#define MPIDU_ATOMICS_SUN_ATOMIC_OPS_H_INCLUDED

#include <atomic.h>

typedef struct { volatile uint_t v;  } MPIDU_Atomic_t;
typedef struct { void * volatile v; } MPIDU_Atomic_ptr_t;

static inline int MPIDU_Atomic_load(MPIDU_Atomic_t *ptr)
{
    return (int)ptr->v;
}

static inline void MPIDU_Atomic_store(MPIDU_Atomic_t *ptr, int val)
{
    ptr->v = (uint)val;
}

static inline void *MPIDU_Atomic_load_ptr(MPIDU_Atomic_ptr_t *ptr)
{
    return ptr->v;
}

static inline void MPIDU_Atomic_store_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    ptr->v = val;
}


static inline void MPIDU_Atomic_add(MPIDU_Atomic_t *ptr, int val)
{
    atomic_add_int(&ptr->v, val);
}

static inline void MPIDU_Atomic_incr(MPIDU_Atomic_t *ptr)
{
    atomic_inc_uint(&ptr->v);
}

static inline void MPIDU_Atomic_decr(MPIDU_Atomic_t *ptr)
{
    atomic_dec_uint(&ptr->v);
}


static inline int MPIDU_Atomic_decr_and_test(MPIDU_Atomic_t *ptr)
{
    return atomic_dec_uint_nv(&ptr->v) == 0;    
}


static inline int MPIDU_Atomic_fetch_and_add(MPIDU_Atomic_t *ptr, int val)
{
    return (int)atomic_add_int_nv(&ptr->v, val) - val;
}

static inline int MPIDU_Atomic_fetch_and_decr(MPIDU_Atomic_t *ptr)
{
    return (int)atomic_dec_uint_nv(&ptr->v) + 1;
}

static inline int MPIDU_Atomic_fetch_and_incr(MPIDU_Atomic_t *ptr)
{
    return (int)atomic_inc_uint_nv(&ptr->v) - 1;
}


static inline int *MPIDU_Atomic_cas_ptr(MPIDU_Atomic_ptr_t *ptr, int *oldv, int *newv)
{
    return atomic_cas_ptr(ptr, oldv, newv);
}

static inline int MPIDU_Atomic_cas_int(MPIDU_Atomic_t *ptr, int oldv, int newv)
{
    return (int)atomic_cas_uint(&ptr->v, (uint_t)oldv, (uint_t)newv);
}


static inline int *MPIDU_Atomic_swap_ptr(MPIDU_Atomic_ptr_t *ptr, int *val)
{
    return atomic_swap_ptr(ptr, val);
}

static inline int MPIDU_Atomic_swap_int(MPIDU_Atomic_t *ptr, int val)
{
    return (int)atomic_swap_uint(&ptr->v, (uint_t) val);
}


#define MPIDU_Shm_write_barrier()      membar_producer()
#define MPIDU_Shm_read_barrier()       membar_consumer()
#define MPIDU_Shm_read_write_barrier() do { membar_consumer(); membar_producer(); } while (0)

#endif /* MPIDU_ATOMICS_SUN_ATOMIC_OPS_H_INCLUDED */
