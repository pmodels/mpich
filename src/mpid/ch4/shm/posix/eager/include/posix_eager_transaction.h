/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_TRANSACTION_H_INCLUDED
#define POSIX_EAGER_TRANSACTION_H_INCLUDED

#include "posix_eager_pre.h"

typedef struct MPIDI_POSIX_eager_recv_transaction {

    /* Public */

    void *msg_hdr;

    void *payload;
    int payload_sz;             /* 2GB limit */

    int src_grank;

    /* Private */

    union {
    MPIDI_POSIX_EAGER_RECV_TRANSACTION_DECL} transport;

} MPIDI_POSIX_eager_recv_transaction_t;

#endif /* POSIX_EAGER_TRANSACTION_H_INCLUDED */
