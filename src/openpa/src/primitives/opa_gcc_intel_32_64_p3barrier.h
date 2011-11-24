/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_GCC_INTEL_32_64_P3BARRIER_H_INCLUDED
#define OPA_GCC_INTEL_32_64_P3BARRIER_H_INCLUDED

/* Some Pentium III and earlier processors have store barriers
   (sfence), but no full barrier or load barriers.  Some other
   x86-like processors don't seem to have sfence either. The lock
   prefix orders loads and stores (essentially doing a full
   barrier). We force everything to a full barrier for compatibility
   with such processors. */

#define OPA_write_barrier OPA_read_write_barrier
#define OPA_read_barrier OPA_read_write_barrier

static inline void OPA_read_write_barrier(void) {
    int a = 0;
    __asm__ __volatile__ ("lock orl $0, %0" : "+m" (a));
}

#endif /* OPA_GCC_INTEL_32_64_P3BARRIER_H_INCLUDED */
