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

/* This file includes all RMA callback routines and the completion function of
 * each callback (e.g., received all data) on the packet receiving side. All handler
 * functions are named with suffix "_target_msg_cb", and all handler completion
 * function are named with suffix "_target_cmpl_cb". */

extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_winlock_getlocallock ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_cas ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_cas_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_win_ctrl ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_iov ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_iov_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_data ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_iov ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_iov ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_iov_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_iov_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_data ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_data ATTRIBUTE((unused));

int MPIDIG_RMA_Init_targetcb_pvars(void);
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
int MPIDI_put_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                MPIR_Request ** req);
int MPIDI_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                MPIR_Request ** req);
int MPIDI_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                    MPIR_Request ** req);
int MPIDI_cswap_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                  int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                  MPIR_Request ** req);
int MPIDI_win_ctrl_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                 MPIR_Request ** req);
int MPIDI_put_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                            int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                            MPIR_Request ** req);
int MPIDI_put_iov_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                MPIR_Request ** req);
int MPIDI_put_iov_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                    MPIR_Request ** req);
int MPIDI_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                    MPIR_Request ** req);
int MPIDI_get_acc_iov_ack_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                        size_t * p_data_sz, int *is_contig,
                                        MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                        MPIR_Request ** req);
int MPIDI_put_data_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                 MPIR_Request ** req);
int MPIDI_acc_data_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                 int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                 MPIR_Request ** req);
int MPIDI_get_acc_data_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                     MPIR_Request ** req);
int MPIDI_cswap_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                              int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                              MPIR_Request ** req);
int MPIDI_acc_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                            int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                            MPIR_Request ** req);
int MPIDI_get_acc_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                MPIR_Request ** req);
int MPIDI_acc_iov_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                MPIR_Request ** req);
int MPIDI_get_acc_iov_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                    int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                    MPIR_Request ** req);
int MPIDI_get_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                            int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                            MPIR_Request ** req);
int MPIDI_get_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                MPIR_Request ** req);

#endif /* CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED */
