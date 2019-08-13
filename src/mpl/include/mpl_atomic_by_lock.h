/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_ATOMIC_BY_LOCK_H_INCLUDED
#define MPL_ATOMIC_BY_LOCK_H_INCLUDED

#include <stdint.h>
#include <assert.h>
#include <pthread.h>

/* defined in mpl_atomic.c */
extern pthread_mutex_t *MPL_emulation_lock;

#define MPL_ATOMIC_IPC_SINGLE_CS_ENTER()        \
    do {                                        \
        assert(MPL_emulation_lock);             \
        pthread_mutex_lock(MPL_emulation_lock); \
    } while (0)

#define MPL_ATOMIC_IPC_SINGLE_CS_EXIT()             \
    do {                                            \
        assert(MPL_emulation_lock);                 \
        pthread_mutex_unlock(MPL_emulation_lock);   \
    } while (0)


#define MPL_ATOMIC_INITIALIZER(val_) { (val_) }

#define MPL_ATOMIC_INT_T_INITIALIZER(val_)    MPL_ATOMIC_INITIALIZER(val_)
#define MPL_ATOMIC_INT32_T_INITIALIZER(val_)  MPL_ATOMIC_INITIALIZER(val_)
#define MPL_ATOMIC_UINT32_T_INITIALIZER(val_) MPL_ATOMIC_INITIALIZER(val_)
#define MPL_ATOMIC_INT64_T_INITIALIZER(val_)  MPL_ATOMIC_INITIALIZER(val_)
#define MPL_ATOMIC_UINT64_T_INITIALIZER(val_) MPL_ATOMIC_INITIALIZER(val_)
#define MPL_ATOMIC_PTR_T_INITIALIZER(val_)    MPL_ATOMIC_INITIALIZER(val_)

#define MPL_ATOMIC_DECL_FUNC_COMMON(TYPE, NAME)                                \
struct MPL_atomic_ ## NAME ## _t {                                             \
    volatile TYPE v;                                                           \
};                                                                             \
static inline TYPE MPL_atomic_relaxed_load_ ## NAME                            \
                                (const struct MPL_atomic_ ## NAME ## _t * ptr) \
{                                                                              \
    TYPE val;                                                                  \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    val = (TYPE)ptr->v;                                                        \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
    return val;                                                                \
}                                                                              \
static inline TYPE MPL_atomic_acquire_load_ ## NAME                            \
                                (const struct MPL_atomic_ ## NAME ## _t * ptr) \
{                                                                              \
    TYPE val;                                                                  \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    val = (TYPE)ptr->v;                                                        \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
    return val;                                                                \
}                                                                              \
static inline void MPL_atomic_relaxed_store_ ## NAME                           \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    ptr->v = val;                                                              \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
}                                                                              \
static inline void MPL_atomic_release_store_ ## NAME                           \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    ptr->v = val;                                                              \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
}                                                                              \
static inline TYPE MPL_atomic_cas_ ## NAME                                     \
                (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE oldv, TYPE newv) \
{                                                                              \
    TYPE prev;                                                                 \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    prev = (TYPE)ptr->v;                                                       \
    if (prev == oldv)                                                          \
        ptr->v = newv;                                                         \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
    return prev;                                                               \
}                                                                              \
static inline TYPE MPL_atomic_swap_ ## NAME                                    \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    TYPE prev;                                                                 \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    prev = (TYPE)ptr->v;                                                       \
    ptr->v = val;                                                              \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
    return prev;                                                               \
}

#define MPL_ATOMIC_DECL_FUNC_FAA(TYPE, NAME)                                   \
static inline TYPE MPL_atomic_fetch_add_ ## NAME                               \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    TYPE prev;                                                                 \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    prev = (TYPE)ptr->v;                                                       \
    ptr->v += val;                                                             \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
    return prev;                                                               \
}                                                                              \
static inline TYPE MPL_atomic_fetch_sub_ ## NAME                               \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    TYPE prev;                                                                 \
    MPL_ATOMIC_IPC_SINGLE_CS_ENTER();                                          \
    prev = (TYPE)ptr->v;                                                       \
    ptr->v -= val;                                                             \
    MPL_ATOMIC_IPC_SINGLE_CS_EXIT();                                           \
    return prev;                                                               \
}

#define MPL_ATOMIC_DECL_FUNC_VAL(TYPE, NAME) \
        MPL_ATOMIC_DECL_FUNC_COMMON(TYPE, NAME) \
        MPL_ATOMIC_DECL_FUNC_FAA(TYPE, NAME)

#define MPL_ATOMIC_DECL_FUNC_PTR(TYPE, NAME) \
        MPL_ATOMIC_DECL_FUNC_COMMON(TYPE, NAME)

/* int */
MPL_ATOMIC_DECL_FUNC_VAL(int, int)
/* int32_t */
MPL_ATOMIC_DECL_FUNC_VAL(int32_t, int32)
/* uint32_t */
MPL_ATOMIC_DECL_FUNC_VAL(uint32_t, uint32)
/* int64_t */
MPL_ATOMIC_DECL_FUNC_VAL(int64_t, int64)
/* uint64_t */
MPL_ATOMIC_DECL_FUNC_VAL(uint64_t, uint64)
/* void * */
MPL_ATOMIC_DECL_FUNC_PTR(void *, ptr)

/* lock/unlock provides barrier */
static inline void MPL_atomic_write_barrier(void)
{
    ;
}

static inline void MPL_atomic_read_barrier(void)
{
    ;
}

static inline void MPL_atomic_read_write_barrier(void)
{
    ;
}

static inline void MPL_atomic_compiler_barrier(void)
{
    ;
}

#endif /* MPL_ATOMIC_BY_LOCK_H_INCLUDED */
