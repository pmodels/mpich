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

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_POSIX_EAGER_FBOX_BATCH_SIZE
      category    : CH4
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The number of fastboxes to poll during one iteration of the progress loop.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_recv_begin(int *src_grank, MPIDI_POSIX_am_header_t ** msg_hdr, void **payload,
                             size_t * payload_sz)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_BEGIN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_BEGIN);

    int j, local_rank, grank;
    MPIDI_POSIX_fastbox_t *fbox_in;
    MPIDI_POSIX_eager_fbox_transport_t *transport;
    int mpi_errno = MPIDI_POSIX_NOK;

    transport = MPIDI_POSIX_eager_fbox_get_transport();

    /* Rather than polling all of the fastboxes on every loop, do a small number and rely on calling
     * this function again if needed. */
    for (j = 0; j < MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE +
         MPIR_CVAR_CH4_POSIX_EAGER_FBOX_BATCH_SIZE; j++) {

        /* Before polling *all* of the fastboxes, poll the ones that are most likely to have messages
         * (where receives have been preposted). */
        if (j < MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE) {
            /* Get the next fastbox to poll. */
            local_rank = transport->first_poll_local_ranks[j];
        }
        /* If we have finished the cached fastboxes, continue polling the rest of the fastboxes
         * where we left off last time. */
        else {
            int16_t last_cache =
                transport->first_poll_local_ranks[MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE];
            /* Figure out the next fastbox to poll by incrementing the counter. */
            last_cache = (last_cache + 1) % (int16_t) MPIDI_POSIX_global.num_local;
            local_rank = last_cache;
            transport->first_poll_local_ranks
                [MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE] = last_cache;
        }

        if (local_rank == -1) {
            continue;
        }

        /* Find the correct fastbox and update the pointer for the next time around the loop. */
        fbox_in = transport->mailboxes.in[local_rank];

        /* If the data ready flag is set, there is a message waiting. */
        if (MPL_atomic_load_int(&fbox_in->data_ready)) {
            /* Initialize public transaction part */
            grank = MPIDI_POSIX_global.local_procs[local_rank];

            if (likely(fbox_in->is_header)) {
                /* Only received the header for the message */
                *msg_hdr = fbox_in->payload;
                *payload = fbox_in->payload + sizeof(MPIDI_POSIX_am_header_t);
                *payload_sz = fbox_in->payload_sz - sizeof(MPIDI_POSIX_am_header_t);
            } else {
                /* Received a fragment of the message payload */
                *msg_hdr = NULL;
                *payload = fbox_in->payload;
                *payload_sz = fbox_in->payload_sz;
            }

            *src_grank = grank;

            /* Initialize private transaction part */
            transport->curr_fbox = fbox_in;

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

            /* We found a message so return success and stop. */
            mpi_errno = MPIDI_POSIX_OK;
            goto fn_exit;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_BEGIN);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_memcpy(void *dst, const void *src, size_t size)
{
    MPIR_Memcpy(dst, src, size);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_end(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_END);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_END);

    MPIDI_POSIX_fastbox_t *fbox_in = NULL;
    MPIDI_POSIX_eager_fbox_transport_t *transport = MPIDI_POSIX_eager_fbox_get_transport();

    fbox_in = (MPIDI_POSIX_fastbox_t *) transport->curr_fbox;

    MPL_atomic_store_int(&fbox_in->data_ready, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_END);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_posted_hook(int grank)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_POSTED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_POSTED_HOOK);

    int local_rank, i;
    MPIDI_POSIX_eager_fbox_transport_t *transport = MPIDI_POSIX_eager_fbox_get_transport();

    if (grank >= 0) {
        local_rank = MPIDI_POSIX_global.local_ranks[grank];

        /* Put the posted receive in the list of fastboxes to be polled first. If the list is full,
         * it will get polled after the boxes in the list are polled, which will be slower, but will
         * still match the message. */
        for (i = 0; i < MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE; i++) {
            if (transport->first_poll_local_ranks[i] == -1) {
                transport->first_poll_local_ranks[i] = local_rank;
                break;
            } else if (transport->first_poll_local_ranks[i] == local_rank) {
                break;
            } else {
                continue;
            }
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_POSTED_HOOK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_eager_recv_completed_hook(int grank)
{
    int i, local_rank;
    MPIDI_POSIX_eager_fbox_transport_t *transport;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMPLETED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMPLETED_HOOK);

    transport = MPIDI_POSIX_eager_fbox_get_transport();

    if (grank >= 0) {
        local_rank = MPIDI_POSIX_global.local_ranks[grank];

        /* Remove the posted receive from the list of fastboxes to be polled first now that the
         * request is done. */
        for (i = 0; i < MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE; i++) {
            if (transport->first_poll_local_ranks[i] == local_rank) {
                transport->first_poll_local_ranks[i] = -1;
                break;
            } else {
                continue;
            }
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_EAGER_RECV_COMPLETED_HOOK);
}

#endif /* POSIX_EAGER_FBOX_RECV_H_INCLUDED */
