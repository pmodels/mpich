/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED)
#define MPIU_THREAD_MULTIPLE_H_INCLUDED

#include "mpiu_thread_priv.h"

/* Nonrecursive mutex macros */
#define MPIU_THREAD_CS_ENTER_NONRECURSIVE(mutex)                        \
    do {                                                                \
        int err_;                                                       \
        MPIU_Thread_mutex_lock(&mutex, &err_);                          \
    } while (0)

#define MPIU_THREAD_CS_EXIT_NONRECURSIVE(mutex)                         \
    do {                                                                \
        int err_;                                                       \
        MPIU_Thread_mutex_unlock(&mutex, &err_);                        \
    } while (0)

#define MPIU_THREAD_CS_YIELD_NONRECURSIVE(mutex)                        \
    do {                                                                \
        MPIU_Thread_yield(&mutex);                                      \
    } while (0)


/* Recursive mutex macros */
/* We don't need to protect the depth variable since it is thread
 * private and sequentially accessed within a thread */
#define MPIU_THREAD_CS_ENTER_RECURSIVE(mutex)           \
    do {                                                \
        int depth_;                                     \
        MPIU_THREADPRIV_DECL;                           \
        MPIU_THREADPRIV_GET;                            \
                                                        \
        depth_ = MPIU_THREADPRIV_FIELD(lock_depth);     \
        if (depth_ == 0) {                              \
            MPIU_THREAD_CS_ENTER_NONRECURSIVE(mutex);   \
        }                                               \
        MPIU_THREADPRIV_FIELD(lock_depth) += 1;         \
    } while (0)

#define MPIU_THREAD_CS_EXIT_RECURSIVE(mutex)            \
    do {                                                \
        int depth_;                                     \
        MPIU_THREADPRIV_DECL;                           \
        MPIU_THREADPRIV_GET;                            \
                                                        \
        depth_ = MPIU_THREADPRIV_FIELD(lock_depth);     \
        if (depth_ == 1) {                              \
            MPIU_THREAD_CS_EXIT_NONRECURSIVE(mutex);    \
        }                                               \
        MPIU_THREADPRIV_FIELD(lock_depth) -= 1;         \
    } while (0)

#define MPIU_THREAD_CS_YIELD_RECURSIVE MPIU_THREAD_CS_YIELD_NONRECURSIVE

#endif /* !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED) */
