/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_H_INCLUDED
#define POSIX_EAGER_H_INCLUDED

#include <posix_eager_transaction.h>

#define MPIDI_MAX_POSIX_EAGER_STRING_LEN 64

typedef int (*MPIDI_POSIX_eager_init_t) (int rank, int size);
typedef int (*MPIDI_POSIX_eager_finalize_t) (void);

typedef int (*MPIDI_POSIX_eager_send_t) (int grank, MPIDI_POSIX_am_header_t * msg_hdr,
                                         const void *am_hdr, MPI_Aint am_hdr_sz, const void *buf,
                                         MPI_Aint count, MPI_Datatype datatype, MPI_Aint offset,
                                         int src_vsi, int dst_vsi, MPI_Aint * bytes_sent);

typedef int (*MPIDI_POSIX_eager_recv_begin_t) (int vsi,
                                               MPIDI_POSIX_eager_recv_transaction_t * transaction);

typedef void (*MPIDI_POSIX_eager_recv_memcpy_t) (MPIDI_POSIX_eager_recv_transaction_t * transaction,
                                                 void *dst, const void *src, size_t size);

typedef void (*MPIDI_POSIX_eager_recv_commit_t) (MPIDI_POSIX_eager_recv_transaction_t *
                                                 transaction);

typedef void (*MPIDI_POSIX_eager_recv_posted_hook_t) (int grank);
typedef void (*MPIDI_POSIX_eager_recv_completed_hook_t) (int grank);

typedef size_t(*MPIDI_POSIX_eager_payload_limit_t) (void);
typedef size_t(*MPIDI_POSIX_eager_buf_limit_t) (void);

typedef struct {
    MPIDI_POSIX_eager_init_t init;
    MPIDI_POSIX_eager_finalize_t finalize;

    MPIDI_POSIX_eager_send_t send;

    MPIDI_POSIX_eager_recv_begin_t recv_begin;
    MPIDI_POSIX_eager_recv_memcpy_t recv_memcpy;
    MPIDI_POSIX_eager_recv_commit_t recv_commit;

    MPIDI_POSIX_eager_recv_posted_hook_t recv_posted_hook;
    MPIDI_POSIX_eager_recv_completed_hook_t recv_completed_hook;

    MPIDI_POSIX_eager_payload_limit_t payload_limit;
    MPIDI_POSIX_eager_buf_limit_t buf_limit;
} MPIDI_POSIX_eager_funcs_t;

extern MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_funcs[];
extern MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_func;
extern int MPIDI_num_posix_eager_fabrics;
extern char MPIDI_POSIX_eager_strings[][MPIDI_MAX_POSIX_EAGER_STRING_LEN];

int MPIDI_POSIX_eager_init(int rank, int size);
int MPIDI_POSIX_eager_finalize(void);

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_send(int grank, MPIDI_POSIX_am_header_t * msg_hdr,
                                                    const void *am_hdr, MPI_Aint am_hdr_sz,
                                                    const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, MPI_Aint offset,
                                                    int src_vsi, int dst_vsi,
                                                    MPI_Aint * bytes_sent) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_recv_begin(int vsi,
                                                          MPIDI_POSIX_eager_recv_transaction_t *
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

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_payload_limit(void) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_buf_limit(void) MPL_STATIC_INLINE_SUFFIX;

#endif /* POSIX_EAGER_H_INCLUDED */
