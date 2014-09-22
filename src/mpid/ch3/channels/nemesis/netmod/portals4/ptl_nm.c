/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include <mpl_utlist.h>

#define NUM_SEND_BUFS 20
#define NUM_RECV_BUFS 20
#define BUFLEN  (sizeof(MPIDI_CH3_Pkt_t) + PTL_MAX_EAGER)

typedef struct MPID_nem_ptl_sendbuf {
    struct MPID_nem_ptl_sendbuf *next;
    union {
        struct {
            MPIDI_CH3_Pkt_t hdr;
            char payload[PTL_MAX_EAGER];
        } hp; /* header+payload */
        char p[BUFLEN]; /* just payload */
    } buf;
} MPID_nem_ptl_sendbuf_t;

static MPID_nem_ptl_sendbuf_t sendbuf[NUM_SEND_BUFS];
static MPID_nem_ptl_sendbuf_t *free_head = NULL;
static MPID_nem_ptl_sendbuf_t *free_tail = NULL;

static char recvbuf[BUFLEN][NUM_RECV_BUFS];
static ptl_me_t recvbuf_me[NUM_RECV_BUFS];
static ptl_handle_me_t recvbuf_me_handle[NUM_RECV_BUFS];

#define FREE_EMPTY() (free_head == NULL)
#define FREE_HEAD() free_head
#define FREE_PUSH(buf_p) MPL_LL_PREPEND(free_head, free_tail, buf_p)
#define FREE_POP(buf_pp) do { *(buf_pp) = free_head; MPL_LL_DELETE(free_head, free_tail, free_head); } while (0)

static struct {MPID_Request *head, *tail;} send_queue;

static int send_queued(void);

static void vc_dbg_print_sendq(FILE *stream, MPIDI_VC_t *vc) {/* FIXME: write real function */ return;}

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

    MPIU_Assert(BUFLEN == sizeof(sendbuf->buf));

    /* init send */
    for (i = 0; i < NUM_SEND_BUFS; ++i)
        FREE_PUSH(&sendbuf[i]);

    send_queue.head = send_queue.tail = NULL;

    MPID_nem_net_module_vc_dbg_print_sendq = vc_dbg_print_sendq;

    /* init recv */
    id_any.phys.pid = PTL_PID_ANY;
    id_any.phys.nid = PTL_NID_ANY;
    
    for (i = 0; i < NUM_RECV_BUFS; ++i) {
        recvbuf_me[i].start = recvbuf[i];
        recvbuf_me[i].length = BUFLEN;
        recvbuf_me[i].ct_handle = PTL_CT_NONE;
        recvbuf_me[i].uid = PTL_UID_ANY;
        recvbuf_me[i].options = (PTL_ME_OP_PUT | PTL_ME_USE_ONCE | PTL_ME_EVENT_UNLINK_DISABLE |
                                 PTL_ME_EVENT_LINK_DISABLE | PTL_ME_IS_ACCESSIBLE);
        recvbuf_me[i].match_id = id_any;
        recvbuf_me[i].match_bits = 0;
        recvbuf_me[i].ignore_bits = (ptl_match_bits_t)~0;

        ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &recvbuf_me[i], PTL_PRIORITY_LIST, (void *)(uint64_t)i,
                          &recvbuf_me_handle[i]);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));
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
        ret = PtlMEUnlink(recvbuf_me_handle[i]);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink", "**ptlmeunlink %s", MPID_nem_ptl_strerror(ret));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_NM_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_sendq_complete_with_error
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_sendq_complete_with_error(MPIDI_VC_t *vc, int req_errno)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_SENDQ_COMPLETE_WITH_ERROR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_SENDQ_COMPLETE_WITH_ERROR);


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_SENDQ_COMPLETE_WITH_ERROR);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME save_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline void save_iov(MPID_Request *sreq, void *hdr, void *data, MPIDI_msg_sz_t data_sz)
{
    int index = 0;
    MPIDI_STATE_DECL(MPID_STATE_SAVE_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_SAVE_IOV);

    MPIU_Assert(hdr || data_sz);
    
    if (hdr) {
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)hdr;
        sreq->dev.iov[index].MPID_IOV_BUF = &sreq->dev.pending_pkt;
        sreq->dev.iov[index].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
        ++index;
    }
    if (data_sz) {
        sreq->dev.iov[index].MPID_IOV_BUF = data;
        sreq->dev.iov[index].MPID_IOV_LEN = data_sz;
        ++index;
    }
    sreq->dev.iov_count = index;
}

#undef FUNCNAME
#define FUNCNAME send_pkt
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int send_pkt(MPIDI_VC_t *vc, void **vhdr_p, void **vdata_p, MPIDI_msg_sz_t *data_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_sendbuf_t *sb;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    MPIDI_CH3_Pkt_t **hdr_p = (MPIDI_CH3_Pkt_t **)vhdr_p;
    char **data_p = (char **)vdata_p;
    MPIDI_STATE_DECL(MPID_STATE_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_PKT);
    
    if (!vc_ptl->id_initialized) {
        mpi_errno = MPID_nem_ptl_init_id(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH3I_Sendq_empty(send_queue) && !FREE_EMPTY()) {
        MPIDI_msg_sz_t len;
        /* send header and first chunk of data */
        FREE_POP(&sb);
        sb->buf.hp.hdr = **hdr_p;
        len = *data_sz_p;
        if (len > PTL_MAX_EAGER)
            len = PTL_MAX_EAGER;
        MPIU_Memcpy(sb->buf.hp.payload, *data_p, len);
        ret = PtlPut(MPIDI_nem_ptl_global_md, (ptl_size_t)sb->buf.p, sizeof(sb->buf.hp.hdr) + len, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->ptc, 0, 0, sb,
                     MPIDI_Process.my_pg_rank);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x) sb=%p",
                                                sizeof(sb->buf.hp.hdr) + len, vc_ptl->id.phys.nid, vc_ptl->id.phys.pid,
                                                vc_ptl->ptc, sb));
        *hdr_p = NULL;
        *data_p += len;
        *data_sz_p -= len;

        /* send additional data chunks if necessary */
        while (*data_sz_p && !FREE_EMPTY()) {
            FREE_POP(&sb);
            len = *data_sz_p;
            if (len > BUFLEN)
                len = BUFLEN;
            MPIU_Memcpy(sb->buf.p, *data_p, len);
            ret = PtlPut(MPIDI_nem_ptl_global_md, (ptl_size_t)sb->buf.p, len, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->ptc, 0, 0, sb, MPIDI_Process.my_pg_rank);
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x) sb=%p", len,
                                                    vc_ptl->id.phys.nid, vc_ptl->id.phys.pid, vc_ptl->ptc, sb));
            *data_p += len;
            *data_sz_p -= len;
        }
    }

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
static int send_noncontig_pkt(MPIDI_VC_t *vc, MPID_Request *sreq, void **vhdr_p, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_sendbuf_t *sb;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    MPIDI_msg_sz_t last;
    MPIDI_CH3_Pkt_t **hdr_p = (MPIDI_CH3_Pkt_t **)vhdr_p;
    MPIDI_STATE_DECL(MPID_STATE_SEND_NONCONTIG_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_NONCONTIG_PKT);

    *complete = 0;
    MPID_nem_ptl_init_req(sreq);

    if (!vc_ptl->id_initialized) {
        mpi_errno = MPID_nem_ptl_init_id(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH3I_Sendq_empty(send_queue) && !FREE_EMPTY()) {
        /* send header and first chunk of data */
        FREE_POP(&sb);
        sb->buf.hp.hdr = **hdr_p;

        MPIU_Assert(sreq->dev.segment_first == 0);

        last = sreq->dev.segment_size;
        if (last > PTL_MAX_EAGER)
            last = PTL_MAX_EAGER;
        MPI_nem_ptl_pack_byte(sreq->dev.segment_ptr, 0, last, sb->buf.hp.payload, &REQ_PTL(sreq)->overflow[0]);
        ret = PtlPut(MPIDI_nem_ptl_global_md, (ptl_size_t)sb->buf.p, sizeof(sb->buf.hp.hdr) + last, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->ptc, 0, 0, sb,
                     MPIDI_Process.my_pg_rank);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x) sb=%p",
                                                sizeof(sb->buf.hp.hdr) + last, vc_ptl->id.phys.nid, vc_ptl->id.phys.pid,
                                                vc_ptl->ptc, sb));
        *vhdr_p = NULL;

        if (last == sreq->dev.segment_size) {
            *complete = 1;
            goto fn_exit;
        }
        
        /* send additional data chunks */
        sreq->dev.segment_first = last;

        while (!FREE_EMPTY()) {
            FREE_POP(&sb);
            
            last = sreq->dev.segment_size;
            if (last > sreq->dev.segment_first+BUFLEN)
                last = sreq->dev.segment_first+BUFLEN;

            MPI_nem_ptl_pack_byte(sreq->dev.segment_ptr, sreq->dev.segment_first, last, sb->buf.p, &REQ_PTL(sreq)->overflow[0]);
            sreq->dev.segment_first = last;
            ret = PtlPut(MPIDI_nem_ptl_global_md, (ptl_size_t)sb->buf.p, last - sreq->dev.segment_first, PTL_NO_ACK_REQ, vc_ptl->id, vc_ptl->ptc, 0, 0, sb,
                         MPIDI_Process.my_pg_rank);
            MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PtlPut(size=%lu id=(%#x,%#x) pt=%#x) sb=%p",
                                                    last - sreq->dev.segment_first, vc_ptl->id.phys.nid, vc_ptl->id.phys.pid,
                                                    vc_ptl->ptc, sb));

            if (last == sreq->dev.segment_size) {
                *complete = 1;
                goto fn_exit;
            }
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_NONCONTIG_PKT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME enqueue_request
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int enqueue_request(MPIDI_VC_t *vc, MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    MPIDI_STATE_DECL(MPID_STATE_ENQUEUE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_ENQUEUE_REQUEST);
    
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");
    MPIU_Assert(FREE_EMPTY() || !MPIDI_CH3I_Sendq_empty(send_queue));
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);

    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

    ++(vc_ptl->num_queued_sends);
        
    if (FREE_EMPTY()) {
        MPIDI_CH3I_Sendq_enqueue(&send_queue, sreq);
    } else {
        /* there are other sends in the queue before this one: try to send from the queue */
        MPIDI_CH3I_Sendq_enqueue(&send_queue, sreq);
        mpi_errno = send_queued();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ENQUEUE_REQUEST);
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
    int complete = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_SENDNONCONTIG);
    MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    mpi_errno = send_noncontig_pkt(vc, sreq, &hdr, &complete);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    if (complete) {
        /* sent whole message */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn) {
            MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
            MPIDI_CH3U_Request_complete(sreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        } else {
            complete = 0;
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                        
            if (complete) {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                goto fn_exit;
            }
            /* not completed: more to send */
        }
    }

    REQ_PTL(sreq)->noncontig = TRUE;
    save_iov(sreq, hdr, NULL, 0); /* save the header in IOV if necessary */

    /* enqueue request */
    mpi_errno = enqueue_request(vc, sreq);
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
int MPID_nem_ptl_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                   MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ISTARTCONTIGMSG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    mpi_errno = send_pkt(vc, &hdr, &data, &data_sz);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    if (hdr == NULL && data_sz == 0) {
        /* sent whole message */
        *sreq_ptr = NULL;
        goto fn_exit;
    }
    
    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert(sreq != NULL);
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = 0;
    REQ_PTL(sreq)->noncontig = FALSE;
    save_iov(sreq, hdr, data, data_sz);

    /* enqueue request */
    mpi_errno = enqueue_request(vc, sreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    *sreq_ptr = sreq;

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
    
    mpi_errno = send_pkt(vc, &hdr, &data, &data_sz);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    if (hdr == NULL && data_sz == 0) {
        /* sent whole message */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn) {
            MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
            MPIDI_CH3U_Request_complete(sreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        } else {
            int complete = 0;
                        
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                        
            if (complete) {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                goto fn_exit;
            }
            /* not completed: more to send */
        }
    } else {
        save_iov(sreq, hdr, data, data_sz);
    }

    REQ_PTL(sreq)->noncontig = FALSE;
    
    /* enqueue request */
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);

    mpi_errno = enqueue_request(vc, sreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ISENDCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME send_queued
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int send_queued(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_sendbuf_t *sb;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_SEND_QUEUED);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_QUEUED);

    while (!MPIDI_CH3I_Sendq_empty(send_queue) && !FREE_EMPTY()) {
        int complete = TRUE;
        MPIDI_msg_sz_t send_len = 0;
        int i;
        MPID_Request *sreq;
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

        sreq = MPIDI_CH3I_Sendq_head(send_queue); /* don't dequeue until we're finished sending this request */
        FREE_POP(&sb);
        
        /* copy the iov */
        MPIU_Assert(sreq->dev.iov_count <= 2);
        for (i = sreq->dev.iov_offset; i < sreq->dev.iov_count + sreq->dev.iov_offset; ++i) {
            MPIDI_msg_sz_t len;
            len = sreq->dev.iov[i].iov_len;
            if (len > BUFLEN)
                len = BUFLEN;
            MPIU_Memcpy(sb->buf.p, sreq->dev.iov[i].iov_base, len);
            send_len += len;
            if (len < sreq->dev.iov[i].iov_len) {
                /* ran out of space in buffer */
                sreq->dev.iov[i].iov_base = (char *)sreq->dev.iov[i].iov_base + len;
                sreq->dev.iov[i].iov_len -= len;
                sreq->dev.iov_offset = i;
                complete = FALSE;
                break;
            }
        }

        /* copy any noncontig data if there's room left in the send buffer */
        if (send_len < BUFLEN && REQ_PTL(sreq)->noncontig) {
            MPIDI_msg_sz_t last;
            MPIU_Assert(complete); /* if complete has been set to false, there can't be any space left in the send buffer */
            last = sreq->dev.segment_size;
            if (last > sreq->dev.segment_first+BUFLEN) {
                last = sreq->dev.segment_first+BUFLEN;
                complete = FALSE;
            }
            MPI_nem_ptl_pack_byte(sreq->dev.segment_ptr, sreq->dev.segment_first, last, sb->buf.p, &REQ_PTL(sreq)->overflow[0]);
            send_len += last - sreq->dev.segment_first;
            sreq->dev.segment_first = last;
        }
        ret = PtlPut(MPIDI_nem_ptl_global_md, (ptl_size_t)sb->buf.p, send_len, PTL_NO_ACK_REQ, VC_PTL(sreq->ch.vc)->id, VC_PTL(sreq->ch.vc)->ptc, 0, 0, sb,
                     MPIDI_Process.my_pg_rank);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlput", "**ptlput %s", MPID_nem_ptl_strerror(ret));

        if (!complete)
            continue;
        
        /* sent all of the data */
        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn) {
            MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
            MPIDI_CH3U_Request_complete(sreq);
        } else {
            complete = 0;
            mpi_errno = reqFn(sreq->ch.vc, sreq, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            if (!complete)
                continue;
        }
        
        /* completed the request */
        --(VC_PTL(sreq->ch.vc)->num_queued_sends);
        MPIDI_CH3I_Sendq_dequeue(&send_queue, &sreq);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");

        if (VC_PTL(sreq->ch.vc)->num_queued_sends == 0 && sreq->ch.vc->state == MPIDI_VC_STATE_CLOSED) {
            /* this VC is closing, if this was the last req queued for that vc, call vc_terminated() */
            mpi_errno = MPID_nem_ptl_vc_terminated(sreq->ch.vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_QUEUED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME handle_ack
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handle_ack(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(HANDLE_ACK);

    MPIDI_FUNC_ENTER(HANDLE_ACK);
    MPIU_Assert(e->type == PTL_EVENT_SEND);

    FREE_PUSH((MPID_nem_ptl_sendbuf_t *)e->user_ptr);

    if (!MPIDI_CH3I_Sendq_empty(send_queue)) {
        mpi_errno = send_queued();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(HANDLE_ACK);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_nm_event_handler
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_nm_event_handler(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_NM_EVENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_NM_EVENT_HANDLER);

    switch (e->type) {
    case PTL_EVENT_PUT:
        MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, (uint64_t)e->hdr_data, &vc);
        mpi_errno = MPID_nem_handle_pkt(vc, e->start, e->rlength);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIU_Assert(e->start == recvbuf[(uint64_t)e->user_ptr]);
        ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &recvbuf_me[(uint64_t)e->user_ptr],
                          PTL_PRIORITY_LIST, e->user_ptr, &recvbuf_me_handle[(uint64_t)e->user_ptr]);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));
        break;
    case PTL_EVENT_ACK:
        mpi_errno = handle_ack(e);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        break;
    case PTL_EVENT_SEND:
        /* ignore */
        break;
    default:
        MPIU_Error_printf("Received unexpected event type: %d %s", e->type, MPID_nem_ptl_strevent(e));
        MPIU_ERR_INTERNALANDJUMP(mpi_errno, "Unexpected event type");
        break;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_NM_EVENT_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
