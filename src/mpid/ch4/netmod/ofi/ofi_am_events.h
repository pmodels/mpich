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
#ifndef OFI_AM_EVENTS_H_INCLUDED
#define OFI_AM_EVENTS_H_INCLUDED

#include "ofi_am_impl.h"

MPL_STATIC_INLINE_PREFIX uint16_t MPIDI_OFI_am_get_next_recv_seqno(fi_addr_t addr)
{
    uint64_t id = addr;
    void *r;

    r = MPIDIU_map_lookup(MPIDI_OFI_global.am_recv_seq_tracker, id);
    if (r == MPIDIU_MAP_NOT_FOUND) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "First time adding recv seqno addr=%" PRIx64 "\n", addr));
        MPIDIU_map_set(MPIDI_OFI_global.am_recv_seq_tracker, id, 0, MPL_MEM_OTHER);
        return 0;
    } else {
        return (uint16_t) (uintptr_t) r;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_am_set_next_recv_seqno(fi_addr_t addr, uint16_t seqno)
{
    uint64_t id = addr;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "Next recv seqno=%d addr=%" PRIx64 "\n", seqno, addr));

    MPIDIU_map_update(MPIDI_OFI_global.am_recv_seq_tracker, id, (void *) (uintptr_t) seqno,
                      MPL_MEM_OTHER);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_enqueue_unordered_msg(const MPIDI_OFI_am_header_t *
                                                                am_hdr)
{
    MPIDI_OFI_am_unordered_msg_t *uo_msg;
    size_t uo_msg_len, packet_len;
    /* Essentially, uo_msg_len == packet_len + sizeof(next,prev pointers) */

    uo_msg_len = sizeof(*uo_msg) + am_hdr->am_hdr_sz + am_hdr->data_sz;

    /* Allocate a new memory region to store this unordered message.
     * We are doing this because the original am_hdr comes from FI_MULTI_RECV
     * buffer, which may be reused soon by OFI. */
    uo_msg = MPL_malloc(uo_msg_len, MPL_MEM_BUFFER);
    if (uo_msg == NULL)
        return MPI_ERR_NO_MEM;

    packet_len = sizeof(*am_hdr) + am_hdr->am_hdr_sz + am_hdr->data_sz;
    MPIR_Memcpy(&uo_msg->am_hdr, am_hdr, packet_len);

    DL_APPEND(MPIDI_OFI_global.am_unordered_msgs, uo_msg);

    return MPI_SUCCESS;
}

/* Find and dequeue a message that matches (comm, src_rank, seqno), then return it.
 * Caller must free the returned pointer. */
MPL_STATIC_INLINE_PREFIX MPIDI_OFI_am_unordered_msg_t
    * MPIDI_OFI_am_claim_unordered_msg(fi_addr_t addr, uint16_t seqno)
{
    MPIDI_OFI_am_unordered_msg_t *uo_msg;

    /* Future optimization note:
     * Currently we are doing linear search every time, assuming that the number of items
     * in the queue is extremely small.
     * If it's not the case, we should consider using better data structure and algorithm
     * to look up. */
    DL_FOREACH(MPIDI_OFI_global.am_unordered_msgs, uo_msg) {
        if (uo_msg->am_hdr.fi_src_addr == addr && uo_msg->am_hdr.seqno == seqno) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                            (MPL_DBG_FDEST,
                             "Found unordered message in the queue: addr=%" PRIx64 ", seqno=%d\n",
                             addr, seqno));
            DL_DELETE(MPIDI_OFI_global.am_unordered_msgs, uo_msg);
            return uo_msg;
        }
    }

    return NULL;
}

/* MPIDI_OFI_EVENT_AM_SEND */

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_send_event(void *context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_am_request_header_t *req_hdr = ((MPIDI_OFI_am_request_t *) context)->req_hdr;

    /* ref: MPIDI_OFI_do_am_isend */
    if (req_hdr->pack_buffer) {
        MPL_free(req_hdr->pack_buffer);
        req_hdr->pack_buffer = NULL;
    }

    MPIR_Request *sreq = MPIDI_OFI_context_to_request(context);
    if (sreq) {
        int handler_id = req_hdr->msg_hdr.handler_id;

        mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* MPIDI_OFI_EVENT_AM_RECV */

static inline int ofi_handle_am(MPIDI_OFI_am_header_t * msg_hdr);

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_recv_event(void *context, void *buffer)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_am_header_t *am_hdr = buffer;

    uint16_t expected_seqno = MPIDI_OFI_am_get_next_recv_seqno(am_hdr->fi_src_addr);
    if (am_hdr->seqno != expected_seqno) {
        /* This message came earlier than the one that we were expecting.
         * Put it in the queue to process it later. */
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                        (MPL_DBG_FDEST,
                         "Expected seqno=%d but got %d (addr=%" PRIx64 "). "
                         "Enqueueing it to the queue.\n",
                         expected_seqno, am_hdr->seqno, am_hdr->fi_src_addr));
        mpi_errno = MPIDI_OFI_am_enqueue_unordered_msg(am_hdr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Received an expected message */
    MPIDI_OFI_am_unordered_msg_t *uo_msg = NULL;
    while (true) {
        fi_addr_t fi_src_addr = am_hdr->fi_src_addr;
        uint16_t next_seqno = am_hdr->seqno + 1;

        mpi_errno = ofi_handle_am(am_hdr);
        MPIR_ERR_CHECK(mpi_errno);

        if (uo_msg) {
            MPL_free(uo_msg);
        }

        /* See if we can process other messages in the queue */
        uo_msg = MPIDI_OFI_am_claim_unordered_msg(fi_src_addr, next_seqno);
        if (uo_msg) {
            am_hdr = &uo_msg->am_hdr;
            continue;
        }

        /* Record the next expected sequence number from fi_src_addr */
        MPIDI_OFI_am_set_next_recv_seqno(fi_src_addr, next_seqno);
        break;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int ofi_handle_am(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    void *p_data;
    void *in_data;
    size_t data_sz;
    int is_contig;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);

    /* note: msg_hdr + 1 points to the payload */
    p_data = in_data = (char *) (msg_hdr + 1) + msg_hdr->am_hdr_sz;
    data_sz = msg_hdr->data_sz;

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, (msg_hdr + 1),
                                                       &p_data, &data_sz, 0 /* is_local */ ,
                                                       &is_contig, &rreq);

    if (!rreq)
        goto fn_exit;

    /* TODO: if we pass a flag, the callback could do all the following */
    MPIDIG_recv_copy(in_data, rreq);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    return mpi_errno;
}

#endif /* OFI_AM_EVENTS_H_INCLUDED */
