/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_SHMAM_EAGER_H_INCLUDED
#define SHM_SHMAM_EAGER_H_INCLUDED

#include <shmam_eager_transaction.h>

#define MPIDI_MAX_SHMAM_EAGER_STRING_LEN 64

typedef int (*MPIDI_SHMAM_eager_init_t) (int rank, int grank, int num_local);
typedef int (*MPIDI_SHMAM_eager_finalize_t) (void);

typedef size_t(*MPIDI_SHMAM_eager_threshold_t) (void);

typedef int (*MPIDI_SHMAM_eager_connect_t) (int grank);
typedef int (*MPIDI_SHMAM_eager_listen_t) (int *grank);
typedef int (*MPIDI_SHMAM_eager_accept_t) (int grank);

typedef int (*MPIDI_SHMAM_eager_send_t) (int grank,
                                         MPIDI_SHM_am_header_t ** msg_hdr,
                                         struct iovec ** iov, size_t * iov_num, int is_blocking);

typedef int (*MPIDI_SHMAM_eager_recv_begin_t) (MPIDI_SHMAM_eager_recv_transaction_t * transaction);

typedef void (*MPIDI_SHMAM_eager_recv_memcpy_t) (MPIDI_SHMAM_eager_recv_transaction_t * transaction,
                                                 void *dst, const void *src, size_t size);

typedef void (*MPIDI_SHMAM_eager_recv_commit_t) (MPIDI_SHMAM_eager_recv_transaction_t *
                                                 transaction);

typedef void (*MPIDI_SHMAM_eager_recv_posted_hook_t) (int grank);
typedef void (*MPIDI_SHMAM_eager_recv_completed_hook_t) (int grank);

typedef void (*MPIDI_SHMAM_eager_anysource_posted_hook_t) (void);
typedef void (*MPIDI_SHMAM_eager_anysource_completed_hook_t) (void);

typedef struct MPIDI_SHMAM_eager_funcs {
    MPIDI_SHMAM_eager_init_t init;
    MPIDI_SHMAM_eager_finalize_t finalize;

    MPIDI_SHMAM_eager_threshold_t threshold;

    MPIDI_SHMAM_eager_connect_t connect;
    MPIDI_SHMAM_eager_listen_t listen;
    MPIDI_SHMAM_eager_accept_t accept;

    MPIDI_SHMAM_eager_send_t send;

    MPIDI_SHMAM_eager_recv_begin_t recv_begin;
    MPIDI_SHMAM_eager_recv_memcpy_t recv_memcpy;
    MPIDI_SHMAM_eager_recv_commit_t recv_commit;

    MPIDI_SHMAM_eager_recv_posted_hook_t recv_posted_hook;
    MPIDI_SHMAM_eager_recv_completed_hook_t recv_completed_hook;
    MPIDI_SHMAM_eager_anysource_posted_hook_t anysource_posted_hook;
    MPIDI_SHMAM_eager_anysource_completed_hook_t anysource_completed_hook;
} MPIDI_SHMAM_eager_funcs_t;

extern MPIDI_SHMAM_eager_funcs_t *MPIDI_SHMAM_eager_funcs[];
extern MPIDI_SHMAM_eager_funcs_t *MPIDI_SHMAM_eager_func;
extern int MPIDI_num_shmam_eager_fabrics;
extern char MPIDI_SHMAM_eager_strings[][MPIDI_MAX_SHMAM_EAGER_STRING_LEN];

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_init(int rank, int grank,
                                                    int num_local) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_finalize() MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHMAM_eager_threshold(void) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_connect(int grank) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_listen(int *grank) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_accept(int grank) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_send(int grank,
                                                    MPIDI_SHM_am_header_t ** msg_hdr,
                                                    struct iovec **iov,
                                                    size_t * iov_num,
                                                    int is_blocking) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_recv_begin(MPIDI_SHMAM_eager_recv_transaction_t *
                                                          transaction) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_memcpy(MPIDI_SHMAM_eager_recv_transaction_t *
                                                            transaction, void *dst, const void *src,
                                                            size_t size) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_commit(MPIDI_SHMAM_eager_recv_transaction_t *
                                                            transaction) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_posted_hook(int grank)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_completed_hook(int grank)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_anysource_posted_hook(void)
    MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_anysource_completed_hook(void)
    MPL_STATIC_INLINE_SUFFIX;

#endif /* SHM_SHMAM_EAGER_H_INCLUDED */
