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
#ifndef SHM_SHMAM_EAGER_FBOX_RECV_H_INCLUDED
#define SHM_SHMAM_EAGER_FBOX_RECV_H_INCLUDED

#include "fbox_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_SHMAM_eager_recv_begin
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHMAM_eager_recv_begin(MPIDI_SHMAM_eager_recv_transaction_t * transaction)
{
    int grank;

    MPIDI_SHMAM_fastbox_t *fbox_in = NULL;

    grank = MPIDI_SHMAM_eager_fbox_control_global.last_polled_grank++ %
        MPIDI_SHMAM_eager_fbox_control_global.num_local;

    fbox_in = MPIDI_SHMAM_eager_fbox_control_global.mailboxes.in[grank];

    if (fbox_in->flag) {
        /* Initialize public transaction part */

        if (likely(fbox_in->is_header)) {
            transaction->msg_hdr = fbox_in->payload;
            transaction->payload = fbox_in->payload + sizeof(MPIDI_SHM_am_header_t);
            transaction->payload_sz = fbox_in->payload_sz - sizeof(MPIDI_SHM_am_header_t);
        }
        else {
            /* We have received a message fragment */

            transaction->msg_hdr = NULL;
            transaction->payload = fbox_in->payload;
            transaction->payload_sz = fbox_in->payload_sz;
        }

        transaction->src_grank = grank;

        /* Initialize private transaction part */

        transaction->transport.fbox.fbox_ptr = fbox_in;

#ifdef SHM_AM_FBOX_DEBUG
        {
            fprintf(MPIDI_SHM_Shmam_global.logfile, "FBOX_IN  (%s) [",
                    fbox_in->is_header ? "H" : "F");

            int i;

            for (i = 0; i < fbox_in->payload_sz; i++) {
                fprintf(MPIDI_SHM_Shmam_global.logfile, "%02X",
                        ((uint8_t *) (fbox_in->payload))[i]);

                if (i == 255) {
                    fprintf(MPIDI_SHM_Shmam_global.logfile, "...");
                    break;
                }
            }

            fprintf(MPIDI_SHM_Shmam_global.logfile, "]@%d from %d\n", fbox_in->payload_sz, grank);
            fflush(MPIDI_SHM_Shmam_global.logfile);
        }
#endif /* SHM_AM_DEBUG */

        return MPIDI_SHMAM_OK;
    }

    return MPIDI_SHMAM_NOK;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHMAM_eager_recv_memcpy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void
MPIDI_SHMAM_eager_recv_memcpy(MPIDI_SHMAM_eager_recv_transaction_t * transaction,
                              void *dst, const void *src, size_t size)
{
    MPIR_Memcpy(dst, src, size);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHMAM_eager_recv_commit
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void
MPIDI_SHMAM_eager_recv_commit(MPIDI_SHMAM_eager_recv_transaction_t * transaction)
{
    MPIDI_SHMAM_fastbox_t *fbox_in = NULL;

    fbox_in = (MPIDI_SHMAM_fastbox_t *) transaction->transport.fbox.fbox_ptr;

    fbox_in->flag = 0;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_posted_hook(int grank)
{
    MPIR_Assert(0);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_recv_completed_hook(int grank)
{
    MPIR_Assert(0);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_anysource_posted_hook()
{
    MPIR_Assert(0);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHMAM_eager_anysource_completed_hook()
{
    MPIR_Assert(0);
}

#endif /* SHM_SHMAM_EAGER_FBOX_RECV_H_INCLUDED */
