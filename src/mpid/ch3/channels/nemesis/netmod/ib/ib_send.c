/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 *      See COPYRIGHT in top-level directory.
 */

#include "ib_impl.h"

//#define MPID_NEM_IB_DEBUG_SEND
#ifdef dprintf  /* avoid redefinition with src/mpid/ch3/include/mpidimpl.h */
#undef dprintf
#endif
#ifdef MPID_NEM_IB_DEBUG_SEND
#define dprintf printf
#else
#define dprintf(...)
#endif

static int entered_send_progress = 0;

#ifdef MPID_NEM_IB_ONDEMAND
#if 0
// tree format is
// one or more <left_pointer(int), right_pointer(int), value(int), length(int), string(char[])>
#define MPID_NEM_IB_MAP_LPTR(ptr) *(int*)((ptr) + sizeof(int)*0)
#define MPID_NEM_IB_MAP_RPTR(ptr) *(int*)((ptr) + sizeof(int)*1)
#define MPID_NEM_IB_MAP_VAL(ptr) *(int*)((ptr) + sizeof(int)*2)
#define MPID_NEM_IB_MAP_LEN(ptr) *(int*)((ptr) + sizeof(int)*3)
#define MPID_NEM_IB_MAP_PBODY(ptr) ((ptr) + sizeof(int)*4)

#define MPID_NEM_IB_MAP_ALLOCATE(map, key, key_length, initial) {                        \
    if (map->length + key_length + sizeof(int)*4 > map->max_length) { \
        map->max_length = map->max_length ? map->max_length * 2 : 4096; \
        map->data = realloc(map->data, map->max_length); \
    } \
    char* new_str = map->data + map->length; \
    MPID_NEM_IB_MAP_LPTR(new_str) = 0; \
    MPID_NEM_IB_MAP_RPTR(new_str) = 0; \
    MPID_NEM_IB_MAP_VAL(new_str) = initial; \
    MPID_NEM_IB_MAP_LEN(new_str) = key_length; \
    memcpy(MPID_NEM_IB_MAP_PBODY(new_str), key, key_length); \
    map->length += sizeof(int)*4 + key_length; \
}

void MPID_nem_ib_cm_map_set(MPID_nem_ib_cm_map_t * map, char *key, int key_length, int val)
{
    char *pTree = map->data;
    dprintf("MPID_nem_ib_cm_map_set,val=%d\n", val);

    if (!pTree) {
        MPID_NEM_IB_MAP_ALLOCATE(map, key, key_length, val);
        dprintf("pTree was empty\n");
        return;
    }
    int s1_minus_s2;
    while (1) {
        int lmin =
            key_length < MPID_NEM_IB_MAP_LEN(pTree) ? key_length : MPID_NEM_IB_MAP_LEN(pTree);
        int residual = key_length - MPID_NEM_IB_MAP_LEN(pTree);
        s1_minus_s2 = memcmp(key, MPID_NEM_IB_MAP_PBODY(pTree), lmin);

        if (!s1_minus_s2 && !residual) {
            MPID_NEM_IB_MAP_VAL(pTree) = val;
            dprintf("found\n");
            return;     // same string, same length
        }
        else if (s1_minus_s2 < 0 || !s1_minus_s2 && residual < 0) {
            // psArg is "smaller" OR same substring, psArg is shorter
            if (MPID_NEM_IB_MAP_LPTR(pTree) == 0) {
                MPID_NEM_IB_MAP_LPTR(pTree) = map->length;      // pointer write
                /* left child */
                MPID_NEM_IB_MAP_ALLOCATE(map, key, key_length, val);
                dprintf("stored as left child\n");
                return;
            }
            pTree = map->data + MPID_NEM_IB_MAP_LPTR(pTree);    // go to left child
        }
        else {
            //  psArg is "larger" OR same substring, psArg is longer
            if (MPID_NEM_IB_MAP_RPTR(pTree) == 0) {
                MPID_NEM_IB_MAP_RPTR(pTree) = map->length;      // pointer write
                /* right child */
                MPID_NEM_IB_MAP_ALLOCATE(map, key, key_length, val);
                dprintf("stored as right child\n");
                return;
            }
            pTree = map->data + MPID_NEM_IB_MAP_RPTR(pTree);    // go to right child
        }
    }
}

int MPID_nem_ib_cm_map_get(MPID_nem_ib_cm_map_t * map, char *key, int key_length, int *val)
{
    int mpi_errno = MPI_SUCCESS;
    char *pTree = map->data;

    dprintf("MPID_nem_ib_cm_map_get,key=%s\n", key);

    if (!pTree) {
        mpi_errno = -1;
        dprintf("pTree is empty\n");
        goto fn_fail;
    }
    int s1_minus_s2;
    while (1) {
        int lmin =
            key_length < MPID_NEM_IB_MAP_LEN(pTree) ? key_length : MPID_NEM_IB_MAP_LEN(pTree);
        int residual = key_length - MPID_NEM_IB_MAP_LEN(pTree);
        s1_minus_s2 = memcmp(key, MPID_NEM_IB_MAP_PBODY(pTree), lmin);

        if (!s1_minus_s2 && !residual) {
            *val = MPID_NEM_IB_MAP_VAL(pTree);
            dprintf("value found=%d\n", *val);
            goto fn_exit;       // same string, same length
        }
        else if (s1_minus_s2 < 0 || !s1_minus_s2 && residual < 0) {
            // psArg is "smaller" OR same substring, psArg is shorter
            if (MPID_NEM_IB_MAP_LPTR(pTree) == 0) {
                mpi_errno = -1;
                dprintf("left is null\n");
                goto fn_fail;
            }
            pTree = map->data + MPID_NEM_IB_MAP_LPTR(pTree);    // go to left child
        }
        else {
            //  psArg is "larger" OR same substring, psArg is longer
            if (MPID_NEM_IB_MAP_RPTR(pTree) == 0) {
                mpi_errno = -1;
                dprintf("right is null\n");
                goto fn_fail;
            }
            pTree = map->data + MPID_NEM_IB_MAP_RPTR(pTree);    // go to right child
        }
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_iSendContig_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_ib_iSendContig_core(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                                        MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    MPID_nem_ib_pkt_prefix_t pkt_netmod;
    void *prefix;
    int sz_prefix;
    void *s_data;
    int s_data_sz;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_ISENDCONTIG_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_ISENDCONTIG_CORE);

    /* piggy-back SR occupancy info might copy and modify given header */

    //dprintf("isendcontig,rsr_seq_num_tail=%d,rsr_seq_num_tail_last_sent=%d\n", vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent);

    int notify_rate;
    ibcom_errno = MPID_nem_ib_com_rdmabuf_occupancy_notify_rate_get(vc_ib->sc->fd, &notify_rate);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_sq_occupancy_notify_rate_get");

    /* send RDMA-write-to buffer occupancy information */
    /* embed SR occupancy information and remember the last one sent */
#if 0
    MPIDI_CH3_Pkt_t *ch3_hdr = (MPIDI_CH3_Pkt_t *) hdr;
#endif
    if (MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent)
        > notify_rate) {
#if 0   /* debug, disabling piggy-back */
        switch (ch3_hdr->type) {
        case MPIDI_CH3_PKT_EAGER_SEND:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_EAGER_SEND;
            goto common_tail;
#if 0   /* modification of mpid_nem_lmt.c is required */
        case MPIDI_NEM_PKT_LMT_RTS:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_LMT_RTS;
            goto common_tail;
#endif
        case MPIDI_CH3_PKT_PUT:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_PUT;
            goto common_tail;
        case MPIDI_CH3_PKT_ACCUMULATE:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_ACCUMULATE;
            goto common_tail;
        case MPIDI_CH3_PKT_GET:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_GET;
            goto common_tail;
        case MPIDI_CH3_PKT_GET_RESP:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_GET_RESP;
          common_tail:
            pkt_netmod.type = MPIDI_NEM_PKT_NETMOD;
            pkt_netmod.seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
            vc_ib->ibcom->rsr_seq_num_tail_last_sent = vc_ib->ibcom->rsr_seq_num_tail;
            prefix = (void *) &pkt_netmod;
            sz_prefix = sizeof(MPID_nem_ib_pkt_prefix_t);
            break;
        default:
            prefix = NULL;
            sz_prefix = 0;
            break;
        }
#else
        prefix = NULL;
        sz_prefix = 0;
#endif
    }
    else {
        prefix = NULL;
        sz_prefix = 0;
    }

    s_data = data;
    s_data_sz = data_sz;

    if (hdr &&
          ((((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_PUT)
            || (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET_RESP)
            || (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_ACCUMULATE))) {
        /* If request length is too long, create LMT packet */
        if (MPID_NEM_IB_NETMOD_HDR_SIZEOF(vc_ib->ibcom->local_ringbuf_type)
               + sizeof(MPIDI_CH3_Pkt_t) + data_sz
                 > MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_netmod_trailer_t)) {
            pkt_netmod.type = MPIDI_NEM_PKT_NETMOD;

            if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_PUT)
                pkt_netmod.subtype = MPIDI_NEM_IB_PKT_PUT;
            else if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET_RESP)
                pkt_netmod.subtype = MPIDI_NEM_IB_PKT_GET_RESP;
            else if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_ACCUMULATE)
                pkt_netmod.subtype = MPIDI_NEM_IB_PKT_ACCUMULATE;

            void *write_from_buf = data;

            uint32_t max_msg_sz;
            MPID_nem_ib_com_get_info_conn(vc_ib->sc->fd, MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ,
                                          &max_msg_sz, sizeof(uint32_t));

            /* RMA : Netmod IB supports only smaller size than max_msg_sz. */
            MPIU_Assert(data_sz <= max_msg_sz);

            MPID_nem_ib_rma_lmt_cookie_t *s_cookie_buf = (MPID_nem_ib_rma_lmt_cookie_t *) MPIU_Malloc(sizeof(MPID_nem_ib_rma_lmt_cookie_t));

            sreq->ch.s_cookie = s_cookie_buf;

            s_cookie_buf->tail = *((uint8_t *) ((uint8_t *) write_from_buf + data_sz - sizeof(uint8_t)));
            /* put IB rkey */
            struct MPID_nem_ib_com_reg_mr_cache_entry_t *mr_cache =
                MPID_nem_ib_com_reg_mr_fetch(write_from_buf, data_sz, 0,
                                             MPID_NEM_IB_COM_REG_MR_GLOBAL);
            MPIU_ERR_CHKANDJUMP(!mr_cache, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_reg_mr_fetch");
            struct ibv_mr *mr = mr_cache->mr;
            REQ_FIELD(sreq, lmt_mr_cache) = (void *) mr_cache;
#ifdef HAVE_LIBDCFA
            s_cookie_buf->addr = (void *) mr->host_addr;
#else
            s_cookie_buf->addr = write_from_buf;
#endif
            s_cookie_buf->rkey = mr->rkey;
            s_cookie_buf->len = data_sz;
            s_cookie_buf->sender_req_id = sreq->handle;
            s_cookie_buf->max_msg_sz = max_msg_sz;

	    /* set for ib_com_isend */
	    prefix = (void *)&pkt_netmod;
	    sz_prefix = sizeof(MPIDI_CH3_Pkt_t);
	    s_data = (void *)s_cookie_buf;
	    s_data_sz = sizeof(MPID_nem_ib_rma_lmt_cookie_t);

	    /* Release Request, when sender receives DONE packet. */
            int incomplete;
            MPIDI_CH3U_Request_increment_cc(sreq, &incomplete); // decrement in drain_scq and pkt_rma_lmt_getdone
        }
    }

    /* packet handlers including MPIDI_CH3_PktHandler_EagerSend and MPID_nem_handle_pkt assume this */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);

    /* send myrank as wr_id so that receiver can find vc using MPID_nem_ib_conns in poll */
    /* packet handler of MPIDI_CH3_PKT_EAGER_SEND uses sizeof(MPIDI_CH3_Pkt_t), so ignoring hdr_sz */

    /* MPIDI_CH3_ReqHandler_GetSendRespComplete, drain_scq decrement it */
    if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET_RESP) {
        //        MPIR_Request_add_ref(sreq);
        //printf("isendcontig_core,MPIDI_CH3_PKT_GET_RESP,ref_count=%d\n", sreq->ref_count);
    }

    /* increment cc because PktHandler_EagerSyncAck, ssend.c, drain_scq decrement it */
    if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_EAGER_SYNC_SEND) {
        //MPIR_Request_add_ref(sreq);
    }
    if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET) {
        //printf("isendcontig_core,MPIDI_CH3_PKT_GET,ref_count=%d\n", sreq->ref_count);
    }
    if (hdr && ((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_ACCUM_IMMED) {
        dprintf("isendcontig_core,MPIDI_CH3_PKT_ACCUM_IMMED,ref_count=%d\n", sreq->ref_count);
    }
    if (hdr && ((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_ACCUMULATE) {
        dprintf("isendcontig_core,MPIDI_CH3_PKT_ACCUMULATE,ref_count=%d\n", sreq->ref_count);
    }

#ifdef MPID_NEM_IB_DEBUG_SEND
    int msg_type = MPIDI_Request_get_msg_type(sreq);
#endif

    dprintf
        ("isendcontig_core,sreq=%p,prefix=%p,sz_prefix=%d,hdr=%p,sz_hdr=%ld,data=%p,sz_data=%d,remote_ringbuf->type=%d\n",
         sreq, prefix, sz_prefix, hdr, hdr_sz, data, (int) data_sz,
         vc_ib->ibcom->remote_ringbuf->type);

    if (sizeof(MPIDI_CH3_Pkt_t) != hdr_sz) {
        printf("type=%d,subtype=%d\n", ((MPID_nem_pkt_netmod_t *) hdr)->type,
               ((MPID_nem_pkt_netmod_t *) hdr)->subtype);
    }

    int copied;
    ibcom_errno =
        MPID_nem_ib_com_isend(vc_ib->sc->fd,
                              (uint64_t) sreq,
                              prefix, sz_prefix,
                              hdr, hdr_sz,
                              s_data, (int) s_data_sz,
                              &copied,
                              vc_ib->ibcom->local_ringbuf_type, vc_ib->ibcom->remote_ringbuf->type,
                              &REQ_FIELD(sreq, buf_from), &REQ_FIELD(sreq, buf_from_sz));
    MPIU_ERR_CHKFATALANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_isend");
    MPID_nem_ib_ncqe += 1;
    //dprintf("isendcontig_core,ncqe=%d\n", MPID_nem_ib_ncqe);
    dprintf("isendcontig_core,isend,kind=%d,msg_type=%d,copied=%d\n", sreq->kind, msg_type, copied);    /*suspicious lines,(if1,on,on,off,if0) works */
#if 0
#define MPID_NEM_IB_TLBPREF_SEND 20     //20
    int tlb_pref_ahd = 4096 * MPID_NEM_IB_TLBPREF_SEND;
    __asm__ __volatile__
        ("movq %0, %%rsi;"
         "movq 0(%%rsi), %%rax;"::"r"((uint64_t) data + tlb_pref_ahd):"%rsi", "%rax");
#endif
#if 1
#ifdef __MIC__
    __asm__ __volatile__
        ("movq %0, %%rsi;"
         "vprefetch0 0x00(%%rsi);"
         "vprefetch0 0x40(%%rsi);"
         "vprefetch0 0x80(%%rsi);"
         "vprefetch0 0xc0(%%rsi);"::"r"((uint64_t) data + 4 * data_sz):"%rsi");
#else
    __asm__ __volatile__
        ("movq %0, %%rsi;"
         "prefetchnta 0x00(%%rsi);"
         "prefetchnta 0x40(%%rsi);"
         "prefetchnta 0x80(%%rsi);"
         "prefetchnta 0xc0(%%rsi);"::"r"((uint64_t) data + 4 * data_sz):"%rsi");
#endif
#endif

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "ib_send, fd=%d", vc_ib->sc->fd));
    vc_ib->pending_sends += 1;
    sreq->ch.vc = vc;   /* used in poll */

    /* calling drain_scq from progress_send deprives of chance
     * for ib_poll to drain-sendq using ncqe
     * however transfers events to
     * (not to reply_seq_num because it's regulated by the rate)
     * fire on ib_poll using nces
     * (make SCQ full once, then put one command in sendq,
     * then send-->drain-scq to reduce CQE level under the threashold)
     * so we need to perform
     * progress_send for all of VCs using nces in ib_poll
     * (if we have drain-sendq in ib_poll, this isn't needed. */
#if 0   /* debug,disabling fast-dec-cc when copied */
    if (copied && !sreq->dev.OnDataAvail) {     /* skip poll scq */
        int (*reqFn) (MPIDI_VC_t *, MPID_Request *, int *);

        (VC_FIELD(sreq->ch.vc, pending_sends)) -= 1;

        /* as in the template */
        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn) {
            /* MPID_Request_release is called in
             * MPI_Wait (in src/mpi/pt2pt/wait.c)
             * MPIR_Wait_impl (in src/mpi/pt2pt/wait.c)
             * MPIR_Request_complete (in /src/mpi/pt2pt/mpir_request.c) */
            int incomplete;
            MPIDI_CH3U_Request_decrement_cc(sreq, &incomplete);
            if (!incomplete) {
                MPIDI_CH3_Progress_signal_completion();
            }
            //dprintf("isendcontig_core,cc_ptr=%d\n", *(sreq->cc_ptr));
            dprintf("sendcontig_core,copied,complete,req=%p,cc incremented to %d,ref_count=%d\n",
                    sreq, MPIDI_CH3I_progress_completion_count.v, sreq->ref_count);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        }
        else {
            MPIDI_VC_t *vc = sreq->ch.vc;
            int complete = 0;
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            /* not-completed case is not implemented */
            MPIU_Assert(complete == TRUE);
            MPIU_Assert(0);     /* decrement ref_count and free sreq causes problem */
        }
    }
    else {
        MPID_nem_ib_ncqe_nces += 1;     /* it has different meaning, counting non-copied eager-send */
    }
#else
    MPID_nem_ib_ncqe_nces += 1; /* it has different meaning, counting non-copied eager-send */
#endif

#ifndef MPID_NEM_IB_DISABLE_VAR_OCC_NOTIFY_RATE
    //dprintf("isendcontig,old rstate=%d\n", vc_ib->ibcom->rdmabuf_occupancy_notify_rstate);
    int *notify_rstate;
    ibcom_errno =
        MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get(vc_ib->sc->fd, &notify_rstate);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get");

    dprintf("isendcontig,head=%d,tail=%d,hw=%d\n", vc_ib->ibcom->sseq_num,
            vc_ib->ibcom->lsr_seq_num_tail, MPID_NEM_IB_COM_RDMABUF_HIGH_WATER_MARK);
    /* if the number of slots in RMDA-write-to buffer have hit the high water-mark */
    if (*notify_rstate == MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW &&
        MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                           vc_ib->ibcom->lsr_seq_num_tail) >
        MPID_NEM_IB_COM_RDMABUF_HIGH_WATER_MARK) {
        dprintf("changing notify_rstate,id=%d\n", vc_ib->ibcom->sseq_num);
        /* remember remote notifying policy so that local can know when to change remote policy back to LW */
        *notify_rstate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW;
        /* change remote notifying policy of RDMA-write-to buf occupancy info */
        MPID_nem_ib_send_change_rdmabuf_occupancy_notify_state(vc,
                                                               MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW);
    }
    //dprintf("isendcontig_core,new rstate=%d\n", vc_ib->ibcom->rdmabuf_occupancy_notify_rstate);
#endif

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_ISENDCONTIG_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_iSendContig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                            MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
#if 0
    int ibcom_errno;
#endif
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_ISENDCONTIG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_ISENDCONTIG);

    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ib_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) hdr);

    if (vc_ib->connection_state == MPID_NEM_IB_CM_CLOSED) {
        if (vc_ib->connection_guard == 0) {
            vc_ib->connection_guard = 1;
            /* connected=no,ringbuf-type=shared,slot-available=no,
             * going-to-be-enqueued=yes case */
            MPID_nem_ib_cm_cas(vc, 0);  /* Call ask_fetch just after it's connected */
        }

    }
    if (vc_ib->connection_state != MPID_NEM_IB_CM_ESTABLISHED) {
        /* connected=closed/transit,ringbuf-type=shared,slot-available=no,
         * going-to-be-enqueued=yes case */
        REQ_FIELD(sreq, ask) = 0;       /* We can't ask because ring-buffer type is not determined yet. */
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_ringbuf_ask_fetch");
    }
    else {
        /* connected=established,ringbuf-type=shared,slot-available=no,
         * going-to-be-enqueued=yes case */
        if (vc_ib->ibcom->local_ringbuf_type == MPID_NEM_IB_RINGBUF_SHARED &&
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                               vc_ib->ibcom->lsr_seq_num_tail) >=
            vc_ib->ibcom->local_ringbuf_nslot) {
            dprintf("isendcontig,RINGBUF_SHARED and full,asking\n");
            mpi_errno = MPID_nem_ib_ringbuf_ask_fetch(vc);
            REQ_FIELD(sreq, ask) = 1;
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_ringbuf_ask_fetch");
        }
        else {
            REQ_FIELD(sreq, ask) = 0;
        }
    }

#if 0
    /* aggressively perform drain_scq */
    /* try to clear the road blocks, i.e. ncom, ncqe */
    if (vc_ib->ibcom->ncom >=
        /*MPID_NEM_IB_COM_MAX_SQ_CAPACITY */ MPID_NEM_IB_COM_MAX_SQ_HEIGHT_DRAIN ||
        MPID_nem_ib_ncqe >=
        /*MPID_NEM_IB_COM_MAX_CQ_CAPACITY */ MPID_NEM_IB_COM_MAX_CQ_HEIGHT_DRAIN) {
        //printf("isendcontig,kick drain_scq\n");
        ibcom_errno = MPID_nem_ib_drain_scq(1); /* set 1st arg to one which means asking it for not calling send_progress because it recursively call isendcontig_core */
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_drain_scq");
    }
#endif
    /* set it for drain_scq */
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);



#ifdef MPID_NEM_IB_ONDEMAND
    if (vc_ib->connection_state != MPID_NEM_IB_CM_ESTABLISHED) {
        goto enqueue;
    }
#endif

#if 0
    /* anticipating received message releases RDMA-write-to buffer or IB command-queue entry */
    /* Unexpected state MPIDI_VC_STATE_CLOSED in vc 0xf1fed0 (expecting MPIDI_VC_STATE_ACTIVE)
     * Assertion failed in file src/mpid/ch3/src/ch3u_handle_connection.c at line 326: vc->state == MPIDI_VC_STATE_ACTIVE */
    if (vc->state == MPIDI_VC_STATE_ACTIVE &&
        MPID_nem_ib_tsc_poll - MPID_nem_ib_rdtsc() > MPID_NEM_IB_POLL_PERIOD_SEND) {
        mpi_errno = MPID_nem_ib_poll(0);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }
#endif

    dprintf
        ("isendcontig,%d->%d,req=%p,type=%d,subtype=%d,data_sz=%ld,ldiff=%d(%d-%d),rdiff=%d(%d-%d)\n",
         MPID_nem_ib_myrank, vc->pg_rank, sreq, ((MPIDI_CH3_Pkt_t *) hdr)->type,
         ((MPID_nem_pkt_netmod_t *) hdr)->subtype, data_sz,
         MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
         vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail,
         MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail,
                            vc_ib->ibcom->rsr_seq_num_tail_last_sent),
         vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent);
    dprintf("isendcontig,sendq_empty=%d,ncom=%d,ncqe=%d,rdmabuf_occ=%d\n",
            MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom, MPID_nem_ib_ncqe,
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));
    /* if IB command overflow-queue is empty AND local IB command queue isn't full AND remote RDMA-write-to buf isn't getting overrun */
    MPIDI_CH3_Pkt_t *ch3_hdr = (MPIDI_CH3_Pkt_t *) hdr;
    MPID_nem_pkt_netmod_t *prefix = (MPID_nem_pkt_netmod_t *) hdr;
    /* reserve one slot for control packet bringing sequence number
     * to avoid dead-lock */
    int slack = ((ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                  prefix->subtype != MPIDI_NEM_IB_PKT_REQ_SEQ_NUM) &&
                 (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                  prefix->subtype != MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) &&
                 (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                  prefix->subtype != MPIDI_NEM_IB_PKT_LMT_GET_DONE) &&
                 ch3_hdr->type != MPIDI_NEM_PKT_LMT_RTS &&
                 ch3_hdr->type != MPIDI_NEM_PKT_LMT_CTS) ? MPID_NEM_IB_COM_AMT_SLACK : 0;
    /* make control packet bringing sequence number go ahead of
     * queued packets to avoid dead-lock */
    int goahead =
        (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD && prefix->subtype == MPIDI_NEM_IB_PKT_REQ_SEQ_NUM)
        || (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD &&
            prefix->subtype == MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) ||
        (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD && prefix->subtype == MPIDI_NEM_IB_PKT_LMT_GET_DONE)
        ? 1 : 0;
    dprintf("isendcontig,slack=%d,goahead=%d\n", slack, goahead);

    if ((goahead || MPID_nem_ib_sendq_empty(vc_ib->sendq)) &&
        vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack &&
        MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack &&
        MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                           vc_ib->ibcom->lsr_seq_num_tail) <
        vc_ib->ibcom->local_ringbuf_nslot - slack) {

        mpi_errno = MPID_nem_ib_iSendContig_core(vc, sreq, hdr, hdr_sz, data, data_sz);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    }
    else {
        /* enqueue command into send_queue */
        dprintf
            ("isendcontig,enqueuing,goahead=%d,sendq=%d,ncom=%d,ncqe=%d,ldiff=%d(%d-%d),slack=%d\n",
             goahead, MPID_nem_ib_sendq_empty(vc_ib->sendq),
             vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack,
             MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack,
             MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
             vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail, slack);

        /* this location because the above message refers undefined */
      enqueue:

        /* store required info. see MPIDI_CH3_iSendv in src/mpid/ch3/channels/nemesis/src/ch3_isendv.c */
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->dev.pending_pkt;
        sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;
        sreq->dev.iov[1].MPID_IOV_BUF = (char *) data;
        sreq->dev.iov[1].MPID_IOV_LEN = data_sz;

        sreq->dev.iov_count = 2;
        sreq->dev.iov_offset = 0;
        sreq->ch.noncontig = FALSE;     /* used in send_progress */
        sreq->ch.vc = vc;

        if (data_sz > 0) {
            dprintf
                ("isendcontig,hdr=%p,hdr_sz=%ld,data=%p,data_sz=%ld,*(sreq->dev.iov[1].MPID_IOV_BUF)=%08x,sz=%ld,sz=%ld\n",
                 hdr, hdr_sz, data, data_sz, *((uint32_t *) sreq->dev.iov[1].MPID_IOV_BUF),
                 sizeof(sreq->dev.pending_pkt), sizeof(MPIDI_CH3_Pkt_t));
        }

        /* enqueue control message telling tail position of ring buffer for eager-send
         * at the head of software MPI command queue. We explain the reason. Consider this case.
         * rank-0 performs 64 eager-sends and 48 of them are enqueued.
         * rank-1 consumes 2 of them and send the control message.
         * rank-0 drains 2 commands from the command queue.
         * ...
         * rank-0 finds that head of ring buffer for receiving messages from rank-1 is
         * growing by the control message from rank-1 and try to send the control message,
         * but the command is queued at the tail.
         * rank-1 stops sending the control message to rank-1 because the ring buffer is full
         * rank-0 stops draining command queue.
         */
        dprintf("isendcontig,enqueuing,type=%d,\n", ((MPIDI_CH3_Pkt_t *) hdr)->type);
#if 0
        if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_NEM_PKT_NETMOD) {
            switch (((MPID_nem_pkt_netmod_t *) hdr)->subtype) {
            case MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM:
                dprintf("enqueuing REPLY_SEQ_NUM\ %d->%d,%d\n", MPID_nem_ib_myrank, vc->pg_rank,
                        MPID_nem_ib_ncqe);
                break;
            default:
                break;
            }
        }
        else {
            switch (((MPIDI_CH3_Pkt_t *) hdr)->type) {
            case MPIDI_CH3_PKT_ACCUMULATE:
                dprintf("enqueuing ACCUMULATE\n");
                break;
            case MPIDI_CH3_PKT_GET_RESP:
                dprintf("enqueuing GET_RESP\n");
                break;
            case MPIDI_CH3_PKT_GET:
                dprintf("enqueuing GET\n");
                break;
            case MPIDI_CH3_PKT_PUT:
                dprintf("enqueuing PUT\n");
                break;
            case MPIDI_NEM_PKT_LMT_DONE:
                dprintf("isendcontig,enqueue,DONE\n");
                break;
            default:
                break;
            }
        }
#endif
        if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_NEM_PKT_NETMOD &&
            ((MPID_nem_pkt_netmod_t *) hdr)->subtype == MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) {
            dprintf("isendcontig,REPLY_SEQ_NUM,enqueue_at_head\n");
            MPID_nem_ib_sendq_enqueue_at_head(&vc_ib->sendq, sreq);
        }
        else {
            MPID_nem_ib_sendq_enqueue(&vc_ib->sendq, sreq);
        }
        /* we don't need to perform send_progress() here because
         * the events where RDMA-write-to buffer release is detected or release of IB command queue id detected happens
         * only after ib_poll is called. it's different than the case where write(2) is used */
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_ISENDCONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr)
{
    MPID_Request *sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
#if 0
    int ibcom_errno;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    int sseq_num;
#endif
    //uint64_t tscs, tsce;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_ISTARTCONTIGMSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_ISTARTCONTIGMSG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ib_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) hdr);

    /* FIXME: avoid creating a request when not queued */

    if (hdr && ((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET) {
        //printf("istarctontig,MPIDI_CH3_PKT_GET,ref_count=%d\n", sreq->ref_count);
        /* sreq here is released by drain_scq, caller
         * request in MPIDI_CH3I_Recv_rma_msg is
         * released by PKT_GET_RESP, MPIDI_CH3I_RMAListComplete */
    }

    //tscs = MPID_nem_ib_rdtsc();
    sreq = MPID_Request_create();
    MPIU_Assert(sreq != NULL);
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = 0;
    //tsce = MPID_nem_ib_rdtsc(); printf("rc,%ld\n", tsce - tscs); // 124.15 cycles

#if 0
    if (hdr) {

        MPIDI_CH3_Pkt_t *pkt = (MPIDI_CH3_Pkt_t *) hdr;
        MPIDI_CH3_Pkt_close_t *close_pkt = &pkt->close;
        dprintf("isend(istartcontig),%d->%d,seq_num=%d,type=%d,ack=%d\n", MPID_nem_ib_myrank,
                vc->pg_rank, vc_ib->ibcom->sseq_num, close_pkt->type, close_pkt->ack);
    }
    else {
        dprintf("isend(istartcontig),%d->%d,seq_num=%d\n", MPID_nem_ib_myrank, vc->pg_rank,
                vc_ib->ibcom->sseq_num);
    }
#endif

    mpi_errno = MPID_nem_ib_iSendContig(vc, sreq, hdr, hdr_sz, data, data_sz);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    *sreq_ptr = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_ISTARTCONTIGMSG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_SendNoncontig_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_ib_SendNoncontig_core(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                                          MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    MPIDI_msg_sz_t last;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    void *prefix;
    int prefix_sz;
    void *data;
    int data_sz;
    MPID_nem_ib_pkt_prefix_t pkt_netmod;

    prefix = NULL;
    prefix_sz = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG_CORE);

    MPIU_Assert(sreq->dev.segment_first == 0);
    last = sreq->dev.segment_size;      /* segment_size is byte offset */
    if (last > 0) {
        REQ_FIELD(sreq, lmt_pack_buf) = MPIU_Malloc((size_t) sreq->dev.segment_size);
        MPIU_ERR_CHKANDJUMP(!REQ_FIELD(sreq, lmt_pack_buf), mpi_errno, MPI_ERR_OTHER,
                            "**outofmemory");
        MPID_Segment_pack(sreq->dev.segment_ptr, sreq->dev.segment_first, &last,
                          (char *) REQ_FIELD(sreq, lmt_pack_buf));
        MPIU_Assert(last == sreq->dev.segment_size);
    }

    data = (void *)REQ_FIELD(sreq, lmt_pack_buf);
    data_sz = last;

    if (hdr &&
          ((((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_PUT)
            || (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET_RESP)
            || (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_ACCUMULATE))) {
	/* If request length is too long, create LMT packet */
	if ( MPID_NEM_IB_NETMOD_HDR_SIZEOF(vc_ib->ibcom->local_ringbuf_type)
               + sizeof(MPIDI_CH3_Pkt_t) + sreq->dev.segment_size
                 > MPID_NEM_IB_COM_RDMABUF_SZSEG - sizeof(MPID_nem_ib_netmod_trailer_t)) {
            pkt_netmod.type = MPIDI_NEM_PKT_NETMOD;

            if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_PUT)
                pkt_netmod.subtype = MPIDI_NEM_IB_PKT_PUT;
            else if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_GET_RESP)
                pkt_netmod.subtype = MPIDI_NEM_IB_PKT_GET_RESP;
            else if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_ACCUMULATE)
                pkt_netmod.subtype = MPIDI_NEM_IB_PKT_ACCUMULATE;

            void *write_from_buf = REQ_FIELD(sreq, lmt_pack_buf);

            uint32_t max_msg_sz;
            MPID_nem_ib_com_get_info_conn(vc_ib->sc->fd, MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ,
                                          &max_msg_sz, sizeof(uint32_t));

            /* RMA : Netmod IB supports only smaller size than max_msg_sz. */
            MPIU_Assert(data_sz <= max_msg_sz);

            MPID_nem_ib_rma_lmt_cookie_t *s_cookie_buf = (MPID_nem_ib_rma_lmt_cookie_t *) MPIU_Malloc(sizeof(MPID_nem_ib_rma_lmt_cookie_t));

            sreq->ch.s_cookie = s_cookie_buf;

            s_cookie_buf->tail = *((uint8_t *) ((uint8_t *) write_from_buf + last - sizeof(uint8_t)));
            /* put IB rkey */
            struct MPID_nem_ib_com_reg_mr_cache_entry_t *mr_cache =
                MPID_nem_ib_com_reg_mr_fetch(write_from_buf, last, 0,
                                             MPID_NEM_IB_COM_REG_MR_GLOBAL);
            MPIU_ERR_CHKANDJUMP(!mr_cache, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_reg_mr_fetch");
            struct ibv_mr *mr = mr_cache->mr;
            REQ_FIELD(sreq, lmt_mr_cache) = (void *) mr_cache;
#ifdef HAVE_LIBDCFA
            s_cookie_buf->addr = (void *) mr->host_addr;
#else
            s_cookie_buf->addr = write_from_buf;
#endif
            s_cookie_buf->rkey = mr->rkey;
            s_cookie_buf->len = last;
            s_cookie_buf->sender_req_id = sreq->handle;
            s_cookie_buf->max_msg_sz = max_msg_sz;

	    /* set for ib_com_isend */
	    prefix = (void *)&pkt_netmod;
	    prefix_sz = sizeof(MPIDI_CH3_Pkt_t);
	    data = (void *)s_cookie_buf;
	    data_sz = sizeof(MPID_nem_ib_rma_lmt_cookie_t);

	    /* Release Request, when sender receives DONE packet. */
            int incomplete;
            MPIDI_CH3U_Request_increment_cc(sreq, &incomplete); // decrement in drain_scq and pkt_rma_lmt_getdone
        }
    }

    /* packet handlers assume this */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);

    /* increment cc because PktHandler_EagerSyncAck, ssend.c, drain_scq decrement it */
    if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_EAGER_SYNC_SEND) {
        //MPIR_Request_add_ref(sreq);
    }

    if (sizeof(MPIDI_CH3_Pkt_t) != hdr_sz) {
        printf("type=%d,subtype=%d\n", ((MPID_nem_pkt_netmod_t *) hdr)->type,
               ((MPID_nem_pkt_netmod_t *) hdr)->subtype);
    }

    int copied;
    dprintf("sendnoncontig_core,isend,%d->%d,seq_num=%d,remote_ringbuf->type=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank, vc_ib->ibcom->sseq_num,
            vc_ib->ibcom->remote_ringbuf->type);

    ibcom_errno =
        MPID_nem_ib_com_isend(vc_ib->sc->fd,
                              (uint64_t) sreq,
                              prefix, prefix_sz,
                              hdr, hdr_sz,
                              data, data_sz,
                              &copied,
                              vc_ib->ibcom->local_ringbuf_type, vc_ib->ibcom->remote_ringbuf->type,
                              &REQ_FIELD(sreq, buf_from), &REQ_FIELD(sreq, buf_from_sz));
    MPIU_ERR_CHKANDJUMP(ibcom_errno != 0, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_isend");
    MPID_nem_ib_ncqe += 1;
    dprintf("sendnoncontig_core,ncqe=%d\n", MPID_nem_ib_ncqe);

    vc_ib->pending_sends += 1;
    sreq->ch.vc = vc;   /* used in poll */

#if 0   /* see contig */
    if (copied) {       /* skip poll scq */
        int (*reqFn) (MPIDI_VC_t *, MPID_Request *, int *);

        (VC_FIELD(sreq->ch.vc, pending_sends)) -= 1;

        /* as in the template */
        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn) {
            /* MPID_Request_release is called in
             * MPI_Wait (in src/mpi/pt2pt/wait.c)
             * MPIR_Wait_impl (in src/mpi/pt2pt/wait.c)
             * MPIR_Request_complete (in /src/mpi/pt2pt/mpir_request.c) */
            int incomplete;
            MPIDI_CH3U_Request_decrement_cc(sreq, &incomplete);
            if (!incomplete) {
                MPIDI_CH3_Progress_signal_completion();
            }
            //dprintf("isendcontig_core,cc_ptr=%d\n", *(sreq->cc_ptr));
            dprintf("sendcontig_core,complete,req=%p,cc incremented to %d\n", sreq,
                    MPIDI_CH3I_progress_completion_count.v);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        }
        else {
            MPIDI_VC_t *vc = sreq->ch.vc;
            int complete = 0;
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            /* not-completed case is not implemented */
            MPIU_Assert(complete == TRUE);
            MPIU_Assert(0);     /* decrement ref_count and free sreq causes problem */
        }
    }
    else {
        MPID_nem_ib_ncqe_nces += 1;     /* it has different meaning, counting non-copied eager-short */
    }
#else
    MPID_nem_ib_ncqe_nces += 1; /* it has different meaning, counting non-copied eager-short */
#endif

#ifndef MPID_NEM_IB_DISABLE_VAR_OCC_NOTIFY_RATE
#if 1
    //dprintf("isendcontig,old rstate=%d\n", vc_ib->ibcom->rdmabuf_occupancy_notify_rstate);
    int *notify_rstate;
    ibcom_errno =
        MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get(vc_ib->sc->fd, &notify_rstate);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get");

    dprintf("isendcontig,head=%d,tail=%d,hw=%d\n", vc_ib->ibcom->sseq_num,
            vc_ib->ibcom->lsr_seq_num_tail, MPID_NEM_IB_COM_RDMABUF_HIGH_WATER_MARK);
    /* if the number of slots in RMDA-write-to buffer have hit the high water-mark */
    if (*notify_rstate == MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW &&
        MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                           vc_ib->ibcom->lsr_seq_num_tail) >
        MPID_NEM_IB_COM_RDMABUF_HIGH_WATER_MARK) {
        dprintf("changing notify_rstate,id=%d\n", vc_ib->ibcom->sseq_num);
        /* remember remote notifying policy so that local can know when to change remote policy back to LW */
        *notify_rstate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW;
        /* change remote notifying policy of RDMA-write-to buf occupancy info */
        MPID_nem_ib_send_change_rdmabuf_occupancy_notify_state(vc,
                                                               MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW);
    }
    //dprintf("isendcontig_core,new rstate=%d\n", vc_ib->ibcom->rdmabuf_occupancy_notify_rstate);
#endif
#endif
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_SendNoncontig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                              MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
#if 0
    int ibcom_errno;
    MPIDI_msg_sz_t last;
#endif
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ib_SendNoncontig");

    if (vc_ib->connection_state == MPID_NEM_IB_CM_CLOSED) {
        if (vc_ib->connection_guard == 0) {
            vc_ib->connection_guard = 1;
            /* connected=closed,ringbuf-type=shared,slot-available=no,
             * going-to-be-enqueued=yes case */
            MPID_nem_ib_cm_cas(vc, 0);  /* Call ask_fetch just after it's connected */
        }
    }
    if (vc_ib->connection_state != MPID_NEM_IB_CM_ESTABLISHED) {
        /* connected=closed/transit,ringbuf-type=shared,slot-available=no,
         * going-to-be-enqueued=yes case */
        REQ_FIELD(sreq, ask) = 0;
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_ringbuf_ask_fetch");
    }
    else {
        /* connected=established,ringbuf-type=shared,slot-available=no,
         * going-to-be-enqueued=yes case */
        if (vc_ib->ibcom->local_ringbuf_type == MPID_NEM_IB_RINGBUF_SHARED &&
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                               vc_ib->ibcom->lsr_seq_num_tail) >=
            vc_ib->ibcom->local_ringbuf_nslot) {
            dprintf("sendnoncontig,RINGBUF_SHARED and full,asking\n");
            REQ_FIELD(sreq, ask) = 1;
            mpi_errno = MPID_nem_ib_ringbuf_ask_fetch(vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_ringbuf_ask_fetch");
        }
        else {
            REQ_FIELD(sreq, ask) = 0;
        }
    }

    dprintf("sendnoncontig,%d->%d,sendq_empty=%d,ncom=%d,rdmabuf_occ=%d\n", MPID_nem_ib_myrank,
            vc->pg_rank, MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom,
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));
#if 0
    /* aggressively perform drain_scq */
    /* try to clear the road blocks, i.e. ncom, ncqe */
    if (vc_ib->ibcom->ncom >=
        /*MPID_NEM_IB_COM_MAX_SQ_CAPACITY */ MPID_NEM_IB_COM_MAX_SQ_HEIGHT_DRAIN ||
        MPID_nem_ib_ncqe >=
        /*MPID_NEM_IB_COM_MAX_CQ_CAPACITY */ MPID_NEM_IB_COM_MAX_CQ_HEIGHT_DRAIN) {
        //printf("isendcontig,kick drain_scq\n");
        ibcom_errno = MPID_nem_ib_drain_scq(1); /* set 1st arg to one which means asking it for not calling send_progress because it recursively call isendcontig_core */
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_drain_scq");
    }
#endif
    /* set it for drain_scq */
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);

    /* if IB command overflow-queue is empty AND local IB command queue isn't full AND remote RDMA-write-to buf isn't getting overrun */
    /* set it for drain_scq */
    int slack = MPID_NEM_IB_COM_AMT_SLACK;      /* slack for control packet bringing sequence number */

    if (
#ifdef MPID_NEM_IB_ONDEMAND
           vc_ib->connection_state == MPID_NEM_IB_CM_ESTABLISHED &&
#endif
           MPID_nem_ib_sendq_empty(vc_ib->sendq) &&
           vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack &&
           MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack &&
           MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                              vc_ib->ibcom->lsr_seq_num_tail) <
           vc_ib->ibcom->local_ringbuf_nslot - slack) {

        mpi_errno = MPID_nem_ib_SendNoncontig_core(vc, sreq, hdr, hdr_sz);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    }
    else {
        /* enqueue command into send_queue */
        dprintf("sendnoncontig, enqueuing");
        //enqueue:
        /* store required info. see MPIDI_CH3_iSendv in src/mpid/ch3/channels/nemesis/src/ch3_isendv.c */
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->dev.pending_pkt;
        sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;

        sreq->dev.iov_count = 1;
        sreq->dev.iov_offset = 0;
        sreq->ch.noncontig = TRUE;
        sreq->ch.vc = vc;

        MPID_nem_ib_sendq_enqueue(&vc_ib->sendq, sreq);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* see MPIDI_CH3I_Shm_send_progress (in src/mpid/ch3/channels/nemesis/src/ch3_progress.c) */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_send_progress(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
#if 0
    int ibcom_errno;
#endif
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    MPID_Request *sreq, *prev_sreq;
    int req_type, msg_type;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_SEND_PROGRESS);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_SEND_PROGRESS);

    //dprintf("send_progress,enter\n");

#ifdef MPID_NEM_IB_ONDEMAND
    if (vc_ib->connection_state != MPID_NEM_IB_CM_ESTABLISHED) {
        //dprintf("send_progress,connection_state=%08x\n", vc_ib->connection_state);
        goto fn_exit;
    }
#endif

    /* prevent a call path send_progress -> drain_scq -> send_progress */
    if (entered_send_progress) {
        goto fn_exit;
    }
    entered_send_progress = 1;

    sreq = MPID_nem_ib_sendq_head(vc_ib->sendq);
    if (sreq) {
        prev_sreq = NULL;
        do {
#if 0
            /* aggressively perform drain_scq */
            /* try to clear the road blocks, i.e. ncom, ncqe */
            if (vc_ib->ibcom->ncom >=
                /*MPID_NEM_IB_COM_MAX_SQ_CAPACITY */ MPID_NEM_IB_COM_MAX_SQ_HEIGHT_DRAIN ||
                MPID_nem_ib_ncqe >=
                /*MPID_NEM_IB_COM_MAX_CQ_CAPACITY */ MPID_NEM_IB_COM_MAX_CQ_HEIGHT_DRAIN) {
                dprintf("send_progress,kick drain_scq\n");
                ibcom_errno = MPID_nem_ib_drain_scq(1); /* set 1st arg to one which means asking it for not calling send_progress because it recursively call isendcontig_core */
                MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_drain_scq");
            }
#endif
            req_type = MPIDI_Request_get_type(sreq);
            msg_type = MPIDI_Request_get_msg_type(sreq);

            MPIDI_CH3_Pkt_t *ch3_hdr = (MPIDI_CH3_Pkt_t *) sreq->dev.iov[0].MPID_IOV_BUF;
            MPID_nem_pkt_netmod_t *netmod_pkt =
                (MPID_nem_pkt_netmod_t *) sreq->dev.iov[0].MPID_IOV_BUF;
            int slack = (msg_type == MPIDI_REQUEST_EAGER_MSG) ? /* guard from RDMA-read or RDMA-write */
                (((ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                   netmod_pkt->subtype != MPIDI_NEM_IB_PKT_REQ_SEQ_NUM) &&
                  (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                   netmod_pkt->subtype != MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) &&
                  (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                   netmod_pkt->subtype != MPIDI_NEM_IB_PKT_LMT_GET_DONE) &&
                  ch3_hdr->type != MPIDI_NEM_PKT_LMT_RTS &&
                  ch3_hdr->type !=
                  MPIDI_NEM_PKT_LMT_CTS) ? MPID_NEM_IB_COM_AMT_SLACK : 0) :
                MPID_NEM_IB_COM_AMT_SLACK;

            /* Temporary fix until removing slack */
            if (vc_ib->ibcom->local_ringbuf_type == MPID_NEM_IB_RINGBUF_SHARED) {
                slack = 0;
            }

            /* Refill slots from queue
             * We don't need refill code in sendcontig because
             * there is an order where (1) send, (2) it's queued, (3) then ask obtains slots,
             * (4) then we can refill them here. */

            if (vc_ib->ibcom->local_ringbuf_type == MPID_NEM_IB_RINGBUF_SHARED &&
                (msg_type == MPIDI_REQUEST_EAGER_MSG &&
                 MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                                    vc_ib->ibcom->lsr_seq_num_tail) >=
                 vc_ib->ibcom->local_ringbuf_nslot)) {
                /* Prevent RDMA-read for rendezvous protocol from issuing ask */

                if (!REQ_FIELD(sreq, ask)) {    /* First packet after connection hasn't asked slot */
                    /* Transitioning from exclusive to shared and need to issue ask.
                     * This case is detected because exclusive entries in the queue are deleted
                     * and deprived of slots of exclusive and the last state is set to
                     * shared when deciding a transition from exclusive to shared
                     * and an issued or queued ask must be in the queue or ringbuf_sendq
                     * when staying shared. */
                    dprintf("send_progress,call ask_fetch,%d->%d\n",
                            MPID_nem_ib_myrank, vc->pg_rank);
                    mpi_errno = MPID_nem_ib_ringbuf_ask_fetch(vc);
                    REQ_FIELD(sreq, ask) = 1;
                    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                        "**MPID_nem_ib_ringbuf_ask_fetch");
                }
                else if (!MPID_nem_ib_ringbuf_sectorq_empty(vc_ib->ibcom->sectorq)) {
                    /* Staying shared or transitioning from shared to exclusive.
                     * We need to consume acquires slots in the latter case.
                     * Transitioning from shared to exclusive is achieved by
                     * finding an exlusive entry. */
                    MPID_nem_ib_ringbuf_sector_t *sector =
                        MPID_nem_ib_ringbuf_sectorq_head(vc_ib->ibcom->sectorq);

                    vc_ib->ibcom->local_ringbuf_type = sector->type;
                    vc_ib->ibcom->local_ringbuf_start = sector->start;
                    vc_ib->ibcom->local_ringbuf_nslot = sector->nslot;
                    vc_ib->ibcom->sseq_num = sector->head;
                    vc_ib->ibcom->lsr_seq_num_tail = sector->tail;

                    MPID_nem_ib_ringbuf_sectorq_dequeue(&vc_ib->ibcom->sectorq, &sector);
                    MPIU_Free(sector);

                    dprintf
                        ("send_progress,refill,next type=%d,start=%p,local_head=%d,local_tail=%d\n",
                         vc_ib->ibcom->local_ringbuf_type, vc_ib->ibcom->local_ringbuf_start,
                         vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail);
                }
            }

            if (vc_ib->ibcom->ncom >= MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack ||
                MPID_nem_ib_ncqe >= MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack ||
                (msg_type == MPIDI_REQUEST_EAGER_MSG &&
                 MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                                    vc_ib->ibcom->lsr_seq_num_tail) >=
                 vc_ib->ibcom->local_ringbuf_nslot - slack)) {
                /* Exit when full because this reduces the search cost.
                 * Note that RDMA-read for rendezvous protocol can be issued even
                 * when no ring-buffer slot is available. */
                goto fn_exit;
            }

            if (vc_ib != MPID_nem_ib_debug_current_vc_ib) {
                dprintf("send_progress,vc_ib != MPID_nem_ib_debug_current_vc_ib\n");
            }
            dprintf("send_progress,req=%p,kind=%d,msg_type=%d\n", sreq, sreq->kind, msg_type);
            if (msg_type == MPIDI_REQUEST_EAGER_MSG) {
                dprintf("send_progress,ch3_hdr->type=%d\n", ch3_hdr->type);
            }
            dprintf("send_progress,%d->%d,rdiff=%d(%d-%d),ldiff=%d(%d-%d),slack=%d\n",
                    MPID_nem_ib_myrank, sreq->ch.vc->pg_rank,
                    MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail,
                                       vc_ib->ibcom->rsr_seq_num_tail_last_sent),
                    vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent,
                    MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num,
                                       vc_ib->ibcom->lsr_seq_num_tail),
                    vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail, slack);
            if (sreq->kind == MPID_REQUEST_SEND && msg_type == MPIDI_REQUEST_EAGER_MSG) {
                if (!sreq->ch.noncontig) {
                    dprintf
                        ("send_progress,contig,ch3_hdr->type=%d,sseq_num=%d,MPIDI_NEM_PKT_LMT_RTS=%d,MPIDI_NEM_IB_PKT_LMT_GET_DONE=%d\n",
                         ch3_hdr->type, vc_ib->ibcom->sseq_num, MPIDI_NEM_PKT_LMT_RTS,
                         MPIDI_NEM_IB_PKT_LMT_GET_DONE);
                    if (sreq->dev.iov[1].MPID_IOV_LEN > 0) {
                        dprintf
                            ("send_progress,send,contig,sreq->dev.iov[1].MPID_IOV_BUF)=%p,*(sreq->dev.iov[1].MPID_IOV_BUF)=%08x\n",
                             sreq->dev.iov[1].MPID_IOV_BUF,
                             *((uint32_t *) sreq->dev.iov[1].MPID_IOV_BUF));
                    }
                    MPIU_Assert(sreq->dev.iov_count > 0);

                    switch (ch3_hdr->type) {
                        /* send current rsr_seq_num_tail because message from target to initiator
                         * might have happened while being queued */
                    case MPIDI_NEM_PKT_LMT_RTS:{
#if 0
                            MPID_nem_ib_lmt_cookie_t *s_cookie_buf =
                                (MPID_nem_ib_lmt_cookie_t *) sreq->dev.iov[1].MPID_IOV_BUF;
#endif
                            dprintf("send_progress,MPIDI_NEM_PKT_LMT_RTS,rsr_seq_num_tail=%d\n",
                                    vc_ib->ibcom->rsr_seq_num_tail);
#if 0   /* moving to packet header */
                            /* embed RDMA-write-to buffer occupancy information */
                            s_cookie_buf->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                            /* remember the last one sent */
                            vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                vc_ib->ibcom->rsr_seq_num_tail;
#endif
                            break;
                        }

                    case MPIDI_NEM_PKT_LMT_CTS:{
#if 0
                            MPID_nem_ib_lmt_cookie_t *s_cookie_buf =
                                (MPID_nem_ib_lmt_cookie_t *) sreq->dev.iov[1].MPID_IOV_BUF;
#endif
                            dprintf("send_progress,MPIDI_NEM_PKT_LMT_CTS,rsr_seq_num_tail=%d\n",
                                    vc_ib->ibcom->rsr_seq_num_tail);
#if 0   /* moving to packet header */
                            /* embed RDMA-write-to buffer occupancy information */
                            s_cookie_buf->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                            /* remember the last one sent */
                            vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                vc_ib->ibcom->rsr_seq_num_tail;
#endif
                            break;
                        }

                    default:;
                    }

                    if (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD) {
                        switch (netmod_pkt->subtype) {
                            /* send current rsr_seq_num_tail because message from target to initiator
                             * might have happened while being queued */
                        case MPIDI_NEM_IB_PKT_LMT_GET_DONE:{
#if 0
                                MPID_nem_ib_pkt_lmt_get_done_t *_done_pkt =
                                    (MPID_nem_ib_pkt_lmt_get_done_t *) sreq->dev.iov[0].
                                    MPID_IOV_BUF;
                                dprintf
                                    ("send_progress,MPIDI_NEM_IB_PKT_LMT_GET_DONE,rsr_seq_num_tail=%d\n",
                                     vc_ib->ibcom->rsr_seq_num_tail);
                                /* embed SR occupancy information */
                                _done_pkt->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                                /* remember the last one sent */
                                vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                    vc_ib->ibcom->rsr_seq_num_tail;
#endif
                                break;
                            }
                        case MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM:{
                                MPID_nem_ib_pkt_reply_seq_num_t *_pkt =
                                    (MPID_nem_ib_pkt_reply_seq_num_t *) sreq->dev.iov[0].
                                    MPID_IOV_BUF;
                                dprintf
                                    ("send_progress,MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM,rsr_seq_num_tail=%d\n",
                                     vc_ib->ibcom->rsr_seq_num_tail);
                                /* embed SR occupancy information */
                                _pkt->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                                /* remember the last one sent */
                                vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                    vc_ib->ibcom->rsr_seq_num_tail;
                                break;
                            }

                        default:;
                        }
                    }


                    mpi_errno =
                        MPID_nem_ib_iSendContig_core(sreq->ch.vc, sreq,
                                                     sreq->dev.iov[0].MPID_IOV_BUF,
                                                     sreq->dev.iov[0].MPID_IOV_LEN,
                                                     sreq->dev.iov[1].MPID_IOV_BUF,
                                                     sreq->dev.iov[1].MPID_IOV_LEN);
                    if (mpi_errno) {
                        MPIU_ERR_POP(mpi_errno);
                    }
                }
                else {
                    dprintf("send_progress,send,noncontig\n");
                    mpi_errno =
                        MPID_nem_ib_SendNoncontig_core(sreq->ch.vc, sreq,
                                                       sreq->dev.iov[0].MPID_IOV_BUF,
                                                       sreq->dev.iov[0].MPID_IOV_LEN);
                    if (mpi_errno) {
                        MPIU_ERR_POP(mpi_errno);
                    }
                }
            }
            else if (sreq->kind == MPID_REQUEST_RECV && msg_type == MPIDI_REQUEST_RNDV_MSG) {

                dprintf("send_progress,kick lmt_start_recv_core,prev=%p,next=%p\n", prev_sreq,
                        MPID_nem_ib_sendq_next(sreq));
                mpi_errno =
                    MPID_nem_ib_lmt_start_recv_core(sreq, REQ_FIELD(sreq, lmt_raddr),
                                                    REQ_FIELD(sreq, lmt_rkey), REQ_FIELD(sreq,
                                                                                         lmt_szsend),
                                                    REQ_FIELD(sreq, lmt_write_to_buf),
                                                    REQ_FIELD(sreq, max_msg_sz), REQ_FIELD(sreq,
                                                                                           last));
                if (mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }
            }
            else if (sreq->kind == MPID_REQUEST_SEND && msg_type == MPIDI_REQUEST_RNDV_MSG) {
            }
            else {
                dprintf("send_progress,unknown sreq=%p,sreq->kind=%d,msg_type=%d\n", sreq,
                        sreq->kind, msg_type);
                assert(0);
                MPIU_ERR_INTERNALANDJUMP(mpi_errno, "send_progress,unknown type");
            }


            /* unlink sreq */
            if (prev_sreq != NULL) {
                MPID_nem_ib_sendq_next(prev_sreq) = MPID_nem_ib_sendq_next(sreq);
            }
            else {
                MPID_nem_ib_sendq_head(vc_ib->sendq) = MPID_nem_ib_sendq_next(sreq);
            }
            if (MPID_nem_ib_sendq_next(sreq) == NULL) {
                vc_ib->sendq.tail = prev_sreq;
            }

            /* save sreq->dev.next (and sreq) because decrementing reference-counter might free sreq */
            //MPID_Request *tmp_sreq = sreq;
            sreq = MPID_nem_ib_sendq_next(sreq);
            goto next_unlinked;
            //next:
            prev_sreq = sreq;
            sreq = MPID_nem_ib_sendq_next(sreq);
          next_unlinked:;
            if (!sreq) {
                dprintf("send_progress,sendq has got empty!\n");
            }
        } while (sreq);
    }

    //dprintf("send_progress,exit,sendq_empty=%d,ncom=%d,ncqe=%d,rdmabuf_occ=%d\n", MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom, MPID_nem_ib_ncqe, MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));

  fn_exit:
    entered_send_progress = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_SEND_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifdef MPID_NEM_IB_ONDEMAND
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_req_t *sreq, *prev_sreq;
    MPID_nem_ib_cm_cmd_shadow_t *shadow;
    int is_established = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_PROGRESS);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_PROGRESS);

    //dprintf("cm_send_progress,enter\n");

    sreq = MPID_nem_ib_cm_sendq_head(MPID_nem_ib_cm_sendq);
    if (sreq) {
        prev_sreq = NULL;
        do {

            if (sreq->ibcom->ncom >= MPID_NEM_IB_COM_MAX_SQ_CAPACITY ||
                MPID_nem_ib_ncqe_scratch_pad >= MPID_NEM_IB_COM_MAX_CQ_CAPACITY) {
                goto next;
            }

            switch (sreq->state) {
            case MPID_NEM_IB_CM_CAS:
                if (is_conn_established(sreq->responder_rank)) {
                    dprintf("cm_progress,cm_cas,connection is already established\n");
                    is_established = 1;
                    break;
                }

                /* This comparison is OK if the diff is within 63-bit range */
                if (MPID_nem_ib_diff63(MPID_nem_ib_progress_engine_vt, sreq->retry_decided) <
                    sreq->retry_backoff) {
#if 0
                    dprintf("cm_progress,vt=%ld,retry_decided=%ld,diff=%ld,backoff=%ld\n",
                            MPID_nem_ib_progress_engine_vt, sreq->retry_decided,
                            MPID_nem_ib_diff63(MPID_nem_ib_progress_engine_vt, sreq->retry_decided),
                            sreq->retry_backoff);
#endif
                    goto next;
                }
                dprintf
                    ("cm_progress,retry CAS,responder_rank=%d,req=%p,decided=%ld,vt=%ld,backoff=%ld\n",
                     sreq->responder_rank, sreq, sreq->retry_decided,
                     MPID_nem_ib_progress_engine_vt, sreq->retry_backoff);
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                mpi_errno = MPID_nem_ib_cm_cas_core(sreq->responder_rank, shadow);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_connect_cas_core");
                break;
            case MPID_NEM_IB_CM_CAS_RELEASE:
                dprintf
                    ("cm_progress,retry CAS_RELEASE,responder_rank=%d,req=%p,decided=%ld,vt=%ld,backoff=%ld\n",
                     sreq->responder_rank, sreq, sreq->retry_decided,
                     MPID_nem_ib_progress_engine_vt, sreq->retry_backoff);
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                mpi_errno = MPID_nem_ib_cm_cas_release_core(sreq->responder_rank, shadow);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_cas_release_core");
                break;
            case MPID_NEM_IB_CM_SYN:
                if (is_conn_established(sreq->responder_rank)) {
#if 1
                    /* Explicitly release CAS word because
                     * ConnectX-3 doesn't support safe CAS with PCI device and CPU */
                    MPID_nem_ib_cm_cas_release(MPID_nem_ib_conns[sreq->responder_rank].vc);
                    dprintf("cm_progress,syn,established is true,%d->%d,connection_tx=%d\n",
                            MPID_nem_ib_myrank, sreq->responder_rank,
                            sreq->ibcom->outstanding_connection_tx);
                    is_established = 1;
                    break;
#else
                    dprintf("cm_progress,syn,switching to cas_release,%d->%d\n",
                            MPID_nem_ib_myrank, sreq->responder_rank);
                    /* Connection was established while SYN command was enqueued.
                     * So we replace SYN with CAS_RELEASE, and send. */

                    /* override req->type */
                    ((MPID_nem_ib_cm_cmd_syn_t *) & sreq->cmd)->type = MPID_NEM_IB_CM_CAS_RELEASE2;
                    ((MPID_nem_ib_cm_cmd_syn_t *) & sreq->cmd)->initiator_rank = MPID_nem_ib_myrank;

                    /* Initiator does not receive SYNACK and ACK2, so we decrement incoming counter here. */
                    sreq->ibcom->incoming_connection_tx -= 2;

                    shadow =
                        (MPID_nem_ib_cm_cmd_shadow_t *)
                        MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));

                    /* override req->state */
                    shadow->type = sreq->state = MPID_NEM_IB_CM_CAS_RELEASE2;
                    shadow->req = sreq;
                    dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                    mpi_errno =
                        MPID_nem_ib_cm_cmd_core(sreq->responder_rank, shadow,
                                                (void *) (&sreq->cmd),
                                                sizeof(MPID_nem_ib_cm_cmd_synack_t), 1 /* syn:1 */ ,
                                                0);
                    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                        "**MPID_nem_ib_cm_send_core");
#endif
                    break;
                }

                /* The initiator acqurire slot for the responder when sending syn */
                if (MPID_nem_ib_diff16(MPID_nem_ib_cm_ringbuf_head,
                                       MPID_nem_ib_cm_ringbuf_tail) >= MPID_NEM_IB_CM_NSEG) {
                    goto next;
                }
                ((MPID_nem_ib_cm_cmd_syn_t *) & sreq->cmd)->responder_ringbuf_index =
                    MPID_nem_ib_cm_ringbuf_head;
                sreq->responder_ringbuf_index = MPID_nem_ib_cm_ringbuf_head;
                ((MPID_nem_ib_cm_cmd_syn_t *) & sreq->cmd)->initiator_rank = MPID_nem_ib_myrank;

                MPID_nem_ib_cm_ringbuf_head++;
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                mpi_errno =
                    MPID_nem_ib_cm_cmd_core(sreq->responder_rank, shadow,
                                            (void *) (&sreq->cmd),
                                            sizeof(MPID_nem_ib_cm_cmd_synack_t), 1 /* syn:1 */ , 0);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_send_core");
                break;
            case MPID_NEM_IB_CM_CAS_RELEASE2:
                dprintf("cm_progress,sending cas_release2,%d->%d\n", MPID_nem_ib_myrank,
                        sreq->responder_rank);

                ((MPID_nem_ib_cm_cmd_syn_t *) & sreq->cmd)->initiator_rank = MPID_nem_ib_myrank;

                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                mpi_errno =
                    MPID_nem_ib_cm_cmd_core(sreq->responder_rank, shadow,
                                            (void *) (&sreq->cmd),
                                            sizeof(MPID_nem_ib_cm_cmd_synack_t), 1 /* syn:1 */ , 0);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_send_core");
                break;
            case MPID_NEM_IB_CM_SYNACK:
                /* The responder acquire slot for the initiator when sending synack */
                if (MPID_nem_ib_diff16(MPID_nem_ib_cm_ringbuf_head,
                                       MPID_nem_ib_cm_ringbuf_tail) >= MPID_NEM_IB_CM_NSEG) {
                    goto next;
                }
                ((MPID_nem_ib_cm_cmd_synack_t *) & sreq->cmd)->initiator_ringbuf_index =
                    MPID_nem_ib_cm_ringbuf_head;
                sreq->initiator_ringbuf_index = MPID_nem_ib_cm_ringbuf_head;
                MPID_nem_ib_cm_ringbuf_head++;
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                mpi_errno =
                    MPID_nem_ib_cm_cmd_core(sreq->initiator_rank, shadow,
                                            (void *) (&sreq->cmd),
                                            sizeof(MPID_nem_ib_cm_cmd_synack_t), 0,
                                            sreq->ringbuf_index);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_send_core");
                break;
            case MPID_NEM_IB_CM_ACK1:
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                mpi_errno =
                    MPID_nem_ib_cm_cmd_core(sreq->responder_rank, shadow,
                                            (void *) (&sreq->cmd),
                                            sizeof(MPID_nem_ib_cm_cmd_ack1_t), 0,
                                            sreq->ringbuf_index);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_send_core");
                break;
            case MPID_NEM_IB_CM_ACK2:
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                mpi_errno =
                    MPID_nem_ib_cm_cmd_core(sreq->initiator_rank, shadow,
                                            (void *) (&sreq->cmd),
                                            sizeof(MPID_nem_ib_cm_cmd_ack2_t), 0,
                                            sreq->ringbuf_index);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_send_core");
                break;
            case MPID_NEM_IB_CM_ALREADY_ESTABLISHED:
            case MPID_NEM_IB_CM_RESPONDER_IS_CONNECTING:
                shadow =
                    (MPID_nem_ib_cm_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                dprintf("shadow=%p,shadow->req=%p\n", shadow, shadow->req);
                mpi_errno =
                    MPID_nem_ib_cm_cmd_core(sreq->initiator_rank, shadow,
                                            (void *) (&sreq->cmd),
                                            sizeof(MPID_nem_ib_cm_cmd_synack_t), 0,
                                            sreq->ringbuf_index);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_cm_send_core");
                break;
            default:
                dprintf("cm_progress,unknown state=%d\n", sreq->state);
                assert(0);
                MPIU_ERR_INTERNALANDJUMP(mpi_errno, "cm_progress,unknown state");
            }

            /* unlink sreq */
            if (prev_sreq != NULL) {
                MPID_nem_ib_cm_sendq_next(prev_sreq) = MPID_nem_ib_cm_sendq_next(sreq);
            }
            else {
                MPID_nem_ib_cm_sendq_head(MPID_nem_ib_cm_sendq) = MPID_nem_ib_cm_sendq_next(sreq);
            }
            if (MPID_nem_ib_cm_sendq_next(sreq) == NULL) {
                MPID_nem_ib_cm_sendq.tail = prev_sreq;
            }

            /* save sreq->dev.next (and sreq) because decrementing reference-counter might free sreq */
            MPID_nem_ib_cm_req_t *tmp_sreq = sreq;
            sreq = MPID_nem_ib_cm_sendq_next(sreq);

            if (is_established) {
                dprintf("cm_progress,destroy connect-op\n");

                /* don't connect */
                tmp_sreq->ibcom->outstanding_connection_tx -= 1;

                /* Let the guard down to let the following connection request go. */
                VC_FIELD(MPID_nem_ib_conns[tmp_sreq->responder_rank].vc, connection_guard) = 0;

                /* free memory : req->ref_count is 2, so call MPIU_Free() directly */
//                MPID_nem_ib_cm_request_release(tmp_sreq);
                MPIU_Free(tmp_sreq);

                is_established = 0;
                break;
            }
            goto next_unlinked;
          next:
            prev_sreq = sreq;
            sreq = MPID_nem_ib_cm_sendq_next(sreq);
          next_unlinked:;
        } while (sreq);
    }

  fn_exit:
    entered_send_progress = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_cas_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_cas_core(int rank, MPID_nem_ib_cm_cmd_shadow_t * shadow)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_CAS_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_CAS_CORE);

    MPID_nem_ib_com_t *conp;
    ibcom_errno = MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[rank], &conp);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_cas_scratch_pad");
    dprintf("cm_cas_core,%d->%d,conp=%p,remote_addr=%lx\n",
            MPID_nem_ib_myrank, rank, conp,
            (unsigned long) conp->icom_rmem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] + 0);

    /* Compare-and-swap rank to acquire communication manager port */
    ibcom_errno =
        MPID_nem_ib_com_cas_scratch_pad(MPID_nem_ib_scratch_pad_fds[rank],
                                        (uint64_t) shadow,
                                        0,
                                        MPID_NEM_IB_CM_RELEASED,
                                        MPID_nem_ib_myrank/*rank*/, /*debug*/
                                        &shadow->buf_from, &shadow->buf_from_sz);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_cas_scratch_pad");
    MPID_nem_ib_ncqe_scratch_pad += 1;

    /* Direct poll to drain CQ to check CAS result */
    MPID_nem_ib_ncqe_scratch_pad_to_drain += 1;
    dprintf("ringbuf_cm_cas_core,scratch_pad_to_drain=%d\n", MPID_nem_ib_ncqe_scratch_pad_to_drain);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_CAS_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_cas
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_cas(MPIDI_VC_t * vc, uint32_t ask_on_connect)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_CAS);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_CAS);

    dprintf("cm_cas,enter\n");

    /* Prepare request structure for enqueued case */
    MPID_nem_ib_cm_req_t *req = MPIU_Malloc(sizeof(MPID_nem_ib_cm_req_t));
    MPIU_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, "**malloc");
    dprintf("req=%p\n", req);
    req->state = MPID_NEM_IB_CM_CAS;
    req->ref_count = 3; /* Released on receiving ACK2 and draining SCQ of SYN and ACK1 */
    req->retry_backoff = 0;
    req->initiator_rank = MPID_nem_ib_myrank;
    req->responder_rank = vc->pg_rank;
    req->ask_on_connect = ask_on_connect;
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[vc->pg_rank], &req->ibcom);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");
    dprintf("req->ibcom=%p\n", req->ibcom);

    /* Increment transaction counter here because cm_cas is called only once
     * (cm_cas_core might be called more than once when retrying) */
    req->ibcom->outstanding_connection_tx += 1;
    dprintf("cm_cas,%d->%d,connection_tx=%d\n", MPID_nem_ib_myrank, vc->pg_rank,
            req->ibcom->outstanding_connection_tx);

    /* Acquire remote scratch pad */
    if (MPID_nem_ib_ncqe_scratch_pad < MPID_NEM_IB_COM_MAX_CQ_CAPACITY &&
        req->ibcom->ncom_scratch_pad < MPID_NEM_IB_COM_MAX_SQ_CAPACITY &&
        MPID_nem_ib_diff16(MPID_nem_ib_cm_ringbuf_head,
                           MPID_nem_ib_cm_ringbuf_tail) < MPID_NEM_IB_CM_NSEG) {
        MPID_nem_ib_cm_cmd_shadow_t *shadow =
            (MPID_nem_ib_cm_cmd_shadow_t *) MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
        shadow->type = req->state;
        shadow->req = req;

        mpi_errno = MPID_nem_ib_cm_cas_core(req->responder_rank, shadow);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_cas");
    }
    else {
        dprintf("cm_cas,enqueue\n");
        req->retry_decided = MPID_nem_ib_progress_engine_vt;
        MPID_nem_ib_cm_sendq_enqueue(&MPID_nem_ib_cm_sendq, req);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_CAS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_cas_release_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_cas_release_core(int rank, MPID_nem_ib_cm_cmd_shadow_t * shadow)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_CAS_RELEASE_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_CAS_RELEASE_CORE);

    MPID_nem_ib_com_t *conp;
    ibcom_errno = MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[rank], &conp);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_cas_scratch_pad");
    dprintf("cm_cas_release_core,%d->%d,conp=%p,remote_addr=%lx\n",
            MPID_nem_ib_myrank, rank, conp,
            (unsigned long) conp->icom_rmem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] + 0);

    /* Compare-and-swap rank to acquire communication manager port */
    ibcom_errno =
        MPID_nem_ib_com_cas_scratch_pad(MPID_nem_ib_scratch_pad_fds[rank],
                                        (uint64_t) shadow,
                                        0,
                                        MPID_nem_ib_myrank,
                                        MPID_NEM_IB_CM_RELEASED/*rank*/, /*debug*/
                                        &shadow->buf_from, &shadow->buf_from_sz);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_cas_scratch_pad");
    MPID_nem_ib_ncqe_scratch_pad += 1;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_CAS_RELEASE_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_cas_release
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_cas_release(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_CAS_RELEASE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_CAS_RELEASE);

    dprintf("cm_cas_release,enter\n");

    /* Prepare request structure for enqueued case */
    MPID_nem_ib_cm_req_t *req = MPIU_Malloc(sizeof(MPID_nem_ib_cm_req_t));
    MPIU_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, "**malloc");
    dprintf("req=%p\n", req);
    req->state = MPID_NEM_IB_CM_CAS_RELEASE;
    req->ref_count = 1; /* Released on draining SCQ */
    req->retry_backoff = 0;
    req->initiator_rank = MPID_nem_ib_myrank;
    req->responder_rank = vc->pg_rank;
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[vc->pg_rank], &req->ibcom);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");
    dprintf("req->ibcom=%p\n", req->ibcom);

    /* Increment transaction counter here because cm_cas_release is called only once
     * (cm_cas_release_core might be called more than once when retrying) */
    req->ibcom->outstanding_connection_tx += 1;
    dprintf("cm_cas_release,%d->%d,connection_tx=%d\n", MPID_nem_ib_myrank, vc->pg_rank,
            req->ibcom->outstanding_connection_tx);

    /* Acquire remote scratch pad */
    if (MPID_nem_ib_ncqe_scratch_pad < MPID_NEM_IB_COM_MAX_CQ_CAPACITY &&
        req->ibcom->ncom_scratch_pad < MPID_NEM_IB_COM_MAX_SQ_CAPACITY) {
        MPID_nem_ib_cm_cmd_shadow_t *shadow =
            (MPID_nem_ib_cm_cmd_shadow_t *) MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
        shadow->type = req->state;
        shadow->req = req;

        mpi_errno = MPID_nem_ib_cm_cas_release_core(req->responder_rank, shadow);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_cas_release");
    }
    else {
        dprintf("cm_cas_release,enqueue\n");
        req->retry_decided = MPID_nem_ib_progress_engine_vt;
        MPID_nem_ib_cm_sendq_enqueue(&MPID_nem_ib_cm_sendq, req);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_CAS_RELEASE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* We're trying to send SYN when syn is one */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_cmd_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_cmd_core(int rank, MPID_nem_ib_cm_cmd_shadow_t * shadow, void *buf,
                            MPIDI_msg_sz_t sz, uint32_t syn, uint16_t ringbuf_index)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    int ib_port = 1;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_CMD_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_CMD_CORE);

    dprintf("cm_cmd_core,enter,syn=%d\n", syn);

    shadow->req->ibcom = MPID_nem_ib_scratch_pad_ibcoms[rank];
    ibcom_errno =
        MPID_nem_ib_com_put_scratch_pad(MPID_nem_ib_scratch_pad_fds[rank],
                                        (uint64_t) shadow,
                                        syn ? MPID_NEM_IB_CM_OFF_SYN :
                                        MPID_NEM_IB_CM_OFF_CMD +
                                        sizeof(MPID_nem_ib_cm_cmd_t) *
                                        ((uint16_t) (ringbuf_index % MPID_NEM_IB_CM_NSEG)),
                                        sz, buf, &(shadow->buf_from), &(shadow->buf_from_sz));

    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_put_scratch_pad");
    MPID_nem_ib_ncqe_scratch_pad += 1;

    if (syn) {
        /* Skip QP createion on race condition */
        if (!(VC_FIELD(MPID_nem_ib_conns[rank].vc, connection_state) &
              MPID_NEM_IB_CM_LOCAL_QP_RESET)) {

            /* Prepare QP (RESET). Attempting to overlap it with preparing QP (RESET) on the responder side */
            ibcom_errno =
                MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_RC, &MPID_nem_ib_conns[rank].fd);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");

            VC_FIELD(MPID_nem_ib_conns[rank].vc, connection_state) |= MPID_NEM_IB_CM_LOCAL_QP_RESET;

            /* Store pointer to MPID_nem_ib_com */
            ibcom_errno = MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_conns[rank].fd,
                                                         &VC_FIELD(MPID_nem_ib_conns[rank].vc,
                                                                   ibcom));
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_obtain_pointer");

            /* Allocate RDMA-write-to ring-buf for remote */
            mpi_errno = MPID_nem_ib_ringbuf_alloc(MPID_nem_ib_conns[rank].vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_ring_alloc");
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_CMD_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_notify_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_notify_send(int pg_rank, int myrank)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPID_nem_ib_cm_cmd_shadow_t *shadow =
        (MPID_nem_ib_cm_cmd_shadow_t *) MPIU_Malloc(sizeof(MPID_nem_ib_cm_cmd_shadow_t));
    MPID_nem_ib_cm_notify_send_t *buf_from = (MPID_nem_ib_cm_notify_send_t *)
        MPID_nem_ib_rdmawr_from_alloc(sizeof(MPID_nem_ib_cm_notify_send_t));
    MPID_nem_ib_cm_req_t *req = MPIU_Malloc(sizeof(MPID_nem_ib_cm_req_t));

    shadow->type = MPID_NEM_IB_NOTIFY_OUTSTANDING_TX_EMPTY;

    buf_from->type = MPID_NEM_IB_NOTIFY_OUTSTANDING_TX_EMPTY;
    buf_from->initiator_rank = myrank;
    shadow->req = req;
    shadow->buf_from = (void *) buf_from;
    shadow->buf_from_sz = sizeof(MPID_nem_ib_cm_notify_send_t);

    shadow->req->ibcom = MPID_nem_ib_scratch_pad_ibcoms[pg_rank];

    ibcom_errno =
        MPID_nem_ib_com_wr_scratch_pad(MPID_nem_ib_scratch_pad_fds[pg_rank],
                                       (uint64_t) shadow, shadow->buf_from, shadow->buf_from_sz);

    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_wr_scratch_pad");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_notify_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_notify_progress(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    MPID_nem_ib_cm_notify_send_req_t *sreq, *prev_sreq;

    sreq = MPID_nem_ib_cm_notify_sendq_head(MPID_nem_ib_cm_notify_sendq);
    if (sreq) {
        prev_sreq = NULL;
        do {
            if (sreq->ibcom->outstanding_connection_tx != 0) {
                goto next;
            }

            ibcom_errno = MPID_nem_ib_cm_notify_send(sreq->pg_rank, sreq->my_rank);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_notify_send");

            /* unlink sreq */
            if (prev_sreq != NULL) {
                MPID_nem_ib_cm_notify_sendq_next(prev_sreq) = MPID_nem_ib_cm_notify_sendq_next(sreq);
            }
            else {
                MPID_nem_ib_cm_notify_sendq_head(MPID_nem_ib_cm_notify_sendq) =
                    MPID_nem_ib_cm_notify_sendq_next(sreq);
            }
            if (MPID_nem_ib_cm_notify_sendq_next(sreq) == NULL) {
                MPID_nem_ib_cm_notify_sendq.tail = prev_sreq;
            }

            MPID_nem_ib_cm_notify_send_req_t *tmp_sreq = sreq;
            sreq = MPID_nem_ib_cm_notify_sendq_next(sreq);

            MPIU_Free(tmp_sreq);

            goto next_unlinked;
          next:
            prev_sreq = sreq;
            sreq = MPID_nem_ib_cm_notify_sendq_next(sreq);
          next_unlinked:;
        } while (sreq);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPID_NEM_ONDEMAND */

/* RDMA-read the head pointer of the shared ring buffer */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_ringbuf_ask_fetch_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_ringbuf_ask_fetch_core(MPIDI_VC_t * vc, MPID_nem_ib_ringbuf_cmd_shadow_t * shadow,
                                       MPIDI_msg_sz_t sz)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_FETCH_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_FETCH_CORE);

    dprintf("ringbuf_ask_fetch_core,req=%p\n", shadow->req);

    ibcom_errno =
        MPID_nem_ib_com_get_scratch_pad(MPID_nem_ib_scratch_pad_fds[vc->pg_rank],
                                        (uint64_t) shadow,
                                        MPID_NEM_IB_RINGBUF_OFF_HEAD,
                                        sz, &shadow->buf_from, &shadow->buf_from_sz);

    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_get_scratch_pad");
    MPID_nem_ib_ncqe_scratch_pad += 1;

    /* Direct poll to drain CQ to issue CAS */
    MPID_nem_ib_ncqe_scratch_pad_to_drain += 1;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_FETCH_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_ringbuf_ask_fetch
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_ringbuf_ask_fetch(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_FETCH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_FETCH);

    dprintf("ringbuf_ask_fetch,enter\n");

    /* Prepare state of ask-send */
    MPID_nem_ib_ringbuf_req_t *req = MPIU_Malloc(sizeof(MPID_nem_ib_ringbuf_req_t));
    MPIU_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, "**malloc");
    dprintf("ask_fetch,req=%p\n", req);
    req->state = MPID_NEM_IB_RINGBUF_ASK_FETCH;
    req->retry_backoff = 0;
    req->vc = vc;
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[vc->pg_rank], &req->ibcom);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

    dprintf("ask_fetch,connection=%08x,ncqe=%d,ncom=%d,guard=%d\n",
            VC_FIELD(vc, connection_state),
            MPID_nem_ib_ncqe_scratch_pad,
            req->ibcom->ncom_scratch_pad, VC_FIELD(vc, ibcom->ask_guard)
);

    /* Acquire remote scratch pad */
    if (VC_FIELD(vc, connection_state) == MPID_NEM_IB_CM_ESTABLISHED &&
        MPID_nem_ib_ncqe_scratch_pad < MPID_NEM_IB_COM_MAX_CQ_CAPACITY &&
        req->ibcom->ncom_scratch_pad < MPID_NEM_IB_COM_MAX_SQ_CAPACITY &&
        !VC_FIELD(vc, ibcom->ask_guard)) {

        /* Let the guard up here to prevent CAS conflicts between consecutive asks
         * from the same process */
        VC_FIELD(vc, ibcom->ask_guard) = 1;

        MPID_nem_ib_ringbuf_cmd_shadow_t *shadow =
            (MPID_nem_ib_ringbuf_cmd_shadow_t *)
            MPIU_Malloc(sizeof(MPID_nem_ib_ringbuf_cmd_shadow_t));
        shadow->type = req->state;
        shadow->req = req;

        mpi_errno =
            MPID_nem_ib_ringbuf_ask_fetch_core(req->vc, shadow,
                                               sizeof(MPID_nem_ib_ringbuf_headtail_t));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_ringbuf_ask_fetch");
    }
    else {
        dprintf("ask_fetch,enqueue,req=%p\n", req);
        MPID_nem_ib_ringbuf_sendq_enqueue(&MPID_nem_ib_ringbuf_sendq, req);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_FETCH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_ringbuf_ask_cas_core
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_ringbuf_ask_cas_core(MPIDI_VC_t * vc, MPID_nem_ib_ringbuf_cmd_shadow_t * shadow,
                                     uint64_t head)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_CAS_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_CAS_CORE);

    dprintf("ringbuf_ask_cas_core,req=%p,head=%ld\n", shadow->req, head);

    /* Compare-and-swap to increment head pointer */
    ibcom_errno =
        MPID_nem_ib_com_cas_scratch_pad(MPID_nem_ib_scratch_pad_fds[vc->pg_rank],
                                        (uint64_t) shadow,
                                        MPID_NEM_IB_RINGBUF_OFF_HEAD,
                                        head, head + 1, &shadow->buf_from, &shadow->buf_from_sz);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_cas_scratch_pad");
    MPID_nem_ib_ncqe_scratch_pad += 1;

    /* Direct poll to drain CQ to check CAS result */
    MPID_nem_ib_ncqe_scratch_pad_to_drain += 1;
    dprintf("ringbuf_ask_cas_core,scratch_pad_to_drain=%d\n",
            MPID_nem_ib_ncqe_scratch_pad_to_drain);

    /* Let the guard down here to overlap CAS with a fetch of the following request
     * when CAS fails, out-of-order acquire may happen, but it's OK */
    VC_FIELD(vc, ibcom->ask_guard) = 0;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_CAS_CORE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_ringbuf_ask_cas
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_ringbuf_ask_cas(MPIDI_VC_t * vc, MPID_nem_ib_ringbuf_req_t * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_CAS);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_CAS);

    dprintf("ask_cas,ncqe=%d,ncom=%d,head=%ld,tail=%d,diff=%d,nslot=%d\n",
            MPID_nem_ib_ncqe_scratch_pad,
            req->ibcom->ncom_scratch_pad,
            req->fetched.head, req->fetched.tail,
            MPID_nem_ib_diff16(req->fetched.head, req->fetched.tail),
            VC_FIELD(vc, ibcom->local_ringbuf_nslot)
);

    /* Acquire one slot of the shared ring buffer */
    if (MPID_nem_ib_ncqe_scratch_pad < MPID_NEM_IB_COM_MAX_CQ_CAPACITY &&
        req->ibcom->ncom_scratch_pad < MPID_NEM_IB_COM_MAX_SQ_CAPACITY) {

        if (MPID_nem_ib_diff16(req->fetched.head, req->fetched.tail) <
            VC_FIELD(vc, ibcom->local_ringbuf_nslot)) {

            dprintf("ask_cas,core\n");
            req->state = MPID_NEM_IB_RINGBUF_ASK_CAS;
            MPID_nem_ib_ringbuf_cmd_shadow_t *shadow =
                (MPID_nem_ib_ringbuf_cmd_shadow_t *)
                MPIU_Malloc(sizeof(MPID_nem_ib_ringbuf_cmd_shadow_t));
            shadow->type = req->state;
            shadow->req = req;
            mpi_errno = MPID_nem_ib_ringbuf_ask_cas_core(vc, shadow, (uint64_t) req->fetched.head);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_ringbuf_ask_cas");
        }
        else {
            dprintf("ask_cas,ringbuf full,enqueue\n");
            /* Ring-buffer is full */

            /* Let the guard down so that this ask-fetch can be issued in ringbuf_progress */
#if 0   /*debug */
            VC_FIELD(vc, ibcom->ask_guard) = 0;
#endif
            /* Retry from fetch */

            /* Schedule retry */
            req->retry_decided = MPID_nem_ib_progress_engine_vt;
            req->retry_backoff = 0;

            /* Make the ask-fetch in order */
            MPID_nem_ib_ringbuf_sendq_enqueue_at_head(&MPID_nem_ib_ringbuf_sendq, req);
        }
    }
    else {
        dprintf("ask_cas,ncqe or ncom full,enqueue\n");
        req->retry_decided = MPID_nem_ib_progress_engine_vt;
        req->retry_backoff = 0;
        MPID_nem_ib_ringbuf_sendq_enqueue(&MPID_nem_ib_ringbuf_sendq, req);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_RINGBUF_ASK_CAS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_ringbuf_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_ringbuf_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_ringbuf_req_t *sreq, *prev_sreq;
    MPID_nem_ib_ringbuf_cmd_shadow_t *shadow;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_RINGBUF_PROGRESS);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_RINGBUF_PROGRESS);

    //dprintf("rinbguf_send_progress,enter\n");

    sreq = MPID_nem_ib_ringbuf_sendq_head(MPID_nem_ib_ringbuf_sendq);
    if (sreq) {
        prev_sreq = NULL;
        do {
            if (VC_FIELD(sreq->vc, connection_state) != MPID_NEM_IB_CM_ESTABLISHED ||
                sreq->ibcom->ncom >= MPID_NEM_IB_COM_MAX_SQ_CAPACITY ||
                MPID_nem_ib_ncqe_scratch_pad >= MPID_NEM_IB_COM_MAX_CQ_CAPACITY) {
                goto next;
            }

            switch (sreq->state) {
            case MPID_NEM_IB_RINGBUF_ASK_CAS:
                dprintf("ringbuf_progress,ask_cas,req=%p\n", sreq);
                shadow =
                    (MPID_nem_ib_ringbuf_cmd_shadow_t *)
                    MPIU_Malloc(sizeof(MPID_nem_ib_ringbuf_cmd_shadow_t));
                shadow->type = sreq->state;
                shadow->req = sreq;
                mpi_errno =
                    MPID_nem_ib_ringbuf_ask_cas_core(sreq->vc, shadow,
                                                     (uint64_t) sreq->fetched.head);
                MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                    "**MPID_nem_ib_ringbuf_connect_cas_core");
                break;
            case MPID_NEM_IB_RINGBUF_ASK_FETCH:
                if (MPID_nem_ib_diff63(MPID_nem_ib_progress_engine_vt, sreq->retry_decided) <
                    sreq->retry_backoff) {
                    dprintf("ringbuf_progress,vt=%ld,retry_decided=%ld,diff=%ld,backoff=%ld\n",
                            MPID_nem_ib_progress_engine_vt, sreq->retry_decided,
                            MPID_nem_ib_diff63(MPID_nem_ib_progress_engine_vt, sreq->retry_decided),
                            sreq->retry_backoff);
                    goto next;
                }
                //dprintf("ringbuf_progress,ask_fetch,decided=%ld,vt=%ld,backoff=%ld\n",
                //sreq->retry_decided, MPID_nem_ib_progress_engine_vt, sreq->retry_backoff);

                /* Enqueued speculatively, so discard if not needed. */
                if (VC_FIELD(sreq->vc, ibcom->local_ringbuf_type) == MPID_NEM_IB_RINGBUF_SHARED) {
                    if (VC_FIELD(sreq->vc, ibcom->ask_guard)) {
                        goto next;
                    }
                    dprintf("ringbuf_progress,ask_fetch,req=%p\n", sreq);
                    VC_FIELD(sreq->vc, ibcom->ask_guard) = 1;
                    shadow =
                        (MPID_nem_ib_ringbuf_cmd_shadow_t *)
                        MPIU_Malloc(sizeof(MPID_nem_ib_ringbuf_cmd_shadow_t));
                    shadow->type = sreq->state;
                    shadow->req = sreq;
                    mpi_errno =
                        MPID_nem_ib_ringbuf_ask_fetch_core(sreq->vc, shadow,
                                                           sizeof(MPID_nem_ib_ringbuf_headtail_t));
                    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                        "**MPID_nem_ib_ringbuf_send_core");
                }
                break;
            default:
                dprintf("ringbuf_progress,unknown state=%d\n", sreq->state);
                assert(0);
                MPIU_ERR_INTERNALANDJUMP(mpi_errno, "ringbuf_progress,unknown state");
            }

            /* unlink sreq */
            if (prev_sreq != NULL) {
                MPID_nem_ib_ringbuf_sendq_next(prev_sreq) = MPID_nem_ib_ringbuf_sendq_next(sreq);
            }
            else {
                MPID_nem_ib_ringbuf_sendq_head(MPID_nem_ib_ringbuf_sendq) =
                    MPID_nem_ib_ringbuf_sendq_next(sreq);
            }
            if (MPID_nem_ib_ringbuf_sendq_next(sreq) == NULL) {
                MPID_nem_ib_ringbuf_sendq.tail = prev_sreq;
            }

            /* save sreq->dev.next (and sreq) because decrementing reference-counter might free sreq */
            //MPID_nem_ib_ringbuf_req_t *tmp_sreq = sreq;
            sreq = MPID_nem_ib_ringbuf_sendq_next(sreq);

            goto next_unlinked;
          next:
            prev_sreq = sreq;
            sreq = MPID_nem_ib_ringbuf_sendq_next(sreq);
          next_unlinked:;
        } while (sreq);
    }

  fn_exit:
    entered_send_progress = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_RINGBUF_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
