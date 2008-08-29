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

/* conditionally include the HP atomic operations library based on a flag
 * determined by configure */
#if defined(HAVE_HP_ATOMIC_OPERATIONS)
#  include "atomic_ops.h"
#endif


/*
    Used to provide atomic operation emulation.  See MPIDU_Interprocess_lock_init()
    for initialization of this variable.
*/
extern MPIDU_Process_lock_t *emulation_lock;
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

/* function prototypes for mpidu_atomic.c */
void MPIDU_Atomic_add_emulated(int *ptr, int val);
int *MPIDU_Atomic_cas_int_ptr_emulated(volatile int **ptr, int *oldv, int *newv);
int MPIDU_Atomic_cas_int_emulated(volatile int *ptr, int oldv, int newv);
MPI_Aint MPIDU_Atomic_cas_aint_emulated(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv);
int MPIDU_Atomic_decr_and_test_emulated(volatile int *ptr);
void MPIDU_Atomic_decr_emulated(volatile int *ptr);
int MPIDU_Atomic_fetch_and_add_emulated(volatile int *ptr, int val);
int MPIDU_Atomic_fetch_and_decr_emulated(volatile int *ptr);
int MPIDU_Atomic_fetch_and_incr_emulated(volatile int *ptr);
void MPIDU_Atomic_incr_emulated(volatile int *ptr);
int *MPIDU_Atomic_swap_int_ptr_emulated(volatile int **ptr, int *val);
int MPIDU_Atomic_swap_int_emulated(volatile int *ptr, int val);
MPI_Aint MPIDU_Atomic_swap_aint_emulated(volatile MPI_Aint *ptr, MPI_Aint val);

/*
    These macros are analogous to the MPIDU_THREAD_XXX_CS_{ENTER,EXIT} macros.
    TODO Consider putting debugging macros in here that utilize 'msg'.
*/
#define MPIDU_IPC_SINGLE_CS_ENTER(msg)     \
    do {                                   \
        MPIU_Assert(emulation_lock);       \
        MPIDU_Process_lock(emulation_lock); \
    } while (0)

#define MPIDU_IPC_SINGLE_CS_EXIT(msg)        \
    do {                                     \
        MPIU_Assert(emulation_lock);         \
        MPIDU_Process_unlock(emulation_lock); \
    } while (0)

#if defined(HAVE_SCHED_YIELD)
#  include <sched.h>
#  define MPIDU_Atomic_busy_wait() sched_yield()
#else
#  define MPIDU_Atomic_busy_wait() do { } while (0)
#endif

/*
    Primitive atomic functions
    --------------------------
    These should not be used directly except by the abstractions defined later
    in this file.  Please note that mixing and matching emulated and
    non-emulated primitives on the same data will likely lead to trouble.  This
    is because the emulation is performed via an inter-process critical section
    mutex, yet a native cmpxchg instruction (or similar) will not respect this
    critical section.

    To help make appropriate algorithmic choices in the higher level
    abstractions, we define cpp macros of the form
    ATOMIC_<OPERATION>_IS_EMULATED where <OPERATION> is the capitalized version
    of the unique part of the routine name.

    A number of these routines have fall-back implementations that are built on
    other non-emulated MPIDU_Atomic operations.  These functions use the
    _IS_EMULATED macros to determine if the routine they depend on is emulated
    and set their own _IS_EMULATED macro correspondingly.
*/

/*
   load-link/store-conditional (LL/SC) primitives.  These are put here near the
   top because we will likely implement several of the other primitives in terms
   of these two primitives.  These are slightly different than the other
   primitives, since they are virtually impossible to emulate in a sensible
   manner.  As such, calls to these functions on plaforms that do not support
   them will result in an abort.
*/
/* Beause Load-Link/Store-Conditional is one low-level approach to
   providing atomic operations but is not (usually) appropriate when
   the underlying hardware doesn't support it, we only define the LL/SC
   routines if they can be supported */
#if defined(HAVE_GCC_AND_POWERPC_ASM)
#define ATOMIC_LL_SC_SUPPORTED
#endif

#ifdef ATOMIC_LL_SC_SUPPORTED

static inline int MPIDU_Atomic_LL_int(volatile int *ptr)
{
#if defined(HAVE_GCC_AND_POWERPC_ASM)
    int val;
    __asm__ __volatile__ ("lwarx %[val],0,%[ptr]"
                          : [val] "=r" (val)
                          : [ptr] "r" (ptr)
                          : "cc");

    return val;
#else
    MPIU_Assertp(0); /*can't emulate*/
    return 0;
#endif
}

/* Returns non-zero if the store was successful, zero otherwise. */
static inline int MPIDU_Atomic_SC_int(volatile int *ptr, int val)
{
#if defined(HAVE_GCC_AND_POWERPC_ASM)
    int ret;
    __asm__ __volatile__ ("stwcx. %[val],0,%[ptr];\n"
                          "beq 1f;\n"
                          "li %[ret], 0;\n"
                          "1: ;\n"
                          : [ret] "=r" (ret)
                          : [ptr] "r" (ptr), [val] "r" (val), "0" (ret)
                          : "cc", "memory");
    return ret;
#else
    MPIU_Assertp(0); /*can't emulate*/
    return 0;
#endif
}


/*
   Pointer versions of LL/SC.  For now we implement them in terms of the integer
   versions with some casting.  If/when we encounter a platform with LL/SC and
   differing word and pointer widths we can write separate versions.
*/

static inline int *MPIDU_Atomic_LL_int_ptr(volatile int **ptr)
{
#if defined(ATOMIC_LL_SC_SUPPORTED)
    switch (sizeof(int *)) { /* should be optimized away */
        case sizeof(int):
            return MPIDU_Atomic_LL_int((int)ptr);
            break;
        default:
            MPIU_Assertp(0); /* need to implement a separate ptr-sized version */
            return NULL; /* placate the compiler */
    }
#else
    MPIU_Assertp(0); /*can't emulate*/
    return NULL;
#endif
}

/* Returns non-zero if the store was successful, zero otherwise. */
static inline int MPIDU_Atomic_SC_int_ptr(volatile int **ptr, int *val)
{
#if defined(ATOMIC_LL_SC_SUPPORTED)
    switch (sizeof(int *)) { /* should be optimized away */
        case sizeof(int):
            return MPIDU_Atomic_SC_int((int *)ptr, (int)val);
            break;
        default:
            MPIU_Assertp(0); /* need to implement a separate ptr-sized version */
            return NULL; /* placate the compiler */
    }
#else
    MPIU_Assertp(0); /*can't emulate*/
    return 0;
#endif
}
#endif /* ATOMIC_LL_SC_SUPPORTED */

static inline void MPIDU_Atomic_add(int *ptr, int val)
{
#if  defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    __asm__ __volatile__ ("lock ; add %1,%0"
                          :"=m" (*ptr)
                          :"ir" (val), "m" (*ptr));
    return;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_ADD_IS_EMULATED
    MPIDU_Atomic_add_emulated(ptr, val);
#endif
}

/* TODO expand this function to encompass non-intel platforms as well */
static inline int *MPIDU_Atomic_cas_int_ptr(int * volatile *ptr, int *oldv, int *newv)
{
/*#if defined(AO_HAVE_compare_and_swap)
    --------------------------------------------------------------------------
    NOTE we would use AO_compare_and_swap here except that it is "broken".  It
    does not return the value that was read, but instead returns a boolean
    that indicates whether the swap was successful.  We need the read-value
    behavior.

    Besides that, the AO lib can decide to emulate CAS without giving any
    indication that it has done so.  CAS emulation by the lib will _NOT_ work
    because it only provides inter-thread safety, not inter-process safety.
    --------------------------------------------------------------------------
*/
#if defined(HAVE_GCC_AND_PENTIUM_ASM)
    void *prev;
    __asm__ __volatile__ ("lock ; cmpxchgl %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv));
    return prev;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    void *prev;
    __asm__ __volatile__ ("lock ; cmpxchgq %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv));
    return prev;   
#elif defined(HAVE_GCC_AND_IA64_ASM)
    void *prev;
    __asm__ __volatile__ ("mov ar.ccv=%1;;"
                          "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                          : "=r"(prev), "=m"(*ptr)
                          : "rO"(oldv), "r"(ptr), "r"(newv));
    return prev;   
#elif defined(ATOMIC_LL_SC_SUPPORTED)
    void *prev;
    do {
        prev = MPIDU_Atomic_LL_int_ptr(ptr);
    } while (prev == oldv && !MPIDU_Atomic_SC_int_ptr(ptr, newv));
    return prev;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_CAS_INT_PTR_IS_EMULATED
    return MPIDU_Atomic_cas_int_ptr_emulated(ptr, oldv, newv);
#endif
}

/*
    Same as _cas_int_ptr() above, but with a different datatype, so we implement
    this one by casting and calling the other.  It would be nice if we could
    implement this once via a (void**) argument, but since you can't deref a
    (void **) we run into problems with that strategy.  The wrong type of
    casting then results in the compiler reordering your instructions and you
    end up with a bug.
*/
static inline char *MPIDU_Atomic_cas_char_ptr(char * volatile *ptr, char *oldv, char *newv)
{
#if defined(ATOMIC_CAS_INT_PTR_IS_EMULATED)
#  define ATOMIC_CAS_CHAR_PTR_IS_EMULATED
#endif
    return (char *)MPIDU_Atomic_cas_int_ptr((int * volatile *)ptr, (int *)oldv, (int *)newv);
}

/* TODO expand this function to encompass non-intel platforms as well */
static inline int MPIDU_Atomic_cas_int(volatile int *ptr, int oldv, int newv)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM)
    int prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv)
                          : "memory");
    return prev;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    int prev;
    __asm__ __volatile__ ("lock ; cmpxchg %2,%3"
                          : "=a" (prev), "=m" (*ptr)
                          : "q" (newv), "m" (*ptr), "0" (oldv)
                          : "memory");
    return prev;   
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int prev;

    switch (sizeof(int)) /* this switch statement should be optimized out */
    {
    case 8:
        __asm__ __volatile__ ("mov ar.ccv=%1;;"
                              "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
                              : "rO"(oldv), "r"(ptr), "r"(newv)
                              : "memory");
        break;
    case 4:
        __asm__ __volatile__ ("zxt4 %1=%1;;" /* don't want oldv sign-extended to 64 bits */
			      "mov ar.ccv=%1;;"
			      "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                              : "=r"(prev), "=m"(*ptr)
			      : "r0"(oldv), "r"(ptr), "r"(newv)
                              : "memory");
        break;
    default:
        MPIU_Assertp (0);
    }
    
    return prev;   
#elif defined(ATOMIC_LL_SC_SUPPORTED)
    void *prev;
    do {
        prev = MPIDU_Atomic_LL_int(ptr);
    } while (prev == oldv && !MPIDU_Atomic_SC_int(ptr, newv));
    return prev;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_CAS_INT_IS_EMULATED
    return MPIDU_Atomic_cas_int_emulated(ptr, oldv, newv);
#endif
}

/* TODO expand this function to encompass non-intel platforms as well */
static inline MPI_Aint MPIDU_Atomic_cas_aint(volatile MPI_Aint *ptr, MPI_Aint oldv, MPI_Aint newv)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    int prev;
    switch (sizeof(MPI_Aint)) {
        case 4:
            __asm__ __volatile__ ("lock ; cmpxchgl %2,%3"
                                  : "=a" (prev), "=m" (*ptr)
                                  : "q" (newv), "m" (*ptr), "0" (oldv)
                                  : "memory");
            break;
        case 8:
            __asm__ __volatile__ ("lock ; cmpxchgq %2,%3"
                                  : "=a" (prev), "=m" (*ptr)
                                  : "q" (newv), "m" (*ptr), "0" (oldv)
                                  : "memory");
            break;
        default:
            MPIU_Assertp (0);
            break;
    }
    return prev;   
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int prev;

    switch (sizeof(MPI_Aint)) /* this switch statement should be optimized out */
    {
        case 8:
            __asm__ __volatile__ ("mov ar.ccv=%1;;"
                                  "cmpxchg8.rel %0=[%3],%4,ar.ccv"
                                  : "=r"(prev), "=m"(*ptr)
                                  : "rO"(oldv), "r"(ptr), "r"(newv)
                                  : "memory");
            break;
        case 4:
            __asm__ __volatile__ ("zxt4 %1=%1;;" /* don't want oldv sign-extended to 64 bits */
                                  "mov ar.ccv=%1;;"
                                  "cmpxchg4.rel %0=[%3],%4,ar.ccv"
                                  : "=r"(prev), "=m"(*ptr)
                                  : "r0"(oldv), "r"(ptr), "r"(newv)
                                  : "memory");
            break;
        default:
            MPIU_Assertp (0);
            break;
    }
    
    return prev;   
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_CAS_AINT_IS_EMULATED
    return MPIDU_Atomic_cas_aint_emulated(ptr, oldv, newv);
#endif
}

static inline void MPIDU_Atomic_decr(volatile int *ptr)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    int val;
    __asm__ __volatile__ ("fetchadd4.rel %0=[%2],%3"
                          : "=r"(val), "=m"(*ptr)
                          : "r"(ptr), "i"(-1));
    return;
#elif AO_HAVE_fetch_and_add
    AO_fetch_and_add(ptr, -1);
    return;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_DECR_IS_EMULATED
    MPIDU_Atomic_decr_emulated(ptr);
#endif
}

/* This routine may seem a bit esoteric, but it's a common operation for
   reference counting and it can be implemented very efficiently on some
   platforms (like x86) compared to a fetch_and_decr or CAS. */
static inline int MPIDU_Atomic_decr_and_test(volatile int *ptr)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    int result;
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; decl %0; setz %1"
                              :"=m" (*ptr), "=q" (result)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; decq %0; setz %1"
                              :"=m" (*ptr), "=q" (result)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return result;
#elif defined(ATOMIC_LL_SC_SUPPORTED)
    int tmp, result;
    do {
        tmp = MPIDU_Atomic_LL_int(ptr);
        --tmp;
    } while (!MPIDU_Atomic_SC_int(ptr, tmp));
    return (0 == tmp);
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_DECR_AND_TEST_IS_EMULATED
    return MPIDU_Atomic_decr_and_test_emulated(ptr);
#endif
}

/* TODO implement a fetchaddN-based version for IA64 */
static inline int MPIDU_Atomic_fetch_and_add(volatile int *ptr, int val)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    __asm__ __volatile__ ("lock ; xadd %0,%1"
                          : "=r" (val), "=m" (*ptr)
                          :  "0" (val),  "m" (*ptr));
    return val;
#elif defined(AO_HAVE_fetch_and_add)
    return AO_fetch_and_add(ptr, val);
#else
#  if defined(ATOMIC_CAS_INT_IS_EMULATED)
#    define USE_ATOMIC_EMULATION
#    define ATOMIC_FETCH_AND_ADD_IS_EMULATED
    /* CAS is emulated, might as well go straight to emulation */
    return MPIDU_Atomic_fetch_and_add_emulated(ptr, val);
#  else
    /* emulate using a real CAS operation */
    int newv;
    int prev;
    int oldv;
    
    do {
        oldv = *ptr;
        newv = oldv + val;
        prev = MPIDU_Atomic_cas_int(ptr, oldv, newv);
    } while (prev != oldv);
    return newv;
#  endif
#endif
}

/* TODO implement a fetchaddN-based version for IA64 */
static inline int MPIDU_Atomic_fetch_and_decr(volatile int *ptr)
{
#if defined(ATOMIC_FETCH_AND_ADD_IS_EMULATED)
#  define USE_ATOMIC_EMULATION /* belt and suspenders */
#  define ATOMIC_FETCH_AND_DECR_IS_EMULATED
    return MPIDU_Atomic_fetch_and_decr_emulated(ptr);
#else
    /* non-emulated _fetch_and_add means we can avoid emulation */
    return MPIDU_Atomic_fetch_and_add(ptr, -1);
#endif
}

/* TODO implement a fetchaddN-based version for IA64 */
static inline int MPIDU_Atomic_fetch_and_incr(volatile int *ptr)
{
#if defined(ATOMIC_FETCH_AND_ADD_IS_EMULATED)
#  define USE_ATOMIC_EMULATION /* belt and suspenders */
#  define ATOMIC_FETCH_AND_INCR_IS_EMULATED
    return MPIDU_Atomic_fetch_and_incr_emulated(ptr);
#else
    /* non-emulated _fetch_and_add means we can avoid emulation */
    return MPIDU_Atomic_fetch_and_add(ptr, 1);
#endif
}

/* TODO implement a fetchaddN-based version for IA64 */
static inline void MPIDU_Atomic_incr(volatile int *ptr)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    switch(sizeof(*ptr))
    {
    case 4:
        __asm__ __volatile__ ("lock ; incl %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    case 8:
        __asm__ __volatile__ ("lock ; incq %0"
                              :"=m" (*ptr)
                              :"m" (*ptr));
        break;
    default:
        /* int is not 64 or 32 bits  */
        MPIU_Assert(0);
    }
    return;
#elif defined(ATOMIC_LL_SC_SUPPORTED)
    /* we have LL/SC but not a native atomic incr, so we'll use LL/SC */
    int tmp;
    do {
        tmp = MPIDU_Atomic_LL_int(ptr);
        ++tmp;
    } while (!MPIDU_Atomic_SC_int(ptr));
    return;
#elif !defined(ATOMIC_CAS_INT_IS_EMULATED)
    /* we have a native CAS but not a native atomic incr, so we'll use CAS */
    int oldv, newv;
    do {
        oldv = *ptr;
        newv = oldv + 1;
    } while (oldv != MPIDU_Atomic_cas_int(ptr, oldv, newv));
    return;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_INCR_IS_EMULATED
    MPIDU_Atomic_incr_emulated(ptr);
#endif
}

static inline int *MPIDU_Atomic_swap_int_ptr(int * volatile *ptr, int *val)
{
#ifdef HAVE_GCC_AND_PENTIUM_ASM
    __asm__ __volatile__ ("xchgl %0,%1"
                          :"=r" (val), "=m" (*ptr)
                          : "0" (val),  "m" (*ptr));
    return val;
#elif defined(HAVE_GCC_AND_X86_64_ASM)
    __asm__ __volatile__ ("xchgq %0,%1"
                          :"=r" (val), "=m" (*ptr)
                          : "0" (val),  "m" (*ptr));
    return val;
#elif defined(HAVE_GCC_AND_IA64_ASM)
    /* is pointer swizzling necessary here? */
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*val)
                          : "r" (ptr), "0" (val));
    return val;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_SWAP_INT_PTR_IS_EMULATED
    return MPIDU_Atomic_swap_int_ptr_emulated(ptr, val);
#endif
}

/*
    Implemented via the (int**) version.  See the comment on _cas_char_ptr for
    info on why this is the way it is.
*/
static inline char *MPIDU_Atomic_swap_char_ptr(char * volatile *ptr, char *val)
{
#if defined(ATOMIC_SWAP_INT_PTR_IS_EMULATED)
#  define ATOMIC_SWAP_CHAR_PTR_IS_EMULATED
#endif
    return (char *)(MPIDU_Atomic_swap_int_ptr((int * volatile *)ptr, (int *)val));
}

static inline int MPIDU_Atomic_swap_int(volatile int *ptr, int val)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    switch(sizeof(*ptr)) {
        case 4:
            __asm__ __volatile__ ("xchgl %0,%1"
                                  :"=r" (val), "=m" (*ptr)
                                  : "0" (val),  "m" (*ptr));
            return val;
            break;
        case 8:
            __asm__ __volatile__ ("xchgq %0,%1"
                                  :"=r" (val), "=m" (*ptr)
                                  : "0" (val),  "m" (*ptr));
            return val;
            break;
        default:
            /* int is not 64 or 32 bits  */
            MPIU_Assert(0);
            return val; /* placate the compiler */
            break;
    }
#elif defined(HAVE_GCC_AND_IA64_ASM)
    /* is pointer swizzling necessary here? */
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*val)
                          : "r" (ptr), "0" (val));
    return val;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_SWAP_INT_IS_EMULATED
    return MPIDU_Atomic_swap_int_emulated(ptr, val);
#endif
}

static inline MPI_Aint MPIDU_Atomic_swap_aint(volatile MPI_Aint *ptr, MPI_Aint val)
{
#if defined(HAVE_GCC_AND_PENTIUM_ASM) || defined(HAVE_GCC_AND_X86_64_ASM)
    switch(sizeof(*ptr)) {
        case 4:
            __asm__ __volatile__ ("xchgl %0,%1"
                                  :"=r" (val), "=m" (*ptr)
                                  : "0" (val),  "m" (*ptr));
            return val;
            break;
        case 8:
            __asm__ __volatile__ ("xchgq %0,%1"
                                  :"=r" (val), "=m" (*ptr)
                                  : "0" (val),  "m" (*ptr));
            return val;
            break;
        default:
            /* int is not 64 or 32 bits  */
            MPIU_Assert(0);
            return val; /* placate the compiler */
            break;
    }
#elif defined(HAVE_GCC_AND_IA64_ASM)
    /* is pointer swizzling necessary here? */
    __asm__ __volatile__ ("xchg8 %0=[%2],%3"
                          : "=r" (val), "=m" (*val)
                          : "r" (ptr), "0" (val));
    return val;
#else /* "lock-op-unlock" fallback */
#  define USE_ATOMIC_EMULATION
#  define ATOMIC_SWAP_AINT_IS_EMULATED
    return MPIDU_Atomic_swap_aint_emulated(ptr, val);
#endif
}

#endif /* defined(MPIDU_ATOMIC_PRIMITIVES_H_INCLUDED) */
