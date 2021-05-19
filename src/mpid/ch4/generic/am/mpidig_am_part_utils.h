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

/* Status to deactivate at request completion */
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

MPL_STATIC_INLINE_PREFIX void MPIDIG_part_match_rreq(MPIR_Request * part_req)
{
    MPI_Aint sdata_size = MPIDIG_PART_REQUEST(part_req, u.recv).sdata_size;

    /* Set status for partitioned req */
    MPIR_STATUS_SET_COUNT(part_req->status, sdata_size);
    part_req->status.MPI_SOURCE = MPIDI_PART_REQUEST(part_req, rank);
    part_req->status.MPI_TAG = MPIDI_PART_REQUEST(part_req, tag);
    part_req->status.MPI_ERROR = MPI_SUCCESS;

    /* Additional check for partitioned pt2pt: require identical buffer size */
    if (part_req->status.MPI_ERROR == MPI_SUCCESS) {
        MPI_Aint rdata_size;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(part_req, datatype), rdata_size);
        rdata_size *= MPIDI_PART_REQUEST(part_req, count) * part_req->u.part.partitions;
        if (sdata_size != rdata_size) {
            part_req->status.MPI_ERROR =
                MPIR_Err_create_code(part_req->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, __FUNCTION__,
                                     __LINE__, MPI_ERR_OTHER, "**ch4|partmismatchsize",
                                     "**ch4|partmismatchsize %d %d %d %d",
                                     part_req->status.MPI_SOURCE, part_req->status.MPI_TAG,
                                     (int) rdata_size, (int) sdata_size);
        }
    }
}

/* Receiver issues a CTS to sender once reach MPIDIG_PART_REQ_CTS */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_cts(MPIR_Request * rreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_ISSUE_CTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_ISSUE_CTS);

    MPIDIG_part_cts_msg_t am_hdr;
    am_hdr.sreq_ptr = MPIDIG_PART_REQUEST(rreq_ptr, peer_req_ptr);
    am_hdr.rreq_ptr = rreq_ptr;

    int source = MPIDI_PART_REQUEST(rreq_ptr, rank);
    MPIR_Context_id_t context_id = MPIDI_PART_REQUEST(rreq_ptr, context_id);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq_ptr, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(context_id, source, MPIDIG_PART_CTS, &am_hdr,
                                        sizeof(am_hdr));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(context_id, source, MPIDIG_PART_CTS, &am_hdr,
                                       sizeof(am_hdr));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_ISSUE_CTS);
    return mpi_errno;
}

/* Sender issues data after all partitions are ready and CTS; called by pready functions. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_data(MPIR_Request * part_sreq, int is_local)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_ISSUE_DATA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_ISSUE_DATA);

    MPIR_Request *sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_part_send_data_msg_t am_hdr;
    am_hdr.rreq_ptr = MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr);

    /* Complete partitioned req at am request completes */
    sreq->completion_notification = &part_sreq->cc;
    /* Will update part_sreq status when the AM request completes
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(sreq, req->part_am_req.part_req_ptr) = part_sreq;

    /* Count of entire send buffer; partitions are contiguous in memory. */
    MPI_Aint count = MPIDI_PART_REQUEST(part_sreq, count) * part_sreq->u.part.partitions;
    /* Check potential overflow */
    MPIR_Assert(MPIDI_PART_REQUEST(part_sreq, count) * part_sreq->u.part.partitions <
                MPIR_AINT_MAX);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (is_local) {
        mpi_errno = MPIDI_SHM_am_isend(MPIDI_PART_REQUEST(part_sreq, rank),
                                       part_sreq->comm, MPIDIG_PART_SEND_DATA,
                                       &am_hdr, sizeof(am_hdr),
                                       MPIDI_PART_REQUEST(part_sreq, buffer), count,
                                       MPIDI_PART_REQUEST(part_sreq, datatype), sreq);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend(MPIDI_PART_REQUEST(part_sreq, rank),
                                      part_sreq->comm, MPIDIG_PART_SEND_DATA,
                                      &am_hdr, sizeof(am_hdr),
                                      MPIDI_PART_REQUEST(part_sreq, buffer), count,
                                      MPIDI_PART_REQUEST(part_sreq, datatype), sreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_ISSUE_DATA);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_AM_PART_UTILS_H_INCLUDED */
