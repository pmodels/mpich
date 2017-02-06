/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPIDI_CH4_COLL_PRE_H_INCLUDED
#define MPIDI_CH4_COLL_PRE_H_INCLUDED

struct MPIR_Comm;
struct MPIR_Op;

#define COLL_Assert MPIR_Assert
#define COLL_queue_t  MPIDI_COLL_queue_t
#define COLL_queue_elem_t MPIDI_COLL_queue_elem_t
#define COLL_progress_global MPIDI_COLL_progress_global

#define MPIDI_COLL_COMM_DECL MPIDI_COLL_comm_t ch4_coll
#define MPIDI_COLL_DT_DECL MPIDI_COLL_dt_t ch4_coll
#define MPIDI_COLL_OP_DECL MPIDI_COLL_op_t ch4_coll
#define MPIDI_COLL_REQ_DECL MPIDI_COLL_req_t ch4_coll

#define MPIDI_COLL_COMM(comm) (&((comm)->dev.ch4.ch4_coll))
#define MPIDI_COLL_DT(dt) (&((dt)->dev.ch4_coll))
#define MPIDI_COLL_OP(op) (&((op)->dev.ch4_coll))
#define MPIDI_COLL_REQ(req) (&((req)->dev.ch4_coll))

#include "../src/coll_progress_impl.h"

/* Coll 'pre'-definitions */
#include "./coll_pre.h"

/* Generic datatypes (generated file) */
#include "./coll_types.h"

#endif /* MPIDI_CH4_PRE_H_INCLUDED */
