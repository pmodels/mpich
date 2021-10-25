/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_RECVQ_H_INCLUDED
#define MPIDIG_RECVQ_H_INCLUDED

#include <mpidimpl.h>
#include "utlist.h"
#include "ch4_impl.h"

extern unsigned PVAR_LEVEL_posted_recvq_length ATTRIBUTE((unused));
extern unsigned PVAR_LEVEL_unexpected_recvq_length ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_posted_recvq_match_attempts ATTRIBUTE((unused));
extern unsigned long long PVAR_COUNTER_unexpected_recvq_match_attempts ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_time_failed_matching_postedq ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_time_matching_unexpectedq ATTRIBUTE((unused));

extern int unexp_message_indices[2];

int MPIDIG_recvq_init(void);

/* match queue types support by these utilities */
enum MPIDIG_queue_type {
    MPIDIG_PT2PT_POSTED,
    MPIDIG_PT2PT_UNEXP,
    MPIDIG_PART,
};

/* match queue PVAR macros */
#define MPIDIG_PVAR_QUEUE_LEVEL_INC(qtype_)                             \
    do {                                                                \
        if (qtype_ == MPIDIG_PT2PT_POSTED) {                            \
            MPIR_T_PVAR_LEVEL_INC(RECVQ, posted_recvq_length, 1);       \
        } else if (qtype_ == MPIDIG_PT2PT_UNEXP) {                      \
            MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_length, 1);   \
        }                                                               \
    } while (0)

#define MPIDIG_PVAR_QUEUE_LEVEL_DEC(qtype_)                             \
    do {                                                                \
        if (qtype_ == MPIDIG_PT2PT_POSTED) {                            \
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, posted_recvq_length, 1);       \
        } else if (qtype_ == MPIDIG_PT2PT_UNEXP) {                      \
            MPIR_T_PVAR_LEVEL_DEC(RECVQ, unexpected_recvq_length, 1);   \
        }                                                               \
    } while (0)

#define MPIDIG_PVAR_QUEUE_TIMER_START(qtype_)                             \
    do {                                                                  \
        if (qtype_ == MPIDIG_PT2PT_POSTED) {                              \
            MPIR_T_PVAR_TIMER_START(RECVQ, time_failed_matching_postedq); \
        } else if (qtype_ == MPIDIG_PT2PT_UNEXP) {                        \
            MPIR_T_PVAR_TIMER_START(RECVQ, time_matching_unexpectedq);    \
        }                                                                 \
    } while (0)

#define MPIDIG_PVAR_QUEUE_TIMER_END(qtype_, req_)                       \
    do {                                                                \
        if (qtype_ == MPIDIG_PT2PT_POSTED && !(req_)) {                 \
            MPIR_T_PVAR_TIMER_END(RECVQ, time_failed_matching_postedq); \
        } else if (qtype_ == MPIDIG_PT2PT_UNEXP) {                      \
            MPIR_T_PVAR_TIMER_END(RECVQ, time_matching_unexpectedq);    \
        }                                                               \
    } while (0)

#define MPIDIG_PVAR_QUEUE_COUNTER_INC(qtype_)                                   \
    do {                                                                        \
        if (qtype_ == MPIDIG_PT2PT_POSTED) {                                    \
            MPIR_T_PVAR_COUNTER_INC(RECVQ, posted_recvq_match_attempts, 1);     \
        } else if (qtype_ == MPIDIG_PT2PT_UNEXP) {                              \
            MPIR_T_PVAR_COUNTER_INC(RECVQ, unexpected_recvq_match_attempts, 1); \
        }                                                                       \
    } while (0)

#define MPIDIG_DO_ENQUEUE_EVENT(qtype_)                      \
    do {                                                     \
        if (qtype_ == MPIDIG_PT2PT_UNEXP) {                  \
            MPIR_T_DO_EVENT(unexp_message_indices[0],        \
                            MPI_T_CB_REQUIRE_MPI_RESTRICTED, \
                            &MPIDIG_REQUEST(req, u.recv.source));     \
        }                                                    \
    } while (0)

#define MPIDIG_DO_DEQUEUE_EVENT(qtype_)                      \
    do {                                                     \
        if (qtype_ == MPIDIG_PT2PT_UNEXP) {                  \
            MPIR_T_DO_EVENT(unexp_message_indices[1],        \
                            MPI_T_CB_REQUIRE_MPI_RESTRICTED, \
                            &MPIDIG_REQUEST(req, u.recv.source));     \
        }                                                    \
    } while (0)

/* match and search functions */
MPL_STATIC_INLINE_PREFIX bool MPIDIG_match_request(int rank, int tag,
                                                   MPIR_Context_id_t context_id, MPIR_Request * req,
                                                   enum MPIDIG_queue_type qtype)
{
    if (qtype == MPIDIG_PT2PT_POSTED) {
        return (rank == MPIDIG_REQUEST(req, u.recv.source) ||
                MPIDIG_REQUEST(req, u.recv.source) == MPI_ANY_SOURCE) &&
            (tag == MPIR_TAG_MASK_ERROR_BITS(MPIDIG_REQUEST(req, u.recv.tag)) ||
             MPIDIG_REQUEST(req, u.recv.tag) == MPI_ANY_TAG) &&
            context_id == MPIDIG_REQUEST(req, u.recv.context_id);
    } else if (qtype == MPIDIG_PT2PT_UNEXP) {
        return (rank == MPIDIG_REQUEST(req, u.recv.source) || rank == MPI_ANY_SOURCE) &&
            (tag == MPIR_TAG_MASK_ERROR_BITS(MPIDIG_REQUEST(req, u.recv.tag)) ||
             tag == MPI_ANY_TAG) && context_id == MPIDIG_REQUEST(req, u.recv.context_id);
    } else if (qtype == MPIDIG_PART) {
        return rank == MPIDI_PART_REQUEST(req, u.recv.source) &&
            tag == MPIR_TAG_MASK_ERROR_BITS(MPIDI_PART_REQUEST(req, u.recv.tag)) &&
            context_id == MPIDI_PART_REQUEST(req, u.recv.context_id);
    } else {
        /* unknown queue type */
        MPIR_Assert(0);
    }

    return false;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_enqueue_request(MPIR_Request * req, MPIR_Request ** list,
                                                     enum MPIDIG_queue_type qtype)
{
    MPIR_FUNC_ENTER;
    DL_APPEND(*list, req);
    MPIDIG_DO_ENQUEUE_EVENT(qtype);
    MPIDIG_PVAR_QUEUE_LEVEL_INC(qtype);
    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_recvq_search(int rank, int tag,
                                                           MPIR_Context_id_t context_id,
                                                           MPIR_Request ** list,
                                                           enum MPIDIG_queue_type qtype,
                                                           bool dequeue)
{
    MPIR_Request *curr, *tmp;
    MPIR_Request *req = NULL;
    MPIR_FUNC_ENTER;

    MPIDIG_PVAR_QUEUE_TIMER_START(qtype);
    DL_FOREACH_SAFE(*list, curr, tmp) {
        MPIDIG_PVAR_QUEUE_COUNTER_INC(qtype);
        if (MPIDIG_match_request(rank, tag, context_id, curr, qtype)) {
            if (dequeue == true) {
                DL_DELETE(*list, curr);
                MPIDIG_DO_DEQUEUE_EVENT(qtype);
                MPIDIG_PVAR_QUEUE_LEVEL_DEC(qtype);
            }
            req = curr;
            break;
        }
    }
    MPIDIG_PVAR_QUEUE_TIMER_END(qtype, req);
    MPIR_FUNC_EXIT;
    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_rreq_find(int rank, int tag,
                                                        MPIR_Context_id_t context_id,
                                                        MPIR_Request ** list,
                                                        enum MPIDIG_queue_type qtype)
{
    return MPIDIG_recvq_search(rank, tag, context_id, list, qtype, false);
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_rreq_dequeue(int rank, int tag,
                                                           MPIR_Context_id_t context_id,
                                                           MPIR_Request ** list,
                                                           enum MPIDIG_queue_type qtype)
{
    return MPIDIG_recvq_search(rank, tag, context_id, list, qtype, true);
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_delete_posted(MPIR_Request * req, MPIR_Request ** list)
{
    int found = 0;
    MPIR_Request *curr, *tmp;
    MPIR_FUNC_ENTER;
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

    MPIR_FUNC_EXIT;
    return found;
}

#endif /* MPIDIG_RECVQ_H_INCLUDED */
