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
#ifndef CH4R_RMA_ORIGIN_CALLBACKS_H_INCLUDED
#define CH4R_RMA_ORIGIN_CALLBACKS_H_INCLUDED

/* This file includes all RMA callback routines on the packet issuing side.
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
int MPIDIG_put_iov_origin_cb(MPIR_Request * sreq);
int MPIDIG_acc_iov_origin_cb(MPIR_Request * sreq);
int MPIDIG_get_acc_iov_origin_cb(MPIR_Request * sreq);
int MPIDIG_get_origin_cb(MPIR_Request * sreq);

#endif /* CH4R_RMA_ORIGIN_CALLBACKS_H_INCLUDED */
