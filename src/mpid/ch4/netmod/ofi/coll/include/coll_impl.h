/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef NETMOD_OFI_MPICH_COLL_IMPL_H_INCLUDED
#define NETMOD_OFI_MPICH_COLL_IMPL_H_INCLUDED

#include <sys/queue.h>

#define COLL_queue_elem_t      MPIDI_OFI_COLL_queue_elem_t
#define COLL_progress_global   MPIDI_OFI_COLL_global

typedef struct MPIDI_OFI_COLL_queue_elem_t {
    TAILQ_ENTRY(MPIDI_OFI_COLL_queue_elem_t) list_data;
    int (*kick_fn)(struct MPIDI_OFI_COLL_queue_elem_t *);
} MPIDI_OFI_COLL_queue_elem_t;

typedef struct MPIDI_OFI_COLL_global_t {
    TAILQ_HEAD(MPIDI_OFI_COLL_queue_t,MPIDI_OFI_COLL_queue_elem_t) head;
    int (*progress_fn)(void);
    int do_progress;
} MPIDI_OFI_COLL_global_t;

extern MPIDI_OFI_COLL_global_t MPIDI_OFI_COLL_global;


static inline int MPIDI_OFI_COLL_Progress(int n, void *cq[])
{
    int i = 0, done;
    MPIDI_OFI_COLL_queue_elem_t  *s = MPIDI_OFI_COLL_global.head.tqh_first;

    for(s = MPIDI_OFI_COLL_global.head.tqh_first; s != NULL;) {
        MPIDI_OFI_COLL_queue_elem_t *next = s->list_data.tqe_next;
        done = s->kick_fn(s);

        if(done) {
            cq[i++] = (void *) s;

            if(i == n) break;
        }

        s = next;
    }

    return i;
}



#endif /* NETMOD_OFI_MPICH_COLL_IMPL_H_INCLUDED */
