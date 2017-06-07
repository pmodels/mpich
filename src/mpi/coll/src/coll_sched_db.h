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
enum {
    BCAST = 0,
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
enum {
    KARY_,
    KNOMIAL_,
};

/*enumerator for collective transports*/
enum {
    MPICH_,
};

typedef void (*MPIC_sched_free_fn) (void *);

typedef struct sched_entry {
    void *sched;                /*pointer to the schedule */
    MPL_UT_hash_handle hh;      /*hash handle that makes this structure hashable */
    int size;                   /*Storage for variable-len key*/
    char arg[0];
} MPIC_sched_entry_t;


static inline void MPIC_add_sched(MPIC_sched_entry_t** table, void * coll_args, int size,
                                  void *sched)
{
    MPIC_sched_entry_t *s=NULL;
    MPL_HASH_FIND(hh, *table, coll_args, size, s);
    if (s == NULL) {
        s = (MPIC_sched_entry_t *) MPL_calloc(sizeof(MPIC_sched_entry_t)+size, 1);
        memcpy(s->arg, (char*)coll_args, size);
        s->size = size;
        s->sched = sched;
        MPL_HASH_ADD_KEYPTR(hh, *table, coll_args, size, s);
    }
}

static inline void *MPIC_get_sched(MPIC_sched_entry_t* table, void * coll_args, int size)
{
    MPIC_sched_entry_t *s;
    MPL_HASH_FIND(hh, table, coll_args, size, s);
    if (s)
        return s->sched;
    else
        return NULL;
}


static inline void MPIC_delete_sched_table(MPIC_sched_entry_t* table, MPIC_sched_free_fn free_fn)
{
    MPIC_sched_entry_t *current_sched, *tmp;
    int num_users = MPL_HASH_COUNT(table);

    MPL_HASH_ITER(hh, table, current_sched, tmp) {
        MPL_HASH_DEL(table, current_sched);  /* delete; MPIC_sched_table advances to next */
        free_fn(current_sched->sched);   /*frees any memory associated with the schedule
                                          * and then the schedule itself */
        MPL_free(current_sched);
    }
}

#endif /* MPIC_SCHED_DB_H_INCLUDED */
