/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_GCC_INTEL_32_64_BARRIER_H_INCLUDED
#define OPA_GCC_INTEL_32_64_BARRIER_H_INCLUDED

#define OPA_write_barrier()      __asm__ __volatile__  ( "sfence" ::: "memory" )
#define OPA_read_barrier()       __asm__ __volatile__  ( "lfence" ::: "memory" )
#define OPA_read_write_barrier() __asm__ __volatile__  ( "mfence" ::: "memory" )


#endif /* OPA_GCC_INTEL_32_64_BARRIER_H_INCLUDED */
