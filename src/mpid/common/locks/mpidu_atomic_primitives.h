/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED
#define MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED

#include <mpichconf.h>
#include "mpidu_process_locks.h"
#include "mpibase.h" /* for MPIU_Assert */
#include "mpiutil.h" /* for MPIU_Assert */
#include "mpi.h" /* for MPI_Aint */

/*
    Primitive atomic functions
    --------------------------
    
    These should not be used directly except by the abstractions
    defined later in this file.  Please note that mixing lock-emulated
    and non-lock-emulated primitives on the same data will likely lead
    to trouble, so when porting to a new architecture, it's best not
    to mix the two.  This is because lock-emulation is performed using
    an inter-process critical section mutex, and a native atomic, like
    a cmpxchg instruction, or a regular load or store will not respect
    this critical section.

    To help make appropriate algorithmic choices in the higher level
    abstractions, we define cpp macros of the form
    ATOMIC_<OPERATION>_IS_EMULATED where <OPERATION> is the
    capitalized version of the unique part of the routine name.  There
    is also the USE_ATOMIC_EMULATION macro which is set to indicate
    that at least one atomic has been emulated using locks.

    FIXME: Since we shouldn't mix lock-emulated and non-lock-emulated
    atomics anyway, maybe we just need the one USE_ATOMIC_EMULATION
    macro and not the operation-specific _IS_EMULATED macros.
*/



/* These are the atomic functions that must be ported to other
   architectures. 

   static inline void MPIDU_Atomic_add(volatile int *ptr, int val);
   static inline void MPIDU_Atomic_incr(volatile int *ptr);
   static inline void MPIDU_Atomic_decr(volatile int *ptr);
   
   static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr);
   static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val);
   static inline int MPIDU_Atomic_fetch_and_decr(volatile int *ptr);
   static inline int MPIDU_Atomic_fetch_and_incr(volatile int *ptr);
   
   static inline int      *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv);
   static inline int       MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv);
   static inline MPI_Aint  MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv);
   
   static inline int      *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val);
   static inline int       MPIDU_Atomic_swap_int(volatile int *ptr, int val);
   static inline MPI_Aint  MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val);
   
   
   The following  need to be ported only for architecture supporting LL/SC:
   
   static inline int MPIDU_Atomic_LL_int_ptr(int * volatile *ptr);
   static inline int MPIDU_Atomic_SC_int_ptr(int * volatile *ptr, int *val);
   static inline int MPIDU_Atomic_LL_int(volatile int *ptr);
   static inline int MPIDU_Atomic_SC_int(volatile int *ptr, int val);
   static inline int MPIDU_Atomic_LL_aint(volatile MPI_Aint *ptr);
   static inline int MPIDU_Atomic_SC_aint(volatile MPI_Aint *ptr, MPI_Aint val);
*/

/* Include the appropriate header for the architecture */
#if   defined(HAVE_GCC_AND_POWERPC_ASM)
#include "mpidu_atomics_gcc_ppc.h"
#elif defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
#include "mpidu_atomics_gcc_intel_32_64.h"
#elif defined(HAVE_GCC_AND_IA64_ASM)
#include "mpidu_atomics_gcc_ia64.h"
#elif defined(HAVE_GCC_INTRINSIC_ATOMICS)
#include "mpidu_atomics_gcc_intrinsics.h"
#elif defined(HAVE_SUN_ATOMIC_OPS)
#include "mpidu_atomics_sun_atomic_ops.h"
#else

/* FIXME: In order to support atomic emulated with locks, we need to
   wrap locks around regular loads and stores.  This means that we
   also need these macros/functions to be used in the non-emulated
   cases too (they'll just be null macros).

   We might need such macros even in the non-emulated cases to cast
   to/from datatypes that enforce alignment anyway.
*/

/* No architecture specific atomics, using lock-based emulated
   implementations */

/* This is defined to ensure that interprocess locks are properly
   initialized. */
#define USE_ATOMIC_EMULATION

/* to help make appropriate algorithmic choices in the higher level
   abstractions, the header file must also define a macro of the form
   ATOMIC_<OPERATION>_IS_EMULATED, where <OPERATION> is the
   capitalized version of the unique part of the routine name, to
   indicate that that particular atomic primitive is being emulated
   using locks. */
#define ATOMIC_ADD_IS_EMULATED
#define ATOMIC_INCR_IS_EMULATED
#define ATOMIC_DECR_IS_EMULATED
#define ATOMIC_DECR_AND_TEST_IS_EMULATED
#define ATOMIC_FETCH_AND_ADD_IS_EMULATED
#define ATOMIC_FETCH_AND_DECR_IS_EMULATED
#define ATOMIC_FETCH_AND_INCR_IS_EMULATED
#define ATOMIC_CAS_INT_PTR_IS_EMULATED
#define ATOMIC_CAS_INT_IS_EMULATED
#define ATOMIC_CAS_AINT_IS_EMULATED
#define ATOMIC_SWAP_INT_PTR_IS_EMULATED
#define ATOMIC_SWAP_INT_IS_EMULATED
#define ATOMIC_SWAP_AINT_IS_EMULATED

#define MPIDU_Atomic_add            MPIDU_Atomic_add_by_lock
#define MPIDU_Atomic_incr           MPIDU_Atomic_incr_by_lock
#define MPIDU_Atomic_decr           MPIDU_Atomic_decr_by_lock
#define MPIDU_Atomic_decr_and_test  MPIDU_Atomic_decr_and_test_by_lock
#define MPIDU_Atomic_fetch_and_add  MPIDU_Atomic_fetch_and_add_by_lock
#define MPIDU_Atomic_fetch_and_decr MPIDU_Atomic_fetch_and_decr_by_lock
#define MPIDU_Atomic_fetch_and_incr MPIDU_Atomic_fetch_and_incr_by_lock
#define MPIDU_Atomic_cas_int_ptr    MPIDU_Atomic_cas_int_ptr_by_lock
#define MPIDU_Atomic_cas_int        MPIDU_Atomic_cas_int_by_lock
#define MPIDU_Atomic_cas_aint       MPIDU_Atomic_cas_aint_by_lock
#define MPIDU_Atomic_swap_int_ptr   MPIDU_Atomic_swap_int_ptr_by_lock
#define MPIDU_Atomic_swap_int       MPIDU_Atomic_swap_int_by_lock
#define MPIDU_Atomic_swap_aint      MPIDU_Atomic_swap_aint_by_lock

#endif

/*  Prototypes for emulations using locks in mpidu_atomic_primitives.c */

void MPIDU_Atomic_add_by_lock(int *ptr, int val);
void MPIDU_Atomic_incr_by_lock(volatile int *ptr);
void MPIDU_Atomic_decr_by_lock(volatile int *ptr);

int MPIDU_Atomic_decr_and_test_by_lock(volatile int *ptr);
int MPIDU_Atomic_fetch_and_add_by_lock(volatile int *ptr, int val);
int MPIDU_Atomic_fetch_and_decr_by_lock(volatile int *ptr);
int MPIDU_Atomic_fetch_and_incr_by_lock(volatile int *ptr);

int      *MPIDU_Atomic_cas_int_ptr_by_lock(int * volatile *ptr, int *oldv, int *newv);
char     *MPIDU_Atomic_cas_char_ptr_by_lock(char * volatile *ptr, char *oldv, char *newv);
int       MPIDU_Atomic_cas_int_by_lock(volatile int *ptr, int oldv, int newv);
MPI_Aint  MPIDU_Atomic_cas_aint_by_lock(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv);

int      *MPIDU_Atomic_swap_int_ptr_by_lock(int * volatile *ptr, int *val);
char     *MPIDU_Atomic_swap_char_ptr_by_lock(char * volatile *ptr, char *val);
int       MPIDU_Atomic_swap_int_by_lock(volatile int *ptr, int val);
MPI_Aint  MPIDU_Atomic_swap_aint_by_lock(volatile MPI_Aint *ptr, MPI_Aint val);

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
int MPIDU_Interprocess_lock_init(MPIDU_Process_lock_t *shm_lock, int isLeader);



#if defined(HAVE_SCHED_YIELD)
#  include <sched.h>
#  define MPIDU_Atomic_busy_wait() sched_yield()
#else
#  define MPIDU_Atomic_busy_wait() do { } while (0)
#endif





/* MPIDU_Atomic_cas_char_ptr() and MPIDU_Atomic_swap_char_ptr() are
   the same as the _int_ptr() versions, but with a different datatype.
   So we implement these simply by casting and calling the other.

   It would be nice if we could implement this once via a (void**)
   argument, but since you can't deref a (void **) we run into
   problems with that strategy.  The wrong type of casting then
   results in the compiler reordering your instructions and you end up
   with a bug. */

static inline char *MPIDU_Atomic_cas_char_ptr(char * volatile *ptr, char *oldv, char *newv)
{
#if defined(ATOMIC_CAS_INT_PTR_IS_EMULATED)
#  define ATOMIC_CAS_CHAR_PTR_IS_EMULATED
#endif
    return (char *)MPIDU_Atomic_cas_int_ptr((int * volatile *)ptr, (int *)oldv, (int *)newv);
}

static inline char *MPIDU_Atomic_swap_char_ptr(char * volatile *ptr, char *val)
{
#if defined(ATOMIC_SWAP_INT_PTR_IS_EMULATED)
#  define ATOMIC_SWAP_CHAR_PTR_IS_EMULATED
#endif
    return (char *)(MPIDU_Atomic_swap_int_ptr((int * volatile *)ptr, (int *)val));
}

#endif /* defined(MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED) */
