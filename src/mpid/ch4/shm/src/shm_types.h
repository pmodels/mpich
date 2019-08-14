/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef SHM_TYPES_H_INCLUDED
#define SHM_TYPES_H_INCLUDED

typedef enum {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_SHM_XPMEM_SEND_LMT_REQ,       /* issued by sender to initialize a LMT;
                                         * receiver transfers data in callback.  */
    MPIDI_SHM_XPMEM_SEND_LMT_ACK,       /* issued by receiver to notify completion of LMT */
#endif
    MPIDI_SHM_CTRL_IDS_MAX
} MPIDI_SHM_ctrl_id_t;

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_req {
    uint64_t src_offset;        /* send data starting address (buffer + true_lb) */
    uint64_t data_sz;           /* data size in bytes */
    uint64_t sreq_ptr;          /* send request pointer */
    int src_lrank;              /* sender rank on local node */

    /* matching info */
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_SHM_ctrl_xpmem_send_lmt_req_t;

typedef struct MPIDI_SHM_ctrl_xpmem_send_lmt_ack {
    uint64_t sreq_ptr;
} MPIDI_SHM_ctrl_xpmem_send_lmt_ack_t;
#endif

typedef struct {
    union {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        MPIDI_SHM_ctrl_xpmem_send_lmt_req_t xpmem_slmt_req;
        MPIDI_SHM_ctrl_xpmem_send_lmt_ack_t xpmem_slmt_ack;
#endif
    };
} MPIDI_SHM_ctrl_hdr_t;

#endif /* SHM_TYPES_H_INCLUDED */
