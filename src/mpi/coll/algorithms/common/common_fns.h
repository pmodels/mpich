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

/*FIXME: COLL_progress_global should be per-VNI */
extern MPIC_progress_global_t MPIC_progress_global;

static inline int COLL_kick(COLL_queue_elem_t * elem)
{
    int done = 0;
    TSP_sched_t *s = ((COLL_req_t *) elem)->phases;
    done = TSP_test(s);

    if (done) {
        TAILQ_REMOVE(&COLL_progress_global.head, elem, list_data);
        TSP_sched_finalize(s);
        ((COLL_req_t *) elem)->phases = NULL;
    }

    return done;
}

static inline void COLL_sched_kick(TSP_sched_t * s)
{
    int rc = 0;

    TSP_sched_start(s);

    while (!rc) {
        rc = TSP_test(s);
        if (!rc) {
            MPIC_progress_global.progress_fn();
        }
    }
    /* Mark sched as completed,
     * release of schedule in case of disabled caching*/
    TSP_sched_finalize(s);
}

static inline int COLL_sched_kick_nb(TSP_sched_t * s, COLL_req_t * request)
{
    int rc;

    TSP_sched_start(s);
    /* Enqueue nonblocking collective. */
    request->phases = s;
    request->elem.kick_fn = COLL_kick;

    /*FIXME: COLL_progress_global should be per-VNI*/
    TAILQ_INSERT_TAIL(&COLL_progress_global.head, &request->elem,list_data);

    rc = TSP_test(s);

    return rc;
}
