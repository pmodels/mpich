/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_THREAD_FALLBACK_H_INCLUDED
#define MPIDU_THREAD_FALLBACK_H_INCLUDED

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ENABLE_HEAVY_YIELD
      category    : THREADS
      type        : boolean
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If enabled, use nanosleep to ensure other threads have a chance to grab the lock.
        Note: this may not work with some thread runtimes, e.g. non-preemptive user-level
        threads.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* some important critical section names:
 *   GLOBAL - entered/exited at beginning/end of (nearly) every MPI_ function
 *   INIT - entered before MPID_Init and exited near the end of MPI_Init(_thread)
 * See the analysis of the MPI routines for thread usage properties.  Those
 * routines considered "Access Only" do not require GLOBAL.  That analysis
 * was very general; in MPICH, some routines may have internal shared
 * state that isn't required by the MPI specification.  Perhaps the
 * best example of this is the MPI_ERROR_STRING routine, where the
 * instance-specific error messages make use of shared state, and hence
 * must be accessed in a thread-safe fashion (e.g., require an GLOBAL
 * critical section).  With such routines removed, the set of routines
 * that (probably) do not require GLOBAL include:
 *
 * MPI_CART_COORDS, MPI_CART_GET, MPI_CART_MAP, MPI_CART_RANK, MPI_CART_SHIFT,
 * MPI_CART_SUB, MPI_CARTDIM_GET, MPI_COMM_GET_NAME,
 * MPI_COMM_RANK, MPI_COMM_REMOTE_SIZE,
 * MPI_COMM_SET_NAME, MPI_COMM_SIZE, MPI_COMM_TEST_INTER, MPI_ERROR_CLASS,
 * MPI_FILE_GET_AMODE, MPI_FILE_GET_ATOMICITY, MPI_FILE_GET_BYTE_OFFSET,
 * MPI_FILE_GET_POSITION, MPI_FILE_GET_POSITION_SHARED, MPI_FILE_GET_SIZE
 * MPI_FILE_GET_TYPE_EXTENT, MPI_FILE_SET_SIZE,
g * MPI_FINALIZED, MPI_GET_COUNT, MPI_GET_ELEMENTS, MPI_GRAPH_GET,
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
 * Note that other issues may force a routine to include the GLOBAL
 * critical section, such as debugging information that requires shared
 * state.  Such situations should be avoided where possible.
 */

typedef struct {
    MPL_thread_mutex_t mutex;
    MPL_thread_id_t owner;
    int count;
} MPIDU_Thread_mutex_t;
typedef MPL_thread_cond_t MPIDU_Thread_cond_t;

typedef MPL_thread_id_t MPIDU_Thread_id_t;
typedef MPL_thread_func_t MPIDU_Thread_func_t;

/*M MPIDU_THREAD_CS_ENTER - Enter a named critical section

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/

/*M MPIDU_THREAD_CS_EXIT - Exit a named critical section

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/

/*M MPIDU_THREAD_CS_YIELD - Temporarily release a critical section and yield
    to other threads

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/

/*M MPIDU_THREAD_ASSERT_IN_CS - Assert whether the code is inside a critical section

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/

#if defined(MPICH_IS_THREADED)
#define MPIDU_THREAD_CS_ENTER(name, mutex) MPIDUI_THREAD_CS_ENTER_##name(mutex)
#define MPIDU_THREAD_CS_EXIT(name, mutex) MPIDUI_THREAD_CS_EXIT_##name(mutex)
#define MPIDU_THREAD_CS_YIELD(name, mutex) MPIDUI_THREAD_CS_YIELD_##name(mutex)
#define MPIDU_THREAD_ASSERT_IN_CS(name, mutex) MPIDUI_THREAD_ASSERT_IN_CS_##name(mutex)

#else
#define MPIDU_THREAD_CS_ENTER(name, mutex)      /* NOOP */
#define MPIDU_THREAD_CS_EXIT(name, mutex)       /* NOOP */
#define MPIDU_THREAD_CS_YIELD(name, mutex)      /* NOOP */
#define MPIDU_THREAD_ASSERT_IN_CS(name, mutex)  /* NOOP */

#endif

/* ***************************************** */
#if defined(MPICH_IS_THREADED)

#define MPIDUI_THREAD_CS_ENTER_REC(mutex)                               \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            int equal_ = 0;                                             \
            MPL_thread_id_t self_, owner_;                              \
            MPL_thread_self(&self_);                                    \
            owner_ = mutex.owner;                                       \
            MPL_thread_same(&self_, &owner_, &equal_);                  \
            if (!equal_) {                                              \
                int err_ = 0;                                           \
                MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"enter MPIDU_Thread_mutex_lock %p", &mutex); \
                MPIDU_Thread_mutex_lock(&mutex, &err_, MPL_THREAD_PRIO_HIGH);\
                MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"exit MPIDU_Thread_mutex_lock %p", &mutex); \
                MPIR_Assert(err_ == 0);                                 \
                MPIR_Assert(mutex.count == 0);                          \
                MPL_thread_self(&mutex.owner);                          \
            }                                                           \
            mutex.count++;                                              \
        }                                                               \
    } while (0)

#define MPIDUI_THREAD_CS_ENTER(mutex)                                   \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            int equal_ = 0;                                             \
            MPL_thread_id_t self_, owner_;                              \
            MPL_thread_self(&self_);                                    \
            owner_ = mutex.owner;                                       \
            MPL_thread_same(&self_, &owner_, &equal_);                  \
            if (!equal_) {                                              \
                int err_ = 0;                                           \
                MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"enter MPIDU_Thread_mutex_lock %p", &mutex); \
                MPIDU_Thread_mutex_lock(&mutex, &err_, MPL_THREAD_PRIO_HIGH);\
                MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"exit MPIDU_Thread_mutex_lock %p", &mutex); \
                MPIR_Assert(err_ == 0);                                 \
                MPIR_Assert(mutex.count == 0);                          \
                MPL_thread_self(&mutex.owner);                          \
            } else {                                                    \
                /* assert all recursive usage */                        \
                MPIR_Assert(0);                                         \
            }                                                           \
            mutex.count++;                                              \
        }                                                               \
    } while (0)

#define MPIDUI_THREAD_CS_EXIT(mutex)                                    \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            mutex.count--;                                              \
            MPIR_Assert(mutex.count >= 0);                              \
            if (mutex.count == 0) {                                     \
                mutex.owner = 0;                                        \
                int err_ = 0;                                           \
                MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"MPIDU_Thread_mutex_unlock %p", &mutex); \
                MPIDU_Thread_mutex_unlock(&mutex, &err_);               \
                MPIR_Assert(err_ == 0);                                 \
            }                                                           \
        }                                                               \
    } while (0)

#define MPIDUI_THREAD_CS_YIELD(mutex)                                   \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            int err_ = 0, equal_ = 0;                                   \
            MPL_thread_id_t self_;                                      \
            MPL_thread_self(&self_);                                    \
            MPL_thread_same(&self_, &mutex.owner, &equal_);             \
            MPIR_Assert(equal_ && mutex.count > 0);                     \
            MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"enter MPIDUI_THREAD_CS_YIELD %p", &mutex); \
            int saved_count_ = mutex.count;                             \
            MPL_thread_id_t saved_owner_ = mutex.owner;                 \
            MPIR_Assert(saved_count_ > 0);                              \
            mutex.count = 0;                                            \
            mutex.owner = 0;                                            \
            MPIDU_Thread_mutex_unlock(&mutex, &err_);                   \
            MPIR_Assert(err_ == 0);                                     \
            MPL_thread_yield();                                         \
            MPIDU_Thread_mutex_lock(&mutex, &err_, MPL_THREAD_PRIO_LOW);\
            MPIR_Assert(mutex.count == 0);                              \
            mutex.count = saved_count_;                                 \
            mutex.owner = saved_owner_;                                 \
            MPIR_Assert(err_ == 0);                                     \
            MPL_DBG_MSG_P(MPIR_DBG_THREAD,VERBOSE,"exit MPIDUI_THREAD_CS_YIELD %p", &mutex); \
            MPIR_Assert(err_ == 0);                                     \
        }                                                               \
    } while (0)

/* debug macros */

/* NOTE this macro is only available with VCI granularity */
#define MPIDUI_THREAD_ASSERT_IN_CS(mutex) \
    do { \
        if (MPIR_ThreadInfo.isThreaded) {  \
            int equal_ = 0;                                             \
            MPL_thread_id_t self_;                                      \
            MPL_thread_self(&self_);                                    \
            MPL_thread_same(&self_, &mutex.owner, &equal_);             \
            MPIR_Assert(equal_ && mutex.count >= 1); \
        } \
    } while (0)

/* MPICH_THREAD_GRANULARITY (set via `--enable-thread-cs=...`) activates one set of locks */

/* GLOBAL is only enabled with MPICH_THREAD_GRANULARITY__GLOBAL */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
#define MPIDUI_THREAD_CS_ENTER_GLOBAL(mutex)  MPIDUI_THREAD_CS_ENTER(mutex)
#define MPIDUI_THREAD_CS_EXIT_GLOBAL(mutex)   MPIDUI_THREAD_CS_EXIT(mutex)
#define MPIDUI_THREAD_CS_YIELD_GLOBAL(mutex)  MPIDUI_THREAD_CS_YIELD(mutex)
#else
#define MPIDUI_THREAD_CS_ENTER_GLOBAL(mutex)    /* NOOP */
#define MPIDUI_THREAD_CS_EXIT_GLOBAL(mutex)     /* NOOP */
#define MPIDUI_THREAD_CS_YIELD_GLOBAL(mutex)    /* NOOP */
#endif

/* POBJ is only enabled with GRANULARITY__POBJ */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
#define MPIDUI_THREAD_CS_ENTER_POBJ(mutex)  MPIDUI_THREAD_CS_ENTER(mutex)
#define MPIDUI_THREAD_CS_EXIT_POBJ(mutex)   MPIDUI_THREAD_CS_EXIT(mutex)
#define MPIDUI_THREAD_CS_YIELD_POBJ(mutex)  MPIDUI_THREAD_CS_YIELD(mutex)
#else
#define MPIDUI_THREAD_CS_ENTER_POBJ(mutex)      /* NOOP */
#define MPIDUI_THREAD_CS_EXIT_POBJ(mutex)       /* NOOP */
#define MPIDUI_THREAD_CS_YIELD_POBJ(mutex)      /* NOOP */
#endif

/* VCI is only enabled with MPICH_THREAD_GRANULARITY__VCI */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
#define MPIDUI_THREAD_CS_ENTER_VCI(mutex) MPIDUI_THREAD_CS_ENTER(mutex)
#define MPIDUI_THREAD_CS_EXIT_VCI(mutex) MPIDUI_THREAD_CS_EXIT(mutex)
#define MPIDUI_THREAD_CS_YIELD_VCI(mutex) MPIDUI_THREAD_CS_YIELD(mutex)
#define MPIDUI_THREAD_ASSERT_IN_CS_VCI(mutex) MPIDUI_THREAD_ASSERT_IN_CS(mutex)
#else
#define MPIDUI_THREAD_CS_ENTER_VCI(mutex)       /* NOOP */
#define MPIDUI_THREAD_CS_EXIT_VCI(mutex)        /* NOOP */
#define MPIDUI_THREAD_CS_YIELD_VCI(mutex)       /* NOOP */
#define MPIDUI_THREAD_ASSERT_IN_CS_VCI(mutex)   /* NOOP */
#endif

#endif /* MPICH_IS_THREADED */

/* ***************************************** */
#define MPIDU_Thread_init         MPL_thread_init
#define MPIDU_Thread_finalize     MPL_thread_finalize
#define MPIDU_Thread_create       MPL_thread_create
#define MPIDU_Thread_exit         MPL_thread_exit
#define MPIDU_Thread_self         MPL_thread_self
#define MPIDU_Thread_join       MPL_thread_join
#define MPIDU_Thread_same       MPL_thread_same

#define MPIDU_Thread_yield() \
    do { \
        if (MPIR_CVAR_ENABLE_HEAVY_YIELD) { \
            /* note: sleep time may be rounded up to the granularity of the underlying clock */ \
            struct timespec t; \
            t.tv_sec = 0; \
            t.tv_nsec = 1; \
            nanosleep(&t, NULL); \
        } else { \
            MPL_thread_yield(); \
        } \
    } while (0)


/*
 *    Mutexes
 */

/*@
  MPIDU_Thread_mutex_create - create a new mutex

  Output Parameters:
+ mutex - mutex
- err - error code (non-zero indicates an error has occurred)
@*/
#define MPIDU_Thread_mutex_create(mutex_ptr_, err_ptr_)                 \
    do {                                                                \
        (mutex_ptr_)->owner = 0;                                        \
        (mutex_ptr_)->count = 0;                                        \
        MPL_thread_mutex_create(&(mutex_ptr_)->mutex, err_ptr_);     \
        MPL_DBG_MSG_P(MPIR_DBG_THREAD,TYPICAL,"Created MPL_thread_mutex %p", (mutex_ptr_)); \
    } while (0)

/*@
  MPIDU_Thread_mutex_destroy - destroy an existing mutex

  Input Parameter:
. mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_mutex_destroy(mutex_ptr_, err_ptr_)                \
    do {                                                                \
        MPL_DBG_MSG_P(MPIR_DBG_THREAD,TYPICAL,"About to destroy MPL_thread_mutex %p", (mutex_ptr_)); \
        MPL_thread_mutex_destroy(&(mutex_ptr_)->mutex, err_ptr_);       \
    } while (0)

/*@
  MPIDU_Thread_lock - acquire a mutex

  Input Parameter:
. mutex - mutex
@*/
#define MPIDU_Thread_mutex_lock(mutex_ptr_, err_ptr_, prio_)            \
    do {                                                                \
        MPL_thread_mutex_lock(&(mutex_ptr_)->mutex, err_ptr_, prio_);\
        MPIR_Assert(*err_ptr_ == 0);                                    \
    } while (0)

/*@
  MPIDU_Thread_unlock - release a mutex

  Input Parameter:
. mutex - mutex
@*/
#define MPIDU_Thread_mutex_unlock(mutex_ptr_, err_ptr_)                 \
    do {                                                                \
        MPL_thread_mutex_unlock(&(mutex_ptr_)->mutex, err_ptr_);        \
        MPIR_Assert(*err_ptr_ == 0);                                    \
    } while (0)

/*
 * Condition Variables
 */

/*@
  MPIDU_Thread_cond_create - create a new condition variable

  Output Parameters:
+ cond - condition variable
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_cond_create(cond_ptr_, err_ptr_)                   \
    do {                                                                \
        MPL_thread_cond_create(cond_ptr_, err_ptr_);                    \
        MPIR_Assert(*err_ptr_ == 0);                                    \
        MPL_DBG_MSG_P(MPIR_DBG_THREAD,TYPICAL,"Created MPL_thread_cond %p", (cond_ptr_)); \
    } while (0)

/*@
  MPIDU_Thread_cond_destroy - destroy an existinga condition variable

  Input Parameter:
. cond - condition variable

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero
        for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_cond_destroy(cond_ptr_, err_ptr_)                  \
    do {                                                                \
        MPL_DBG_MSG_P(MPIR_DBG_THREAD,TYPICAL,"About to destroy MPL_thread_cond %p", (cond_ptr_)); \
        MPL_thread_cond_destroy(cond_ptr_, err_ptr_);                   \
        MPIR_Assert(*err_ptr_ == 0);                                    \
    } while (0)

/*@
  MPIDU_Thread_cond_wait - wait (block) on a condition variable

  Input Parameters:
+ cond - condition variable
- mutex - mutex

  Notes:
  This function may return even though another thread has not requested that a
  thread be released.  Therefore, the calling
  program must wrap the function in a while loop that verifies program state
  has changed in a way that warrants letting the
  thread proceed.
@*/
#define MPIDU_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)         \
    do {                                                                \
        int saved_count_ = (mutex_ptr_)->count;                         \
        MPL_thread_id_t saved_owner_ = (mutex_ptr_)->owner;              \
        (mutex_ptr_)->count = 0;                                        \
        (mutex_ptr_)->owner = 0;                                        \
        MPL_DBG_MSG_FMT(MPIR_DBG_THREAD,TYPICAL,(MPL_DBG_FDEST,"Enter cond_wait on cond=%p mutex=%p",(cond_ptr_),&(mutex_ptr_)->mutex)); \
        MPL_thread_cond_wait(cond_ptr_, &(mutex_ptr_)->mutex, err_ptr_); \
        MPIR_Assert_fmt_msg(*((int *) err_ptr_) == 0,                   \
                            ("cond_wait failed, err=%d (%s)", *((int *) err_ptr_), strerror(*((int *) err_ptr_)))); \
        MPL_DBG_MSG_FMT(MPIR_DBG_THREAD,TYPICAL,(MPL_DBG_FDEST,"Exit cond_wait on cond=%p mutex=%p",(cond_ptr_),&(mutex_ptr_)->mutex)); \
        (mutex_ptr_)->count = saved_count_;                             \
        (mutex_ptr_)->owner = saved_owner_;                             \
    } while (0)

/*@
  MPIDU_Thread_cond_broadcast - release all threads currently waiting on a condition variable

  Input Parameter:
. cond - condition variable
@*/
#define MPIDU_Thread_cond_broadcast(cond_ptr_, err_ptr_)                \
    do {                                                                \
        MPL_DBG_MSG_P(MPIR_DBG_THREAD,TYPICAL,"About to cond_broadcast on MPL_thread_cond %p", (cond_ptr_)); \
        MPL_thread_cond_broadcast(cond_ptr_, err_ptr_);                 \
        MPIR_Assert_fmt_msg(*((int *) err_ptr_) == 0,                   \
                            ("cond_broadcast failed, err=%d (%s)", *((int *) err_ptr_), strerror(*((int *) err_ptr_)))); \
    } while (0)

/*@
  MPIDU_Thread_cond_signal - release one thread currently waitng on a condition variable

  Input Parameter:
. cond - condition variable
@*/
#define MPIDU_Thread_cond_signal(cond_ptr_, err_ptr_)                   \
    do {                                                                \
        MPL_DBG_MSG_P(MPIR_DBG_THREAD,TYPICAL,"About to cond_signal on MPL_thread_cond %p", (cond_ptr_)); \
        MPL_thread_cond_signal(cond_ptr_, err_ptr_);                    \
        MPIR_Assert_fmt_msg(*((int *) err_ptr_) == 0,                   \
                            ("cond_signal failed, err=%d (%s)", *((int *) err_ptr_), strerror(*((int *) err_ptr_)))); \
    } while (0)

#endif /* MPIDU_THREAD_FALLBACK_H_INCLUDED */
