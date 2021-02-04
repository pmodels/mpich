/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_UTILS_H_INCLUDED
#define MPIDIG_AM_PART_UTILS_H_INCLUDED

#include "ch4_impl.h"

/* Request status used for partitioned comm.
 * Updated as atomic cntr because AM callback and local operation may be
 * concurrently performed by threads. */
#define MPIDIG_PART_REQ_UNSET 0 /* Initial status set at psend|precv_init */
#define MPIDIG_PART_REQ_PARTIAL_ACTIVATED 1     /* Intermediate status (unused).
                                                 * On receiver means either (1) request matched at initial round
                                                 * or (2) locally started;
                                                 * On sender means either (1) receiver started
                                                 * or (2) locally started. */
#define MPIDIG_PART_REQ_CTS 2   /* Indicating receiver is ready to receive data
                                 * (requests matched and start called) */

/* Status to inactivate at request completion */
#define MPIDIG_PART_SREQ_INACTIVE 0     /* Sender need 2 increments to be CTS: start and remote start */
#define MPIDIG_PART_RREQ_INACTIVE 1     /* Receiver need 1 increments to be CTS: start */

/* Atomically increase partitioned rreq status and fetch state after increase.
 * Called when receiver matches request, sender receives CTS, or either side calls start. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_PART_REQ_INC_FETCH_STATUS(MPIR_Request * part_req)
{
    int prev_stat = MPL_atomic_fetch_add_int(&MPIDIG_PART_REQUEST(part_req, status), 1);
    return prev_stat + 1;
}

#ifdef HAVE_ERROR_CHECKING
#define MPIDIG_PART_CHECK_RREQ_CTS(req)                                                 \
    do {                                                                                \
        MPID_BEGIN_ERROR_CHECKS;                                                        \
        if (MPL_atomic_load_int(&MPIDIG_PART_REQUEST(req, status))                      \
                   != MPIDIG_PART_REQ_CTS) {                                            \
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_OTHER, goto fn_fail, "**ch4|partitionsync"); \
        }                                                                               \
        MPID_END_ERROR_CHECKS;                                                          \
    } while (0);
#else
#define MPIDIG_PART_CHECK_RREQ_CTS(rreq)
#endif

MPL_STATIC_INLINE_PREFIX int MPIDIG_part_match(int rank, int tag,
                                               MPIR_Context_id_t context_id, MPIR_Request * req)
{
    return rank == MPIDI_PART_REQUEST(req, rank) &&
        tag == MPIR_TAG_MASK_ERROR_BITS(MPIDI_PART_REQUEST(req, tag)) &&
        context_id == MPIDI_PART_REQUEST(req, context_id);
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_part_enqueue(MPIR_Request * part_req,
                                                  MPIDIG_part_rreq_t ** list)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_ENQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_ENQUEUE);
    MPIDIG_PART_REQUEST(part_req, u.recv).request = part_req;
    DL_APPEND(*list, &MPIDIG_PART_REQUEST(part_req, u.recv));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_ENQUEUE);
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDIG_part_dequeue(int rank, int tag,
                                                           MPIR_Context_id_t context_id,
                                                           MPIDIG_part_rreq_t ** list)
{
    MPIR_Request *part_req = NULL;
    MPIDIG_part_rreq_t *curr, *tmp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_DEQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_DEQUEUE);

    DL_FOREACH_SAFE(*list, curr, tmp) {
        if (MPIDIG_part_match(rank, tag, context_id, curr->request)) {
            part_req = curr->request;
            DL_DELETE(*list, curr);
            break;
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_DEQUEUE);
    return part_req;
}

#endif /* MPIDIG_AM_PART_UTILS_H_INCLUDED */
