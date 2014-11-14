/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "stddef.h"  /* C99; for offsetof */
#include <mpl_utlist.h>
#include "rptl.h"

#define NUM_RECV_BUFS 50
#define CTL_TAG 0
#define PAYLOAD_SIZE  (PTL_MAX_EAGER - offsetof(buf_t, packet) - sizeof(MPIDI_CH3_Pkt_t))
#define SENDBUF_SIZE(sent_sz_) (offsetof(buf_t, packet) + sizeof(MPIDI_CH3_Pkt_t) + (sent_sz_))
#define SENDBUF(req_) REQ_PTL(req_)->chunk_buffer[0]
#define TMPBUF(req_) REQ_PTL(req_)->chunk_buffer[1]
#define NEW_TAG(tag_) do {       \
    if (++global_tag == CTL_TAG) \
        ++global_tag;            \
    (tag_) = global_tag;         \
} while(0)

typedef struct {
    size_t remaining;
    ptl_match_bits_t tag;
    char packet[PTL_MAX_EAGER];
} buf_t;

static buf_t recvbufs[NUM_RECV_BUFS];
static ptl_me_t mes[NUM_RECV_BUFS];
static ptl_handle_me_t me_handles[NUM_RECV_BUFS];
static unsigned long long put_cnt = 0;  /* required to not finalizing too early */
static ptl_match_bits_t global_tag = 0;


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    ptl_process_t id_any;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_NM_INIT);

    /* init recv */
    id_any.phys.pid = PTL_PID_ANY;
    id_any.phys.nid = PTL_NID_ANY;
    
    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        mes[i].start = &recvbufs[i];
        mes[i].length = sizeof(buf_t);
        mes[i].ct_handle = PTL_CT_NONE;
        mes[i].uid = PTL_UID_ANY;
        mes[i].options = (PTL_ME_OP_PUT | PTL_ME_USE_ONCE | PTL_ME_EVENT_UNLINK_DISABLE |
                         PTL_ME_EVENT_LINK_DISABLE | PTL_ME_IS_ACCESSIBLE);
        mes[i].match_id = id_any;
        mes[i].match_bits = CTL_TAG;
        mes[i].ignore_bits = 0;

        ret = MPID_nem_ptl_me_append(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &mes[i],
                                     PTL_PRIORITY_LIST, (void *)(uint64_t)i, &me_handles[i]);
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

    while (put_cnt) MPID_nem_ptl_poll(1);  /* Wait for puts to finish */

    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        ret = PtlMEUnlink(me_handles[i]);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink", "**ptlmeunlink %s",
                             MPID_nem_ptl_strerror(ret));
    }

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
static inline int meappend_large(ptl_process_t id, MPID_Request *req, ptl_match_bits_t tag, void *buf, size_t remaining)
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
    me.match_bits = tag;
    me.ignore_bits = 0;
    me.min_free = 0;

    while (remaining) {
        ptl_handle_me_t foo_me_handle;

        ++REQ_PTL(req)->num_gets;

        ret = MPID_nem_ptl_me_append(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &me, PTL_PRIORITY_LIST, req,
                                     &foo_me_handle);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s",
                             MPID_nem_ptl_strerror(ret));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlMEAppend(req=%p tag=%#lx)", req, tag));

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
    buf_t *sendbuf;
    const size_t sent_sz = data_sz < PAYLOAD_SIZE ? data_sz : PAYLOAD_SIZE;
    const size_t sendbuf_sz = SENDBUF_SIZE(sent_sz);
    MPIDI_STATE_DECL(MPID_STATE_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_PKT);
    
    sendbuf = MPIU_Malloc(sendbuf_sz);
    MPIU_Assert(sendbuf != NULL);
    MPIU_Memcpy(sendbuf->packet, hdr_p, sizeof(MPIDI_CH3_Pkt_t));
    sendbuf->remaining = data_sz - sent_sz;
    NEW_TAG(sendbuf->tag);
    TMPBUF(sreq) = NULL;
    REQ_PTL(sreq)->num_gets = 0;
    REQ_PTL(sreq)->put_done = 0;

    if (data_sz) {
        MPIU_Memcpy(sendbuf->packet + sizeof(MPIDI_CH3_Pkt_t), data_p, sent_sz);
        if (sendbuf->remaining)  /* Post MEs for the remote gets */
            mpi_errno = meappend_large(vc_ptl->id, sreq, sendbuf->tag, (char *)data_p + sent_sz, sendbuf->remaining);
            if (mpi_errno)
                goto fn_fail;
    }

    SENDBUF(sreq) = sendbuf;
    REQ_PTL(sreq)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;

    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)sendbuf, sendbuf_sz, PTL_NO_ACK_REQ,
                                vc_ptl->id, vc_ptl->ptc, CTL_TAG, 0, sreq, MPIDI_Process.my_pg_rank);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s",
                         MPID_nem_ptl_strerror(ret));
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x)",
                                            sendbuf_sz, vc_ptl->id.phys.nid,
                                            vc_ptl->id.phys.pid, vc_ptl->ptc));
    ++put_cnt;

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
    buf_t *sendbuf;
    const size_t sent_sz = sreq->dev.segment_size < PAYLOAD_SIZE ? sreq->dev.segment_size : PAYLOAD_SIZE;
    size_t sendbuf_sz = SENDBUF_SIZE(sent_sz);
    MPIDI_STATE_DECL(MPID_STATE_SEND_NONCONTIG_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_SEND_NONCONTIG_PKT);

    MPIU_Assert(sreq->dev.segment_first == 0);

    sendbuf = MPIU_Malloc(sendbuf_sz);
    MPIU_Assert(sendbuf != NULL);
    MPIU_Memcpy(sendbuf->packet, hdr_p, sizeof(MPIDI_CH3_Pkt_t));
    sendbuf->remaining = sreq->dev.segment_size - sent_sz;
    NEW_TAG(sendbuf->tag);
    TMPBUF(sreq) = NULL;
    REQ_PTL(sreq)->num_gets = 0;
    REQ_PTL(sreq)->put_done = 0;

    if (sreq->dev.segment_size) {
        MPIDI_msg_sz_t last = sent_sz;
        MPID_Segment_pack(sreq->dev.segment_ptr, 0, &last, sendbuf->packet + sizeof(MPIDI_CH3_Pkt_t));

        if (sendbuf->remaining) {  /* Post MEs for the remote gets */
            TMPBUF(sreq) = MPIU_Malloc(sendbuf->remaining);
            sreq->dev.segment_first = last;
            last = sreq->dev.segment_size;
            MPID_Segment_pack(sreq->dev.segment_ptr, sreq->dev.segment_first, &last, TMPBUF(sreq));
            MPIU_Assert(last == sreq->dev.segment_size);

            mpi_errno = meappend_large(vc_ptl->id, sreq, sendbuf->tag, TMPBUF(sreq), sendbuf->remaining);
            if (mpi_errno)
                goto fn_fail;
        }
    }

    SENDBUF(sreq) = sendbuf;
    REQ_PTL(sreq)->event_handler = MPID_nem_ptl_nm_ctl_event_handler;

    ret = MPID_nem_ptl_rptl_put(MPIDI_nem_ptl_global_md, (ptl_size_t)sendbuf, sendbuf_sz, PTL_NO_ACK_REQ,
                                vc_ptl->id, vc_ptl->ptc, CTL_TAG, 0, sreq, MPIDI_Process.my_pg_rank);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s",
                         MPID_nem_ptl_strerror(ret));
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x)",
                                            sendbuf_sz, vc_ptl->id.phys.nid,
                                            vc_ptl->id.phys.pid, vc_ptl->ptc));
    ++put_cnt;

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
    MPIDI_STATE_DECL(MPID_STATE_ON_DATA_AVAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_ON_DATA_AVAIL);

    reqFn = req->dev.OnDataAvail;
    if (!reqFn) {
        MPIDI_CH3U_Request_complete(req);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
    }
    else {
        int complete;
        MPIDI_VC_t *vc = req->ch.vc;
        reqFn(vc, req, &complete);
        MPIU_Assert(complete == TRUE);
    }

    --put_cnt;

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
            const uint64_t buf_idx = (uint64_t) e->user_ptr;
            const size_t packet_sz = e->mlength - offsetof(buf_t, packet);
            MPIDI_VC_t *vc;
            MPID_nem_ptl_vc_area * vc_ptl;

            MPIU_Assert(e->start == &recvbufs[buf_idx]);

            MPIDI_PG_Get_vc(MPIDI_Process.my_pg, (uint64_t)e->hdr_data, &vc);
            vc_ptl = VC_PTL(vc);

            if (recvbufs[buf_idx].remaining == 0) {
                mpi_errno = MPID_nem_handle_pkt(vc, recvbufs[buf_idx].packet, packet_sz);
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
                REQ_PTL(req)->bytes_put = packet_sz + recvbufs[buf_idx].remaining;
                TMPBUF(req) = MPIU_Malloc(REQ_PTL(req)->bytes_put);
                MPIU_Assert(TMPBUF(req) != NULL);
                MPIU_Memcpy(TMPBUF(req), recvbufs[buf_idx].packet, packet_sz);

                req->ch.vc = vc;

                req->dev.match.parts.tag = recvbufs[buf_idx].tag;

                size = recvbufs[buf_idx].remaining < MPIDI_nem_ptl_ni_limits.max_msg_size ?
                           recvbufs[buf_idx].remaining : MPIDI_nem_ptl_ni_limits.max_msg_size;
                buf_ptr = (char *)TMPBUF(req) + packet_sz;
                while (recvbufs[buf_idx].remaining) {
                    MPIDI_CH3U_Request_increment_cc(req, &incomplete);  /* Will be decremented - and eventually freed in REPLY */
                    ret = MPID_nem_ptl_rptl_get(MPIDI_nem_ptl_global_md, (ptl_size_t)buf_ptr,
                                                size, vc_ptl->id, vc_ptl->ptc, recvbufs[buf_idx].tag, 0, req);
                    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlget", "**ptlget %s",
                                         MPID_nem_ptl_strerror(ret));
                    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,
                                                            "PtlGet(size=%lu id=(%#x,%#x) pt=%#x tag=%#lx)", size,
                                                            vc_ptl->id.phys.nid,
                                                            vc_ptl->id.phys.pid, vc_ptl->ptc, recvbufs[buf_idx].tag));
                    buf_ptr += size;
                    recvbufs[buf_idx].remaining -= size;
                    if (recvbufs[buf_idx].remaining < MPIDI_nem_ptl_ni_limits.max_msg_size)
                        size = recvbufs[buf_idx].remaining;
                }
            }

            /* Repost the recv buffer */
            ret = MPID_nem_ptl_me_append(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &mes[buf_idx],
                                         PTL_PRIORITY_LIST, e->user_ptr /* buf_idx */, &me_handles[buf_idx]);
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend",
                                 "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));
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

    default:
        MPIU_Error_printf("Received unexpected event type: %d %s", e->type, MPID_nem_ptl_strevent(e));
        MPIU_ERR_INTERNALANDJUMP(mpi_errno, "Unexpected event type");
        break;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_NM_CTL_EVENT_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
