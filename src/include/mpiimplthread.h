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

#if !defined(MPID_THREAD_DEBUG) && defined(MPICH_DEBUG_MUTEXNESTING)
#define MPID_THREAD_DEBUG 1
#endif

/* Make the CPP definition that will be used to control whether threaded
   code is supported.  Test ONLY on whether MPICH_IS_THREADED is defined.

   Rather than embed a conditional test in the MPICH2 code, we define a
   single value on which we can test
 */
#if !defined(MPICH_THREAD_LEVEL) || !defined(MPI_THREAD_MULTIPLE)
#error Internal error in macro definitions in include/mpiimplthread.h
#endif
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
#define MPICH_IS_THREADED 1
#endif

/* mpid_thread.h is needed at serialized and above because we might need to
 * store some thread-local data */
#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
#include "mpid_thread.h"
#endif

#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_INVALID
#  error Invalid thread granularity option specified (possibly none)
#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_BRIEF_GLOBAL
#  error The BRIEF_GLOBAL thread granularity option is no longer supported
#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_LOCK_FREE
#  error MPIU_THREAD_GRANULARITY_LOCK_FREE not implemented yet
#endif


typedef struct MPICH_ThreadInfo_t {
    int               thread_provided;  /* Provided level of thread support */
    /* This is a special case for is_thread_main, which must be
       implemented even if MPICH2 itself is single threaded.  */
#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
#  if !defined(MPIU_TLS_SPECIFIER)
    MPID_Thread_tls_t thread_storage;   /* Id for perthread data */
#  endif /* !TLS */
    MPID_Thread_id_t  master_thread;    /* Thread that started MPI */
#endif
#ifdef HAVE_RUNTIME_THREADCHECK
    int isThreaded;                      /* Set to true if user requested
					    THREAD_MULTIPLE */
#endif

    /* Define the mutex values used for each kind of implementation */
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL || \
    MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
    MPID_Thread_mutex_t global_mutex;
    /* We need the handle mutex to avoid problems with lock nesting */
    MPID_Thread_mutex_t handle_mutex;
#endif

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
    MPID_Thread_mutex_t memalloc_mutex; /* for MPIU_{Malloc,Free,Calloc} */
#endif
} MPICH_ThreadInfo_t;
extern MPICH_ThreadInfo_t MPIR_ThreadInfo;

#ifndef MPICH_IS_THREADED

#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#define MPIU_ISTHREADED (0)

#else /* defined(MPICH_IS_THREADED) */

/* We want to avoid the overhead of the thread call if we're in the
   runtime state and threads are not in use.  In that case, MPIR_Thread
   is still a pointer but it was already allocated in InitThread */
#ifdef HAVE_RUNTIME_THREADCHECK

#define MPIU_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIU_THREAD_CHECK_END   }
/* This macro used to take an argument of one or more statements that
 * would be wrapped in a conditional.  However it was not used anywhere
 * in part because it looked unnatural.  This version takes no arguments
 * and evaluates to a boolean expression that is true if MPICH2 is
 * threaded (including any runtime check if needed) and false otherwise.
 * MPIU_THREAD_CHECK_{BEGIN,END} should probably still be used if you
 * don't have any other conditions that you need to check in order to
 * completely eliminate unnecessary if's. */
#define MPIU_ISTHREADED (MPIR_ThreadInfo.isThreaded)

#else /* !defined(HAVE_RUNTIME_THREADCHECK) */

#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#define MPIU_ISTHREADED (1)

#endif /* HAVE_RUNTIME_THREADCHECK */

#endif /* MPICH_IS_THREADED */

/* ------------------------------------------------------------------------- */
/* thread-local storage macros */
/* moved here from mpiimpl.h because they logically belong here */

/* Time stamps */
/* Get the timer definitions.  The source file for this include is
   src/mpi/timer/mpichtimer.h.in */
#include "mpichtimer.h"

typedef struct MPID_Stateinfo_t {
    MPID_Time_t stamp;
    int count;
} MPID_Stateinfo_t;
#define MPICH_MAX_STATES 512
/* Timer state routines (src/util/instrm/states.c) */
void MPID_TimerStateBegin( int, MPID_Time_t * );
void MPID_TimerStateEnd( int, MPID_Time_t * );

#ifdef MPICH_DEBUG_NESTING
#define MPICH_MAX_NESTFILENAME 256
typedef struct MPICH_Nestinfo {
    char file[MPICH_MAX_NESTFILENAME];
    int  line;
} MPICH_Nestinfo_t;
#define MPICH_MAX_NESTINFO 16
#endif /* MPICH_DEBUG_NESTING */

/* arbitrary, just needed to avoid cleaning up heap allocated memory at thread
 * destruction time */
#define MPIU_STRERROR_BUF_SIZE (1024)

/* this structure contains all thread-local variables, will be zeroed at
 * allocation time */
typedef struct MPICH_PerThread_t {
    int              nest_count;   /* For layered MPI implementation */
    int              op_errno;     /* For errors in predefined MPI_Ops */

    /* error string storage for MPIU_Strerror */
    char strerrbuf[MPIU_STRERROR_BUF_SIZE];

#ifdef MPICH_DEBUG_NESTING
    MPICH_Nestinfo_t nestinfo[MPICH_MAX_NESTINFO];
    struct MPIU_ThreadDebug *nest_debug;
#endif
    /* FIXME: Is this used anywhere? */
#ifdef HAVE_TIMING
    MPID_Stateinfo_t timestamps[MPICH_MAX_STATES];  /* per thread state info */
#endif
#if defined(MPID_DEV_PERTHREAD_DECL)
    MPID_DEV_PERTHREAD_DECL
#endif
} MPICH_PerThread_t;

#if !defined(MPICH_IS_THREADED)
/* If single threaded, make this point at a pre-allocated segment.
   This structure is allocated in src/mpi/init/initthread.c */
extern MPICH_PerThread_t MPIR_Thread;

/* The following three macros define a way to portably access thread-private
   storage in MPICH2, and avoid extra overhead when MPICH2 is single
   threaded
   INITKEY - Create the key.  Must happen *before* the other threads
             are created
   INIT    - Create the thread-private storage.  Must happen once per thread
   DECL    - Declare local variables
   GET     - Access the thread-private storage
   FIELD   - Access the thread-private field (by name)
   FINALIZE - to be invoked when all threads no longer need access to the thread
              local storage, such as at MPI_Finalize time

   The "DECL" is the extern so that there is always a statement for
   the declaration.
*/
#define MPIU_THREADPRIV_INITKEY
#define MPIU_THREADPRIV_INIT
/* Empty declarations are not allowed in C. However multiple decls are allowed */
/* FIXME it might be better to just use a dummy variable here like we do when
 * real TLS is available to reduce the amount of compiler warnings. */
#define MPIU_THREADPRIV_DECL extern MPICH_PerThread_t MPIR_Thread
#define MPIU_THREADPRIV_GET
#define MPIU_THREADPRIV_FIELD(a_) (MPIR_Thread.a_)

#else /* defined(MPICH_IS_THREADED) */

/* We need to provide a function that will cleanup the storage attached
   to the key.  */
void MPIR_CleanupThreadStorage(void *a);

#if !defined(MPIU_TLS_SPECIFIER)
#  if defined(HAVE_RUNTIME_THREADCHECK)
/* In the case where the thread level is set in MPI_Init_thread, we
   need a blended version of the non-threaded and the thread-multiple
   definitions.

   The approach is to have TWO MPICH_PerThread_t pointers.  One is local
   (The MPIU_THREADPRIV_DECL is used in the routines local definitions),
   as in the threaded version of these macros.  This is set by using a routine
   to get thread-private storage.  The second is a preallocated, extern
   MPICH_PerThread_t struct, as in the single threaded case.  Based on
   MPIR_Process.isThreaded, one or the other is used.

 */
/* For the single threaded case, we use a preallocated structure
   This structure is allocated in src/mpi/init/initthread.c */
extern MPICH_PerThread_t MPIR_ThreadSingle;

#define MPIU_THREADPRIV_INITKEY                                                                     \
    do {                                                                                            \
        if (MPIU_ISTHREADED) {                                                                      \
            MPID_Thread_tls_create(MPIR_CleanupThreadStorage,&MPIR_ThreadInfo.thread_storage,NULL); \
        }                                                                                           \
    } while (0)
#define MPIU_THREADPRIV_INIT                                                                            \
    do {                                                                                                \
        if (MPIU_ISTHREADED) {                                                                          \
            MPIR_Thread = (MPICH_PerThread_t *) MPIU_Calloc(1, sizeof(MPICH_PerThread_t));              \
            MPIU_Assert(MPIR_Thread);                                                                   \
            MPID_Thread_tls_set(&MPIR_ThreadInfo.thread_storage, (void *)MPIR_Thread);                  \
        }                                                                                               \
    } while (0)
/* MPIU_THREADPRIV_INIT must *always* be called before MPIU_THREADPRIV_GET */
#define MPIU_THREADPRIV_GET                                                         \
    do {                                                                            \
        if (!MPIR_Thread) {                                                         \
            if (MPIU_ISTHREADED) {                                                  \
                MPID_Thread_tls_get(&MPIR_ThreadInfo.thread_storage, &MPIR_Thread); \
            }                                                                       \
            else {                                                                  \
                MPIR_Thread = &MPIR_ThreadSingle;                                   \
            }                                                                       \
            MPIU_Assert(MPIR_Thread);                                               \
        }                                                                           \
    } while (0)

#  else /* unconditional thread multiple */

/* We initialize the MPIR_Thread pointer to null so that we need call the
 * routine to get the thread-private storage only once in an invocation of a
 * routine.  */
#define MPIU_THREADPRIV_INITKEY  \
    MPID_Thread_tls_create(MPIR_CleanupThreadStorage,&MPIR_ThreadInfo.thread_storage,NULL)
#define MPIU_THREADPRIV_INIT                                                                        \
    do {                                                                                            \
        MPIR_Thread = (MPICH_PerThread_t *) MPIU_Calloc(1, sizeof(MPICH_PerThread_t));              \
        MPIU_Assert(MPIR_Thread);                                                                   \
        MPID_Thread_tls_set(&MPIR_ThreadInfo.thread_storage, (void *)MPIR_Thread);                  \
    } while (0)
#define MPIU_THREADPRIV_GET                                                     \
    do {                                                                        \
        if (!MPIR_Thread) {                                                     \
            MPID_Thread_tls_get(&MPIR_ThreadInfo.thread_storage, &MPIR_Thread); \
            MPIU_Assert(MPIR_Thread);                                           \
        }                                                                       \
    } while (0)

#  endif /* runtime vs. unconditional */

/* common definitions when using MPID_Thread-based TLS */
#define MPIU_THREADPRIV_DECL MPICH_PerThread_t *MPIR_Thread=NULL
#define MPIU_THREADPRIV_FIELD(a_) (MPIR_Thread->a_)
#define MPIU_THREADPRIV_FINALIZE                                           \
    do {                                                                   \
        MPIU_THREADPRIV_DECL;                                              \
        if (MPIU_ISTHREADED) {                                             \
            MPIU_THREADPRIV_GET;                                           \
            MPIU_Free(MPIR_Thread);                                        \
            MPID_Thread_tls_set(&MPIR_ThreadInfo.thread_storage,NULL);     \
            MPID_Thread_tls_destroy(&MPIR_ThreadInfo.thread_storage,NULL); \
        }                                                                  \
    } while (0)

#else /* defined(MPIU_TLS_SPECIFIER) */

/* We have proper thread-local storage (TLS) support from the compiler, which
 * should yield the best performance and simplest code, so we'll use that. */
extern MPIU_TLS_SPECIFIER MPICH_PerThread_t MPIR_Thread;

#define MPIU_THREADPRIV_INITKEY
#define MPIU_THREADPRIV_INIT
/* Empty declarations are not allowed in C, so we just define a dummy
 * variable and tell the compiler it is unused. */
#define MPIU_THREADPRIV_DECL int MPIU_THREADPRIV_LOCAL_DUMMY_VAR ATTRIBUTE((unused))
#define MPIU_THREADPRIV_GET
#define MPIU_THREADPRIV_FIELD(a_) (MPIR_Thread.a_)
#define MPIU_THREADPRIV_FINALIZE do{}while(0)

#endif /* defined(MPIU_TLS_SPECIFIER) */
#endif /* defined(MPICH_IS_THREADED) */

/* ------------------------------------------------------------------------- */
/*
 * New definitions for controling the granularity of thread atomicity
 *
 * If MPICH supports threads and the device doesn't wish to override our
 * definitions, we supply default definitions.
 *
 * If the device does choose to override our definitions, it needs to do the
 * following at a minimum:
 * 1) #define MPID_DEVICE_DEFINES_THREAD_CS in mpidpre.h
 * 2) provide #defines for MPIU_THREAD_CS_{INIT,FINALIZE}
 * 3) provide #defines for MPIU_THREAD_CS_{ENTER,EXIT,YIELD}(name_,context_)
 * 4) provide #defines for MPIU_THREADSAFE_INIT_{DECL,STMT,BLOCK_BEGIN,BLOCK_END,CLEAR}
 */
#if defined(MPICH_IS_THREADED) && !defined(MPID_DEVICE_DEFINES_THREAD_CS)

/* MPIU_THREAD_CS_INIT will be invoked early in the top level
 * MPI_Init/MPI_Init_thread process.  It is responsible for performing any
 * initialization for the MPIU_THREAD_CS_ macros, as well as invoking
 * MPIU_THREADPRIV_INITKEY and MPIU_THREADPRIV_INIT.
 *
 * MPIR_ThreadInfo.isThreaded will be set prior to this macros invocation, based
 * on the "provided" value to MPI_Init_thread
 */
#define MPIU_THREAD_CS_INIT     MPIR_Thread_CS_Init()     /* in src/mpi/init/initthread.c */
#define MPIU_THREAD_CS_FINALIZE MPIR_Thread_CS_Finalize() /* in src/mpi/init/initthread.c */

/* some important critical section names:
 *   ALLFUNC - entered/exited at beginning/end of (nearly) every MPI_ function
 *   INIT - entered before MPID_Init and exited near the end of MPI_Init(_thread)
 */

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

/* FIXME this is a broken version of the Double-Checked Locking Pattern (DCLP).
 * In practice it may work OK, but it certainly isn't guaranteed depending on
 * what the processor, compiler, and pthread implementation choose to do.
 *
 * These might be fixable in practice via OPA memory barriers. */
#define MPIU_THREADSAFE_INIT_DECL(_var) static volatile int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt)   \
    do {                                        \
        if (_var) {                             \
            MPIU_THREAD_CS_ENTER(INITFLAG,);    \
            _stmt;                              \
            _var=0;                             \
            MPIU_THREAD_CS_EXIT(INITFLAG,);     \
        }                                       \
    while (0)
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var)  \
    MPIU_THREAD_CS_ENTER(INITFLAG,);            \
    if (_var) {
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var)    \
    }                                           \
    MPIU_THREAD_CS_EXIT(INITFLAG,)

#else /* !defined(MPICH_IS_THREADED) */

/* These provide a uniform way to perform a first-use initialization
   in a thread-safe way.  See the web page or mpidtime.c for the generic
   wtick */
#define MPIU_THREADSAFE_INIT_DECL(_var) static int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) \
    do {                                      \
        if (_var) {                           \
            _stmt;                            \
            _var = 0;                         \
        }                                     \
    } while (0)
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var)
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var)

#endif  /* MPICH_IS_THREADED */

/* Helper definitions for the default macro definitions */
#if defined(MPICH_IS_THREADED) && !defined(MPID_DEVICE_DEFINES_THREAD_CS)
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL

/* FIXME what is this really supposed to do?  Is it supposed to check against
 * the "MPI nesting" or the critical section nesting? */
/* It seems to just be a hook gating actual mutex lock/unlock for usage in
 * _GLOBAL mode */
/* should be followed by a {} block that will be conditionally executed */
#define MPIU_THREAD_CHECKNEST(kind_, lockname_)                                         \
    MPIU_THREADPRIV_GET;                                                                \
    MPIU_DBG_MSG_D(THREAD,VERBOSE,"CHECKNEST, MPIR_Nest_value()=%d",MPIR_Nest_value()); \
    if (MPIR_Nest_value() == 0)

#define MPIU_THREAD_CHECKDEPTH(kind_, lockname_, value_)
#define MPIU_THREAD_UPDATEDEPTH(kind_, lockname_, value_)

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT

/* this nesting is different than the MPIR_Nest_* macros, this is critical
 * section nesting, the other is MPI/NMPI nesting */

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

/* helper macro to allocate and initialize storage for the CS nest checks, not
 * part of the public API */
/* NOTE: assumes that MPIU_THREADPRIV_DECL has been done already */
#define MPIU_THREAD_INIT_NEST_CHECK_                                            \
    do {                                                                        \
        MPIU_THREADPRIV_GET;                                                    \
        if (!MPIU_THREADPRIV_FIELD(nest_debug)) {                               \
            MPIU_THREADPRIV_FIELD(nest_debug) =                                 \
                MPIU_Calloc(MPIU_Nest_NUM_MUTEXES, sizeof(MPIU_ThreadDebug_t)); \
        }                                                                       \
    } while (0)
#define MPIU_THREAD_CHECKDEPTH(kind_, lockname_, value_)                                   \
    do {                                                                                   \
        MPIU_ThreadDebug_t *nest_ptr_ = NULL;                                              \
        MPIU_THREADPRIV_GET;                                                               \
        MPIU_THREAD_INIT_NEST_CHECK_;                                                      \
        nest_ptr_ = MPIU_THREADPRIV_FIELD(nest_debug);                                     \
        if (nest_ptr_[kind_].count != value_) {                                            \
            fprintf(stderr, "%s:%d %s = %d, required %d; previously set in %s:%d(%s)\n",   \
                    __FILE__, __LINE__,  lockname_,                                        \
                    nest_ptr_[kind_].count, value_,                                        \
                    nest_ptr_[kind_].file,                                                 \
                    nest_ptr_[kind_].line,                                                 \
                    nest_ptr_[kind_].fname );                                              \
            fflush(stderr);                                                                \
        }                                                                                  \
    } while (0)
#define MPIU_THREAD_UPDATEDEPTH(kind_, lockname_, value_)                                  \
    do {                                                                                   \
        MPIU_ThreadDebug_t *nest_ptr_ = NULL;                                              \
        MPIU_THREADPRIV_GET;                                                               \
        MPIU_THREAD_INIT_NEST_CHECK_;                                                      \
        nest_ptr_ = MPIU_THREADPRIV_FIELD(nest_debug);                                     \
        if (nest_ptr_[kind_].count + (value_) < 0) {                                       \
            fprintf(stderr, "%s:%d %s = %d (<0); previously set in %s:%d(%s)\n",           \
                    __FILE__, __LINE__,  lockname_,                                        \
                    nest_ptr_[kind_].count,                                                \
                    nest_ptr_[kind_].file,                                                 \
                    nest_ptr_[kind_].line,                                                 \
                    nest_ptr_[kind_].fname );                                              \
            fflush(stderr);                                                                \
        }                                                                                  \
        nest_ptr_[kind_].count += value_;                                                  \
        nest_ptr_[kind_].line = __LINE__;                                                  \
        MPIU_Strncpy( nest_ptr_[kind_].file, __FILE__, MPIU_THREAD_LOC_LEN );              \
        MPIU_Strncpy( nest_ptr_[kind_].fname, FCNAME, MPIU_THREAD_FNAME_LEN );             \
    } while (0)
#define MPIU_THREAD_CHECKNEST(kind_, lockname_)

#else
#define MPIU_THREAD_CHECKDEPTH(kind_, lockname_, value_)
#define MPIU_THREAD_UPDATEDEPTH(kind_, lockname_, value_)
#define MPIU_THREAD_CHECKNEST(kind_, lockname_)
#endif /* MPID_THREAD_DEBUG */
#else
#define MPIU_THREAD_CHECKNEST(kind_, lockname_)
#endif /* test on THREAD_GRANULARITY */

#define MPIU_THREAD_CS_ENTER_LOCKNAME(name_)                                                   \
    do {                                                                                       \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to ENTER lockname=%s", #name_);              \
        MPIU_Thread_CS_enter_lockname_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_); \
    } while (0)
#define MPIU_THREAD_CS_EXIT_LOCKNAME(name_)                                                    \
    do {                                                                                       \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to EXIT lockname=%s", #name_);               \
        MPIU_Thread_CS_exit_lockname_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_);  \
    } while (0)
#define MPIU_THREAD_CS_YIELD_LOCKNAME(name_)                                                   \
    do {                                                                                       \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to YIELD lockname=%s", #name_);              \
        MPIU_Thread_CS_yield_lockname_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_); \
    } while (0)

enum MPIU_Nest_mutexes {
    MPIU_Nest_global_mutex = 0,
    MPIU_Nest_handle_mutex,
    MPIU_Nest_NUM_MUTEXES
};

/* Some locks (such as the memory allocation locks) are needed to
 * bootstrap the lock checking code.  These macros provide a similar
 * interface to the MPIU_THREAD_CS_{ENTER,EXIT}_LOCKNAME macros but
 * without the bootstrapping issue. */
#define MPIU_THREAD_CS_ENTER_LOCKNAME_NOCHECK(_name)                  \
    do {                                                              \
        MPIU_DBG_MSG(THREAD,TYPICAL,"Enter critical section "#_name " (checking disabled)"); \
        MPID_Thread_mutex_lock(&MPIR_ThreadInfo._name);               \
    } while (0)
#define MPIU_THREAD_CS_EXIT_LOCKNAME_NOCHECK(_name)                  \
    do {                                                             \
        MPIU_DBG_MSG(THREAD,TYPICAL,"Exit critical section "#_name " (checking disabled)"); \
        MPID_Thread_mutex_unlock(&MPIR_ThreadInfo._name);            \
    } while (0)

#if defined(USE_MEMORY_TRACING)
    /* These are defined for all levels of thread granularity due to the
     * bootstrapping issue mentioned above */
#   define MPIU_THREAD_CS_ENTER_MEMALLOC(_context)                  \
        do {                                                        \
            MPIU_THREAD_CHECK_BEGIN                                 \
            MPIU_THREAD_CS_ENTER_LOCKNAME_NOCHECK(memalloc_mutex);  \
            MPIU_THREAD_CHECK_END                                   \
        } while(0)
#   define MPIU_THREAD_CS_EXIT_MEMALLOC(_context)                   \
        do {                                                        \
            MPIU_THREAD_CHECK_BEGIN                                 \
            MPIU_THREAD_CS_EXIT_LOCKNAME_NOCHECK(memalloc_mutex);   \
            MPIU_THREAD_CHECK_END                                   \
        } while(0)
#else
    /* we don't need these macros when using the system malloc */
#   define MPIU_THREAD_CS_ENTER_MEMALLOC(_context) do {} while (0)
#   define MPIU_THREAD_CS_EXIT_MEMALLOC(_context) do {} while (0)
#endif

/* helper macros to insert thread checks around LOCKNAME actions */
#define MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(name_)    \
    MPIU_THREAD_CHECK_BEGIN                             \
    MPIU_THREAD_CS_ENTER_LOCKNAME(name_);               \
    MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(name_)     \
    MPIU_THREAD_CHECK_BEGIN                             \
    MPIU_THREAD_CS_EXIT_LOCKNAME(name_);                \
    MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(name_)    \
    MPIU_THREAD_CHECK_BEGIN                             \
    MPIU_THREAD_CS_YIELD_LOCKNAME(name_);               \
    MPIU_THREAD_CHECK_END


/* Definitions of the thread support for various levels of thread granularity */
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_YIELD_ALLFUNC(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(global_mutex)

/* not _CHECKED, invoked before MPIU_ISTHREADED will work */
#define MPIU_THREAD_CS_ENTER_INIT(_context)    MPIU_THREAD_CS_ENTER_LOCKNAME(global_mutex)
#define MPIU_THREAD_CS_EXIT_INIT(_context)     MPIU_THREAD_CS_EXIT_LOCKNAME(global_mutex)

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
#define MPIU_THREAD_CS_ENTER_PMI(_context)
#define MPIU_THREAD_CS_EXIT_PMI(_context)

#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context)
/* XXX DJG at the global level should CONTEXTID yield even though it
 * doesn't do anything at ENTER/EXIT? */
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(global_mutex)


#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
/* There are multiple locks, one for each (major) object */

/* FIXME needs testing and work */

#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)
#define MPIU_THREAD_CS_YIELD_ALLFUNC(_context)

#define MPIU_THREAD_CS_ENTER_INIT(_context)
#define MPIU_THREAD_CS_EXIT_INIT(_context)

#define MPIU_THREAD_CS_ENTER_HANDLE(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(handle_mutex)
#define MPIU_THREAD_CS_EXIT_HANDLE(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(handle_mutex)

#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(handle_mutex)
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(handle_mutex)

#define MPIU_THREAD_CS_ENTER_MPIDCOMM(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_MPIDCOMM(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)

#define MPIU_THREAD_CS_ENTER_MSGQUEUE(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_MSGQUEUE(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)

#define MPIU_THREAD_CS_ENTER_INITFLAG(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_INITFLAG(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)

#define MPIU_THREAD_CS_ENTER_PMI(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_PMI(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)

/* TODO this can probably use its own mutex... */
#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(global_mutex)

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

#endif /* !defined(MPIIMPLTHREAD_H_INCLUDED) */

