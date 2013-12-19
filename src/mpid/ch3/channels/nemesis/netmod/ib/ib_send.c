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
    int llc_errno = LLC_SUCCESS;
    char *pTree = map->data;

    dprintf("MPID_nem_ib_cm_map_get,key=%s\n", key);

    if (!pTree) {
        llc_errno = -1;
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
                llc_errno = -1;
                dprintf("left is null\n");
                goto fn_fail;
            }
            pTree = map->data + MPID_NEM_IB_MAP_LPTR(pTree);    // go to left child
        }
        else {
            //  psArg is "larger" OR same substring, psArg is longer
            if (MPID_NEM_IB_MAP_RPTR(pTree) == 0) {
                llc_errno = -1;
                dprintf("right is null\n");
                goto fn_fail;
            }
            pTree = map->data + MPID_NEM_IB_MAP_RPTR(pTree);    // go to right child
        }
    }
  fn_exit:
    return llc_errno;
  fn_fail:
    goto fn_exit;
}
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
    void *netmod_hdr;
    int sz_netmod_hdr;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_ISENDCONTIG_CORE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_ISENDCONTIG_CORE);

    /* piggy-back SR occupancy info might copy and modify given header */

    /* remote SR sequence number which is last sent */
    int *rsr_seq_num_tail;
    ibcom_errno = MPID_nem_ib_com_rsr_seq_num_tail_get(vc_ib->sc->fd, &rsr_seq_num_tail);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_rsr_seq_num_tail_get");

    /* remote SR sequence number which is last sent */
    int *rsr_seq_num_tail_last_sent;
    ibcom_errno =
        MPID_nem_ib_com_rsr_seq_num_tail_last_sent_get(vc_ib->sc->fd, &rsr_seq_num_tail_last_sent);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_rsr_seq_num_tail_last_sent_get");

    //dprintf("isendcontig,rsr_seq_num_tail=%d,rsr_seq_num_tail_last_sent=%d\n", *rsr_seq_num_tail, *rsr_seq_num_tail_last_sent);

    int notify_rate;
    ibcom_errno = MPID_nem_ib_com_rdmabuf_occupancy_notify_rate_get(vc_ib->sc->fd, &notify_rate);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_sq_occupancy_notify_rate_get");

    /* send RDMA-write-to buffer occupancy information */
    /* embed SR occupancy information and remember the last one sent */
    MPIDI_CH3_Pkt_t *ch3_hdr = (MPIDI_CH3_Pkt_t *) hdr;
    if (MPID_nem_ib_diff32(*rsr_seq_num_tail, *rsr_seq_num_tail_last_sent) > notify_rate) {
#if 1   /* debug, disabling piggy-back */
        switch (ch3_hdr->type) {
        case MPIDI_CH3_PKT_EAGER_SEND:
            pkt_netmod.subtype = MPIDI_NEM_IB_PKT_EAGER_SEND;
            goto common_tail;
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
            pkt_netmod.seq_num_tail = *rsr_seq_num_tail;
            *rsr_seq_num_tail_last_sent = *rsr_seq_num_tail;
            netmod_hdr = (void *) &pkt_netmod;
            sz_netmod_hdr = sizeof(MPID_nem_ib_pkt_prefix_t);
            break;
        default:
            netmod_hdr = NULL;
            sz_netmod_hdr = 0;
            break;
        }
#else
        netmod_hdr = NULL;
        sz_netmod_hdr = 0;
#endif
    }
    else {
        netmod_hdr = NULL;
        sz_netmod_hdr = 0;
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
        MPIR_Request_add_ref(sreq);
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

    int msg_type = MPIDI_Request_get_msg_type(sreq);

    dprintf
        ("isendcontig_core,netmod_hdr=%p,sz_netmod_hdr=%d,hdr=%p,sz_hdr=%ld,data=%p,sz_data=%d\n",
         netmod_hdr, sz_netmod_hdr, hdr, hdr_sz, data, (int) data_sz);

    if (sizeof(MPIDI_CH3_Pkt_t) != hdr_sz) {
        printf("type=%d,subtype=%d\n", ((MPID_nem_pkt_netmod_t *) hdr)->type,
               ((MPID_nem_pkt_netmod_t *) hdr)->subtype);
    }

    int copied;
    ibcom_errno =
        MPID_nem_ib_com_isend(vc_ib->sc->fd, (uint64_t) sreq, netmod_hdr, sz_netmod_hdr, hdr,
                              hdr_sz, data, (int) data_sz, &copied);
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
        MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num,
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
    int ibcom_errno;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_ISENDCONTIG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_ISENDCONTIG);

    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ib_iSendContig");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) hdr);

#ifdef MPID_NEM_IB_ONDEMAND
    if (!vc_ib->is_connected) {
        MPID_nem_ib_send_syn(vc);
    }
#endif

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

    int *lsr_seq_num_tail;
    /* sequence number of (largest) completed send command */
    ibcom_errno = MPID_nem_ib_com_lsr_seq_num_tail_get(vc_ib->sc->fd, &lsr_seq_num_tail);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_lsr_seq_num_tail_get");

    int lsr_seq_num_head;
    /* sequence number of (largest) in-flight send command */
    ibcom_errno = MPID_nem_ib_com_sseq_num_get(vc_ib->sc->fd, &lsr_seq_num_head);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_sseq_num_get");

    dprintf("isendcontig,%d->%d,type=%d,subtype=%d,data_sz=%ld,ldiff=%d(%d-%d),rdiff=%d(%d-%d)\n",
            MPID_nem_ib_myrank, vc->pg_rank, ((MPIDI_CH3_Pkt_t *) hdr)->type,
            ((MPID_nem_pkt_netmod_t *) hdr)->subtype, data_sz,
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail,
            MPID_nem_ib_diff32(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent);
    dprintf("isendcontig,sendq_empty=%d,ncom=%d,ncqe=%d,rdmabuf_occ=%d\n",
            MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom, MPID_nem_ib_ncqe,
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));
    /* if IB command overflow-queue is empty AND local IB command queue isn't full AND remote RDMA-write-to buf isn't getting overrun */
    MPIDI_CH3_Pkt_t *ch3_hdr = (MPIDI_CH3_Pkt_t *) hdr;
    MPID_nem_pkt_netmod_t *netmod_hdr = (MPID_nem_pkt_netmod_t *) hdr;
    /* reserve one slot for control packet bringing sequence number
     * to avoid dead-lock */
    int slack = ((ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                  netmod_hdr->subtype != MPIDI_NEM_IB_PKT_REQ_SEQ_NUM) &&
                 (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                  netmod_hdr->subtype != MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) &&
                 (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                  netmod_hdr->subtype != MPIDI_NEM_IB_PKT_LMT_GET_DONE) &&
                 ch3_hdr->type != MPIDI_NEM_PKT_LMT_RTS &&
                 ch3_hdr->type != MPIDI_NEM_PKT_LMT_CTS) ? MPID_NEM_IB_COM_AMT_SLACK : 0;
    /* make control packet bringing sequence number go ahead of
     * queued packets to avoid dead-lock */
    int goahead =
        (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD &&
         netmod_hdr->subtype == MPIDI_NEM_IB_PKT_REQ_SEQ_NUM)
        || (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD &&
            netmod_hdr->subtype == MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) ||
        (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD &&
         netmod_hdr->subtype == MPIDI_NEM_IB_PKT_LMT_GET_DONE)
        ? 1 : 0;
    dprintf("isendcontig,slack=%d,goahead=%d\n", slack, goahead);

    if (
#ifdef MPID_NEM_IB_ONDEMAND
           vc_ib->is_connected &&
#endif
           (goahead || MPID_nem_ib_sendq_empty(vc_ib->sendq)) &&
           vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack &&
           MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack &&
           MPID_nem_ib_diff32(lsr_seq_num_head,
                              *lsr_seq_num_tail) < MPID_NEM_IB_COM_RDMABUF_NSEG - slack) {

        mpi_errno = MPID_nem_ib_iSendContig_core(vc, sreq, hdr, hdr_sz, data, data_sz);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    }
    else {

        /* enqueue command into send_queue */
        dprintf("isendcontig,enqueuing,sendq=%d,ncom=%d,ncqe=%d,ldiff=%d(%d-%d),slack=%d\n",
                (goahead || MPID_nem_ib_sendq_empty(vc_ib->sendq)),
                vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack,
                MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack,
                MPID_nem_ib_diff32(lsr_seq_num_head, *lsr_seq_num_tail),
                vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail, slack);

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
    int ibcom_errno;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    int sseq_num;
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
    ibcom_errno = MPID_nem_ib_com_sseq_num_get(vc_ib->sc->fd, &sseq_num);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_sseq_num_get");

    if (hdr) {

        MPIDI_CH3_Pkt_t *pkt = (MPIDI_CH3_Pkt_t *) hdr;
        MPIDI_CH3_Pkt_close_t *close_pkt = &pkt->close;
        dprintf("isend(istartcontig),%d->%d,seq_num=%d,type=%d,ack=%d\n", MPID_nem_ib_myrank,
                vc->pg_rank, sseq_num, close_pkt->type, close_pkt->ack);
    }
    else {
        dprintf("isend(istartcontig),%d->%d,seq_num=%d\n", MPID_nem_ib_myrank, vc->pg_rank,
                sseq_num);
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
    int sseq_num;

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

    /* increment cc because PktHandler_EagerSyncAck, ssend.c, drain_scq decrement it */
    if (((MPIDI_CH3_Pkt_t *) hdr)->type == MPIDI_CH3_PKT_EAGER_SYNC_SEND) {
        MPIR_Request_add_ref(sreq);
    }

    ibcom_errno = MPID_nem_ib_com_sseq_num_get(vc_ib->sc->fd, &sseq_num);
    MPIU_ERR_CHKANDJUMP(ibcom_errno != 0, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_com_sseq_num_get");

    int copied;
    dprintf("sendnoncontig_core,isend,%d->%d,seq_num=%d\n", MPID_nem_ib_myrank, vc->pg_rank,
            sseq_num);
    ibcom_errno =
        MPID_nem_ib_com_isend(vc_ib->sc->fd, (uint64_t) sreq, NULL, 0, hdr, sizeof(MPIDI_CH3_Pkt_t),
                              (void *) REQ_FIELD(sreq, lmt_pack_buf), (int) last, &copied);
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
        MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num,
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
    int ibcom_errno;
    MPIDI_msg_sz_t last;
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    int sseq_num;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_SENDNONCONTIG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ib_SendNoncontig");

    dprintf("sendnoncontig,%d->%d,sendq_empty=%d,ncom=%d,rdmabuf_occ=%d\n", MPID_nem_ib_myrank,
            vc->pg_rank, MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom,
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));
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
    if (MPID_nem_ib_sendq_empty(vc_ib->sendq) &&
        vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack &&
        MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack &&
        MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num,
                           vc_ib->ibcom->lsr_seq_num_tail) < MPID_NEM_IB_COM_RDMABUF_NSEG - slack) {

        mpi_errno = MPID_nem_ib_SendNoncontig_core(vc, sreq, hdr, hdr_sz);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

    }
    else {
        /* enqueue command into send_queue */
        dprintf("sendnoncontig, enqueuing");

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
int MPID_nem_ib_send_progress(MPID_nem_ib_vc_area * vc_ib)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    MPID_IOV *iov;
    int n_iov;
    MPID_Request *sreq, *prev_sreq;
    int again = 0;
    int msg_type;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_SEND_PROGRESS);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_SEND_PROGRESS);

    //dprintf("send_progress,enter\n");

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
            msg_type = MPIDI_Request_get_msg_type(sreq);

            MPIDI_CH3_Pkt_t *ch3_hdr = (MPIDI_CH3_Pkt_t *) sreq->dev.iov[0].MPID_IOV_BUF;
            MPID_nem_pkt_netmod_t *netmod_hdr =
                (MPID_nem_pkt_netmod_t *) sreq->dev.iov[0].MPID_IOV_BUF;
            int slack = (msg_type == MPIDI_REQUEST_EAGER_MSG) ? /* guard from RDMA-read or RDMA-write */
                (((ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                   netmod_hdr->subtype != MPIDI_NEM_IB_PKT_REQ_SEQ_NUM) &&
                  (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                   netmod_hdr->subtype != MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM) &&
                  (ch3_hdr->type != MPIDI_NEM_PKT_NETMOD ||
                   netmod_hdr->subtype != MPIDI_NEM_IB_PKT_LMT_GET_DONE) &&
                  ch3_hdr->type != MPIDI_NEM_PKT_LMT_RTS &&
                  ch3_hdr->type !=
                  MPIDI_NEM_PKT_LMT_CTS) ? MPID_NEM_IB_COM_AMT_SLACK : 0) :
                MPID_NEM_IB_COM_AMT_SLACK;
            if (vc_ib->ibcom->ncom >= MPID_NEM_IB_COM_MAX_SQ_CAPACITY - slack ||
                MPID_nem_ib_ncqe >= MPID_NEM_IB_COM_MAX_CQ_CAPACITY - slack ||
                MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num,
                                   vc_ib->ibcom->lsr_seq_num_tail) >=
                MPID_NEM_IB_COM_RDMABUF_NSEG - slack) {
                break;
            }


            if (vc_ib != MPID_nem_ib_debug_current_vc_ib) {
                dprintf("send_progress,vc_ib != MPID_nem_ib_debug_current_vc_ib\n");
            }
            dprintf("send_progress,kind=%d,msg_type=%d\n", sreq->kind, msg_type);
            if (msg_type == MPIDI_REQUEST_EAGER_MSG) {
                dprintf("send_progress,type=%d\n", ch3_hdr->type);
            }
            dprintf("send_progress,%d->%d,rdiff=%d(%d-%d),ldiff=%d(%d-%d),slack=%d\n",
                    MPID_nem_ib_myrank, sreq->ch.vc->pg_rank,
                    MPID_nem_ib_diff32(vc_ib->ibcom->rsr_seq_num_tail,
                                       vc_ib->ibcom->rsr_seq_num_tail_last_sent),
                    vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent,
                    MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num,
                                       vc_ib->ibcom->lsr_seq_num_tail),
                    vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail, slack);
            if (sreq->kind == MPID_REQUEST_SEND && msg_type == MPIDI_REQUEST_EAGER_MSG) {
                if (!sreq->ch.noncontig) {
                    dprintf
                        ("send_progress,contig,type=%d,sseq_num=%d,MPIDI_NEM_PKT_LMT_RTS=%d,MPIDI_NEM_IB_PKT_LMT_GET_DONE=%d\n",
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
                            MPID_nem_ib_lmt_cookie_t *s_cookie_buf =
                                (MPID_nem_ib_lmt_cookie_t *) sreq->dev.iov[1].MPID_IOV_BUF;
                            dprintf("send_progress,MPIDI_NEM_PKT_LMT_RTS,rsr_seq_num_tail=%d\n",
                                    vc_ib->ibcom->rsr_seq_num_tail);
                            /* embed RDMA-write-to buffer occupancy information */
                            s_cookie_buf->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                            /* remember the last one sent */
                            vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                vc_ib->ibcom->rsr_seq_num_tail;
                            break;
                        }

                    case MPIDI_NEM_PKT_LMT_CTS:{
                            MPID_nem_ib_lmt_cookie_t *s_cookie_buf =
                                (MPID_nem_ib_lmt_cookie_t *) sreq->dev.iov[1].MPID_IOV_BUF;
                            dprintf("send_progress,MPIDI_NEM_PKT_LMT_CTS,rsr_seq_num_tail=%d\n",
                                    vc_ib->ibcom->rsr_seq_num_tail);
                            /* embed RDMA-write-to buffer occupancy information */
                            s_cookie_buf->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                            /* remember the last one sent */
                            vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                vc_ib->ibcom->rsr_seq_num_tail;
                            break;
                        }

                    default:;
                    }

                    if (ch3_hdr->type == MPIDI_NEM_PKT_NETMOD) {
                        switch (netmod_hdr->subtype) {
                            /* send current rsr_seq_num_tail because message from target to initiator
                             * might have happened while being queued */
                        case MPIDI_NEM_IB_PKT_LMT_GET_DONE:{
                                MPID_nem_ib_pkt_lmt_get_done_t *_done_pkt =
                                    (MPID_nem_ib_pkt_lmt_get_done_t *) sreq->dev.
                                    iov[0].MPID_IOV_BUF;
                                dprintf
                                    ("send_progress,MPIDI_NEM_IB_PKT_LMT_GET_DONE,rsr_seq_num_tail=%d\n",
                                     vc_ib->ibcom->rsr_seq_num_tail);
                                /* embed SR occupancy information */
                                _done_pkt->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail;
                                /* remember the last one sent */
                                vc_ib->ibcom->rsr_seq_num_tail_last_sent =
                                    vc_ib->ibcom->rsr_seq_num_tail;
                                break;
                            }
                        case MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM:{
                                MPID_nem_ib_pkt_reply_seq_num_t *_pkt =
                                    (MPID_nem_ib_pkt_reply_seq_num_t *) sreq->dev.
                                    iov[0].MPID_IOV_BUF;
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
                                                                                         lmt_write_to_buf));
                if (mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }
            }
            else if (sreq->kind == MPID_REQUEST_SEND && msg_type == MPIDI_REQUEST_RNDV_MSG) {
            }
            else {
                dprintf("send_progress,unknown sreq->type=%d,msg_type=%d\n", sreq->kind, msg_type);
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
            MPID_Request *tmp_sreq = sreq;
            sreq = MPID_nem_ib_sendq_next(sreq);
            goto next_unlinked;
            //next:
            prev_sreq = sreq;
            sreq = MPID_nem_ib_sendq_next(sreq);
          next_unlinked:;
        } while (sreq);
    }

    //dprintf("send_progress,exit,sendq_empty=%d,ncom=%d,ncqe=%d,rdmabuf_occ=%d\n", MPID_nem_ib_sendq_empty(vc_ib->sendq), vc_ib->ibcom->ncom, MPID_nem_ib_ncqe, MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail));

  fn_exit:
    entered_send_progress = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_SEND_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifdef MPID_NEM_IB_ONDEMAND
int MPID_nem_ib_cm_send_core(int rank, MPID_nem_ib_cm_cmd_t * cmd)
{
    MPID_nem_ib_com *ibcom_scratch_pad;
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[rank], &ibcom_scratch_pad);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

    if (MPID_nem_ib_ncqe_scratch_pad >= MPID_NEM_IB_COM_MAX_SQ_CAPACITY ||
        ibcom_scratch_pad->ncom_scratch_pad >= MPID_NEM_IB_COM_MAX_CQ_CAPACITY) {
        mpi_errno = MPID_nem_ib_drain_scq_scratch_pad();
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                            "**MPID_nem_ib_drain_scq_scratch_pad");
    }

    ibcom_errno =
        MPID_nem_ib_com_put_scratch_pad(MPID_nem_ib_scratch_pad_fds[rank],
                                        (uint64_t) ibcom_scratch_pad, sizeof(uint32_t),
                                        sizeof(MPID_nem_ib_cm_cmd_t), (void *) cmd);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_put_scratch_pad");
    MPID_nem_ib_ncqe_scratch_pad += 1;

    /* atomic write to doorbell */
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_cm_connect(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int val;
    MPID_nem_ib_cm_cmd_t cmd;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CM_CONNECT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CM_CONNECT);

    dprintf("connect,enter\n");

    cmd.type = MPID_NEM_IB_CM_SYN;
    mpi_errno = MPID_nem_ib_cm_send_core(rank, &cmd);
    MPIU_ERR_CHKANDJUMP(mp_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_put");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CM_CONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif
