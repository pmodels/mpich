/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED
#define MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED

#include <mpichconf.h>
#include <mpid_locksconf.h>

/* FIXME We need assert capability but we can't rely on MPIU_Assert down at this
   level.  To futher complicate matters this is definitely performance critical
   code so these asserts need to be configureable just as MPIU_Asserts are.
   Just define these asserts to be empty do/while loops for now.
   [goodell@ 2009-01-27]. */
#undef USE_ASSERT_IN_ATOMICS
#if defined(USE_ASSERT_IN_ATOMICS)
#  if defined(HAVE_ASSERT_H)
#    include <assert.h>
#    define MPIDU_Atomic_assert(expr) assert((expr))
#  else
/* Just intentionally deref NULL so that we get a segfault. */
#    define MPIDU_Atomic_assert(expr) do { if (!(expr)) ++(*(int*)NULL); } while (0)
#  endif
#else /* !defined(USE_ASSERT_IN_ATOMICS) */
#  define MPIDU_Atomic_assert(expr) do {} while (0)
#endif


/*
   Primitive atomic functions
   --------------------------

   The included file is responsible for defining the types of MPIDU_Atomic_t and
   MPIDU_Atomic_ptr_t as well as a set of functions for operating on these
   types.  If you have the following declaration:

       MPIDU_Atomic_t atomic_var;

   Then in order for the emulation functions compile the underlying value of
   atomic_var should be accessible via:

       atomic_var.v;

   The same goes for MPIDU_Atomic_ptr_t.
   
   The atomic functions that must be ported for each architecture: 

   static inline int   MPIDU_Atomic_load(MPIDU_Atomic_t *ptr);
   static inline void  MPIDU_Atomic_store(MPIDU_Atomic_t *ptr, int val);
   static inline void *MPIDU_Atomic_load_ptr(MPIDU_Atomic_t *ptr);
   static inline void  MPIDU_Atomic_store_ptr(MPIDU_Atomic_t *ptr, void *val);

   static inline void MPIDU_Atomic_add(MPIDU_Atomic_t *ptr, int val);
   static inline void MPIDU_Atomic_incr(MPIDU_Atomic_t *ptr);
   static inline void MPIDU_Atomic_decr(MPIDU_Atomic_t *ptr);
   
   static inline int MPIDU_Atomic_decr_and_test(MPIDU_Atomic_t *ptr);
   static inline int MPIDU_Atomic_fetch_and_add(MPIDU_Atomic_t *ptr, int val);
   static inline int MPIDU_Atomic_fetch_and_decr(MPIDU_Atomic_t *ptr);
   static inline int MPIDU_Atomic_fetch_and_incr(MPIDU_Atomic_t *ptr);
   
   static inline int *MPIDU_Atomic_cas_ptr(MPIDU_Atomic_ptr_t *ptr, void *oldv, void *newv);
   static inline int  MPIDU_Atomic_cas_int(MPIDU_Atomic_t *ptr, int oldv, int newv);
   
   static inline int *MPIDU_Atomic_swap_ptr(MPIDU_Atomic_ptr_t *ptr, int *val);
   static inline int  MPIDU_Atomic_swap_int(MPIDU_Atomic_t *ptr, int val);
   
   
   The following need to be ported only for architectures supporting LL/SC:
   
   static inline int MPIDU_Atomic_LL_ptr(MPIDU_Atomic_ptr_t *ptr);
   static inline int MPIDU_Atomic_SC_ptr(MPIDU_Atomic_ptr_t *ptr, int *val);
   static inline int MPIDU_Atomic_LL_int(volatile int *ptr);
   static inline int MPIDU_Atomic_SC_int(volatile int *ptr, int val);

   MPIDU_ATOMIC_UNIVERSAL_PRIMITIVE should be defined to be one of:
     MPIDU_ATOMIC_CAS
     MPIDU_ATOMIC_LL_SC
     MPIDU_ATOMIC_NO_UNIVERSAL
*/
#define MPIDU_ATOMIC_NO_UNIVERSAL 0
#define MPIDU_ATOMIC_CAS          1
#define MPIDU_ATOMIC_LL_SC        2

/* Include the appropriate header for the architecture */
#if   defined(HAVE_GCC_AND_POWERPC_ASM)
#include "mpidu_atomics_gcc_ppc.h"
#elif defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
#include "mpidu_atomics_gcc_intel_32_64.h"
#elif defined(HAVE_GCC_AND_IA64_ASM)
#include "mpidu_atomics_gcc_ia64.h"
#elif defined(HAVE_GCC_AND_SICORTEX_ASM)
#include "mpidu_atomics_gcc_sicortex.h"
#elif defined(HAVE_GCC_INTRINSIC_ATOMICS)
#include "mpidu_atomics_gcc_intrinsics.h"
#elif defined(HAVE_SUN_ATOMIC_OPS)
#include "mpidu_atomics_sun_atomic_ops.h"
#elif defined(HAVE_NT_INTRINSICS)
#include "mpidu_atomics_nt_intrinsics.h"
#else
/* No architecture specific atomics, using lock-based emulated
   implementations */

/* This is defined to ensure that interprocess locks are properly
   initialized. */
#define USE_ATOMIC_EMULATION
#include "mpidu_atomics_by_lock.h"
#endif

/*
    This routine is needed because the MPIU_THREAD_XXX_CS_{ENTER,EXIT} macros do
    not provide synchronization across multiple processes, only across multiple
    threads within a process.  In order to safely emulate atomic operations on a
    shared memory region, we need a shared memory backed lock mechanism.

    This routine must be called by any subsystem that intends to use the atomic
    abstractions if the cpp directive USE_ATOMIC_EMULATION is defined.  It must
    be called exactly once by _all_ processes, not just a single leader.  This
    function will initialize the contents of the lock variable if the caller
    specifies (isLeader==true).  Note that multiple initialization is forbidden
    by several lock implementations, especially pthreads.

    Inputs:
      shm_lock - A pointer to an allocated piece of shared memory that can hold
                 an MPIDU_Process_lock_t.  This 
      isLeader - This boolean value should be set to true 
*/
/* FIXME We need to extricate ourselves from MPIDU_Process_locks because they
   are hopelessly broken. */
#if defined(HAVE_PTHREAD_H)
#  include <pthread.h>
int MPIDU_Interprocess_lock_init(pthread_mutex_t *shm_lock, int isLeader);
#endif


/* FIXME This should probably be pushed down into the platform-specific headers. */
#if defined(HAVE_SCHED_YIELD)
#  include <sched.h>
#  define MPIDU_Atomic_busy_wait() sched_yield()
#else
#  define MPIDU_Atomic_busy_wait() do { } while (0)
#endif

#endif /* defined(MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED) */
