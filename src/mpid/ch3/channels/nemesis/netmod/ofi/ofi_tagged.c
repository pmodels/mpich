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

#define MPID_NORMAL_SEND 0

/* ------------------------------------------------------------------------ */
/* Receive callback called after sending a syncronous send acknowledgement. */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_sync_recv_callback)
static inline int MPID_nem_ofi_sync_recv_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                                  MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    BEGIN_FUNC(FCNAME);

    MPIDI_CH3U_Recvq_DP(REQ_OFI(rreq)->parent);
    MPIDI_CH3U_Request_complete(REQ_OFI(rreq)->parent);
    MPIDI_CH3U_Request_complete(rreq);

    END_FUNC(FCNAME);
    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* Send done callback                                                       */
/* Free any temporary/pack buffers and complete the send request            */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_send_callback)
static inline int MPID_nem_ofi_send_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                             MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    if (REQ_OFI(sreq)->pack_buffer)
        MPIU_Free(REQ_OFI(sreq)->pack_buffer);
    MPIDI_CH3U_Request_complete(sreq);
    END_FUNC(FCNAME);
    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* Receive done callback                                                    */
/* Handle an incoming receive completion event                              */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_recv_callback)
static inline int MPID_nem_ofi_recv_callback(cq_tagged_entry_t * wc, MPID_Request * rreq)
{
    int err0, err1, src, mpi_errno = MPI_SUCCESS;
    uint64_t ssend_bits;
    MPIDI_msg_sz_t sz;
    MPIDI_VC_t *vc;
    MPID_Request *sync_req;
    BEGIN_FUNC(FCNAME);
    /* ---------------------------------------------------- */
    /* Populate the MPI Status and unpack noncontig buffer  */
    /* ---------------------------------------------------- */
    rreq->status.MPI_ERROR = MPI_SUCCESS;
    rreq->status.MPI_SOURCE = get_source(wc->tag);
    rreq->status.MPI_TAG = get_tag(wc->tag);
    REQ_OFI(rreq)->req_started = 1;
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);

    if (REQ_OFI(rreq)->pack_buffer) {
        MPIDI_CH3U_Buffer_copy(REQ_OFI(rreq)->pack_buffer,
                               MPIR_STATUS_GET_COUNT(rreq->status),
                               MPI_BYTE, &err0, rreq->dev.user_buf,
                               rreq->dev.user_count, rreq->dev.datatype, &sz, &err1);
        MPIR_STATUS_SET_COUNT(rreq->status, sz);
        MPIU_Free(REQ_OFI(rreq)->pack_buffer);
        if (err0 || err1) {
            rreq->status.MPI_ERROR = MPI_ERR_TYPE;
        }
    }

    if ((wc->tag & MPID_PROTOCOL_MASK) == MPID_SYNC_SEND) {
        /* ---------------------------------------------------- */
        /* Ack the sync send and wait for the send request      */
        /* completion(when callback executed.  A protocol bit   */
        /* MPID_SYNC_SEND_ACK is set in the tag bits to provide */
        /* separation of MPI messages and protocol messages     */
        /* ---------------------------------------------------- */
        vc = REQ_OFI(rreq)->vc;
        if (!vc) {      /* MPI_ANY_SOURCE -- Post message from status, complete the VC */
            src = get_source(wc->tag);
            vc = rreq->comm->vcr[src];
            MPIU_Assert(vc);
        }
        ssend_bits = init_sendtag(rreq->dev.match.parts.context_id,
                                  rreq->comm->rank, rreq->status.MPI_TAG, MPID_SYNC_SEND_ACK);
        MPID_nem_ofi_create_req(&sync_req, 1);
        sync_req->dev.OnDataAvail = NULL;
        sync_req->dev.next = NULL;
        REQ_OFI(sync_req)->event_callback = MPID_nem_ofi_sync_recv_callback;
        REQ_OFI(sync_req)->parent = rreq;
        FI_RC(fi_tsend(gl_data.endpoint,
                         NULL,
                         0,
                         gl_data.mr,
                         VC_OFI(vc)->direct_addr,
                         ssend_bits, &(REQ_OFI(sync_req)->ofi_context)), tsend);
    }
    else {
        /* ---------------------------------------------------- */
        /* Non-syncronous send, complete normally               */
        /* by removing from the CH3 queue and completing the    */
        /* request object                                       */
        /* ---------------------------------------------------- */
        MPIDI_CH3U_Recvq_DP(rreq);
        MPIDI_CH3U_Request_complete(rreq);
    }
    END_FUNC_RC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(do_isend)
static inline int do_isend(struct MPIDI_VC *vc,
                           const void *buf,
                           int count,
                           MPI_Datatype datatype,
                           int dest,
                           int tag,
                           MPID_Comm * comm,
                           int context_offset, struct MPID_Request **request, uint64_t type)
{
    int err0, err1, dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buffer;
    uint64_t match_bits, ssend_match, ssend_mask;
    MPI_Aint dt_true_lb;
    MPID_Request *sreq = NULL, *sync_req = NULL;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    BEGIN_FUNC(FCNAME);
    VC_READY_CHECK(vc);

    /* ---------------------------------------------------- */
    /* Create the MPI request                               */
    /* ---------------------------------------------------- */
    MPID_nem_ofi_create_req(&sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = NULL;
    REQ_OFI(sreq)->event_callback = MPID_nem_ofi_send_callback;
    REQ_OFI(sreq)->vc = vc;

    /* ---------------------------------------------------- */
    /* Create the pack buffer (if required), and allocate   */
    /* a send request                                       */
    /* ---------------------------------------------------- */
    match_bits = init_sendtag(comm->context_id + context_offset, comm->rank, tag, type);
    sreq->dev.match.parts.tag = match_bits;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    send_buffer = (char *) buf + dt_true_lb;
    if (!dt_contig) {
        send_buffer = (char *) MPIU_Malloc(data_sz);
        MPIU_ERR_CHKANDJUMP1(send_buffer == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send buffer alloc");
        MPIDI_CH3U_Buffer_copy(buf, count, datatype, &err0,
                               send_buffer, data_sz, MPI_BYTE, &data_sz, &err1);
        REQ_OFI(sreq)->pack_buffer = send_buffer;
    }

    if (type == MPID_SYNC_SEND) {
        /* ---------------------------------------------------- */
        /* For syncronous send, we post a receive to catch the  */
        /* match ack, but use the tag protocol bits to avoid    */
        /* matching with MPI level messages.                    */
        /* ---------------------------------------------------- */
        int c = 1;
        MPID_cc_incr(sreq->cc_ptr, &c);
        MPID_nem_ofi_create_req(&sync_req, 1);
        sync_req->dev.OnDataAvail = NULL;
        sync_req->dev.next = NULL;
        REQ_OFI(sync_req)->event_callback = MPID_nem_ofi_sync_recv_callback;
        REQ_OFI(sync_req)->parent = sreq;
        ssend_match = init_recvtag(&ssend_mask, comm->context_id + context_offset, dest, tag);
        ssend_match |= MPID_SYNC_SEND_ACK;
        FI_RC(fi_trecv(gl_data.endpoint,    /* endpoint    */
                           NULL,        /* recvbuf     */
                           0,   /* data sz     */
                           gl_data.mr,  /* dynamic mr  */
                           VC_OFI(vc)->direct_addr,     /* remote proc */
                           ssend_match, /* match bits  */
                           0ULL,        /* mask bits   */
                           &(REQ_OFI(sync_req)->ofi_context)), trecv);
    }
    FI_RC(fi_tsend(gl_data.endpoint,  /* Endpoint                       */
                     send_buffer,       /* Send buffer(packed or user)    */
                     data_sz,   /* Size of the send               */
                     gl_data.mr,        /* Dynamic memory region          */
                     VC_OFI(vc)->direct_addr,   /* Use the address of this VC     */
                     match_bits,        /* Match bits                     */
                     &(REQ_OFI(sreq)->ofi_context)), tsend);
    *request = sreq;
    END_FUNC_RC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_recv_posted)
int MPID_nem_ofi_recv_posted(struct MPIDI_VC *vc, struct MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS, dt_contig, src, tag;
    uint64_t match_bits = 0, mask_bits = 0;
    fi_addr_t remote_proc = 0;
    MPIDI_msg_sz_t data_sz;
    MPI_Aint dt_true_lb;
    MPID_Datatype *dt_ptr;
    MPIR_Context_id_t context_id;
    char *recv_buffer;
    BEGIN_FUNC(FCNAME);

    /* ------------------------ */
    /* Initialize the request   */
    /* ------------------------ */
    MPID_nem_ofi_init_req(rreq);
    REQ_OFI(rreq)->event_callback = MPID_nem_ofi_recv_callback;
    REQ_OFI(rreq)->vc = vc;

    /* ---------------------------------------------------- */
    /* Fill out the match info, and allocate the pack buffer */
    /* a send request                                       */
    /* ---------------------------------------------------- */
    src = rreq->dev.match.parts.rank;
    tag = rreq->dev.match.parts.tag;
    context_id = rreq->dev.match.parts.context_id;
    match_bits = init_recvtag(&mask_bits, context_id, src, tag);
    OFI_ADDR_INIT(src, vc, remote_proc);
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (dt_contig) {
        recv_buffer = (char *) rreq->dev.user_buf + dt_true_lb;
    }
    else {
        recv_buffer = (char *) MPIU_Malloc(data_sz);
        MPIU_ERR_CHKANDJUMP1(recv_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "Recv Pack Buffer alloc");
        REQ_OFI(rreq)->pack_buffer = recv_buffer;
    }

    /* ---------------- */
    /* Post the receive */
    /* ---------------- */
    FI_RC(fi_trecv(gl_data.endpoint,
                       recv_buffer,
                       data_sz,
                       gl_data.mr,
                       remote_proc,
                       match_bits, mask_bits, &(REQ_OFI(rreq)->ofi_context)), trecv);
    MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
    END_FUNC_RC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_send)
int MPID_nem_ofi_send(struct MPIDI_VC *vc,
                      const void *buf,
                      int count,
                      MPI_Datatype datatype,
                      int dest,
                      int tag, MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;

    BEGIN_FUNC(FCNAME);
    mpi_errno = do_isend(vc, buf, count, datatype, dest, tag,
                         comm, context_offset, request, MPID_NORMAL_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_isend)
int MPID_nem_ofi_isend(struct MPIDI_VC *vc,
                       const void *buf,
                       int count,
                       MPI_Datatype datatype,
                       int dest,
                       int tag, MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = do_isend(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_NORMAL_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_ssend)
int MPID_nem_ofi_ssend(struct MPIDI_VC *vc,
                       const void *buf,
                       int count,
                       MPI_Datatype datatype,
                       int dest,
                       int tag, MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = do_isend(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_SYNC_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_issend)
int MPID_nem_ofi_issend(struct MPIDI_VC *vc,
                        const void *buf,
                        int count,
                        MPI_Datatype datatype,
                        int dest,
                        int tag,
                        MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = do_isend(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_SYNC_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#define DO_CANCEL(req)                                  \
({                                                      \
  int mpi_errno = MPI_SUCCESS;                          \
  int ret;                                              \
  BEGIN_FUNC(FCNAME);                                   \
  MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);             \
  ret = fi_cancel((fid_t)gl_data.endpoint,              \
                  &(REQ_OFI(req)->ofi_context));        \
  if (ret == 0) {                                        \
    MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);      \
  } else {                                              \
    MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);     \
  }                                                     \
  END_FUNC(FCNAME);                                     \
  return mpi_errno;                                     \
})

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_cancel_send)
int MPID_nem_ofi_cancel_send(struct MPIDI_VC *vc ATTRIBUTE((unused)), struct MPID_Request *sreq)
{
    DO_CANCEL(sreq);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_cancel_recv)
int MPID_nem_ofi_cancel_recv(struct MPIDI_VC *vc ATTRIBUTE((unused)), struct MPID_Request *rreq)
{
    DO_CANCEL(rreq);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_posted)
void MPID_nem_ofi_anysource_posted(MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = MPID_nem_ofi_recv_posted(NULL, rreq);
    MPIU_Assert(mpi_errno == MPI_SUCCESS);
    END_FUNC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_matched)
int MPID_nem_ofi_anysource_matched(MPID_Request * rreq)
{
    int mpi_errno = FALSE;
    int ret;
    BEGIN_FUNC(FCNAME);
    /* ----------------------------------------------------- */
    /* Netmod has notified us that it has matched an any     */
    /* source request on another device.  We have the chance */
    /* to cancel this shared request if it has been posted   */
    /* ----------------------------------------------------- */
    ret = fi_cancel((fid_t) gl_data.endpoint, &(REQ_OFI(rreq)->ofi_context));
    if (ret == 0) {
        /* --------------------------------------------------- */
        /* Request cancelled:  cancel and complete the request */
        /* --------------------------------------------------- */
        mpi_errno = TRUE;
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
        MPIDI_CH3U_Request_complete(rreq);
    }
    END_FUNC(FCNAME);
    return mpi_errno;
}
