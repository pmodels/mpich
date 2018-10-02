/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_IOVEC_UTIL_H_INCLUDED
#define OFI_IOVEC_UTIL_H_INCLUDED

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif /* HAVE_SYS_UIO_H */
#include <assert.h>
#include "ofi_types.h"

#define MPIDI_OFI_SEG_DONE     0
#define MPIDI_OFI_SEG_EAGAIN   1
#define MPIDI_OFI_SEG_ERROR   -1

/*
 * Notes:
 * Segment processor transforms MPI datatype into OFI datatype as follows:
 * - MPIR_Segment is initialized for MPI datatype.
 * - A single OFI call can only take number of iovecs up to origin/target/result_max_iovs,
 *   MPIDI_OFI_merge_segment/MPIDI_OFI_merge_segment2 generates iovs subject to this limit.
 */
#define MPIDI_OFI_INIT_SEG_STATE(var, VAR)                              \
do {                                                                    \
    MPIDI_OFI_segment_next(seg_state,                                   \
                           &seg_state->var## _iov,                      \
                           MPIDI_OFI_SEGMENT_## VAR);                   \
    seg_state->var## _addr    = (uintptr_t)seg_state->var## _iov.iov_base; \
    seg_state->var## _iov_len = (uintptr_t)seg_state->var## _iov.iov_len; \
    /* Pack first iovec in initialization, and it completes in two cases:
     * - iovec contains valid bytes (i.e., iov_len > 0).
     * - iovec contains 0 byte and no more data to pack. */             \
    while (seg_state->var## _iov_len == 0) {                            \
        int done =                                                      \
        MPIDI_OFI_segment_next(seg_state,                               \
                               &seg_state->var## _iov,                  \
                               MPIDI_OFI_SEGMENT_## VAR);               \
        if (!done) {                                                    \
            seg_state->var## _addr = (uintptr_t)seg_state->var## _iov.iov_base; \
            seg_state->var## _iov_len = (uintptr_t)seg_state->var## _iov.iov_len;  \
        } else {                                                        \
            break;                                                      \
        }                                                               \
    }                                                                   \
} while (0)

#define MPIDI_OFI_NEXT_SEG_STATE(var,VAR)                               \
do {                                                                    \
    *var## _addr_next       = seg_state->var## _addr;                   \
    seg_state->var## _addr += buf_size;                                 \
    seg_state->var## _iov_len -= buf_size;                              \
    while (seg_state->var## _iov_len == 0) {                            \
        int done =                                                      \
        MPIDI_OFI_segment_next(seg_state,                               \
                               &seg_state->var## _iov,                  \
                               MPIDI_OFI_SEGMENT_## VAR);               \
        if (!done) {                                                    \
            seg_state->var## _addr    = (uintptr_t)seg_state->var## _iov.iov_base; \
            seg_state->var## _iov_len = (uintptr_t)seg_state->var## _iov.iov_len; \
        } else {                                                        \
            break;                                                      \
        }                                                               \
    }                                                                   \
} while (0)

#define MPIDI_OFI_INIT_SEG(var)                                         \
do {                                                                    \
    ((struct iovec*)(&var## _iov[0]))->iov_len  = last_len;                \
    ((struct iovec*)(&var## _iov[0]))->iov_base = (void*)var## _last_addr; \
    *var## _iovs_nout = 1;                                              \
} while (0)

#define MPIDI_OFI_UPDATE_SEG(var,VAR)                                   \
do {                                                                    \
    var## _idx++;                                                       \
    ((struct iovec*)(&var## _iov[var## _idx]))->iov_base = (void *)var## _addr; \
    ((struct iovec*)(&var## _iov[var## _idx]))->iov_len  = len;         \
    (*var## _iovs_nout)++;                                              \
} while (0)

#define MPIDI_OFI_UPDATE_SEG_STATE1(var1,var2)                          \
do {                                                                    \
    if (*var2## _iovs_nout>=var2## _max_iovs) return MPIDI_OFI_SEG_EAGAIN; \
    ((struct iovec*)(&var1## _iov[var1## _idx]))->iov_len += len;       \
    var2## _idx++;                                                      \
    ((struct iovec*)(&var2## _iov[var2## _idx]))->iov_base = (void *)var2## _addr; \
    ((struct iovec*)(&var2## _iov[var2## _idx]))->iov_len  = len; \
    (*var2## _iovs_nout)++;                                             \
    MPIDI_OFI_next_seg_state(seg_state,&origin_addr, &target_addr, &len); \
} while (0)

#define MPIDI_OFI_UPDATE_SEG_STATE2(var1,var2,var3)                          \
do {                                                                         \
    if (*var2## _iovs_nout>=var2## _max_iovs) return MPIDI_OFI_SEG_EAGAIN;   \
    if (*var3## _iovs_nout>=var3## _max_iovs) return MPIDI_OFI_SEG_EAGAIN;   \
    ((struct iovec*)(&var1## _iov[var1## _idx]))->iov_len += len;            \
    var2## _idx++;                                                      \
    var3## _idx++;                                                      \
    ((struct iovec*)(&var2## _iov[var2## _idx]))->iov_base = (void *)var2## _addr; \
    ((struct iovec*)(&var2## _iov[var2## _idx]))->iov_len  = len; \
    ((struct iovec*)(&var3## _iov[var3## _idx]))->iov_base = (void *)var3## _addr; \
    ((struct iovec*)(&var3## _iov[var3## _idx]))->iov_len  = len; \
    (*var2## _iovs_nout)++;                                             \
    (*var3## _iovs_nout)++;                                             \
    MPIDI_OFI_next_seg_state2(seg_state,&origin_addr, &result_addr, &target_addr,&len); \
  } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_segment_next(MPIDI_OFI_seg_state_t * state,
                                                    DLOOP_VECTOR * out_vector,
                                                    MPIDI_OFI_segment_side_t side)
{
    DLOOP_VECTOR dloop;
    DLOOP_Offset last;
    MPIR_Segment *seg;
    size_t *cursor;
    int num_contig = 1;

    switch (side) {
        case MPIDI_OFI_SEGMENT_ORIGIN:
            last = state->origin_end;
            seg = &state->origin_seg;
            cursor = &state->origin_cursor;
            break;
        case MPIDI_OFI_SEGMENT_TARGET:
            last = state->target_end;
            seg = &state->target_seg;
            cursor = &state->target_cursor;
            break;
        case MPIDI_OFI_SEGMENT_RESULT:
            last = state->result_end;
            seg = &state->result_seg;
            cursor = &state->result_cursor;
            break;
        default:
            MPIR_Assert(0);
            break;
    }
    if (*cursor >= last)
        return 1;
    /* Pack datatype into iovec during runtime. Everytime only one vector is processed,
     * and we try to pack as much as possible using last byte of datatype.
     * If pack is complete, num_contig returns as 0. */
    MPIR_Segment_pack_vector(seg, *cursor, &last, &dloop, &num_contig);
    MPIR_Assert(num_contig <= 1);
    *cursor = last;
    *out_vector = dloop;
    return num_contig == 0 ? 1 : 0;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_init_seg_state(MPIDI_OFI_seg_state_t * seg_state,
                                                       const void *origin,
                                                       const MPI_Aint target,
                                                       size_t origin_count,
                                                       size_t target_count,
                                                       size_t buf_limit,
                                                       MPI_Datatype origin_type,
                                                       MPI_Datatype target_type)
{
    /* `seg_state->buflimit` is `DLOOP_Count` == `MPI_Aint` (typically, long or other signed types)
     * and its maximum value is likely to be smaller than that of `buf_limit` of `size_t`.
     * So round down to the maximum of MPI_Aint if necessary.
     * For instance, as of libfabric 1.6.2, sockets provider has (SIZE_MAX-4K) as buf_limit. */
    CH4_COMPILE_TIME_ASSERT(sizeof(seg_state->buf_limit) == sizeof(MPI_Aint));
    if (likely(buf_limit > MPIR_AINT_MAX))
        buf_limit = MPIR_AINT_MAX;
    seg_state->buf_limit = buf_limit;
    seg_state->buf_limit_left = buf_limit;

    seg_state->origin_cursor = 0;
    MPIDI_Datatype_check_size(origin_type, origin_count, seg_state->origin_end);
    MPIR_Segment_init(origin, origin_count, origin_type, &seg_state->origin_seg);

    seg_state->target_cursor = 0;
    MPIDI_Datatype_check_size(target_type, target_count, seg_state->target_end);
    MPIR_Segment_init((const void *) target, target_count, target_type, &seg_state->target_seg);

    MPIDI_OFI_INIT_SEG_STATE(target, TARGET);
    MPIDI_OFI_INIT_SEG_STATE(origin, ORIGIN);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_init_seg_state2(MPIDI_OFI_seg_state_t * seg_state,
                                                        const void *origin,
                                                        const void *result,
                                                        const MPI_Aint target,
                                                        size_t origin_count,
                                                        size_t result_count,
                                                        size_t target_count,
                                                        size_t buf_limit,
                                                        MPI_Datatype origin_type,
                                                        MPI_Datatype result_type,
                                                        MPI_Datatype target_type)
{
    /* `seg_state->buflimit` is `DLOOP_Count` == `MPI_Aint` (typically, long or other signed types)
     * and its maximum value is likely to be smaller than that of `buf_limit` of `size_t`.
     * So round down to the maximum of MPI_Aint if necessary.
     * For instance, as of libfabric 1.6.2, sockets provider has (SIZE_MAX-4K) as buf_limit. */
    CH4_COMPILE_TIME_ASSERT(sizeof(seg_state->buf_limit) == sizeof(MPI_Aint));
    if (likely(buf_limit > MPIR_AINT_MAX))
        buf_limit = MPIR_AINT_MAX;
    seg_state->buf_limit = buf_limit;
    seg_state->buf_limit_left = buf_limit;

    seg_state->origin_cursor = 0;
    MPIDI_Datatype_check_size(origin_type, origin_count, seg_state->origin_end);
    MPIR_Segment_init(origin, origin_count, origin_type, &seg_state->origin_seg);

    seg_state->target_cursor = 0;
    MPIDI_Datatype_check_size(target_type, target_count, seg_state->target_end);
    MPIR_Segment_init((const void *) target, target_count, target_type, &seg_state->target_seg);

    seg_state->result_cursor = 0;
    MPIDI_Datatype_check_size(result_type, result_count, seg_state->result_end);
    MPIR_Segment_init(result, result_count, result_type, &seg_state->result_seg);

    MPIDI_OFI_INIT_SEG_STATE(target, TARGET);
    MPIDI_OFI_INIT_SEG_STATE(origin, ORIGIN);
    MPIDI_OFI_INIT_SEG_STATE(result, RESULT);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_next_seg_state(MPIDI_OFI_seg_state_t * seg_state,
                                                      uintptr_t * origin_addr_next,
                                                      uintptr_t * target_addr_next,
                                                      size_t * buf_len)
{
    if ((seg_state->origin_iov_len != 0) && (seg_state->target_iov_len != 0)) {
        uintptr_t buf_size = MPL_MIN(MPL_MIN(seg_state->target_iov_len, seg_state->origin_iov_len),
                                     seg_state->buf_limit_left);
        *buf_len = buf_size;
        MPIDI_OFI_NEXT_SEG_STATE(target, TARGET);
        MPIDI_OFI_NEXT_SEG_STATE(origin, ORIGIN);
        return MPIDI_OFI_SEG_EAGAIN;
    } else {
        if (((seg_state->origin_iov_len != 0) || (seg_state->target_iov_len != 0)))
            return MPIDI_OFI_SEG_ERROR;
        return MPIDI_OFI_SEG_DONE;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_next_seg_state2(MPIDI_OFI_seg_state_t * seg_state,
                                                       uintptr_t * origin_addr_next,
                                                       uintptr_t * result_addr_next,
                                                       uintptr_t * target_addr_next,
                                                       size_t * buf_len)
{
    if ((seg_state->origin_iov_len != 0) && (seg_state->target_iov_len != 0) &&
        (seg_state->result_iov_len != 0)) {
        uintptr_t buf_size =
            MPL_MIN(MPL_MIN(MPL_MIN(seg_state->target_iov_len, seg_state->origin_iov_len),
                            seg_state->result_iov_len), seg_state->buf_limit_left);
        *buf_len = buf_size;
        MPIDI_OFI_NEXT_SEG_STATE(target, TARGET);
        MPIDI_OFI_NEXT_SEG_STATE(origin, ORIGIN);
        MPIDI_OFI_NEXT_SEG_STATE(result, RESULT);
        return MPIDI_OFI_SEG_EAGAIN;
    } else {
        if (((seg_state->origin_iov_len != 0) ||
             (seg_state->target_iov_len != 0) || (seg_state->result_iov_len != 0)))
            return MPIDI_OFI_SEG_ERROR;

        return MPIDI_OFI_SEG_DONE;
    }
}

/*
 *  Update the length of data that can fit into an iovec with current buffer limit left.
 *  (Used for functions taking two buffers -- origin, target)
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_peek_seg_state(MPIDI_OFI_seg_state_t * seg_state,
                                                      uintptr_t * next_origin_addr,
                                                      uintptr_t * next_target_addr,
                                                      size_t * buf_len)
{
    if ((seg_state->origin_iov_len != 0) && (seg_state->target_iov_len != 0)) {
        *next_origin_addr = seg_state->origin_addr;
        *next_target_addr = seg_state->target_addr;
        *buf_len =
            MPL_MIN(MPL_MIN(seg_state->target_iov_len,
                            seg_state->origin_iov_len), seg_state->buf_limit_left);
        return MPIDI_OFI_SEG_EAGAIN;
    } else {
        if (((seg_state->origin_iov_len != 0) || (seg_state->target_iov_len != 0)))
            return MPIDI_OFI_SEG_ERROR;

        return MPIDI_OFI_SEG_DONE;
    }
}

/*
 *  Update the length of data that can fit into an iovec with current buffer limit left.
 *  (Used for functions taking three buffers -- origin, target, result)
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_peek_seg_state2(MPIDI_OFI_seg_state_t * seg_state,
                                                       uintptr_t * next_origin_addr,
                                                       uintptr_t * next_result_addr,
                                                       uintptr_t * next_target_addr,
                                                       size_t * buf_len)
{
    if ((seg_state->origin_iov_len != 0) && (seg_state->target_iov_len != 0) &&
        (seg_state->result_iov_len != 0)) {
        *next_origin_addr = seg_state->origin_addr;
        *next_result_addr = seg_state->result_addr;
        *next_target_addr = seg_state->target_addr;
        *buf_len = MPL_MIN(MPL_MIN(MPL_MIN(seg_state->target_iov_len, seg_state->origin_iov_len),
                                   seg_state->result_iov_len), seg_state->buf_limit_left);
        return MPIDI_OFI_SEG_EAGAIN;
    } else {
        if (((seg_state->origin_iov_len != 0) ||
             (seg_state->target_iov_len != 0) || (seg_state->result_iov_len != 0)))
            return MPIDI_OFI_SEG_ERROR;

        return MPIDI_OFI_SEG_DONE;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_merge_segment(MPIDI_OFI_seg_state_t * seg_state,
                                                     struct iovec *origin_iov,
                                                     size_t origin_max_iovs,
                                                     struct fi_rma_iov *target_iov,
                                                     size_t target_max_iovs,
                                                     size_t * origin_iovs_nout,
                                                     size_t * target_iovs_nout)
{
    int rc;
    uintptr_t origin_addr = (uintptr_t) NULL, target_addr = (uintptr_t) NULL;
    uintptr_t origin_last_addr = 0, target_last_addr = 0;
    int origin_idx = 0, target_idx = 0;
    size_t len = 0, last_len = 0;

    CH4_COMPILE_TIME_ASSERT(offsetof(struct iovec, iov_base) == offsetof(struct fi_rma_iov, addr));
    CH4_COMPILE_TIME_ASSERT(offsetof(struct iovec, iov_len) == offsetof(struct fi_rma_iov, len));

    rc = MPIDI_OFI_next_seg_state(seg_state, &origin_last_addr, &target_last_addr, &last_len);
    MPIR_Assert(rc != MPIDI_OFI_SEG_ERROR);

    MPIDI_OFI_INIT_SEG(target);
    MPIDI_OFI_INIT_SEG(origin);
    seg_state->buf_limit_left -= last_len;

    /* Merge multiple iovecs into a single OFI call,
     * subject to OFI iovec count limit, i.e., origin_max_iovs and target_max_iovs. */
    while (rc > 0) {
        rc = MPIDI_OFI_peek_seg_state(seg_state, &origin_addr, &target_addr, &len);
        MPIR_Assert(rc != MPIDI_OFI_SEG_ERROR);

        if (rc == MPIDI_OFI_SEG_DONE) {
            seg_state->buf_limit_left = seg_state->buf_limit;
            return MPIDI_OFI_SEG_EAGAIN;
        }

        /* IOV count is initialized as 1. Limit check should be done before update.
         * Otherwise check doesn't work correctly for max_iovs = 1. */
        if ((*origin_iovs_nout >= origin_max_iovs) || (*target_iovs_nout >= target_max_iovs)) {
            seg_state->buf_limit_left = seg_state->buf_limit;
            return MPIDI_OFI_SEG_EAGAIN;
        }

        if (target_last_addr + last_len == target_addr) {
            /* target address is contiguous, update a new iovec for origin only. */
            MPIDI_OFI_UPDATE_SEG_STATE1(target, origin);
        } else if (origin_last_addr + last_len == origin_addr) {
            /* origin address is contiguous, update a new iovec for target only. */
            MPIDI_OFI_UPDATE_SEG_STATE1(origin, target);
        } else {
            MPIDI_OFI_UPDATE_SEG(target, TARGET);
            MPIDI_OFI_UPDATE_SEG(origin, ORIGIN);
            MPIDI_OFI_next_seg_state(seg_state, &origin_addr, &target_addr, &len);
        }

        origin_last_addr = origin_addr;
        target_last_addr = target_addr;
        last_len = len;
        seg_state->buf_limit_left -= len;
        if (seg_state->buf_limit_left == 0) {
            seg_state->buf_limit_left = seg_state->buf_limit;
            MPIR_Assert(*origin_iovs_nout <= origin_max_iovs);
            MPIR_Assert(*target_iovs_nout <= target_max_iovs);
            return MPIDI_OFI_SEG_EAGAIN;
        }
    }

    if (rc == MPIDI_OFI_SEG_DONE) {
        return MPIDI_OFI_SEG_DONE;
    } else {
        seg_state->buf_limit_left = seg_state->buf_limit;
        return MPIDI_OFI_SEG_EAGAIN;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_merge_segment2(MPIDI_OFI_seg_state_t * seg_state,
                                                      struct iovec *origin_iov,
                                                      size_t origin_max_iovs,
                                                      struct iovec *result_iov,
                                                      size_t result_max_iovs,
                                                      struct fi_rma_iov *target_iov,
                                                      size_t target_max_iovs,
                                                      size_t * origin_iovs_nout,
                                                      size_t * result_iovs_nout,
                                                      size_t * target_iovs_nout)
{
    int rc;
    uintptr_t origin_addr = (uintptr_t) NULL, result_addr = (uintptr_t) NULL, target_addr =
        (uintptr_t) NULL;
    uintptr_t origin_last_addr = 0, result_last_addr = 0, target_last_addr = 0;
    int origin_idx = 0, result_idx = 0, target_idx = 0;
    size_t len = 0, last_len = 0;

    rc = MPIDI_OFI_next_seg_state2(seg_state, &origin_last_addr, &result_last_addr,
                                   &target_last_addr, &last_len);
    MPIR_Assert(rc != MPIDI_OFI_SEG_ERROR);
    MPIDI_OFI_INIT_SEG(target);
    MPIDI_OFI_INIT_SEG(origin);
    MPIDI_OFI_INIT_SEG(result);
    seg_state->buf_limit_left -= last_len;

    /* Merge multiple iovecs into a single OFI call,
     * subject to OFI iovec count limit, i.e., origin_max_iovs, target_max_iovs and result_max_iovs. */
    while (rc > 0) {
        rc = MPIDI_OFI_peek_seg_state2(seg_state, &origin_addr, &result_addr, &target_addr, &len);
        MPIR_Assert(rc != MPIDI_OFI_SEG_ERROR);

        if (rc == MPIDI_OFI_SEG_DONE) {
            seg_state->buf_limit_left = seg_state->buf_limit;
            return MPIDI_OFI_SEG_EAGAIN;
        }

        /* IOV count is initialized as 1. Limit check should be done before update.
         * Otherwise check doesn't work correctly for max_iovs = 1. */
        if ((*origin_iovs_nout >= origin_max_iovs) ||
            (*target_iovs_nout >= target_max_iovs) || (*result_iovs_nout >= result_max_iovs)) {
            seg_state->buf_limit_left = seg_state->buf_limit;
            return MPIDI_OFI_SEG_EAGAIN;
        }

        if (target_last_addr + last_len == target_addr) {
            MPIDI_OFI_UPDATE_SEG_STATE2(target, origin, result);
        } else if (origin_last_addr + last_len == origin_addr) {
            MPIDI_OFI_UPDATE_SEG_STATE2(origin, target, result);
        } else if (result_last_addr + last_len == result_addr) {
            MPIDI_OFI_UPDATE_SEG_STATE2(result, target, origin);
        } else {
            MPIDI_OFI_UPDATE_SEG(target, TARGET);
            MPIDI_OFI_UPDATE_SEG(origin, ORIGIN);
            MPIDI_OFI_UPDATE_SEG(result, RESULT);
            MPIDI_OFI_next_seg_state2(seg_state, &origin_addr, &result_addr, &target_addr, &len);
        }

        origin_last_addr = origin_addr;
        result_last_addr = result_addr;
        target_last_addr = target_addr;
        last_len = len;
        seg_state->buf_limit_left -= len;
        if (seg_state->buf_limit_left == 0) {
            seg_state->buf_limit_left = seg_state->buf_limit;
            MPIR_Assert(*origin_iovs_nout <= origin_max_iovs);
            MPIR_Assert(*target_iovs_nout <= target_max_iovs);
            MPIR_Assert(*result_iovs_nout <= result_max_iovs);
            return MPIDI_OFI_SEG_EAGAIN;
        }
    }

    if (rc == MPIDI_OFI_SEG_DONE) {
        return MPIDI_OFI_SEG_DONE;
    } else {
        seg_state->buf_limit_left = seg_state->buf_limit;
        return MPIDI_OFI_SEG_EAGAIN;
    }
}

#endif /* OFI_IOVEC_UTIL_H_INCLUDED */
