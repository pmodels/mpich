/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPID_TICKETLOCK_H_INCLUDED
#define MPID_TICKETLOCK_H_INCLUDED

#define MPIDI_CH4_CACHELINE_SIZE 64

typedef union MPIDI_CH4_Ticket_lock {
    unsigned u;
    char cacheline[MPIDI_CH4_CACHELINE_SIZE];
    struct {
        unsigned short ticket;
        unsigned short clients;
    } s;
} MPIDI_CH4_Ticket_lock MPL_ATTR_ALIGNED(MPIDI_CH4_CACHELINE_SIZE);

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_Thread_mutex_acquire(MPIDI_CH4_Ticket_lock * m)
{
    uint16_t u = __sync_fetch_and_add(&m->s.clients, 1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_ACQUIRE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_ACQUIRE);

    while (m->s.ticket != u)
        asm volatile ("pause\n":::"memory");

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_ACQUIRE);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_Thread_mutex_release(MPIDI_CH4_Ticket_lock * m)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_RELEASE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_RELEASE);

    asm volatile ("":::"memory");
    m->s.ticket++;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_RELEASE);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4I_Thread_mutex_try_acquire(MPIDI_CH4_Ticket_lock * m)
{
    uint16_t u = m->s.clients;
    uint16_t u2 = u + 1;
    uint32_t val = ((uint32_t) u << 16) + u;
    uint32_t val2 = ((uint32_t) u2 << 16) + u;
    int ret = EBUSY;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_TRY_ACQUIRE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_TRY_ACQUIRE);

    if (__sync_val_compare_and_swap(&m->u, val, val2) == val)
        ret = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_TRY_ACQUIRE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_Thread_mutex_lock(MPIDI_CH4_Ticket_lock * m,
                                                           int *mpi_error)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_LOCK);

    MPIDI_CH4I_Thread_mutex_acquire(m);
    *mpi_error = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_LOCK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_Thread_mutex_unlock(MPIDI_CH4_Ticket_lock * m,
                                                             int *mpi_error)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_UNLOCK);

    MPIDI_CH4I_Thread_mutex_release(m);
    *mpi_error = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_UNLOCK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_Thread_mutex_create(MPIDI_CH4_Ticket_lock * m,
                                                             int *mpi_error)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_CREATE);

    m->u = 0;
    *mpi_error = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_CREATE);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_CH4I_Thread_mutex_destroy(MPIDI_CH4_Ticket_lock * m,
                                                              int *mpi_error)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_DESTROY);

    m->u = 0;
    *mpi_error = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_MUTEX_DESTROY);
}

/* For this implementation we have two options                                        */
/* 1)  Split the typedef for condition variable mutexes and call the utility routines */
/* 2)  Implement it from scratch                                                      */
/* Currently only async.c is using condition variables, so we should figure out what  */
/* we really want from the cv implementations                                         */
MPL_STATIC_INLINE_PREFIX void
MPIDI_CH4I_Thread_cond_wait(MPIDU_Thread_cond_t * cond, MPIDI_CH4_Ticket_lock * m, int *mpi_error)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4I_THREAD_COND_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4I_THREAD_COND_WAIT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4I_THREAD_COND_WAIT);
}


#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL

#define MPIDI_CH4I_THREAD_CS_ENTER_POBJ(mutex)
#define MPIDI_CH4I_THREAD_CS_EXIT_POBJ(mutex)
#define MPIDI_CH4I_THREAD_CS_TRY_POBJ(mutex)
#define MPIDI_CH4I_THREAD_CS_YIELD_POBJ(mutex)

#define MPIDI_CH4I_THREAD_CS_ENTER_GLOBAL(m) do { if (MPIR_ThreadInfo.isThreaded) {  MPIDI_CH4I_Thread_mutex_acquire(&m); }} while (0)
#define MPIDI_CH4I_THREAD_CS_EXIT_GLOBAL(m)  do { if (MPIR_ThreadInfo.isThreaded) {  MPIDI_CH4I_Thread_mutex_release(&m); }} while (0)
#define MPIDI_CH4I_THREAD_CS_TRY_GLOBAL(m)   do { (0==MPIDI_CH4I_Thread_mutex_try_acquire(&m));}} while (0)
#define MPIDI_CH4I_THREAD_CS_YIELD_GLOBAL(m) do { if (MPIR_ThreadInfo.isThreaded) {  MPIDI_CH4I_Thread_mutex_release(&m); sched_yield(); MPIDI_CH4I_Thread_mutex_acquire(&m); }} while (0)

#define MPIDI_CH4I_THREAD_CS_ENTER_ALLGRAN(mutex) MPIDI_CH4I_THREAD_CS_ENTER_GLOBAL(m)
#define MPIDI_CH4I_THREAD_CS_EXIT_ALLGRAN(mutex)  MPIDI_CH4I_THREAD_CS_EXIT_GLOBAL(m)
#define MPIDI_CH4I_THREAD_CS_TRY_ALLGRAN(mutex)   MPIDI_CH4I_THREAD_CS_TRY_GLOBAL(m)
#define MPIDI_CH4I_THREAD_CS_YIELD_ALLGRAN(mutex) MPIDI_CH4I_THREAD_CS_YIELD_GLOBAL(m)

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ

#define MPIDI_CH4I_THREAD_CS_ENTER_POBJ(m)                   \
        do {                                                \
                if (likely(MPIR_ThreadInfo.isThreaded)) {   \
                        MPIDI_CH4I_Thread_mutex_acquire(&m); \
                }                                           \
        } while (0)

#define MPIDI_CH4I_THREAD_CS_EXIT_POBJ(m)                    \
        do {                                                \
                if (likely(MPIR_ThreadInfo.isThreaded)) {   \
                        MPIDI_CH4I_Thread_mutex_release(&m); \
                }                                           \
        } while (0)

#define MPIDI_CH4I_THREAD_CS_TRY_POBJ(m)                         \
        do {                                                    \
                if (likely(MPIR_ThreadInfo.isThreaded)) {       \
                        MPIDI_CH4I_Thread_mutex_try_acquire(&m); \
                }                                               \
        } while (0)

#define MPIDI_CH4I_THREAD_CS_YIELD_POBJ(m)                   \
        do {                                                \
                if (likely(MPIR_ThreadInfo.isThreaded)) {   \
                        MPIDI_CH4I_Thread_mutex_release(&m); \
                        sched_yield();                      \
                        MPIDI_CH4I_Thread_mutex_acquire(&m); \
                }                                           \
        } while (0)

#define MPIDI_CH4I_THREAD_CS_ENTER_ALLGRAN MPIDI_CH4I_THREAD_CS_ENTER_POBJ
#define MPIDI_CH4I_THREAD_CS_EXIT_ALLGRAN  MPIDI_CH4I_THREAD_CS_EXIT_POBJ
#define MPIDI_CH4I_THREAD_CS_TRY_ALLGRAN   MPIDI_CH4I_THREAD_CS_TRY_POBJ
#define MPIDI_CH4I_THREAD_CS_YIELD_ALLGRAN MPIDI_CH4I_THREAD_CS_YIELD_POBJ

/* GLOBAL locks are all NO-OPs */
#define MPIDI_CH4I_THREAD_CS_ENTER_GLOBAL(mutex)
#define MPIDI_CH4I_THREAD_CS_EXIT_GLOBAL(mutex)
#define MPIDI_CH4I_THREAD_CS_TRY_GLOBAL(mutex)
#define MPIDI_CH4I_THREAD_CS_YIELD_GLOBAL(mutex)

#else

#error "Ticket locks are only supported in Global or Per-Object Granularity"

#endif /* MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL */

#define MPIDI_CH4I_THREAD_CS_ENTER(name, mutex) MPIDI_CH4I_THREAD_CS_ENTER_##name(mutex)
#define MPIDI_CH4I_THREAD_CS_EXIT(name, mutex)  MPIDI_CH4I_THREAD_CS_EXIT_##name(mutex)
#define MPIDI_CH4I_THREAD_CS_TRY(name, mutex)  MPIDI_CH4I_THREAD_CS_TRY_##name(mutex)
#define MPIDI_CH4I_THREAD_CS_YIELD(name, mutex) MPIDI_CH4I_THREAD_CS_YIELD_##name(mutex)

#endif /* MPID_TICKETLOCK_H_INCLUDED */
