/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include <mpl_utlist.h>
#include "rptl.h"

#define NUM_RECV_BUFS 2
#define BUFSIZE (1024*1024*50)
#define CTL_TAG 0
#define GET_TAG 1
#define PAYLOAD_SIZE  (PTL_MAX_EAGER - sizeof(MPIDI_CH3_Pkt_t))
#define SENDBUF_SIZE(sent_sz_) (sizeof(MPIDI_CH3_Pkt_t) + (sent_sz_))
#define SENDBUF(req_) REQ_PTL(req_)->chunk_buffer[0]
#define TMPBUF(req_) REQ_PTL(req_)->chunk_buffer[1]

static char *recvbufs[NUM_RECV_BUFS];
static ptl_me_t overflow_me;
static ptl_me_t get_me;
static ptl_handle_me_t me_handles[NUM_RECV_BUFS];
static ptl_handle_me_t get_me_handle;


/* AUX STUFF FOR REORDERING LOGIC */
static GENERIC_Q_DECL(struct MPIR_Request) reorder_queue;
static char *expected_recv_ptr, *max_recv_ptr[NUM_RECV_BUFS];
static int expected_recv_idx;

#undef FUNCNAME
#define FUNCNAME incr_expected_recv_ptr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void incr_expected_recv_ptr(size_t size)
{
    expected_recv_ptr += size > PTL_MAX_EAGER ? PTL_MAX_EAGER : size;
    if (expected_recv_ptr > max_recv_ptr[expected_recv_idx]) {
        ++expected_recv_idx;
        if (expected_recv_idx == NUM_RECV_BUFS)
            expected_recv_idx = 0;
        expected_recv_ptr = recvbufs[expected_recv_idx];
    }
}

#undef FUNCNAME
#define FUNCNAME handle_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int handle_request(MPIR_Request *req)
{
    int mpi_errno = MPID_nem_handle_pkt(req->ch.vc, TMPBUF(req), REQ_PTL(req)->bytes_put);
    incr_expected_recv_ptr(REQ_PTL(req)->bytes_put);
    /* Free resources */
    MPL_free(TMPBUF(req));
    MPIR_Request_free(req);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME progress_reorder
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int progress_reorder(void)
{
    MPIR_Request *req;
    int mpi_errno = MPI_SUCCESS;

    GENERIC_Q_SEARCH_REMOVE(&reorder_queue,
                            REQ_PTL(_e)->recv_ptr == expected_recv_ptr,
                            &req, MPIR_Request, dev.next);
    while (req) {
        mpi_errno = handle_request(req);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        GENERIC_Q_SEARCH_REMOVE(&reorder_queue,
                                REQ_PTL(_e)->recv_ptr == expected_recv_ptr,
                                &req, MPIR_Request, dev.next);
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* END AUX STUFF FOR REORDERING LOGIC */


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    char *tmp_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_PTL_NM_INIT);

    /* init recv */
    overflow_me.length = BUFSIZE;
    overflow_me.ct_handle = PTL_CT_NONE;
    overflow_me.uid = PTL_UID_ANY;
    overflow_me.options = ( PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL | PTL_ME_NO_TRUNCATE | PTL_ME_MAY_ALIGN |
                            PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE );
    overflow_me.match_id.phys.pid = PTL_PID_ANY;
    overflow_me.match_id.phys.nid = PTL_NID_ANY;
    overflow_me.match_bits = NPTL_MATCH(MPI_ANY_TAG, CTL_TAG, MPI_ANY_SOURCE);
    overflow_me.ignore_bits = NPTL_MATCH_IGNORE_ANY_TAG;
    overflow_me.min_free = PTL_MAX_EAGER;

    /* allocate all overflow space at once */
    tmp_ptr = MPL_malloc(NUM_RECV_BUFS * BUFSIZE);
    expected_recv_ptr = tmp_ptr;
    expected_recv_idx = 0;

    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        recvbufs[i] = tmp_ptr;
        overflow_me.start = tmp_ptr;
        ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &overflow_me,
                          PTL_OVERFLOW_LIST, (void *)(size_t)i, &me_handles[i]);
        MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s",
                             MPID_nem_ptl_strerror(ret));
        tmp_ptr += BUFSIZE;
        max_recv_ptr[i] = tmp_ptr - overflow_me.min_free;
    }

    /* register persistent ME for GET operations */
    get_me.start = NULL;
    get_me.length = PTL_SIZE_MAX;
    get_me.ct_handle = PTL_CT_NONE;
    get_me.uid = PTL_UID_ANY;
    get_me.options = ( PTL_ME_OP_GET | PTL_ME_IS_ACCESSIBLE | PTL_ME_NO_TRUNCATE |
                       PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE );
    get_me.match_id.phys.pid = PTL_PID_ANY;
    get_me.match_id.phys.nid = PTL_NID_ANY;
    get_me.match_bits = NPTL_MATCH(MPI_ANY_TAG, GET_TAG, MPI_ANY_SOURCE);
    get_me.ignore_bits = NPTL_MATCH_IGNORE_ANY_TAG;
    get_me.min_free = 0;
    ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &get_me, PTL_PRIORITY_LIST, NULL,
                      &get_me_handle);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));

    /* init the reorder queue */
    reorder_queue.head = reorder_queue.tail = NULL;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_PTL_NM_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);

    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        ret = PtlMEUnlink(me_handles[i]);
        MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink", "**ptlmeunlink %s",
                             MPID_nem_ptl_strerror(ret));
    }

    ret = PtlMEUnlink(get_me_handle);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink", "**ptlmeunlink %s",
                         MPID_nem_ptl_strerror(ret));

    /* Freeing first element because the allocation was a single contiguous buffer */
    MPL_free(recvbufs[0]);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME send_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int send_pkt(MPIDI_VC_t *vc, void *hdr_p, void *data_p, intptr_t data_sz,
                           MPIR_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    char *sendbuf, *sendbuf_ptr;
    const size_t sent_sz = data_sz < (PAYLOAD_SIZE-sreq->dev.ext_hdr_sz) ? data_sz : (PAYLOAD_SIZE-sreq->dev.ext_hdr_sz-sizeof(ptl_size_t));
    const size_t remaining = data_sz - sent_sz;
    const size_t sendbuf_sz = SENDBUF_SIZE(sent_sz+sreq->dev.ext_hdr_sz+(remaining?sizeof(ptl_size_t):0));
    ptl_match_bits_t match_bits = NPTL_MATCH(sreq->handle, CTL_TAG, MPIDI_Process.my_pg_rank);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_PKT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_PKT);
    
    sendbuf = MPL_malloc(sendbuf_sz);
    MPIR_Assert(sendbuf != NULL);
    MPIR_Memcpy(sendbuf, hdr_p, sizeof(MPIDI_CH3_Pkt_t));
    sendbuf_ptr = sendbuf + sizeof(MPIDI_CH3_Pkt_t);

    if (sreq->dev.ext_hdr_sz > 0) {
        /* copy extended packet header to send buf */
        MPIR_Memcpy(sendbuf_ptr, sreq->dev.ext_hdr_ptr, sreq->dev.ext_hdr_sz);
        sendbuf_ptr += sreq->dev.ext_hdr_sz;
    }

    TMPBUF(sreq) = NULL;
    REQ_PTL(sreq)->num_gets = 0;
    REQ_PTL(sreq)->put_done = 0;

    if (data_sz) {
        MPIR_Memcpy(sendbuf_ptr, data_p, sent_sz);
        sendbuf_ptr += sent_sz;
        if (remaining) {
            /* The address/offset for the remote side to do the get is last in the buffer */
            ptl_size_t *offset = (ptl_size_t *)sendbuf_ptr;
            *offset = (ptl_size_t)data_p + sent_sz;
            REQ_PTL(sreq)->num_gets = remaining / MPIDI_nem_ptl_ni_limits.max_msg_size;
            if (remaining % MPIDI_nem_ptl_ni_limits.max_msg_size) REQ_PTL(sreq)->num_gets++;
        }
    }

    SENDBUF(sreq) = sendbuf;
    sreq->ch.vc = vc;
    REQ_PTL(sreq)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;

    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)sendbuf, sendbuf_sz, PTL_NO_ACK_REQ,
                                vc_ptl->id, vc_ptl->ptc, match_bits, 0, sreq, NPTL_HEADER(0, remaining));
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s",
                         MPID_nem_ptl_strerror(ret));
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE, (MPL_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x)",
                                            sendbuf_sz, vc_ptl->id.phys.nid,
                                            vc_ptl->id.phys.pid, vc_ptl->ptc));

    vc_ptl->num_queued_sends++;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_PKT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME send_noncontig_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int send_noncontig_pkt(MPIDI_VC_t *vc, MPIR_Request *sreq, void *hdr_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    char *sendbuf, *sendbuf_ptr;
    const size_t data_sz = sreq->dev.segment_size - sreq->dev.segment_first;
    const size_t sent_sz = data_sz < (PAYLOAD_SIZE-sreq->dev.ext_hdr_sz) ? data_sz : (PAYLOAD_SIZE-sreq->dev.ext_hdr_sz-sizeof(ptl_size_t));
    const size_t remaining = data_sz - sent_sz;
    const size_t sendbuf_sz = SENDBUF_SIZE(sent_sz+sreq->dev.ext_hdr_sz+(remaining?sizeof(ptl_size_t):0));
    ptl_match_bits_t match_bits = NPTL_MATCH(sreq->handle, CTL_TAG, MPIDI_Process.my_pg_rank);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_NONCONTIG_PKT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_NONCONTIG_PKT);

    sendbuf = MPL_malloc(sendbuf_sz);
    MPIR_Assert(sendbuf != NULL);
    MPIR_Memcpy(sendbuf, hdr_p, sizeof(MPIDI_CH3_Pkt_t));
    sendbuf_ptr = sendbuf + sizeof(MPIDI_CH3_Pkt_t);

    if (sreq->dev.ext_hdr_sz > 0) {
        /* copy extended packet header to send buf */
        MPIR_Memcpy(sendbuf_ptr, sreq->dev.ext_hdr_ptr, sreq->dev.ext_hdr_sz);
        sendbuf_ptr += sreq->dev.ext_hdr_sz;
    }

    TMPBUF(sreq) = NULL;
    REQ_PTL(sreq)->num_gets = 0;
    REQ_PTL(sreq)->put_done = 0;

    if (data_sz) {
        intptr_t first = sreq->dev.segment_first;
        intptr_t last = sreq->dev.segment_first + sent_sz;
        MPIR_Segment_pack(sreq->dev.segment_ptr, first, &last, sendbuf_ptr);
        sendbuf_ptr += sent_sz;

        if (remaining) {  /* Post MEs for the remote gets */
            ptl_size_t *offset = (ptl_size_t *)sendbuf_ptr;
            TMPBUF(sreq) = MPL_malloc(remaining);
            *offset = (ptl_size_t)TMPBUF(sreq);
            first = last;
            last = sreq->dev.segment_size;
            MPIR_Segment_pack(sreq->dev.segment_ptr, first, &last, TMPBUF(sreq));
            MPIR_Assert(last == sreq->dev.segment_size);

            REQ_PTL(sreq)->num_gets = remaining / MPIDI_nem_ptl_ni_limits.max_msg_size;
            if (remaining % MPIDI_nem_ptl_ni_limits.max_msg_size) REQ_PTL(sreq)->num_gets++;
        }
    }

    SENDBUF(sreq) = sendbuf;
    sreq->ch.vc = vc;
    REQ_PTL(sreq)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;

    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)sendbuf, sendbuf_sz, PTL_NO_ACK_REQ,
                                vc_ptl->id, vc_ptl->ptc, match_bits, 0, sreq, NPTL_HEADER(0, remaining));
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s",
                         MPID_nem_ptl_strerror(ret));
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE, (MPL_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x)",
                                            sendbuf_sz, vc_ptl->id.phys.nid,
                                            vc_ptl->id.phys.pid, vc_ptl->ptc));

    vc_ptl->num_queued_sends++;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_NONCONTIG_PKT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_SendNoncontig
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_SendNoncontig(MPIDI_VC_t *vc, MPIR_Request *sreq, void *hdr, intptr_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);
    
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    mpi_errno = send_noncontig_pkt(vc, sreq, hdr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_iStartContigMsg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, intptr_t hdr_sz, void *data,
                                 intptr_t data_sz, MPIR_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* create a request */
    *sreq_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_Assert(*sreq_ptr != NULL);
    MPIR_Object_set_ref(*sreq_ptr, 2);
    (*sreq_ptr)->kind = MPIR_REQUEST_KIND__SEND;
    (*sreq_ptr)->dev.OnDataAvail = NULL;
    (*sreq_ptr)->dev.user_buf = NULL;

    mpi_errno = send_pkt(vc, hdr, data, data_sz, *sreq_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_iSendContig
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_iSendContig(MPIDI_VC_t *vc, MPIR_Request *sreq, void *hdr, intptr_t hdr_sz,
                               void *data, intptr_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    mpi_errno = send_pkt(vc, hdr, data, data_sz, sreq);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME on_data_avail
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int on_data_avail(MPIR_Request * req)
{
    int (*reqFn) (MPIDI_VC_t *, MPIR_Request *, int *);
    MPIDI_VC_t *vc = req->ch.vc;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ON_DATA_AVAIL);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ON_DATA_AVAIL);

    reqFn = req->dev.OnDataAvail;
    if (!reqFn) {
        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
    }
    else {
        int complete;
        reqFn(vc, req, &complete);
        MPIR_Assert(complete == TRUE);
    }

    vc_ptl->num_queued_sends--;

    if (vc->state == MPIDI_VC_STATE_CLOSED && vc_ptl->num_queued_sends == 0)
        MPID_nem_ptl_vc_terminated(vc);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ON_DATA_AVAIL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_ctl_event_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_ctl_event_handler(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);

    switch(e->type) {

    case PTL_EVENT_PUT:
        {
            int ret;
            const size_t packet_sz = e->mlength;
            MPIDI_VC_t *vc;
            MPID_nem_ptl_vc_area * vc_ptl;
            ptl_size_t remaining = NPTL_HEADER_GET_LENGTH(e->hdr_data);
            ptl_me_t search_me;
            MPI_Request sender_req_id = NPTL_MATCH_GET_TAG(e->match_bits);

            MPIDI_PG_Get_vc(MPIDI_Process.my_pg, NPTL_MATCH_GET_RANK(e->match_bits), &vc);
            vc_ptl = VC_PTL(vc);

            if (remaining == 0) {
                if (e->start == expected_recv_ptr) {
                    incr_expected_recv_ptr(packet_sz);
                    mpi_errno = MPID_nem_handle_pkt(vc, e->start, packet_sz);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    mpi_errno = progress_reorder();
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
                else {
                    MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
                    /* This request is actually complete; just needs to wait to enforce ordering */
                    TMPBUF(req) = MPL_malloc(packet_sz);
                    MPIR_Assert(TMPBUF(req));
                    MPIR_Memcpy(TMPBUF(req), e->start, packet_sz);
                    REQ_PTL(req)->bytes_put = packet_sz;
                    req->ch.vc = vc;
                    REQ_PTL(req)->recv_ptr = e->start;
                    GENERIC_Q_ENQUEUE(&reorder_queue, req, dev.next);
                }
            }
            else {
                int incomplete;
                size_t size;
                char *buf_ptr;
                ptl_size_t target_offset;

                MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
                MPIR_Assert(req != NULL);
                MPIDI_CH3U_Request_decrement_cc(req, &incomplete);  /* We'll increment it below */
                REQ_PTL(req)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;
                REQ_PTL(req)->bytes_put = packet_sz + remaining - sizeof(ptl_size_t);
                TMPBUF(req) = MPL_malloc(REQ_PTL(req)->bytes_put);
                MPIR_Assert(TMPBUF(req) != NULL);
                MPIR_Memcpy(TMPBUF(req), e->start, packet_sz);
                REQ_PTL(req)->recv_ptr = e->start;
                req->ch.vc = vc;

                buf_ptr = (char *)TMPBUF(req) + packet_sz - sizeof(ptl_size_t);
                target_offset = *((ptl_size_t *)buf_ptr);
                do {
                    size = MPL_MIN(remaining, MPIDI_nem_ptl_ni_limits.max_msg_size);
                    MPIDI_CH3U_Request_increment_cc(req, &incomplete);  /* Will be decremented - and eventually freed in REPLY */
                    ret = MPID_nem_ptl_rptl_get(MPIDI_nem_ptl_global_md, (ptl_size_t)buf_ptr,
                                                size, vc_ptl->id, vc_ptl->ptc, NPTL_MATCH(sender_req_id, GET_TAG, MPIDI_Process.my_pg_rank), target_offset, req);
                    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlget", "**ptlget %s",
                                         MPID_nem_ptl_strerror(ret));
                    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE, (MPL_DBG_FDEST,
                                                            "PtlGet(size=%lu id=(%#x,%#x) pt=%#x tag=%d)", size,
                                                            vc_ptl->id.phys.nid,
                                                            vc_ptl->id.phys.pid, vc_ptl->ptc, sender_req_id));
                    buf_ptr += size;
                    remaining -= size;
                    target_offset += size;
                } while (remaining);
            }

            /* FIXME: this search/delete not be necessary if we set PTL_ME_UNEXPECTED_HDR_DISABLE
               on the overflow list entries. However, doing so leads to PTL_IN_USE errors
               during finalize in rare cases, even though all messages are handled. */
            memcpy(&search_me, &overflow_me, sizeof(ptl_me_t));
            search_me.options |= PTL_ME_USE_ONCE;
            /* find and delete the header for this message in the overflow list */
            PtlMESearch(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &search_me, PTL_SEARCH_DELETE, NULL);
        }
        break;

    case PTL_EVENT_GET:
        {
            MPI_Request handle = NPTL_MATCH_GET_TAG(e->match_bits);
            MPIR_Request *req = NULL;

            MPIR_Request_get_ptr(handle, req);
            if (--REQ_PTL(req)->num_gets == 0) {
                MPL_free(TMPBUF(req));
                if (REQ_PTL(req)->put_done) {
                    mpi_errno = on_data_avail(req);  /* Otherwise we'll do it on the SEND */
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIR_ERR_POP(mpi_errno);
                    }
                }
            }
        }
        break;

    case PTL_EVENT_SEND:
        {
            MPIR_Request *const req = e->user_ptr;

            MPL_free(SENDBUF(req));
            REQ_PTL(req)->put_done = 1;
            if (REQ_PTL(req)->num_gets == 0) {  /* Otherwise GET will do it */
                mpi_errno = on_data_avail(req);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        break;

    case PTL_EVENT_REPLY:
        {
            int incomplete;
            MPIR_Request *const req = e->user_ptr;

            MPIDI_CH3U_Request_decrement_cc(req, &incomplete);
            if (!incomplete) {
                if (REQ_PTL(req)->recv_ptr == expected_recv_ptr) {
                    mpi_errno = handle_request(req);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    mpi_errno = progress_reorder();
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
                else
                    GENERIC_Q_ENQUEUE(&reorder_queue, req, dev.next);
            }
        }
        break;

    case PTL_EVENT_AUTO_FREE:
        {
            size_t buf_idx = (size_t)e->user_ptr;
            int ret;

            overflow_me.start = recvbufs[buf_idx];

            ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &overflow_me,
                              PTL_OVERFLOW_LIST, e->user_ptr, &me_handles[buf_idx]);
            MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s",
                                 MPID_nem_ptl_strerror(ret));
        }

    case PTL_EVENT_AUTO_UNLINK:
    case PTL_EVENT_PUT_OVERFLOW:
        break;

    default:
        MPL_error_printf("Received unexpected event type: %d %s\n", e->type, MPID_nem_ptl_strevent(e));
        MPIR_ERR_INTERNALANDJUMP(mpi_errno, "Unexpected event type");
        break;
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
