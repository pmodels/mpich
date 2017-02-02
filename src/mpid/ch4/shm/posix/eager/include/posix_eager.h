/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_H_INCLUDED
#define POSIX_EAGER_H_INCLUDED

#include <posix_eager_transaction.h>

#define MPIDI_MAX_POSIX_EAGER_STRING_LEN 64

typedef int (*MPIDI_POSIX_eager_init_t) (int rank, int size);
typedef int (*MPIDI_POSIX_eager_finalize_t) (void);

typedef int (*MPIDI_POSIX_eager_send_t) (int grank,
                                         MPIDI_POSIX_am_header_t ** msg_hdr,
                                         struct iovec ** iov, size_t * iov_num, int is_blocking);

typedef int (*MPIDI_POSIX_eager_recv_begin_t) (MPIDI_POSIX_eager_recv_transaction_t * transaction);

typedef void (*MPIDI_POSIX_eager_recv_memcpy_t) (MPIDI_POSIX_eager_recv_transaction_t * transaction,
                                                 void *dst, const void *src, size_t size);

typedef void (*MPIDI_POSIX_eager_recv_commit_t) (MPIDI_POSIX_eager_recv_transaction_t *
                                                 transaction);

typedef void (*MPIDI_POSIX_eager_recv_posted_hook_t) (int grank);
typedef void (*MPIDI_POSIX_eager_recv_completed_hook_t) (int grank);

typedef struct {
    MPIDI_POSIX_eager_init_t init;
    MPIDI_POSIX_eager_finalize_t finalize;

    MPIDI_POSIX_eager_send_t send;

    MPIDI_POSIX_eager_recv_begin_t recv_begin;
    MPIDI_POSIX_eager_recv_memcpy_t recv_memcpy;
    MPIDI_POSIX_eager_recv_commit_t recv_commit;

    MPIDI_POSIX_eager_recv_posted_hook_t recv_posted_hook;
    MPIDI_POSIX_eager_recv_completed_hook_t recv_completed_hook;
} MPIDI_POSIX_eager_funcs_t;

extern MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_funcs[];
extern MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_func;
extern int MPIDI_num_posix_eager_fabrics;
extern char MPIDI_POSIX_eager_strings[][MPIDI_MAX_POSIX_EAGER_STRING_LEN];

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int size) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_finalize(void) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_send(int grank,
                                                    MPIDI_POSIX_am_header_t ** msg_hdr,
                                                    struct iovec **iov,
                                                    size_t * iov_num,
                                                    int is_blocking) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t *
                                                          transaction) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t *
                                                            transaction, void *dst, const void *src,
                                                            size_t size) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t *
                                                            transaction) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
    MPL_STATIC_INLINE_SUFFIX;

#endif /* POSIX_EAGER_H_INCLUDED */
