/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 NEC Corporation
 *      Author: Masamichi Takagi
 *      See COPYRIGHT in top-level directory.
 */

#include "ib_impl.h"

//#define MPID_NEM_IB_DEBUG_LMT
#ifdef dprintf  /* avoid redefinition with src/mpid/ch3/include/mpidimpl.h */
#undef dprintf
#endif
#ifdef MPID_NEM_IB_DEBUG_LMT
#define dprintf printf
#else
#define dprintf(...)
#endif

/* Get mode: sender sends RTS */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_initiate_lmt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_initiate_lmt(struct MPIDI_VC *vc, union MPIDI_CH3_Pkt *rts_pkt,
                                 struct MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
#if 0
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
#endif

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_INITIATE_LMT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_INITIATE_LMT);

    dprintf("lmt_initiate_lmt,enter,%d->%d,req=%p\n", MPID_nem_ib_myrank, vc->pg_rank, req);

    /* obtain dt_true_lb */
    /* see MPIDI_Datatype_get_info(in, in, out, out, out, out) (in src/mpid/ch3/include/mpidimpl.h) */
    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr,
                            dt_true_lb);

    /* FIXME: who frees s_cookie_buf? */
    /* malloc memory area for cookie. auto variable is NG because isend does not copy payload */
    MPID_nem_ib_lmt_cookie_t *s_cookie_buf =
        (MPID_nem_ib_lmt_cookie_t *) MPIU_Malloc(sizeof(MPID_nem_ib_lmt_cookie_t));

    /* remember address to "free" when receiving DONE from receiver */
    req->ch.s_cookie = s_cookie_buf;

    /* see MPIDI_CH3_PktHandler_RndvClrToSend (in src/mpid/ch3/src/ch3u_rndv.c) */
    //assert(dt_true_lb == 0);
    void *write_from_buf;
    if (dt_contig) {
        write_from_buf = (void *) ((char *) req->dev.user_buf + dt_true_lb);
    }
    else {
        /* see MPIDI_CH3_EagerNoncontigSend (in ch3u_eager.c) */
        req->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP((req->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER,
                            "**outofmemory");

        MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype,
                          req->dev.segment_ptr, 0);
        req->dev.segment_first = 0;
        req->dev.segment_size = data_sz;

        MPIDI_msg_sz_t last;
        last = req->dev.segment_size;   /* segment_size is byte offset */
        MPIU_Assert(last > 0);
        REQ_FIELD(req, lmt_pack_buf) = MPIU_Malloc((size_t) req->dev.segment_size);
        MPIU_ERR_CHKANDJUMP(!REQ_FIELD(req, lmt_pack_buf), mpi_errno, MPI_ERR_OTHER,
                            "**outofmemory");
        MPID_Segment_pack(req->dev.segment_ptr, req->dev.segment_first, &last,
                          (char *) (REQ_FIELD(req, lmt_pack_buf)));
        MPIU_Assert(last == req->dev.segment_size);
        write_from_buf = REQ_FIELD(req, lmt_pack_buf);
    }
    dprintf
        ("lmt_initate_lmt,dt_contig=%d,write_from_buf=%p,req->dev.user_buf=%p,REQ_FIELD(req, lmt_pack_buf)=%p\n",
         dt_contig, write_from_buf, req->dev.user_buf, REQ_FIELD(req, lmt_pack_buf));

#ifdef HAVE_LIBDCFA
#else
    s_cookie_buf->addr = write_from_buf;
#endif
    /* put sz, see MPID_nem_lmt_RndvSend (in src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt.c) */
    /* TODO remove sz field
     *   pkt_RTS_handler (in src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt.c)
     * rreq->ch.lmt_data_sz = rts_pkt->data_sz; */
    //s_cookie_buf->sz = (uint32_t)((MPID_nem_pkt_lmt_rts_t*)rts_pkt)->data_sz;

    /* preserve and put tail, because tail magic is written on the tail of payload
     * because we don't want to add another SGE or RDMA command */
    MPIU_Assert(((MPID_nem_pkt_lmt_rts_t *) rts_pkt)->data_sz == data_sz);
    s_cookie_buf->tail = *((uint8_t *) ((uint8_t *) write_from_buf + data_sz - sizeof(uint8_t)));
    /* prepare magic */
    //*((uint32_t*)(write_from_buf + data_sz - sizeof(tailmagic_t))) = MPID_NEM_IB_COM_MAGIC;

#if 0   /* moving to packet header */   /* embed RDMA-write-to buffer occupancy information */
    dprintf("lmt_initiate_lmt,rsr_seq_num_tail=%d\n", vc_ib->ibcom->rsr_seq_num_tail);
    /* embed RDMA-write-to buffer occupancy information */
    s_cookie_buf->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;

    /* remember the last one sent */
    vc_ib->ibcom->rsr_seq_num_tail_last_sent = vc_ib->ibcom->rsr_seq_num_tail;
#endif

    int post_num;
    uint32_t max_msg_sz;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    MPID_nem_ib_com_get_info_conn(vc_ib->sc->fd, MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ,
                                  &max_msg_sz, sizeof(uint32_t));

    /* Type of max_msg_sz is uint32_t. */
    post_num = (data_sz + (long) max_msg_sz - 1) / (long) max_msg_sz;

    s_cookie_buf->max_msg_sz = max_msg_sz;
    s_cookie_buf->seg_seq_num = 1;
    s_cookie_buf->seg_num = post_num;

    REQ_FIELD(req, buf.from) = write_from_buf;
    REQ_FIELD(req, data_sz) = data_sz;
    REQ_FIELD(req, seg_seq_num) = 1;    // only send 1st-segment, even if there are some segments.
    REQ_FIELD(req, seg_num) = post_num;
    REQ_FIELD(req, max_msg_sz) = max_msg_sz;

    long length;
    if (post_num > 1) {
        length = max_msg_sz;
    }
    else {
        length = data_sz;
    }
    /* put IB rkey */
    struct MPID_nem_ib_com_reg_mr_cache_entry_t *mr_cache =
        MPID_nem_ib_com_reg_mr_fetch(write_from_buf, length, 0, MPID_NEM_IB_COM_REG_MR_GLOBAL);
    MPIU_ERR_CHKANDJUMP(!mr_cache, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_reg_mr_fetch");
    struct ibv_mr *mr = mr_cache->mr;
    REQ_FIELD(req, lmt_mr_cache) = (void *) mr_cache;
#ifdef HAVE_LIBDCFA
    s_cookie_buf->addr = (void *) mr->host_addr;
    dprintf("lmt_initiate_lmt,s_cookie_buf->addr=%p\n", s_cookie_buf->addr);
#endif
    s_cookie_buf->rkey = mr->rkey;
    dprintf("lmt_initiate_lmt,tail=%02x,mem-tail=%p,%02x,sz=%ld,raddr=%p,rkey=%08x\n",
            s_cookie_buf->tail, write_from_buf + data_sz - sizeof(uint8_t),
            *((uint8_t *) (write_from_buf + data_sz - sizeof(uint8_t))), data_sz,
            s_cookie_buf->addr, s_cookie_buf->rkey);
    /* send cookie. rts_pkt as the MPI-header, s_cookie_buf as the payload */
    MPID_nem_lmt_send_RTS(vc, (MPID_nem_pkt_lmt_rts_t *) rts_pkt, s_cookie_buf,
                          sizeof(MPID_nem_ib_lmt_cookie_t));

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_INITIATE_LMT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* essential lrecv part extracted for dequeueing and issue from sendq */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_start_recv_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_start_recv_core(struct MPID_Request *req, void *raddr, uint32_t rkey, long len,
                                    void *write_to_buf, uint32_t max_msg_sz, int end)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    struct MPIDI_VC *vc = req->ch.vc;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    int i;
    int divide;
    int posted_num;
    int last;
    uint32_t r_max_msg_sz;      /* responder's max_msg_sz */
    void *write_pos;
    void *addr;
    long data_sz;
    MPIDI_msg_sz_t rest_data_sz;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_START_RECV_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_START_RECV_CORE);

    MPID_nem_ib_com_get_info_conn(vc_ib->sc->fd, MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ,
                                  &r_max_msg_sz, sizeof(uint32_t));

    divide = (max_msg_sz + r_max_msg_sz - 1) / r_max_msg_sz;

    write_pos = write_to_buf;
    posted_num = 0;
    last = MPID_NEM_IB_LMT_PART_OF_SEGMENT;
    rest_data_sz = len;
    addr = raddr;

    for (i = 0; i < divide; i++) {
        if (i == divide - 1)
            data_sz = max_msg_sz - i * r_max_msg_sz;
        else
            data_sz = r_max_msg_sz;

        if (i == divide - 1) {
            if (end)
                last = MPID_NEM_IB_LMT_LAST_PKT;        /* last part of last segment packet */
            else
                last = MPID_NEM_IB_LMT_SEGMENT_LAST;    /* last part of this segment */

            /* last data may be smaller than initiator's max_msg_sz */
            if (rest_data_sz < max_msg_sz)
                data_sz = rest_data_sz;
        }

        ibcom_errno =
            MPID_nem_ib_com_lrecv(vc_ib->sc->fd, (uint64_t) req, addr, data_sz, rkey,
                                  write_pos, last);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_lrecv");

        /* update position */
        write_pos = (void *) ((char *) write_pos + data_sz);
        addr = (void *) ((char *) addr + data_sz);

        /* update rest data size */
        rest_data_sz -= data_sz;

        /* count request number */
        posted_num++;
    }

    MPIU_Assert(rest_data_sz == 0);
    MPID_nem_ib_ncqe += posted_num;
    //dprintf("start_recv,ncqe=%d\n", MPID_nem_ib_ncqe);
    dprintf("lmt_start_recv_core,MPID_nem_ib_ncqe=%d\n", MPID_nem_ib_ncqe);
    dprintf
        ("lmt_start_recv_core,req=%p,sz=%ld,write_to_buf=%p,lmt_pack_buf=%p,user_buf=%p,raddr=%p,rkey=%08x,tail=%p=%02x\n",
         req, req->ch.lmt_data_sz, write_to_buf, REQ_FIELD(req, lmt_pack_buf), req->dev.user_buf,
         raddr, rkey, write_to_buf + req->ch.lmt_data_sz - sizeof(uint8_t),
         *((uint8_t *) (write_to_buf + req->ch.lmt_data_sz - sizeof(uint8_t))));
    //fflush(stdout);

#ifdef MPID_NEM_IB_LMT_GET_CQE
    MPID_nem_ib_ncqe_to_drain += posted_num;    /* use CQE instead of polling */
#else
    /* drain_scq and ib_poll is not ordered, so both can decrement ref_count */
    MPIR_Request_add_ref(req);

    /* register to poll list in ib_poll() */
    /* don't use req->dev.next because it causes unknown problem */
    MPID_nem_ib_lmtq_enqueue(&MPID_nem_ib_lmtq, req);
    dprintf("lmt_start_recv_core,lmtq enqueue\n");
    //volatile uint8_t* tailmagic = (uint8_t*)((void*)req->dev.user_buf + req->ch.lmt_data_sz - sizeof(uint8_t));
    //dprintf("start_recv_core,cur_tail=%02x,lmt_receiver_tail=%02x\n", *tailmagic, REQ_FIELD(req, lmt_receiver_tail));
#endif

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_START_RECV_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Get protocol: (1) sender sends rts to receiver (2) receiver RDMA-reads (here)
   (3) receiver polls on end-flag (4) receiver sends done to sender
   caller: (in mpid_nem_lmt.c)
*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_start_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_start_recv(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV s_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_START_RECV);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_START_RECV);

    dprintf("lmt_start_recv,enter,%d<-%d,req=%p\n", MPID_nem_ib_myrank, vc->pg_rank, req);

    /* obtain dt_true_lb */
    /* see MPIDI_Datatype_get_info(in, in, out, out, out, out) (in src/mpid/ch3/include/mpidimpl.h) */
    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr,
                            dt_true_lb);

    MPID_nem_ib_lmt_cookie_t *s_cookie_buf = s_cookie.iov_base;

    /* stash vc for ib_poll */
    req->ch.vc = vc;

    void *write_to_buf;
    if (dt_contig) {
        write_to_buf = (void *) ((char *) req->dev.user_buf + dt_true_lb);
    }
    else {
        //REQ_FIELD(req, lmt_pack_buf) = MPIU_Malloc((size_t)req->ch.lmt_data_sz);
        REQ_FIELD(req, lmt_pack_buf) = MPID_nem_ib_stmalloc((size_t) req->ch.lmt_data_sz);
        MPIU_ERR_CHKANDJUMP(!REQ_FIELD(req, lmt_pack_buf), mpi_errno, MPI_ERR_OTHER,
                            "**outofmemory");
        write_to_buf = REQ_FIELD(req, lmt_pack_buf);
    }

    REQ_FIELD(req, buf.to) = write_to_buf;

#ifdef MPID_NEM_IB_LMT_GET_CQE
#else
    /* unmark magic */
    *((uint8_t *) (write_to_buf + req->ch.lmt_data_sz - sizeof(uint8_t))) = ~s_cookie_buf->tail;        /* size in cookie was not set */
#endif
    dprintf
        ("lmt_start_recv,dt_contig=%d,write_to_buf=%p,req->dev.user_buf=%p,REQ_FIELD(req, lmt_pack_buf)=%p,marked-tail=%02x,unmarked-tail=%02x\n",
         dt_contig, write_to_buf, req->dev.user_buf, REQ_FIELD(req, lmt_pack_buf),
         s_cookie_buf->tail, *((uint8_t *) (write_to_buf + req->ch.lmt_data_sz - sizeof(uint8_t))));

    /* stash tail for poll because do_cts in mpid_nem_lmt.c free s_cookie_buf just after this function */
    REQ_FIELD(req, lmt_tail) = s_cookie_buf->tail;
    dprintf("lmt_start_recv,mem-tail=%p,%02x\n",
            write_to_buf + req->ch.lmt_data_sz - sizeof(uint8_t),
            *((uint8_t *) (write_to_buf + req->ch.lmt_data_sz - sizeof(uint8_t))));

    //dprintf("lmt_start_recv,sendq_empty=%d,ncom=%d,ncqe=%d\n", MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY, MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY);

    int last = 1;
    long length = req->ch.lmt_data_sz;

    if (s_cookie_buf->seg_seq_num != s_cookie_buf->seg_num) {
        last = 0;
        length = s_cookie_buf->max_msg_sz;
    }

    REQ_FIELD(req, max_msg_sz) = s_cookie_buf->max_msg_sz; /* store initiator's max_msg_sz */
    REQ_FIELD(req, seg_num) = s_cookie_buf->seg_num; /* store number of segments */

    /* try to issue RDMA-read command */
    int slack = 1;              /* slack for control packet bringing sequence number */
    if (MPID_nem_ib_sendq_empty(vc_ib->sendq) &&
        vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack &&
        MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack) {
        mpi_errno =
            MPID_nem_ib_lmt_start_recv_core(req, s_cookie_buf->addr, s_cookie_buf->rkey, length,
                                            write_to_buf, s_cookie_buf->max_msg_sz, last);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }
    else {
        /* enqueue command into send_queue */
        dprintf("lmt_start_recv, enqueuing,sendq_empty=%d,ncom=%d,ncqe=%d\n",
                MPID_nem_ib_sendq_empty(vc_ib->sendq),
                vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY,
                MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY);

        /* make raddr, (sz is in rreq->ch.lmt_data_sz), rkey, (user_buf is in req->dev.user_buf) survive enqueue, free cookie, dequeue */
        REQ_FIELD(req, lmt_raddr) = s_cookie_buf->addr;
        REQ_FIELD(req, lmt_rkey) = s_cookie_buf->rkey;
        REQ_FIELD(req, lmt_write_to_buf) = write_to_buf;
        REQ_FIELD(req, lmt_szsend) = length;
        REQ_FIELD(req, last) = last;

        MPID_nem_ib_sendq_enqueue(&vc_ib->sendq, req);
    }

#if 0   /* moving to packet header */
    /* extract embeded RDMA-write-to buffer occupancy information */
    dprintf("lmt_start_recv,old lsr_seq_num=%d,s_cookie_buf->seq_num_tail=%d\n",
            vc_ib->ibcom->lsr_seq_num_tail, s_cookie_buf->seq_num_tail);
    vc_ib->ibcom->lsr_seq_num_tail = s_cookie_buf->seq_num_tail;
    //dprintf("lmt_start_recv,new lsr_seq_num=%d\n", vc_ib->ibcom->lsr_seq_num_tail);
#endif

#ifndef MPID_NEM_IB_DISABLE_VAR_OCC_NOTIFY_RATE
    /* change remote notification policy of RDMA-write-to buf */
    //dprintf("lmt_start_recv,reply_seq_num,old rstate=%d\n", vc_ib->ibcom->rdmabuf_occupancy_notify_rstate);
    MPID_nem_ib_change_rdmabuf_occupancy_notify_policy_lw(vc_ib, &vc_ib->ibcom->lsr_seq_num_tail);
    //dprintf("lmt_start_recv,reply_seq_num,new rstate=%d\n", vc_ib->ibcom->rdmabuf_occupancy_notify_rstate);
#endif
    //dprintf("lmt_start_recv,reply_seq_num,sendq_empty=%d,ncom=%d,ncqe=%d,rdmabuf_occ=%d\n", MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom, MPID_nem_ib_ncqe, MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));
    /* try to send from sendq because at least one RDMA-write-to buffer has been released */
    //dprintf("lmt_start_recv,reply_seq_num,send_progress\n");
    if (!MPID_nem_ib_sendq_empty(vc_ib->sendq)) {
        dprintf("lmt_start_recv,ncom=%d,ncqe=%d,diff=%d\n",
                vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY,
                MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY,
                MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                                   vc_ib->ibcom->lsr_seq_num_tail) < MPID_NEM_IB_COM_RDMABUF_NSEG);
    }
    if (!MPID_nem_ib_sendq_empty(vc_ib->sendq) && MPID_nem_ib_sendq_ready_to_send_head(vc_ib)) {
        dprintf("lmt_start_recv,send_progress\n");
        fflush(stdout);
        MPID_nem_ib_send_progress(vc);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_START_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#if 0   /* unused function */
/* fall-back to lmt-get if end-flag of send-buf has the same value as the end-flag of recv-buf */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_switch_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_switch_send(struct MPIDI_VC *vc, struct MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPID_IOV r_cookie = req->ch.lmt_tmp_cookie;
    MPID_nem_ib_lmt_cookie_t *r_cookie_buf = r_cookie.iov_base;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_SWITCH_SEND);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_SWITCH_SEND);

    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr,
                            dt_true_lb);

    void *write_from_buf;
    if (dt_contig) {
        write_from_buf = req->dev.user_buf;
    }
    else {
        /* see MPIDI_CH3_EagerNoncontigSend (in ch3u_eager.c) */
        req->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP((req->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER,
                            "**outofmemory");

        MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype,
                          req->dev.segment_ptr, 0);
        req->dev.segment_first = 0;
        req->dev.segment_size = data_sz;

        MPIDI_msg_sz_t last;
        last = req->dev.segment_size;   /* segment_size is byte offset */
        MPIU_Assert(last > 0);

        REQ_FIELD(req, lmt_pack_buf) = MPIU_Malloc(data_sz);
        MPIU_ERR_CHKANDJUMP(!REQ_FIELD(req, lmt_pack_buf), mpi_errno, MPI_ERR_OTHER,
                            "**outofmemory");

        MPID_Segment_pack(req->dev.segment_ptr, req->dev.segment_first, &last,
                          (char *) (REQ_FIELD(req, lmt_pack_buf)));
        MPIU_Assert(last == req->dev.segment_size);

        write_from_buf = REQ_FIELD(req, lmt_pack_buf);
    }

    //assert(dt_true_lb == 0);
    uint8_t *tailp =
        (uint8_t *) ((uint8_t *) write_from_buf /*+ dt_true_lb */  + data_sz - sizeof(uint8_t));
#if 0
    *is_end_flag_same = (r_cookie_buf->tail == *tailp) ? 1 : 0;
#else
    REQ_FIELD(req, lmt_receiver_tail) = r_cookie_buf->tail;
    REQ_FIELD(req, lmt_sender_tail) = *tailp;
    dprintf("lmt_switch_send,tail on sender=%02x,tail onreceiver=%02x,req=%p\n", *tailp,
            r_cookie_buf->tail, req);
#ifdef MPID_NEM_IB_DEBUG_LMT
    uint8_t *tail_wordp = (uint8_t *) ((uint8_t *) write_from_buf + data_sz - sizeof(uint32_t) * 2);
#endif
    dprintf("lmt_switch_send,tail on sender=%d\n", *tail_wordp);
    fflush(stdout);
#endif

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_SWITCH_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif

/* when cookie is received in the middle of the lmt */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_handle_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_handle_cookie(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_HANDLE_COOKIE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_HANDLE_COOKIE);

    dprintf("lmt_handle_cookie,enter\n");

    /* Nothing to do */

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_HANDLE_COOKIE);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

/* when sender receives DONE from receiver */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_done_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_done_send(struct MPIDI_VC *vc, struct MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_DONE_SEND);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_DONE_SEND);

    dprintf("lmt_done_send,enter,%d<-%d,req=%p,REQ_FIELD(req, lmt_pack_buf)=%p\n",
            MPID_nem_ib_myrank, vc->pg_rank, req, REQ_FIELD(req, lmt_pack_buf));


    /* free memory area for cookie */
    if (!req->ch.s_cookie) {
        dprintf("lmt_done_send,enter,req->ch.s_cookie is zero");
    }
    MPIU_Free(req->ch.s_cookie);
    //dprintf("lmt_done_send,free cookie,%p\n", req->ch.s_cookie);

    /* free temporal buffer for eager-send non-contiguous data.
     * MPIDI_CH3U_Recvq_FDU_or_AEP (in mpid_isend.c) sets req->dev.datatype */
    int is_contig;
    MPID_Datatype_is_contig(req->dev.datatype, &is_contig);
    if (!is_contig && REQ_FIELD(req, lmt_pack_buf)) {
        dprintf("lmt_done_send,lmt-get,non-contiguous,free lmt_pack_buf\n");
#if 1   /* debug, enable again later */
        MPIU_Free(REQ_FIELD(req, lmt_pack_buf));
#endif
    }

    /* mark completion on sreq */
    MPIU_ERR_CHKANDJUMP(req->dev.OnDataAvail, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_lmt_done_send");
    dprintf("lmt_done_send,1,req=%p,pcc=%d\n", req, MPIDI_CH3I_progress_completion_count.v);
    MPIDI_CH3U_Request_complete(req);
    dprintf("lmt_done_send,complete,req=%p\n", req);
    dprintf("lmt_done_send,2,req=%p,pcc=%d\n", req, MPIDI_CH3I_progress_completion_count.v);
    //dprintf("lmt_done_send, mark completion on sreq\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_DONE_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* lmt-put (1) sender sends done when finding cqe of put (2) packet-handler of DONE on receiver (3) here */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_done_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_done_recv(struct MPIDI_VC *vc, struct MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_DONE_RECV);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_DONE_RECV);

    dprintf("lmt_done_recv,enter,rreq=%p,head=%p\n", rreq, MPID_nem_ib_lmtq.head);


    int is_contig;
    MPID_Datatype_is_contig(rreq->dev.datatype, &is_contig);
    if (!is_contig) {
        dprintf("lmt_done_recv,copying noncontiguous data to user buffer\n");

        /* see MPIDI_CH3U_Request_unpack_uebuf (in /src/mpid/ch3/src/ch3u_request.c) */
        /* or MPIDI_CH3U_Receive_data_found (in src/mpid/ch3/src/ch3u_handle_recv_pkt.c) */
        MPIDI_msg_sz_t unpack_sz = rreq->ch.lmt_data_sz;
        MPID_Segment seg;
        MPI_Aint last;

        MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, &seg, 0);
        last = unpack_sz;
        MPID_Segment_unpack(&seg, 0, &last, REQ_FIELD(rreq, lmt_pack_buf));
        if (last != unpack_sz) {
            /* --BEGIN ERROR HANDLING-- */
            /* received data was not entirely consumed by unpack()
             * because too few bytes remained to fill the next basic
             * datatype */
            MPIR_STATUS_SET_COUNT(rreq->status, last);
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                     MPI_ERR_TYPE, "**MPID_nem_ib_lmt_done_recv", 0);
            /* --END ERROR HANDLING-- */
        }

        //MPIU_Free(REQ_FIELD(rreq, lmt_pack_buf));
        MPID_nem_ib_stfree(REQ_FIELD(rreq, lmt_pack_buf), (size_t) rreq->ch.lmt_data_sz);
    }

    dprintf("lmt_done_recv,1,req=%p,pcc=%d\n", rreq, MPIDI_CH3I_progress_completion_count.v);
    MPIDI_CH3U_Request_complete(rreq);
    dprintf("lmt_done_recv,complete,req=%p\n", rreq);
    dprintf("lmt_done_recv,2,pcc=%d\n", MPIDI_CH3I_progress_completion_count.v);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_DONE_RECV);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lmt_vc_terminated
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_lmt_vc_terminated(struct MPIDI_VC *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_LMT_VC_TERMINATED);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_LMT_VC_TERMINATED);

    dprintf("lmt_vc_terminated,enter\n");

    /* Nothing to do */

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_LMT_VC_TERMINATED);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}
