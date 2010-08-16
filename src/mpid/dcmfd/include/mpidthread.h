/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpidthread.h
 * \brief ???
 *
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef MPICH_MPIDTHREAD_H_INCLUDED
#define MPICH_MPIDTHREAD_H_INCLUDED

/**
 * ******************************************************************
 * \brief Mutexes for thread/interrupt safety
 * ******************************************************************
 */

/* This file is included by mpidpre.h, so it is included before mpiimplthread.h.
 * This is intentional because it lets us override the critical section macros */

#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE) /* == defined(MPICH_IS_THREADED) */

/* suppress default macro definitions */
#define MPID_DEFINES_MPIU_THREAD_CS 1

/* FIXME this is set/unset by the MPICH2 top level configure, shouldn't be
 * defined here as well... */
#define HAVE_RUNTIME_THREADCHECK

/************** BEGIN PUBLIC INTERFACE ***************/

/* assumes an MPIU_THREADPRIV_DECL is present in an enclosing scope */
#define MPIU_THREAD_CS_INIT         \
    do {                            \
        MPIU_THREADPRIV_INITKEY;    \
        MPIU_THREADPRIV_INIT;       \
    } while (0)
#define MPIU_THREAD_CS_FINALIZE  do{}while(0)

/* definitions for main macro maze entry/exit */
#define MPIU_THREAD_CS_ENTER(name_,context_) MPIU_THREAD_CS_ENTER_##name_(context_)
#define MPIU_THREAD_CS_EXIT(name_,context_)  MPIU_THREAD_CS_EXIT_##name_(context_)
#define MPIU_THREAD_CS_YIELD(name_,context_) MPIU_THREAD_CS_YIELD_##name_(context_)

/* see FIXME in mpiimplthread.h if you are hacking on THREADSAFE_INIT code */
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

/************** END PUBLIC INTERFACE ***************/
/* everything that follows is just implementation details */

#if HAVE_DMA_CHANNELS
#define MPIDI_GetChannel(peerrank,contextid) ((((peerrank) + MPIR_Process.comm_world->rank-1) & (MPIDI_dma_channel_num - 1) ) + MPIDI_dma_channel_base)
#else
#define MPIDI_GetChannel(rank,contextid)  0
#endif

#define MPID_GLOBAL_MUTEX    (0)
#define MPID_DCMF_MUTEX      (MPID_GLOBAL_MUTEX)
#define MPID_HANDLE_MUTEX    (1)
#define MPID_RECVQ_MUTEX     (2)
#define MPID_RECVQ_MUTEX0    (MPID_RECVQ_MUTEX+0)
#define MPID_RECVQ_MUTEX1    (MPID_RECVQ_MUTEX+1)
#define MPID_RECVQ_MUTEX2    (MPID_RECVQ_MUTEX+2)
#define MPID_RECVQ_MUTEX3    (MPID_RECVQ_MUTEX+3)
#define MPID_RECVQ_MUTEX4    (MPID_RECVQ_MUTEX+4)
#define MPID_RECVQ_MUTEX5    (MPID_RECVQ_MUTEX+5)
#define MPIDI_MAX_NUM_CHANNELS (4)

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

#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(GLOBAL)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(GLOBAL)
#define MPIU_THREAD_CS_YIELD_ALLFUNC(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(GLOBAL)
#define MPIU_THREAD_CS_ENTER_HANDLE(_context)
#define MPIU_THREAD_CS_EXIT_HANDLE(_context)
#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context)
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context)
#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context) 
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context)
#define MPIU_THREAD_CS_ENTER_DCMF(_context)
#define MPIU_THREAD_CS_EXIT_DCMF(_context) 
#define MPIU_THREAD_CS_YIELD_DCMF(_context)
#define MPIU_THREAD_CS_ENTER_RECVQ(_context)
#define MPIU_THREAD_CS_EXIT_RECVQ(_context) 

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT

#define MPIU_THREAD_CS_ENTER_ALLFUNC(_context)
#define MPIU_THREAD_CS_EXIT_ALLFUNC(_context)
#define MPIU_THREAD_CS_YIELD_ALLFUNC(_context) 

/* We use the handle mutex to avoid conflicts with the global mutex */
#define MPIU_THREAD_CS_ENTER_HANDLE(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(HANDLE)
#define MPIU_THREAD_CS_EXIT_HANDLE(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(HANDLE)
/* The request handles may be allocated, and many other handles might
      be deallocated, within the communication routines.  To avoid 
      problems with lock nesting, for this particular case, we use a
      separate lock (similar to the per-object lock) */
#define MPIU_THREAD_CS_ENTER_HANDLEALLOC(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(HANDLE)
#define MPIU_THREAD_CS_EXIT_HANDLEALLOC(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(HANDLE)

#define MPIU_THREAD_CS_ENTER_CONTEXTID(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(GLOBAL)
#define MPIU_THREAD_CS_EXIT_CONTEXTID(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(GLOBAL)
#define MPIU_THREAD_CS_YIELD_CONTEXTID(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(GLOBAL)

#if HAVE_DMA_CHANNELS
   /* DG : DCMF global lock is not used in multichannel dcmf mode */
#  define MPIU_THREAD_CS_ENTER_DCMF(_context)
#  define MPIU_THREAD_CS_EXIT_DCMF(_context)
#  define MPIU_THREAD_CS_YIELD_DCMF(_context)
#else /* !HAVE_DMA_CHANNELS */
#  define MPIU_THREAD_CS_ENTER_DCMF(_context) MPIU_THREAD_CS_ENTER_LOCKNAME_CHECKED(DCMF)
#  define MPIU_THREAD_CS_EXIT_DCMF(_context)  MPIU_THREAD_CS_EXIT_LOCKNAME_CHECKED(DCMF)
#  define MPIU_THREAD_CS_YIELD_DCMF(_context) MPIU_THREAD_CS_YIELD_LOCKNAME_CHECKED(DCMF)
#endif /* HAVE_DMA_CHANNELS */

#define MPIDI_TS_INCR_RECVQ(_context) do {} while (0)

#define MPIU_THREAD_CS_ENTER_RECVQ(_context)                            \
    MPIU_THREAD_CHECK_BEGIN                                             \
    do {                                                                \
        DCMF_CriticalSection_enter(MPID_RECVQ_MUTEX + (_context));      \
        MPIDI_TS_INCR_RECVQ(_context);                                  \
    } while (0);                                                        \
    MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_EXIT_RECVQ(_context)                             \
    MPIU_THREAD_CHECK_BEGIN                                             \
    do {                                                                \
        MPIDI_TS_INCR_RECVQ(_context);                                  \
        DCMF_CriticalSection_exit(MPID_RECVQ_MUTEX + (_context));       \
    } while (0);                                                        \
    MPIU_THREAD_CHECK_END

#define MPIU_THREAD_CS_ENTER_LOCKNAME(name_) \
    do {                                                    \
        DCMF_CriticalSection_enter(MPID_##name_##_MUTEX);   \
        MPIDI_TS_INCR(name_);                               \
    } while (0)
#define MPIU_THREAD_CS_EXIT_LOCKNAME(name_)  \
    do {                                                    \
        MPIDI_TS_INCR(name_);                               \
        DCMF_CriticalSection_exit(MPID_##name_##_MUTEX);    \
    } while (0)
#define MPIU_THREAD_CS_YIELD_LOCKNAME(name_)                \
    do {                                                    \
        MPIDI_TS_INCR(name_);                               \
        DCMF_CriticalSection_cycle(MPID_##name_##_MUTEX);   \
        MPIDI_TS_INCR(name_);                               \
    } while (0)

#else 
#error "unhandled thread granularity"
#endif /* MPIU_THREAD_GRANULARITY == ... */

#endif /* MPICH_IS_THREADED */
#endif /* !MPICH_MPIDTHREAD_H_INCLUDED */
