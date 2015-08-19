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
 * \file include/mpidi_thread.h
 * \brief ???
 *
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_mutex.h"


#ifndef __include_mpidi_thread_h__
#define __include_mpidi_thread_h__


/**
 * ******************************************************************
 * \brief Mutexes for thread/interrupt safety
 * ******************************************************************
 */

/* This file is included by mpidpre.h, so it is included before mpiimplthread.h.
 * This is intentional because it lets us override the critical section macros */

#define MPID_DEVICE_DEFINES_THREAD_CS 1


#if (MPICH_THREAD_LEVEL != MPI_THREAD_MULTIPLE)
#error MPICH_THREAD_LEVEL should be MPI_THREAD_MULTIPLE
#endif

#define MPIU_THREAD_CS_INIT     ({ MPIDI_Mutex_initialize(); })
#define MPIU_THREAD_CS_FINALIZE

#define MPIU_THREADSAFE_INIT_DECL(_var) static volatile int _var=1
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var)  \
    MPIU_THREAD_CS_ENTER(INITFLAG,);            \
    if (_var)                                   \
      {
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var)    \
      }                                         \
    MPIU_THREAD_CS_EXIT(INITFLAG,)


#define MPIU_THREAD_CS_ENTER(name,_context) MPIU_THREAD_CS_##name##_ENTER(_context)
#define MPIU_THREAD_CS_EXIT(name,_context)  MPIU_THREAD_CS_##name##_EXIT (_context)
#define MPIU_THREAD_CS_YIELD(name,_context) MPIU_THREAD_CS_##name##_YIELD(_context)
#define MPIU_THREAD_CS_SCHED_YIELD(name,_context) MPIU_THREAD_CS_##name##_SCHED_YIELD(_context)
#define MPIU_THREAD_CS_TRY(name,_context)   MPIU_THREAD_CS_##name##_TRY(_context)

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL

#define MPIDI_CS_ENTER(m) ({ if (MPIR_ThreadInfo.isThreaded) {                                             MPIDI_Mutex_acquire(m); } })
#define MPIDI_CS_EXIT(m)  ({ if (MPIR_ThreadInfo.isThreaded) { MPIDI_Mutex_sync(); MPIDI_Mutex_release(m);                         } })
#define MPIDI_CS_YIELD(m) ({ if (MPIR_ThreadInfo.isThreaded) { MPIDI_Mutex_sync(); MPIDI_Mutex_release(m); MPIDI_Mutex_acquire(m); } })
#define MPIDI_CS_TRY(m)   ({ (0==MPIDI_Mutex_try_acquire(m)); })
#define MPIDI_CS_SCHED_YIELD(m) ({ if (MPIR_ThreadInfo.isThreaded) { MPIDI_Mutex_sync(); MPIDI_Mutex_release(m); sched_yield(); MPIDI_Mutex_acquire(m); } })

/* There is a single, global lock, held for the duration of an MPI call */
#define MPIU_THREAD_CS_ALLFUNC_ENTER(_context)      MPIDI_CS_ENTER(0)
#define MPIU_THREAD_CS_ALLFUNC_EXIT(_context)       MPIDI_CS_EXIT (0)
#define MPIU_THREAD_CS_ALLFUNC_YIELD(_context)      MPIDI_CS_YIELD(0)
#define MPIU_THREAD_CS_ALLFUNC_SCHED_YIELD(_context) MPIDI_CS_SCHED_YIELD(0)
#define MPIU_THREAD_CS_ALLFUNC_TRY(_context)        MPIDI_CS_TRY(0)
#define MPIU_THREAD_CS_INIT_ENTER(_context)         MPIDI_Mutex_acquire(0)
#define MPIU_THREAD_CS_INIT_EXIT(_context)          MPIDI_Mutex_release(0)

#define MPIU_THREAD_CS_CONTEXTID_ENTER(_context)
#define MPIU_THREAD_CS_CONTEXTID_EXIT(_context)
#define MPIU_THREAD_CS_CONTEXTID_YIELD(_context)    MPIDI_CS_YIELD(0)
#define MPIU_THREAD_CS_CONTEXTID_SCHED_YIELD(_context) MPIDI_CS_SCHED_YIELD(0)
#define MPIU_THREAD_CS_HANDLEALLOC_ENTER(_context)
#define MPIU_THREAD_CS_HANDLEALLOC_EXIT(_context)
#define MPIU_THREAD_CS_HANDLE_ENTER(_context)
#define MPIU_THREAD_CS_HANDLE_EXIT(_context)
#define MPIU_THREAD_CS_INITFLAG_ENTER(_context)
#define MPIU_THREAD_CS_INITFLAG_EXIT(_context)
#define MPIU_THREAD_CS_MEMALLOC_ENTER(_context)
#define MPIU_THREAD_CS_MEMALLOC_EXIT(_context)
#define MPIU_THREAD_CS_MPI_OBJ_ENTER(_context)      MPIDI_CS_ENTER(5)
#define MPIU_THREAD_CS_MPI_OBJ_EXIT(_context)       MPIDI_CS_EXIT(5)
#define MPIU_THREAD_CS_MSGQUEUE_ENTER(_context)
#define MPIU_THREAD_CS_MSGQUEUE_EXIT(_context)
#define MPIU_THREAD_CS_PAMI_ENTER(_context)
#define MPIU_THREAD_CS_PAMI_EXIT(_context)
#define MPIU_THREAD_CS_ASYNC_ENTER(_context)        MPIDI_CS_ENTER(8)
#define MPIU_THREAD_CS_ASYNC_EXIT(_context)         MPIDI_CS_EXIT (8)

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT

#define MPIDI_CS_ENTER(m)                               \
    do {                                                \
        if (likely(MPIR_ThreadInfo.isThreaded)) {       \
            MPIDI_Mutex_acquire(m);                     \
        }                                               \
    } while (0)

#define MPIDI_CS_EXIT(m)                                \
    do {                                                \
        if (likely(MPIR_ThreadInfo.isThreaded)) {       \
            MPIDI_Mutex_sync();                         \
            MPIDI_Mutex_release(m);                     \
        }                                               \
    } while (0)

#define MPIDI_CS_YIELD(m)                               \
    do {                                                \
        if (likely(MPIR_ThreadInfo.isThreaded)) {       \
            MPIDI_Mutex_sync();                         \
            MPIDI_Mutex_release(m);                     \
            MPIDI_Mutex_acquire(m);                     \
        }                                               \
    } while (0)

#define MPIDI_CS_TRY(m)                                 \
    do {                                                \
        if (likely(MPIR_ThreadInfo.isThreaded)) {       \
            MPIDI_Mutex_try_acquire(m);                 \
        }                                               \
    } while (0)

#define MPIDI_CS_SCHED_YIELD(m)                         \
    do {                                                \
        if (likely(MPIR_ThreadInfo.isThreaded)) {       \
            MPIDI_Mutex_sync();                         \
            MPIDI_Mutex_release(m);                     \
            sched_yield();                              \
            MPIDI_Mutex_acquire(m);                     \
        }                                               \
    } while (0)

#define MPIU_THREAD_CS_ALLFUNC_ENTER(_context)
#define MPIU_THREAD_CS_ALLFUNC_EXIT(_context)
#define MPIU_THREAD_CS_ALLFUNC_YIELD(_context)
#define MPIU_THREAD_CS_ALLFUNC_SCHED_YIELD(_context)
#define MPIU_THREAD_CS_ALLFUNC_TRY(_context)        (0)

#define MPIU_THREAD_CS_INIT_ENTER(_context)         MPIDI_CS_ENTER(0)
#define MPIU_THREAD_CS_INIT_EXIT(_context)          MPIDI_CS_EXIT(0)

#define MPIU_THREAD_CS_CONTEXTID_ENTER(_context)    MPIDI_CS_ENTER(0)
#define MPIU_THREAD_CS_CONTEXTID_EXIT(_context)     MPIDI_CS_EXIT (0)
#define MPIU_THREAD_CS_CONTEXTID_YIELD(_context)    MPIDI_CS_YIELD(0)
#define MPIU_THREAD_CS_CONTEXTID_SCHED_YIELD(_context) MPIDI_CS_SCHED_YIELD(0)
#define MPIU_THREAD_CS_HANDLEALLOC_ENTER(_context)  MPIDI_CS_ENTER(1)
#define MPIU_THREAD_CS_HANDLEALLOC_EXIT(_context)   MPIDI_CS_EXIT (1)
#define MPIU_THREAD_CS_HANDLE_ENTER(_context)       MPIDI_CS_ENTER(2)
#define MPIU_THREAD_CS_HANDLE_EXIT(_context)        MPIDI_CS_EXIT (2)
#define MPIU_THREAD_CS_INITFLAG_ENTER(_context)     MPIDI_CS_ENTER(3)
#define MPIU_THREAD_CS_INITFLAG_EXIT(_context)      MPIDI_CS_EXIT (3)
#define MPIU_THREAD_CS_MEMALLOC_ENTER(_context)     MPIDI_CS_ENTER(4)
#define MPIU_THREAD_CS_MEMALLOC_EXIT(_context)      MPIDI_CS_EXIT (4)
#define MPIU_THREAD_CS_MPI_OBJ_ENTER(context_)      MPIDI_CS_ENTER(5)
#define MPIU_THREAD_CS_MPI_OBJ_EXIT(context_)       MPIDI_CS_EXIT (5)
#define MPIU_THREAD_CS_MSGQUEUE_ENTER(_context)     MPIDI_CS_ENTER(6)
#define MPIU_THREAD_CS_MSGQUEUE_EXIT(_context)      MPIDI_CS_EXIT (6)
#define MPIU_THREAD_CS_PAMI_ENTER(_context)         MPIDI_CS_ENTER(7)
#define MPIU_THREAD_CS_PAMI_EXIT(_context)          MPIDI_CS_EXIT (7)
#define MPIU_THREAD_CS_ASYNC_ENTER(_context)        MPIDI_CS_ENTER(8)
#define MPIU_THREAD_CS_ASYNC_EXIT(_context)         MPIDI_CS_EXIT (8)

#endif /* MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL */


#endif /* !MPICH_MPIDTHREAD_H_INCLUDED */
