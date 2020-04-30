/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_TYPES_H_INCLUDED
#define SHM_TYPES_H_INCLUDED

typedef enum {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_SHM_XPMEM_SEND_LMT_RTS,       /* issued by sender to initialize XPMEM protocol with sbuf info */
    MPIDI_SHM_XPMEM_SEND_LMT_CTS,       /* issued by receiver with rbuf info after receiving RTS if it performs coop copy. */
    MPIDI_SHM_XPMEM_SEND_LMT_SEND_FIN,  /* issued by sender to notify completion of coop copy */
    MPIDI_SHM_XPMEM_SEND_LMT_RECV_FIN,  /* issued by receiver to notify completion of coop copy or single copy */
    MPIDI_SHM_XPMEM_SEND_LMT_CNT_FREE,  /* issued by sender to notify free counter obj in coop copy */
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_GPU_IPC
    MPIDI_SHM_GPU_SEND_IPC_RECV_REQ,    /* issued by sender to request a receiver driven data transfer */
    MPIDI_SHM_GPU_SEND_IPC_RECV_ACK,    /* issued by receiver to acknowledge receiver driven data transfer completion */
#endif
    MPIDI_SHM_CTRL_IDS_MAX
} MPIDI_SHM_ctrl_id_t;

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_rts {
    uint64_t src_offset;        /* send data starting address (buffer + true_lb) */
    uint64_t data_sz;           /* data size in bytes */
    uint64_t sreq_ptr;          /* send request pointer */
    int src_lrank;              /* sender rank on local node */

    /* matching info */
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_SHM_ctrl_xpmem_send_lmt_rts_t;

typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_cts {
    uint64_t dest_offset;       /* receiver buffer starting address (buffer + true_lb) */
    uint64_t data_sz;           /* receiver buffer size in bytes */
    uint64_t sreq_ptr;          /* send request pointer */
    uint64_t rreq_ptr;          /* recv request pointer */
    int dest_lrank;             /* receiver rank on local node */

    /* counter info */
    int coop_counter_direct_flag;
    uint64_t coop_counter_offset;
} MPIDI_SHM_ctrl_xpmem_send_lmt_cts_t;

typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_send_fin {
    uint64_t req_ptr;
} MPIDI_SHM_ctrl_xpmem_send_lmt_send_fin_t;

typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_cnt_free {
    int coop_counter_direct_flag;
    uint64_t coop_counter_offset;
} MPIDI_SHM_ctrl_xpmem_send_lmt_cnt_free_t;

typedef MPIDI_SHM_ctrl_xpmem_send_lmt_send_fin_t MPIDI_SHM_ctrl_xpmem_send_lmt_recv_fin_t;
#endif

#ifdef MPIDI_CH4_SHM_ENABLE_GPU_IPC
typedef struct MPIDI_SHM_ctrl_gpu_send_recv_req {
    uint64_t data_sz;
    MPL_gpu_ipc_mem_handle_t mem_handle;
    uint64_t sreq_ptr;

    /* matching info */
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_SHM_ctrl_gpu_send_recv_req_t;

typedef struct MPIDI_SHM_ctrl_gpu_send_recv_ack {
    uint64_t req_ptr;
} MPIDI_SHM_ctrl_gpu_send_recv_ack_t;
#endif

typedef union {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_SHM_ctrl_xpmem_send_lmt_rts_t xpmem_slmt_rts;
    MPIDI_SHM_ctrl_xpmem_send_lmt_cts_t xpmem_slmt_cts;
    MPIDI_SHM_ctrl_xpmem_send_lmt_send_fin_t xpmem_slmt_send_fin;
    MPIDI_SHM_ctrl_xpmem_send_lmt_recv_fin_t xpmem_slmt_recv_fin;
    MPIDI_SHM_ctrl_xpmem_send_lmt_cnt_free_t xpmem_slmt_cnt_free;
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_GPU_IPC
    MPIDI_SHM_ctrl_gpu_send_recv_req_t gpu_recv_req;
    MPIDI_SHM_ctrl_gpu_send_recv_ack_t gpu_recv_ack;
#endif
    char dummy;                 /* some compilers (suncc) does not like empty struct */
} MPIDI_SHM_ctrl_hdr_t;

#endif /* SHM_TYPES_H_INCLUDED */
