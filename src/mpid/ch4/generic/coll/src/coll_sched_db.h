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
#ifndef MPIDI_COLL_SCHED_DB_H_INCLUDED
#define MPIDI_COLL_SCHED_DB_H_INCLUDED

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

#endif /* MPIDI_COLL_SCHED_DB_H_INCLUDED */
