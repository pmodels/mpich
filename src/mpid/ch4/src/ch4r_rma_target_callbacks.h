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
#ifndef CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED
#define CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED

#include "ch4r_request.h"
#include "ch4r_callbacks.h"

int MPIDI_CH4R_RMA_Init_targetcb_pvars(void);

int MPIDI_ack_put(MPIR_Request * rreq);
int MPIDI_ack_cswap(MPIR_Request * rreq);
int MPIDI_ack_acc(MPIR_Request * rreq);
int MPIDI_ack_get_acc(MPIR_Request * rreq);
int MPIDI_win_lock_advance(MPIR_Win * win);
void MPIDI_win_lock_req_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
void MPIDI_win_lock_ack_proc(int handler_id, const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
void MPIDI_win_unlock_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
void MPIDI_win_complete_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
void MPIDI_win_post_proc(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
void MPIDI_win_unlock_done(const MPIDIG_win_cntrl_msg_t * info, MPIR_Win * win);
int MPIDI_handle_acc_cmpl(MPIR_Request * rreq);
int MPIDI_handle_get_acc_cmpl(MPIR_Request * rreq);
void MPIDI_handle_acc_data(void **data, size_t * p_data_sz, int *is_contig, MPIR_Request * rreq);
int MPIDI_get_target_cmpl_cb(MPIR_Request * req);
int MPIDI_put_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_put_iov_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_acc_iov_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_get_acc_iov_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_cswap_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_acc_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_get_acc_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_get_ack_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_get_acc_ack_target_cmpl_cb(MPIR_Request * areq);
int MPIDI_cswap_ack_target_cmpl_cb(MPIR_Request * rreq);
int MPIDI_put_ack_target_msg_cb(int handler_id, void *am_hdr,
                                void **data,
                                size_t * p_data_sz, int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_acc_ack_target_msg_cb(int handler_id, void *am_hdr,
                                void **data,
                                size_t * p_data_sz, int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr,
                                    void **data,
                                    size_t * p_data_sz, int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_cswap_ack_target_msg_cb(int handler_id, void *am_hdr,
                                  void **data,
                                  size_t * p_data_sz, int *is_contig,
                                  MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_win_ctrl_target_msg_cb(int handler_id, void *am_hdr,
                                 void **data,
                                 size_t * p_data_sz, int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_put_target_msg_cb(int handler_id, void *am_hdr,
                            void **data,
                            size_t * p_data_sz,
                            int *is_contig,
                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_put_iov_target_msg_cb(int handler_id, void *am_hdr,
                                void **data,
                                size_t * p_data_sz,
                                int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_put_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                    void **data,
                                    size_t * p_data_sz,
                                    int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                    void **data,
                                    size_t * p_data_sz,
                                    int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr,
                                        void **data,
                                        size_t * p_data_sz,
                                        int *is_contig,
                                        MPIDIG_am_target_cmpl_cb *
                                        target_cmpl_cb, MPIR_Request ** req);
int MPIDI_put_data_target_msg_cb(int handler_id, void *am_hdr,
                                 void **data,
                                 size_t * p_data_sz,
                                 int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_acc_data_target_msg_cb(int handler_id, void *am_hdr,
                                 void **data,
                                 size_t * p_data_sz,
                                 int *is_contig,
                                 MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_acc_data_target_msg_cb(int handler_id, void *am_hdr,
                                     void **data,
                                     size_t * p_data_sz,
                                     int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                     MPIR_Request ** req);
int MPIDI_cswap_target_msg_cb(int handler_id, void *am_hdr,
                              void **data,
                              size_t * p_data_sz,
                              int *is_contig,
                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_acc_target_msg_cb(int handler_id, void *am_hdr,
                            void **data,
                            size_t * p_data_sz,
                            int *is_contig,
                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_acc_target_msg_cb(int handler_id, void *am_hdr,
                                void **data,
                                size_t * p_data_sz,
                                int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_acc_iov_target_msg_cb(int handler_id, void *am_hdr,
                                void **data,
                                size_t * p_data_sz,
                                int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_acc_iov_target_msg_cb(int handler_id, void *am_hdr,
                                    void **data,
                                    size_t * p_data_sz,
                                    int *is_contig,
                                    MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_target_msg_cb(int handler_id, void *am_hdr,
                            void **data,
                            size_t * p_data_sz,
                            int *is_contig,
                            MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
int MPIDI_get_ack_target_msg_cb(int handler_id, void *am_hdr,
                                void **data,
                                size_t * p_data_sz,
                                int *is_contig,
                                MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);

#endif /* CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED */
