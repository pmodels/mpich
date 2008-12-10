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

/* FIXME: TEMP - should make sure enable-g set this */
/* #define MPID_THREAD_DEBUG */

/* Rather than embed a conditional test in the MPICH2 code, we define a 
   single value on which we can test */
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
#define MPICH_IS_THREADED 1
#endif

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)    
#include "mpid_thread.h"
#endif

/* 
 * Define the four ways that we achieve proper thread-safe updates of 
 * shared structures and services
 * 
 * A configure choice will set MPIU_THREAD_GRANULARITY to one of these values
 */
#define MPIU_THREAD_GRANULARITY_GLOBAL 1
#define MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL 2
#define MPIU_THREAD_GRANULARITY_PER_OBJECT 3
#define MPIU_THREAD_GRANULARITY_LOCK_FREE 4

/*
 * Define possible thread implementations that could be selected at 
 * configure time.  
 * 
 * These mean what ?
 *
 * not_implemented - used in src/mpid/ch3/channels/{sock,sctp}/src/ch3_progress.c 
 * only, and basically commented out
 * none - never used
 * global_mutex - 
 *
 */
/* FIXME: These are old and deprecated */
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

    /* Define the mutex values used for each kind of implementation */
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL || \
    MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL || \
    MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
    /* The global mutex goes here when we eliminate USE_THREAD_IMPL */
    /* We need the handle mutex to avoid problems with lock nesting */
    MPID_Thread_mutex_t handle_mutex;
#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_LOCK_FREE
#error MPIU_THREAD_GRANULARITY_LOCK_FREE not implemented yet
#endif

#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL || \
    MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
    MPID_Thread_tls_t nest_storage;   /* Id for perthread data */
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
    MPID_Thread_tls_get(&MPIR_ThreadInfo.thread_storage, (pt_));	\
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

int MPIR_Thread_CS_Init( void );
int MPIR_Thread_CS_Finalize( void );
#define MPID_CS_INITIALIZE() MPIR_Thread_CS_Init()
#define MPID_CS_FINALIZE() MPIR_Thread_CS_Finalize()

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
	MPIU_THREAD_UPDATEDEPTH(global_mutex,1);                \
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
	MPIU_THREAD_UPDATEDEPTH(global_mutex,-1);               \
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


#if !defined(MPID_DEFINES_MPID_CS)
#ifdef MPICH_IS_THREADED

#ifdef HAVE_RUNTIME_THREADCHECK
#define MPIU_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIU_THREAD_CHECK_END   }  
#else
#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#endif /* HAVE_RUNTIME_THREADCHECK */

#define MPIU_ISTHREADED(_s) { MPIU_THREAD_CHECK_BEGIN _s MPIU_THREAD_CHECK_END }

/* SINGLE_CS_DECL needs to take over the decl used by MPID_CS_xxx when that
   is removed */
#define MPIU_THREAD_SINGLE_CS_DECL
#define MPIU_THREAD_SINGLE_CS_INITIALIZE MPID_CS_INITIALIZE()
/* #define MPIU_THREAD_SINGLE_CS_FINALIZE   MPIU_THREAD_CHECK_BEGIN MPID_CS_FINALIZE(); MPIU_THREAD_CHECK_END */
/* Because we unconditionally invoke the initialize, we need to do the 
   same with the finalize */
#define MPIU_THREAD_SINGLE_CS_FINALIZE   MPID_CS_FINALIZE()
#define MPIU_THREAD_SINGLE_CS_ASSERT_WITHIN(_msg)
/*
#define MPIU_THREAD_SINGLE_CS_ENTER(_msg) MPIU_THREAD_CHECK_BEGIN MPID_CS_ENTER() MPIU_THREAD_CHECK_END
#define MPIU_THREAD_SINGLE_CS_EXIT(_msg)  MPIU_THREAD_CHECK_BEGIN MPID_CS_EXIT() MPIU_THREAD_CHECK_END
*/
#define MPIU_THREAD_SINGLE_CS_ENTER(_msg) ''' remove me '''
#define MPIU_THREAD_SINGLE_CS_EXIT(_msg)  ''' remove me '''

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
#endif  /* MPICH_IS_THREADED */
#endif  /* !defined(MPID_DEFINES_MPID_CS) */

/* ------------------------------------------------------------------------- */
/*
 * New definitions for controling the granularity of thread atomicity
 *
 */
#if !defined(MPID_DEFINES_MPID_CS)
#ifdef MPICH_IS_THREADED

/*M MPIU_THREAD_CS_ENTER - Enter a named critical section

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/
#define MPIU_THREAD_CS_ENTER(_name,_context) MPIU_THREAD_CS_ENTER_##_name(_context)

/*M MPIU_THREAD_CS_EXIT - Exit a named critical section

  Input Parameters:
+ _name - cname of the critical section
- _context - A context (typically an object) of the critical section

M*/
#define MPIU_THREAD_CS_EXIT(_name,_context) MPIU_THREAD_CS_EXIT_##_name(_context)

/*M MPIU_THREAD_CS_YIELD - Temporarily release a critical section and yield
    to other threads

  Input Parameters:
+ _name - cname of the critical section
- _context - A context (typically an object) of the critical section

  M*/
#define MPIU_THREAD_CS_YIELD(_name,_context) MPIU_THREAD_CS_YIELD_##_name(_context)

/*M
  ... move the threadsafe init block
  
  These use a private critical section called INITFLAG
  M*/

#ifdef HAVE_RUNTIME_THREADCHECK
#define MPIU_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIU_THREAD_CHECK_END   }  
#else
#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#endif

#define MPIU_ISTHREADED(_s) { MPIU_THREAD_CHECK_BEGIN _s MPIU_THREAD_CHECK_END }

#undef MPIU_THREADSAFE_INIT_DECL
#undef MPIU_THREADSAFE_INIT_STMT
#undef MPIU_THREADSAFE_INIT_BLOCK_BEGIN
#undef MPIU_THREADSAFE_INIT_BLOCK_END
#define MPIU_THREADSAFE_INIT_DECL(_var) static volatile int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) \
     if (_var) { \
	 MPIU_THREAD_CS_ENTER(INITFLAG,);		\
      _stmt; _var=0; \
      MPIU_THREAD_CS_EXIT(INITFLAG,);			\
     }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var) \
    MPIU_THREAD_CS_ENTER(INITFLAG,);		       \
     if (_var) {
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var) \
      } \
	  MPIU_THREAD_CS_EXIT(INITFLAG,)
#else
/* These provide a uniform way to perform a first-use initialization
   in a thread-safe way.  See the web page or mpidtime.c for the generic
   wtick */
#define MPIU_THREADSAFE_INIT_DECL(_var) static int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) if (_var) { _stmt; _var = 0; }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var) 
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var) 

#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#define MPIU_ISTHREADED(_s) 

#endif  /* MPICH_IS_THREADED */
#endif  /* !defined(MPID_DEFINES_MPID_CS) */

#if !defined(MPID_DEFINES_MPID_CS)
#ifdef MPICH_IS_THREADED

/* Helper definitions */
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/*#define MPIU_THREAD_CHECKNEST(_name)*/
/* XXX DJG temporarily disabled for the paper... this breaks our CS_EXIT(ALLFUNC) in sock_wait.i */
/* #if 0 */
#define MPIU_THREAD_CHECKNEST(_name)				\
    MPIU_THREADPRIV_DECL;                                       \
    MPIU_THREADPRIV_GET;                                        \
    if (MPIR_Nest_value() == 0)	
/* #endif */

#define MPIU_THREAD_CHECKDEPTH(_name,_value)
#define MPIU_THREAD_UPDATEDEPTH(_name,_value)				

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL || \
      MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
/* This structure is used to keep track of where the last change was made
   to the thread cs depth */
#ifdef MPID_THREAD_DEBUG
#define MPIU_THREAD_LOC_LEN 127
#define MPIU_THREAD_FNAME_LEN 31
typedef struct MPIU_ThreadDebug {
    int count;
    int line;
    char file[MPIU_THREAD_LOC_LEN+1];
    char fname[MPIU_THREAD_FNAME_LEN+1];
} MPIU_ThreadDebug_t;
#define MPIU_THREAD_CHECKDEPTH(_name,_value) \
    {if (1){ MPIU_ThreadDebug_t *_nest_ptr=0;				\
    MPID_Thread_tls_get( &MPIR_ThreadInfo.nest_storage, &_nest_ptr );\
    if (!_nest_ptr) { _nest_ptr = (MPIU_ThreadDebug_t*)MPIU_Calloc(2,sizeof(MPIU_ThreadDebug_t));\
	    MPID_Thread_tls_set( &MPIR_ThreadInfo.nest_storage,_nest_ptr);}\
    if (_nest_ptr[MPIUNest_##_name].count != _value) {\
      fprintf(stderr, "%s:%d %s = %d, required %d; previously set in %s:%d(%s)\n",\
	      __FILE__, __LINE__,  #_name, 			\
	      _nest_ptr[MPIUNest_##_name].count, _value, 	\
	      _nest_ptr[MPIUNest_##_name].file, \
	      _nest_ptr[MPIUNest_##_name].line,\
	      _nest_ptr[MPIUNest_##_name].fname );			       \
      fflush(stdout); \
    }}}
#define MPIU_THREAD_UPDATEDEPTH(_name,_value)	 {if (1){	\
	MPIU_ThreadDebug_t *_nest_ptr=0;\
	MPID_Thread_tls_get( &MPIR_ThreadInfo.nest_storage, &_nest_ptr );\
	if (!_nest_ptr) { _nest_ptr = (MPIU_ThreadDebug_t*)MPIU_Calloc(2,sizeof(MPIU_ThreadDebug_t));\
	    MPID_Thread_tls_set( &MPIR_ThreadInfo.nest_storage,_nest_ptr);}\
	_nest_ptr[MPIUNest_##_name].count += _value;\
	_nest_ptr[MPIUNest_##_name].line = __LINE__;			\
	MPIU_Strncpy( _nest_ptr[MPIUNest_##_name].file, __FILE__, MPIU_THREAD_LOC_LEN );	\
	MPIU_Strncpy( _nest_ptr[MPIUNest_##_name].fname, FCNAME, MPIU_THREAD_FNAME_LEN );\
}}
#define MPIU_THREAD_CHECKNEST(_name)
/* __thread would be nice here, but it is not portable (not even available
   on Macs) */
/* defined in mpi/init/initthread.c  */
#define MPIUNest_global_mutex 0
#define MPIUNest_handle_mutex 1
#else
#define MPIU_THREAD_CHECKDEPTH(_name,_value)
#define MPIU_THREAD_UPDATEDEPTH(_name,_value)
#define MPIU_THREAD_CHECKNEST(_name)
#endif /* MPID_THREAD_DEBUG */
#else
#define MPIU_THREAD_CHECKNEST(_name)
#endif

#define MPIU_THREAD_CS_ENTER_LOCKNAME(_name) \
{								\
    MPIU_THREAD_CHECKDEPTH(_name,0)                             \
    MPIU_THREAD_CHECKNEST(_name)				\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Enter critical section "#_name);\
	MPID_Thread_mutex_lock(&MPIR_ThreadInfo._name);	\
	MPIU_THREAD_UPDATEDEPTH(_name,1)                        \
    }								\
}
#define MPIU_THREAD_CS_EXIT_LOCKNAME(_name)			\
{								\
    MPIU_THREAD_CHECKDEPTH(_name,1)                             \
    MPIU_THREAD_CHECKNEST(_name)				\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Exit critical section "#_name);\
	MPID_Thread_mutex_unlock(&MPIR_ThreadInfo._name);	\
	MPIU_THREAD_UPDATEDEPTH(_name,-1)                       \
    }								\
}


/* Definitions of the thread support for various levels of thread granularity */
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_ENTER_HANDLE(_context)
#define MPIU_THREAD_CS_EXIT_HANDLE(_context)
#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context)
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context)
#define MPIU_THREAD_CS_ENTER_MSGQUEUE(_context)
#define MPIU_THREAD_CS_EXIT_MSGQUEUE(_context)
#define MPIU_THREAD_CS_ENTER_MPIDCOMM(_context)
#define MPIU_THREAD_CS_EXIT_MPIDCOMM(_context)
#define MPIU_THREAD_CS_ENTER_INITFLAG(_context)
#define MPIU_THREAD_CS_EXIT_INITFLAG(_context)

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL
/* There is a single, global lock, held only when needed */
#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)
/* We use the handle mutex to avoid conflicts with the global mutex - 
   this is a temporary setting until the brief-global option is fully
   implemented */
#define MPIU_THREAD_CS_ENTER_HANDLE(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_HANDLE(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END
   /* The request handles may be allocated, and many other handles might
      be deallocated, within the communication routines.  To avoid 
      problems with lock nesting, for this particular case, we use a
      separate lock (similar to the per-object lock) */
#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_MPIDCOMM(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_MPIDCOMM(_context) \
    MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_MSGQUEUE(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_MSGQUEUE(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_ENTER_INITFLAG(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_INITFLAG(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
/* There are multiple locks, one for each logical class (e.g., each type of 
   object) */

/* FIXME: That's the ugliest hack I've ever written. Use the same lock
 * macro as LOCKNAME */
#define MPIU_THREAD_CS_ENTER_POBJ_LOCKNAME(_name) \
{								\
    MPIU_THREAD_CHECKDEPTH(_name,0)                             \
    MPIU_THREAD_CHECKNEST(_name)				\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Enter critical section "#_name);\
	MPID_Thread_mutex_lock(&_name);	                        \
	MPIU_THREAD_UPDATEDEPTH(_name,1)                        \
    }								\
}
#define MPIU_THREAD_CS_EXIT_POBJ_LOCKNAME(_name)		\
{								\
    MPIU_THREAD_CHECKDEPTH(_name,1)                             \
    MPIU_THREAD_CHECKNEST(_name)				\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Exit critical section "#_name);\
	MPID_Thread_mutex_unlock(&_name);	                \
	MPIU_THREAD_UPDATEDEPTH(_name,-1)                       \
    }								\
}

#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)

#define MPIU_THREAD_CS_ENTER_HANDLE(_context) { \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END \
}
#define MPIU_THREAD_CS_EXIT_HANDLE(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context) { \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END \
}
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(handle_mutex) MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_MPIDCOMM(_context) { \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END \
}
#define MPIU_THREAD_CS_EXIT_MPIDCOMM(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_MSGQUEUE(_context) {\
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END \
}
#define MPIU_THREAD_CS_EXIT_MSGQUEUE(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_INITFLAG(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_INITFLAG(_context) \
   MPIU_THREAD_CHECK_BEGIN MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex) MPIU_THREAD_CHECK_END

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_LOCK_FREE
/* Updates to shared data and access to shared services is handled without 
   locks where ever possible. */
#error lock-free not yet implemented

#else
#error Unrecognized thread granularity
#endif

#else /* ! MPICH_IS_THREAED */
#define MPIU_THREAD_CS_ENTER(_name,_context)
#define MPIU_THREAD_CS_EXIT(_name,_context)
#define MPIU_THREAD_CS_YIELD(_name,_context)
#endif /* MPICH_IS_THREADED */
#endif /* !defined(MPID_DEFINES_MPID_CS)a */

#endif /* !defined(MPIIMPLTHREAD_H_INCLUDED) */

/* This block of text makes it easier to add local use of the thread macros */
# if 0
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL
/* There is a single, global lock, held only when needed */

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
/* There are multiple locks, one for each logical class (e.g., each type of 
   object) */

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_LOCK_FREE
/* Updates to shared data and access to shared services is handled without 
   locks where ever possible. */
#error lock-free not yet implemented

#else
#error Unrecognized thread granularity
#endif
#endif /* 0 */

