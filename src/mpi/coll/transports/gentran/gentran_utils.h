/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GENTRAN_UTILS_H_INCLUDED
#define GENTRAN_UTILS_H_INCLUDED

#include "mpiimpl.h"
#include "gentran_types.h"

extern MPII_Coll_queue_t MPII_coll_queue;
extern int MPII_Genutil_progress_hook_id;

static inline int *MPII_Genutil_vtx_list_data(const MPII_Genutil_vtx_list_t * l)
{
    return l->cap ? l->heap : (int *) l->inl;
}

static inline void MPII_Genutil_vtx_list_init(MPII_Genutil_vtx_list_t * l)
{
    l->n = 0;
    l->cap = 0;
    l->heap = NULL;
}

static inline void MPII_Genutil_vtx_list_push(MPII_Genutil_vtx_list_t * l, int id)
{
    if (l->cap == 0 && l->n < MPII_GENUTIL_VTX_LIST_INLINE) {
        l->inl[l->n++] = id;
        return;
    }
    if (l->cap == 0) {
        l->cap = 2 * MPII_GENUTIL_VTX_LIST_INLINE;
        l->heap = MPL_malloc(l->cap * sizeof(int), MPL_MEM_COLL);
        MPIR_Assert(l->heap);
        memcpy(l->heap, l->inl, l->n * sizeof(int));
    } else if (l->n == l->cap) {
        l->cap *= 2;
        l->heap = MPL_realloc(l->heap, l->cap * sizeof(int), MPL_MEM_COLL);
        MPIR_Assert(l->heap);
    }
    l->heap[l->n++] = id;
}

static inline void MPII_Genutil_vtx_list_free(MPII_Genutil_vtx_list_t * l)
{
    MPL_free(l->heap);
}

/* vertex copy function, required by utarray */
void MPII_Genutil_vtx_copy(void *_dst, const void *_src);

/* vertex destructor, required by utarray */
void MPII_Genutil_vtx_dtor(void *_elt);

/* Function to add incoming vertices of a vertex.  This vertex sets
 * the incoming vertices to vtx and also adds vtx to the outgoing
 * vertex list of the vertives in in_vtcs.  NOTE: This function should
 * only be called when a new vertex is added to the groph */
void MPII_Genutil_vtx_add_dependencies(MPII_Genutil_sched_t * sched, int vtx_id,
                                       int n_in_vtcs, int *in_vtcs);

/* Function to get a new vertex in the graph */
int MPII_Genutil_vtx_create(MPII_Genutil_sched_t * sched, MPII_Genutil_vtx_t ** vtx);

/* Functions to start the schedule, and to make progress on it */
int MPII_Genutil_sched_start(MPII_Genutil_sched_t * sched);
int MPII_Genutil_sched_poke(MPII_Genutil_sched_t * sched, int *is_complete, int *made_progress);
int MPII_Genutil_sched_recycle(MPII_Genutil_sched_t * sched);
void MPII_Genutil_sched_cache_free(void);

/* Hook to make progress on nonblocking collective operations  */
int MPII_Genutil_progress_hook(int vci, int *made_progress);

#endif /* GENTRAN_UTILS_H_INCLUDED */
