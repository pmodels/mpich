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
#include "sys/queue.h"
#define MPIC_FLAG_REDUCE_L 1
#define MPIC_FLAG_REDUCE_R 0

struct MPIR_Comm;
struct MPIR_Op;

#define COLL_Assert MPIR_Assert
#define COLL_queue_t  MPIC_queue_t
#define COLL_queue_elem_t MPIC_queue_elem_t
#define COLL_progress_global MPIC_progress_global

#define MPIC_DT_DECL MPIC_dt_t ch4_coll;
#define MPIC_OP_DECL MPIC_op_t ch4_coll;
#define MPIC_COMM_DECL MPIC_comm_t ch4_coll;
#define MPIC_REQ_DECL MPIC_req_t ch4_coll;

#define MPIC_COMM(comm) (&((comm)->ch4_coll))
#define MPIC_REQ(req) (&((req)->ch4_coll))

#ifdef MPIC_DEBUG
#define MPIC_DBG(...) MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,__VA_ARGS__))
#else
#define MPIC_DBG(...)
#endif

typedef struct COLL_queue_elem_t {
    TAILQ_ENTRY(COLL_queue_elem_t) list_data;
    int (*kick_fn) (struct COLL_queue_elem_t *);
} COLL_queue_elem_t;

typedef struct MPIC_progress_global_t {
    TAILQ_HEAD(COLL_queue_t, COLL_queue_elem_t) head;
    int (*progress_fn) (void);
    int do_progress;
} MPIC_progress_global_t;


/* Coll 'pre'-definitions */
#include "../mpi/coll/include/types_decl.h"

/* Generic datatypes */

typedef struct {
    int use_tag;
    int issued_collectives;
    MPIC_STUB_KARY_comm_t stub_kary;
    MPIC_STUB_KNOMIAL_comm_t stub_knomial;
    MPIC_MPICH_KARY_comm_t mpich_kary;
    MPIC_MPICH_KNOMIAL_comm_t mpich_knomial;
} MPIC_comm_t;

typedef struct {
    MPIC_STUB_global_t tsp_stub;
    MPIC_MPICH_global_t tsp_mpich;

    MPIC_STUB_KARY_global_t stub_kary;
    MPIC_STUB_KNOMIAL_global_t stub_knomial;
    MPIC_MPICH_KARY_global_t mpich_kary;
    MPIC_MPICH_KNOMIAL_global_t mpich_knomial;
} MPIC_global_t;

typedef struct {
} MPIC_op_t;

typedef struct {
} MPIC_dt_t;


extern MPIC_global_t MPIC_global_instance;

typedef union {
    MPIC_STUB_KARY_req_t stub_kary;
    MPIC_STUB_KNOMIAL_req_t stub_knomial;
    MPIC_MPICH_KARY_req_t mpich_kary;
    MPIC_MPICH_KNOMIAL_req_t mpich_knomial;
} MPIC_req_t;


#include "../mpi/coll/src/coll_sched_db.h"

#endif /* MPIDI_CH4_PRE_H_INCLUDED */
