/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_RMA_CALLBACKS_H_INCLUDED
#define MPIDIG_RMA_CALLBACKS_H_INCLUDED

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

/* RMA callback routines on the packet issuing side.
 * All handler functions are named with suffix "_origin_cb". */

int MPIDIG_put_ack_origin_cb(MPIR_Request * req);
int MPIDIG_acc_ack_origin_cb(MPIR_Request * req);
int MPIDIG_get_acc_ack_origin_cb(MPIR_Request * req);
int MPIDIG_cswap_ack_origin_cb(MPIR_Request * req);
int MPIDIG_get_ack_origin_cb(MPIR_Request * req);
int MPIDIG_put_origin_cb(MPIR_Request * sreq);
int MPIDIG_cswap_origin_cb(MPIR_Request * sreq);
int MPIDIG_acc_origin_cb(MPIR_Request * sreq);
int MPIDIG_get_acc_origin_cb(MPIR_Request * sreq);
int MPIDIG_put_data_origin_cb(MPIR_Request * sreq);
int MPIDIG_acc_data_origin_cb(MPIR_Request * sreq);
int MPIDIG_get_acc_data_origin_cb(MPIR_Request * sreq);
int MPIDIG_put_dt_origin_cb(MPIR_Request * sreq);
int MPIDIG_acc_dt_origin_cb(MPIR_Request * sreq);
int MPIDIG_get_acc_dt_origin_cb(MPIR_Request * sreq);
int MPIDIG_get_origin_cb(MPIR_Request * sreq);

/* All RMA callback routines and the completion function of
 * each callback (e.g., received all data) on the packet receiving side. All handler
 * functions are named with suffix "_target_msg_cb", and all handler completion
 * function are named with suffix "_target_cmpl_cb". */

int MPIDIG_put_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 uint32_t attr, MPIR_Request ** req);
int MPIDIG_acc_ack_target_msg_cb(void *am_hdr, void *data,
                                 MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_acc_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                     uint32_t attr, MPIR_Request ** req);
int MPIDIG_cswap_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_complete_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                      uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_post_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_lock_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_lockall_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                     uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_unlock_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_unlockall_target_msg_cb(void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_lock_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                      uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_lockall_ack_target_msg_cb(void *am_hdr, void *data,
                                         MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_unlock_ack_target_msg_cb(void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_win_unlockall_ack_target_msg_cb(void *am_hdr, void *data,
                                           MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_put_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                             uint32_t attr, MPIR_Request ** req);
int MPIDIG_put_dt_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                uint32_t attr, MPIR_Request ** req);
int MPIDIG_put_dt_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    uint32_t attr, MPIR_Request ** req);
int MPIDIG_acc_dt_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_acc_dt_ack_target_msg_cb(void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDIG_put_data_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  uint32_t attr, MPIR_Request ** req);
int MPIDIG_acc_data_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                  uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_acc_data_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                      uint32_t attr, MPIR_Request ** req);
int MPIDIG_cswap_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                               uint32_t attr, MPIR_Request ** req);
int MPIDIG_acc_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                             uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_acc_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 uint32_t attr, MPIR_Request ** req);
int MPIDIG_acc_dt_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_acc_dt_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                    uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                             uint32_t attr, MPIR_Request ** req);
int MPIDIG_get_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                 uint32_t attr, MPIR_Request ** req);

#endif /* MPIDIG_RMA_CALLBACKS_H_INCLUDED */
