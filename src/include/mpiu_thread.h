/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_H_INCLUDED)
#define MPIU_THREAD_H_INCLUDED

#include "mpichconf.h" /* defines MPIU_THREAD_PACKAGE_NAME */

#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif

/* _INVALID exists to avoid accidental macro evaluations to 0 */
#define MPIU_THREAD_PACKAGE_INVALID 0
#define MPIU_THREAD_PACKAGE_NONE    1
#define MPIU_THREAD_PACKAGE_POSIX   2
#define MPIU_THREAD_PACKAGE_SOLARIS 3
#define MPIU_THREAD_PACKAGE_WIN     4

#if defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_POSIX)
#  include "thread/mpiu_thread_posix_types.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_SOLARIS)
#  include "thread/mpiu_thread_solaris_types.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_WIN)
#  include "thread/mpiu_thread_win_types.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_NONE)
typedef int MPIU_Thread_mutex_t;
typedef int MPIU_Thread_cond_t;
typedef int MPIU_Thread_id_t;
typedef int MPIU_Thread_tls_t;
#else
#  error "thread package not defined or unknown"
#endif


/*
 * threading function prototypes
 *
 * Typically some or all of these are actually implemented as macros
 * rather than actual or even inline functions.
 */

typedef void (* MPIU_Thread_func_t)(void * data);

/*@
  MPIU_Thread_create - create a new thread

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
void MPIU_Thread_create(MPIU_Thread_func_t func, void * data, MPIU_Thread_id_t * id, int * err);

/*@
  MPIU_Thread_exit - exit from the current thread
@*/
void MPIU_Thread_exit(void);

/*@
  MPIU_Thread_self - get the identifier of the current thread

  Output Parameter:
. id - identifier of current thread
@*/
void MPIU_Thread_self(MPIU_Thread_id_t * id);

/*@
  MPIU_Thread_same - compare two threads identifiers to see if refer to the same thread

  Input Parameters:
+ id1 - first identifier
- id2 - second identifier
  
  Output Parameter:
. same - TRUE if the two threads identifiers refer to the same thread; FALSE otherwise
@*/
void MPIU_Thread_same(MPIU_Thread_id_t * id1, MPIU_Thread_id_t * id2, int * same);

/*@
  MPIU_Thread_yield - voluntarily relinquish the CPU, giving other threads an opportunity to run
@*/
void MPIU_Thread_yield(void);


/*
 *    Mutexes
 */

/*@
  MPIU_Thread_mutex_create - create a new mutex
  
  Output Parameters:
+ mutex - mutex
- err - error code (non-zero indicates an error has occurred)
@*/
void MPIU_Thread_mutex_create(MPIU_Thread_mutex_t * mutex, int * err);

/*@
  MPIU_Thread_mutex_destroy - destroy an existing mutex
  
  Input Parameter:
. mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_mutex_destroy(MPIU_Thread_mutex_t * mutex, int * err);

/*@
  MPIU_Thread_lock - acquire a mutex
  
  Input Parameter:
. mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_mutex_lock(MPIU_Thread_mutex_t * mutex, int * err);

/*@
  MPIU_Thread_unlock - release a mutex
  
  Input Parameter:
. mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_mutex_unlock(MPIU_Thread_mutex_t * mutex, int * err);

/*@
  MPIU_Thread_mutex_trylock - try to acquire a mutex, but return even if unsuccessful
  
  Input Parameter:
. mutex - mutex

  Output Parameters:
+ flag - flag
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_mutex_trylock(MPIU_Thread_mutex_t * mutex, int * flag, int * err);


/*
 * Condition Variables
 */

/*@
  MPIU_Thread_cond_create - create a new condition variable
  
  Output Parameters:
+ cond - condition variable
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_cond_create(MPIU_Thread_cond_t * cond, int * err);

/*@
  MPIU_Thread_cond_destroy - destroy an existinga condition variable
  
  Input Parameter:
. cond - condition variable

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_cond_destroy(MPIU_Thread_cond_t * cond, int * err);

/*@
  MPIU_Thread_cond_wait - wait (block) on a condition variable
  
  Input Parameters:
+ cond - condition variable
- mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred

  Notes:
  This function may return even though another thread has not requested that a thread be released.  Therefore, the calling
  program must wrap the function in a while loop that verifies program state has changed in a way that warrants letting the
  thread proceed.
@*/
void MPIU_Thread_cond_wait(MPIU_Thread_cond_t * cond, MPIU_Thread_mutex_t * mutex, int * err);

/*@
  MPIU_Thread_cond_broadcast - release all threads currently waiting on a condition variable
  
  Input Parameter:
. cond - condition variable

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_cond_broadcast(MPIU_Thread_cond_t * cond, int * err);

/*@
  MPIU_Thread_cond_signal - release one thread currently waitng on a condition variable
  
  Input Parameter:
. cond - condition variable

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_cond_signal(MPIU_Thread_cond_t * cond, int * err);


/*
 * Thread Local Storage
 */
typedef void (*MPIU_Thread_tls_exit_func_t)(void * value);


/*@
  MPIU_Thread_tls_create - create a thread local storage space

  Input Parameter:
. exit_func - function to be called when the thread exists; may be NULL is a callback is not desired
  
  Output Parameters:
+ tls - new thread local storage space
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_tls_create(MPIU_Thread_tls_exit_func_t exit_func, MPIU_Thread_tls_t * tls, int * err);

/*@
  MPIU_Thread_tls_destroy - destroy a thread local storage space
  
  Input Parameter:
. tls - thread local storage space to be destroyed

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
  
  Notes:
  The destroy function associated with the thread local storage will not called after the space has been destroyed.
@*/
void MPIU_Thread_tls_destroy(MPIU_Thread_tls_t * tls, int * err);

/*@
  MPIU_Thread_tls_set - associate a value with the current thread in the thread local storage space
  
  Input Parameters:
+ tls - thread local storage space
- value - value to associate with current thread

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_tls_set(MPIU_Thread_tls_t * tls, void * value, int * err);

/*@
  MPIU_Thread_tls_get - obtain the value associated with the current thread from the thread local storage space
  
  Input Parameter:
. tls - thread local storage space

  Output Parameters:
+ value - value associated with current thread
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
void MPIU_Thread_tls_get(MPIU_Thread_tls_t * tls, void ** value, int * err);

/* Error values */
#define MPIU_THREAD_SUCCESS MPIU_THREAD_ERR_SUCCESS
#define MPIU_THREAD_ERR_SUCCESS 0
/* FIXME: Define other error codes.  For now, any non-zero value is an error. */

/* Implementation specific function definitions (usually in the form of macros) */
#if defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_POSIX)
#  include "thread/mpiu_thread_posix_funcs.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_SOLARIS)
#  include "thread/mpiu_thread_solaris_funcs.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_WIN)
#  include "thread/mpiu_thread_win_funcs.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_NONE)
/* do nothing */
#else
#  error "thread package not defined or unknown"
#endif

#endif /* !defined(MPIU_THREAD_H_INCLUDED) */
