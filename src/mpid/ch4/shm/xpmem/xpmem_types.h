/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_TYPES_H_INCLUDED
#define XPMEM_TYPES_H_INCLUDED

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

typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_recv_fin {
    uint64_t req_ptr;
} MPIDI_SHM_ctrl_xpmem_send_lmt_recv_fin_t;

typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_cnt_free {
    int coop_counter_direct_flag;
    uint64_t coop_counter_offset;
} MPIDI_SHM_ctrl_xpmem_send_lmt_cnt_free_t;

#endif /* XPMEM_TYPES_H_INCLUDED */
