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
 * \file src/mpid_progress.h
 * \brief ???
 */

#ifndef __src_mpid_progress_h__
#define __src_mpid_progress_h__


/**
 * \defgroup MPID_PROGRESS MPID Progress engine
 *
 * Maintain the state and rules of the MPI progress semantics.
 *
 * The behavior of asynchronous progress depends on many configurable variables,
 * and the requirements and/or effects of several combinations may not be
 * obvious. There are certain truths and simplifying assertions that, once
 * identified, can eliminate many configurations from consideration which
 * greatly reduces code complexity.
 *
 * 1. Asynchronous progress with a NULL progress function requires context post.
 *
 *    When a NULL progress function is specified for async progress, in some
 *    implementations, the context lock will be internally acquired and  held
 *    until async progress is disabled or suspended. As such, any application
 *    thread that waits to acquire a context lock will block indefinitely. This
 *    is considered an invalid configuration.
 *
 * 2. Asynchronous progress with a NULL progress function requires "per object"
 *    mpich locks.
 *
 *    When compiled using the "global lock" mpich thread mode the individual
 *    "per object" lock macros are defined as noops, yet all mpich callbacks
 *    that could be invoked from within a context advance only use the
 *    "per object" lock macros and *not* the "global" lock macros. This is
 *    because the async progress thread never actually leaves the API, but
 *    remains within the library. As designed, the mpich "global" locks are only
 *    acquired and released at the very top API entry and exit points.
 *
 *    If noop "per object" lock macros are ever used the async progress threads
 *    will cause data corruption of mpich data structures.
 *
 * 3. Asynchronous progress with a non-NULL progress function using the "global"
 *    mpich locks requires 'MPI_THREAD_MULTIPLE'.
 *
 *    The "global" mpich lock macros, as defined, include a check of the thread
 *    mode and only performs the mutex operations if the thread mode is
 *    'MPI_THREAD_MULTIPLE'. When using a non-NULL progress function for async
 *    progress the thread that invokes the progress function is essentially
 *    'outside' of the mpich library. Consequently, similar to the other API
 *    entry points, the "global" mpich lock must be acquired in the progress
 *    function before any mpich data structures may be accessed.
 *
 *    If acquiring the "global" lock is attempted in any thread mode other
 *    than 'MPI_THREAD_MULTIPLE' the async progress threads will cause data
 *    corruption of mpich data structures. To avoid this data corruption
 *    problem the mpi thread mode is promoted to 'MPI_THREAD_MULTIPLE' when
 *    async progress with a non-NULL progress function is enabled in the
 *    "global" mpich lock mode.
 *
 * 4. Only a single context is supported in the "global" mpich lock mode.
 *
 *    A multi-context application will always perform better with the
 *    "per object" mpich lock mode - regardless of whether async progress is
 *    enabled or not. This is because in the "global" mpich lock mode all
 *    threads, application and async progress, must acquire the single global
 *    lock which effectively serializes the threads and negates any benefit
 *    of multiple contexts.
 *
 *    Asserting this single context limitation removes code, improves
 *    performance, and greatly simplifies logic.
 *
 * 5. The "global" mpich lock mode does not support context post.
 *
 *    As the "global" mpich lock mode only supports a single context, and all
 *    threads, application and async progress, must first acquire the global
 *    lock before accessing the mpich data structures or the single context,
 *    it is detrimental to performance to invoke context post instead of
 *    directly operating on the context.
 *
 *    Asserting this context post limitation further removes code, improves
 *    performance, and simplifies logic.
 *
 * 6. The "global" mpich lock mode does not require context lock.
 *
 *    Access to all internal mpich data structures, including the single
 *    context, for all application and async progress threads, is protected by
 *    the "global" mpich lock upon entry to the API. This makes the context
 *    lock redundant.
 *
 * 7. The "per object" mpich lock mode assumes active asynchronous progress and
 *    mpi thread mode 'MPI_THREAD_MULTIPLE'.
 *
 *    Strictly defined, the context lock is NOT required in the "per object"
 *    mpich lock mode if asyncronous progress is not active and the mpi thread
 *    level is not 'MPI_THREAD_MULTIPLE'. This means that the run environment
 *    is completely single threaded.
 *
 *    An application running completely single threaded using the "per object"
 *    mpich lock mode will always perform worse than the same completely single
 *    threaded application using the "global" mpich lock mode. This is because
 *    a "per object" single thread will take more locks than a "global" single
 *    thread.
 *
 *    Therefore, a simplifying assertion is made that, as the "per object"
 *    completely single threaded configuration is always worse than the "global"
 *    completely single threaded configuration, the "per object" mpich lock mode
 *    will assume the application is running a multi-threaded configuration and
 *    optimize the code logic accordingly.
 *
 * \addtogroup MPID_PROGRESS
 * \{
 */

typedef enum
{
  ASYNC_PROGRESS_MODE_DISABLED = 0, /**< async progress is disabled                  */
  ASYNC_PROGRESS_MODE_LOCKED,       /**< async progress uses a \c NULL progress_fn   */
  ASYNC_PROGRESS_MODE_TRIGGER,      /**< async progress uses a 'trigger' progress_fn */
  ASYNC_PROGRESS_MODE_COUNT         /**< number of sync progress modes               */
} async_progress_mode_t;


/** \brief The same as MPID_Progress_wait(), since it does not block */
#define MPID_Progress_test() MPID_Progress_wait_inline(1)
/** \brief The same as MPID_Progress_wait(), since it does not block */
#define MPID_Progress_poke() MPID_Progress_wait_inline(1)


/**
 * \brief A macro to easily implement advancing until a specific
 * condition becomes false.
 *
 * \param[in] COND This is not a true parameter.  It is *specifically*
 * designed to be evaluated several times, allowing for the result to
 * change.  The condition would generally look something like
 * "(cb.client == 0)".  This would be used as the condition on a while
 * loop.
 *
 * \returns MPI_SUCCESS
 *
 * This correctly checks the condition before attempting to loop,
 * since the call to MPID_Progress_wait() may not return if the event
 * is already complete.  Any system *not* using this macro *must* use
 * a similar check before waiting.
 */
#define MPID_PROGRESS_WAIT_WHILE(COND)          \
({                                              \
  while (COND)                                  \
    MPID_Progress_wait(&__state);               \
  MPI_SUCCESS;                                  \
})


/**
 * \brief A macro to easily implement advancing until a specific
 * condition becomes false.
 *
 * \param[in] COND This is not a true parameter.  It is *specifically*
 * designed to be evaluated several times, allowing for the result to
 * change.  The condition would generally look something like
 * "(cb.client == 0)".  This would be used as the condition on a while
 * loop.
 *
 * \returns MPI_SUCCESS
 *
 * This macro makes one pami advance regardless of the state of the COND.
 */
#define MPID_PROGRESS_WAIT_DO_WHILE(COND)       \
({                                              \
  do {                                          \
    MPID_Progress_wait(&__state);               \
  } while(COND);                                \
  MPI_SUCCESS;                                  \
})


/**
 * \brief Unused, provided since MPI calls it.
 * \param[in] state The previously seen state of advance
 */
#define MPID_Progress_start(state)

/**
 * \brief Unused, provided since MPI calls it.
 * \param[in] state The previously seen state of advance
 */
#define MPID_Progress_end(state)

/**
 * \brief Signal MPID_Progress_wait() that something is done/changed
 *
 * It is therefore important that the ADI layer include a call to
 * MPIDI_Progress_signal() whenever something occurs that a node might
 * be waiting on.
 */
#define MPIDI_Progress_signal()


#define MPID_Progress_wait(state) MPID_Progress_wait_inline(100)


void MPIDI_Progress_init();
void MPIDI_Progress_async_start(pami_context_t context, void *cookie);
void MPIDI_Progress_async_end  (pami_context_t context, void *cookie);
void MPIDI_Progress_async_poll ();
void MPIDI_Progress_async_poll_perobj ();

/**
 * \brief This function blocks until a request completes
 * \param[in] state The previously seen state of advance
 *
 * It does not check what has completed, only that the counter
 * changes.  That happens whenever there is a call to
 * MPIDI_Progress_signal().  It is therefore important that the ADI
 * layer include a call to MPIDI_Progress_signal() whenever something
 * occurs that a node might be waiting on.
 *
 */
static inline int
MPID_Progress_wait_inline(unsigned loop_count)
{
  pami_result_t rc = 0;

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
  /*
   * In the "per object" thread mode the "global" lock is defined as a noop and
   * therefore no locks are held at this point.
   */
  if (unlikely(MPIDI_Process.async_progress.active == 0))
    {
      /*
       * Asynchronous progress is currently inactive; therefore this application
       * thread must drive progress.
       *
       * It is not actually neccesary to lock the context when async progress is
       * disabled and only a single context is being used and the mpi thread
       * mode is not mpi thread multiple.  This is unlikely to be true in the
       * mpich "per obj" thread granularity compile. As an optimization, do not
       * check for this condition and simply always perform the context lock.
       */
      rc = PAMI_Context_trylock_advancev(MPIDI_Context, MPIDI_Process.avail_contexts, 1);
      MPID_assert( (rc == PAMI_SUCCESS) || (rc == PAMI_EAGAIN) );
    }
#else
  /*
   * In the "global lock" thread mode the single mpich lock has already been
   * acquired at this point. Any other application thread or asynchronous
   * progress execution resource must also acquire this global lock and will
   * block until the global lock is cycled or released.
   *
   * Because only one thread will enter this code at any time, including any
   * async progress threads if async progress is enabled, it is unneccesary
   * to acquire any context locks before the context advance operation.
   *
   * NOTE: The 'NULL' progress function configuation for async progress is not
   *       valid in the 'global' mpich lock mode. See discussion above for more
   *       information.
   *
   * NOTE: There is a simplifying assertion for the "global" mpich lock mode
   *       that only a single context is supported. See discussion in above for
   *       more information.
   */
  rc = PAMI_Context_advance(MPIDI_Context[0], 1);
  MPID_assert( (rc == PAMI_SUCCESS) || (rc == PAMI_EAGAIN) );
#ifdef __PE__
  if (rc == PAMI_EAGAIN) {
       MPIU_THREAD_CS_SCHED_YIELD(ALLFUNC,); /* sync, release(0), yield, acquire(0) */
  } else
#endif
  MPIU_THREAD_CS_YIELD(ALLFUNC,); /* sync, release(0), acquire(0) */
#endif

  return MPI_SUCCESS;
}

/** \} */


#endif
