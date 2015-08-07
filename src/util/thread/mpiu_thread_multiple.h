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

#define MPIU_THREAD_CS_ENTER(_name,_context) MPIU_THREAD_CS_ENTER_##_name(_context)
#define MPIU_THREAD_CS_EXIT(_name,_context) MPIU_THREAD_CS_EXIT_##_name(_context)
#define MPIU_THREAD_CS_YIELD(_name,_context) MPIU_THREAD_CS_YIELD_##_name(_context)

/* recursive locks */
#define MPIU_THREAD_CS_ENTER_LOCKNAME_RECURSIVE(name_,mutex_)           \
    do {                                                                \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to recursively ENTER lockname=%s", #name_); \
        MPIU_Thread_CS_enter_lockname_recursive_impl_(MPIU_NEST_##name_, #name_, &mutex_); \
    } while (0)

#define MPIU_THREAD_CS_EXIT_LOCKNAME_RECURSIVE(name_,mutex_)            \
    do {                                                                \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to recursively EXIT lockname=%s", #name_); \
        MPIU_Thread_CS_exit_lockname_recursive_impl_(MPIU_NEST_##name_, #name_, &mutex_); \
    } while (0)

#define MPIU_THREAD_CS_YIELD_LOCKNAME_RECURSIVE(name_,mutex_)           \
    do {                                                                \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to recursively YIELD lockname=%s", #name_); \
        MPIU_Thread_CS_yield_lockname_recursive_impl_(MPIU_NEST_##name_, #name_, &mutex_); \
    } while (0)

/* regular locks */
#define MPIU_THREAD_CS_ENTER_LOCKNAME(name_,mutex_)                     \
    do {                                                                \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to ENTER lockname=%s", #name_); \
        MPIU_Thread_CS_enter_lockname_impl_(#name_, &mutex_); \
    } while (0)

#define MPIU_THREAD_CS_EXIT_LOCKNAME(name_,mutex_)                      \
    do {                                                                \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to EXIT lockname=%s", #name_); \
        MPIU_Thread_CS_exit_lockname_impl_(#name_, &mutex_); \
    } while (0)

#define MPIU_THREAD_CS_YIELD_LOCKNAME(name_,mutex_)                     \
    do {                                                                \
        MPIU_DBG_MSG_S(THREAD,VERBOSE,"attempting to YIELD lockname=%s", #name_); \
        MPIU_Thread_CS_yield_lockname_impl_(#name_, &mutex_); \
    } while (0)


enum MPIU_Nest_mutexes {
    MPIU_NEST_GLOBAL = 0,
    MPIU_NEST_POBJ,   /* Nesting does not really make sense for POBJ */
    MPIU_NEST_NUM_MUTEXES
};


/* Definitions of the thread support for various levels of thread granularity */
#if MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_GLOBAL
#include "mpiu_thread_global.h"
#elif MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_PER_OBJECT
#include "mpiu_thread_pobj.h"
#elif MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_LOCK_FREE
#error lock-free not yet implemented
#else
#error Unrecognized thread granularity
#endif


#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_enter_lockname_impl_
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* these are inline functions instead of macros to avoid some of the
 * MPIU_THREADPRIV_DECL scoping issues */
MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((unused))
static MPL_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_enter_lockname_impl_(const char *lockname, MPIU_Thread_mutex_t * mutex)
{
    int err;
    MPIU_DBG_MSG_S(THREAD, TYPICAL, "locking %s", lockname);
    MPIU_Thread_mutex_lock(mutex, &err);
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_exit_lockname_impl_
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((unused))
static MPL_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_exit_lockname_impl_(const char *lockname, MPIU_Thread_mutex_t * mutex)
{
    int err;
    MPIU_DBG_MSG_S(THREAD, TYPICAL, "unlocking %s", lockname);
    MPIU_Thread_mutex_unlock(mutex, &err);
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_yield_lockname_impl_
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((unused))
static MPL_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_yield_lockname_impl_(const char *lockname, MPIU_Thread_mutex_t * mutex)
{
    MPIU_DBG_MSG_S(THREAD, TYPICAL, "yielding %s", lockname);
    MPIU_Thread_yield(mutex);
}

/* ------------------------------------------------------------------- */
/* recursive versions, these are for the few locks where it is difficult or
 * impossible to eliminate recursive locking usage */

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_enter_lockname_recursive_impl_
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* these are inline functions instead of macros to avoid some of the
 * MPIU_THREADPRIV_DECL scoping issues */
MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((unused))
static MPL_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_enter_lockname_recursive_impl_(enum MPIU_Nest_mutexes kind,
                                              const char *lockname, MPIU_Thread_mutex_t * mutex)
{
    int depth, err;
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    depth = MPIU_THREADPRIV_FIELD(lock_depth)[kind];
    MPIU_DBG_MSG_D(THREAD, TYPICAL, "recursive enter, depth=%d", depth);

    MPIU_Assert(depth >= 0 && depth < 10);      /* probably a mismatch if we hit this */

    if (depth == 0) {
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "locking %s", lockname);
        MPIU_Thread_mutex_lock(mutex, &err);
    }
    MPIU_THREADPRIV_FIELD(lock_depth)[kind] += 1;
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_exit_lockname_recursive_impl_
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((unused))
static MPL_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_exit_lockname_recursive_impl_(enum MPIU_Nest_mutexes kind,
                                             const char *lockname, MPIU_Thread_mutex_t * mutex)
{
    int depth, err;
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    depth = MPIU_THREADPRIV_FIELD(lock_depth)[kind];
    MPIU_DBG_MSG_D(THREAD, TYPICAL, "recursive exit, depth=%d", depth);

    MPIU_Assert(depth > 0 && depth < 10);       /* probably a mismatch if we hit this */

    if (depth == 1) {
        MPIU_DBG_MSG_S(THREAD, TYPICAL, "unlocking %s", lockname);
        MPIU_Thread_mutex_unlock(mutex, &err);
        MPIU_Assert_fmt_msg(err == MPIU_THREAD_SUCCESS,
                            ("mutex_unlock failed, err=%d (%s)", err, MPIU_Strerror(err)));

    }
    MPIU_THREADPRIV_FIELD(lock_depth)[kind] -= 1;
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_yield_lockname_recursive_impl_
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_DBG_ATTRIBUTE_NOINLINE ATTRIBUTE((unused))
static MPL_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_yield_lockname_recursive_impl_(enum MPIU_Nest_mutexes kind,
                                              const char *lockname, MPIU_Thread_mutex_t * mutex)
{
    int depth ATTRIBUTE((unused));
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    depth = MPIU_THREADPRIV_FIELD(lock_depth)[kind];
    MPIU_DBG_MSG_D(THREAD, TYPICAL, "recursive yield, depth=%d", depth);

    MPIU_Assert(depth > 0 && depth < 10);       /* we must hold the mutex */
    /* no need to update depth, this is a thread-local value */

    MPIU_DBG_MSG_S(THREAD, TYPICAL, "yielding %s", lockname);
    MPIU_Thread_yield(mutex);
}

/* undef for safety, this is a commonly-included header */
#undef FUNCNAME
#undef FCNAME

#endif /* !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED) */
