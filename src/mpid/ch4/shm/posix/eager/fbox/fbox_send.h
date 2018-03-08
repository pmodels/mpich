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

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_send(int grank,
                       MPIDI_POSIX_am_header_t ** msg_hdr,
                       struct iovec **iov, size_t * iov_num, int is_blocking)
{
    int local_rank = MPIDI_POSIX_eager_fbox_control_global.local_ranks[grank];
    MPIDI_POSIX_fastbox_t *fbox_out =
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[local_rank];
    int i;
    size_t iov_done = 0;
    uint32_t spin_count = 8;
    uint8_t *fbox_payload_ptr;
    size_t fbox_payload_size = MPIDI_POSIX_FBOX_DATA_LEN;
    size_t fbox_payload_size_left = MPIDI_POSIX_FBOX_DATA_LEN;

    while (fbox_out->flag && ((--spin_count) || is_blocking)) {
        __asm__ __volatile__("pause":::"memory");
    }

    if (!spin_count && !is_blocking) {
        return MPIDI_POSIX_NOK;
    }

    fbox_payload_ptr = fbox_out->payload;

    fbox_out->is_header = 0;

    if (*msg_hdr) {
        *((MPIDI_POSIX_am_header_t *) fbox_payload_ptr) = **msg_hdr;

        fbox_payload_ptr += sizeof(MPIDI_POSIX_am_header_t);
        fbox_payload_size_left -= sizeof(MPIDI_POSIX_am_header_t);

        *msg_hdr = NULL;        /* completed header copy */

        fbox_out->is_header = 1;
    }

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

    fbox_out->flag = 1;

    *iov_num -= iov_done;

    if (*iov_num) {
        *iov = &((*iov)[iov_done]);     /* Rewind iov array */
    } else {
        *iov = NULL;
    }

    return MPIDI_POSIX_OK;
}

#endif /* POSIX_EAGER_FBOX_SEND_H_INCLUDED */
