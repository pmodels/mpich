/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIDU_THREAD_H_INCLUDED)
#define MPIDU_THREAD_H_INCLUDED

#include "mpiutil.h"

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

typedef MPIU_Thread_mutex_t MPIDU_Thread_mutex_t;
typedef MPIU_Thread_cond_t  MPIDU_Thread_cond_t;
typedef MPIU_Thread_id_t    MPIDU_Thread_id_t;
typedef MPIU_Thread_tls_t   MPIDU_Thread_tls_t;
typedef MPIU_Thread_func_t  MPIDU_Thread_func_t;

/*M MPIU_THREAD_CS_ENTER - Enter a named critical section

  Input Parameters:
+ _name - name of the critical section
- _context - A context (typically an object) of the critical section

M*/
#define MPIDU_THREAD_CS_ENTER       MPIU_THREAD_CS_ENTER

/*M MPIU_THREAD_CS_EXIT - Exit a named critical section

  Input Parameters:
+ _name - cname of the critical section
- _context - A context (typically an object) of the critical section

M*/
#define MPIDU_THREAD_CS_EXIT        MPIU_THREAD_CS_EXIT

/*M MPIU_THREAD_CS_YIELD - Temporarily release a critical section and yield
    to other threads

  Input Parameters:
+ _name - cname of the critical section
- _context - A context (typically an object) of the critical section

  M*/
#define MPIDU_THREAD_CS_YIELD       MPIU_THREAD_CS_YIELD

#define MPIDU_THREAD_CHECK_BEGIN    MPIU_THREAD_CHECK_BEGIN
#define MPIDU_THREAD_CHECK_END      MPIU_THREAD_CHECK_END

/*@
  MPIDU_Thread_create - create a new thread

  Input Parameters:
+ func - function to run in new thread
- data - data to be passed to thread function

  Output Parameters:
+ id - identifier for the new thread
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred

  Notes:
  The thread is created in a detach state, meaning that is may not be waited upon.  If another thread needs to wait for this
  thread to complete, the threads must provide their own synchronization mechanism.
@*/
#define MPIDU_Thread_create       MPIU_Thread_create

/*@
  MPIDU_Thread_exit - exit from the current thread
@*/
#define MPIDU_Thread_exit         MPIU_Thread_exit

/*@
  MPIDU_Thread_self - get the identifier of the current thread

  Output Parameter:
. id - identifier of current thread
@*/
#define MPIDU_Thread_self         MPIU_Thread_self

/*@
  MPIDU_Thread_same - compare two threads identifiers to see if refer to the same thread

  Input Parameters:
+ id1 - first identifier
- id2 - second identifier

  Output Parameter:
. same - TRUE if the two threads identifiers refer to the same thread; FALSE otherwise
@*/
#define MPIDU_Thread_same       MPIU_Thread_same

/*@
  MPIDU_Thread_yield - voluntarily relinquish the CPU, giving other threads an opportunity to run
@*/
#define MPIDU_Thread_same       MPIU_Thread_same

/*
 *    Mutexes
 */

/*@
  MPIDU_Thread_mutex_create - create a new mutex

  Output Parameters:
+ mutex - mutex
- err - error code (non-zero indicates an error has occurred)
@*/
#define MPIDU_Thread_mutex_create  MPIU_Thread_mutex_create

/*@
  MPIDU_Thread_mutex_destroy - destroy an existing mutex

  Input Parameter:
. mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_mutex_destroy  MPIU_Thread_mutex_destroy

/*@
  MPIDU_Thread_lock - acquire a mutex

  Input Parameter:
. mutex - mutex
@*/
#define MPIDU_Thread_mutex_lock MPIU_Thread_mutex_lock

/*@
  MPIDU_Thread_unlock - release a mutex

  Input Parameter:
. mutex - mutex
@*/
#define MPIDU_Thread_mutex_unlock MPIU_Thread_mutex_unlock

/*
 * Condition Variables
 */

/*@
  MPIDU_Thread_cond_create - create a new condition variable

  Output Parameters:
+ cond - condition variable
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_cond_create MPIU_Thread_cond_create

/*@
  MPIDU_Thread_cond_destroy - destroy an existinga condition variable

  Input Parameter:
. cond - condition variable

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero
        for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_cond_destroy MPIU_Thread_cond_destroy

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
#define MPIDU_Thread_cond_wait MPIU_Thread_cond_wait

/*@
  MPIDU_Thread_cond_broadcast - release all threads currently waiting on a condition variable

  Input Parameter:
. cond - condition variable
@*/
#define MPIDU_Thread_cond_broadcast MPIU_Thread_cond_broadcast

/*@
  MPIDU_Thread_cond_signal - release one thread currently waitng on a condition variable

  Input Parameter:
. cond - condition variable
@*/
#define MPIDU_Thread_cond_signal MPIU_Thread_cond_signal

/*
 * Thread Local Storage
 */
/*@
  MPIDU_Thread_tls_create - create a thread local storage space

  Input Parameter:
. exit_func - function to be called when the thread exists; may be NULL if a
  callback is not desired

  Output Parameters:
+ tls - new thread local storage space
- err - location to store the error code; pointer may be NULL; error is zero
        for success, non-zero if a failure occurred
@*/
#define MPIDU_Thread_tls_create MPIU_Thread_tls_create

/*@
  MPIDU_Thread_tls_destroy - destroy a thread local storage space

  Input Parameter:
. tls - thread local storage space to be destroyed

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero
        for success, non-zero if a failure occurred

  Notes:
  The destroy function associated with the thread local storage will not
  called after the space has been destroyed.
@*/
#define MPIDU_Thread_tls_destroy MPIU_Thread_tls_destroy

/*@
  MPIDU_Thread_tls_set - associate a value with the current thread in the
  thread local storage space

  Input Parameters:
+ tls - thread local storage space
- value - value to associate with current thread
@*/
#define MPIDU_Thread_tls_set MPIU_Thread_tls_set

/*@
  MPIDU_Thread_tls_get - obtain the value associated with the current thread
  from the thread local storage space

  Input Parameter:
. tls - thread local storage space

  Output Parameter:
. value - value associated with current thread
@*/
#define MPIDU_Thread_tls_get MPIU_Thread_tls_get


#define MPIDU_cc_t            MPIU_cc_t
#define MPIDU_cc_get          MPIU_cc_get
#define MPIDU_cc_set          MPIU_cc_set
#define MPIDU_cc_is_complete  MPIU_cc_is_complete
#define MPIDU_cc_incr         MPIU_cc_incr
#define MPIDU_cc_decr         MPIU_cc_decr

#define MPIDU_OBJ_PUBLISH_HANDLE  MPIU_OBJ_PUBLISH_HANDLE

#define MPIDU_THREADPRIV_INITKEY  MPIU_THREADPRIV_INITKEY
#define MPIDU_THREADPRIV_INIT     MPIU_THREADPRIV_INIT
#define MPIDU_THREADPRIV_GET      MPIU_THREADPRIV_GET
#define MPIDU_THREADPRIV_DECL     MPIU_THREADPRIV_DECL
#define MPIDU_THREADPRIV_FIELD    MPIU_THREADPRIV_FIELD
#define MPIDU_THREADPRIV_FINALIZE MPIU_THREADPRIV_FINALIZE


#endif /* !defined(MPIDU_THREAD_H_INCLUDED) */
