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
#ifndef MPIDI_COLL_PROGRESS_IMPL_H_INCLUDED
#define MPIDI_COLL_PROGRESS_IMPL_H_INCLUDED

#include <sys/queue.h>

typedef struct COLL_queue_elem_t {
    TAILQ_ENTRY(COLL_queue_elem_t) list_data;
    int (*kick_fn) (struct COLL_queue_elem_t *);
} COLL_queue_elem_t;

typedef struct MPIDI_COLL_global_t {
    TAILQ_HEAD(COLL_queue_t, COLL_queue_elem_t) head;
    int (*progress_fn) (void);
    int do_progress;
} MPIDI_COLL_progress_global_t;

extern MPIDI_COLL_progress_global_t MPIDI_COLL_progress_global;


static inline int MPIDI_COLL_Progress(int n, void *cq[])
{
    int i = 0;
    COLL_queue_elem_t *s = MPIDI_COLL_progress_global.head.tqh_first;
    for ( ; ((s != NULL) && (i < n)); s = s->list_data.tqe_next) {
        if(s->kick_fn(s))
            cq[i++] = (void *) s;
    }
    return i;
}

#endif /* MPIDI_COLL_PROGRESS_IMPL_H_INCLUDED */
