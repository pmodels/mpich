/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Threads
 */
#include <limits.h>
#include "mpiu_process_wrappers.h" /* for MPIU_PW_Sched_yield */
#include "mpidbg.h"
#include "opa_primitives.h"

/* 
   One of PTHREAD_MUTEX_RECURSIVE_NP and PTHREAD_MUTEX_RECURSIVE seem to be 
   present in different versions.  For example, Mac OS X 10.4 had 
   PTHREAD_MUTEX_RECURSIVE_NP but Mac OS X 10.5 does not; instead it has
   PTHREAD_MUTEX_RECURSIVE 
*/
#if defined(HAVE_PTHREAD_MUTEX_RECURSIVE_NP)
#define PTHREAD_MUTEX_RECURSIVE_VALUE PTHREAD_MUTEX_RECURSIVE_NP
#elif defined(HAVE_PTHREAD_MUTEX_RECURSIVE)
#define PTHREAD_MUTEX_RECURSIVE_VALUE PTHREAD_MUTEX_RECURSIVE
#else
#error 'Unable to determine pthrad mutex recursive value'
#endif /* pthread mutex recursive value */

#if defined(NEEDS_PTHREAD_MUTEXATTR_SETTYPE_DECL)
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind);
#endif /* NEEDS_PTHREAD_MUTEXATTR_SETTYPE_DECL */

/* MPIU_Thread_create() defined in mpiu_thread.c */

#define MPIU_Thread_exit()			\
do {                                               \
    pthread_exit(NULL);				\
} while (0)

#define MPIU_Thread_self(id_)			\
do {                                               \
    *(id_) = pthread_self();			\
} while (0)

#define MPIU_Thread_same(id1_, id2_, same_)			\
do {                                                               \
    *(same_) = pthread_equal(*(id1_), *(id2_)) ? TRUE : FALSE;	\
} while (0)

#define MPIU_Thread_yield()						\
do {                                                                       \
    MPIU_DBG_MSG(THREAD,VERBOSE,"enter MPIU_Thread_yield");    \
    MPIU_PW_Sched_yield();                                     \
    MPIU_DBG_MSG(THREAD,VERBOSE,"exit MPIU_Thread_yield");     \
} while (0)

/*----------------------*/
/* Ticket Lock Routines */
/*----------------------*/

static inline int ticket_lock_init(ticket_lock_t *lock)
{
    OPA_store_int(&lock->next_ticket, 0);
    OPA_store_int(&lock->now_serving, 0);
    return 0;
}

/* Atomically increment the nex_ticket counter and get my ticket.
   Then spin on now_serving until it equals my ticket.
   */
static inline int ticket_acquire_lock(ticket_lock_t* lock)
{
    int my_ticket = OPA_fetch_and_add_int(&lock->next_ticket, 1);
    while(OPA_load_int(&lock->now_serving) != my_ticket)
            ;
    return 0;
}

/* Release the lock
   */
static inline int ticket_release_lock(ticket_lock_t* lock)
{
    /* Avoid compiler reordering before releasing the lock*/
    OPA_compiler_barrier();
    OPA_incr_int(&lock->now_serving);
    return 0;
}

/*------------------------*/
/* Priority Lock Routines */
/*------------------------*/

static inline int priority_lock_init(priority_lock_t *lock)
{
    OPA_store_int(&lock->next_ticket_H, 0);
    OPA_store_int(&lock->now_serving_H, 0);
    OPA_store_int(&lock->next_ticket_L, 0);
    OPA_store_int(&lock->now_serving_L, 0);
    OPA_store_int(&lock->next_ticket_B, 0);
    OPA_store_int(&lock->now_serving_B, 0);
    lock->already_blocked = 0;
    return 0;
}

/* First wait my turn in this priority level, Then if I am the first
   one to block the LPRs, wait for the last LPR to terminate.
   */
static inline int priority_acquire_lock(priority_lock_t* lock)
{
    int my_ticket = OPA_fetch_and_add_int(&lock->next_ticket_H, 1);
    while(OPA_load_int(&lock->now_serving_H) != my_ticket)
            ;
    int B_ticket;
    if(!lock->already_blocked)
    {
      B_ticket = OPA_fetch_and_add_int(&lock->next_ticket_B, 1);
      while(OPA_load_int(&lock->now_serving_B) != B_ticket)
            ;
      lock->already_blocked = 1;
    }
    lock->last_acquisition_priority = HIGH_PRIORITY;
    return 0;
}
static inline int priority_acquire_lock_low(priority_lock_t* lock)
{
    int my_ticket = OPA_fetch_and_add_int(&lock->next_ticket_L, 1);
    while(OPA_load_int(&lock->now_serving_L) != my_ticket)
           ;
    int  B_ticket = OPA_fetch_and_add_int(&lock->next_ticket_B, 1);
    while(OPA_load_int(&lock->now_serving_B) != B_ticket)
           ;
    lock->last_acquisition_priority = LOW_PRIORITY;
    return 0;
}

/* Release the lock
   */
static inline int priority_release_lock(priority_lock_t* lock)
{
    int err=0;
    if(lock->last_acquisition_priority==HIGH_PRIORITY)
         err = priority_release_lock_high(lock);
    else
         err = priority_release_lock_low(lock);
    return err;
}

static inline int priority_release_lock_high(priority_lock_t* lock)
{
    /* Avoid compiler reordering before releasing the lock*/
    OPA_compiler_barrier();
    /* Only me in the HPRs queue -> let the LPRs pass */
    if(OPA_load_int(&lock->now_serving_H) == OPA_load_int(&lock->next_ticket_H) - 1)
    {
       lock->already_blocked = 0;
       OPA_incr_int(&lock->now_serving_B);
    }
    OPA_incr_int(&lock->now_serving_H);
    return 0;
}

static inline int priority_release_lock_low(priority_lock_t* lock)
{
    /* Avoid compiler reordering before releasing the lock*/
    OPA_compiler_barrier();
    OPA_incr_int(&lock->now_serving_B);
    OPA_incr_int(&lock->now_serving_L);
    return 0;
}

/*
 *    MPIU Mutexes: encapsulate lower level locks like pthread mutexes
 *    and ticket locks
 */

#if !defined(MPICH_DEBUG_MUTEX) || !defined(PTHREAD_MUTEX_ERRORCHECK_VALUE)
static inline void MPIU_Thread_mutex_create(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;
    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_init(&mutex_ptr_->pthread_lock, NULL);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_lock_init(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_lock_init(&mutex_ptr_->priority_lock);
         break;
      }
    }
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_mutex %p", (mutex_ptr_));
}
#else /* MPICH_DEBUG_MUTEX */
static inline void MPIU_Thread_mutex_create(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;
    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         pthread_mutexattr_t attr__;
         /* FIXME this used to be PTHREAD_MUTEX_ERRORCHECK_NP, but we had to change
          it for the thread granularity work when we needed recursive mutexes.  We
          should go through this code and see if there's any good way to implement
          error checked versions with the recursive mutexes. */
         pthread_mutexattr_init(&attr__);
         pthread_mutexattr_settype(&attr__, PTHREAD_MUTEX_ERRORCHECK_VALUE);
         err__ = pthread_mutex_init(&mutex_ptr_->pthread_lock, &attr__);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_lock_init(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_lock_init(&mutex_ptr_->priority_lock);
         break;
      }
    }
    if (err__)
        MPIU_Internal_sys_error_printf("pthread_mutex_init", err__,     \
                                       "    %s:%d\n", __FILE__, __LINE__);
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_mutex %p", (mutex_ptr_));
}
#endif

static inline void MPIU_Thread_mutex_destroy(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;

    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to destroy MPIU_Thread_mutex %p", (mutex_ptr_));

    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_destroy(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         /* FIXME Should we do something here ?*/
         err__ = 0;
         break;
      }
      case MPIU_PRIORITY:
      {
         /* FIXME Should we do something here ?*/
         err__ = 0;
         break;
      }
    }
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
}

#ifndef MPICH_DEBUG_MUTEX
static inline void MPIU_Thread_mutex_lock(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_));

    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_lock(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_acquire_lock(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_acquire_lock(&mutex_ptr_->priority_lock);
         break;
      }
    }

    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_));
}
#else /* MPICH_DEBUG_MUTEX */
static inline void MPIU_Thread_mutex_lock(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_));
    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_lock(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_acquire_lock(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_acquire_lock(&mutex_ptr_->priority_lock);
         break;
      }
    }

    if (err__)
    {
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex lock error: %s", MPIU_Strerror(err__));
        MPIU_Internal_sys_error_printf("pthread_mutex_lock", err__,\
                                       "    %s:%d\n", __FILE__, __LINE__);
    }
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_));
}
#endif

#ifndef MPICH_DEBUG_MUTEX
static inline void MPIU_Thread_mutex_lock_low(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_));

    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_lock(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_acquire_lock(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_acquire_lock_low(&mutex_ptr_->priority_lock);
         break;
      }
    }

    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_));
}
#else /* MPICH_DEBUG_MUTEX */
static inline void MPIU_Thread_mutex_lock_low(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"enter MPIU_Thread_mutex_lock %p", (mutex_ptr_));
    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_lock(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_acquire_lock(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_acquire_lock_low(&mutex_ptr_->priority_lock);
         break;
      }
    }

    if (err__)
    {
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex lock error: %s", MPIU_Strerror(err__));
        MPIU_Internal_sys_error_printf("pthread_mutex_lock", err__,\
                                       "    %s:%d\n", __FILE__, __LINE__);
    }
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
    MPIU_DBG_MSG_P(THREAD,VERBOSE,"exit MPIU_Thread_mutex_lock %p", (mutex_ptr_));
}
#endif

#ifndef MPICH_DEBUG_MUTEX
static inline void MPIU_Thread_mutex_unlock(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;

    MPIU_DBG_MSG_P(THREAD,TYPICAL,"MPIU_Thread_mutex_unlock %p", (mutex_ptr_));

    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_unlock(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_release_lock(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_release_lock(&mutex_ptr_->priority_lock);
         break;
      }
    }

    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
}
#else /* MPICH_DEBUG_MUTEX */
static inline void MPIU_Thread_mutex_unlock(MPIU_Thread_mutex_t* mutex_ptr_, int* err_ptr_)
{
    int err__=0;

    MPIU_DBG_MSG_P(THREAD,VERBOSE,"MPIU_Thread_mutex_unlock %p", (mutex_ptr_));

    switch(MPIU_lock_type){
      case MPIU_MUTEX:
      {
         err__ = pthread_mutex_unlock(&mutex_ptr_->pthread_lock);
         break;
      }
      case MPIU_TICKET:
      {
         err__ = ticket_release_lock(&mutex_ptr_->ticket_lock);
         break;
      }
      case MPIU_PRIORITY:
      {
         err__ = priority_release_lock(&mutex_ptr_->priority_lock);
         break;
      }
    }
    if (err__)
    {
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex unlock error: %s", MPIU_Strerror(err__));
        MPIU_Internal_sys_error_printf("pthread_mutex_unlock", err__,\
                                       "    %s:%d\n", __FILE__, __LINE__);
    }
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
    *(int *)(err_ptr_) = err__;
}
#endif

#ifndef MPICH_DEBUG_MUTEX
static inline void MPIU_Thread_mutex_trylock(MPIU_Thread_mutex_t* mutex_ptr_, int* flag_ptr_, int* err_ptr_)
{
    int err__=0;

    if(MPIU_lock_type == MPIU_MUTEX)
      err__ = pthread_mutex_trylock(&mutex_ptr_->pthread_lock);
    else
      /*No trylock routine for ticket-based locks*/
      err__ = 1;

    *(flag_ptr_) = (err__ == 0) ? TRUE : FALSE;
    MPIU_DBG_MSG_FMT(THREAD,VERBOSE,(MPIU_DBG_FDEST, "MPIU_Thread_mutex_trylock mutex=%p result=%s", (mutex_ptr_), (*(flag_ptr_) ? "success" : "failure")));
    *(int *)(err_ptr_) = (err__ == EBUSY) ? MPIU_THREAD_SUCCESS : err__;
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
}
#else /* MPICH_DEBUG_MUTEX */
static inline void MPIU_Thread_mutex_trylock(MPIU_Thread_mutex_t* mutex_ptr_, int* flag_ptr_, int* err_ptr_)
{
    int err__=0;

    if(MPIU_lock_type == MPIU_MUTEX)
      err__ = pthread_mutex_trylock(&mutex_ptr_->pthread_lock);
    else
      /*No trylock routine for ticket-based locks*/
      err__ = 1;

    if (err__ && err__ != EBUSY)
    {
        MPIU_DBG_MSG_S(THREAD,TERSE,"  mutex trylock error: %s", MPIU_Strerror(err__));
        MPIU_Internal_sys_error_printf("pthread_mutex_trylock", err__,\
                                       "    %s:%d\n", __FILE__, __LINE__);
    }
    *(flag_ptr_) = (err__ == 0) ? TRUE : FALSE;
    MPIU_DBG_MSG_FMT(THREAD,VERBOSE,(MPIU_DBG_FDEST, "MPIU_Thread_mutex_trylock mutex=%p result=%s", (mutex_ptr_), (*(flag_ptr_) ? "success" : "failure")));
    *(int *)(err_ptr_) = (err__ == EBUSY) ? MPIU_THREAD_SUCCESS : err__;
    /* FIXME: convert error to an MPIU_THREAD_ERR value */
}
#endif

/*
 * Condition Variables
 */

#define MPIU_Thread_cond_create(cond_ptr_, err_ptr_)                    \
  do {                                                                  \
      int err__;							\
                                                                        \
      err__ = pthread_cond_init((cond_ptr_), NULL);                     \
      MPIU_DBG_MSG_P(THREAD,TYPICAL,"Created MPIU_Thread_cond %p", (cond_ptr_)); \
      /* FIXME: convert error to an MPIU_THREAD_ERR value */            \
      *(int *)(err_ptr_) = err__;                                       \
  } while (0)

#define MPIU_Thread_cond_destroy(cond_ptr_, err_ptr_)                   \
  do {                                                                  \
      int err__;							\
                                                                        \
      MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to destroy MPIU_Thread_cond %p", (cond_ptr_)); \
      err__ = pthread_cond_destroy(cond_ptr_);                          \
      /* FIXME: convert error to an MPIU_THREAD_ERR value */            \
      *(int *)(err_ptr_) = err__;                                       \
  } while (0)

#define MPIU_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)		\
do {                                                                       \
    int err__;								\
    									\
    /* The latest pthread specification says that cond_wait routines    \
       aren't allowed to return EINTR,	                                \
       but some of the older implementations still do. */		\
    MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"Enter cond_wait on cond=%p mutex=%p",(cond_ptr_),(mutex_ptr_))) \
    do									\
    {									\
	err__ = pthread_cond_wait((cond_ptr_), (mutex_ptr_));		\
    }									\
    while (err__ == EINTR);						\
									\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */		\
    *(int *)(err_ptr_) = err__;                                         \
    if (err__) {                                                        \
        MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"error in cond_wait on cond=%p mutex=%p err__=%d",(cond_ptr_),(mutex_ptr_), err__)) \
            }                                                           \
    MPIU_DBG_MSG_FMT(THREAD,TYPICAL,(MPIU_DBG_FDEST,"Exit cond_wait on cond=%p mutex=%p",(cond_ptr_),(mutex_ptr_))) \
} while (0)

#define MPIU_Thread_cond_broadcast(cond_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
    								\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to cond_broadcast on MPIU_Thread_cond %p", (cond_ptr_));    \
    err__ = pthread_cond_broadcast(cond_ptr_);			\
								\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
    *(int *)(err_ptr_) = err__;                                 \
} while (0)

#define MPIU_Thread_cond_signal(cond_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
								\
    MPIU_DBG_MSG_P(THREAD,TYPICAL,"About to cond_signal on MPIU_Thread_cond %p", (cond_ptr_));    \
    err__ = pthread_cond_signal(cond_ptr_);			\
								\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
    *(int *)(err_ptr_) = err__;                                 \
} while (0)


/*
 * Thread Local Storage
 */

#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
do {                                                                       \
    int err__;								\
    									\
    err__ = pthread_key_create((tls_ptr_), (exit_func_ptr_));		\
									\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */		\
    *(int *)(err_ptr_) = err__;                                         \
} while (0)

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)		\
do {                                                               \
    int err__;							\
    								\
    err__ = pthread_key_delete(*(tls_ptr_));			\
								\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
    *(int *)(err_ptr_) = err__;                                 \
} while (0)

#define MPIU_Thread_tls_set(tls_ptr_, value_, err_ptr_)		\
do {                                                               \
    int err__;							\
								\
    err__ = pthread_setspecific(*(tls_ptr_), (value_));		\
								\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
    *(int *)(err_ptr_) = err__;                                 \
} while (0)

#define MPIU_Thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)	\
do {                                                               \
    *(value_ptr_) = pthread_getspecific(*(tls_ptr_));		\
								\
    /* FIXME: convert error to an MPIU_THREAD_ERR value */	\
    *(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;                   \
} while (0)
