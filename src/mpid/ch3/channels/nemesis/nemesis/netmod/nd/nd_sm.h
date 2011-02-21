/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef ND_SM_H_INCLUDED
#define ND_SM_H_INCLUDED

#include "nd_impl.h"

/* FIXME: Use inline buffering - see ND_SGE doc - to improve latency */
/* ND packet types */
typedef enum {
    /* Ack packet - no data */
    /* FIXME: we could piggyback data on a CACK*/
    MPID_NEM_ND_CONN_ACK_PKT=0,
    /* Nack packet - no data*/
    MPID_NEM_ND_CONN_NAK_PKT,
    /* Credit packet - contains credits to send data & may contain data */
    MPID_NEM_ND_CRED_PKT,
    /* Contains data - No credits/MW etc */
    MPID_NEM_ND_DATA_PKT,
    /* Read Src Avail pkt - contains handle to atleast one MW & may contain data */
    MPID_NEM_ND_RD_AVAIL_PKT,
    /* Read ack i.e., RDMA read complete packet - may contain data */
    MPID_NEM_ND_RD_ACK_PKT,
    MPID_NEM_ND_INVALID_PKT
} MPID_Nem_nd_pkt_type_t;

/* Is pkt flow controlled ? */
#define MPID_NEM_ND_IS_FC_PKT(pkt_type) ((pkt_type != MPID_NEM_ND_CRED_PKT) && \
                                        (pkt_type != MPID_NEM_ND_RD_AVAIL_PKT) && \
                                        (pkt_type != MPID_NEM_ND_RD_ACK_PKT))

typedef enum{
    MPID_NEM_ND_SR_PACK=0,
    MPID_NEM_ND_ZCP_PACK,
    MPID_NEM_ND_INVALID_PACK
} MPID_Nem_nd_pack_t;

/* We use a simple cookie to make sure that the connection
 * is an MPICH2 nd connection
 */
#define MPID_NEM_ND_MAGIC_COOKIE_LEN 8
#define MPID_NEM_ND_MAGIC_COOKIE   "mpichnd"
/* pg_info is not a packet */
typedef struct MPID_Nem_nd_pg_info_hdr_{
    char magic_cookie[MPID_NEM_ND_MAGIC_COOKIE_LEN];
    int pg_rank;
    int pg_id_len;
} MPID_Nem_nd_pg_info_hdr_t;
#define MPID_NEM_ND_IS_SAME_PGID(_id1, _id2) (strcmp(_id1, _id2) == 0)

#define GET_CONNHND_FROM_EX_RECV_OV(_ex_ov_addr) CONTAINING_RECORD(_ex_ov_addr, MPID_Nem_nd_conn_hnd_, recv_ov)
#define GET_CONNHND_FROM_EX_SEND_OV(_ex_ov_addr) CONTAINING_RECORD(_ex_ov_addr, MPID_Nem_nd_conn_hnd_, send_ov)
#define GET_CONNHND_FROM_EX_RD_OV(_ex_ov_addr) CONTAINING_RECORD(_ex_ov_addr, MPID_Nem_nd_conn_hnd_, rd_ov)

#define SET_EX_RD_HANDLER(_conn_hnd, _succ_fn, _fail_fn) MPIU_ExInitOverlapped(&(_conn_hnd->recv_ov), _succ_fn, _fail_fn)
#define SET_EX_WR_HANDLER(_conn_hnd, _succ_fn, _fail_fn) MPIU_ExInitOverlapped(&(_conn_hnd->send_ov), _succ_fn, _fail_fn)

#define GET_MSGRESULT_FROM_NDRESULT(_pnd_result) CONTAINING_RECORD(_pnd_result, MPID_Nem_nd_msg_result_t , result)
/* Get scratch buf from MSG result */
#define GET_MSGBUF_FROM_MSGRESULT(_pmsg_result) (&((CONTAINING_RECORD(_pmsg_result, MPID_Nem_nd_msg_ele_t, msg_result))->msg))
#define GET_CONNHND_FROM_MSGRESULT(_pmsg_result) ((CONTAINING_RECORD(_pmsg_result, MPID_Nem_nd_msg_ele_t, msg_result))->conn_hnd)
#define GET_STATUS_FROM_MSGRESULT(_pmsg_result) (_pmsg_result->result.Status)
#define GET_NB_FROM_MSGRESULT(_pmsg_result) (_pmsg_result->result.BytesTransferred)
#define GET_PNDRESULT_FROM_MSGBUF(_pmsg_buf) (&((CONTAINING_RECORD(_pmsg_buf, MPID_Nem_nd_msg_ele_t, msg))->msg_result.result))
#define GET_CONNHND_FROM_MSGBUF(_pmsg_buf) ((CONTAINING_RECORD(_pmsg_buf, MPID_Nem_nd_msg_ele_t, msg))->conn_hnd)

#define GET_CONNHND_FROM_ZCP_SEND_MSGRESULT(_pmsg_result) CONTAINING_RECORD(_pmsg_result, MPID_Nem_nd_conn_hnd_, zcp_send_result);

#define SET_MSGBUF_HANDLER(_pmsg_buf, _succ_fn, _fail_fn) do{ \
    MPID_Nem_nd_msg_result_t *pmsg_result;          \
    pmsg_result = &((CONTAINING_RECORD(_pmsg_buf, MPID_Nem_nd_msg_ele_t, msg))->msg_result);   \
    pmsg_result->succ_fn = _succ_fn; \
    pmsg_result->fail_fn = _fail_fn; \
}while(0)

#define INIT_MSGRESULT(_pmsg_result, _succ_fn, _fail_fn) do{\
    (_pmsg_result)->succ_fn = _succ_fn;   \
    (_pmsg_result)->fail_fn = _fail_fn;   \
}while(0)

#define GET_RECV_SBUF_HEAD(_conn_hnd) (&(_conn_hnd->rsbuf[0].msg))
#define MSGBUF_FREEQ_INIT(_conn_hnd)do{\
    _conn_hnd->ssbuf_freeq.head = 0; \
    _conn_hnd->ssbuf_freeq.nbuf = MPID_NEM_ND_CONN_SENDQ_SZ; \
}while(0)
#define MSGBUF_FREEQ_IS_EMPTY(_conn_hnd) (_conn_hnd->ssbuf_freeq.nbuf == 0)
#define MSGBUF_FREEQ_DEQUEUE(_conn_hnd, _pmsg_buf) do{\
    MPIU_Assert(!MSGBUF_FREEQ_IS_EMPTY(_conn_hnd)); \
    _pmsg_buf = &(_conn_hnd->ssbuf[_conn_hnd->ssbuf_freeq.head].msg);  \
    (_pmsg_buf)->hdr.type = MPID_NEM_ND_INVALID_PKT; \
    (_pmsg_buf)->hdr.credits = 0;   \
    _conn_hnd->ssbuf_freeq.head = (_conn_hnd->ssbuf_freeq.head + 1) % MPID_NEM_ND_CONN_SENDQ_SZ; \
    _conn_hnd->ssbuf_freeq.nbuf -= 1;    \
}while(0)
#define MSGBUF_FREEQ_ENQUEUE(_conn_hnd, _pmsg_buf) (_conn_hnd->ssbuf_freeq.nbuf++)

#endif
