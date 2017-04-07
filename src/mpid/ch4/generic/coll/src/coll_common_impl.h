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

#ifndef COLL_NAMESPACE
#error "The tree template must be namespaced with COLL_NAMESPACE"
#endif

#ifndef COLL_SCHED_MACROS
#define COLL_SCHED_MACROS

#define SCHED_FOREACHCHILD()                    \
    for(i=0; i<tree->numRanges; i++)            \
        for(j=tree->children[i].startRank;      \
            j<=tree->children[i].endRank;       \
            j++)                                \

#define SCHED_FOREACHCHILDDO(stmt)              \
    ({                                          \
        SCHED_FOREACHCHILD()                    \
        {                                       \
            stmt;                               \
        }                                       \
    })

#endif /* COLL_MACROS */

static inline void COLL_sched_init(COLL_sched_t * s)
{
    TSP_sched_init(&(s->tsp_sched));
}

static inline void COLL_sched_reset(COLL_sched_t *s){
    TSP_sched_reset(&s->tsp_sched);
}

static inline void COLL_sched_free(COLL_sched_t *s){
    if(0) fprintf(stderr,"freeing sched buffers\n");
    TSP_free_buffers(&s->tsp_sched);
    TSP_free_mem(s);
}

static inline int COLL_kick(COLL_queue_elem_t * elem);
static inline void COLL_sched_init_nb(COLL_sched_t ** sched, COLL_req_t * request)
{
    COLL_sched_t *s;
    int size = sizeof(*s);
    s = request->phases = (COLL_sched_t *) TSP_allocate_mem(size);
    memset(s, 0, size);
    TSP_sched_init(&(s->tsp_sched));
    request->elem.kick_fn = COLL_kick;
    *sched = s;
}

static inline void COLL_sched_kick(COLL_sched_t * s)
{
    int rc = 0;

    if (!s->sched_started) {
        TSP_sched_start(&(s->tsp_sched));
        s->sched_started = 1;
    }

    while (!rc) {
        rc = TSP_test(&(s->tsp_sched));

        if (rc)
            TSP_sched_finalize(&(s->tsp_sched));
        else
            MPIDI_COLL_progress_global.progress_fn();
    }
}

static inline int COLL_sched_kick_nb(COLL_sched_t * s)
{
    int rc;

    if (!s->sched_started) {
        TSP_sched_start(&(s->tsp_sched));
        s->sched_started = 1;
    }

    rc = TSP_test(&(s->tsp_sched));
    return rc;
}
