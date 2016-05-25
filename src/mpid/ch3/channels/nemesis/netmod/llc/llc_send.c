/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */
/*
 *
 */



#include "mpid_nem_impl.h"
#include "llc_impl.h"

//#define MPID_NEM_LLC_DEBUG_SEND
#ifdef MPID_NEM_LLC_DEBUG_SEND
#define dprintf printf
#else
#define dprintf(...)
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_isend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
                       int dest, int tag, MPIR_Comm * comm, int context_offset,
                       struct MPIR_Request **req_out)
{
    int mpi_errno = MPI_SUCCESS, llc_errno;
    int dt_contig;
    intptr_t data_sz;
    MPIR_Datatype*dt_ptr;
    MPI_Aint dt_true_lb;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LLC_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LLC_ISEND);

    dprintf
        ("llc_isend,%d->%d,buf=%p,count=%d,datatype=%08x,dest=%d,tag=%08x,comm=%p,context_offset=%d\n",
         MPIDI_Process.my_pg_rank, vc->pg_rank, buf, count, datatype, dest, tag, comm,
         context_offset);

    int LLC_my_rank;
    LLC_comm_rank(LLC_COMM_MPICH, &LLC_my_rank);
    dprintf("llc_isend,LLC_my_rank=%d\n", LLC_my_rank);

    struct MPIR_Request *sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_Assert(sreq != NULL);
    MPIR_Object_set_ref(sreq, 2);
    sreq->kind = MPIR_REQUEST_KIND__SEND;

    /* Used in llc_poll --> MPID_nem_llc_send_handler */
    sreq->ch.vc = vc;
    sreq->dev.OnDataAvail = 0;
    /* Don't save iov_offset because it's not used. */

    /* Save it because it's used in send_handler */
#ifndef	notdef_leak_0002_hack
    /*   See also MPIDI_Request_create_sreq() */
    /*     in src/mpid/ch3/include/mpidimpl.h */
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
#endif /* notdef_leak_0002_hack */
    sreq->dev.datatype = datatype;
#ifndef	notdef_leak_0002_hack
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
#endif /* notdef_leak_0002_hack */
#ifndef	notdef_scan_hack

    /* used for MPI_Cancel() */
    sreq->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, FALSE);
    sreq->dev.cancel_pending = FALSE;
    /* Do not reset dev.state after calling MPIDI_Request_set_type() */
    /* sreq->dev.state = 0; */
    sreq->dev.match.parts.rank = dest;
    sreq->dev.match.parts.tag = tag;
    sreq->dev.match.parts.context_id = comm->context_id + context_offset;
#endif /* notdef_scan_hack */

    dprintf("llc_isend,remote_endpoint_addr=%ld\n", VC_FIELD(vc, remote_endpoint_addr));

    REQ_FIELD(sreq, rma_buf) = NULL;

    LLC_cmd_t *cmd = LLC_cmd_alloc2(1, 1, 1);
    cmd[0].opcode = LLC_OPCODE_SEND;
    cmd[0].comm = LLC_COMM_MPICH;
    cmd[0].rank = VC_FIELD(vc, remote_endpoint_addr);
    cmd[0].req_id = (uint64_t) cmd;

    /* Prepare bit-vector to perform tag-match. We use the same bit-vector as in CH3 layer. */
    /* See src/mpid/ch3/src/mpid_isend.c */
    *(int32_t *) ((uint8_t *) & cmd[0].tag) = tag;
    *(MPIR_Context_id_t *) ((uint8_t *) & cmd[0].tag + sizeof(int32_t)) =
        comm->context_id + context_offset;
    MPIR_Assert(sizeof(LLC_tag_t) >= sizeof(int32_t) + sizeof(MPIR_Context_id_t));
    memset((uint8_t *) & cmd[0].tag + sizeof(int32_t) + sizeof(MPIR_Context_id_t),
           0, sizeof(LLC_tag_t) - sizeof(int32_t) - sizeof(MPIR_Context_id_t));

    dprintf("llc_isend,tag=");
    for (i = 0; i < sizeof(LLC_tag_t); i++) {
        dprintf("%02x", (int) *((uint8_t *) & cmd[0].tag + i));
    }
    dprintf("\n");

    /* Prepare RDMA-write from buffer */
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    dprintf("llc_isend,dt_contig=%d,data_sz=%ld\n", dt_contig, data_sz);


    const void *write_from_buf;
    if (dt_contig) {
        write_from_buf = (void *) ((char *) buf + dt_true_lb);
#ifndef	notdef_leak_0002_hack
        REQ_FIELD(sreq, pack_buf) = 0;
#endif /* notdef_leak_0002_hack */
    }
    else {
        /* See MPIDI_CH3_EagerNoncontigSend (in ch3u_eager.c) */
        struct MPIR_Segment *segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP(!segment_ptr, mpi_errno, MPI_ERR_OTHER, "**outofmemory");
#ifndef	notdef_leak_0001_hack
        /* See also MPIDI_CH3_Request_create and _destory() */
        /*     in src/mpid/ch3/src/ch3u_request.c */
        sreq->dev.segment_ptr = segment_ptr;
#endif /* notdef_leak_0001_hack */

        MPIR_Segment_init(buf, count, datatype, segment_ptr, 0);
        intptr_t segment_first = 0;
        intptr_t segment_size = data_sz;
        intptr_t last = segment_size;
        MPIR_Assert(last > 0);
        REQ_FIELD(sreq, pack_buf) = MPL_malloc((size_t) data_sz);
        MPIR_ERR_CHKANDJUMP(!REQ_FIELD(sreq, pack_buf), mpi_errno, MPI_ERR_OTHER, "**outofmemory");
        MPIR_Segment_pack(segment_ptr, segment_first, &last, (char *) (REQ_FIELD(sreq, pack_buf)));
        MPIR_Assert(last == data_sz);
        write_from_buf = REQ_FIELD(sreq, pack_buf);
    }

    cmd[0].iov_local[0].addr = (uint64_t) write_from_buf;
    cmd[0].iov_local[0].length = data_sz;
    cmd[0].niov_local = 1;

    cmd[0].iov_remote[0].addr = 0;
    cmd[0].iov_remote[0].length = data_sz;
    cmd[0].niov_remote = 1;

    ((struct llc_cmd_area *) cmd[0].usr_area)->cbarg = sreq;
    ((struct llc_cmd_area *) cmd[0].usr_area)->raddr = VC_FIELD(vc, remote_endpoint_addr);

    llc_errno = LLC_post(cmd, 1);
    MPIR_ERR_CHKANDJUMP(llc_errno != LLC_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**LLC_post");

  fn_exit:
    *req_out = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LLC_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_iStartContigMsg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, intptr_t hdr_sz, void *data,
                                 intptr_t data_sz, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPID_nem_llc_vc_area *vc_llc = 0;
    int need_to_queue = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LLC_ISTARTCONTIGMSG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LLC_ISTARTCONTIGMSG);

    dprintf("llc_iStartContigMsg,%d->%d,hdr=%p,hdr_sz=%ld,data=%p,data_sz=%ld\n",
            MPIDI_Process.my_pg_rank, vc->pg_rank, hdr, hdr_sz, data, data_sz);

    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) hdr);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "vc.pg_rank = %d", vc->pg_rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "my_pg_rank = %d", MPIDI_Process.my_pg_rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "hdr_sz     = %d", (int) hdr_sz);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "data_sz    = %d", (int) data_sz);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "hdr type   = %d", ((MPIDI_CH3_Pkt_t *) hdr)->type);

    /* create a request */
    sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_Assert(sreq != NULL);
    MPIR_Object_set_ref(sreq, 2);
    sreq->kind = MPIR_REQUEST_KIND__SEND;

    sreq->ch.vc = vc;
    sreq->dev.OnDataAvail = 0;
    sreq->dev.iov_offset = 0;

    REQ_FIELD(sreq, rma_buf) = NULL;

    /* sreq: src/mpid/ch3/include/mpidpre.h */
    sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) & sreq->dev.pending_pkt;
    sreq->dev.iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    sreq->dev.iov_count = 1;
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "IOV_LEN    = %d", (int) sreq->dev.iov[0].MPL_IOV_LEN);
    if (data_sz > 0) {
        sreq->dev.iov[1].MPL_IOV_BUF = data;
        sreq->dev.iov[1].MPL_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                       "IOV_LEN    = %d", (int) sreq->dev.iov[0].MPL_IOV_LEN);
    }

    vc_llc = VC_LLC(vc);
    if (!MPIDI_CH3I_Sendq_empty(vc_llc->send_queue)) {
        need_to_queue = 1;
        goto queue_it;
    }

    {
        int ret;

        ret = llc_writev(vc_llc->endpoint,
                         vc_llc->remote_endpoint_addr,
                         sreq->dev.iov, sreq->dev.iov_count, sreq, &REQ_LLC(sreq)->cmds);
        if (ret < 0) {
            mpi_errno = MPI_ERR_OTHER;
            MPIR_ERR_POP(mpi_errno);
        }
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                       "IOV_LEN    = %d", (int) sreq->dev.iov[0].MPL_IOV_LEN);
        if (!MPIDI_nem_llc_Rqst_iov_update(sreq, ret)) {
            need_to_queue = 2;  /* YYY */
        }
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                       "IOV_LEN    = %d", (int) sreq->dev.iov[0].MPL_IOV_LEN);
    }

  queue_it:
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "need_to_que  %d", need_to_queue);
    if (need_to_queue > 0) {
        MPIDI_CH3I_Sendq_enqueue(&vc_llc->send_queue, sreq);
    }

  fn_exit:
    *sreq_ptr = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LLC_ISTARTCONTIGMSG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_iSendContig
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_iSendContig(MPIDI_VC_t * vc, MPIR_Request * sreq, void *hdr, intptr_t hdr_sz,
                             void *data, intptr_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_llc_vc_area *vc_llc = 0;
    int need_to_queue = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LLC_ISENDCONTIGMSG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LLC_ISENDCONTIGMSG);

    if (sreq->kind == MPIR_REQUEST_KIND__UNDEFINED) {
        sreq->kind = MPIR_REQUEST_KIND__SEND;
    }
    dprintf("llc_iSendConitig,sreq=%p,hdr=%p,hdr_sz=%ld,data=%p,data_sz=%ld\n",
            sreq, hdr, hdr_sz, data, data_sz);

    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) hdr);
    MPL_DBG_PKT(vc, hdr, "isendcontig");
    {
        MPIDI_CH3_Pkt_t *pkt = (MPIDI_CH3_Pkt_t *) hdr;

        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "pkt->type  = %d", pkt->type);
    }

    MPIR_Assert(sreq != NULL);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "OnDataAvail= %p", sreq->dev.OnDataAvail);
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

    /* sreq: src/mpid/ch3/include/mpidpre.h */
    sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) & sreq->dev.pending_pkt;
    sreq->dev.iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    sreq->dev.iov_count = 1;
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "IOV_LEN    = %d", (int) sreq->dev.iov[0].MPL_IOV_LEN);
    if (data_sz > 0) {
        sreq->dev.iov[1].MPL_IOV_BUF = data;
        sreq->dev.iov[1].MPL_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                       "IOV_LEN    = %d", (int) sreq->dev.iov[1].MPL_IOV_LEN);
    }

    vc_llc = VC_LLC(vc);
    if (!MPIDI_CH3I_Sendq_empty(vc_llc->send_queue)) {
        need_to_queue = 1;
        goto queue_it;
    }

    {
        int ret;

        ret = llc_writev(vc_llc->endpoint,
                         vc_llc->remote_endpoint_addr,
                         sreq->dev.iov, sreq->dev.iov_count, sreq, &REQ_LLC(sreq)->cmds);
        if (ret < 0) {
            mpi_errno = MPI_ERR_OTHER;
            MPIR_ERR_POP(mpi_errno);
        }
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "WRITEV()   = %d", ret);
        if (!MPIDI_nem_llc_Rqst_iov_update(sreq, ret)) {
            need_to_queue = 2;  /* YYY */
        }
    }

  queue_it:
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "need_to_que  %d", need_to_queue);
    if (need_to_queue > 0) {
        MPIDI_CH3I_Sendq_enqueue(&vc_llc->send_queue, sreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LLC_ISENDCONTIGMSG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_SendNoncontig
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_SendNoncontig(MPIDI_VC_t * vc, MPIR_Request * sreq, void *hdr,
                               intptr_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LLC_SENDNONCONTIG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LLC_SENDNONCONTIG);
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_SendNoncontig");

    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    intptr_t data_sz;
    MPID_nem_llc_vc_area *vc_llc = 0;
    int need_to_queue = 0;

    MPIR_Assert(sreq->dev.segment_first == 0);
    REQ_FIELD(sreq, rma_buf) = NULL;

    sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) & sreq->dev.pending_pkt;
    sreq->dev.iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    sreq->dev.iov_count = 1;
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "IOV_LEN = %d", (int) sreq->dev.iov[0].MPL_IOV_LEN);

    data_sz = sreq->dev.segment_size;
    if (data_sz > 0) {
        REQ_FIELD(sreq, rma_buf) = MPL_malloc((size_t) sreq->dev.segment_size);
        MPIR_ERR_CHKANDJUMP(!REQ_FIELD(sreq, rma_buf), mpi_errno, MPI_ERR_OTHER, "**outofmemory");
        MPIR_Segment_pack(sreq->dev.segment_ptr, sreq->dev.segment_first, &data_sz,
                          (char *) REQ_FIELD(sreq, rma_buf));

        sreq->dev.iov[1].MPL_IOV_BUF = REQ_FIELD(sreq, rma_buf);
        sreq->dev.iov[1].MPL_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "IOV_LEN = %d", (int) sreq->dev.iov[1].MPL_IOV_LEN);
    }

    sreq->ch.vc = vc;
    vc_llc = VC_LLC(vc);
    if (!MPIDI_CH3I_Sendq_empty(vc_llc->send_queue)) {
        need_to_queue = 1;
        goto queue_it;
    }

    {
        int ret;

        ret = llc_writev(vc_llc->endpoint,
                         vc_llc->remote_endpoint_addr,
                         sreq->dev.iov, sreq->dev.iov_count, sreq, &REQ_LLC(sreq)->cmds);
        if (ret < 0) {
            mpi_errno = MPI_ERR_OTHER;
            MPIR_ERR_POP(mpi_errno);
        }

        if (!MPIDI_nem_llc_Rqst_iov_update(sreq, ret)) {
            need_to_queue = 2;  /* YYY */
        }
    }

  queue_it:
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "need_to_que %d", need_to_queue);
    if (need_to_queue > 0) {
        MPIDI_CH3I_Sendq_enqueue(&vc_llc->send_queue, sreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LLC_SENDNONCONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_send_queued
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_send_queued(MPIDI_VC_t * vc, rque_t * send_queue)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_llc_vc_area *vc_llc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LLC_SEND_QUEUED);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LLC_SEND_QUEUED);

    MPIR_Assert(vc != NULL);
    vc_llc = VC_LLC(vc);
    MPIR_Assert(vc_llc != NULL);

    while (!MPIDI_CH3I_Sendq_empty(*send_queue)) {
        ssize_t ret = 0;
        MPIR_Request *sreq;
        void *endpt = vc_llc->endpoint;
        MPL_IOV *iovs;
        int niov;

        sreq = MPIDI_CH3I_Sendq_head(*send_queue);
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "sreq %p", sreq);

        if (mpi_errno == MPI_SUCCESS) {
            iovs = &sreq->dev.iov[sreq->dev.iov_offset];
            niov = sreq->dev.iov_count;

            ret = llc_writev(endpt, vc_llc->remote_endpoint_addr,
                             iovs, niov, sreq, &REQ_LLC(sreq)->cmds);
            if (ret < 0) {
                mpi_errno = MPI_ERR_OTHER;
            }
        }
        if (mpi_errno != MPI_SUCCESS) {
            MPIDI_CH3I_Sendq_dequeue(send_queue, &sreq);
            sreq->status.MPI_ERROR = mpi_errno;

            MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "OnDataAvail = %p", sreq->dev.OnDataAvail);
            MPID_Request_complete(sreq);
            continue;
        }
        if (!MPIDI_nem_llc_Rqst_iov_update(sreq, ret)) {
            MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "skip %p", sreq);
            break;
        }
        MPIDI_CH3I_Sendq_dequeue(send_queue, &sreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LLC_SEND_QUEUED);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_nem_llc_Rqst_iov_update
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_nem_llc_Rqst_iov_update(MPIR_Request * mreq, intptr_t consume)
{
    int ret = TRUE;
    /* intptr_t oconsume = consume; */
    int iv, nv;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEM_LLC_RQST_IOV_UPDATE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEM_LLC_RQST_IOV_UPDATE);

    MPIR_Assert(consume >= 0);

    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() : consume    %d", (int) consume);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() : iov_count  %d", mreq->dev.iov_count);

    nv = mreq->dev.iov_count;
    for (iv = mreq->dev.iov_offset; iv < nv; iv++) {
        MPL_IOV *iov = &mreq->dev.iov[iv];

        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() : iov[iv]    %d", iv);
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() : consume b  %d", (int) consume);
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                       "iov_update() : iov_len b  %d", (int) iov->MPL_IOV_LEN);
        if (iov->MPL_IOV_LEN > consume) {
            iov->MPL_IOV_BUF = ((char *) iov->MPL_IOV_BUF) + consume;
            iov->MPL_IOV_LEN -= consume;
            consume = 0;
            ret = FALSE;
            break;
        }
        consume -= iov->MPL_IOV_LEN;
        iov->MPL_IOV_LEN = 0;
    }
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() : consume %d", (int) consume);

    mreq->dev.iov_count = nv - iv;
    mreq->dev.iov_offset = iv;

    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() : iov_offset %ld", mreq->dev.iov_offset);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iov_update() = %d", ret);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEM_LLC_RQST_IOV_UPDATE);
    return ret;
}

ssize_t llc_writev(void *endpt, uint64_t raddr,
                   const struct iovec * iovs, int niov, void *cbarg, void **vpp_reqid)
{
    ssize_t nw = 0;
    LLC_cmd_t *lcmd = 0;
#ifndef	notdef_hsiz_hack
    uint32_t bsiz;
#endif /* notdef_hsiz_hack */

    dprintf("writev,raddr=%ld,niov=%d,sreq=%p", raddr, niov, cbarg);

    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_writev(%d)", (int) raddr);
    {
        uint8_t *buff = 0;
#ifdef	notdef_hsiz_hack
        uint32_t bsiz;
#endif /* notdef_hsiz_hack */

        {
            int iv, nv = niov;
            bsiz = 0;
            for (iv = 0; iv < nv; iv++) {
                size_t len = iovs[iv].iov_len;

                if (len <= 0) {
                    continue;
                }
                bsiz += len;
            }
#ifdef	notdef_hsiz_hack
            if (bsiz > 0) {
                buff = MPL_malloc(bsiz + sizeof(MPID_nem_llc_netmod_hdr_t));
                if (buff == 0) {
                    nw = -1;    /* ENOMEM */
                    goto bad;
                }
            }
#else /* notdef_hsiz_hack */
            buff = MPL_malloc(bsiz + sizeof(MPID_nem_llc_netmod_hdr_t));
            if (buff == 0) {
                nw = -1;        /* ENOMEM */
                goto bad;
            }
#endif /* notdef_hsiz_hack */
        }

        lcmd = LLC_cmd_alloc2(1, 1, 1);
        if (lcmd == 0) {
            if (buff != 0) {
                MPL_free(buff);
                buff = 0;
            }
            nw = -1;    /* ENOMEM */
            goto bad;
        }

        UNSOLICITED_NUM_INC(cbarg);
        lcmd->opcode = LLC_OPCODE_UNSOLICITED;
        lcmd->comm = LLC_COMM_MPICH;
        lcmd->rank = (uint32_t) raddr;  /* XXX */
        lcmd->req_id = (uint64_t) lcmd;

        lcmd->iov_local[0].addr = (uintptr_t) buff;
        lcmd->iov_local[0].length = bsiz;
        lcmd->niov_local = 1;

        lcmd->iov_remote[0].addr = 0;
        lcmd->iov_remote[0].length = bsiz;
        lcmd->niov_remote = 1;

        {
            struct llc_cmd_area *usr = (void *) lcmd->usr_area;
            usr->cbarg = cbarg;
            usr->raddr = lcmd->rank;
        }
        buff = 0;
    }

    {
        int iv, nv = niov;
        char *bp;
        size_t bz;

        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_writev() : nv %d", nv);
        bp = (void *) lcmd->iov_local[0].addr;
        bz = lcmd->iov_local[0].length;

        /* Prepare netmod header */
        ((MPID_nem_llc_netmod_hdr_t *) bp)->initiator_pg_rank = MPIDI_Process.my_pg_rank;
        bp += sizeof(MPID_nem_llc_netmod_hdr_t);
#ifndef	notdef_hsiz_hack
        lcmd->iov_local[0].length += sizeof(MPID_nem_llc_netmod_hdr_t);
        lcmd->iov_remote[0].length += sizeof(MPID_nem_llc_netmod_hdr_t);
#endif /* notdef_hsiz_hack */

        /* Pack iovs into buff */
        for (iv = 0; iv < nv; iv++) {
            size_t len = iovs[iv].iov_len;

            if (len <= 0) {
                continue;
            }
            if (len > bz) {
                len = bz;
            }
            memcpy(bp, iovs[iv].iov_base, len);
            if ((bz -= len) <= 0) {
                break;
            }
            bp += len;
        }
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_writev() : iv %d", iv);
        {
            void *bb = (void *) lcmd->iov_local[0].addr;
            MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "wptr       = %d", (int) (bp - (char *) bb));
            MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                           "blocklengt = %d", (int) lcmd->iov_local[0].length);
            MPL_DBG_PKT(endpt, bb, "writev");
        }
    }
    {
        int llc_errno;
        llc_errno = LLC_post(lcmd, 1);
        if (llc_errno != 0) {
            if ((llc_errno == EAGAIN) || (llc_errno == ENOSPC)) {
                nw = 0;
            }
            else {
                if (lcmd->iov_local[0].addr != 0) {
                    MPL_free((void *) lcmd->iov_local[0].addr);
                    lcmd->iov_local[0].addr = 0;
                }
                (void) LLC_cmd_free(lcmd, 1);
                nw = -1;
                goto bad;
            }
        }
        else {
#ifdef	notdef_hsiz_hack
            nw = (ssize_t) lcmd->iov_local[0].length;
#else /* notdef_hsiz_hack */
            nw = bsiz;
#endif /* notdef_hsiz_hack */
        }
    }
    if (vpp_reqid != 0) {
        vpp_reqid[0] = lcmd;
    }

  bad:
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "llc_writev() : nw %d", (int) nw);
    return nw;
}

int convert_rank_llc2mpi(MPIR_Comm * comm, int llc_rank, int *mpi_rank)
{
    int size, rank;
    int found = 0;
    MPIDI_VC_t *vc;
    size = MPIR_Comm_size(comm);

    for (rank = 0; rank < size; rank++) {
        MPIDI_Comm_get_vc(comm, rank, &vc);
        MPIDI_CH3I_VC *vc_ch = &vc->ch;

        /* Self-vc isn't initialized, so vc_ch->is_local is 0.
         *
         * If vc_ch->is_local is 1, vc_init is not called.
         *   - vc->comm_ops is not overridden, so send to / receive from this vc
         *     are not via LLC_post.
         *   - VC_FIELD(vc, remote_endpoint_addr) is 0.
         */
        if (vc->pg_rank == MPIDI_Process.my_pg_rank || vc_ch->is_local == 1) {
            if (llc_rank == MPID_nem_llc_my_llc_rank) {
                *mpi_rank = rank;
                found = 1;
                break;
            }
        }
        else if (llc_rank == VC_FIELD(vc, remote_endpoint_addr)) {
            *mpi_rank = rank;   // rank number in the req->comm
            found = 1;
            break;
        }
    }

    return found;
}

int llc_poll(int in_blocking_poll, llc_send_f sfnc, llc_recv_f rfnc)
{
    int mpi_errno = MPI_SUCCESS;

    int llc_errno;
    int nevents;
    LLC_event_t events[1];

    while (1) {
        llc_errno = LLC_poll(LLC_COMM_MPICH, 1, events, &nevents);
        MPIR_ERR_CHKANDJUMP(llc_errno, mpi_errno, MPI_ERR_OTHER, "**LLC_poll");

        LLC_cmd_t *lcmd;
        void *vp_sreq;
        uint64_t reqid = 0;

        if (nevents == 0) {
            break;
        }
        MPIR_Assert(nevents == 1);

        switch (events[0].type) {
        case LLC_EVENT_SEND_LEFT:{
                dprintf("llc_poll,EVENT_SEND_LEFT\n");
                lcmd = (LLC_cmd_t *) events[0].side.initiator.req_id;
                MPIR_Assert(lcmd != 0);
                MPIR_Assert(lcmd->opcode == LLC_OPCODE_SEND || lcmd->opcode == LLC_OPCODE_SSEND);

                if (events[0].side.initiator.error_code != LLC_ERROR_SUCCESS) {
                    printf("llc_poll,error_code=%d\n", events[0].side.initiator.error_code);
                    MPID_nem_llc_segv;
                }

                /* Call send_handler. First arg is a pointer to MPIR_Request */
                (*sfnc) (((struct llc_cmd_area *) lcmd->usr_area)->cbarg, &reqid);

                /* Don't free iov_local[0].addr */

                llc_errno = LLC_cmd_free(lcmd, 1);
                MPIR_ERR_CHKANDJUMP(llc_errno, mpi_errno, MPI_ERR_OTHER, "**LLC_cmd_free");
                break;
            }

        case LLC_EVENT_UNSOLICITED_LEFT:{
                dprintf("llc_poll,EVENT_UNSOLICITED_LEFT\n");
                lcmd = (LLC_cmd_t *) events[0].side.initiator.req_id;
                MPIR_Assert(lcmd != 0);
                MPIR_Assert(lcmd->opcode == LLC_OPCODE_UNSOLICITED);

                struct llc_cmd_area *usr;
                usr = (void *) lcmd->usr_area;
                vp_sreq = usr->cbarg;

                UNSOLICITED_NUM_DEC(vp_sreq);

                if (events[0].side.initiator.error_code != LLC_ERROR_SUCCESS) {
                    printf("llc_poll,error_code=%d\n", events[0].side.initiator.error_code);
                    MPID_nem_llc_segv;
                }
                (*sfnc) (vp_sreq, &reqid);

                if (lcmd->iov_local[0].addr != 0) {
                    MPL_free((void *) lcmd->iov_local[0].addr);
                    lcmd->iov_local[0].addr = 0;
                }
                llc_errno = LLC_cmd_free(lcmd, 1);
                MPIR_ERR_CHKANDJUMP(llc_errno, mpi_errno, MPI_ERR_OTHER, "**LLC_cmd_free");

                break;
            }
        case LLC_EVENT_UNSOLICITED_ARRIVED:{
                void *vp_vc = 0;
                void *buff;
                size_t bsiz;

                buff = events[0].side.responder.addr;
                bsiz = events[0].side.responder.length;
#ifndef	notdef_hsiz_hack
#if defined(__sparc__)
                MPIR_Assert(((uintptr_t) buff % 8) == 0);
#endif
#endif /* notdef_hsiz_hack */
                {
                    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "LLC_leng   = %d", (int) bsiz);
                    MPL_DBG_PKT(vp_vc, buff, "poll");
                }
                dprintf("llc_poll,EVENT_UNSOLICITED_ARRIVED,%d<-%d\n",
                        MPIDI_Process.my_pg_rank,
                        ((MPID_nem_llc_netmod_hdr_t *) buff)->initiator_pg_rank);
#ifdef	notdef_hsiz_hack
                (*rfnc) (vp_vc,
                         ((MPID_nem_llc_netmod_hdr_t *) buff)->initiator_pg_rank,
                         (uint8_t *) buff + sizeof(MPID_nem_llc_netmod_hdr_t), bsiz);
#else /* notdef_hsiz_hack */
                (*rfnc) (vp_vc,
                         ((MPID_nem_llc_netmod_hdr_t *) buff)->initiator_pg_rank,
                         (uint8_t *) buff + sizeof(MPID_nem_llc_netmod_hdr_t),
                         bsiz - sizeof(MPID_nem_llc_netmod_hdr_t));
#endif /* notdef_hsiz_hack */
                llc_errno = LLC_release_buffer(&events[0]);
                MPIR_ERR_CHKANDJUMP(llc_errno, mpi_errno, MPI_ERR_OTHER, "**LLC_release_buffer");

                break;
            }
        case LLC_EVENT_RECV_MATCHED:{
                dprintf("llc_poll,EVENT_RECV_MATCHED\n");
                lcmd = (LLC_cmd_t *) events[0].side.initiator.req_id;
                MPIR_Request *req = ((struct llc_cmd_area *) lcmd->usr_area)->cbarg;

                if (req->kind != MPIR_REQUEST_KIND__MPROBE) {
                    /* Unpack non-contiguous dt */
                    int is_contig;
                    MPIR_Datatype_is_contig(req->dev.datatype, &is_contig);
                    if (!is_contig) {
                        dprintf("llc_poll,unpack noncontiguous data to user buffer\n");

                        /* see MPIDI_CH3U_Request_unpack_uebuf (in /src/mpid/ch3/src/ch3u_request.c) */
                        /* or MPIDI_CH3U_Receive_data_found (in src/mpid/ch3/src/ch3u_handle_recv_pkt.c) */

                        /* set_request_info() sets req->dev.recv_data_sz to pkt->data_sz.
                         * pkt->data_sz is sender's request size.
                         */
                        intptr_t unpack_sz = events[0].side.initiator.length;
                        MPIR_Segment seg;
                        MPI_Aint last;

                        /* user_buf etc. are set in MPID_irecv --> MPIDI_CH3U_Recvq_FDU_or_AEP */
                        MPIR_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype,
                                          &seg, 0);
                        last = unpack_sz;
                        MPIR_Segment_unpack(&seg, 0, &last, REQ_FIELD(req, pack_buf));
                        if (last != unpack_sz) {
                            /* --BEGIN ERROR HANDLING-- */
                            /* received data was not entirely consumed by unpack()
                             * because too few bytes remained to fill the next basic
                             * datatype */
                            MPIR_STATUS_SET_COUNT(req->status, last);
                            req->status.MPI_ERROR =
                                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME,
                                                     __LINE__, MPI_ERR_TYPE, "**llc_poll", 0);
                            /* --END ERROR HANDLING-- */
                        }
                        dprintf("llc_poll,ref_count=%d,pack_buf=%p\n", req->ref_count,
                                REQ_FIELD(req, pack_buf));
                        MPL_free(REQ_FIELD(req, pack_buf));
                    }

                    req->status.MPI_TAG = events[0].side.initiator.tag & 0xffffffff;;
                    if (req->dev.match.parts.rank != MPI_ANY_SOURCE) {
                        req->status.MPI_SOURCE = req->dev.match.parts.rank;
                    }
                    else {
                        /* 'events[0].side.initiator.rank' is LLC rank.
                         * Convert it to a rank number in the communicator. */
                        int found = 0;
                        found =
                            convert_rank_llc2mpi(req->comm, events[0].side.initiator.rank,
                                                 &req->status.MPI_SOURCE);
                        MPIR_Assert(found);
                    }

                    if (unlikely(events[0].side.initiator.error_code == LLC_ERROR_TRUNCATE)) {
                        req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                        MPIR_STATUS_SET_COUNT(req->status, lcmd->iov_local[0].length);
                    }
                    else {
                        MPIR_STATUS_SET_COUNT(req->status, events[0].side.initiator.length);
                    }

                    /* Dequeue request from posted queue.
                     * It's posted in MPID_Irecv --> MPIDI_CH3U_Recvq_FDU_or_AEP */
                    int found = MPIDI_CH3U_Recvq_DP(req);
                    MPIR_Assert(found);
                }

                /* Mark completion on rreq */
                MPID_Request_complete(req);

                llc_errno = LLC_cmd_free(lcmd, 1);
                MPIR_ERR_CHKANDJUMP(llc_errno, mpi_errno, MPI_ERR_OTHER, "**LLC_cmd_free");
                break;
            }
        case LLC_EVENT_TARGET_PROC_FAIL:{
                MPIR_Request *req;

                lcmd = (LLC_cmd_t *) events[0].side.initiator.req_id;
                MPIR_Assert(lcmd != 0);

                req = ((struct llc_cmd_area *) lcmd->usr_area)->cbarg;

                if (lcmd->opcode == LLC_OPCODE_UNSOLICITED) {
                    struct llc_cmd_area *usr;
                    usr = (void *) lcmd->usr_area;
                    vp_sreq = usr->cbarg;

                    UNSOLICITED_NUM_DEC(vp_sreq);

                    req->status.MPI_ERROR = MPI_SUCCESS;
                    MPIR_ERR_SET(req->status.MPI_ERROR, MPIX_ERR_PROC_FAIL_STOP, "**comm_fail");

                    MPID_Request_complete(req);

                    if (lcmd->iov_local[0].addr != 0) {
                        MPL_free((void *) lcmd->iov_local[0].addr);
                        lcmd->iov_local[0].addr = 0;
                    }
                }
                else if (lcmd->opcode == LLC_OPCODE_SEND || lcmd->opcode == LLC_OPCODE_SSEND) {
                    req->status.MPI_ERROR = MPI_SUCCESS;
                    MPIR_ERR_SET(req->status.MPI_ERROR, MPIX_ERR_PROC_FAIL_STOP, "**comm_fail");

                    MPID_Request_complete(req);
                }
                else if (lcmd->opcode == LLC_OPCODE_RECV) {
                    /* Probably ch3 dequeued and completed this request. */
#if 0
                    MPIDI_CH3U_Recvq_DP(req);
                    req->status.MPI_ERROR = MPI_SUCCESS;
                    MPIR_ERR_SET(req->status.MPI_ERROR, MPIX_ERR_PROC_FAIL_STOP, "**comm_fail");
                    MPID_Request_complete(req);
#endif
                }
                else {
                    printf("llc_poll,target dead, unknown opcode=%d\n", lcmd->opcode);
                    MPID_nem_llc_segv;
                }

                llc_errno = LLC_cmd_free(lcmd, 1);
                MPIR_ERR_CHKANDJUMP(llc_errno, mpi_errno, MPI_ERR_OTHER, "**LLC_cmd_free");

                break;
            }
        default:
            printf("llc_poll,unknown event type=%d\n", events[0].type);
            MPID_nem_llc_segv;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_issend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
                        int dest, int tag, MPIR_Comm * comm, int context_offset,
                        struct MPIR_Request **request)
{
    int mpi_errno = MPI_SUCCESS, llc_errno;
    int dt_contig;
    intptr_t data_sz;
    MPIR_Datatype*dt_ptr;
    MPI_Aint dt_true_lb;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LLC_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LLC_ISSEND);

    dprintf
        ("llc_isend,%d->%d,buf=%p,count=%d,datatype=%08x,dest=%d,tag=%08x,comm=%p,context_offset=%d\n",
         MPIDI_Process.my_pg_rank, vc->pg_rank, buf, count, datatype, dest, tag, comm,
         context_offset);

    int LLC_my_rank;
    LLC_comm_rank(LLC_COMM_MPICH, &LLC_my_rank);
    dprintf("llc_isend,LLC_my_rank=%d\n", LLC_my_rank);

    struct MPIR_Request *sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_Assert(sreq != NULL);
    MPIR_Object_set_ref(sreq, 2);
    sreq->kind = MPIR_REQUEST_KIND__SEND;

    /* Used in llc_poll --> MPID_nem_llc_send_handler */
    sreq->ch.vc = vc;
    sreq->dev.OnDataAvail = 0;
    /* Don't save iov_offset because it's not used. */

    /* Save it because it's used in send_handler */
    /*   See also MPIDI_Request_create_sreq() */
    /*     in src/mpid/ch3/include/mpidimpl.h */
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    sreq->dev.datatype = datatype;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    /* used for MPI_Cancel() */
    sreq->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, FALSE);
    sreq->dev.cancel_pending = FALSE;
    /* Do not reset dev.state after calling MPIDI_Request_set_type() */
    /* sreq->dev.state = 0; */
    sreq->dev.match.parts.rank = dest;
    sreq->dev.match.parts.tag = tag;
    sreq->dev.match.parts.context_id = comm->context_id + context_offset;

    dprintf("llc_isend,remote_endpoint_addr=%ld\n", VC_FIELD(vc, remote_endpoint_addr));

    LLC_cmd_t *cmd = LLC_cmd_alloc2(1, 1, 1);
    cmd[0].opcode = LLC_OPCODE_SSEND;
    cmd[0].comm = LLC_COMM_MPICH;
    cmd[0].rank = VC_FIELD(vc, remote_endpoint_addr);
    cmd[0].req_id = (uint64_t) cmd;

    /* Prepare bit-vector to perform tag-match. We use the same bit-vector as in CH3 layer. */
    /* See src/mpid/ch3/src/mpid_isend.c */
    *(int32_t *) ((uint8_t *) & cmd[0].tag) = tag;
    *(MPIR_Context_id_t *) ((uint8_t *) & cmd[0].tag + sizeof(int32_t)) =
        comm->context_id + context_offset;
    MPIR_Assert(sizeof(LLC_tag_t) >= sizeof(int32_t) + sizeof(MPIR_Context_id_t));
    memset((uint8_t *) & cmd[0].tag + sizeof(int32_t) + sizeof(MPIR_Context_id_t),
           0, sizeof(LLC_tag_t) - sizeof(int32_t) - sizeof(MPIR_Context_id_t));

    dprintf("llc_isend,tag=");
    for (i = 0; i < sizeof(LLC_tag_t); i++) {
        dprintf("%02x", (int) *((uint8_t *) & cmd[0].tag + i));
    }
    dprintf("\n");

    /* Prepare RDMA-write from buffer */
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    dprintf("llc_isend,dt_contig=%d,data_sz=%ld\n", dt_contig, data_sz);


    const void *write_from_buf;
    if (dt_contig) {
        write_from_buf = (void *) ((char *) buf + dt_true_lb);
        REQ_FIELD(sreq, pack_buf) = 0;
    }
    else {
        /* See MPIDI_CH3_EagerNoncontigSend (in ch3u_eager.c) */
        struct MPIR_Segment *segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP(!segment_ptr, mpi_errno, MPI_ERR_OTHER, "**outofmemory");
        /* See also MPIDI_CH3_Request_create and _destory() */
        /*     in src/mpid/ch3/src/ch3u_request.c */
        sreq->dev.segment_ptr = segment_ptr;

        MPIR_Segment_init(buf, count, datatype, segment_ptr, 0);
        intptr_t segment_first = 0;
        intptr_t segment_size = data_sz;
        intptr_t last = segment_size;
        MPIR_Assert(last > 0);
        REQ_FIELD(sreq, pack_buf) = MPL_malloc((size_t) data_sz);
        MPIR_ERR_CHKANDJUMP(!REQ_FIELD(sreq, pack_buf), mpi_errno, MPI_ERR_OTHER, "**outofmemory");
        MPIR_Segment_pack(segment_ptr, segment_first, &last, (char *) (REQ_FIELD(sreq, pack_buf)));
        MPIR_Assert(last == data_sz);
        write_from_buf = REQ_FIELD(sreq, pack_buf);
    }

    cmd[0].iov_local[0].addr = (uint64_t) write_from_buf;
    cmd[0].iov_local[0].length = data_sz;
    cmd[0].niov_local = 1;

    cmd[0].iov_remote[0].addr = 0;
    cmd[0].iov_remote[0].length = data_sz;
    cmd[0].niov_remote = 1;

    ((struct llc_cmd_area *) cmd[0].usr_area)->cbarg = sreq;
    ((struct llc_cmd_area *) cmd[0].usr_area)->raddr = VC_FIELD(vc, remote_endpoint_addr);

    llc_errno = LLC_post(cmd, 1);
    MPIR_ERR_CHKANDJUMP(llc_errno != LLC_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**LLC_post");

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LLC_ISSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
