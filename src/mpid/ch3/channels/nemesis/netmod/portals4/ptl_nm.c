/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
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

static char *recvbufs;
static ptl_me_t overflow_me;
static ptl_handle_me_t me_handles[NUM_RECV_BUFS];

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_NM_INIT);

    /* init recv */
    overflow_me.length = BUFSIZE;
    overflow_me.ct_handle = PTL_CT_NONE;
    overflow_me.uid = PTL_UID_ANY;
    overflow_me.options = ( PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL | PTL_ME_NO_TRUNCATE | PTL_ME_MAY_ALIGN |
                   PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE );
    overflow_me.match_id.phys.pid = PTL_PID_ANY;
    overflow_me.match_id.phys.nid = PTL_NID_ANY;
    overflow_me.match_bits = NPTL_MATCH(CTL_TAG, 0, MPI_ANY_SOURCE);
    overflow_me.ignore_bits = NPTL_MATCH_IGNORE;
    overflow_me.min_free = PTL_MAX_EAGER;

    /* allocate all overflow space at once */
    recvbufs = MPIU_Malloc(NUM_RECV_BUFS * BUFSIZE);

    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        overflow_me.start = recvbufs + (i * BUFSIZE);
        ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &overflow_me,
                          PTL_OVERFLOW_LIST, (void *)(size_t)i, &me_handles[i]);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s",
                             MPID_nem_ptl_strerror(ret));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_NM_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_finalize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);

    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        ret = PtlMEUnlink(me_handles[i]);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink", "**ptlmeunlink %s",
                             MPID_nem_ptl_strerror(ret));
    }

    MPIU_Free(recvbufs);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME meappend_large
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int meappend_large(ptl_process_t id, MPID_Request *req, ptl_match_bits_t match_bits, void *buf, size_t remaining)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    ptl_me_t me;
    MPIDI_STATE_DECL(MPID_STATE_MEAPPEND_LARGE);

    MPIDI_FUNC_ENTER(MPID_STATE_MEAPPEND_LARGE);

    me.start = buf;
    me.length = remaining < MPIDI_nem_ptl_ni_limits.max_msg_size ?
                    remaining : MPIDI_nem_ptl_ni_limits.max_msg_size;
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = ( PTL_ME_OP_GET | PTL_ME_USE_ONCE | PTL_ME_IS_ACCESSIBLE |
                   PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE );
    me.match_id = id;
    me.match_bits = match_bits;
    me.ignore_bits = NPTL_MATCH_IGNORE;
    me.min_free = 0;

    while (remaining) {
        ptl_handle_me_t foo_me_handle;

        ++REQ_PTL(req)->num_gets;

        ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &me, PTL_PRIORITY_LIST, req,
                          &foo_me_handle);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s",
                             MPID_nem_ptl_strerror(ret));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlMEAppend(req=%p tag=%#lx)", req, NPTL_MATCH_GET_TAG(match_bits)));

        me.start = (char *)me.start + me.length;
        remaining -= me.length;
        if (remaining < MPIDI_nem_ptl_ni_limits.max_msg_size)
            me.length = remaining;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MEAPPEND_LARGE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME send_pkt
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int send_pkt(MPIDI_VC_t *vc, void *hdr_p, void *data_p, MPIDI_msg_sz_t data_sz,
                           MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    char *sendbuf;
    const size_t sent_sz = data_sz < PAYLOAD_SIZE ? data_sz : PAYLOAD_SIZE;
    const size_t sendbuf_sz = SENDBUF_SIZE(sent_sz);
    const size_t remaining = data_sz - sent_sz;
    ptl_match_bits_t match_bits = NPTL_MATCH(CTL_TAG, 0, MPIDI_Process.my_pg_rank);
    MPIDI_STATE_DECL(MPID_STATE_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_PKT);
    
    sendbuf = MPIU_Malloc(sendbuf_sz);
    MPIU_Assert(sendbuf != NULL);
    MPIU_Memcpy(sendbuf, hdr_p, sizeof(MPIDI_CH3_Pkt_t));
    TMPBUF(sreq) = NULL;
    REQ_PTL(sreq)->num_gets = 0;
    REQ_PTL(sreq)->put_done = 0;

    if (data_sz) {
        MPIU_Memcpy(sendbuf + sizeof(MPIDI_CH3_Pkt_t), data_p, sent_sz);
        if (remaining)  /* Post MEs for the remote gets */
            mpi_errno = meappend_large(vc_ptl->id, sreq, NPTL_MATCH(GET_TAG, 0, MPIDI_Process.my_pg_rank),
                                       (char *)data_p + sent_sz, remaining);
            if (mpi_errno)
                goto fn_fail;
    }

    SENDBUF(sreq) = sendbuf;
    sreq->ch.vc = vc;
    REQ_PTL(sreq)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;

    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)sendbuf, sendbuf_sz, PTL_NO_ACK_REQ,
                                vc_ptl->id, vc_ptl->ptc, match_bits, 0, sreq, NPTL_HEADER(0, remaining));
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s",
                         MPID_nem_ptl_strerror(ret));
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x)",
                                            sendbuf_sz, vc_ptl->id.phys.nid,
                                            vc_ptl->id.phys.pid, vc_ptl->ptc));

    vc_ptl->num_queued_sends++;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_PKT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME send_noncontig_pkt
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int send_noncontig_pkt(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    char *sendbuf;
    const size_t data_sz = sreq->dev.segment_size - sreq->dev.segment_first;
    const size_t sent_sz = data_sz < PAYLOAD_SIZE ? data_sz : PAYLOAD_SIZE;
    const size_t sendbuf_sz = SENDBUF_SIZE(sent_sz);
    const size_t remaining = data_sz - sent_sz;
    ptl_match_bits_t match_bits = NPTL_MATCH(CTL_TAG, 0, MPIDI_Process.my_pg_rank);
    MPIDI_STATE_DECL(MPID_STATE_SEND_NONCONTIG_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_SEND_NONCONTIG_PKT);

    sendbuf = MPIU_Malloc(sendbuf_sz);
    MPIU_Assert(sendbuf != NULL);
    MPIU_Memcpy(sendbuf, hdr_p, sizeof(MPIDI_CH3_Pkt_t));
    TMPBUF(sreq) = NULL;
    REQ_PTL(sreq)->num_gets = 0;
    REQ_PTL(sreq)->put_done = 0;

    if (data_sz) {
        MPIDI_msg_sz_t first = sreq->dev.segment_first;
        MPIDI_msg_sz_t last = sreq->dev.segment_first + sent_sz;
        MPID_Segment_pack(sreq->dev.segment_ptr, first, &last, sendbuf + sizeof(MPIDI_CH3_Pkt_t));

        if (remaining) {  /* Post MEs for the remote gets */
            TMPBUF(sreq) = MPIU_Malloc(remaining);
            first = last;
            last = sreq->dev.segment_size;
            MPID_Segment_pack(sreq->dev.segment_ptr, first, &last, TMPBUF(sreq));
            MPIU_Assert(last == sreq->dev.segment_size);

            mpi_errno = meappend_large(vc_ptl->id, sreq, NPTL_MATCH(GET_TAG, 0, MPIDI_Process.my_pg_rank), TMPBUF(sreq), remaining);
            if (mpi_errno)
                goto fn_fail;
        }
    }

    SENDBUF(sreq) = sendbuf;
    sreq->ch.vc = vc;
    REQ_PTL(sreq)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;

    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)sendbuf, sendbuf_sz, PTL_NO_ACK_REQ,
                                vc_ptl->id, vc_ptl->ptc, match_bits, 0, sreq, NPTL_HEADER(0, remaining));
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s",
                         MPID_nem_ptl_strerror(ret));
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x)",
                                            sendbuf_sz, vc_ptl->id.phys.nid,
                                            vc_ptl->id.phys.pid, vc_ptl->ptc));

    vc_ptl->num_queued_sends++;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_NONCONTIG_PKT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_SendNoncontig
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    mpi_errno = send_noncontig_pkt(vc, sreq, hdr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_iStartContigMsg
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data,
                                 MPIDI_msg_sz_t data_sz, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* create a request */
    *sreq_ptr = MPID_Request_create();
    MPIU_Assert(*sreq_ptr != NULL);
    MPIU_Object_set_ref(*sreq_ptr, 2);
    (*sreq_ptr)->kind = MPID_REQUEST_SEND;
    (*sreq_ptr)->dev.OnDataAvail = NULL;
    (*sreq_ptr)->dev.user_buf = NULL;

    mpi_errno = send_pkt(vc, hdr, data, data_sz, *sreq_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_iSendContig
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                               void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    mpi_errno = send_pkt(vc, hdr, data, data_sz, sreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME on_data_avail
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline void on_data_avail(MPID_Request * req)
{
    int (*reqFn) (MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_VC_t *vc = req->ch.vc;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);

    MPIDI_STATE_DECL(MPID_STATE_ON_DATA_AVAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_ON_DATA_AVAIL);

    reqFn = req->dev.OnDataAvail;
    if (!reqFn) {
        MPIDI_CH3U_Request_complete(req);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
    }
    else {
        int complete;
        reqFn(vc, req, &complete);
        MPIU_Assert(complete == TRUE);
    }

    vc_ptl->num_queued_sends--;

    if (vc->state == MPIDI_VC_STATE_CLOSED && vc_ptl->num_queued_sends == 0)
        MPID_nem_ptl_vc_terminated(vc);

    MPIDI_FUNC_EXIT(MPID_STATE_ON_DATA_AVAIL);
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_ctl_event_handler
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_ctl_event_handler(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);

    switch(e->type) {

    case PTL_EVENT_PUT:
        {
            int ret;
            const size_t packet_sz = e->mlength;
            MPIDI_VC_t *vc;
            MPID_nem_ptl_vc_area * vc_ptl;
            ptl_size_t remaining = NPTL_HEADER_GET_LENGTH(e->hdr_data);
            ptl_me_t search_me;

            MPIDI_PG_Get_vc(MPIDI_Process.my_pg, NPTL_MATCH_GET_RANK(e->match_bits), &vc);
            vc_ptl = VC_PTL(vc);

            if (remaining == 0) {
                mpi_errno = MPID_nem_handle_pkt(vc, e->start, packet_sz);
                if (mpi_errno)
                    MPIU_ERR_POP(mpi_errno);
            }
            else {
                int incomplete;
                size_t size;
                char *buf_ptr;

                MPID_Request *req = MPID_Request_create();
                MPIU_Assert(req != NULL);
                MPIDI_CH3U_Request_decrement_cc(req, &incomplete);  /* We'll increment it below */
                REQ_PTL(req)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;
                REQ_PTL(req)->bytes_put = packet_sz + remaining;
                TMPBUF(req) = MPIU_Malloc(REQ_PTL(req)->bytes_put);
                MPIU_Assert(TMPBUF(req) != NULL);
                MPIU_Memcpy(TMPBUF(req), e->start, packet_sz);

                req->ch.vc = vc;

                size = remaining < MPIDI_nem_ptl_ni_limits.max_msg_size ? remaining : MPIDI_nem_ptl_ni_limits.max_msg_size;
                buf_ptr = (char *)TMPBUF(req) + packet_sz;
                while (remaining) {
                    MPIDI_CH3U_Request_increment_cc(req, &incomplete);  /* Will be decremented - and eventually freed in REPLY */
                    ret = MPID_nem_ptl_rptl_get(MPIDI_nem_ptl_global_md, (ptl_size_t)buf_ptr,
                                                size, vc_ptl->id, vc_ptl->ptc, NPTL_MATCH(GET_TAG, 0, MPIDI_Process.my_pg_rank), 0, req);
                    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlget", "**ptlget %s",
                                         MPID_nem_ptl_strerror(ret));
                    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,
                                                            "PtlGet(size=%lu id=(%#x,%#x) pt=%#x tag=%d)", size,
                                                            vc_ptl->id.phys.nid,
                                                            vc_ptl->id.phys.pid, vc_ptl->ptc, GET_TAG));
                    buf_ptr += size;
                    remaining -= size;
                    if (remaining < MPIDI_nem_ptl_ni_limits.max_msg_size)
                        size = remaining;
                }
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
            MPID_Request *const req = e->user_ptr;

            if (--REQ_PTL(req)->num_gets == 0) {
                MPIU_Free(TMPBUF(req));
                if (REQ_PTL(req)->put_done)
                    on_data_avail(req);  /* Otherwise we'll do it on the SEND */
            }
        }
        break;

    case PTL_EVENT_SEND:
        {
            MPID_Request *const req = e->user_ptr;

            MPIU_Free(SENDBUF(req));
            REQ_PTL(req)->put_done = 1;
            if (REQ_PTL(req)->num_gets == 0)  /* Otherwise GET will do it */
                on_data_avail(req);
        }
        break;

    case PTL_EVENT_REPLY:
        {
            int incomplete;
            MPID_Request *const req = e->user_ptr;

            MPIDI_CH3U_Request_decrement_cc(req, &incomplete);
            if (!incomplete) {
                mpi_errno = MPID_nem_handle_pkt(req->ch.vc, TMPBUF(req), REQ_PTL(req)->bytes_put);
                if (mpi_errno)
                    MPIU_ERR_POP(mpi_errno);

                /* Free resources */
                MPIU_Free(TMPBUF(req));
                MPID_Request_release(req);
            }
        }
        break;

    case PTL_EVENT_AUTO_FREE:
        {
            size_t buf_idx = (size_t)e->user_ptr;
            int ret;

            overflow_me.start = recvbufs + (buf_idx * BUFSIZE);

            ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &overflow_me,
                              PTL_OVERFLOW_LIST, e->user_ptr, &me_handles[buf_idx]);
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s",
                                 MPID_nem_ptl_strerror(ret));
        }

    case PTL_EVENT_AUTO_UNLINK:
    case PTL_EVENT_PUT_OVERFLOW:
        break;

    default:
        MPIU_Error_printf("Received unexpected event type: %d %s\n", e->type, MPID_nem_ptl_strevent(e));
        MPIU_ERR_INTERNALANDJUMP(mpi_errno, "Unexpected event type");
        break;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
