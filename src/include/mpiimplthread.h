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


/* I don't have a better place for this at the moment, so it lives here for now. */
enum MPIU_Thread_cs_name {
    MPIU_THREAD_CS_NAME_ALLFUNC = 0,
    MPIU_THREAD_CS_NAME_INIT,
    MPIU_THREAD_CS_NAME_HANDLE,
    MPIU_THREAD_CS_NAME_HANDLEALLOC,
    MPIU_THREAD_CS_NAME_MPIDCOMM,
    MPIU_THREAD_CS_NAME_COMPLETION,
    MPIU_THREAD_CS_NAME_PMI,
    MPIU_THREAD_CS_NAME_CONTEXTID,
    MPIU_THREAD_CS_NAME_MPI_OBJ,
    MPIU_THREAD_CS_NAME_MSGQUEUE,
    /* FIXME device-specific, should this live here? */
    /*
    MPIU_THREAD_CS_NAME_CH3COMM,
     */
    MPIU_THREAD_CS_NUM_NAMES
};

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

#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
    MPID_Thread_mutex_t msgq_mutex;
    MPID_Thread_mutex_t completion_mutex;
    MPID_Thread_mutex_t ctx_mutex;
    MPID_Thread_mutex_t pmi_mutex;
#endif

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
    MPID_Thread_mutex_t memalloc_mutex; /* for MPIU_{Malloc,Free,Calloc} */
    MPID_Thread_id_t cs_holder[MPIU_THREAD_CS_NUM_NAMES];
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

/* arbitrary, just needed to avoid cleaning up heap allocated memory at thread
 * destruction time */
#define MPIU_STRERROR_BUF_SIZE (1024)

/* FIXME should really be MPIU_Nest_NUM_MUTEXES, but it's defined later */
#define MPICH_MAX_LOCKS (6)

/* This structure contains all thread-local variables and will be zeroed at
 * allocation time.
 *
 * Note that any pointers to dynamically allocated memory stored in this
 * structure must be externally cleaned up.
 * */
typedef struct MPICH_PerThread_t {
    int              op_errno;     /* For errors in predefined MPI_Ops */

    /* error string storage for MPIU_Strerror */
    char strerrbuf[MPIU_STRERROR_BUF_SIZE];

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
    int lock_depth[MPICH_MAX_LOCKS];
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
                if (!MPIR_Thread) {                                                 \
                    MPIU_THREADPRIV_INIT; /* subtle, sets MPIR_Thread */            \
                }                                                                   \
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
            if (!MPIR_Thread) {                                                 \
                MPIU_THREADPRIV_INIT; /* subtle, sets MPIR_Thread */            \
            }                                                                   \
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
 * See the analysis of the MPI routines for thread usage properties.  Those
 * routines considered "Access Only" do not require ALLFUNC.  That analysis
 * was very general; in MPICH2, some routines may have internal shared
 * state that isn't required by the MPI specification.  Perhaps the
 * best example of this is the MPI_ERROR_STRING routine, where the 
 * instance-specific error messages make use of shared state, and hence
 * must be accessed in a thread-safe fashion (e.g., require an ALLFUNC
 * critical section).  With such routines removed, the set of routines
 * that (probably) do not require ALLFUNC include:
 *
 * MPI_CART_COORDS, MPI_CART_GET, MPI_CART_MAP, MPI_CART_RANK, MPI_CART_SHIFT, 
 * MPI_CART_SUB, MPI_CARTDIM_GET, MPI_COMM_GET_NAME, 
 * MPI_COMM_RANK, MPI_COMM_REMOTE_SIZE, 
 * MPI_COMM_SET_NAME, MPI_COMM_SIZE, MPI_COMM_TEST_INTER, MPI_ERROR_CLASS,
 * MPI_FILE_GET_AMODE, MPI_FILE_GET_ATOMICITY, MPI_FILE_GET_BYTE_OFFSET, 
 * MPI_FILE_GET_POSITION, MPI_FILE_GET_POSITION_SHARED, MPI_FILE_GET_SIZE
 * MPI_FILE_GET_TYPE_EXTENT, MPI_FILE_SET_SIZE, 
 * MPI_FINALIZED, MPI_GET_COUNT, MPI_GET_ELEMENTS, MPI_GRAPH_GET, 
 * MPI_GRAPH_MAP, MPI_GRAPH_NEIGHBORS, MPI_GRAPH_NEIGHBORS_COUNT, 
 * MPI_GRAPHDIMS_GET, MPI_GROUP_COMPARE, MPI_GROUP_RANK, 
 * MPI_GROUP_SIZE, MPI_GROUP_TRANSLATE_RANKS, MPI_INITIALIZED, 
 * MPI_PACK, MPI_PACK_EXTERNAL, MPI_PACK_SIZE, MPI_TEST_CANCELLED, 
 * MPI_TOPO_TEST, MPI_TYPE_EXTENT, MPI_TYPE_GET_ENVELOPE, 
 * MPI_TYPE_GET_EXTENT, MPI_TYPE_GET_NAME, MPI_TYPE_GET_TRUE_EXTENT, 
 * MPI_TYPE_LB, MPI_TYPE_SET_NAME, MPI_TYPE_SIZE, MPI_TYPE_UB, MPI_UNPACK, 
 * MPI_UNPACK_EXTERNAL, MPI_WIN_GET_NAME, MPI_WIN_SET_NAME
 *
 * Some of the routines that could be read-only, but internally may
 * require access or updates to shared data include
 * MPI_COMM_COMPARE (creation of group sets)
 * MPI_COMM_SET_ERRHANDLER (reference count on errhandler)
 * MPI_COMM_CALL_ERRHANDLER (actually ok, but risk high, usage low)
 * MPI_FILE_CALL_ERRHANDLER (ditto)
 * MPI_WIN_CALL_ERRHANDLER (ditto)
 * MPI_ERROR_STRING (access to instance-specific string, which could
 *                   be overwritten by another thread)
 * MPI_FILE_SET_VIEW (setting view a big deal)
 * MPI_TYPE_COMMIT (could update description of type internally,
 *                  including creating a new representation.  Should
 *                  be ok, but, like call_errhandler, low usage)
 * 
 * Note that other issues may force a routine to include the ALLFUNC
 * critical section, such as debugging information that requires shared
 * state.  Such situations should be avoided where possible.
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

/*M MPIU_THREAD_CS_TRY - Trylock version of ENTER to a named critical section

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/
#define MPIU_THREAD_CS_TRY(_name,_locked,_context) MPIU_THREAD_CS_TRY_##_name(_context,_locked)


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

#elif !defined(MPID_DEVICE_DEFINES_THREAD_CS) /* && !defined(MPICH_IS_THREADED) */

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

#endif

/* Helper definitions for the default macro definitions */
#if defined(MPICH_IS_THREADED) && !defined(MPID_DEVICE_DEFINES_THREAD_CS)
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL

#define MPIU_THREAD_CHECKDEPTH(kind_, lockname_, value_)
#define MPIU_THREAD_UPDATEDEPTH(kind_, lockname_, value_)

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT

/* this nesting is different than the MPIR_Nest_* macros, this is critical
 * section nesting, the other is MPI/NMPI nesting */

/* This structure is used to keep track of where the last change was made
   to the thread cs depth */
/* temporarily disabled until we figure out if these should be restored (they
 * are currently broken) */
#if 0 && MPID_THREAD_DEBUG

#define MPIU_THREAD_CHECKDEPTH(kind_, lockname_, value_)                                   \
    do {                                                                                   \
        MPIU_ThreadDebug_t *nest_ptr_ = NULL;                                              \
        MPIU_THREADPRIV_GET;                                                               \
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
#else
#define MPIU_THREAD_CHECKDEPTH(kind_, lockname_, value_)
#define MPIU_THREAD_UPDATEDEPTH(kind_, lockname_, value_)
#endif /* MPID_THREAD_DEBUG */
#endif /* test on THREAD_GRANULARITY */

#define MPIU_THREAD_CS_ENTER_LOCKNAME_RECURSIVE(name_)                                          \
    do {                                                                                        \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to recursively ENTER lockname=%s", #name_);   \
        MPIU_Thread_CS_enter_lockname_recursive_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_); \
    } while (0)
#define MPIU_THREAD_CS_EXIT_LOCKNAME_RECURSIVE(name_)                                           \
    do {                                                                                        \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to recursively EXIT lockname=%s", #name_);    \
        MPIU_Thread_CS_exit_lockname_recursive_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_);  \
    } while (0)
#define MPIU_THREAD_CS_YIELD_LOCKNAME_RECURSIVE(name_)                                          \
    do {                                                                                        \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to recursively YIELD lockname=%s", #name_);   \
        MPIU_Thread_CS_yield_lockname_recursive_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_); \
    } while (0)
#define MPIU_THREAD_CS_TRY_LOCKNAME_RECURSIVE(name_,locked_)                                   \
    do {                                                                                      \
        MPIU_Thread_CS_try_lockname_recursive_impl_(MPIU_Nest_##name_, #name_, &MPIR_ThreadInfo.name_, locked_); \
    } while (0)


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
    MPIU_Nest_msgq_mutex,
    MPIU_Nest_completion_mutex,
    MPIU_Nest_ctx_mutex,
    MPIU_Nest_pmi_mutex,
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
/* FIXME this shouldn't need to be recursive, but using a recursive mutex here
 * greatly simplifies thread safety in ROMIO right now.  In the long term we
 * should make ROMIO internally thread safe and then this can go back to being a
 * non-recursive mutex. [goodell@ 2010-10-15] */
#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context)                 \
    do {                                                       \
        MPIU_THREAD_CHECK_BEGIN                                \
        MPIU_THREAD_CS_ENTER_LOCKNAME_RECURSIVE(global_mutex); \
        MPIU_THREAD_CHECK_END                                  \
    } while (0)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)                 \
    do {                                                      \
        MPIU_THREAD_CHECK_BEGIN                               \
        MPIU_THREAD_CS_EXIT_LOCKNAME_RECURSIVE(global_mutex); \
        MPIU_THREAD_CHECK_END                                 \
    } while (0)
#define MPIU_THREAD_CS_YIELD_ALLFUNC(_context)                 \
    do {                                                       \
        MPIU_THREAD_CHECK_BEGIN                                \
        MPIU_THREAD_CS_YIELD_LOCKNAME_RECURSIVE(global_mutex); \
        MPIU_THREAD_CHECK_END                                  \
    } while (0)
#define MPIU_THREAD_CS_TRY_ALLFUNC(_context,_locked)           \
    do {                                                       \
        MPIU_THREAD_CHECK_BEGIN                                \
        MPIU_THREAD_CS_TRY_LOCKNAME_RECURSIVE(global_mutex,_locked);   \
        MPIU_THREAD_CHECK_END                                  \
    } while (0)

/* not _CHECKED, invoked before MPIU_ISTHREADED will work */
#define MPIU_THREAD_CS_ENTER_INIT(_context)    MPIU_THREAD_CS_ENTER_LOCKNAME_RECURSIVE(global_mutex)
#define MPIU_THREAD_CS_EXIT_INIT(_context)     MPIU_THREAD_CS_EXIT_LOCKNAME_RECURSIVE(global_mutex)

#define MPIU_THREAD_CS_ENTER_HANDLE(_context)
#define MPIU_THREAD_CS_EXIT_HANDLE(_context)
#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context)
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context)
#define MPIU_THREAD_CS_ENTER_MSGQUEUE(_context)
#define MPIU_THREAD_CS_EXIT_MSGQUEUE(_context)

#define MPIU_THREAD_CS_ENTER_MPIDCOMM(_context)
#define MPIU_THREAD_CS_EXIT_MPIDCOMM(_context)
#define MPIU_THREAD_CS_YIELD_MPIDCOMM(_context)

#define MPIU_THREAD_CS_ENTER_COMPLETION(_context)
#define MPIU_THREAD_CS_EXIT_COMPLETION(_context)
#define MPIU_THREAD_CS_YIELD_COMPLETION(_context)

#define MPIU_THREAD_CS_ENTER_INITFLAG(_context)
#define MPIU_THREAD_CS_EXIT_INITFLAG(_context)
#define MPIU_THREAD_CS_ENTER_PMI(_context)
#define MPIU_THREAD_CS_EXIT_PMI(_context)

#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context)
/* XXX DJG at the global level should CONTEXTID yield even though it
 * doesn't do anything at ENTER/EXIT? */
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context)               \
    do {                                                       \
        MPIU_THREAD_CHECK_BEGIN                                \
        MPIU_THREAD_CS_YIELD_LOCKNAME_RECURSIVE(global_mutex); \
        MPIU_THREAD_CHECK_END                                  \
    } while (0)

#define MPIU_THREAD_CS_ENTER_MPI_OBJ(context_) do {} while(0)
#define MPIU_THREAD_CS_EXIT_MPI_OBJ(context_)  do {} while(0)


#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
/* There are multiple locks, one for each (major) object */

#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)
#define MPIU_THREAD_CS_YIELD_ALLFUNC(_context)

#define MPIU_THREAD_CS_ENTER_INIT(_context)
#define MPIU_THREAD_CS_EXIT_INIT(_context)

#define MPIU_THREAD_CS_ENTER_HANDLE(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(handle_mutex)
#define MPIU_THREAD_CS_EXIT_HANDLE(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(handle_mutex)

#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(handle_mutex)
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(handle_mutex)

/* MPIDCOMM protects the progress engine and related code */
#define MPIU_THREAD_CS_ENTER_MPIDCOMM(_context)                \
    MPIU_THREAD_CHECK_BEGIN                                    \
    do {                                                       \
        MPIU_THREAD_CS_ENTER_LOCKNAME_RECURSIVE(global_mutex); \
    } while (0);                                               \
    MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_EXIT_MPIDCOMM(_context)                \
    MPIU_THREAD_CHECK_BEGIN                                   \
    do {                                                      \
        MPIU_THREAD_CS_EXIT_LOCKNAME_RECURSIVE(global_mutex); \
    } while (0);                                              \
    MPIU_THREAD_CHECK_END
#define MPIU_THREAD_CS_YIELD_MPIDCOMM(_context)                \
    MPIU_THREAD_CHECK_BEGIN                                    \
    do {                                                       \
        MPIU_THREAD_CS_YIELD_LOCKNAME_RECURSIVE(global_mutex); \
    } while (0);                                               \
    MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_COMPLETION(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(completion_mutex)
#define MPIU_THREAD_CS_EXIT_COMPLETION(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(completion_mutex)


/* MT FIXME the following description is almost right, but it needs minor
 * updates and revision to account for the COMPLETION CS and other issues in the
 * request */
/* The fine-grained locking discipline for requests is unfortunately complicated:
 *
 * (1) Raw allocation and deallocation of requests is protected internally by
 * the HANDLEALLOC critical section.  This is currently the same as the HANDLE
 * CS, not sure why we have both...
 *
 * (2) Once allocated, a directly allocated request is intially held exclusively
 * by a single thread.  Direct allocation is common for send requests, but recv
 * requests are usually created differently.
 *
 * (3) Most receive requests are created as the result of a call to FDP_or_AEU
 * or FDU_or_AEP.  Calls to these functions (along with the other receive queue
 * functions) must be inside a MSGQUEUE CS.  This CS protects the queue data
 * structures as well as any fields inside the requests while they are in the
 * queue.  For example, assume a call to FDU_or_AEP, as in MPID_Recv.  If the
 * FDU case hits, the MSGQUEUE CS may be released immediately after the call.
 * If the AEP case hits, however, the MSGQUEUE CS must remain held until any
 * request field manipulation (such as dev.recv_pending_count) is complete.
 *
 * (4) In both the send and receive request cases, there is usually a particular
 * thread in some upper-level code (e.g. MPI_Send) with interest in the
 * completion of the request.  This may or may not be a thread that is also
 * making progress on this request (often not).  The upper level code must not
 * attempt to access any request fields (such as the status) until completion is
 * signalled by the lower layer.
 *
 * (5) Once removed from the receive queue, the request is once again
 * exclusively owned by the dequeuing thread.  From here, the dequeuing thread
 * may do whatever it wants with the request without holding any CS, until it
 * signals the request's completion.  Signalling completion indicates that the
 * thread in the upper layer polling on it may access the rest of the fields in
 * the request.  This completion signalling is lock-free and must be implemented
 * carefully to work correctly in the face of optimizing compilers and CPUs.
 * The upper-level thread now wholly owns the request until it is deallocated.
 *
 * (6) In ch3:nemesis at least, multithreaded access to send requests is managed
 * by the MPIDCOMM (progress engine) CS.  The completion signalling pattern
 * applies here (think MPI_Isend/MPI_Wait).
 *
 * (7) Request cancellation is tricky-ish.  For send cancellation, it is
 * possible that the completion counter is actually *incremented* because a
 * pkt is sent to the recipient asking for remote cancellation.  By asking for
 * cancellation (of any kind of req), the upper layer gives up its exclusive
 * access to the request and must wait for the completion counter to drop to 0
 * before exclusively accessing the request fields.
 *
 * The completion counter is a reference count, much like the object liveness
 * reference count.  However it differs from a normal refcount because of
 * guarantees in the MPI Standard.  Applications must not attempt to complete
 * (wait/test/free) a given request concurrently in two separate threads.  So
 * checking for cc==0 is safe because only one thread is ever allowed to make
 * that check.
 *
 * A non-zero completion count must always be accompanied by a normal reference
 * that is logically held by the progress engine.  Similarly, once the
 * completion counter drops to zero, the progress engine is expected to release
 * its reference.
 */
/* lock ordering: if MPIDCOMM+MSGQUEUE must be aquired at the same time, then
 * the order should be to acquire MPIDCOMM first, then MSGQUEUE.  Release in
 * reverse order. */
#define MPIU_THREAD_CS_ENTER_MSGQUEUE(context_)           \
    do {                                                  \
        MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(msgq_mutex) \
        MPIU_THREAD_CS_ASSERT_ENTER_HELPER(MSGQUEUE);     \
    } while (0)
#define MPIU_THREAD_CS_EXIT_MSGQUEUE(context_)            \
    do {                                                  \
        MPIU_THREAD_CS_ASSERT_EXIT_HELPER(MSGQUEUE);      \
        MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(msgq_mutex)  \
    } while (0)

/* change to 1 to enable some iffy CS assert_held logic */
#if 0
#define MPIU_THREAD_CS_ASSERT_HELD(name_)                                                      \
    do {                                                                                       \
        int same = FALSE;                                                                      \
        MPID_Thread_id_t me;                                                                   \
        if (MPIU_ISTHREADED) {                                                                 \
            MPID_Thread_self(&me);                                                                 \
            MPID_Thread_same(&me, &MPIR_ThreadInfo.cs_holder[MPIU_THREAD_CS_NAME_##name_], &same); \
            MPIU_Assert_fmt_msg(same, ("me=%#08x holder=%#08x",                                              \
                                       ((unsigned)(size_t)me),                                                       \
                                       ((unsigned)(size_t)MPIR_ThreadInfo.cs_holder[MPIU_THREAD_CS_NAME_##name_]))); \
        }                                                                                      \
    } while (0)

/* MT FIXME buggy, these defs don't work with recursive mutexes as implemented */
#define MPIU_THREAD_CS_ASSERT_ENTER_HELPER(name_)                                                        \
    do {                                                                                                 \
        MPID_Thread_self(&MPIR_ThreadInfo.cs_holder[MPIU_THREAD_CS_NAME_##name_]);                       \
    } while (0)
#define MPIU_THREAD_CS_ASSERT_EXIT_HELPER(name_)                                                         \
    do {                                                                                                 \
        /* FIXME hack, assumes all 0-bytes won't be a valid thread ID */                                 \
        memset(&MPIR_ThreadInfo.cs_holder[MPIU_THREAD_CS_NAME_##name_], 0x00, sizeof(MPID_Thread_id_t)); \
    } while (0)
#endif

/* MT FIXME should this CS share the global_mutex?  If yes, how do
 * we avoid accidental deadlock? */
#define MPIU_THREAD_CS_ENTER_INITFLAG(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(global_mutex)
#define MPIU_THREAD_CS_EXIT_INITFLAG(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(global_mutex)

#define MPIU_THREAD_CS_ENTER_PMI(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(pmi_mutex)
#define MPIU_THREAD_CS_EXIT_PMI(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(pmi_mutex)

#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(ctx_mutex)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(ctx_mutex)
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(ctx_mutex)

/* must include semicolon */
#define MPIU_THREAD_OBJECT_HOOK MPID_Thread_mutex_t pobj_mutex;

#define MPIU_THREAD_CS_ENTER_MPI_OBJ(context_)                      \
    do {                                                            \
        if (MPIU_ISTHREADED)                                        \
            MPID_Thread_mutex_lock(&context_->pobj_mutex);          \
    } while (0)
#define MPIU_THREAD_CS_EXIT_MPI_OBJ(context_)                       \
    do {                                                            \
        if (MPIU_ISTHREADED)                                        \
            MPID_Thread_mutex_unlock(&context_->pobj_mutex);        \
    } while (0)

/* initialize any per-object mutex in an MPI handle allocated object.  Mainly
 * provided to reduce the amount of #ifdef/#endif code outside of this file. */
#define MPIU_THREAD_MPI_OBJ_INIT(objptr_)                        \
    do {                                                         \
        int err_ = 0;                                            \
        MPID_Thread_mutex_create(&(objptr_)->pobj_mutex, &err_); \
        MPIU_Assert(!err_);                                      \
    } while (0)

#define MPIU_THREAD_MPI_OBJ_FINALIZE(objptr_)                    \
    do {                                                         \
        int err;                                                 \
        MPID_Thread_mutex_destroy(&(objptr_)->pobj_mutex, &err); \
        MPIU_Assert(!err);                                       \
    } while (0)                                                  \


#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_LOCK_FREE
/* Updates to shared data and access to shared services is handled without
   locks where ever possible. */
#error lock-free not yet implemented

#else
#error Unrecognized thread granularity
#endif

#elif !defined(MPID_DEVICE_DEFINES_THREAD_CS) /* && !defined(MPICH_IS_THREADED) */
#define MPIU_THREAD_CS_INIT
#define MPIU_THREAD_CS_FINALIZE
#define MPIU_THREAD_CS_ENTER(_name,_context)
#define MPIU_THREAD_CS_EXIT(_name,_context)
#define MPIU_THREAD_CS_YIELD(_name,_context)
#endif /* MPICH_IS_THREADED */

/* catch-all */
#ifndef MPIU_THREAD_OBJECT_HOOK
#  define MPIU_THREAD_OBJECT_HOOK /**/
#  define MPIU_THREAD_OBJECT_HOOK_INIT /**/
#endif

#ifndef MPIU_THREAD_CS_ASSERT_HELD
#define MPIU_THREAD_CS_ASSERT_HELD(name_) do{/*nothing*/}while(0)
#define MPIU_THREAD_CS_ASSERT_ENTER_HELPER(name_)
#define MPIU_THREAD_CS_ASSERT_EXIT_HELPER(name_)
#endif


/* define a type for the completion counter */
#if defined(MPICH_IS_THREADED)
# if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL

/* memory barriers aren't needed in this impl, because all access to completion
 * counters is done while holding the ALLFUNC critical section */
typedef volatile int MPID_cc_t;
#  define MPID_cc_set(cc_ptr_, val_) (*(cc_ptr_)) = (val_)
#  define MPID_cc_is_complete(cc_ptr_) (0 == *(cc_ptr_))
#define MPID_cc_decr(cc_ptr_, incomplete_)     \
do {                                           \
    *(incomplete_) = --(*(cc_ptr_));           \
} while (0)
#define MPID_cc_incr(cc_ptr_, was_incomplete_) \
do {                                           \
    *(was_incomplete_) = (*(cc_ptr_))++;       \
} while (0)

# elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT

#include "opa_primitives.h"

typedef OPA_int_t MPID_cc_t;

/* implies no barrier, since this routine should only be used for request
 * initialization */
static inline void MPID_cc_set(MPID_cc_t *cc_ptr, int val)
{
    if (val == 0) {
        /* values other than 0 do not enforce any ordering, and therefore do not
         * start a HB arc */
        /* MT FIXME using cc_set in this way is sloppy.  Sometimes the caller
         * really does know that the cc value may cleared, but more likely this
         * is just a hack to avoid the work of figuring out what the cc value
         * currently is and decrementing it instead. */
        /* barrier ensures that any state written before indicating completion is
         * seen by the thread polling on the cc.  If OPA adds store-release
         * semantics, we can convert to that instead. */
        OPA_write_barrier();
        MPL_VG_ANNOTATE_HAPPENS_BEFORE(cc_ptr);
    }
#if defined(MPL_VG_AVAILABLE)
    /* MT subtle: store_int is actually safe to use, but Helgrind/DRD/TSan all
     * view the store/load pair as a race.  Using an atomic operation for the
     * store side makes all three happy.  DRD & TSan also support
     * ANNOTATE_BENIGN_RACE, but Helgrind does not. */
    OPA_swap_int(cc_ptr, val);
#else
    OPA_store_int(cc_ptr, val);
#endif
}

ATTRIBUTE((unused))
static MPIU_DBG_INLINE_KEYWORD int MPID_cc_is_complete(MPID_cc_t *cc_ptr)
{
    int complete;
    complete = (0 == OPA_load_int(cc_ptr));
    if (complete) {
        MPL_VG_ANNOTATE_HAPPENS_AFTER(cc_ptr);
        OPA_read_barrier();
    }
    return complete;
}

/* incomplete_==TRUE iff the cc > 0 after the decr */
#define MPID_cc_decr(cc_ptr_, incomplete_)                                  \
    do {                                                                    \
        OPA_write_barrier();                                                \
        MPL_VG_ANNOTATE_HAPPENS_BEFORE(cc_ptr_);                            \
        *(incomplete_) = !OPA_decr_and_test_int(cc_ptr_);                   \
        /* TODO check if this HA is actually necessary */                   \
        if (!*(incomplete_)) {                                              \
            MPL_VG_ANNOTATE_HAPPENS_AFTER(cc_ptr_);                         \
        }                                                                   \
    } while (0)

/* MT FIXME does this need a HB/HA annotation?  This macro is only used for
 * cancel_send right now. */
/* was_incomplete_==TRUE iff the cc==0 before the decr */
#define MPID_cc_incr(cc_ptr_, was_incomplete_)                              \
    do {                                                                    \
        *(was_incomplete_) = OPA_fetch_and_incr_int(cc_ptr_);               \
    } while (0)

# else
#  error "unexpected thread granularity"
# endif /* granularity */
#else /* !defined(MPICH_IS_THREADED) */

typedef int MPID_cc_t;
# define MPID_cc_set(cc_ptr_, val_) (*(cc_ptr_)) = (val_)
# define MPID_cc_is_complete(cc_ptr_) (0 == *(cc_ptr_))
#define MPID_cc_decr(cc_ptr_, incomplete_)     \
do {                                           \
    *(incomplete_) = --(*(cc_ptr_));           \
} while (0)
#define MPID_cc_incr(cc_ptr_, was_incomplete_) \
do {                                           \
    *(was_incomplete_) = (*(cc_ptr_))++;       \
} while (0)

#endif /* defined(MPICH_IS_THREADED) */


/* "publishes" the obj with handle value (handle_) via the handle pointer
 * (hnd_lval_).  That is, it is a version of the following statement that fixes
 * memory consistency issues:
 *     (hnd_lval_) = (handle_);
 *
 * assumes that the following is always true: typeof(*hnd_lval_ptr_)==int
 */
/* This could potentially be generalized beyond MPI-handle objects, but we
 * should only take that step after seeing good evidence of its use.  A general
 * macro (that is portable to non-gcc compilers) will need type information to
 * make the appropriate volatile cast. */
/* Ideally _GLOBAL would use this too, but we don't want to count on OPA
 * availability in _GLOBAL mode.  Instead the ALLFUNC critical section should be
 * used. */
#if defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT)
#define MPIU_OBJ_PUBLISH_HANDLE(hnd_lval_, handle_)                                 \
    do {                                                                            \
        if (MPIU_ISTHREADED) {                                                      \
            /* wmb ensures all read-only object field values are seen before the */ \
            /* handle value is seen at the application level */                     \
            OPA_write_barrier();                                                    \
            /* volatile ensures lval is not speculatively read or written */        \
            *(volatile int *)&(hnd_lval_) = (handle_);                              \
        }                                                                           \
        else {                                                                      \
            (hnd_lval_) = (handle_);                                                \
        }                                                                           \
    } while (0)
#else
#define MPIU_OBJ_PUBLISH_HANDLE(hnd_lval_, handle_)  \
    do {                                             \
        (hnd_lval_) = (handle_);                     \
    } while (0)
#endif

#ifndef MPIU_THREAD_MPI_OBJ_INIT
#define MPIU_THREAD_MPI_OBJ_INIT(objptr_) do {} while (0)
#endif
#ifndef MPIU_THREAD_MPI_OBJ_FINALIZE
#define MPIU_THREAD_MPI_OBJ_FINALIZE(objptr_) do {} while (0)
#endif

#endif /* !defined(MPIIMPLTHREAD_H_INCLUDED) */

