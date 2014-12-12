/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
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
  match_bits = (uint64_t)MPIR_Process.comm_world->rank <<               \
    (MPID_PORT_SHIFT);                                                  \
  if (0 == pgid) {                                                      \
    match_bits |= (uint64_t)vc->port_name_tag<<                         \
      (MPID_PORT_SHIFT+MPID_PSOURCE_SHIFT);                             \
  }                                                                     \
  match_bits |= pgid;                                                   \
  match_bits |= MPID_MSG_RTS;                                           \
})

/* ------------------------------------------------------------------------ */
/* START_COMM is common code used by the nemesis netmod functions:          */
/* iSendContig                                                              */
/* SendNoncontig                                                            */
/* iStartContigMsg                                                          */
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
  ({                                                                    \
    GET_PGID_AND_SET_MATCH();                                           \
    VC_READY_CHECK(vc);                                                 \
    c = 1;                                                              \
    MPID_cc_incr(sreq->cc_ptr, &c);                                     \
    MPID_cc_incr(sreq->cc_ptr, &c);                                     \
    REQ_OFI(sreq)->event_callback   = MPID_nem_ofi_data_callback;       \
    REQ_OFI(sreq)->pack_buffer      = pack_buffer;                      \
    REQ_OFI(sreq)->pack_buffer_size = pkt_len;                          \
    REQ_OFI(sreq)->vc               = vc;                               \
    REQ_OFI(sreq)->tag              = match_bits;                       \
                                                                        \
    MPID_nem_ofi_create_req(&cts_req, 1);                               \
    cts_req->dev.OnDataAvail         = NULL;                            \
    cts_req->dev.next                = NULL;                            \
    REQ_OFI(cts_req)->event_callback = MPID_nem_ofi_cts_recv_callback;  \
    REQ_OFI(cts_req)->parent         = sreq;                            \
                                                                        \
    FI_RC(fi_trecv(gl_data.endpoint,                                \
                       NULL,                                            \
                       0,                                               \
                       gl_data.mr,                                      \
                       VC_OFI(vc)->direct_addr,                         \
                       match_bits | MPID_MSG_CTS,                       \
                       0, /* Exact tag match, no ignore bits */         \
                       &(REQ_OFI(cts_req)->ofi_context)),trecv);    \
    FI_RC(fi_tsend(gl_data.endpoint,                                  \
                     &REQ_OFI(sreq)->pack_buffer_size,                  \
                     sizeof(REQ_OFI(sreq)->pack_buffer_size),           \
                     gl_data.mr,                                        \
                     VC_OFI(vc)->direct_addr,                           \
                     match_bits,                                        \
                     &(REQ_OFI(sreq)->ofi_context)),tsend);           \
  })


/* ------------------------------------------------------------------------ */
/* General handler for RTS-CTS-Data protocol.  Waits for the cc counter     */
/* to hit two (send RTS and receive CTS decrementers) before kicking off the*/
/* bulk data transfer.  On data send completion, the request can be freed   */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_data_callback)
static int MPID_nem_ofi_data_callback(cq_tagged_entry_t * wc, MPID_Request * sreq)
{
    int complete = 0, mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    req_fn reqFn;
    uint64_t tag = 0;
    BEGIN_FUNC(FCNAME);
    if (sreq->cc == 2) {
        vc = REQ_OFI(sreq)->vc;
        REQ_OFI(sreq)->tag = tag | MPID_MSG_DATA;
        FI_RC(fi_tsend(gl_data.endpoint,
                         REQ_OFI(sreq)->pack_buffer,
                         REQ_OFI(sreq)->pack_buffer_size,
                         gl_data.mr,
                         VC_OFI(vc)->direct_addr,
                         wc->tag | MPID_MSG_DATA, (void *) &(REQ_OFI(sreq)->ofi_context)), tsend);
    }
    if (sreq->cc == 1) {
        if (REQ_OFI(sreq)->pack_buffer)
            MPIU_Free(REQ_OFI(sreq)->pack_buffer);

        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn) {
            MPIDI_CH3U_Request_complete(sreq);
        }
        else {
            vc = REQ_OFI(sreq)->vc;
            MPI_RC(reqFn(vc, sreq, &complete));
        }
    }
    else {
        MPIDI_CH3U_Request_complete(sreq);
    }
    END_FUNC_RC(FCNAME);
}

/* ------------------------------------------------------------------------ */
/* Signals the CTS has been received.  Call MPID_nem_ofi_data_callback on   */
/* the parent send request to kick off the bulk data transfer               */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_cts_recv_callback)
static int MPID_nem_ofi_cts_recv_callback(cq_tagged_entry_t * wc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    MPI_RC(MPID_nem_ofi_data_callback(wc, REQ_OFI(rreq)->parent));
    MPIDI_CH3U_Request_complete(rreq);
    END_FUNC_RC(FCNAME);
}

/* ------------------------------------------------------------------------ */
/* The nemesis API implementations:                                         */
/* These functions currently memory copy into a pack buffer before sending  */
/* To improve performance, we can replace the memory copy with a non-contig */
/* send (using tsendmsg)                                                    */
/* For now, the memory copy is the simplest implementation of these         */
/* functions over a tagged msg interface                                    */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_iSendContig)
int MPID_nem_ofi_iSendContig(MPIDI_VC_t * vc,
                             MPID_Request * sreq,
                             void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz)
{
    int pgid, c, pkt_len, mpi_errno = MPI_SUCCESS;
    char *pack_buffer;
    uint64_t match_bits;
    MPID_Request *cts_req;

    BEGIN_FUNC(FCNAME);
    MPIU_Assert(hdr_sz <= (MPIDI_msg_sz_t) sizeof(MPIDI_CH3_Pkt_t));
    MPID_nem_ofi_init_req(sreq);
    pkt_len = sizeof(MPIDI_CH3_Pkt_t) + data_sz;
    pack_buffer = MPIU_Malloc(pkt_len);
    MPIU_Assert(pack_buffer);
    MPIU_Memcpy(pack_buffer, hdr, hdr_sz);
    MPIU_Memcpy(pack_buffer + sizeof(MPIDI_CH3_Pkt_t), data, data_sz);
    START_COMM();
    END_FUNC_RC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_SendNoncontig)
int MPID_nem_ofi_SendNoncontig(MPIDI_VC_t * vc,
                               MPID_Request * sreq, void *hdr, MPIDI_msg_sz_t hdr_sz)
{
    int c, pgid, pkt_len, mpi_errno = MPI_SUCCESS;
    char *pack_buffer;
    MPI_Aint data_sz;
    uint64_t match_bits;
    MPID_Request *cts_req;

    BEGIN_FUNC(FCNAME);
    MPIU_Assert(hdr_sz <= (MPIDI_msg_sz_t) sizeof(MPIDI_CH3_Pkt_t));
    MPIU_Assert(sreq->dev.segment_first == 0);

    data_sz = sreq->dev.segment_size;
    pkt_len = sizeof(MPIDI_CH3_Pkt_t) + data_sz;
    pack_buffer = MPIU_Malloc(pkt_len);
    MPIU_Assert(pack_buffer);
    MPIU_Memcpy(pack_buffer, hdr, hdr_sz);
    MPID_Segment_pack(sreq->dev.segment_ptr, 0, &data_sz, pack_buffer + sizeof(MPIDI_CH3_Pkt_t));
    START_COMM();
    MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
    END_FUNC_RC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_iStartContigMsg)
int MPID_nem_ofi_iStartContigMsg(MPIDI_VC_t * vc,
                                 void *hdr,
                                 MPIDI_msg_sz_t hdr_sz,
                                 void *data, MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr)
{
    int pkt_len, c, pgid, mpi_errno = MPI_SUCCESS;
    MPID_Request *sreq;
    MPID_Request *cts_req;
    char *pack_buffer;
    uint64_t match_bits;
    BEGIN_FUNC(FCNAME);
    MPIU_Assert(hdr_sz <= (MPIDI_msg_sz_t) sizeof(MPIDI_CH3_Pkt_t));

    MPID_nem_ofi_create_req(&sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = NULL;
    sreq->dev.next = NULL;
    pkt_len = sizeof(MPIDI_CH3_Pkt_t) + data_sz;
    pack_buffer = MPIU_Malloc(pkt_len);
    MPIU_Assert(pack_buffer);
    MPIU_Memcpy((void *) pack_buffer, hdr, hdr_sz);
    if (data_sz)
        MPIU_Memcpy((void *) (pack_buffer + sizeof(MPIDI_CH3_Pkt_t)), data, data_sz);
    START_COMM();
    *sreq_ptr = sreq;
    END_FUNC_RC(FCNAME);
}
