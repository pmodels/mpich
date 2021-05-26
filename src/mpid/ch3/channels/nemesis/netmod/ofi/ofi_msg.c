/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "ofi_impl.h"

/* ------------------------------------------------------------------------ */
/* GET_PGID_AND_SET_MATCH macro looks up the process group to find the      */
/* correct rank in multiple process groups.  The "contigmsg" family of apis */
/* work on a global scope, not on a communicator scope(like tagged MPI.)    */
/* The pgid matching is used for uniquely scoping the tag, usually in       */
/* intercomms and dynamic process management where there are multiple       */
/* global world spaces with similar ranks in the global space               */
/* ------------------------------------------------------------------------ */
#define GET_PGID_AND_SET_MATCH()                                        \
({                                                                      \
  if (vc->pg) {                                                         \
    MPIDI_PG_IdToNum(gl_data.pg_p, &pgid);                              \
  } else {                                                              \
    pgid = NO_PGID;                                                     \
  }                                                                     \
  if (gl_data.api_set == API_SET_1){                                    \
      match_bits = ((uint64_t)pgid << MPIDI_OFI_PGID_SHIFT);                 \
  }else{                                                                \
      match_bits = 0;                                                   \
  }                                                                     \
  if (NO_PGID == pgid) {                                                \
    match_bits |= (uint64_t)vc->port_name_tag<<                         \
        (MPIDI_OFI_PORT_SHIFT);                                              \
  }else{                                                                \
      match_bits |= (uint64_t)MPIR_Process.comm_world->rank <<          \
          (MPIDI_OFI_PSOURCE_SHIFT);                                         \
  }                                                                     \
  match_bits |= MPIDI_OFI_MSG_RTS;                                           \
})

/* ------------------------------------------------------------------------ */
/* START_COMM is common code used by the nemesis netmod functions:          */
/* iSendContig                                                              */
/* SendNoncontig                                                            */
/* iStartContigMsg                                                          */
/* iSendIov                                                                 */
/* These routines differ slightly in their behaviors, but can share common  */
/* code to perform the send.  START_COMM provides that common code, which   */
/* is based on a tagged rendezvous message.                                 */
/* The rendezvous is implemented with an RTS-CTS-Data send protocol:        */
/* CTS_POST()   |                                  |                        */
/* RTS_SEND()   | -------------------------------> | ue_callback()(ofi_cm.c)*/
/*              |                                  |   pack_buffer()        */
/*              |                                  |   DATA_POST()          */
/*              |                                  |   RTS_POST()           */
/*              |                                  |   CTS_SEND()           */
/* CTS_MATCH()  | <------------------------------- |                        */
/* DATA_SEND()  | ===============================> | handle_packet()        */
/*              |                                  |   notify_ch3_pkt()     */
/*              v                                  v                        */
/* ------------------------------------------------------------------------ */
#define START_COMM()                                                    \
    ({                                                                  \
        gl_data.rts_cts_in_flight++;                                    \
        GET_PGID_AND_SET_MATCH();                                       \
        VC_READY_CHECK(vc);                                             \
        MPIR_cc_inc(sreq->cc_ptr);                                      \
        MPIR_cc_inc(sreq->cc_ptr);                                      \
        REQ_OFI(sreq)->event_callback   = MPID_nem_ofi_data_callback;   \
        REQ_OFI(sreq)->pack_buffer      = pack_buffer;                  \
        REQ_OFI(sreq)->pack_buffer_size = pkt_len;                      \
        REQ_OFI(sreq)->vc               = vc;                           \
        REQ_OFI(sreq)->tag              = match_bits;                   \
                                                                        \
        MPID_nem_ofi_create_req(&cts_req, 1);                           \
        cts_req->dev.OnDataAvail         = NULL;                        \
        cts_req->dev.next                = NULL;                        \
        REQ_OFI(cts_req)->event_callback = MPID_nem_ofi_cts_recv_callback; \
        REQ_OFI(cts_req)->parent         = sreq;                        \
                                                                        \
        FI_RC_RETRY(fi_trecv(gl_data.endpoint,                          \
                       NULL,                                            \
                       0,                                               \
                       gl_data.mr,                                      \
                       VC_OFI(vc)->direct_addr,                         \
                       match_bits | MPIDI_OFI_MSG_CTS,                       \
                       0, /* Exact tag match, no ignore bits */         \
                       &(REQ_OFI(cts_req)->ofi_context)),trecv);        \
        if (gl_data.api_set == API_SET_1){                              \
            FI_RC_RETRY(fi_tsend(gl_data.endpoint,                      \
                           &REQ_OFI(sreq)->pack_buffer_size,            \
                           sizeof(REQ_OFI(sreq)->pack_buffer_size),     \
                           gl_data.mr,                                  \
                           VC_OFI(vc)->direct_addr,                     \
                           match_bits,                                  \
                           &(REQ_OFI(sreq)->ofi_context)),tsend);       \
        }else{                                                          \
            FI_RC_RETRY(fi_tsenddata(gl_data.endpoint,                  \
                               &REQ_OFI(sreq)->pack_buffer_size,        \
                               sizeof(REQ_OFI(sreq)->pack_buffer_size), \
                               gl_data.mr,                              \
                               pgid,                                    \
                               VC_OFI(vc)->direct_addr,                 \
                               match_bits,                              \
                               &(REQ_OFI(sreq)->ofi_context)),tsend);   \
        }                                                               \
    })


/* ------------------------------------------------------------------------ */
/* General handler for RTS-CTS-Data protocol.  Waits for the cc counter     */
/* to hit two (send RTS and receive CTS decrementers) before kicking off the*/
/* bulk data transfer.  On data send completion, the request can be freed   */
/* Handles SEND-side events only.  We cannot rely on wc->tag field being    */
/* set for these events, so we must use the TAG stored in the sreq.         */
/* ------------------------------------------------------------------------ */
static int MPID_nem_ofi_data_callback(cq_tagged_entry_t * wc, MPIR_Request * sreq)
{
    int complete = 0, mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    req_fn reqFn;
    uint64_t tag = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_DATA_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_DATA_CALLBACK);
    switch (REQ_OFI(sreq)->tag & MPIDI_OFI_PROTOCOL_MASK) {
        case MPIDI_OFI_MSG_CTS | MPIDI_OFI_MSG_RTS | MPIDI_OFI_MSG_DATA:
            /* Verify request is complete prior to freeing buffers.
             * Multiple DATA events may arrive because we need
             * to store updated TAG values in the sreq.
             */
            if (MPIR_cc_get(sreq->cc) == 1) {
                MPL_free(REQ_OFI(sreq)->pack_buffer);

                MPL_free(REQ_OFI(sreq)->real_hdr);

                reqFn = sreq->dev.OnDataAvail;
                if (!reqFn) {
                    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));
                } else {
                    vc = REQ_OFI(sreq)->vc;
                    MPIDI_CH3I_NM_OFI_RC(reqFn(vc, sreq, &complete));
                }
                gl_data.rts_cts_in_flight--;

            } else {
                MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));
            }
            break;
        case MPIDI_OFI_MSG_RTS:
            MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));
            break;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_DATA_CALLBACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* Signals the CTS has been received.  Call MPID_nem_ofi_data_callback on   */
/* the parent send request to kick off the bulk data transfer               */
/* Handles RECV-side events only.  We rely on wc->tag field being set for   */
/* these events.                                                            */
/* ------------------------------------------------------------------------ */
static int MPID_nem_ofi_cts_recv_callback(cq_tagged_entry_t * wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *preq;
    MPIDI_VC_t *vc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_CTS_RECV_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_CTS_RECV_CALLBACK);
    preq = REQ_OFI(rreq)->parent;
    switch (wc->tag & MPIDI_OFI_PROTOCOL_MASK) {
        case MPIDI_OFI_MSG_CTS | MPIDI_OFI_MSG_RTS:
            vc = REQ_OFI(preq)->vc;
            /* store tag in the request for SEND-side event processing */
            REQ_OFI(preq)->tag = wc->tag | MPIDI_OFI_MSG_DATA;
            if (REQ_OFI(preq)->pack_buffer) {
                FI_RC_RETRY(fi_tsend(gl_data.endpoint,
                                     REQ_OFI(preq)->pack_buffer,
                                     REQ_OFI(preq)->pack_buffer_size,
                                     gl_data.mr,
                                     VC_OFI(vc)->direct_addr,
                                     REQ_OFI(preq)->tag,
                                     (void *) &(REQ_OFI(preq)->ofi_context)), tsend);
            } else {
                struct fi_msg_tagged msg;
                void *desc = NULL;
                msg.msg_iov = REQ_OFI(preq)->iov;
                msg.desc = &desc;
                msg.iov_count = REQ_OFI(preq)->iov_count;
                msg.addr = VC_OFI(vc)->direct_addr;
                msg.tag = REQ_OFI(preq)->tag, msg.ignore = 0ULL;
                msg.context = &(REQ_OFI(preq)->ofi_context);
                msg.data = 0ULL;
                FI_RC_RETRY(fi_tsendmsg(gl_data.endpoint, &msg, 0ULL), tsend);
            }
            MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(preq));
            break;
    }
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(rreq));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_CTS_RECV_CALLBACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* The nemesis API implementations:                                         */
/* Use packing if iovecs are not supported by the OFI provider              */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_iSendContig(MPIDI_VC_t * vc,
                             MPIR_Request * sreq,
                             void *hdr, intptr_t hdr_sz, void *data, intptr_t data_sz)
{
    int pgid, c, mpi_errno = MPI_SUCCESS;
    char *pack_buffer = NULL;
    uint64_t match_bits;
    MPIR_Request *cts_req;
    intptr_t buf_offset = 0;
    size_t pkt_len;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_ISENDCONTIG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_ISENDCONTIG);
    MPIR_Assert(hdr_sz <= (intptr_t) sizeof(MPIDI_CH3_Pkt_t));
    MPID_nem_ofi_init_req(sreq);
    pkt_len = sizeof(MPIDI_CH3_Pkt_t) + data_sz;
    if (gl_data.iov_limit > 1) {
        REQ_OFI(sreq)->real_hdr = MPL_malloc(sizeof(MPIDI_CH3_Pkt_t), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(REQ_OFI(sreq)->real_hdr == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "iSendContig header allocation");
        MPIR_Memcpy(REQ_OFI(sreq)->real_hdr, hdr, hdr_sz);
        REQ_OFI(sreq)->iov[0].iov_base = REQ_OFI(sreq)->real_hdr;
        REQ_OFI(sreq)->iov[0].iov_len = sizeof(MPIDI_CH3_Pkt_t);
        REQ_OFI(sreq)->iov[1].iov_base = data;
        REQ_OFI(sreq)->iov[1].iov_len = data_sz;
        REQ_OFI(sreq)->iov_count = 2;
    } else {
        pack_buffer = MPL_malloc(pkt_len, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "iSendContig pack buffer allocation");
        MPIR_Memcpy(pack_buffer, hdr, hdr_sz);
        buf_offset += sizeof(MPIDI_CH3_Pkt_t);
        MPIR_Memcpy(pack_buffer + buf_offset, data, data_sz);
    }
    START_COMM();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_ISENDCONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ofi_iSendIov(MPIDI_VC_t * vc, MPIR_Request * sreq, void *hdr, intptr_t hdr_sz,
                          struct iovec * iov, int n_iov)
{
    int pgid, c, mpi_errno = MPI_SUCCESS;
    char *pack_buffer = NULL;
    uint64_t match_bits;
    MPIR_Request *cts_req;
    intptr_t buf_offset = 0;
    size_t pkt_len;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_ISENDIOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_ISENDIOV);

    MPID_nem_ofi_init_req(sreq);

    /* Compute packed buffer size */
    MPIR_Assert(hdr_sz <= (intptr_t) sizeof(MPIDI_CH3_Pkt_t));
    pkt_len = sizeof(MPIDI_CH3_Pkt_t);
    for (i = 0; i < n_iov; i++)
        pkt_len += iov[i].iov_len;

    pack_buffer = MPL_malloc(pkt_len, MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                         "**nomem", "**nomem %s", "iSendIov pack buffer allocation");

    /* Copy header and iovs into packed buffer */
    MPIR_Memcpy(pack_buffer, hdr, hdr_sz);
    buf_offset += sizeof(MPIDI_CH3_Pkt_t);
    for (i = 0; i < n_iov; i++) {
        MPIR_Memcpy(pack_buffer + buf_offset, iov[i].iov_base, iov[i].iov_len);
        buf_offset += iov[i].iov_len;
    }
    START_COMM();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_ISENDIOV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ofi_SendNoncontig(MPIDI_VC_t * vc, MPIR_Request * sreq, void *hdr, intptr_t hdr_sz,
                               struct iovec * hdr_iov, int n_hdr_iov)
{
    int c, i, pgid, mpi_errno = MPI_SUCCESS;
    char *pack_buffer;
    MPI_Aint data_sz;
    uint64_t match_bits;
    MPIR_Request *cts_req;
    intptr_t buf_offset = 0;
    void *data = NULL;
    size_t pkt_len;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_SENDNONCONTIG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_SENDNONCONTIG);
    MPIR_Assert(hdr_sz <= (intptr_t) sizeof(MPIDI_CH3_Pkt_t));

    MPID_nem_ofi_init_req(sreq);
    data_sz = sreq->dev.msgsize - sreq->dev.msg_offset;
    pkt_len = sizeof(MPIDI_CH3_Pkt_t) + data_sz;
    if (n_hdr_iov > 0) {
        /* add length of extended header iovs */
        for (i = 0; i < n_hdr_iov; i++)
            pkt_len += hdr_iov[i].iov_len;
    }

    pack_buffer = MPL_malloc(pkt_len, MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                         "**nomem", "**nomem %s", "SendNonContig pack buffer allocation");
    MPIR_Memcpy(pack_buffer, hdr, hdr_sz);
    buf_offset += sizeof(MPIDI_CH3_Pkt_t);

    if (n_hdr_iov > 0) {
        /* pack extended header iovs */
        for (i = 0; i < n_hdr_iov; i++) {
            MPIR_Memcpy(pack_buffer + buf_offset, hdr_iov[i].iov_base, hdr_iov[i].iov_len);
            buf_offset += hdr_iov[i].iov_len;
        }
    }

    MPI_Aint actual_pack_bytes;
    MPIR_Typerep_pack(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype,
                      sreq->dev.msg_offset, pack_buffer + buf_offset,
                      sreq->dev.msgsize - sreq->dev.msg_offset, &actual_pack_bytes);
    MPIR_Assert(actual_pack_bytes == sreq->dev.msgsize - sreq->dev.msg_offset);

    START_COMM();
    MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_SENDNONCONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_ofi_iStartContigMsg(MPIDI_VC_t * vc,
                                 void *hdr,
                                 intptr_t hdr_sz,
                                 void *data, intptr_t data_sz, MPIR_Request ** sreq_ptr)
{
    int c, pgid, mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIR_Request *cts_req;
    char *pack_buffer = NULL;
    uint64_t match_bits;
    size_t pkt_len;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_ISTARTCONTIGMSG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_ISTARTCONTIGMSG);
    MPIR_Assert(hdr_sz <= (intptr_t) sizeof(MPIDI_CH3_Pkt_t));

    MPID_nem_ofi_create_req(&sreq, 2);
    sreq->kind = MPIR_REQUEST_KIND__SEND;
    sreq->dev.OnDataAvail = NULL;
    sreq->dev.next = NULL;
    pkt_len = sizeof(MPIDI_CH3_Pkt_t) + data_sz;
    if (gl_data.iov_limit > 1) {
        REQ_OFI(sreq)->real_hdr = MPL_malloc(sizeof(MPIDI_CH3_Pkt_t), MPL_MEM_BUFFER);
        MPIR_Memcpy(REQ_OFI(sreq)->real_hdr, hdr, hdr_sz);
        REQ_OFI(sreq)->iov[0].iov_base = REQ_OFI(sreq)->real_hdr;
        REQ_OFI(sreq)->iov[0].iov_len = sizeof(MPIDI_CH3_Pkt_t);
        REQ_OFI(sreq)->iov[1].iov_base = data;
        REQ_OFI(sreq)->iov[1].iov_len = data_sz;
        REQ_OFI(sreq)->iov_count = 2;
    } else {
        pack_buffer = MPL_malloc(pkt_len, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "iStartContig pack buffer allocation");
        MPIR_Memcpy((void *) pack_buffer, hdr, hdr_sz);
        if (data_sz)
            MPIR_Memcpy((void *) (pack_buffer + sizeof(MPIDI_CH3_Pkt_t)), data, data_sz);
    }
    START_COMM();
    *sreq_ptr = sreq;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_ISTARTCONTIGMSG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
