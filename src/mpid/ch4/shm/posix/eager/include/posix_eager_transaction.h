/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_TRANSACTION_H_INCLUDED
#define POSIX_EAGER_TRANSACTION_H_INCLUDED

#include "posix_eager_pre.h"

typedef struct MPIDI_POSIX_eager_recv_transaction {

    /* Public */

    void *msg_hdr;

    void *payload;
    size_t payload_sz;          /* 2GB limit */

    int src_local_rank;
    int src_vsi;
    int dst_vsi;

    /* Private */

    union {
    MPIDI_POSIX_EAGER_RECV_TRANSACTION_DECL} transport;

} MPIDI_POSIX_eager_recv_transaction_t;

#endif /* POSIX_EAGER_TRANSACTION_H_INCLUDED */
