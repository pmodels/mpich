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
#error "The template must be namespaced with COLL_NAMESPACE"
#endif

/*FIXME: MPIC_progress_global should be per-VNI */
extern MPIC_progress_global_t MPIC_progress_global;

#undef FUNCNAME
#define FUNCNAME COLL_sched_wait
/* Routing to kick-start and completely execute a blocking schedule*/
MPL_STATIC_INLINE_PREFIX void COLL_sched_wait(TSP_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_WAIT);

    int rc = 0;

    while (!rc) {
        rc = TSP_test(sched);   /* Make progress on the collective schedule */
        if (!rc) {
            MPIC_progress_global.progress_fn();
        }
    }

    /* Mark sched as completed */
    TSP_sched_finalize(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_WAIT);
}

#undef FUNCNAME
#define FUNCNAME COLL_kick_nb
/* Function to make progress on a non-blocking collective*/
MPL_STATIC_INLINE_PREFIX int COLL_kick_nb(MPIC_queue_elem_t * elem)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_KICK_NB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_KICK_NB);

    int done = 0;
    TSP_sched_t *sched = (TSP_sched_t *) (((COLL_req_t *) elem)->sched);

    /* make progress on the collective */
    done = TSP_test(sched);

    if (done) {
        /* remove the collective from the queue */
        TAILQ_REMOVE(&MPIC_progress_global.head, elem, list_data);

        /* delete all memory associated with the schedule */
        TSP_sched_finalize(sched);
        ((COLL_req_t *) elem)->sched = NULL;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_KICK_NB);

    return done;
}

#undef FUNCNAME
#define FUNCNAME COLL_sched_test
/* Routing to kick-start a non-blocking schedule*/
MPL_STATIC_INLINE_PREFIX int COLL_sched_test(TSP_sched_t * sched, COLL_req_t * request)
{
    int rc;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_TEST);

    /* Enqueue nonblocking collective */
    request->sched = (void *) sched;
    /* function to make progress on the collective */
    request->elem.kick_fn = COLL_kick_nb;

    /* FIXME: MPIC_progress_global should be per-VNI */
    TAILQ_INSERT_TAIL(&MPIC_progress_global.head, &request->elem, list_data);

    /* Make some progress now */
    rc = TSP_test(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_TEST);

    return rc;
}
