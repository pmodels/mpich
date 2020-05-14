/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED
#define CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED

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
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_dt ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_dt_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_data ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_dt ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_dt ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_dt_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_dt_ack ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_data ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_data ATTRIBUTE((unused));

int MPIDIG_RMA_Init_targetcb_pvars(void);
int MPIDIG_put_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                 MPI_Aint in_data_sz, int is_local, int is_async,
                                 MPIR_Request ** req);
int MPIDIG_get_acc_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                     int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_cswap_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_win_ctrl_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_put_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                             int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_put_dt_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_put_dt_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_acc_dt_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_get_acc_dt_ack_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, int is_local, int is_async,
                                        MPIR_Request ** req);
int MPIDIG_put_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_acc_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_get_acc_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                      int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_cswap_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                               int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_acc_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                             int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_get_acc_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_acc_dt_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_get_acc_dt_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_get_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                             int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_get_ack_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 int is_local, int is_async, MPIR_Request ** req);

#endif /* CH4R_RMA_TARGET_CALLBACKS_H_INCLUDED */
