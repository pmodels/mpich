/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_INT_H_INCLUDED
#define MPIR_INT_H_INCLUDED

#if (defined(MPICH_IS_THREADED)) && (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY__GLOBAL)
/* we need atomics */

#define MPIR_INT_T_INITIALIZER(val) OPA_INT_T_INITIALIZER(val)
typedef OPA_int_t MPIR_Int_t;

MPL_STATIC_INLINE_PREFIX int MPIR_Int_load(MPIR_Int_t * ptr)
{
    return OPA_load_int(ptr);
}

MPL_STATIC_INLINE_PREFIX void MPIR_Int_store(MPIR_Int_t * ptr, int val)
{
    OPA_store_int(ptr, val);
}

MPL_STATIC_INLINE_PREFIX void MPIR_Int_incr(MPIR_Int_t * ptr)
{
    OPA_incr_int(ptr);
}

MPL_STATIC_INLINE_PREFIX void MPIR_Int_decr(MPIR_Int_t * ptr)
{
    OPA_decr_int(ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIR_Int_fetch_and_add(MPIR_Int_t * ptr, int val)
{
    return OPA_fetch_and_add_int(ptr, val);
}

#else
/* we don't need atomics */

#define MPIR_INT_T_INITIALIZER(val) val

typedef volatile int MPIR_Int_t;

MPL_STATIC_INLINE_PREFIX int MPIR_Int_load(MPIR_Int_t * ptr)
{
    return *ptr;
}

MPL_STATIC_INLINE_PREFIX void MPIR_Int_store(MPIR_Int_t * ptr, int val)
{
    *ptr = val;
}

MPL_STATIC_INLINE_PREFIX void MPIR_Int_incr(MPIR_Int_t * ptr)
{
    (*ptr)++;
}

MPL_STATIC_INLINE_PREFIX void MPIR_Int_decr(MPIR_Int_t * ptr)
{
    (*ptr)--;
}

MPL_STATIC_INLINE_PREFIX int MPIR_Int_fetch_and_add(MPIR_Int_t * ptr, int val)
{
    int prev_val;

    prev_val = *ptr;
    (*ptr) += val;

    return prev_val;
}

#endif /* check for the need of atomics */

#endif /* MPIR_INT_H_INCLUDED */
