/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_RECVQ_H_INCLUDED
#define CH4R_RECVQ_H_INCLUDED

#include <mpidimpl.h>
#include "mpidig.h"
#include "utlist.h"
#include "ch4_impl.h"

extern unsigned PVAR_LEVEL_posted_recvq_length ATTRIBUTE((unused));
extern unsigned PVAR_LEVEL_unexpected_recvq_length ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_posted_recvq_match_attempts ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_unexpected_recvq_match_attempts ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_time_failed_matching_postedq ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_time_matching_unexpectedq ATTRIBUTE((unused));

int MPIDIG_recvq_init(void);

MPL_STATIC_INLINE_PREFIX int MPIDIG_match_posted(int rank, int tag,
                                                 MPIR_Context_id_t context_id, MPIR_Request * req)
{
    return (rank == MPIDIG_REQUEST(req, rank) || MPIDIG_REQUEST(req, rank) == MPI_ANY_SOURCE) &&
        (tag == MPIR_TAG_MASK_ERROR_BITS(MPIDIG_REQUEST(req, tag)) ||
         MPIDIG_REQUEST(req, tag) == MPI_ANY_TAG) && context_id == MPIDIG_REQUEST(req, context_id);
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_match_unexp(int rank, int tag,
                                                MPIR_Context_id_t context_id, MPIR_Request * req)
{
    return (rank == MPIDIG_REQUEST(req, rank) || rank == MPI_ANY_SOURCE) &&
        (tag == MPIR_TAG_MASK_ERROR_BITS(MPIDIG_REQUEST(req, tag)) ||
         tag == MPI_ANY_TAG) && context_id == MPIDIG_REQUEST(req, context_id);
}

#ifdef MPIDI_CH4U_USE_PER_COMM_QUEUE

MPL_STATIC_INLINE_PREFIX void MPIDIG_enqueue_posted(MPIR_Request * req, MPIDIG_rreq_t ** list)
{
    MPIDIG_REQUEST(req, req->rreq.request) = req;
    DL_APPEND(*list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, posted_recvq_length, 1);

}

MPL_STATIC_INLINE_PREFIX void MPIDIG_enqueue_unexp(MPIR_Request * req, MPIDIG_rreq_t ** list)
{
    MPIDIG_REQUEST(req, req->rreq.request) = req;
    DL_APPEND(*list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_length, 1);

}

MPL_STATIC_INLINE_PREFIX void MPIDIG_delete_unexp(MPIR_Request * req, MPIDIG_rreq_t ** list)
{
    DL_DELETE(*list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);

}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_dequeue_unexp_strict(int rank, int tag,
                                                                   MPIR_Context_id_t context_id,
                                                                   MPIDIG_rreq_t ** list)
{
    MPIDIG_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = curr->request;
        if (!(MPIDIG_REQUEST(req, req->status) & MPIDIG_REQ_BUSY) &&
            MPIDIG_match_unexp(rank, tag, context_id, req)) {
            DL_DELETE(*list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_dequeue_unexp(int rank, int tag,
                                                            MPIR_Context_id_t context_id,
                                                            MPIDIG_rreq_t ** list)
{
    MPIDIG_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = curr->request;
        if (MPIDIG_match_unexp(rank, tag, context_id, req)) {
            DL_DELETE(*list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_find_unexp(int rank, int tag,
                                                         MPIR_Context_id_t context_id,
                                                         MPIDIG_rreq_t ** list)
{
    MPIDIG_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = curr->request;
        if (MPIDIG_match_unexp(rank, tag, context_id, req)) {
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_dequeue_posted(int rank, int tag,
                                                             MPIR_Context_id_t context_id,
                                                             MPIDIG_rreq_t ** list)
{
    MPIR_Request *req = NULL;
    MPIDIG_rreq_t *curr, *tmp;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        req = curr->request;
        if (MPIDIG_match_posted(rank, tag, context_id, req)) {
            DL_DELETE(*list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    if (!req)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_delete_posted(MPIDIG_rreq_t * req, MPIDIG_rreq_t ** list)
{
    int found = 0;
    MPIDIG_rreq_t *curr, *tmp;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        if (curr == req) {
            DL_DELETE(*list, curr);
            found = 1;
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
    }
    if (!found)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    return found;
}

#else /* #ifdef MPIDI_CH4U_USE_PER_COMM_QUEUE */

/* Use global queue */

MPL_STATIC_INLINE_PREFIX void MPIDIG_enqueue_posted(MPIR_Request * req, MPIDIG_rreq_t ** list)
{
    MPIDIG_REQUEST(req, req->rreq.request) = req;
    DL_APPEND(MPIDI_global.posted_list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, posted_recvq_length, 1);

}

MPL_STATIC_INLINE_PREFIX void MPIDIG_enqueue_unexp(MPIR_Request * req, MPIDIG_rreq_t ** list)
{
    MPIDIG_REQUEST(req, req->rreq.request) = req;
    DL_APPEND(MPIDI_global.unexp_list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_length, 1);

}

MPL_STATIC_INLINE_PREFIX void MPIDIG_delete_unexp(MPIR_Request * req, MPIDIG_rreq_t ** list)
{
    DL_DELETE(MPIDI_global.unexp_list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);

}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_dequeue_unexp_strict(int rank, int tag,
                                                                   MPIR_Context_id_t context_id,
                                                                   MPIDIG_rreq_t ** list)
{
    MPIDIG_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    DL_FOREACH_SAFE(MPIDI_global.unexp_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if (!(MPIDIG_REQUEST(req, req->status) & MPIDIG_REQ_BUSY) &&
            MPIDIG_match_unexp(rank, tag, context_id, req)) {
            DL_DELETE(MPIDI_global.unexp_list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_dequeue_unexp(int rank, int tag,
                                                            MPIR_Context_id_t context_id,
                                                            MPIDIG_rreq_t ** list)
{
    MPIDIG_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    DL_FOREACH_SAFE(MPIDI_global.unexp_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if (MPIDIG_match_unexp(rank, tag, context_id, req)) {
            DL_DELETE(MPIDI_global.unexp_list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_find_unexp(int rank, int tag,
                                                         MPIR_Context_id_t context_id,
                                                         MPIDIG_rreq_t ** list)
{
    MPIDIG_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    DL_FOREACH_SAFE(MPIDI_global.unexp_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if (MPIDIG_match_unexp(rank, tag, context_id, req)) {
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_dequeue_posted(int rank, int tag,
                                                             MPIR_Context_id_t context_id,
                                                             MPIDIG_rreq_t ** list)
{
    MPIR_Request *req = NULL;
    MPIDIG_rreq_t *curr, *tmp;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    DL_FOREACH_SAFE(MPIDI_global.posted_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if (MPIDIG_match_posted(rank, tag, context_id, req)) {
            DL_DELETE(MPIDI_global.posted_list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    if (!req)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    return req;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_delete_posted(MPIDIG_rreq_t * req, MPIDIG_rreq_t ** list)
{
    int found = 0;
    MPIDIG_rreq_t *curr, *tmp;

    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    DL_FOREACH_SAFE(MPIDI_global.posted_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        if (curr == req) {
            DL_DELETE(MPIDI_global.posted_list, curr);
            found = 1;
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
    }
    if (!found)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    return found;
}

#endif /* MPIDI_CH4U_USE_PER_COMM_QUEUE */

#endif /* CH4R_RECVQ_H_INCLUDED */
