/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_SHMAM_EAGER_TRANSACTION_H_INCLUDED
#define SHM_SHMAM_EAGER_TRANSACTION_H_INCLUDED

#include "shmam_eager_pre.h"

typedef struct MPIDI_SHMAM_eager_recv_transaction {

    /* Public */

    void *msg_hdr;

    void *payload;
    int payload_sz;             /* 2GB limit */

    int src_grank;

    /* Private */

    union {
    MPIDI_SHMAM_EAGER_RECV_TRANSACTION_DECL} transport;

} MPIDI_SHMAM_eager_recv_transaction_t;

#endif /* SHM_SHMAM_EAGER_TRANSACTION_H_INCLUDED */
