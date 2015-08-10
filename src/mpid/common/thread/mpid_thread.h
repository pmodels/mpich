/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_THREAD_H_INCLUDED)
#define MPID_THREAD_H_INCLUDED

/*
 * Implementation specific type definitions
 *
 * (formerly lived in mpe_types.i)
 */

#include "mpiutil.h"

typedef MPIU_Thread_mutex_t MPID_Thread_mutex_t;
typedef MPIU_Thread_cond_t  MPID_Thread_cond_t;
typedef MPIU_Thread_id_t    MPID_Thread_id_t;
typedef MPIU_Thread_tls_t   MPID_Thread_tls_t;


/*
 * Threads
 */

typedef void (* MPID_Thread_func_t)(void * data);

/*@
  MPID_Thread_create - create a new thread

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
static inline void MPID_Thread_create(MPID_Thread_func_t func, void * data, MPID_Thread_id_t * id, int * err)
{
    MPIU_Thread_create(func,data,id,err);
}

/*@
  MPID_Thread_exit - exit from the current thread
@*/
static inline void MPID_Thread_exit(void)
{
    MPIU_Thread_exit();
}
/*@
  MPID_Thread_self - get the identifier of the current thread

  Output Parameter:
. id - identifier of current thread
@*/
static inline void MPID_Thread_self(MPID_Thread_id_t * id)
{
    MPIU_Thread_self(id);
}
/*@
  MPID_Thread_same - compare two threads identifiers to see if refer to the same thread

  Input Parameters:
+ id1 - first identifier
- id2 - second identifier
  
  Output Parameter:
. same - TRUE if the two threads identifiers refer to the same thread; FALSE otherwise
@*/
static inline void MPID_Thread_same(MPID_Thread_id_t * id1, MPID_Thread_id_t * id2, int * same)
{
    MPIU_Thread_same(id1,id2,same);
}
/*@
  MPID_Thread_yield - voluntarily relinquish the CPU, giving other threads an opportunity to run
@*/
static inline void MPID_Thread_yield(MPID_Thread_mutex_t *t)
{
    MPIU_Thread_yield(t);
}

/*
 *    Mutexes
 */

/*@
  MPID_Thread_mutex_create - create a new mutex
  
  Output Parameters:
+ mutex - mutex
- err - error code (non-zero indicates an error has occurred)
@*/
static inline void MPID_Thread_mutex_create(MPID_Thread_mutex_t * mutex, int * err)
{
    MPIU_Thread_mutex_create(mutex,err);
}
/*@
  MPID_Thread_mutex_destroy - destroy an existing mutex
  
  Input Parameter:
. mutex - mutex

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
static inline void MPID_Thread_mutex_destroy(MPID_Thread_mutex_t * mutex, int * err)
{
    MPIU_Thread_mutex_destroy(mutex,err);
}
/*@
  MPID_Thread_lock - acquire a mutex
  
  Input Parameter:
. mutex - mutex
@*/
static inline void MPID_Thread_mutex_lock(MPID_Thread_mutex_t * mutex)
{
    int err;
    MPIU_Thread_mutex_lock(mutex,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("mutex_lock failed, err=%d (%s)",err,MPIU_Strerror(err)));
}
/*@
  MPID_Thread_unlock - release a mutex
  
  Input Parameter:
. mutex - mutex
@*/
static inline void MPID_Thread_mutex_unlock(MPID_Thread_mutex_t * mutex)
{
    int err;
    MPIU_Thread_mutex_unlock(mutex,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("mutex_unlock failed, err=%d (%s)",err,MPIU_Strerror(err)));
}
/*@
  MPID_Thread_mutex_trylock - try to acquire a mutex, but return even if unsuccessful
  
  Input Parameter:
. mutex - mutex

  Output Parameter:
. flag - flag
@*/
static inline void MPID_Thread_mutex_trylock(MPID_Thread_mutex_t * mutex, int * flag)
{
    int err;
    MPIU_Thread_mutex_trylock(mutex,flag,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("mutex_trylock failed, err=%d (%s)",err,MPIU_Strerror(err)));
}

/*
 * Condition Variables
 */

/*@
  MPID_Thread_cond_create - create a new condition variable
  
  Output Parameters:
+ cond - condition variable
- err - location to store the error code; pointer may be NULL; error is zero for success, non-zero if a failure occurred
@*/
static inline void MPID_Thread_cond_create(MPID_Thread_cond_t * cond, int * err)
{
    MPIU_Thread_cond_create(cond,err);
}
/*@
  MPID_Thread_cond_destroy - destroy an existinga condition variable
  
  Input Parameter:
. cond - condition variable

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero 
        for success, non-zero if a failure occurred
@*/
static inline void MPID_Thread_cond_destroy(MPID_Thread_cond_t * cond, int * err)
{
    MPIU_Thread_cond_destroy(cond,err);
}
/*@
  MPID_Thread_cond_wait - wait (block) on a condition variable
  
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
static inline void MPID_Thread_cond_wait(MPID_Thread_cond_t * cond, MPID_Thread_mutex_t * mutex)
{
    int err;
    MPIU_Thread_cond_wait(cond,mutex,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("cond_wait failed, err=%d (%s)",err,MPIU_Strerror(err)));
}
/*@
  MPID_Thread_cond_broadcast - release all threads currently waiting on a condition variable
  
  Input Parameter:
. cond - condition variable
@*/
static inline void MPID_Thread_cond_broadcast(MPID_Thread_cond_t * cond)
{
    int err;
    MPIU_Thread_cond_broadcast(cond,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("cond_broadcast failed, err=%d (%s)",err,MPIU_Strerror(err)));
}
/*@
  MPID_Thread_cond_signal - release one thread currently waitng on a condition variable
  
  Input Parameter:
. cond - condition variable
@*/
static inline void MPID_Thread_cond_signal(MPID_Thread_cond_t * cond)
{
    int err;
    MPIU_Thread_cond_signal(cond,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("cond_signal failed, err=%d (%s)",err,MPIU_Strerror(err)));
}

/*
 * Thread Local Storage
 */
typedef void (*MPID_Thread_tls_exit_func_t)(void * value);
/*@
  MPID_Thread_tls_create - create a thread local storage space

  Input Parameter:
. exit_func - function to be called when the thread exists; may be NULL if a 
  callback is not desired
  
  Output Parameters:
+ tls - new thread local storage space
- err - location to store the error code; pointer may be NULL; error is zero 
        for success, non-zero if a failure occurred
@*/
static inline void MPID_Thread_tls_create(MPID_Thread_tls_exit_func_t exit_func, MPID_Thread_tls_t * tls, int * err)
{
    MPIU_Thread_tls_create(exit_func,tls,err);
}
/*@
  MPID_Thread_tls_destroy - destroy a thread local storage space
  
  Input Parameter:
. tls - thread local storage space to be destroyed

  Output Parameter:
. err - location to store the error code; pointer may be NULL; error is zero 
        for success, non-zero if a failure occurred
  
  Notes:
  The destroy function associated with the thread local storage will not 
  called after the space has been destroyed.
@*/
static inline void MPID_Thread_tls_destroy(MPID_Thread_tls_t * tls, int * err)
{
    MPIU_Thread_tls_destroy(tls,err);
}
/*@
  MPID_Thread_tls_set - associate a value with the current thread in the 
  thread local storage space
  
  Input Parameters:
+ tls - thread local storage space
- value - value to associate with current thread
@*/
static inline void MPID_Thread_tls_set(MPID_Thread_tls_t * tls, void * value)
{
    int err;
    MPIU_Thread_tls_set(tls,value,&err);
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                        ("tls_set failed, err=%d (%s)",err,MPIU_Strerror(err)));
}
/*@
  MPID_Thread_tls_get - obtain the value associated with the current thread 
  from the thread local storage space
  
  Input Parameter:
. tls - thread local storage space

  Output Parameter:
. value - value associated with current thread
@*/
static inline void MPID_Thread_tls_get(MPID_Thread_tls_t * tls, void ** value)
{
    int err;
    MPIU_Thread_tls_get(tls,value,&err);
    /* can't strerror here, possible endless recursion in strerror */
    MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,("tls_get failed, err=%d",err));
}
#endif /* !defined(MPID_THREAD_H_INCLUDED) */
