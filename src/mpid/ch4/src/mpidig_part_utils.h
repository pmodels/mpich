/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef MPIDIG_PART_UTILS_H_INCLUDED
#define MPIDIG_PART_UTILS_H_INCLUDED

#include "ch4_impl.h"

/* defines the different stages a send partitions must go through */
enum MPIDIG_part_status_send_cc_t {
    MPIDIG_PART_STATUS_SEND_READY,      /* 0 = partition is ready to be sent */
    /* below insert all the needed operations */
    MPIDIG_PART_STATUS_SEND_CTS,        /* CTS received */
    /* the last value is the initialization value */
    MPIDIG_PART_STATUS_SEND_INIT
};

/* defines the different stages a recv partitions must go through */
enum MPIDIG_part_status_recv_cc_t {
    MPIDIG_PART_STATUS_RECV_ARRIVED,    /* 0 = partition has arrived */
    /* below insert all the needed operations */
    /* the last value is the initialization value */
    MPIDIG_PART_STATUS_RECV_INIT
};

/* functions defined in .c */
void MPIDIG_part_rreq_create(MPIR_Request ** req);
void MPIDIG_part_sreq_create(MPIR_Request ** req);

void MPIDIG_part_rreq_reset_cc_part(MPIR_Request * rqst);
void MPIDIG_part_sreq_reset_cc_part(MPIR_Request * rqst);

void MPIDIG_part_rreq_matched(MPIR_Request * rreq);
void MPIDIG_part_rreq_update_sinfo(MPIR_Request * rreq, MPIDIG_part_send_init_msg_t * msg_hdr);

/* Receiver issues a TS to sender once reach MPIDIG_PART_REQ_CTS */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_cts(MPIR_Request * rreq_ptr)
{
    MPIR_Assert(rreq_ptr->kind == MPIR_REQUEST_KIND__PART_RECV);
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_part_cts_msg_t am_hdr;
    am_hdr.msg_part = MPIDIG_PART_REQUEST(rreq_ptr, u.recv.msg_part);
    am_hdr.sreq_ptr = MPIDIG_PART_REQUEST(rreq_ptr, peer_req_ptr);
    am_hdr.rreq_ptr = rreq_ptr;

    int source = MPIDI_PART_REQUEST(rreq_ptr, u.recv.source);
    CH4_CALL(am_send_hdr_reply(rreq_ptr->comm, source, MPIDIG_PART_CTS, &am_hdr, sizeof(am_hdr),
                               0, 0), MPIDI_REQUEST(rreq_ptr, is_local), mpi_errno);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

enum MPIDIG_part_issue_mode {
    MPIDIG_PART_REGULAR,        /* when called outside of the progress engine */
    MPIDIG_PART_REPLY,          /* when called from the progress engine */
};

/* Convert the 'origin' indexing layout into the 'arget' indexing layout.
 * For a given index in the origin (io) return the lower index to vising in the target indexing layout.
 * no = the number of indexes in the origin indexing layout
 * nt = the number of indexes in the target indexing layout
 */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_idx_lb(const int io, const int no, const int nt)
{
    return (io * nt) / no;
}

/* Convert the 'origin' indexing layout into the 'arget' indexing layout.
 * For a given index in the origin (io) return the upper index (not included!) to vising in the target indexing layout.
 * no = the number of indexes in the origin indexing layout
 * nt = the number of indexes in the target indexing layout
 */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_idx_ub(const int io, const int no, const int nt)
{
    const int tmp = (io + 1) * nt;
    const int it = tmp / no + (tmp % no > 0);
    return MPL_MIN(it, nt);
}

/* Sender issues data after all partitions are ready and CTS; called by pready functions. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_data(const int imsg, MPIR_Request * part_sreq,
                                                    enum MPIDIG_part_issue_mode mode)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    // TG: why is the user-request created with MPIDI_CH4_REQUEST_CREATE and this one a normal request on the VCI (target + source)

    // create a request specifically for this partition
    //  TODO change the VCI
    MPIR_Request *sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1, 0, 0);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    sreq->comm = part_sreq->comm;
    MPIR_Comm_add_ref(sreq->comm);

    /* we already know which request is assigned at the receiver
     * so there is no need to match anything here
     */
    const int msg_part = MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part);
    MPIR_Assert(msg_part >= 0);

    MPIDIG_part_send_data_msg_t am_hdr;
    am_hdr.rreq_ptr = MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr);
    am_hdr.imsg = imsg;
    MPIR_Assert(imsg >= 0);
    MPIR_Assert(imsg < msg_part);

    /* Complete partitioned req at am request completes */
    sreq->dev.completion_notification = &part_sreq->cc;
    /* Will update part_sreq status when the AM request completes
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(sreq, req->part_am_req.part_req_ptr) = part_sreq;

    /* decrease the counter of the number of msgs requested to be send
     * if we have requested the send of every msgs then we can reset the counters */
    int incomplete;
    MPIR_cc_decr(&MPIDIG_PART_REQUEST(part_sreq, u.send.cc_send), &incomplete);
    if (!incomplete) {
        const int n_part = part_sreq->u.part.partitions;
        MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(part_sreq, u.send.cc_part);
        for (int ip = 0; ip < n_part; ++ip) {
            MPIR_cc_set(&cc_part[ip], MPIDIG_PART_STATUS_SEND_INIT);
        }
    }

    /* get the count per the msg partition and the shift in the buffer from the datatype */
    MPI_Aint count = MPIDI_PART_REQUEST(part_sreq, count) * part_sreq->u.part.partitions;
    MPIR_Assert(count % msg_part == 0);
    count /= msg_part;
    MPIR_Assert(count < MPIR_AINT_MAX);

    MPI_Aint part_offset;
    MPIR_Datatype_get_extent_macro(MPIDI_PART_REQUEST(part_sreq, datatype), part_offset);
    part_offset *= count;
    const void *part_buf = (char *) MPIDI_PART_REQUEST(part_sreq, buffer) + imsg * part_offset;

    if (mode == MPIDIG_PART_REGULAR) {
        CH4_CALL(am_isend
                 (/*matching data */ MPIDI_PART_REQUEST(part_sreq, u.send.dest), part_sreq->comm,
                  MPIDIG_PART_SEND_DATA,
                  /* header */ &am_hdr, sizeof(am_hdr),
                  /* buff */ part_buf, count, MPIDI_PART_REQUEST(part_sreq, datatype), 0, 0, sreq),
                 MPIDI_REQUEST(part_sreq, is_local), mpi_errno);
    } else {    /* MPIDIG_PART_REPLY */
        CH4_CALL(am_isend_reply(part_sreq->comm, MPIDI_PART_REQUEST(part_sreq, u.send.dest),
                                MPIDIG_PART_SEND_DATA, &am_hdr, sizeof(am_hdr), part_buf, count,
                                MPIDI_PART_REQUEST(part_sreq, datatype), 0, 0, sreq),
                 MPIDI_REQUEST(part_sreq, is_local), mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_part_issue_msg_if_ready(const int msg_lb, const int msg_ub,
                                                            MPIR_Request * sreq,
                                                            enum MPIDIG_part_issue_mode mode)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__PART_SEND);
    const int send_part = sreq->u.part.partitions;
    const int msg_part = MPIDIG_PART_REQUEST(sreq, u.send.msg_part);
    const MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(sreq, u.send.cc_part);
    MPIR_Assert(msg_part > 0);

    /*for each of the communication msgs in the range, try to see if they are ready */
    for (int im = msg_lb; im < msg_ub; ++im) {
        const int ip_lb = MPIDIG_part_idx_lb(im, msg_part, send_part);
        const int ip_ub = MPIDIG_part_idx_ub(im, msg_part, send_part);

        bool ready = true;
        for (int ip = ip_lb; ip < ip_ub; ++ip) {
            MPIR_Assert(ip >= 0);
            MPIR_Assert(ip < send_part);
            ready &= (MPIDIG_PART_STATUS_SEND_READY == MPIR_cc_get(cc_part[ip]));
        }
        if (ready) {
            mpi_errno = MPIDIG_part_issue_data(im, sreq, MPIDIG_PART_REPLY);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPIDIG_PART_UTILS_H_INCLUDED */
