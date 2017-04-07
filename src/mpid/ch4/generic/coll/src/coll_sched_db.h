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
    MPICH_
};

typedef void (*sched_free_fn)(void*);

typedef union {
        MPIDI_COLL_MPICH_KARY_args_t mpich_kary;
        MPIDI_COLL_MPICH_KNOMIAL_args_t mpich_knomial;
        MPIDI_COLL_MPICH_DISSEM_args_t mpich_dissem;
        MPIDI_COLL_MPICH_RECEXCH_args_t mpich_recexch;
        MPIDI_COLL_STUB_KARY_args_t stub_kary;
        MPIDI_COLL_STUB_KNOMIAL_args_t stub_knomial;
        MPIDI_COLL_STUB_DISSEM_args_t stub_dissem;
        MPIDI_COLL_STUB_RECEXCH_args_t stub_recexch;
} coll_args_t;

typedef struct sched_entry{
    coll_args_t coll_args; /*key*/
    void *sched;           /*pointer to the schedule*/
    sched_free_fn free_fn;    /*function pointer for freeing the schedule*/
    MPL_UT_hash_handle hh; /*hash handle that makes this structure hashable*/
} sched_entry_t;

extern sched_entry_t *sched_table;      /*this is the hash table pointer*/

static inline void add_sched(coll_args_t coll_args, void *sched, sched_free_fn free_fn){
    sched_entry_t *s;
    if(0) fprintf(stderr,"adding sched to sched_table\n");
    MPL_HASH_FIND(hh, sched_table, &coll_args, sizeof(coll_args), s);
    if(s==NULL){
        s = (sched_entry_t*) MPL_malloc(sizeof(sched_entry_t));
        s->coll_args = coll_args;
        MPL_HASH_ADD(hh,sched_table,coll_args,sizeof(coll_args),s);
    }
    s->sched = sched;
    s->free_fn = free_fn;
}

static inline void* get_sched(coll_args_t coll_args){
    sched_entry_t* s;
    MPL_HASH_FIND(hh, sched_table, &coll_args, sizeof(coll_args), s);
    if(s) return s->sched;
    else return NULL;
}

static inline void delete_sched_table(){
    if(0) fprintf(stderr, "deleting collective schedules\n");
    sched_entry_t *current_sched, *tmp;
    //MPIR_Assert(sched_table!=NULL);
    int num_users = MPL_HASH_COUNT(sched_table);
    if(0) fprintf(stderr,"there are %d users\n", num_users);
    MPL_HASH_ITER(hh, sched_table, current_sched, tmp) {
        if(0) fprintf(stderr, "deleting a sched from sched_table\n");
        MPL_HASH_DEL(sched_table, current_sched);         /* delete; sched_table advances to next */
        current_sched->free_fn(current_sched->sched); /*frees any memory associated with the schedule
                                                        and then the schedule itself */
        MPL_free(current_sched);
  }
}

#endif /* MPIDI_COLL_SCHED_DB_H_INCLUDED */
