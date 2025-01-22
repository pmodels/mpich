/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_ATOMIC_FLAG_H_INCLUDED
#define MPIR_ATOMIC_FLAG_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"

#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE && \
    MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI

typedef MPL_atomic_int_t MPIR_atomic_flag_t;

static inline void MPIR_atomic_flag_set(MPIR_atomic_flag_t * flag_ptr, int val)
{
    MPL_atomic_relaxed_store_int(flag_ptr, val);
}

static inline int MPIR_atomic_flag_get(MPIR_atomic_flag_t * flag_ptr)
{
    return MPL_atomic_relaxed_load_int(flag_ptr);
}

static inline int MPIR_atomic_flag_swap(MPIR_atomic_flag_t * flag_ptr, int val)
{
    return MPL_atomic_swap_int(flag_ptr, val);
}

static inline int MPIR_atomic_flag_cas(MPIR_atomic_flag_t * flag_ptr, int old_val, int new_val)
{
    return MPL_atomic_cas_int(flag_ptr, old_val, new_val);
}

#else

typedef int MPIR_atomic_flag_t;

static inline void MPIR_atomic_flag_set(MPIR_atomic_flag_t * flag_ptr, int val)
{
    *flag_ptr = val;
}

static inline int MPIR_atomic_flag_get(MPIR_atomic_flag_t * flag_ptr)
{
    return *flag_ptr;
}

static inline int MPIR_atomic_flag_swap(MPIR_atomic_flag_t * flag_ptr, int val)
{
    int ret = *flag_ptr;
    *flag_ptr = val;
    return ret;
}

static inline int MPIR_atomic_flag_cas(MPIR_atomic_flag_t * flag_ptr, int old_val, int new_val)
{
    int ret = *flag_ptr;
    if (*flag_ptr == old_val) {
        *flag_ptr = new_val;
    }
    return ret;
}

#endif

#endif /* MPIR_ATOMIC_FLAG_H_INCLUDED */
