/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

/*#define MPICH_DBG_OUTPUT*/

#include "ibu.h"

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    char * pg_id;
    int rank;
    int size;
    /*struct MPIDI_Process_group_s *next;*/
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PG_DECL MPIDI_CH3I_Process_group_t ch;

#define MPIDI_CH3_PKT_DECL \
MPIDI_CH3_Pkt_rdma_rts_iov_t rts_iov; \
MPIDI_CH3_Pkt_rdma_cts_iov_t cts_iov; \
MPIDI_CH3_Pkt_rdma_reload_t reload; \
MPIDI_CH3_Pkt_rdma_iov_t iov; \
MPIDI_CH3_Pkt_rdma_limit_upt_t limit_upt; \
MPIDI_CH3_Pkt_rndv_reg_error_t rndv_reg_error; \
MPIDI_CH3_Pkt_rndv_eager_send_t rndv_eager_send;

#define MPIDI_CH3_PKT_DEFS \
typedef struct MPIDI_CH3_Pkt_rdma_rts_iov_t \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    MPI_Request sreq; \
    int iov_len; \
} MPIDI_CH3_Pkt_rdma_rts_iov_t; \
typedef struct MPIDI_CH3_Pkt_rdma_cts_iov_t \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    MPI_Request sreq, rreq; \
    int iov_len; \
} MPIDI_CH3_Pkt_rdma_cts_iov_t; \
typedef struct MPIDI_CH3_Pkt_rdma_reload_t \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    int send_recv; \
    MPI_Request sreq, rreq; \
} MPIDI_CH3_Pkt_rdma_reload_t; \
typedef struct MPIDI_CH3_Pkt_rdma_iov_t \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    MPI_Request req; \
    int send_recv; \
    int iov_len; \
} MPIDI_CH3_Pkt_rdma_iov_t; \
typedef struct MPIDI_CH3_Pkt_rdma_limit_upt_t \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    int iov_len; \
} MPIDI_CH3_Pkt_rdma_limit_upt_t; \
typedef struct MPIDI_CH3_Pkt_rndv_reg_error \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    int iov_len; \
    MPI_Request sreq, rreq; \
} MPIDI_CH3_Pkt_rndv_reg_error_t; \
typedef struct MPIDI_CH3_Pkt_rndv_eager_send \
{ \
    MPIDI_CH3_Pkt_type_t type; \
    MPIDI_Message_match match; \
    MPI_Request sender_req_id, receiver_req_id;	\
    MPIDI_msg_sz_t data_sz; \
    ibu_seqnum_t \
} MPIDI_CH3_Pkt_rndv_eager_send_t;

#define MPIDI_CH3_PKT_ENUM \
    MPIDI_CH3_PKT_RTS_PUT, \
    MPIDI_CH3_PKT_RTS_IOV, \
    MPIDI_CH3_PKT_CTS_IOV, \
    MPIDI_CH3_PKT_RELOAD,  \
    MPIDI_CH3_PKT_IOV,  \
    MPIDI_CH3_PKT_LMT_UPT, \
    MPIDI_CH3_PKT_RNDV_EAGER_SEND, \
    MPIDI_CH3_PKT_RNDV_CTS_IOV_REG_ERR

#define MPIDI_CH3_PKT_RELOAD_SEND 1
#define MPIDI_CH3_PKT_RELOAD_RECV 0

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

typedef struct MPIDI_CH3I_VC
{
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    ibu_t ibu;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    int reading_pkt;
    MPIDI_CH3I_VC_state_t state;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;


#define MPIDI_CH3I_RELOAD_SENDER   0x1
#define MPIDI_CH3I_RELOAD_RECEIVER 0x2

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    /* iov_offset points to the current head element in the IOV */	\
    int iov_offset;							\
    									\
    /*  pkt is used to temporarily store a packet header associated	\
       with this request */						\
    MPIDI_CH3_Pkt_t pkt;						\
									\
    struct MPID_Request *req;						\
									\
    int riov_offset, siov_offset;      					\
    int reload_state;							\
    ibu_mem_t local_iov_mem[MPID_IOV_LIMIT];                            \
    ibu_mem_t remote_iov_mem[MPID_IOV_LIMIT];                           \
    ibu_rndv_status_t rndv_status;                           \
} ch;

/*
 * MPID_Progress_state - device/channel dependent state to be passed between MPID_Progress_{start,wait,end}
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
} MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

#define MPIDI_CH3_REQUEST_KIND_DECL \
MPIDI_CH3I_IOV_WRITE_REQUEST, \
MPIDI_CH3I_IOV_READ_REQUEST, \
MPIDI_CH3I_RTS_IOV_READ_REQUEST

/*
#define MPIDI_CH3I_IOV_WRITE_REQUEST MPID_LAST_REQUEST_KIND + 1
#define MPIDI_CH3I_IOV_READ_REQUEST MPID_LAST_REQUEST_KIND + 2
#define MPIDI_CH3I_RTS_IOV_READ_REQUEST MPID_LAST_REQUEST_KIND + 3
*/

typedef struct MPIDI_CH3I_Alloc_mem_list_t
{
    ibu_mem_t mem;
    void *ptr;
    size_t length;
    struct MPIDI_CH3I_Alloc_mem_list_t *next;
} MPIDI_CH3I_Alloc_mem_list_t;

/* in ib/src/ch3_mem.c for the moment (might be promoted if other channels need it) */
/* extern MPIDI_CH3I_Alloc_mem_list_t *MPIDI_CH3I_Alloc_mem_list_head; */

/*
 * Features needed or implemented by the channel
 */
#define MPIDI_DEV_IMPLEMENTS_KVS
#define MPIDI_DEV_IMPLEMENTS_ABORT

/* FIXME: These should be at most a single definition for the module */
#define MPIDI_CH3_IMPLEMENTS_ALLOC_MEM
#define MPIDI_CH3_IMPLEMENTS_FREE_MEM
#define MPIDI_CH3_IMPLEMENTS_CLEANUP_MEM
/*
#define MPIDI_CH3_IMPLEMENTS_START_EPOCH
#define MPIDI_CH3_IMPLEMENTS_END_EPOCH
#define MPIDI_CH3_IMPLEMENTS_PUT
#define MPIDI_CH3_IMPLEMENTS_GET
#define MPIDI_CH3_IMPLEMENTS_ACCUMULATE
#define MPIDI_CH3_IMPLEMENTS_WIN_CREATE
#define MPIDI_CH3_IMPLEMENTS_WIN_FREE
#define MPIDI_CH3_IMPLEMENTS_START_PT_EPOCH
#define MPIDI_CH3_IMPLEMENTS_END_PT_EPOCH
*/

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
