/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_ATOMIC_OPA_H_INCLUDED
#define MPL_ATOMIC_OPA_H_INCLUDED

#include <stdint.h>
#include "../../openpa/src/opa_primitives.h"

/* int, int32_t, and uint32_t */

struct MPL_atomic_int32_static_assert_t {
    /* The following causes an error if the assumption does not hold. */
    int assert1[sizeof(int) == 4 ? 1 : -1];
    int assert2[((int32_t) ((int) ((int32_t) 2) - (int) ((int32_t) - 3)) ==
                 ((int32_t) 5)) ? 1 : -1];
    int assert3[((int32_t) ((int) ((int32_t) 2) + (int) ((int32_t) - 3)) ==
                 ((int32_t) - 1)) ? 1 : -1];
    int assert4[((uint32_t) ((int) ((uint32_t) 2) - (int) ((uint32_t) 1)) ==
                 ((uint32_t) 1)) ? 1 : -1];
    int assert5[((uint32_t) ((int) ((uint32_t) 2) + (int) ((uint32_t) 1)) ==
                 ((uint32_t) 3)) ? 1 : -1];
};

#define MPL_ATOMIC_INT_T_INITIALIZER(val_)    { OPA_INT_T_INITIALIZER(val_) }
#define MPL_ATOMIC_INT32_T_INITIALIZER(val_)  { OPA_INT_T_INITIALIZER((int32_t)val_) }
#define MPL_ATOMIC_UINT32_T_INITIALIZER(val_) { OPA_INT_T_INITIALIZER((uint32_t)val_) }

#define MPL_ATOMIC_DECL_FUNC_INT32(TYPE, NAME)                                 \
struct MPL_atomic_ ## NAME ## _t {                                             \
     OPA_int_t v;                                                              \
};                                                                             \
static inline TYPE MPL_atomic_relaxed_load_ ## NAME                            \
                                (const struct MPL_atomic_ ## NAME ## _t * ptr) \
{                                                                              \
    return (TYPE)OPA_load_int(&ptr->v);                                        \
}                                                                              \
static inline TYPE MPL_atomic_acquire_load_ ## NAME                            \
                                (const struct MPL_atomic_ ## NAME ## _t * ptr) \
{                                                                              \
    return (TYPE)OPA_load_acquire_int(&ptr->v);                                \
}                                                                              \
static inline void MPL_atomic_relaxed_store_ ## NAME                           \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    OPA_store_int(&ptr->v, (int)val);                                          \
}                                                                              \
static inline void MPL_atomic_release_store_ ## NAME                           \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    OPA_store_release_int(&ptr->v, (int)val);                                  \
}                                                                              \
static inline TYPE MPL_atomic_cas_ ## NAME                                     \
                (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE oldv, TYPE newv) \
{                                                                              \
    return (TYPE)OPA_cas_int(&ptr->v, (int)oldv, (int)newv);                   \
}                                                                              \
static inline TYPE MPL_atomic_swap_ ## NAME                                    \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    return (TYPE)OPA_swap_int(&ptr->v, (int)val);                              \
}                                                                              \
static inline TYPE MPL_atomic_fetch_add_ ## NAME                               \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    return (TYPE)OPA_fetch_and_add_int(&ptr->v, (int)val);                     \
}                                                                              \
static inline TYPE MPL_atomic_fetch_sub_ ## NAME                               \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    return (TYPE)OPA_fetch_and_add_int(&ptr->v, -(int)val);                    \
}

/* int */
MPL_ATOMIC_DECL_FUNC_INT32(int, int)
/* int32_t */
MPL_ATOMIC_DECL_FUNC_INT32(int32_t, int32)
/* uint32_t */
MPL_ATOMIC_DECL_FUNC_INT32(uint32_t, uint32)

/* int64_t and uint64_t */
struct MPL_atomic_int64_static_assert_t {
    /* The following causes an error if the assumption does not hold. */
    int assert1[sizeof(void *) == 8 ? 1 : -1];
};

#define MPL_ATOMIC_INT64_T_INITIALIZER(val_)  { OPA_PTR_T_INITIALIZER((void *)(intptr_t)val_) }
#define MPL_ATOMIC_UINT64_T_INITIALIZER(val_) { OPA_PTR_T_INITIALIZER((void *)(intptr_t)val_) }

#define MPL_ATOMIC_DECL_FUNC_INT64(TYPE, NAME)                                 \
struct MPL_atomic_ ## NAME ## _t {                                             \
     OPA_ptr_t v;                                                              \
};                                                                             \
static inline TYPE MPL_atomic_relaxed_load_ ## NAME                            \
                                (const struct MPL_atomic_ ## NAME ## _t * ptr) \
{                                                                              \
    return (TYPE)(intptr_t)OPA_load_ptr(&ptr->v);                              \
}                                                                              \
static inline TYPE MPL_atomic_acquire_load_ ## NAME                            \
                                (const struct MPL_atomic_ ## NAME ## _t * ptr) \
{                                                                              \
    return (TYPE)(intptr_t)OPA_load_acquire_ptr(&ptr->v);                      \
}                                                                              \
static inline void MPL_atomic_relaxed_store_ ## NAME                           \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    OPA_store_ptr(&ptr->v, (void *)((intptr_t)val));                           \
}                                                                              \
static inline void MPL_atomic_release_store_ ## NAME                           \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    OPA_store_release_ptr(&ptr->v, (void *)((intptr_t)val));                   \
}                                                                              \
static inline TYPE MPL_atomic_cas_ ## NAME                                     \
                (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE oldv, TYPE newv) \
{                                                                              \
    return (TYPE)(intptr_t)OPA_cas_ptr(&ptr->v, (void *)((intptr_t)oldv),      \
                                       (void *)((intptr_t)newv));              \
}                                                                              \
static inline TYPE MPL_atomic_swap_ ## NAME                                    \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    return (TYPE)(intptr_t)OPA_swap_ptr(&ptr->v, (void *)((intptr_t)val));     \
}                                                                              \
static inline TYPE MPL_atomic_fetch_add_ ## NAME                               \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    while (1) {                                                                \
        TYPE old_val = (TYPE)(intptr_t)OPA_load_acquire_ptr(&ptr->v);          \
        TYPE new_val = old_val + val;                                          \
        void *cas_val = OPA_cas_ptr(&ptr->v, (void *)((intptr_t)old_val),      \
                                             (void *)((intptr_t)new_val));     \
        if (cas_val == (void *)((intptr_t)old_val)) {                          \
            return old_val;                                                    \
        }                                                                      \
    }                                                                          \
}                                                                              \
static inline TYPE MPL_atomic_fetch_sub_ ## NAME                               \
                            (struct MPL_atomic_ ## NAME ## _t * ptr, TYPE val) \
{                                                                              \
    while (1) {                                                                \
        TYPE old_val = (TYPE)(intptr_t)OPA_load_acquire_ptr(&ptr->v);          \
        TYPE new_val = old_val - val;                                          \
        void *cas_val = OPA_cas_ptr(&ptr->v, (void *)((intptr_t)old_val),      \
                                             (void *)((intptr_t)new_val));     \
        if (cas_val == (void *)((intptr_t)old_val)) {                          \
            return old_val;                                                    \
        }                                                                      \
    }                                                                          \
}

/* int64_t */
MPL_ATOMIC_DECL_FUNC_INT64(int64_t, int64)

/* uint64_t */
    MPL_ATOMIC_DECL_FUNC_INT64(uint64_t, uint64)

/* void * */
#define MPL_ATOMIC_PTR_T_INITIALIZER(val_)    { OPA_PTR_T_INITIALIZER(val_) }
struct MPL_atomic_ptr_t {
    OPA_ptr_t v;
};

static inline void *MPL_atomic_relaxed_load_ptr(const MPL_atomic_ptr_t * ptr)
{
    return OPA_load_ptr(&ptr->v);
}

static inline void *MPL_atomic_acquire_load_ptr(const MPL_atomic_ptr_t * ptr)
{
    return OPA_load_acquire_ptr(&ptr->v);
}

static inline void MPL_atomic_relaxed_store_ptr(MPL_atomic_ptr_t * ptr, void *val)
{
    OPA_store_ptr(&ptr->v, val);
}

static inline void MPL_atomic_release_store_ptr(MPL_atomic_ptr_t * ptr, void *val)
{
    OPA_store_release_ptr(&ptr->v, val);
}

static inline void *MPL_atomic_cas_ptr(MPL_atomic_ptr_t * ptr, void *oldv, void *newv)
{
    return OPA_cas_ptr(&ptr->v, oldv, newv);
}

static inline void *MPL_atomic_swap_ptr(MPL_atomic_ptr_t * ptr, void *val)
{
    return OPA_swap_ptr(&ptr->v, val);
}

/* Barriers */
static inline void MPL_atomic_write_barrier(void)
{
    OPA_write_barrier();
}

static inline void MPL_atomic_read_barrier(void)
{
    OPA_read_barrier();
}

static inline void MPL_atomic_read_write_barrier(void)
{
    OPA_read_write_barrier();
}

static inline void MPL_atomic_compiler_barrier(void)
{
    OPA_compiler_barrier();
}

#endif /* MPL_ATOMIC_OPA_H_INCLUDED */
