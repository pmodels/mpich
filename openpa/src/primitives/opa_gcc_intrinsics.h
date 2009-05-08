/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME needs to be converted to new style functions with OPA_int_t/OPA_ptr_t-types */
#ifndef OPA_GCC_INTRINSICS_H_INCLUDED
#define OPA_GCC_INTRINSICS_H_INCLUDED

/* FIXME do we need to align these? */
typedef struct { volatile int v;    } OPA_int_t;
typedef struct { void * volatile v; } OPA_ptr_t;

/* Assume that loads/stores are atomic on the current platform, even though this
   may not be true at all. */
static inline int OPA_load(OPA_int_t *ptr)
{
    return ptr->v;
}

static inline void OPA_store(OPA_int_t *ptr, int val)
{
    ptr->v = val;
}

static inline void *OPA_load_ptr(OPA_ptr_t *ptr)
{
    return ptr->v;
}

static inline void OPA_store_ptr(OPA_ptr_t *ptr, void *val)
{
    ptr->v = val;
}


/* gcc atomic intrinsics accept an optional list of variables to be
   protected by a memory barrier.  These variables are labeled
   below by "protected variables :". */

static inline int OPA_fetch_and_add(OPA_int_t *ptr, int val)
{
    return __sync_fetch_and_add(&ptr->v, val, /* protected variables: */ &ptr->v);
}

static inline int OPA_decr_and_test(OPA_int_t *ptr)
{
    return __sync_sub_and_fetch(&ptr->v, 1, /* protected variables: */ &ptr->v) == 0;
}

#define OPA_fetch_and_incr_by_faa OPA_fetch_and_incr 
#define OPA_fetch_and_decr_by_faa OPA_fetch_and_decr 
#define OPA_add_by_faa OPA_add 
#define OPA_incr_by_fai OPA_incr 
#define OPA_decr_by_fad OPA_decr 


static inline void *OPA_cas_ptr(OPA_ptr_t *ptr, void *oldv, void *newv)
{
    return __sync_val_compare_and_swap(&ptr->v, oldv, newv, /* protected variables: */ &ptr->v);
}

static inline int OPA_cas_int(OPA_int_t *ptr, int oldv, int newv)
{
    return __sync_val_compare_and_swap(&ptr->v, oldv, newv, /* protected variables: */ &ptr->v);
}

#ifdef SYNC_LOCK_TEST_AND_SET_IS_SWAP
static inline void *OPA_swap_int_ptr(OPA_ptr_t *ptr, void *val)
{
    return __sync_lock_test_and_set(&ptr->v, val, /* protected variables: */ &ptr->v);
}

static inline int OPA_swap_int(OPA_int_t *ptr, int val)
{
    return __sync_lock_test_and_set(&ptr->v, val, /* protected variables: */ &ptr->v);
}

#else
#define OPA_swap_int_ptr_by_cas OPA_swap_int_ptr 
#define OPA_swap_int_by_cas OPA_swap_int 
#endif

#define OPA_write_barrier()      __sync_synchronize()
#define OPA_read_barrier()       __sync_synchronize()
#define OPA_read_write_barrier() __sync_synchronize()



#include"opa_emulated.h"

#endif /* OPA_GCC_INTRINSICS_H_INCLUDED */
