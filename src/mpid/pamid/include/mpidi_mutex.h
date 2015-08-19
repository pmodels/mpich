/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file include/mpidi_mutex.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef __include_mpidi_mutex_h__
#define __include_mpidi_mutex_h__

#include <opa_primitives.h>
#include <mpiutil.h>
#include <malloc.h>

#define MPIDI_THREAD_ID() Kernel_ProcessorID()

#if MPIDI_MUTEX_L2_ATOMIC



#define MUTEX_FAIL 0x8000000000000000UL

#include <spi/include/kernel/location.h>
#include <spi/include/kernel/memory.h>
#include <spi/include/l2/atomic.h>


#define  MPIDI_MAX_MUTEXES 16
typedef struct
{
  uint64_t     counter;
  uint64_t     bound;
} MPIDI_Mutex_t;

extern  MPIDI_Mutex_t *   MPIDI_Mutex_vector;
extern  uint32_t          MPIDI_Mutex_counter[MPIDI_MAX_THREADS][MPIDI_MAX_MUTEXES];
int MPIDI_Mutex_initialize();


/**
 *  \brief Try to acquire a mutex identified by an index.
 *  \param[in] m Index of the mutex
 *  \return 0    Lock successfully acquired
 *  \return 1    Lock was not acquired
 */
static inline int
MPIDI_Mutex_try_acquire(unsigned m)
{

#if MPIDI_MUTEX_RECURSIVE
  size_t tid = MPIDI_THREAD_ID();
  MPID_assert(m < MPIDI_MAX_MUTEXES);
  if (MPIDI_Mutex_counter[tid][m] >= 1) {
    ++MPIDI_Mutex_counter[tid][m];
    return 0;
  }
#endif

  MPIDI_Mutex_t *mutex  = &(MPIDI_Mutex_vector[m]);
  size_t rc = L2_AtomicLoadIncrementBounded(&mutex->counter);

  if (rc == MUTEX_FAIL)
    return 1;

#if MPIDI_MUTEX_RECURSIVE
  MPIDI_Mutex_counter[tid][m] =  1;
#endif
  return 0;   /* Lock succeeded */
}


/**
 *  \brief Acquire a mutex identified by an index.
 *  \param[in] m Index of the mutex
 *  \return 0    Lock successfully acquired
 *  \return 1    Fail
 */
static inline int
MPIDI_Mutex_acquire(unsigned m)
{
#if MPIDI_MUTEX_RECURSIVE
  size_t tid = MPIDI_THREAD_ID();
  MPID_assert(m < MPIDI_MAX_MUTEXES);

  if (unlikely(MPIDI_Mutex_counter[tid][m] >= 1)) {
    ++MPIDI_Mutex_counter[tid][m];
    return 0;
  }
#endif

  MPIDI_Mutex_t *mutex  = &(MPIDI_Mutex_vector[m]);
  size_t rc = 0;
  do {
    rc = L2_AtomicLoadIncrementBounded(&mutex->counter);
  } while (rc == MUTEX_FAIL);

#if MPIDI_MUTEX_RECURSIVE
  MPIDI_Mutex_counter[tid][m] =  1;
#endif
  return 0;
}


/**
 *  \brief Release a mutex identified by an index.
 *  \param[in] m Index of the mutex
 *  \return 0    Lock successfully released
 *  \return 1    Fail
 */
static inline int
MPIDI_Mutex_release(unsigned m)
{
#if MPIDI_MUTEX_RECURSIVE
  size_t tid = MPIDI_THREAD_ID();
  MPID_assert(m < MPIDI_MAX_MUTEXES);
  /* Verify this thread is the owner of this lock */
  MPID_assert(MPIDI_Mutex_counter[tid][m] > 0);

  --MPIDI_Mutex_counter[tid][m];
  MPID_assert(MPIDI_Mutex_counter[tid][m] >= 0);
  if (unlikely(MPIDI_Mutex_counter[tid][m] > 0))
    return 0;    /* Future calls will release the lock to other threads */
#endif

  /* Wait till all the writes in the critical sections from this
     thread have completed and invalidates have been delivered */
  //OPA_read_write_barrier();

  /* Release the lock */
  L2_AtomicStore(&(MPIDI_Mutex_vector[m].counter), 0);

  return 0;
}


#define MPIDI_Mutex_sync() OPA_read_write_barrier()



#elif MPIDI_MUTEX_LLSC



#include <spi/include/kernel/location.h>

#define  MPIDI_MAX_MUTEXES 16
typedef OPA_int_t MPIDI_Mutex_t;
extern  MPIDI_Mutex_t MPIDI_Mutex_vector [MPIDI_MAX_MUTEXES];
extern  uint32_t      MPIDI_Mutex_counter[MPIDI_MAX_THREADS][MPIDI_MAX_MUTEXES];

/**
 *  \brief Initialize a mutex.
 *
 *  In this API, mutexes are acessed via indices from
 *  0..MPIDI_MAX_MUTEXES. The mutexes are recursive
 */
static inline int
MPIDI_Mutex_initialize()
{
  size_t i, j;
  for (i=0; i<MPIDI_MAX_MUTEXES; ++i) {
    OPA_store_int(&(MPIDI_Mutex_vector[i]), 0);
  }

  for (i=0; i<MPIDI_MAX_MUTEXES; ++i) {
    for (j=0; j<MPIDI_MAX_THREADS; ++j) {
      MPIDI_Mutex_counter[j][i] = 0;
    }
  }

  return 0;
}


/**
 *  \brief Try to acquire a mutex identified by an index.
 *  \param[in] m Index of the mutex
 *  \return 0    Lock successfully acquired
 *  \return 1    Lock was not acquired
 */
static inline int
MPIDI_Mutex_try_acquire(unsigned m)
{
  register int old_val;
  size_t tid = MPIDI_THREAD_ID();

  MPID_assert(m < MPIDI_MAX_MUTEXES);

  if (MPIDI_Mutex_counter[tid][m] >= 1) {
    ++MPIDI_Mutex_counter[tid][m];
    return 0;
  }

  MPIDI_Mutex_t *mutex  = &(MPIDI_Mutex_vector[m]);
  old_val = OPA_LL_int(mutex);
  if (old_val != 0)
    return 1;  /* Lock failed */

  int rc = OPA_SC_int(mutex, 1);  /* returns 0 when SC fails */

  if (rc == 0)
    return 1; /* Lock failed */

  MPIDI_Mutex_counter[tid][m] =  1;
  return 0;   /* Lock succeeded */
}


/**
 *  \brief Acquire a mutex identified by an index.
 *  \param[in] m Index of the mutex
 *  \return 0    Lock successfully acquired
 *  \return 1    Fail
 */
static inline int
MPIDI_Mutex_acquire(unsigned m)
{
  register int old_val;
  size_t tid = MPIDI_THREAD_ID();

  MPID_assert(m < MPIDI_MAX_MUTEXES);

  if (unlikely(MPIDI_Mutex_counter[tid][m] >= 1)) {
    ++MPIDI_Mutex_counter[tid][m];
    return 0;
  }

  MPIDI_Mutex_t *mutex  = &(MPIDI_Mutex_vector[m]);
  do {
    do {
      old_val = OPA_LL_int(mutex);
    } while (old_val != 0);

  } while (!OPA_SC_int(mutex, 1));

  MPIDI_Mutex_counter[tid][m] =  1;
  return 0;
}


/**
 *  \brief Release a mutex identified by an index.
 *  \param[in] m Index of the mutex
 *  \return 0    Lock successfully released
 *  \return 1    Fail
 */
static inline int
MPIDI_Mutex_release(unsigned m)
{
  size_t tid = MPIDI_THREAD_ID();
  MPID_assert(m < MPIDI_MAX_MUTEXES);
  /* Verify this thread is the owner of this lock */
  MPID_assert(MPIDI_Mutex_counter[tid][m] > 0);

  --MPIDI_Mutex_counter[tid][m];
  MPID_assert(MPIDI_Mutex_counter[tid][m] >= 0);
  if (unlikely(MPIDI_Mutex_counter[tid][m] > 0))
    return 0;    /* Future calls will release the lock to other threads */

  /* Wait till all the writes in the critical sections from this
     thread have completed and invalidates have been delivered */
  //OPA_read_write_barrier();

  /* Release the lock */
  OPA_store_int(&(MPIDI_Mutex_vector[m]), 0);

  return 0;
}


#define MPIDI_Mutex_sync() OPA_read_write_barrier()



#else



extern pthread_mutex_t MPIDI_Mutex_lock;

static inline int
MPIDI_Mutex_initialize()
{
  int rc;

  pthread_mutexattr_t attr;
  rc = pthread_mutexattr_init(&attr);
  MPID_assert(rc == 0);
#if !defined(__AIX__)
  extern int pthread_mutexattr_settype(pthread_mutexattr_t *__attr, int __kind) __THROW __nonnull ((1));
#else
  extern int pthread_mutexattr_settype(pthread_mutexattr_t *__attr, int __kind);
#endif
#ifndef __PE__
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#else /*(MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)*/
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif /*(MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)*/
#else /* __PE__ */
#if !defined(__AIX__)
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#else
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#endif
#endif
  MPID_assert(rc == 0);

  rc = pthread_mutex_init(&MPIDI_Mutex_lock, &attr);
  MPID_assert(rc == 0);

  return 0;
}


static inline int
MPIDI_Mutex_try_acquire(unsigned m)
{
  int rc;
  rc = pthread_mutex_trylock(&MPIDI_Mutex_lock);
  MPID_assert( (rc == 0) || (rc == EBUSY) );
  /* fprintf(stderr, "%s:%u (rc=%d)\n", __FUNCTION__, __LINE__, rc); */
  return rc;
}


static inline int
MPIDI_Mutex_acquire(unsigned m)
{
  int rc;
  /* fprintf(stderr, "%s:%u\n", __FUNCTION__, __LINE__); */
  rc = pthread_mutex_lock(&MPIDI_Mutex_lock);
  /* fprintf(stderr, "%s:%u (rc=%d)\n", __FUNCTION__, __LINE__, rc); */
  MPID_assert(rc == 0);
  return rc;
}


static inline int
MPIDI_Mutex_release(unsigned m)
{
  int rc;
  rc = pthread_mutex_unlock(&MPIDI_Mutex_lock);
  /* fprintf(stderr, "%s:%u (rc=%d)\n", __FUNCTION__, __LINE__, rc); */
  MPID_assert(rc == 0);
  return rc;
}


#define MPIDI_Mutex_sync()


#endif


#endif
