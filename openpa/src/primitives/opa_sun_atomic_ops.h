/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef OPA_SUN_ATOMIC_OPS_H_INCLUDED
#define OPA_SUN_ATOMIC_OPS_H_INCLUDED

#include <atomic.h>

typedef struct { volatile uint_t v;  } OPA_int_t;
typedef struct { void * volatile v; } OPA_ptr_t;

static inline int OPA_load(OPA_int_t *ptr)
{
    return (int)ptr->v;
}

static inline void OPA_store(OPA_int_t *ptr, int val)
{
    ptr->v = (uint)val;
}

static inline void *OPA_load_ptr(OPA_ptr_t *ptr)
{
    return ptr->v;
}

static inline void OPA_store_ptr(OPA_ptr_t *ptr, void *val)
{
    ptr->v = val;
}


static inline void OPA_add(OPA_int_t *ptr, int val)
{
    atomic_add_int(&ptr->v, val);
}

static inline void OPA_incr(OPA_int_t *ptr)
{
    atomic_inc_uint(&ptr->v);
}

static inline void OPA_decr(OPA_int_t *ptr)
{
    atomic_dec_uint(&ptr->v);
}


static inline int OPA_decr_and_test(OPA_int_t *ptr)
{
    return atomic_dec_uint_nv(&ptr->v) == 0;    
}


static inline int OPA_fetch_and_add(OPA_int_t *ptr, int val)
{
    return (int)atomic_add_int_nv(&ptr->v, val) - val;
}

static inline int OPA_fetch_and_decr(OPA_int_t *ptr)
{
    return (int)atomic_dec_uint_nv(&ptr->v) + 1;
}

static inline int OPA_fetch_and_incr(OPA_int_t *ptr)
{
    return (int)atomic_inc_uint_nv(&ptr->v) - 1;
}


static inline void *OPA_cas_ptr(OPA_ptr_t *ptr, void *oldv, void *newv)
{
    return atomic_cas_ptr(ptr, oldv, newv);
}

static inline int OPA_cas_int(OPA_int_t *ptr, int oldv, int newv)
{
    return (int)atomic_cas_uint(&ptr->v, (uint_t)oldv, (uint_t)newv);
}


static inline void *OPA_swap_ptr(OPA_ptr_t *ptr, void *val)
{
    return atomic_swap_ptr(ptr, val);
}

static inline int OPA_swap_int(OPA_int_t *ptr, int val)
{
    return (int)atomic_swap_uint(&ptr->v, (uint_t) val);
}


#define OPA_write_barrier()      membar_producer()
#define OPA_read_barrier()       membar_consumer()
#define OPA_read_write_barrier() do { membar_consumer(); membar_producer(); } while (0)

#endif /* OPA_SUN_ATOMIC_OPS_H_INCLUDED */
