/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_GCC_INTEL_32_64_P3BARRIER_H_INCLUDED
#define OPA_GCC_INTEL_32_64_P3BARRIER_H_INCLUDED

/* Pentium III and earlier processors have store barriers (sfence),
   but no full barrier or load barriers.  However, the lock prefix
   orders loads and stores (essentially doing a full barrier).  */

#define OPA_write_barrier() __asm__ __volatile__ ( "sfence" ::: "memory" )

static inline void OPA_read_barrier(void) {
    int a = 0;
    __asm__ __volatile__ ("lock orl $0, %0" : "+m" (a));
}

static inline void OPA_read_write_barrier(void) {
    int a = 0;
    __asm__ __volatile__ ("lock orl $0, %0" : "+m" (a));
}



#endif /* OPA_GCC_INTEL_32_64_P3BARRIER_H_INCLUDED */
