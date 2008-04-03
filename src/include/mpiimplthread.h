/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIIMPLTHREAD_H_INCLUDED)
#define MPIIMPLTHREAD_H_INCLUDED

#ifndef HAVE_MPICHCONF
#error 'This file requires mpichconf.h'
#endif

/* Rather than embed a conditional test in the MPICH2 code, we define a 
   single value on which we can test */
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
#define MPICH_IS_THREADED 1
#endif

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)    
#include "mpid_thread.h"
#endif

/*
 * Define possible thread implementations that could be selected at 
 * configure time.  
 * 
 * These mean what ?
 */
#define MPICH_THREAD_IMPL_NOT_IMPLEMENTED -1
#define MPICH_THREAD_IMPL_NONE 1
#define MPICH_THREAD_IMPL_GLOBAL_MUTEX 2


typedef struct MPICH_ThreadInfo_t {
    int               thread_provided;  /* Provided level of thread support */
    /* This is a special case for is_thread_main, which must be
       implemented even if MPICH2 itself is single threaded.  */
#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)    
    MPID_Thread_tls_t thread_storage;   /* Id for perthread data */
    MPID_Thread_id_t  master_thread;    /* Thread that started MPI */
#endif
#ifdef HAVE_RUNTIME_THREADCHECK
    int isThreaded;                      /* Set to true if user requested
					    THREAD_MULTIPLE */
#endif
# if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    MPID_Thread_mutex_t global_mutex;
# endif
} MPICH_ThreadInfo_t;
extern MPICH_ThreadInfo_t MPIR_ThreadInfo;

/*
 * Get a pointer to the thread's private data
 * Also define a macro to release any storage that may be allocated
 * by Malloc to ensure that memory leak tools don't report this when
 * there is no real leak.
 */
#ifndef MPICH_IS_THREADED
#define MPIR_GetPerThread(pt_)			\
{						\
    *(pt_) = &MPIR_Thread;			\
}
#define MPIR_ReleasePerThread
#else
/* Define a macro to acquire or create the thread private storage */
#define MPIR_GetOrInitThreadPriv( pt_ ) \
{									\
    MPID_Thread_tls_get(&MPIR_ThreadInfo.thread_storage, (pt_));		\
    if (*(pt_) == NULL)							\
    {									\
	*(pt_) = (MPICH_PerThread_t *) MPIU_Calloc(1, sizeof(MPICH_PerThread_t));	\
	MPID_Thread_tls_set(&MPIR_ThreadInfo.thread_storage, (void *) *(pt_));\
    }									\
    MPIU_DBG_MSG_FMT(THREAD,VERBOSE,(MPIU_DBG_FDEST,\
     "perthread storage (key = %x) is %p", (unsigned int)MPIR_ThreadInfo.thread_storage,*pt_));\
}
/* We want to avoid the overhead of the thread call if we're in the
   runtime state and threads are not in use.  In that case, MPIR_Thread 
   is still a pointer but it was already allocated in InitThread */
#ifdef HAVE_RUNTIME_THREADCHECK
#define MPIR_GetPerThread(pt_) {\
 if (MPIR_ThreadInfo.isThreaded) { MPIR_GetOrInitThreadPriv( pt_ ); } \
 else { *(pt_) = &MPIR_ThreadSingle; } \
 }
/* Note that we set the value on the thread_storage key to zero.  This
   is because we have set an exit handler on this thread key when it
   was created; that handler will try to delete the storage associated
   with that value. */
#define MPIR_ReleasePerThread { \
	if (MPIR_ThreadInfo.isThreaded) { \
         MPICH_PerThread_t *pt_; \
         MPIR_GetOrInitThreadPriv( &pt_ ); MPIU_Free( pt_ ); \
         MPID_Thread_tls_set(&MPIR_ThreadInfo.thread_storage,(void *)0);} }
#else
#define MPIR_GetPerThread(pt_) MPIR_GetOrInitThreadPriv( pt_ )
#define MPIR_ReleasePerThread { \
MPICH_PerThread_t *pt_; MPIR_GetOrInitThreadPriv( &pt_ ); MPIU_Free( pt_ ); }
#endif /* HAVE_RUNTIME_THREADCHECK */

#endif /* MPICH_IS_THREADED */


/*
 * Define MPID Critical Section macros, unless the device will be defining them
 */
#if !defined(MPID_DEFINES_MPID_CS)
#ifndef MPICH_IS_THREADED
#define MPID_CS_INITIALIZE()
#define MPID_CS_FINALIZE()
#define MPID_CS_ENTER()
#define MPID_CS_EXIT()
#elif (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
/* Function prototype (needed when no weak symbols available) */
void MPIR_CleanupThreadStorage( void *a );

/* FIXME: The "thread storage" needs to be moved out of this */
#define MPID_CS_INITIALIZE()						\
{									\
    MPID_Thread_mutex_create(&MPIR_ThreadInfo.global_mutex, NULL);		\
    MPID_Thread_tls_create(MPIR_CleanupThreadStorage, &MPIR_ThreadInfo.thread_storage, NULL);   \
    MPIU_DBG_MSG(THREAD,TYPICAL,"Created global mutex and private storage");\
}
#define MPID_CS_FINALIZE()						\
{									\
    MPIU_DBG_MSG(THREAD,TYPICAL,"Freeing global mutex and private storage");\
    MPIR_ReleasePerThread;						\
    MPID_Thread_tls_destroy(&MPIR_ThreadInfo.thread_storage, NULL);	\
    MPID_Thread_mutex_destroy(&MPIR_ThreadInfo.global_mutex, NULL);	\
}
/* FIXME: Figure out what we want to do for the nest count on 
   these routines, so as to avoid extra function calls */
/* 
 * The Nest value must *not* be changed by MPID_CS_ENTER or MPID_CS_EXIT.
 * It is the obligation of the implementation of the MPI routines to 
 * use MPIR_Nest_incr and MPIR_Nest_decr before invoking other MPI routines
 */
#define MPID_CS_ENTER()						\
{								\
    MPIU_THREADPRIV_DECL;                                       \
    MPIU_THREADPRIV_GET;                                        \
    if (MPIR_Nest_value() == 0)					\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Enter global critical section");\
	MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex);	\
    }								\
}
#define MPID_CS_EXIT()						\
{								\
    MPIU_THREADPRIV_DECL;                                       \
    MPIU_THREADPRIV_GET;                                        \
    if (MPIR_Nest_value() == 0)					\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Exit global critical section");\
	MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex);	\
    }								\
}
#else
#error "Critical section macros not defined"
#endif
#endif /* !defined(MPID_DEFINES_MPID_CS) */


#if defined(HAVE_THR_YIELD)
#undef MPID_Thread_yield
#define MPID_Thread_yield() thr_yield()
#elif defined(HAVE_SCHED_YIELD)
#undef MPID_Thread_yield
#define MPID_Thread_yield() sched_yield()
#elif defined(HAVE_YIELD)
#undef MPID_Thread_yield
#define MPID_Thread_yield() yield()
#endif

/*
 * New Thread Interface and Macros
 * See http://www-unix.mcs.anl.gov/mpi/mpich2/developer/design/threads.htm
 * for a detailed discussion of this interface and the rationale.
 * 
 * This code implements the single CS approach, and is similar to 
 * the MPID_CS_ENTER and MPID_CS_EXIT macros above.  
 *
 */

/* Make the CPP definition that will be used to control whether threaded 
   code is supported.  Test ONLY on whether MPICH_IS_THREADED is defined.
*/
#if !defined(MPICH_THREAD_LEVEL) || !defined(MPI_THREAD_MULTIPLE)
#error Internal error in macro definitions in include/mpiimplthread.h
#endif


#ifdef MPICH_IS_THREADED

#ifdef HAVE_RUNTIME_THREADCHECK
#define MPIU_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIU_THREAD_CHECK_END   }  
#else
#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#endif

#define MPIU_ISTHREADED(_s) { MPIU_THREAD_CHECK_BEGIN _s MPIU_THREAD_CHECK_END }

/* SINGLE_CS_DECL needs to take over the decl used by MPID_CS_xxx when that
   is removed */
#define MPIU_THREAD_SINGLE_CS_DECL
#define MPIU_THREAD_SINGLE_CS_INITIALIZE MPID_CS_INITIALIZE()
#define MPIU_THREAD_SINGLE_CS_FINALIZE   MPIU_THREAD_CHECK_BEGIN MPID_CS_FINALIZE(); MPIU_THREAD_CHECK_END
#define MPIU_THREAD_SINGLE_CS_ASSERT_WITHIN(_msg)
#define MPIU_THREAD_SINGLE_CS_ENTER(_msg) MPIU_THREAD_CHECK_BEGIN MPID_CS_ENTER() MPIU_THREAD_CHECK_END
#define MPIU_THREAD_SINGLE_CS_EXIT(_msg)  MPIU_THREAD_CHECK_BEGIN MPID_CS_EXIT() MPIU_THREAD_CHECK_END

/* These provide a uniform way to perform a first-use initialization
   in a thread-safe way.  See the web page or wtime.c */
#define MPIU_THREADSAFE_INIT_DECL(_var) static volatile int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) \
     if (_var) { \
      MPIU_THREAD_SINGLE_CS_ENTER(""); \
      _stmt; _var=0; \
      MPIU_THREAD_SINGLE_CS_EXIT(""); \
     }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var) \
     MPIU_THREAD_SINGLE_CS_ENTER(""); \
     if (_var) {
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var) \
      } \
     MPIU_THREAD_SINGLE_CS_EXIT("")

#else
/* MPICH user routines are single-threaded */
#define MPIU_ISTHREADED(_s) 
#define MPIU_THREAD_SINGLE_CS_DECL
#define MPIU_THREAD_SINGLE_CS_INITIALIZE
#define MPIU_THREAD_SINGLE_CS_FINALIZE
#define MPIU_THREAD_SINGLE_CS_ASSERT_WITHIN(_msg)
#define MPIU_THREAD_SINGLE_CS_ENTER(_msg)
#define MPIU_THREAD_SINGLE_CS_EXIT(_msg)
#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END

/* These provide a uniform way to perform a first-use initialization
   in a thread-safe way.  See the web page or mpidtime.c for the generic
   wtick */
#define MPIU_THREADSAFE_INIT_DECL(_var) static int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) if (_var) { _stmt; _var = 0; }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var) 
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var) 
#endif

#endif /* !defined(MPIIMPLTHREAD_H_INCLUDED) */
