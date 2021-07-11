/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_UTILS_H_INCLUDED
#define MPIDIG_AM_PART_UTILS_H_INCLUDED

#include "ch4_impl.h"

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
    CH4_CALL(am_send_hdr_reply(rreq_ptr->comm, source, MPIDIG_PART_CTS, &am_hdr, sizeof(am_hdr)),
             MPIDI_REQUEST(rreq_ptr, is_local), mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_ISSUE_CTS);
    return mpi_errno;
}

/* Sender issues data after all partitions are ready and CTS; called by pready functions. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_data(MPIR_Request * part_sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_ISSUE_DATA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_ISSUE_DATA);

    MPIR_Request *sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    sreq->comm = part_sreq->comm;
    MPIR_Comm_add_ref(sreq->comm);

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

    CH4_CALL(am_isend(MPIDI_PART_REQUEST(part_sreq, rank), part_sreq->comm, MPIDIG_PART_SEND_DATA,
                      &am_hdr, sizeof(am_hdr),
                      MPIDI_PART_REQUEST(part_sreq, buffer), count,
                      MPIDI_PART_REQUEST(part_sreq, datatype), sreq),
             MPIDI_REQUEST(part_sreq, is_local), mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_ISSUE_DATA);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_AM_PART_UTILS_H_INCLUDED */
