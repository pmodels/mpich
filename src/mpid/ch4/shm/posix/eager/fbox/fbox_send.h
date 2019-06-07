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
#ifndef POSIX_EAGER_FBOX_SEND_H_INCLUDED
#define POSIX_EAGER_FBOX_SEND_H_INCLUDED

#include "fbox_impl.h"

/* This function attempts to send the next chunk of a message via the fastbox. If the fastbox is
 * already full, this function will return and the caller is expected to queue the message for later
 * and retry.
 *
 * grank       - The global rank (the rank in MPI_COMM_WORLD) of the receiving process. Used to look up
 *               the correct fastbox.
 * msg_hdr     - The header of the message to be sent. This can be NULL if there is no header to be sent
 *               (such as if the header was sent in a previous chunk).
 * iov         - The array of iovec entries to be sent.
 * iov_num     - The number of entries in the iovec array.
 */
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_send(int grank,
                       MPIDI_POSIX_am_header_t ** msg_hdr, struct iovec **iov, size_t * iov_num)
{
    MPIDI_POSIX_fastbox_t *fbox_out;
    int i;
    size_t iov_done = 0;
    uint8_t *fbox_payload_ptr;
    size_t fbox_payload_size = MPIDI_POSIX_FBOX_DATA_LEN;
    size_t fbox_payload_size_left = MPIDI_POSIX_FBOX_DATA_LEN;
    int ret = MPIDI_POSIX_OK;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_SEND);

    fbox_out =
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[MPIDI_POSIX_global.local_ranks[grank]];

    /* Check if the fastbox is already full and if so, return to the caller, which will cause this
     * message to be queued. */
    if (fbox_out->data_ready) {
        ret = MPIDI_POSIX_NOK;
        goto fn_exit;
    }

    fbox_payload_ptr = fbox_out->payload;

    fbox_out->is_header = 0;

    /* If there is a header, put that in the fastbox first and account for that in the remaining
     * size. */
    if (*msg_hdr) {
        *((MPIDI_POSIX_am_header_t *) fbox_payload_ptr) = **msg_hdr;

        fbox_payload_ptr += sizeof(MPIDI_POSIX_am_header_t);
        fbox_payload_size_left -= sizeof(MPIDI_POSIX_am_header_t);

        *msg_hdr = NULL;        /* completed header copy */

        fbox_out->is_header = 1;
    }

    /* Copy all of the user data that will fit into the fastbox. */
    for (i = 0; i < *iov_num; i++) {
        if (unlikely(fbox_payload_size_left < (*iov)[i].iov_len)) {
            MPIR_Memcpy(fbox_payload_ptr, (*iov)[i].iov_base, fbox_payload_size_left);

            (*iov)[i].iov_base = (char *) (*iov)[i].iov_base + fbox_payload_size_left;
            (*iov)[i].iov_len -= fbox_payload_size_left;

            fbox_payload_size_left = 0;

            break;
        }

        MPIR_Memcpy(fbox_payload_ptr, (*iov)[i].iov_base, (*iov)[i].iov_len);

        fbox_payload_ptr += (*iov)[i].iov_len;
        fbox_payload_size_left -= (*iov)[i].iov_len;

        iov_done++;
    }

    fbox_out->payload_sz = fbox_payload_size - fbox_payload_size_left;

#ifdef POSIX_FBOX_DEBUG
    {
        int i;

        POSIX_FBOX_TRACE("FBOX_OUT (%s) [", fbox_out->is_header ? "H" : "F");

        for (i = 0; i < fbox_out->payload_sz; i++) {
            POSIX_FBOX_TRACE("%02X", ((uint8_t *) (fbox_out->payload))[i]);

            if (i == 255) {
                POSIX_FBOX_TRACE("...");
                break;
            }
        }

        POSIX_FBOX_TRACE("]@%d to %d\n", fbox_out->payload_sz, grank);
    }
#endif /* POSIX_FBOX_DEBUG */

    OPA_compiler_barrier();

    /* Update the data ready flag to indicate that there is data in the box for the receiver. */
    fbox_out->data_ready = 1;

    /* Update the number of iovecs left and the pointer to the next one. */
    *iov_num -= iov_done;
    if (*iov_num) {
        *iov = &((*iov)[iov_done]);     /* Rewind iov array */
    } else {
        *iov = NULL;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_SEND);
    return ret;
}

#endif /* POSIX_EAGER_FBOX_SEND_H_INCLUDED */
