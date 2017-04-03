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

enum{
    BCAST=0,
    REDUCE,
    ALLREDUCE,
    SCATTER,
    GATHER,
    ALLGATHER,
    ALLTOALL,
    BARRIER,
    REDUCESCATTER,
    SCAN,
    EXSCAN,
    GATHERV,
    ALLGATHERV,
    SCATTERV,
    ALLTOALLV
};

typedef struct {
    int coll_type; /*collective type id based on the enum above*/
    union{/*store collective arguments based on the collective type*/
        struct {
            void *buf; int count; int dt_id; int root; int comm_id;
        } bcast;
        struct {
            void *sbuf; void* rbuf; int count; int dt_id; int op_id; int root; int comm_id;    
        } reduce;
        struct {
            void *sbuf; void *rbuf; int count; int dt_id; int op_id; int comm_id;
        }allreduce;
    }args;
} coll_args_t;

typedef struct sched_entry{
    coll_args_t coll_args; /*key*/
    void *sched;           /*pointer to the schedule*/
    MPL_UT_hash_handle hh; /*hash handle that makes this structure hashable*/
} sched_entry_t;

extern sched_entry_t *sched_table;      /*this is the hash table pointer*/

static inline void add_sched(coll_args_t coll_args, void *sched){
    sched_entry_t *s;
    MPL_HASH_FIND(hh, sched_table, &coll_args, sizeof(coll_args), s);
    if(s==NULL){
        s = (sched_entry_t*) MPL_malloc(sizeof(sched_entry_t));
        s->coll_args = coll_args;
        MPL_HASH_ADD(hh,sched_table,coll_args,sizeof(coll_args),s);
    }
    s->sched = sched;
}

static inline void* get_sched(coll_args_t coll_args){
    sched_entry_t* s;
    MPL_HASH_FIND(hh, sched_table, &coll_args, sizeof(coll_args), s);
    if(s) return s->sched;
    else return NULL;
}

#include "../src/coll_progress_impl.h"

/* Coll 'pre'-definitions */
#include "./coll_pre.h"

/* Generic datatypes (generated file) */
#include "./coll_types.h"

#endif /* MPIDI_CH4_PRE_H_INCLUDED */
