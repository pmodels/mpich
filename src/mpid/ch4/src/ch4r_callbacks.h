/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_CALLBACKS_H_INCLUDED
#define CH4R_CALLBACKS_H_INCLUDED

/* This file includes all callback routines and the completion function of
 * receive callback for send-receive AM. All handlers on the packet issuing
 * side are named with suffix "_origin_cb", and all handlers on the
 * packet receiving side are named with "_target_msg_cb". */

#include "mpidig.h"

int MPIDIG_check_cmpl_order(MPIR_Request * req, MPIDIG_am_target_cmpl_cb target_cmpl_cb);
void MPIDIG_progress_compl_list(void);
int MPIDIG_send_origin_cb(MPIR_Request * sreq);
int MPIDIG_send_long_lmt_origin_cb(MPIR_Request * sreq);
int MPIDIG_ssend_ack_origin_cb(MPIR_Request * req);
int MPIDIG_send_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                              int is_local, int *is_contig,
                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDIG_send_long_req_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                       size_t * p_data_sz, int is_local, int *is_contig,
                                       MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                       MPIR_Request ** req);
int MPIDIG_send_long_lmt_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                       size_t * p_data_sz, int is_local, int *is_contig,
                                       MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                       MPIR_Request ** req);
int MPIDIG_ssend_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                               int is_local, int *is_contig,
                               MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDIG_ssend_ack_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                   size_t * p_data_sz, int is_local, int *is_contig,
                                   MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDIG_send_long_ack_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                       size_t * p_data_sz, int is_local, int *is_contig,
                                       MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                       MPIR_Request ** req);
int MPIDIG_comm_abort_origin_cb(MPIR_Request * sreq);
int MPIDIG_comm_abort_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int is_local, int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);

#endif /* CH4R_CALLBACKS_H_INCLUDED */
