/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef GENTRAN_UTILS_H_INCLUDED
#define GENTRAN_UTILS_H_INCLUDED

#include "mpiimpl.h"
#include "tsp_gentran_types.h"
#include "tsp_gentran.h"
#include "utarray.h"

extern MPII_Coll_queue_t coll_queue;
extern int MPII_Genutil_progress_hook_id;

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

/* Function to make progress on the schedule */
int MPII_Genutil_sched_poke(MPII_Genutil_sched_t * sched, int *is_complete, int *made_progress);

/* Hook to make progress on nonblocking collective operations  */
int MPII_Genutil_progress_hook(int *);

#endif /* GENTRAN_UTILS_H_INCLUDED */
