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
#include "mpl_utlist.h"
#include "ch4_impl.h"

static unsigned PVAR_LEVEL_posted_recvq_length ATTRIBUTE((unused));
static unsigned PVAR_LEVEL_unexpected_recvq_length ATTRIBUTE((unused));
static unsigned long long PVAR_COUNTER_posted_recvq_match_attempts ATTRIBUTE((unused));
static unsigned long long PVAR_COUNTER_unexpected_recvq_match_attempts ATTRIBUTE((unused));
static MPIR_T_pvar_timer_t PVAR_TIMER_time_failed_matching_postedq ATTRIBUTE((unused));
static MPIR_T_pvar_timer_t PVAR_TIMER_time_matching_unexpectedq ATTRIBUTE((unused));

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_Recvq_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_Recvq_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(
        RECVQ,
        MPI_UNSIGNED,
        posted_recvq_length,
        0, /* init value */
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
        "CH4", /* category name */
        "length of the posted message receive queue");

    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(
        RECVQ,
        MPI_UNSIGNED,
        unexpected_recvq_length,
        0, /* init value */
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
        "CH4", /* category name */
        "length of the unexpected message receive queue");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RECVQ,
        MPI_UNSIGNED_LONG_LONG,
        posted_recvq_match_attempts,
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
        "CH4", /* category name */
        "number of search passes on the posted message receive queue");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RECVQ,
        MPI_UNSIGNED_LONG_LONG,
        unexpected_recvq_match_attempts,
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
        "CH4",
        "number of search passes on the unexpected message receive queue");

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RECVQ,
        MPI_DOUBLE,
        time_failed_matching_postedq,
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
        "CH4", /* category name */
        "total time spent on unsuccessful search passes on the posted receives queue");

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RECVQ,
        MPI_DOUBLE,
        time_matching_unexpectedq,
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
        "CH4", /* category name */
        "total time spent on search passes on the unexpected receive queue");

    return mpi_errno;
}

#ifdef MPIDI_CH4U_USE_PER_COMM_QUEUE

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_enqueue_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_enqueue_posted(MPIR_Request * req,
                                                        MPIDI_CH4U_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_ENQUEUE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_ENQUEUE_POSTED);
    MPIDI_CH4U_REQUEST(req, req->rreq.request) = (uint64_t) req;
    MPL_DL_APPEND(*list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, posted_recvq_length, 1);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_ENQUEUE_POSTED);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_enqueue_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_enqueue_unexp(MPIR_Request * req,
                                                       MPIDI_CH4U_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_ENQUEUE_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_ENQUEUE_UNEXP);
    MPIDI_CH4U_REQUEST(req, req->rreq.request) = (uint64_t) req;
    MPL_DL_APPEND(*list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_length, 1);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_ENQUEUE_UNEXP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_delete_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_delete_unexp(MPIR_Request * req, MPIDI_CH4U_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DELETE_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DELETE_UNEXP);
    MPL_DL_DELETE(*list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DELETE_UNEXP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_dequeue_unexp_strict
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_dequeue_unexp_strict(uint64_t tag,
                                                                       uint64_t ignore,
                                                                       MPIDI_CH4U_rreq_t ** list)
{
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP_STRICT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP_STRICT);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    MPL_DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if (!(MPIDI_CH4U_REQUEST(req, req->status) & MPIDI_CH4U_REQ_BUSY) &&
            ((tag & ~ignore) == (MPIDI_CH4U_REQUEST(req, tag) & ~ignore))) {
            MPL_DL_DELETE(*list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP_STRICT);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_dequeue_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_dequeue_unexp(uint64_t tag, uint64_t ignore,
                                                                MPIDI_CH4U_rreq_t ** list)
{
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    MPL_DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if ((tag & ~ignore) == (MPIDI_CH4U_REQUEST(req, tag) & ~ignore)) {
            MPL_DL_DELETE(*list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_find_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_find_unexp(uint64_t tag, uint64_t ignore,
                                                             MPIDI_CH4U_rreq_t ** list)
{
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_FIND_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_FIND_UNEXP);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    MPL_DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if ((tag & ~ignore) == (MPIDI_CH4U_REQUEST(req, tag) & ~ignore)) {
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_FIND_UNEXP);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_dequeue_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_dequeue_posted(uint64_t tag,
                                                                 MPIDI_CH4U_rreq_t ** list)
{
    MPIR_Request *req = NULL;
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DEQUEUE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DEQUEUE_POSTED);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    MPL_DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if ((tag & ~(MPIDI_CH4U_REQUEST(req, req->rreq.ignore))) ==
            (MPIDI_CH4U_REQUEST(req, tag) & ~(MPIDI_CH4U_REQUEST(req, req->rreq.ignore)))) {
            MPL_DL_DELETE(*list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    if(!req)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DEQUEUE_POSTED);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_delete_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_delete_posted(MPIDI_CH4U_rreq_t * req,
                                                      MPIDI_CH4U_rreq_t ** list)
{
    int found = 0;
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DELETE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DELETE_POSTED);
    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    MPL_DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        if (curr == req) {
            MPL_DL_DELETE(*list, curr);
            found = 1;
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
    }
    if(!found)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DELETE_POSTED);
    return found;
}

#else /* #ifdef MPIDI_CH4U_USE_PER_COMM_QUEUE */

/* Use global queue */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_enqueue_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_enqueue_posted(MPIR_Request * req,
                                                        MPIDI_CH4U_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_ENQUEUE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_ENQUEUE_POSTED);
    MPIDI_CH4U_REQUEST(req, req->rreq.request) = (uint64_t) req;
    MPL_DL_APPEND(MPIDI_CH4_Global.posted_list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, posted_recvq_length, 1);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_ENQUEUE_POSTED);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_enqueue_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_enqueue_unexp(MPIR_Request * req,
                                                       MPIDI_CH4U_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_ENQUEUE_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_ENQUEUE_UNEXP);
    MPIDI_CH4U_REQUEST(req, req->rreq.request) = (uint64_t) req;
    MPL_DL_APPEND(MPIDI_CH4_Global.unexp_list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_length, 1);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_ENQUEUE_UNEXP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_delete_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_CH4U_delete_unexp(MPIR_Request * req, MPIDI_CH4U_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DELETE_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DELETE_UNEXP);
    MPL_DL_DELETE(MPIDI_CH4_Global.unexp_list, &req->dev.ch4.am.req->rreq);
    MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DELETE_UNEXP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_dequeue_unexp_strict
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_dequeue_unexp_strict(uint64_t tag,
                                                                       uint64_t ignore,
                                                                       MPIDI_CH4U_rreq_t ** list)
{
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP_STRICT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP_STRICT);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    MPL_DL_FOREACH_SAFE(MPIDI_CH4_Global.unexp_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if (!(MPIDI_CH4U_REQUEST(req, req->status) & MPIDI_CH4U_REQ_BUSY) &&
            ((tag & ~ignore) == (MPIDI_CH4U_REQUEST(req, tag) & ~ignore))) {
            MPL_DL_DELETE(MPIDI_CH4_Global.unexp_list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP_STRICT);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_dequeue_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_dequeue_unexp(uint64_t tag, uint64_t ignore,
                                                                MPIDI_CH4U_rreq_t ** list)
{
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    MPL_DL_FOREACH_SAFE(MPIDI_CH4_Global.unexp_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if ((tag & ~ignore) == (MPIDI_CH4U_REQUEST(req, tag) & ~ignore)) {
            MPL_DL_DELETE(MPIDI_CH4_Global.unexp_list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DEQUEUE_UNEXP);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_find_unexp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_find_unexp(uint64_t tag, uint64_t ignore,
                                                             MPIDI_CH4U_rreq_t ** list)
{
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_FIND_UNEXP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_FIND_UNEXP);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);
    MPL_DL_FOREACH_SAFE(MPIDI_CH4_Global.unexp_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if ((tag & ~ignore) == (MPIDI_CH4U_REQUEST(req, tag) & ~ignore)) {
            break;
        }
        req = NULL;
    }
    MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_FIND_UNEXP);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_dequeue_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_CH4U_dequeue_posted(uint64_t tag,
                                                                 MPIDI_CH4U_rreq_t ** list)
{
    MPIR_Request *req = NULL;
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DEQUEUE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DEQUEUE_POSTED);

    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    MPL_DL_FOREACH_SAFE(MPIDI_CH4_Global.posted_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        req = (MPIR_Request *) curr->request;
        if ((tag & ~MPIDI_CH4U_REQUEST(req, req->rreq.ignore)) ==
            (MPIDI_CH4U_REQUEST(req, tag) & ~MPIDI_CH4U_REQUEST(req, req->rreq.ignore))) {
            MPL_DL_DELETE(MPIDI_CH4_Global.posted_list, curr);
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
        req = NULL;
    }
    if(!req)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DEQUEUE_POSTED);
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_delete_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_delete_posted(MPIDI_CH4U_rreq_t * req,
                                                      MPIDI_CH4U_rreq_t ** list)
{
    int found = 0;
    MPIDI_CH4U_rreq_t *curr, *tmp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DELETE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DELETE_POSTED);
    MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq);
    MPL_DL_FOREACH_SAFE(MPIDI_CH4_Global.posted_list, curr, tmp) {
        MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);
        if (curr == req) {
            MPL_DL_DELETE(MPIDI_CH4_Global.posted_list, curr);
            found = 1;
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);
            break;
        }
    }
    if(!found)
        MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DELETE_POSTED);
    return found;
}

#endif /* MPIDI_CH4U_USE_PER_COMM_QUEUE */

#endif /* CH4R_RECVQ_H_INCLUDED */
