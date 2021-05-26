/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_REFCOUNT_GLOBAL_H_INCLUDED
#define MPIR_REFCOUNT_GLOBAL_H_INCLUDED

/* define a type for the completion counter */
/* memory barriers aren't needed in this impl, because all access to completion
 * counters is done while holding the GLOBAL critical section */
typedef volatile int MPIR_cc_t;
#define MPIR_cc_get(cc_) (cc_)
#define MPIR_cc_set(cc_ptr_, val_) (*(cc_ptr_)) = (val_)
#define MPIR_cc_is_complete(cc_ptr_) (0 == *(cc_ptr_))

#define MPIR_cc_incr(cc_ptr_, was_incomplete_)  \
    do {                                        \
        *(was_incomplete_) = (*(cc_ptr_))++;    \
    } while (0)

#define MPIR_cc_decr(cc_ptr_, incomplete_)      \
    do {                                        \
        *(incomplete_) = --(*(cc_ptr_));        \
        MPIR_Assert(*(incomplete_) >= 0);       \
    } while (0)

#define MPIR_cc_inc(cc_ptr_) \
    do { \
        (*(cc_ptr_))++; \
    } while (0)

#define MPIR_cc_dec(cc_ptr_) \
    do { \
        (*(cc_ptr_))--; \
        MPIR_Assert(*(cc_ptr_) >= 0); \
    } while (0)

/* "publishes" the obj with handle value (handle_) via the handle pointer
 * (hnd_lval_).  That is, it is a version of the following statement that fixes
 * memory consistency issues:
 *     (hnd_lval_) = (handle_);
 *
 * assumes that the following is always true: typeof(*hnd_lval_ptr_)==int
 */
/* This could potentially be generalized beyond MPI-handle objects, but we
 * should only take that step after seeing good evidence of its use.  A general
 * macro (that is portable to non-gcc compilers) will need type information to
 * make the appropriate volatile cast. */
/* Ideally _GLOBAL would use this too, but we don't want to count on OPA
 * availability in _GLOBAL mode.  Instead the GLOBAL critical section should be
 * used. */
#define MPIR_OBJ_PUBLISH_HANDLE(hnd_lval_, handle_)     \
    do {                                                \
        (hnd_lval_) = (handle_);                        \
    } while (0)

#endif /* MPIR_REFCOUNT_GLOBAL_H_INCLUDED */
