/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_SEND_H_INCLUDED
#define POSIX_EAGER_IQUEUE_SEND_H_INCLUDED

#include "iqueue_impl.h"

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_eager_iqueue_cell_t
    * MPIDI_POSIX_eager_iqueue_new_cell(MPIDI_POSIX_eager_iqueue_transport_t * transport)
{
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_IQUEUE_NEW_CELL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_IQUEUE_NEW_CELL);

    for (i = 0; i < transport->num_cells; i++) {
        MPIDI_POSIX_eager_iqueue_cell_t *cell = MPIDI_POSIX_EAGER_IQUEUE_THIS_CELL(transport, i);
        if (cell->type == MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL) {
            return cell;
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_IQUEUE_NEW_CELL);
    return NULL;
}

/* This function attempts to send the next chunk of a message via the queue. If no cells are
 * available, this function will return and the caller is expected to queue the message for later
 * and retry.
 *
 * grank   - The global rank (the rank in MPI_COMM_WORLD) of the receiving process.
 * msg_hdr - The header of the message to be sent. This can be NULL if there is no header to be sent
 *           (such as if the header was sent in a previous chunk).
 * iov     - The array of iovec entries to be sent.
 * iov_num - The number of entries in the iovec array.
 */
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_send(int grank,
                       MPIDI_POSIX_am_header_t ** msg_hdr, struct iovec **iov, size_t * iov_num)
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    MPIDI_POSIX_eager_iqueue_cell_t *cell;
    MPIDI_POSIX_eager_iqueue_terminal_t *terminal;
    size_t i, iov_done, capacity, available;
    char *payload;
    int ret = MPIDI_POSIX_OK;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_SEND);

    /* Get the transport object that holds all of the global variables. */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    /* TODO: This function can only send one cell at a time. For a large
     * message that require multiple cells this function has to be invoked
     * multiple times. Can we try to send all data in this call? Moreover,
     * we have to traverse the transport to find an empty cell, can we find
     * all needed cells in single traversal? When putting them into the terminal,
     * can we put all in single insertion? */

    /* Try to get a new cell to hold the message */
    cell = MPIDI_POSIX_eager_iqueue_new_cell(transport);

    /* If a cell wasn't available, let the caller know that we weren't able to send the message
     * immediately. */
    if (unlikely(!cell)) {
        ret = MPIDI_POSIX_NOK;
        goto fn_exit;
    }

    /* Find the correct queue for this rank pair. */
    terminal = &transport->terminals[MPIDI_POSIX_global.local_ranks[grank]];

    /* Get the offset of the cell in the queue */
    void *handle;
    handle = (void *) MPIDI_POSIX_EAGER_IQUEUE_GET_HANDLE(transport, cell);

    /* Get the memory allocated to be used for the message transportation. */
    payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell);

    /* Figure out the capacity of each cell */
    capacity = MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport);
    available = capacity;

    /* If this is the beginning of the message, mark it as the head. Otherwise it will be the
     * tail. */
    if (*msg_hdr) {
        cell->am_header = **msg_hdr;
        *msg_hdr = NULL;        /* completed header copy */
        cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR;
    } else {
        cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA;
    }

    /* Pack the data into the cells */
    iov_done = 0;
    for (i = 0; i < *iov_num; i++) {
        /* Optimize for the case where the message will fit into the cell. */
        if (likely(available >= (*iov)[i].iov_len)) {
            MPIR_Memcpy(payload, (*iov)[i].iov_base, (*iov)[i].iov_len);

            payload += (*iov)[i].iov_len;
            available -= (*iov)[i].iov_len;

            iov_done++;
        } else {
            /* If the message won't fit, put as much as we can and update the iovec for the next
             * time around. */
            MPIR_Memcpy(payload, (*iov)[i].iov_base, available);

            (*iov)[i].iov_base = (char *) (*iov)[i].iov_base + available;
            (*iov)[i].iov_len -= available;

            available = 0;

            break;
        }
    }

    cell->payload_size = capacity - available;

    /* Move the flag to indicate the head. */
    /* The condition swaps the head of the terminal with the current handle if the previous head
     * has now been consumed. Continues until we swap out the prev pointer. */
    void *prev;
    do {
        prev = MPL_atomic_load_ptr(&terminal->head);
        cell->prev = (uintptr_t) prev;
        MPL_atomic_compiler_barrier();
    } while (MPL_atomic_cas_ptr(&terminal->head, prev, handle) != prev);

    /* Update the user counter for number of iovecs left */
    *iov_num -= iov_done;

    /* Check to see if we finished all of the iovecs that came from the caller. If not, update
     * the iov pointer. If so, set it to NULL. Either way, the caller will know the status of
     * the operation from the value of iov. */
    if (*iov_num) {
        *iov = &((*iov)[iov_done]);
    } else {
        *iov = NULL;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_SEND);
    return ret;
}

#endif /* POSIX_EAGER_IQUEUE_SEND_H_INCLUDED */
