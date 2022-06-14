/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_PART_UTILS_H_INCLUDED
#define MPIDIG_PART_UTILS_H_INCLUDED

#include "ch4_impl.h"

/* Receiver issues a CTS to sender once reach MPIDIG_PART_REQ_CTS */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_cts(MPIR_Request * rreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_part_cts_msg_t am_hdr;
    am_hdr.sreq_ptr = MPIDIG_PART_REQUEST(rreq_ptr, peer_req_ptr);
    am_hdr.rreq_ptr = rreq_ptr;

    int source = MPIDI_PART_REQUEST(rreq_ptr, u.recv.source);
    CH4_CALL(am_send_hdr_reply(rreq_ptr->comm, source, MPIDIG_PART_CTS, &am_hdr, sizeof(am_hdr),
                               0, 0), MPIDI_REQUEST(rreq_ptr, is_local), mpi_errno);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

enum MPIDIG_part_issue_mode {
    MPIDIG_PART_REGULAR,
    MPIDIG_PART_REPLY,
};

/* Sender issues data after all partitions are ready and CTS; called by pready functions. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_data(MPIR_Request * part_sreq,
                                                    enum MPIDIG_part_issue_mode mode)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Request *sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1, 0, 0);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    sreq->comm = part_sreq->comm;
    MPIR_Comm_add_ref(sreq->comm);

    MPIDIG_part_send_data_msg_t am_hdr;
    am_hdr.rreq_ptr = MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr);

    /* Complete partitioned req at am request completes */
    sreq->dev.completion_notification = &part_sreq->cc;
    /* Will update part_sreq status when the AM request completes
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(sreq, req->part_am_req.part_req_ptr) = part_sreq;

    /* Count of entire send buffer; partitions are contiguous in memory. */
    MPI_Aint count = MPIDI_PART_REQUEST(part_sreq, count) * part_sreq->u.part.partitions;
    /* Check potential overflow */
    MPIR_Assert(MPIDI_PART_REQUEST(part_sreq, count) * part_sreq->u.part.partitions <
                MPIR_AINT_MAX);

    if (mode == MPIDIG_PART_REGULAR) {
        CH4_CALL(am_isend(MPIDI_PART_REQUEST(part_sreq, u.send.dest), part_sreq->comm,
                          MPIDIG_PART_SEND_DATA,
                          &am_hdr, sizeof(am_hdr), MPIDI_PART_REQUEST(part_sreq, buffer),
                          count, MPIDI_PART_REQUEST(part_sreq, datatype), 0, 0, sreq),
                 MPIDI_REQUEST(part_sreq, is_local), mpi_errno);
    } else {    /* MPIDIG_PART_REPLY */
        CH4_CALL(am_isend_reply(part_sreq->comm, MPIDI_PART_REQUEST(part_sreq, u.send.dest),
                                MPIDIG_PART_SEND_DATA,
                                &am_hdr, sizeof(am_hdr),
                                MPIDI_PART_REQUEST(part_sreq, buffer), count,
                                MPIDI_PART_REQUEST(part_sreq, datatype), 0, 0, sreq),
                 MPIDI_REQUEST(part_sreq, is_local), mpi_errno);
    }

    /* reset ready counter */
    MPIR_cc_set(&MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr,
                part_sreq->u.part.partitions + 1);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_PART_UTILS_H_INCLUDED */
