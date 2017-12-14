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
#ifndef MPIR_COLL_SCHED_DB_H_INCLUDED
#define MPIR_COLL_SCHED_DB_H_INCLUDED

#include "uthash.h"
#include "mpir_func.h"

/* enumerator for all the collective operations */
enum {
    BCAST = 0,
    REDUCE,
    ALLREDUCE,
    SCATTER,
    GATHER,
    ALLGATHER,
    ALLTOALL,
    ALLTOALLV,
    ALLTOALLW,
    BARRIER,
    REDUCESCATTER,
    SCAN,
    EXSCAN,
    GATHERV,
    ALLGATHERV,
    SCATTERV
};

/* function pointer to free a schedule */
typedef void (*MPIR_COLL_sched_free_fn) (void *);

/* schedule entry in the database */
typedef struct sched_entry {
    void *sched;                /* pointer to the schedule */
    UT_hash_handle handle;  /* hash handle that makes this structure hashable */
    int size;                   /* Storage for variable-len key */
    char arg[0];
} MPIR_COLL_sched_entry_t;


/* function to add schedule to the database */
MPL_STATIC_INLINE_PREFIX void MPIR_COLL_add_sched(MPIR_COLL_sched_entry_t ** table, void *coll_args, int size, void *sched)
{
    MPIR_COLL_sched_entry_t *s = NULL;
    HASH_FIND(handle, *table, coll_args, size, s);
    if (s == NULL) {
        s = (MPIR_COLL_sched_entry_t *) MPL_calloc(sizeof(MPIR_COLL_sched_entry_t) + size, 1, MPL_MEM_COLL);
        memcpy(s->arg, (char *) coll_args, size);
        s->size = size;
        s->sched = sched;
        HASH_ADD(handle, *table, arg, size, s, MPL_MEM_COLL);
    }
}

/* function to retrieve schedule from a database */
MPL_STATIC_INLINE_PREFIX void *MPIR_COLL_get_sched(MPIR_COLL_sched_entry_t * table, void *coll_args, int size)
{
    MPIR_COLL_sched_entry_t *s = NULL;
    HASH_FIND(handle, table, coll_args, size, s);
    if (s)
        return s->sched;
    else
        return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_delete_sched_table
/* function to delete all schedule entries in a table */
MPL_STATIC_INLINE_PREFIX void MPIR_COLL_delete_sched_table(MPIR_COLL_sched_entry_t * table, MPIR_COLL_sched_free_fn free_fn)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_DELETE_SCHED_TABLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_DELETE_SCHED_TABLE);

    MPIR_COLL_sched_entry_t *current_sched, *tmp;
    current_sched = tmp = NULL;

    HASH_ITER(handle, table, current_sched, tmp) {
        HASH_DELETE(handle, table, current_sched);  /* delete; MPIR_COLL_sched_table advances to next */
        free_fn(current_sched->sched);  /* frees any memory associated with the schedule
                                         * and then the schedule itself */
        MPL_free(current_sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_DELETE_SCHED_TABLE);
}

#endif /* MPIR_COLL_SCHED_DB_H_INCLUDED */
