/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_FBOX_RECV_H_INCLUDED
#define POSIX_EAGER_FBOX_RECV_H_INCLUDED

#include "fbox_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_recv_begin
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_recv_begin(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_BEGIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_BEGIN);

    enum { BATCH_SIZE = 4 };
    int j, local_rank, grank;
    MPIDI_POSIX_fastbox_t *fbox_in;
    int mpi_errno = MPIDI_POSIX_NOK;

    for (j = 0; j < BATCH_SIZE; j++) {
        local_rank = MPIDI_POSIX_eager_fbox_control_global.next_poll_local_rank;
        fbox_in = MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[local_rank];

        MPIDI_POSIX_eager_fbox_control_global.next_poll_local_rank =
            (local_rank + 1) % MPIDI_POSIX_eager_fbox_control_global.num_local;

        if (fbox_in->flag) {
            /* Initialize public transaction part */

            grank = MPIDI_POSIX_eager_fbox_control_global.local_procs[local_rank];

            if (likely(fbox_in->is_header)) {
                transaction->msg_hdr = fbox_in->payload;
                transaction->payload = fbox_in->payload + sizeof(MPIDI_POSIX_am_header_t);
                transaction->payload_sz = fbox_in->payload_sz - sizeof(MPIDI_POSIX_am_header_t);
            } else {
                /* We have received a message fragment */

                transaction->msg_hdr = NULL;
                transaction->payload = fbox_in->payload;
                transaction->payload_sz = fbox_in->payload_sz;
            }

            transaction->src_grank = grank;

            /* Initialize private transaction part */

            transaction->transport.fbox.fbox_ptr = fbox_in;

#ifdef POSIX_FBOX_DEBUG
            {
                int i;

                POSIX_FBOX_TRACE("FBOX_IN  (%s) [", fbox_in->is_header ? "H" : "F");

                for (i = 0; i < fbox_in->payload_sz; i++) {
                    POSIX_FBOX_TRACE("%02X", ((uint8_t *) (fbox_in->payload))[i]);

                    if (i == 255) {
                        POSIX_FBOX_TRACE("...");
                        break;
                    }
                }

                POSIX_FBOX_TRACE("]@%d from %d\n", fbox_in->payload_sz, grank);
            }
#endif /* POSIX_FBOX_DEBUG */

            mpi_errno = MPIDI_POSIX_OK;
            goto fn_exit;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_BEGIN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_recv_memcpy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_memcpy(MPIDI_POSIX_eager_recv_transaction_t * transaction,
                              void *dst, const void *src, size_t size)
{
    MPIR_Memcpy(dst, src, size);

    return;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_recv_commit
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_recv_commit(MPIDI_POSIX_eager_recv_transaction_t * transaction)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMMIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMMIT);

    MPIDI_POSIX_fastbox_t *fbox_in = NULL;

    fbox_in = (MPIDI_POSIX_fastbox_t *) transaction->transport.fbox.fbox_ptr;

    fbox_in->flag = 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMMIT);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_POSTED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_POSTED_HOOK);

    int local_rank;

    if (grank >= 0) {
        local_rank = MPIDI_POSIX_eager_fbox_control_global.local_ranks[grank];
        MPIDI_POSIX_eager_fbox_control_global.next_poll_local_rank = local_rank;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_POSTED_HOOK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMPLETED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMPLETED_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMPLETED_HOOK);
}

#endif /* POSIX_EAGER_FBOX_RECV_H_INCLUDED */
