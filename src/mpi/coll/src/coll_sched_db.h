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
#ifndef MPIC_SCHED_DB_H_INCLUDED
#define MPIC_SCHED_DB_H_INCLUDED
#include "mpl_uthash.h"
/*enumerator for all the collective operations*/
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

/*enumerator for all the collective algorithm types*/
enum{
    STUB_,
    TREE_,
    KARY_,
    KNOMIAL_,
    RECEXCH_,
    DISSEM_,
    RING_,
    TREEBASIC_
};

/*enumerator for collective transports*/
enum{
    MPICH_,
    BMPICH_
};

typedef void (*MPIC_sched_free_fn)(void*);

typedef union {
        MPIC_MPICH_KARY_args_t mpich_kary;
        MPIC_MPICH_KNOMIAL_args_t mpich_knomial;
        MPIC_MPICH_DISSEM_args_t mpich_dissem;
        MPIC_MPICH_RECEXCH_args_t mpich_recexch;
        MPIC_BMPICH_KARY_args_t bmpich_kary;
        MPIC_BMPICH_KNOMIAL_args_t bmpich_knomial;
        MPIC_STUB_KARY_args_t stub_kary;
        MPIC_STUB_KNOMIAL_args_t stub_knomial;
        MPIC_STUB_DISSEM_args_t stub_dissem;
        MPIC_STUB_RECEXCH_args_t stub_recexch;
} MPIC_coll_args_t;

typedef struct sched_entry{
    MPIC_coll_args_t coll_args; /*key*/
    void *sched;           /*pointer to the schedule*/
    MPIC_sched_free_fn free_fn;    /*function pointer for freeing the schedule*/
    MPL_UT_hash_handle hh; /*hash handle that makes this structure hashable*/
} MPIC_sched_entry_t;

extern MPIC_sched_entry_t *MPIC_sched_table;      /*this is the hash table pointer*/

static inline void MPIC_add_sched(MPIC_coll_args_t coll_args, void *sched, MPIC_sched_free_fn free_fn){
    MPIC_sched_entry_t *s;
    if(0) fprintf(stderr,"adding sched to MPIC_sched_table\n");
    MPL_HASH_FIND(hh, MPIC_sched_table, &coll_args, sizeof(coll_args), s);
    if(s==NULL){
        s = (MPIC_sched_entry_t*) MPL_malloc(sizeof(MPIC_sched_entry_t));
        s->coll_args = coll_args;
        MPL_HASH_ADD(hh,MPIC_sched_table,coll_args,sizeof(coll_args),s);
    }
    s->sched = sched;
    s->free_fn = free_fn;
}

static inline void* MPIC_get_sched(MPIC_coll_args_t coll_args){
    MPIC_sched_entry_t* s;
    MPL_HASH_FIND(hh, MPIC_sched_table, &coll_args, sizeof(coll_args), s);
    if(s) return s->sched;
    else return NULL;
}

static inline void MPIC_delete_sched_table(){
    if(0) fprintf(stderr, "deleting collective schedules\n");
    MPIC_sched_entry_t *current_sched, *tmp;
    //MPIR_Assert(MPIC_sched_table!=NULL);
    int num_users = MPL_HASH_COUNT(MPIC_sched_table);
    if(0) fprintf(stderr,"there are %d users\n", num_users);
    MPL_HASH_ITER(hh, MPIC_sched_table, current_sched, tmp) {
        if(0) fprintf(stderr, "deleting a sched from MPIC_sched_table\n");
        MPL_HASH_DEL(MPIC_sched_table, current_sched);         /* delete; MPIC_sched_table advances to next */
        current_sched->free_fn(current_sched->sched); /*frees any memory associated with the schedule
                                                        and then the schedule itself */
        MPL_free(current_sched);
  }
}

#endif /* MPIC_SCHED_DB_H_INCLUDED */
