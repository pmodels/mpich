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
#ifndef MPIDIG_H_INCLUDED
#define MPIDIG_H_INCLUDED

#include <mpidimpl.h>

#define MPIDI_AM_HANDLERS_MAX (64)

typedef int (*MPIDIG_am_target_cmpl_cb) (MPIR_Request * req);
typedef int (*MPIDIG_am_origin_cb) (MPIR_Request * req);

/* Callback function setup by handler register function */
/* for short cases, output arguments are NULL */
typedef int (*MPIDIG_am_target_msg_cb)
 (int handler_id, void *am_hdr, void **data,    /* data should be iovs if *is_contig is false */
  size_t * data_sz, int *is_contig, MPIDIG_am_target_cmpl_cb * target_cmpl_cb,  /* completion handler */
  MPIR_Request ** req);         /* if allocated, need pointer to completion function */

typedef struct MPIDIG_global_t {
    MPIDIG_am_target_msg_cb target_msg_cbs[MPIDI_AM_HANDLERS_MAX];
    MPIDIG_am_origin_cb origin_cbs[MPIDI_AM_HANDLERS_MAX];
} MPIDIG_global_t;
extern MPIDIG_global_t MPIDIG_global;

int MPIDIG_am_reg_cb(int handler_id,
                     MPIDIG_am_origin_cb origin_cb, MPIDIG_am_target_msg_cb target_msg_cb);
int MPIDIG_init(MPIR_Comm * comm_world, MPIR_Comm * comm_self, int n_vnis);
void MPIDIG_finalize(void);

#endif /* MPIDIG_H_INCLUDED */
