/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_REFCOUNT_POBJ_H_INCLUDED
#define MPIR_REFCOUNT_POBJ_H_INCLUDED

/* define a type for the completion counter */
typedef MPL_atomic_int_t MPIR_cc_t;

static inline void MPIR_cc_set(MPIR_cc_t * cc_ptr, int val)
{
    /* Using relaxed semantics since this routine should only be used for
     * request initialization. */
    MPL_atomic_relaxed_store_int(cc_ptr, val);
}

ATTRIBUTE((unused))
static inline int MPIR_cc_is_complete(MPIR_cc_t * cc_ptr)
{
    int complete;

    complete = (0 == MPL_atomic_load_int(cc_ptr));

    return complete;
}

/* incomplete_==TRUE iff the cc > 0 after the decr */
#define MPIR_cc_decr(cc_ptr_, incomplete_)                      \
    do {                                                        \
        int ctr_;                                               \
        ctr_ = MPL_atomic_fetch_sub_int(cc_ptr_, 1);            \
        MPIR_Assert(ctr_ >= 1);                                 \
        *(incomplete_) = (ctr_ != 1);                           \
    } while (0)

/* This macro is only used for cancel_send right now. */
/* was_incomplete_==TRUE iff the cc==0 before the decr */
#define MPIR_cc_incr(cc_ptr_, was_incomplete_)                  \
    do {                                                        \
        *(was_incomplete_) = MPL_atomic_fetch_add_int(cc_ptr_, 1);   \
    } while (0)

#define MPIR_cc_get(cc_) MPL_atomic_load_int(&(cc_))

/* if we don't need return return - */
#define MPIR_cc_inc(cc_ptr_) \
    do { \
        MPL_atomic_fetch_add_int(cc_ptr_, 1); \
    } while (0)

#define MPIR_cc_dec(cc_ptr_) \
    do { \
        int ctr_;                                               \
        ctr_ = MPL_atomic_fetch_sub_int(cc_ptr_, 1); \
        MPIR_Assert(ctr_ >= 1); \
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
/* Ideally _GLOBAL would use this too, but we don't want to count on atomic
 * availability in _GLOBAL mode.  Instead the GLOBAL critical section should be
 * used. */
#define MPIR_OBJ_PUBLISH_HANDLE(hnd_lval_, handle_)                     \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            /* wmb ensures all read-only object field values are seen before the */ \
            /* handle value is seen at the application level */         \
            MPL_atomic_write_barrier();                                        \
            /* volatile ensures lval is not speculatively read or written */ \
            *(volatile int *)&(hnd_lval_) = (handle_);                  \
        }                                                               \
        else {                                                          \
            (hnd_lval_) = (handle_);                                    \
        }                                                               \
    } while (0)

#endif /* MPIR_REFCOUNT_POBJ_H_INCLUDED */
