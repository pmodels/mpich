/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMICS_SUN_ATOMIC_OPS_H_INCLUDED
#define MPIDU_ATOMICS_SUN_ATOMIC_OPS_H_INCLUDED

#include <atomic.h>

static inline void MPIDU_Atomic_add(volatile int *ptr, int val)
{
    atomic_add_int((volatile uint_t *)ptr, val);
}

static inline void MPIDU_Atomic_incr(volatile int *ptr)
{
    atomic_inc_uint((volatile uint_t *)ptr);
}

static inline void MPIDU_Atomic_decr(volatile int *ptr)
{
    atomic_dec_uint((volatile uint_t *)ptr);
}


static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr)
{
    return atomic_dec_uint_nv((volatile uint_t *)ptr) == 0;    
}


static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val)
{
    return (int)atomic_add_int_nv((volatile uint_t *)ptr, val) - val;
}

static inline int MPIDU_Atomic_fetch_and_decr(volatile int *ptr)
{
    return (int)atomic_dec_uint_nv((volatile uint_t *)ptr) + 1;
}

static inline int MPIDU_Atomic_fetch_and_incr(volatile int *ptr)
{
    return (int)atomic_inc_uint_nv((volatile uint_t *)ptr) - 1;
}


static inline int *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv)
{
    return atomic_cas_ptr(ptr, oldv, newv);
}

static inline int MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv)
{
    return (int)atomic_cas_uint((volatile uint_t *)ptr, (uint_t)oldv, (uint_t)newv);
}

static inline MPI_Aint MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
    switch (sizeof(MPI_Aint))
    {
    case 4:
        return (MPI_Aint)atomic_cas_32((volatile uint32_t *)ptr, (uint32_t)oldv, (uint32_t)newv);
        break;
    case 8:
        return (MPI_Aint)atomic_cas_64((volatile uint64_t *)ptr, (uint64_t)oldv, (uint64_t)newv);
        break;
    default:
        /* FIXME: A compile-time check for this would be better. */
        MPIU_Assertp(0);
        break;
    }
}


static inline int *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val)
{
    return atomic_swap_ptr(ptr, val);
}

static inline int MPIDU_Atomic_swap_int(volatile int *ptr, int val)
{
    return (int)atomic_swap_uint((volatile uint_t *)ptr, (uint_t) val);
}

static inline MPI_Aint MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val)
{
    switch (sizeof(MPI_Aint))
    {
    case 4:
        return (MPI_Aint)atomic_swap_32((volatile uint32_t *)ptr, (uint32_t)val);
        break;
    case 8:
        return (MPI_Aint)atomic_swap_64((volatile uint64_t *)ptr, (uint64_t)val);
        break;
    default:
        /* FIXME: A compile-time check for this would be better. */
        MPIU_Assertp(0);
        break;
    }
}

#define MPIDU_Shm_write_barrier()      membar_producer()
#define MPIDU_Shm_read_barrier()       membar_consumer()
#define MPIDU_Shm_read_write_barrier() do { membar_consumer(); membar_producer(); } while (0)

#endif /* MPIDU_ATOMICS_SUN_ATOMIC_OPS_H_INCLUDED */
