/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_CALLBACKS_H_INCLUDED
#define CH4R_CALLBACKS_H_INCLUDED

/* This file includes all callback routines and the completion function of
 * receive callback for send-receive AM. All handlers on the packet issuing
 * side are named with suffix "_origin_cb", and all handlers on the
 * packet receiving side are named with "_target_msg_cb". */

#include "mpidig_am.h"

int MPIDIG_do_cts(MPIR_Request * rreq);
int MPIDIG_check_cmpl_order(MPIR_Request * req);
void MPIDIG_progress_compl_list(void);
int MPIDIG_send_origin_cb(MPIR_Request * sreq);
int MPIDIG_send_data_origin_cb(MPIR_Request * sreq);
int MPIDIG_ssend_ack_origin_cb(MPIR_Request * req);
int MPIDIG_send_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                              int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_send_data_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint in_data_sz,
                                   int is_local, int is_async, MPIR_Request ** req);
int MPIDIG_ssend_ack_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                   MPI_Aint p_data_sz, int is_local, int is_async,
                                   MPIR_Request ** req);
int MPIDIG_send_cts_target_msg_cb(int handler_id, void *am_hdr, void *data, MPI_Aint p_data_sz,
                                  int is_local, int is_async, MPIR_Request ** req);

#endif /* CH4R_CALLBACKS_H_INCLUDED */
