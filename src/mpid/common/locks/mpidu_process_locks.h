/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDU_PROCESS_LOCKS_H
#define MPIDU_PROCESS_LOCKS_H

/* FIXME: This inclusion is disgusting but necessary for now.  Including
 * mpidimpl.h will get us USE_BUSY_LOCKS from the ssm channel (or any other
 * device that defines it), that's why we do it.  Unfortunately, it sucks in the
 * whole MPI headers ball of wax at the same time, and implies a potential
 * circular dependency.  Doing the right thing here basically means a total gut
 * job, which is out of scope at this time. [goodell@ 2008-03-19] */

/* FIXME: It should be sufficient to include mpidpre.h (updating the ssm etc
   devices if necessary).  That's still not great, but better than all of 
   mpidimpl.h, which should not be used outside of the device code. 
   WDG - 8/8/08 */
#include "mpidimpl.h"

/* FIXME: Including mpidimpl.h includes mpiimpl.h, which is a superset of 
   mpishared.h.  mpishared.h should only be included when the regular mpiimpl.h
   headers are not used. */
/* #include "mpishared.h" */
#include "mpid_locksconf.h"

/* XXX DJG TODO eliminate any atomic operations assembly in here and convert it
   to using the OPA_* functions. */

/* This is used to quote a name in a definition (see FUNCNAME/FCNAME below) */
#ifndef MPIDI_QUOTE
#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A
#endif

/*#include <stdio.h>*/

/* FIXME: First use the configure ifdefs to decide on an approach for 
   locks.  Then put all lock code in one place, or at least guarded by
   the same "USE_xxx" ifdef.  It is nearly impossible with the current code
   to determine, for example, what is the definition of MPIDU_Process_lock_t.
   (Specifically, for the Intel compiler on an x86, it appears to be
   missing a volatile, needed when using the _InterlockedExchange inline 
   function
*/

/* Determine the lock type to use */
#if defined(HAVE_SPARC_INLINE_PROCESS_LOCKS)
#define USE_SPARC_ASM_LOCKS
#elif defined(HAVE_NT_LOCKS)
#define USE_NT_LOCKS
#elif defined(HAVE_MUTEX_INIT)
#define USE_SUN_MUTEX
#elif defined(HAVE_PTHREAD_H)
#define USE_PTHREAD_LOCKS
#else
#error Locking functions not defined
#endif

/* Temp name shift for backward compatibility.  The new name doesn't have
   exactly the same meaning, but the old one was misnamed */
#ifdef USE_BUSY_LOCKS
#define USE_INLINE_LOCKS
#endif

/* 
 * Define MPIDU_Yield() 
 * This is always defined as a macro for now
 */
#ifdef HAVE_YIELD
#define MPIDU_Yield() yield()
#elif defined(HAVE_WIN32_SLEEP)
#define MPIDU_Yield() Sleep(0)
#elif defined (HAVE_SCHED_YIELD)
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#define MPIDU_Yield() sched_yield()
#elif defined (HAVE_SELECT)
#define MPIDU_Yield() { struct timeval t; t.tv_sec = 0; t.tv_usec = 0; select(0,0,0,0,&t); }
#elif defined (HAVE_USLEEP)
#define MPIDU_Yield() usleep(0)
#elif defined (HAVE_SLEEP)
#define MPIDU_Yield() sleep(0)
#else
#error *** No yield function specified ***
#endif

/* There are several cases.
 * First, inline or routine.  If inline, define them here.
 */


#if defined(USE_INLINE_LOCKS) 
/* These are definitions that are intended to inline simple code that
   manages a lock between processes.  It may exploit assembly language
   on systems that provide asm() extensions to the C compiler */

#if defined(USE_BUSY_LOCKS)
typedef volatile long MPIDU_Process_lock_t;
/* FIXME: This uses an invalid prefix */
extern int MPIU_g_nLockSpinCount;

/* We need an atomic "test and set if clear" operation.  The following
   definitions are used to create that.  The operation itself
   is MPID_ATOMIC_SET_IF_ZERO */

#ifdef HAVE_GCC_AND_PENTIUM_ASM
#define HAVE_COMPARE_AND_SWAP
static inline char
__attribute__ ((unused))
     compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  char ret;
  long int readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
                : "=q" (ret), "=m" (*p), "=a" (readval)
            : "r" (newval), "m" (*p), "a" (oldval) : "memory");
  return ret;
}
#endif

#ifdef HAVE_GCC_AND_X86_64_ASM
#define HAVE_COMPARE_AND_SWAP
static inline char
__attribute__ ((unused))
     compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  char ret;
  long int readval;

  __asm__ __volatile__ ("lock; cmpxchgq %3, %1; sete %0"
                : "=q" (ret), "=m" (*p), "=a" (readval)
            : "r" (newval), "m" (*p), "a" (oldval) : "memory");
  return ret;
}
#endif

#ifdef HAVE_GCC_AND_IA64_ASM
#define HAVE__INTERLOCKEDEXCHANGE 1
/* Make sure that the type here is compatible with the use and with 
 * MPIDU_Process_lock_t */
static inline unsigned long _InterlockedExchange(volatile long *ptr, unsigned long x)
{
   unsigned long result;
   __asm__ __volatile ("xchg4 %0=[%1],%2" : "=r" (result) : "r" (ptr), "r" (x) : "memory");
   return result;
}
#endif

#ifdef HAVE_ICC_AND_IA64
#define HAVE__INTERLOCKEDEXCHANGE 1
#include <ia64intrin.h>
#define _InterlockedExchange(ptr,x) _InterlockedExchange(ptr,x)
#endif

/* Given the possible atomic operations, define the set if zero operation.
   This operation returns true if the value was zero.  */
#ifdef HAVE_INTERLOCKEDEXCHANGE
#define MPID_ATOMIC_SET_IF_ZERO(_lock) \
    (InterlockedExchange((LPLONG)_lock, 1) == 0)
#elif defined(HAVE__INTERLOCKEDEXCHANGE)
	/* The Intel compiler complains if the lock is cast to
	 * volatile void * (the type of lock is probably
	 * volatile long *).  The void * works for the Intel 
	 * compiler. */
#define MPID_ATOMIC_SET_IF_ZERO(_lock) \
    (_InterlockedExchange((void *)lock, 1) == 0)
#elif defined(HAVE_COMPARE_AND_SWAP)
#define MPID_ATOMIC_SET_IF_ZERO(_lock) (compare_and_swap(_lock, 0, 1) == 1)
#else
#error Cannot define atomic set flag if zero operation; needed for busy locks
#endif

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    *(lock) = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

/* 
 * This routine requires an atomic "test and set if clear" operation
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    for (;;) {
        for (i=0; i<MPIU_g_nLockSpinCount; i++) {
            if (*lock == 0) {
		if (MPID_ATOMIC_SET_IF_ZERO(lock)) {
                    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
                    return;
                }
            }
        }
        MPIDU_Yield();
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    *(lock) = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_busy_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock )
{
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    for (;;)
    {
        for (i=0; i<MPIU_g_nLockSpinCount; i++)
            if (!*lock)
            {
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
                return;
            }
        MPIDU_Yield();
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}

/* End of USE_BUSY_LOCKS */
#elif defined(USE_SUN_MUTEX)
#include <synch.h>
/* This may allow the use of Sun Mutex as an inline */
typedef mutex_t                 MPIDU_Process_lock_t;
#define MPIDU_Process_lock_init(lock)   mutex_init(lock,USYNC_PROCESS,(void *)0)
#define MPIDU_Process_lock(lock)        mutex_lock(lock)
#define MPIDU_Process_unlock(lock)      mutex_unlock(lock)
#define MPIDU_Process_lock_free(lock)   mutex_destroy(lock)
/* End of case USE_SUN_MUTEX */
#endif /* USE_BUSY_LOCKS */
/* End of USE_INLINE_LOCKS */
#else
/* In the case where we use the routines in mpidu_process_locks.c , 
   we only need to define the type for the lock */
#if defined(USE_SPARC_ASM_LOCKS)
typedef int MPIDU_Process_lock_t;
#elif defined(USE_NT_LOCKS)
#include <winsock2.h>
#include <windows.h>
typedef HANDLE MPIDU_Process_lock_t;
#elif defined(USE_SUN_MUTEX)
#include <synch.h>
typedef mutex_t MPIDU_Process_lock_t;
#elif defined(USE_PTHREAD_LOCKS)
#include <pthread.h>
typedef pthread_mutex_t MPIDU_Process_lock_t;  

#endif /* Case on lock type */
void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock );
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock );
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock );
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock );
void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock );

#endif /* (of else defined) USE_INLINE_LOCKS */

/* If no one uses this, then remove it.  We probably do not want a "lock"
   as an argument in any case (use a common lock or no lock at all) */
#if 0

#undef FUNCNAME
#define FUNCNAME MPIDU_Compare_swap
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
   MPIDU_Compare_swap - 

   Parameters:
+  void **dest
.  void *new_val
.  void *compare_val
.  MPIDU_Process_lock_t *lock
-  void **original_val

   Notes:
@*/
static inline int MPIDU_Compare_swap( void **dest, void *new_val, void *compare_val,            
                        MPIDU_Process_lock_t *lock, void **original_val )
{
    /* dest = pointer to value to be checked (address size)
       new_val = value to set dest to if *dest == compare_val
       original_val = value of dest prior to this operation */

    MPIDI_STATE_DECL(MPID_STATE_MPIDU_COMPARE_SWAP);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_COMPARE_SWAP);
#ifdef HAVE_NT_LOCKS
    MPIU_UNREFERENCED_ARG(lock);
    /**original_val = InterlockedCompareExchange(dest, new_val, compare_val);*/
    /**original_val = (void*)InterlockedCompareExchange((LONG*)dest, (LONG)new_val, (LONG)compare_val);*/
    *original_val = (void*)InterlockedCompareExchangePointer(dest, new_val, compare_val);
#elif defined(HAVE_COMPARE_AND_SWAP)
    MPIU_UNREFERENCED_ARG(lock);
    if (compare_and_swap((volatile long *)dest, (long)compare_val, (long)new_val))
        *original_val = new_val;
#elif defined(HAVE_SPARC_INLINE_PROCESS_LOCKS) || defined(HAVE_PTHREAD_H) || defined(HAVE_MUTEX_INIT)
    MPIDU_Process_lock( lock );

    *original_val = *dest;
    
    if ( *dest == compare_val )
        *dest = new_val;

    MPIDU_Process_unlock( lock );
#else
#error *** No locking functions specified ***
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_COMPARE_SWAP);
    return 0;
}
#endif /* 0 for compareSwap */

#endif /* MPIDU_PROCESS_LOCKS_H */
