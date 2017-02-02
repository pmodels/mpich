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
#ifndef POSIX_EAGER_IQUEUE_SEND_H_INCLUDED
#define POSIX_EAGER_IQUEUE_SEND_H_INCLUDED

#include "iqueue_impl.h"

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_EAGER_IQUEUE_cell_t
    * MPIDI_POSIX_EAGER_IQUEUE_new_cell(MPIDI_POSIX_EAGER_IQUEUE_transport_t * transport)
{
    int i;
    for (i = 0; i < transport->num_cells; i++) {
        MPIDI_POSIX_EAGER_IQUEUE_cell_t *cell = MPIDI_POSIX_EAGER_IQUEUE_THIS_CELL(transport, i);
        if (cell->type == MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL) {
            return cell;
        }
    }
    return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_eager_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_send(int grank,
                       MPIDI_POSIX_am_header_t ** msg_hdr,
                       struct iovec **iov, size_t * iov_num, int is_blocking)
{
    MPIDI_POSIX_EAGER_IQUEUE_transport_t *transport;
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *cell;

    transport = MPIDI_POSIX_EAGER_IQUEUE_get_transport();
    cell = MPIDI_POSIX_EAGER_IQUEUE_new_cell(transport);

    if (likely(cell)) {
        MPIDI_POSIX_EAGER_IQUEUE_terminal_t *terminal;
        size_t i, iov_done, capacity, available;
        uintptr_t prev, handle;
        char *payload;

        terminal = &transport->terminals[transport->local_ranks[grank]];
        handle = MPIDI_POSIX_EAGER_IQUEUE_GET_HANDLE(transport, cell);
        payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell);
        capacity = MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport);
        available = capacity;

        if (*msg_hdr) {
            cell->am_header = **msg_hdr;
            *msg_hdr = NULL;    /* completed header copy */
            cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HEAD;
        } else {
            cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_TAIL;
        }

        iov_done = 0;
        for (i = 0; i < *iov_num; i++) {
            if (unlikely(available < (*iov)[i].iov_len)) {
                memcpy(payload, (*iov)[i].iov_base, available);

                (*iov)[i].iov_base = (char *) (*iov)[i].iov_base + available;
                (*iov)[i].iov_len -= available;

                available = 0;

                break;
            }

            memcpy(payload, (*iov)[i].iov_base, (*iov)[i].iov_len);

            payload += (*iov)[i].iov_len;
            available -= (*iov)[i].iov_len;

            iov_done++;
        }

        cell->payload_size = capacity - available;

        do {
            prev = terminal->head;
            cell->prev = prev;
            OPA_compiler_barrier();
        } while (((uintptr_t) (unsigned int *)
                  OPA_cas_ptr((OPA_ptr_t *) & terminal->head, (void *) prev,
                              (void *) handle) != prev));

        *iov_num -= iov_done;

        if (*iov_num) {
            *iov = &((*iov)[iov_done]); /* Rewind iov array */
        } else {
            *iov = NULL;
        }

        return MPIDI_POSIX_OK;
    }

    return MPIDI_POSIX_NOK;
}

#endif /* POSIX_EAGER_IQUEUE_SEND_H_INCLUDED */
