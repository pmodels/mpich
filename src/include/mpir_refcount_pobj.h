/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_REFCOUNT_POBJ_H_INCLUDED
#define MPIR_REFCOUNT_POBJ_H_INCLUDED

/* define a type for the completion counter */
#include "opa_primitives.h"

typedef OPA_int_t MPIR_cc_t;

/* implies no barrier, since this routine should only be used for request
 * initialization */
static inline void MPIR_cc_set(MPIR_cc_t * cc_ptr, int val)
{
    if (val == 0) {
        /* values other than 0 do not enforce any ordering, and therefore do not
         * start a HB arc */
        /* MT FIXME using cc_set in this way is sloppy.  Sometimes the caller
         * really does know that the cc value may cleared, but more likely this
         * is just a hack to avoid the work of figuring out what the cc value
         * currently is and decrementing it instead. */
        /* barrier ensures that any state written before indicating completion is
         * seen by the thread polling on the cc.  If OPA adds store-release
         * semantics, we can convert to that instead. */
        OPA_write_barrier();
        MPL_VG_ANNOTATE_HAPPENS_BEFORE(cc_ptr);
    }
#if defined(MPL_VG_AVAILABLE)
    /* MT subtle: store_int is actually safe to use, but Helgrind/DRD/TSan all
     * view the store/load pair as a race.  Using an atomic operation for the
     * store side makes all three happy.  DRD & TSan also support
     * ANNOTATE_BENIGN_RACE, but Helgrind does not. */
    OPA_swap_int(cc_ptr, val);
#else
    OPA_store_int(cc_ptr, val);
#endif
}

ATTRIBUTE((unused))
static inline int MPIR_cc_is_complete(MPIR_cc_t * cc_ptr)
{
    int complete;

    complete = (0 == OPA_load_int(cc_ptr));
    if (complete) {
        MPL_VG_ANNOTATE_HAPPENS_AFTER(cc_ptr);
        OPA_read_barrier();
    }

    return complete;
}

/* incomplete_==TRUE iff the cc > 0 after the decr */
#define MPIR_cc_decr(cc_ptr_, incomplete_)                      \
    do {                                                        \
        int ctr_;                                               \
        OPA_write_barrier();                                    \
        MPL_VG_ANNOTATE_HAPPENS_BEFORE(cc_ptr_);                \
        ctr_ = OPA_fetch_and_decr_int(cc_ptr_);                 \
        MPIR_Assert(ctr_ >= 1);                                 \
        *(incomplete_) = (ctr_ != 1);                           \
        /* TODO check if this HA is actually necessary */       \
        if (!*(incomplete_)) {                                  \
            MPL_VG_ANNOTATE_HAPPENS_AFTER(cc_ptr_);             \
        }                                                       \
    } while (0)

/* MT FIXME does this need a HB/HA annotation?  This macro is only used for
 * cancel_send right now. */
/* was_incomplete_==TRUE iff the cc==0 before the decr */
#define MPIR_cc_incr(cc_ptr_, was_incomplete_)                  \
    do {                                                        \
        *(was_incomplete_) = OPA_fetch_and_incr_int(cc_ptr_);   \
    } while (0)

#define MPIR_cc_get(cc_) OPA_load_int(&(cc_))

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
#define MPIR_OBJ_PUBLISH_HANDLE(hnd_lval_, handle_)                     \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            /* wmb ensures all read-only object field values are seen before the */ \
            /* handle value is seen at the application level */         \
            OPA_write_barrier();                                        \
            /* volatile ensures lval is not speculatively read or written */ \
            *(volatile int *)&(hnd_lval_) = (handle_);                  \
        }                                                               \
        else {                                                          \
            (hnd_lval_) = (handle_);                                    \
        }                                                               \
    } while (0)

#endif /* MPIR_REFCOUNT_POBJ_H_INCLUDED */
