/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IB_IMPL_H_INCLUDED
#define IB_IMPL_H_INCLUDED

#include "mpid_nem_impl.h"
#include "ib_ibcom.h"
#include <sys/types.h>
#include <errno.h>
#include <linux/mman.h> /* make it define MAP_ANONYMOUS */
#include <sys/mman.h>

#define MPID_NEM_IB_LMT_GET_CQE /* detect RDMA completion by CQE */
#define MPID_NEM_IB_DISABLE_VAR_OCC_NOTIFY_RATE
/* lmt-put:
   (1) receiver sends cts to sender (2) sender RDMA-write to receiver
   (3) sender fetch CQE (4) receiver polls on end-flag
*/
//#define MPID_NEM_IB_ONDEMAND

typedef struct {
    union ibv_gid gid;
    uint16_t lid;
    uint32_t qpn;
} MPID_nem_ib_conn_ud_t;

typedef struct {
    int fd, fd_lmt_put;
    MPIDI_VC_t *vc;
} MPID_nem_ib_conn_t;

/* see src/mpid/ch3/channels/nemesis/include/mpid_nem_generic_queue.h */
typedef GENERIC_Q_DECL(struct MPID_Request) MPID_nem_ib_sendq_t;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct {
    MPID_nem_ib_conn_t *sc;
    int pending_sends;          /* number of send in flight */
    MPID_nem_ib_com_t *ibcom, *ibcom_lmt_put;
    MPID_nem_ib_sendq_t sendq;  /* overflow queue for IB commands */
    MPID_nem_ib_sendq_t sendq_lmt_put;
    int is_connected;           /* dynamic connection, checked in iSendContig, protocol processed there and in progress engine */
} MPID_nem_ib_vc_area;

/* macro for secret area in vc */
#define VC_CH(vc) ((MPIDI_CH3I_VC *)&(vc)->ch)
#define VC_IB(vc) ((MPID_nem_ib_vc_area *)VC_CH((vc))->netmod_area.padding)
#define VC_FIELD(vcp, field) (((MPID_nem_ib_vc_area *)VC_CH(((vcp)))->netmod_area.padding)->field)

/* The req provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the req structure
   on the network module, facilitating dynamic module loading. */
typedef struct {
    int seq_num;                /* NOT USED, DELETE IT: sequence number of SR which RDMA-RD for lmt releases in ib_poll */
    struct MPID_Request *lmt_next;      /* for lmtq */
    struct MPID_Request *sendq_next;    /* for sendq */
    void *lmt_raddr;            /* remember this for sendq, it might be better to use sreq->dev.iov[0].MPID_IOV_BUF instead */
    uint32_t lmt_rkey;          /* remember this for sendq, survive over lrecv and referenced when dequeueing from sendq */
    uint32_t lmt_szsend;        /* remember this for sendq */
    uint8_t lmt_tail, lmt_sender_tail, lmt_receiver_tail;       /* survive over lrecv and referenced when polling */
    MPI_Aint lmt_dt_true_lb;    /* to locate the last byte of receive buffer */
    void *lmt_write_to_buf;     /* user buffer or temporary buffer for pack and remember it for lmt_orderq */
    void *lmt_pack_buf;         /* to pack non-contiguous data */
} MPID_nem_ib_req_area;

/* macro for secret area in req */
#define REQ_IB(req) ((MPID_nem_ib_req_area *)(&(req)->ch.netmod_area.padding))
#define REQ_FIELD(reqp, field) (((MPID_nem_ib_req_area *)((reqp)->ch.netmod_area.padding))->field)

/* see src/mpid/ch3/channels/nemesis/include/mpidi_ch3_impl.h */
/* sreq is never enqueued into posted-queue nor unexpected-queue, so we can reuse sreq->dev.next */
#define MPID_nem_ib_sendq_empty(q) GENERICM_Q_EMPTY (q)
#define MPID_nem_ib_sendq_head(q) GENERICM_Q_HEAD (q)
#define MPID_nem_ib_sendq_next_field(ep, next_field) REQ_FIELD(ep, next_field)
#define MPID_nem_ib_sendq_next(ep) REQ_FIELD(ep, sendq_next)
//#define MPID_nem_ib_sendq_next(ep) (ep->dev.next) /*takagi*/
#define MPID_nem_ib_sendq_enqueue(qp, ep) GENERICM_Q_ENQUEUE (qp, ep, MPID_nem_ib_sendq_next_field, sendq_next);
#define MPID_nem_ib_sendq_enqueue_at_head(qp, ep) GENERICM_Q_ENQUEUE_AT_HEAD(qp, ep, MPID_nem_ib_sendq_next_field, sendq_next);
#define MPID_nem_ib_sendq_dequeue(qp, ep) GENERICM_Q_DEQUEUE (qp, ep, MPID_nem_ib_sendq_next_field, sendq_next);

/* see src/mpid/ch3/channels/nemesis/include/mpid_nem_generic_queue.h */
typedef GENERIC_Q_DECL(struct MPID_Request) MPID_nem_ib_lmtq_t;

/* connection manager */
typedef struct {
    int remote_rank;
    uint32_t type;              /* SYN */
    uint32_t qpn;               /* QPN for eager-send channel */
    uint32_t rkey;              /* key for RDMA-write-to buffer of eager-send channel */
    void *rmem;                 /* address of RDMA-write-to buffer of eager-send channel */
} MPID_nem_ib_cm_pkt_syn_t;

typedef struct {
    int remote_rank;
    uint32_t type;              /* SYNACK */
    uint32_t qpn;               /* QPN for eager-send channel */
    uint32_t rkey;              /* key for RDMA-write-to buffer of eager-send channel */
    void *rmem;                 /* address of RDMA-write-to buffer of eager-send channel */
} MPID_nem_ib_cm_pkt_synack_t;

typedef union {
    MPID_nem_ib_cm_pkt_syn_t syn;
    MPID_nem_ib_cm_pkt_synack_t synack;
} MPID_nem_ib_cm_pkt_t;

typedef struct MPID_nem_ib_cm_sendq_entry {
    MPID_nem_ib_cm_pkt_t pending_pkt;
    struct MPID_nem_ib_cm_sendq_entry *sendq_next;      /* for software command queue */
} MPID_nem_ib_cm_sendq_entry_t;

#ifdef MPID_NEM_IB_ONDEMAND
typedef struct {
    char *data;
    int length;
    int max_length;
} MPID_nem_ib_cm_map_t;

typedef struct {
    uint32_t type;
    uint32_t qpnum;
    uint16_t lid;
    union ibv_gid gid;
    void *rmem;
    uint32_t rkey;
} MPID_nem_ib_cm_cmd_t;
#endif

typedef GENERIC_Q_DECL(struct MPID_Request) MPID_nem_ib_cm_sendq_t;

#define MPID_nem_ib_cm_sendq_empty(q) GENERICM_Q_EMPTY (q)
#define MPID_nem_ib_cm_sendq_head(q) GENERICM_Q_HEAD (q)
#define MPID_nem_ib_cm_sendq_next_field(ep, next_field) ((ep)->next_field)
#define MPID_nem_ib_cm_sendq_next(ep) ((ep)->sendq_next)
#define MPID_nem_ib_cm_sendq_enqueue(qp, ep) GENERICM_Q_ENQUEUE (qp, ep, MPID_nem_ib_cm_sendq_next_field, sendq_next);
#define MPID_nem_ib_cm_sendq_enqueue_at_head(qp, ep) GENERICM_Q_ENQUEUE_AT_HEAD(qp, ep, MPID_nem_ib_cm_sendq_next_field, sendq_next);
#define MPID_nem_ib_cm_sendq_dequeue(qp, ep) GENERICM_Q_DEQUEUE (qp, ep, MPID_nem_ib_cm_sendq_next_field, sendq_next);

/* see src/mpid/ch3/channels/nemesis/include/mpidi_ch3_impl.h */
/* TODO: rreq for rendezvous is dequeued from posted-queue nor unexpected-queue when do_cts is called,
   so we can reuse rreq->dev.next */
#define MPID_nem_ib_lmtq_empty(q) GENERICM_Q_EMPTY(q)
#define MPID_nem_ib_lmtq_head(q) GENERICM_Q_HEAD(q)
#define MPID_nem_ib_lmtq_next_field(ep, next_field) REQ_FIELD(ep, next_field)
#define MPID_nem_ib_lmtq_next(ep) REQ_FIELD(ep, lmt_next)
#define MPID_nem_ib_lmtq_enqueue(qp, ep) GENERICM_Q_ENQUEUE(qp, ep, MPID_nem_ib_lmtq_next_field, lmt_next);

#define MPID_nem_ib_diff32(a, b) ((uint32_t)((a + (1ULL<<32) - b) & ((1ULL<<32)-1)))
#define MPID_nem_ib_sendq_ready_to_send_head(vc_ib) (vc_ib->ibcom->ncom < MPID_NEM_IB_COM_MAX_SQ_CAPACITY && MPID_nem_ib_ncqe < MPID_NEM_IB_COM_MAX_CQ_CAPACITY && MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail) < MPID_NEM_IB_COM_RDMABUF_NSEG)
#define MPID_nem_ib_sendq_ready_to_send_head_lmt_put(vc_ib) (vc_ib->ibcom->ncom_lmt_put < MPID_NEM_IB_COM_MAX_SQ_CAPACITY && MPID_nem_ib_ncqe_lmt_put < MPID_NEM_IB_COM_MAX_CQ_CAPACITY)

/* counting bloom filter to detect multiple lmt-sends in one send-wait period to
   avoid overwriting the last byte in the receive buffer */
#define MPID_nem_ib_cbf_nslot 16        /* slots */
#define MPID_nem_ib_cbf_bitsperslot 4   /* one slot can accomodate multiple bits */
#define MPID_nem_ib_cbf_lognslot 4
#define MPID_nem_ib_cbf_nhash 3 /* number of hash functions */
#define MPID_nem_ib_getpos \
    int pos_8b = pos / (8 / MPID_nem_ib_cbf_bitsperslot);\
    assert(0 <= pos_8b && pos_8b < MPID_nem_ib_cbf_nslot * MPID_nem_ib_cbf_bitsperslot / 8);\
    int pos_bps = pos & (8 / MPID_nem_ib_cbf_bitsperslot - 1);
#define MPID_nem_ib_shift \
    ((array[pos_8b] >> (pos_bps * MPID_nem_ib_cbf_bitsperslot)) & ((1ULL<<MPID_nem_ib_cbf_bitsperslot) - 1))
#define MPID_nem_ib_maskset \
    array[pos_8b] &= ~(((1ULL<<MPID_nem_ib_cbf_bitsperslot) - 1) << (pos_bps * MPID_nem_ib_cbf_bitsperslot)); \
    array[pos_8b] |= (bits & ((1ULL<<MPID_nem_ib_cbf_bitsperslot)-1)) << (pos_bps * MPID_nem_ib_cbf_bitsperslot)
static inline int MPID_nem_ib_cbf_get(uint8_t * array, int pos)
{
    MPID_nem_ib_getpos;
    return MPID_nem_ib_shift;
}

static inline void MPID_nem_ib_cbf_set(uint8_t * array, int pos, uint16_t bits)
{
    MPID_nem_ib_getpos;
    MPID_nem_ib_maskset;
}

static inline void MPID_nem_ib_cbf_inc(uint8_t * array, int pos)
{
    MPID_nem_ib_getpos;
    int16_t bits = MPID_nem_ib_shift;
    assert(bits != (1ULL << MPID_nem_ib_cbf_bitsperslot) - 1);
    bits++;
    MPID_nem_ib_maskset;
}

static inline void MPID_nem_ib_cbf_dec(uint8_t * array, int pos)
{
    MPID_nem_ib_getpos;
    int16_t bits = MPID_nem_ib_shift;
    assert(bits != 0);
    bits--;
    MPID_nem_ib_maskset;
}

static inline int MPID_nem_ib_cbf_hash1(uint64_t addr)
{
    return
        (((addr >> (MPID_nem_ib_cbf_lognslot * 0)) & (MPID_nem_ib_cbf_nslot - 1)) ^
         ((addr >> (MPID_nem_ib_cbf_lognslot * 3)) & (MPID_nem_ib_cbf_nslot - 1)) ^
         ((addr >> (MPID_nem_ib_cbf_lognslot * 6)) & (MPID_nem_ib_cbf_nslot - 1))
         + 1) & (MPID_nem_ib_cbf_nslot - 1);
}

static inline int MPID_nem_ib_cbf_hash2(uint64_t addr)
{
    /* adding one because addr tends to have a postfix of "fff" */
    return
        (((addr >> (MPID_nem_ib_cbf_lognslot * 1)) & (MPID_nem_ib_cbf_nslot - 1)) ^
         ((addr >> (MPID_nem_ib_cbf_lognslot * 4)) & (MPID_nem_ib_cbf_nslot - 1)) ^
         ((addr >> (MPID_nem_ib_cbf_lognslot * 7)) & (MPID_nem_ib_cbf_nslot - 1))
         + 1) & (MPID_nem_ib_cbf_nslot - 1);
}

static inline int MPID_nem_ib_cbf_hash3(uint64_t addr)
{
    /* adding two because addr tends to have a postfix of "fff" */
    return
        (((addr >> (MPID_nem_ib_cbf_lognslot * 2)) & (MPID_nem_ib_cbf_nslot - 1)) ^
         ((addr >> (MPID_nem_ib_cbf_lognslot * 5)) & (MPID_nem_ib_cbf_nslot - 1)) ^
         ((addr >> (MPID_nem_ib_cbf_lognslot * 8)) & (MPID_nem_ib_cbf_nslot - 1))
         + 2) & (MPID_nem_ib_cbf_nslot - 1);

}

static inline void MPID_nem_ib_cbf_add(uint64_t addr, uint8_t * array)
{
    //dprintf("cbf_add,addr=%08lx,%08x,%08x,%08x\n", addr, MPID_nem_ib_cbf_hash1(addr), MPID_nem_ib_cbf_hash2(addr), MPID_nem_ib_cbf_hash3(addr));
    //dprintf("cbf_add,%d,%d,%d\n", MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)));
    MPID_nem_ib_cbf_inc(array, MPID_nem_ib_cbf_hash1(addr));
    MPID_nem_ib_cbf_inc(array, MPID_nem_ib_cbf_hash2(addr));
    MPID_nem_ib_cbf_inc(array, MPID_nem_ib_cbf_hash3(addr));
    //dprintf("cbf_add,%d,%d,%d\n", MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)));
}

static inline void MPID_nem_ib_cbf_delete(uint64_t addr, uint8_t * array)
{
    //dprintf("cbf_delete,addr=%08lx,%08x,%08x,%08x\n", addr, MPID_nem_ib_cbf_hash1(addr), MPID_nem_ib_cbf_hash2(addr), MPID_nem_ib_cbf_hash3(addr));
    //dprintf("cbf_delete,%d,%d,%d\n", MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)));
    MPID_nem_ib_cbf_dec(array, MPID_nem_ib_cbf_hash1(addr));
    MPID_nem_ib_cbf_dec(array, MPID_nem_ib_cbf_hash2(addr));
    MPID_nem_ib_cbf_dec(array, MPID_nem_ib_cbf_hash3(addr));
    //dprintf("cbf_delete,%d,%d,%d\n", MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)));
}

static inline int MPID_nem_ib_cbf_query(uint64_t addr, uint8_t * array)
{
    //dprintf("cbf_query,addr=%08lx,%08x,%08x,%08x\n", addr, MPID_nem_ib_cbf_hash1(addr), MPID_nem_ib_cbf_hash2(addr), MPID_nem_ib_cbf_hash3(addr));
    //dprintf("cbf_query,%d,%d,%d\n", MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)));
    return
        MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)) > 0 &&
        MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)) > 0 &&
        MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)) > 0;
}

static inline int MPID_nem_ib_cbf_would_overflow(uint64_t addr, uint8_t * array)
{
    //dprintf("cbf_would_overflow,addr=%08lx,%08x,%08x,%08x\n", addr, MPID_nem_ib_cbf_hash1(addr), MPID_nem_ib_cbf_hash2(addr), MPID_nem_ib_cbf_hash3(addr));
    //dprintf("cbf_would_overflow,%d,%d,%d\n", MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash1(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash2(addr)), MPID_nem_ib_cbf_get(array, MPID_nem_ib_cbf_hash3(addr)));
    return
        MPID_nem_ib_cbf_get(array,
                            MPID_nem_ib_cbf_hash1(addr)) ==
        (1ULL << MPID_nem_ib_cbf_bitsperslot) - 1 ||
        MPID_nem_ib_cbf_get(array,
                            MPID_nem_ib_cbf_hash2(addr)) ==
        (1ULL << MPID_nem_ib_cbf_bitsperslot) - 1 ||
        MPID_nem_ib_cbf_get(array,
                            MPID_nem_ib_cbf_hash3(addr)) ==
        (1ULL << MPID_nem_ib_cbf_bitsperslot) - 1;
}

/* functions */
uint8_t MPID_nem_ib_rand(void);
uint64_t MPID_nem_ib_rdtsc(void);
int MPID_nem_ib_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_ib_finalize(void);
int MPID_nem_ib_drain_scq(int dont_call_progress);
int MPID_nem_ib_drain_scq_lmt_put(void);
int MPID_nem_ib_drain_scq_scratch_pad(void);
int MPID_nem_ib_poll(int in_blocking_poll);
int MPID_nem_ib_poll_eager(MPIDI_VC_t * vc);

int MPID_nem_ib_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_ib_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc);
int MPID_nem_ib_vc_init(MPIDI_VC_t * vc);
int MPID_nem_ib_vc_destroy(MPIDI_VC_t * vc);
int MPID_nem_ib_vc_terminate(MPIDI_VC_t * vc);
int MPID_nem_ib_pkthandler_init(MPIDI_CH3_PktHandler_Fcn * pktArray[], int arraySize);

int MPID_nem_ib_SendNoncontig(MPIDI_VC_t * vc, MPID_Request * sreq, void *header,
                              MPIDI_msg_sz_t hdr_sz);

/* CH3 send/recv functions */
int MPID_nem_ib_iSendContig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr,
                            MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_ib_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data,
                                MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr);

/* used by ib_poll.c */
int MPID_nem_ib_send_progress(MPID_nem_ib_vc_area * vc_ib);

/* CH3--lmt send/recv functions */
int MPID_nem_ib_lmt_initiate_lmt(struct MPIDI_VC *vc, union MPIDI_CH3_Pkt *rts_pkt,
                                 struct MPID_Request *req);
int MPID_nem_ib_lmt_start_recv_core(struct MPID_Request *req, void *raddr, uint32_t rkey,
                                    void *write_to_buf);
int MPID_nem_ib_lmt_start_recv(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV s_cookie);
int MPID_nem_ib_lmt_handle_cookie(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV cookie);
int MPID_nem_ib_lmt_switch_send(struct MPIDI_VC *vc, struct MPID_Request *req);
int MPID_nem_ib_lmt_done_send(struct MPIDI_VC *vc, struct MPID_Request *req);
int MPID_nem_ib_lmt_done_recv(struct MPIDI_VC *vc, struct MPID_Request *req);
int MPID_nem_ib_lmt_vc_terminated(struct MPIDI_VC *vc);
/* overriding functions
   initialize the value of a member named "recv_posted"
   in BSS-variable named "comm_ops" with type of MPIDI_Comm_ops_t
   to "MPID_nem_ib_recv_posted" in ib_init.c
   MPIDI_Comm_ops_t is defined in src/mpid/ch3/include/mpidimpl.h */
int MPID_nem_ib_recv_posted(struct MPIDI_VC *vc, struct MPID_Request *req);
int MPID_nem_ib_recv_buf_released(struct MPIDI_VC *vc, void *user_data);

/* Keys for business cards */
#define MPID_NEM_IB_GID_KEY "gid"
#define MPID_NEM_IB_LID_KEY "lid"
#define MPID_NEM_IB_QPN_KEY "qpn"
#define MPID_NEM_IB_RKEY_KEY "rkey"
#define MPID_NEM_IB_RMEM_KEY "rmem"

#define MPID_NEM_IB_RECV_MAX_PKT_LEN 1024

extern int MPID_nem_ib_conn_ud_fd;
extern MPID_nem_ib_com_t *MPID_nem_ib_conn_ud_ibcom;
extern MPID_nem_ib_conn_ud_t *MPID_nem_ib_conn_ud;
extern MPID_nem_ib_conn_t *MPID_nem_ib_conns;
extern MPIDI_VC_t **MPID_nem_ib_pollingset;
extern int *MPID_nem_ib_scratch_pad_fds;
extern int MPID_nem_ib_npollingset;
extern void *MPID_nem_ib_fl[18];
extern int MPID_nem_ib_nranks;
//extern char *MPID_nem_ib_recv_buf;
extern int MPID_nem_ib_myrank;
extern uint64_t MPID_nem_ib_tsc_poll;   /* to throttle ib_poll in recv_posted (in ib_poll.c) */
extern int MPID_nem_ib_ncqe;    /* for lazy poll scq */
extern int MPID_nem_ib_ncqe_lmt_put;    /* lmt-put uses another QP, SQ, CQ to speed-up fetching CQE */
#ifdef MPID_NEM_IB_ONDEMAND
extern MPID_nem_ib_cm_map_t MPID_nem_ib_cm_state;
extern int MPID_nem_ib_ncqe_connect;    /* couting outstanding connection requests */
#endif
extern int MPID_nem_ib_ncqe_scratch_pad;
extern int MPID_nem_ib_ncqe_to_drain;   /* count put in lmt-put-done protocol */
extern int MPID_nem_ib_ncqe_nces;       /* counting non-copied eager-send */
extern MPID_nem_ib_lmtq_t MPID_nem_ib_lmtq;     /* poll queue for lmt */
extern MPID_nem_ib_lmtq_t MPID_nem_ib_lmt_orderq;       /* force order when two or more rts_to_sender randomizes the last byte of receive buffer */
extern MPID_nem_ib_vc_area *MPID_nem_ib_debug_current_vc_ib;

/* to detect multiple lmt-sends in one send-wait period to
   avoid overwriting the last byte in the receive buffer */
extern uint8_t MPID_nem_ib_lmt_tail_addr_cbf[MPID_nem_ib_cbf_nslot *
                                             MPID_nem_ib_cbf_bitsperslot / 8];

#define MPID_NEM_IB_MAX_POLLINGSET 65536

/* xfer.c manages memory region using memid */
#define MPID_NEM_IB_MEMID_RDMA 0

/* command using IB UD */
#define MPID_NEM_IB_SYNC_SYN 0
#define MPID_NEM_IB_SYNC_SYNACK 1
#define MPID_NEM_IB_SYNC_NACK 2

#define MPID_NEM_IB_EAGER_MAX_MSG_SZ (MPID_NEM_IB_COM_RDMABUF_SZSEG/*1024*/-sizeof(MPIDI_CH3_Pkt_t)+sizeof(MPIDI_CH3_Pkt_eager_send_t)-sizeof(MPID_nem_ib_sz_hdrmagic_t)-sizeof(MPID_nem_ib_pkt_prefix_t)-sizeof(MPID_nem_ib_tailmagic_t))      /* when > this size, lmt is used. see src/mpid/ch3/src/mpid_isend.c */
#define MPID_NEM_IB_POLL_PERIOD_RECV_POSTED 2000        /* minimum period from previous ib_poll to ib_poll in recv_posted */
#define MPID_NEM_IB_POLL_PERIOD_SEND_POSTED 2000

typedef struct {
    void *addr;
    uint32_t rkey;
    int seq_num_tail;           /* notify RDMA-write-to buffer occupation */
    uint8_t tail;               /* last word of payload */
} MPID_nem_ib_lmt_cookie_t;

typedef enum MPID_nem_ib_pkt_subtype {
    MPIDI_NEM_IB_PKT_EAGER_SEND,
    MPIDI_NEM_IB_PKT_PUT,
    MPIDI_NEM_IB_PKT_ACCUMULATE,
    MPIDI_NEM_IB_PKT_GET,
    MPIDI_NEM_IB_PKT_GET_RESP,
    MPIDI_NEM_IB_PKT_LMT_GET_DONE,
    MPIDI_NEM_IB_PKT_REQ_SEQ_NUM,
    MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM,
    MPIDI_NEM_IB_PKT_CHG_RDMABUF_OCC_NOTIFY_STATE,
    MPIDI_NEM_IB_PKT_NUM_PKT_HANDLERS
} MPID_nem_ib_pkt_subtype_t;

/* derived from MPID_nem_pkt_netmod_t */
typedef struct MPID_nem_ib_pkt_prefix {
    MPID_nem_pkt_type_t type;
    unsigned subtype;
    /* additional field */
    int seq_num_tail;
} MPID_nem_ib_pkt_prefix_t;

/* derived from MPID_nem_pkt_netmod_t and MPID_nem_pkt_lmt_done_t */
typedef struct MPID_nem_ib_pkt_lmt_get_done {
    MPID_nem_pkt_type_t type;
    unsigned subtype;
    /* additional field */
    MPI_Request req_id;
    int seq_num_tail;
} MPID_nem_ib_pkt_lmt_get_done_t;

/* derived from MPID_nem_pkt_netmod_t */
typedef struct MPID_nem_ib_pkt_req_seq_num_t {
    MPID_nem_pkt_type_t type;
    unsigned subtype;
    /* additional field */
    int seq_num_tail;
} MPID_nem_ib_pkt_req_seq_num_t;

/* derived from MPID_nem_pkt_netmod_t */
typedef struct MPID_nem_ib_pkt_reply_seq_num_t {
    MPID_nem_pkt_type_t type;
    unsigned subtype;
    /* additional field */
    int seq_num_tail;
} MPID_nem_ib_pkt_reply_seq_num_t;

/* derived from MPID_nem_pkt_netmod_t */
typedef struct MPID_nem_ib_pkt_change_rdmabuf_occupancy_notify_state_t {
    MPID_nem_pkt_type_t type;
    unsigned subtype;
    /* additional field */
    int state;
} MPID_nem_ib_pkt_change_rdmabuf_occupancy_notify_state_t;

int MPID_nem_ib_PktHandler_EagerSend(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                     MPIDI_msg_sz_t * buflen /* out */ ,
                                     MPID_Request ** rreqp /* out */);
int MPID_nem_ib_PktHandler_Put(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                               MPIDI_msg_sz_t * buflen /* out */ ,
                               MPID_Request ** rreqp /* out */);
int MPID_nem_ib_PktHandler_Accumulate(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                      MPIDI_msg_sz_t * buflen /* out */ ,
                                      MPID_Request ** rreqp /* out */);
int MPID_nem_ib_PktHandler_Get(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                               MPIDI_msg_sz_t * buflen /* out */ ,
                               MPID_Request ** rreqp /* out */);
int MPID_nem_ib_PktHandler_GetResp(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                   MPIDI_msg_sz_t * buflen /* out */ ,
                                   MPID_Request ** rreqp /* out */);
int MPID_nem_ib_PktHandler_lmt_done(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                    MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp);
int MPID_nem_ib_pkt_GET_DONE_handler(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                     MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp);
int MPID_nem_ib_PktHandler_req_seq_num(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp);
int MPID_nem_ib_PktHandler_reply_seq_num(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                         MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp);
int MPID_nem_ib_PktHandler_change_rdmabuf_occupancy_notify_state(MPIDI_VC_t * vc,
                                                                 MPIDI_CH3_Pkt_t * pkt,
                                                                 MPIDI_msg_sz_t * buflen,
                                                                 MPID_Request ** rreqp);

/* MPID_nem_ib_PktHandler_lmt_done is a wrapper of pkt_DONE_handler and calls it */
/* pkt_DONE_handler (in src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt.c) is not exported */
int pkt_DONE_handler(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, MPIDI_msg_sz_t * buflen,
                     MPID_Request ** rreqp);


#define MPID_nem_ib_send_req_seq_num(vc) do { \
        MPID_PKT_DECL_CAST(_upkt, MPID_nem_ib_pkt_req_seq_num_t, _pkt); \
        MPID_Request *_req; \
        \
        MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending req_seq_num packet"); \
        MPIDI_Pkt_init(_pkt, MPIDI_NEM_PKT_NETMOD); \
        _pkt->subtype = MPIDI_NEM_IB_PKT_REQ_SEQ_NUM; \
        \
        MPID_nem_ib_vc_area *vc_ib = VC_IB(vc); \
        _pkt->seq_num_tail = vc_ib->ibcom->rsr_seq_num_tail; \
        vc_ib->ibcom->rsr_seq_num_tail_last_sent = vc_ib->ibcom->rsr_seq_num_tail; \
        \
        mpi_errno = MPIDI_CH3_iStartMsg((vc), _pkt, sizeof(*_pkt), &_req); \
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_req_seq_num"); \
        if (_req != NULL) { \
            MPIU_ERR_CHKANDJUMP(_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_req_seq_num"); \
            MPID_Request_release(_req); \
        } \
    } while (0)

#define MPID_nem_ib_send_reply_seq_num(vc) do {                                                             \
        MPID_PKT_DECL_CAST(_upkt, MPID_nem_ib_pkt_reply_seq_num_t, _pkt);                                   \
        MPID_Request *_req;                                                                                   \
                                                                                                              \
        MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending reply_seq_num packet"); \
        MPIDI_Pkt_init(_pkt, MPIDI_NEM_PKT_NETMOD); \
        _pkt->subtype = MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM; \
                                                                                                              \
        int *rsr_seq_num_tail;                                                                                \
        ibcom_errno = MPID_nem_ib_com_rsr_seq_num_tail_get(VC_FIELD(vc, sc->fd), &rsr_seq_num_tail); \
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rsr_seq_num_tail_get");           \
        _pkt->seq_num_tail = *rsr_seq_num_tail;                                                               \
                                                                                                              \
        int *rsr_seq_num_tail_last_sent;                                                                      \
        ibcom_errno = MPID_nem_ib_com_rsr_seq_num_tail_last_sent_get(VC_FIELD(vc, sc->fd), &rsr_seq_num_tail_last_sent); \
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rsr_seq_num_tail_last_sent_get");           \
        *rsr_seq_num_tail_last_sent = *rsr_seq_num_tail;                                                      \
\
        mpi_errno = MPIDI_CH3_iStartMsg((vc), _pkt, sizeof(*_pkt), &_req);                                    \
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_reply_seq_num");                          \
        if (_req != NULL) {                                                                                   \
            MPIU_ERR_CHKANDJUMP(_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_reply_seq_num");         \
            MPID_Request_release(_req);                                                                       \
        }                                                                                                     \
    } while (0)

#define MPID_nem_ib_send_change_rdmabuf_occupancy_notify_state(vc, _state) do {                             \
        MPID_PKT_DECL_CAST(_upkt, MPID_nem_ib_pkt_change_rdmabuf_occupancy_notify_state_t, _pkt);           \
        MPID_Request *_req;                                                                                   \
                                                                                                              \
        MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending change_rdmabuf_occupancy_notify_state packet");               \
        MPIDI_Pkt_init(_pkt, MPIDI_NEM_PKT_NETMOD); \
        _pkt->subtype = MPIDI_NEM_IB_PKT_CHG_RDMABUF_OCC_NOTIFY_STATE; \
        _pkt->state = _state;                                                                                 \
                                                                                                              \
        mpi_errno = MPIDI_CH3_iStartMsg((vc), _pkt, sizeof(*_pkt), &_req);                                    \
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_change_rdmabuf_occupancy_notify_state");                          \
        if (_req != NULL) {                                                                                   \
            MPIU_ERR_CHKANDJUMP(_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_change_rdmabuf_occupancy_notify_state");         \
            MPID_Request_release(_req);                                                                       \
        }                                                                                                     \
    } while (0)

#define MPID_nem_ib_change_rdmabuf_occupancy_notify_policy_lw(vc_ib, lsr_seq_num_tail) \
    do { \
        int lsr_seq_num_head; \
        /* sequence number of (largest) in-flight send command */ \
        ibcom_errno = MPID_nem_ib_com_sseq_num_get(vc_ib->sc->fd, &lsr_seq_num_head); \
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_sseq_num_get"); \
        \
        int *rdmabuf_occupancy_notify_rstate; \
        ibcom_errno = MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get(vc_ib->sc->fd, &rdmabuf_occupancy_notify_rstate); \
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rdmabuf_occupancy_notify_rstate_get"); \
        \
        /*dprintf("notify_policy_lw,head=%d,tail=%d,lw=%d\n", lsr_seq_num_head, *lsr_seq_num_tail, MPID_NEM_IB_COM_RDMABUF_LOW_WATER_MARK);*/ \
        /* if the number of occupied slot of RDMA-write-to buffer have got below the low water-mark */ \
        if (*rdmabuf_occupancy_notify_rstate == MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_HW && \
           MPID_nem_ib_diff32(lsr_seq_num_head, *lsr_seq_num_tail) < MPID_NEM_IB_COM_RDMABUF_LOW_WATER_MARK) { \
            dprintf("changing notify_rstate\n"); \
            /* remember remote notifying policy so that local can know when to change remote policy back to HW */ \
            *rdmabuf_occupancy_notify_rstate = MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW; \
            /* change remote notifying policy of RDMA-write-to buf occupancy info */ \
            MPID_nem_ib_send_change_rdmabuf_occupancy_notify_state(vc, MPID_NEM_IB_COM_RDMABUF_OCCUPANCY_NOTIFY_STATE_LW); \
        } \
    } while (0)

#define MPID_nem_ib_lmt_send_GET_DONE(vc, rreq) do {                                                                   \
        MPID_PKT_DECL_CAST(_upkt, MPID_nem_ib_pkt_lmt_get_done_t, _done_pkt);                                          \
        MPID_Request *_done_req;                                                                                \
                                                                                                                \
        MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending rndv DONE packet"); \
        MPIDI_Pkt_init(_done_pkt, MPIDI_NEM_PKT_NETMOD); \
        _done_pkt->subtype = MPIDI_NEM_IB_PKT_LMT_GET_DONE;\
        _done_pkt->req_id = (rreq)->ch.lmt_req_id; \
            /* embed SR occupancy information */ \
        _done_pkt->seq_num_tail = VC_FIELD(vc, ibcom->rsr_seq_num_tail); \
 \
            /* remember the last one sent */ \
        VC_FIELD(vc, ibcom->rsr_seq_num_tail_last_sent) = VC_FIELD(vc, ibcom->rsr_seq_num_tail); \
                                                                                                                \
        mpi_errno = MPIDI_CH3_iStartMsg((vc), _done_pkt, sizeof(*_done_pkt), &_done_req);                       \
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_lmt_send_GET_DONE");                                  \
        if (_done_req != NULL)                                                                                  \
        {                                                                                                       \
            MPIU_ERR_CHKANDJUMP(_done_req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_lmt_send_GET_DONE");            \
            MPID_Request_release(_done_req);                                                                    \
        }                                                                                                       \
    } while (0)

#define MPID_NEM_IB_MAX(a, b) ((a) > (b) ? (a) : (b))

static inline void *MPID_nem_ib_stmalloc(size_t _sz)
{
    size_t sz = _sz;
    int i = 0;
    do {
        i++;
        sz >>= 1;
    } while (sz > 0);
    if (i < 12) {
        return MPIU_Malloc(sz);
    }
    if (i > 30) {
        return mmap(0, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    }
    int ndx = i - 12;
    void *slot;
    if (MPID_nem_ib_fl[ndx]) {
        slot = MPID_nem_ib_fl[ndx];
        if (MPID_nem_ib_myrank == 1) {
            //printf("stmalloc,reuse %p,%08x\n", slot, (int)_sz);
        }
        MPID_nem_ib_fl[ndx] = *((void **) MPID_nem_ib_fl[ndx]);
    }
    else {
        slot = mmap(0, 1 << i, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (MPID_nem_ib_myrank == 1) {
            //printf("stmalloc,new %p,%08x\n", slot, (int)_sz);
        }
    }
    return slot;
}

static inline void MPID_nem_ib_stfree(void *ptr, size_t sz)
{
    if (MPID_nem_ib_myrank == 1) {
        //printf("stfree,%p,%08x\n", ptr, (int)sz);
    }
    int i = 0;
    do {
        i++;
        sz >>= 1;
    } while (sz > 0);
    if (i < 12) {
        MPIU_Free(ptr);
        goto fn_exit;
    }
    if (i > 30) {
        munmap(ptr, sz);
        goto fn_exit;
    }
    int ndx = i - 12;
    *((void **) ptr) = MPID_nem_ib_fl[ndx];
    MPID_nem_ib_fl[ndx] = ptr;
  fn_exit:;
}
#endif /* IB_IMPL_H_INCLUDED */
