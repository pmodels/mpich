/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED)
#define MPIU_THREAD_MULTIPLE_H_INCLUDED

#include "mpiu_thread_priv.h"

#define MPIU_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIU_THREAD_CHECK_END   }

/* Nonrecursive mutex macros */
#define MPIUI_THREAD_CS_ENTER_NONRECURSIVE(lockname, mutex)             \
    do {                                                                \
        int err_;                                                       \
        MPIU_THREAD_CHECK_BEGIN;                                        \
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "locking %s", lockname);        \
        MPIU_Thread_mutex_lock(&mutex, &err_);                          \
        MPIU_THREAD_CHECK_END;                                          \
    } while (0)

#define MPIUI_THREAD_CS_EXIT_NONRECURSIVE(lockname, mutex)              \
    do {                                                                \
        int err_;                                                       \
        MPIU_THREAD_CHECK_BEGIN;                                        \
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "unlocking %s", lockname);      \
        MPIU_Thread_mutex_unlock(&mutex, &err_);                        \
        MPIU_THREAD_CHECK_END;                                          \
    } while (0)

#define MPIUI_THREAD_CS_YIELD_NONRECURSIVE(lockname, mutex)             \
    do {                                                                \
        MPIU_THREAD_CHECK_BEGIN;                                        \
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "yielding %s", lockname);       \
        MPIU_Thread_yield(&mutex);                                      \
        MPIU_THREAD_CHECK_END;                                          \
    } while (0)


/* Recursive mutex macros */
/* We don't need to protect the depth variable since it is thread
 * private and sequentially accessed within a thread */
#define MPIUI_THREAD_CS_ENTER_RECURSIVE(lockname, mutex)                \
    do {                                                                \
        int depth_;                                                     \
        MPIU_THREADPRIV_DECL;                                           \
        MPIU_THREADPRIV_GET;                                            \
                                                                        \
        MPIU_THREAD_CHECK_BEGIN;                                        \
        depth_ = MPIU_THREADPRIV_FIELD(lock_depth);                     \
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "recursive locking %s", lockname); \
        if (depth_ == 0) {                                              \
            MPIUI_THREAD_CS_ENTER_NONRECURSIVE(lockname, mutex);        \
        }                                                               \
        MPIU_THREADPRIV_FIELD(lock_depth) += 1;                         \
        MPIU_THREAD_CHECK_END;                                          \
    } while (0)

#define MPIUI_THREAD_CS_EXIT_RECURSIVE(lockname, mutex)                 \
    do {                                                                \
        int depth_;                                                     \
        MPIU_THREADPRIV_DECL;                                           \
        MPIU_THREADPRIV_GET;                                            \
                                                                        \
        MPIU_THREAD_CHECK_BEGIN;                                        \
        depth_ = MPIU_THREADPRIV_FIELD(lock_depth);                     \
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "recursive unlocking %s", lockname); \
        if (depth_ == 1) {                                              \
            MPIUI_THREAD_CS_EXIT_NONRECURSIVE(lockname, mutex);         \
        }                                                               \
        MPIU_THREADPRIV_FIELD(lock_depth) -= 1;                         \
        MPIU_THREAD_CHECK_END;                                          \
    } while (0)

#define MPIUI_THREAD_CS_YIELD_RECURSIVE MPIUI_THREAD_CS_YIELD_NONRECURSIVE

#define MPIU_THREAD_CS_ENTER(name, mutex) MPIUI_THREAD_CS_ENTER_##name(mutex)
#define MPIU_THREAD_CS_EXIT(name, mutex) MPIUI_THREAD_CS_EXIT_##name(mutex)
#define MPIU_THREAD_CS_YIELD(name, mutex) MPIUI_THREAD_CS_YIELD_##name(mutex)

/* Definitions of the thread support for various levels of thread granularity */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL
#include "mpiu_thread_global.h"
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT
#include "mpiu_thread_pobj.h"
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_LOCK_FREE
#error lock-free not yet implemented
#else
#error Unrecognized thread granularity
#endif

#endif /* !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED) */
