/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidig_part_utils.h"

/* creates a MPIDIG SEND request */
void MPIDIG_part_sreq_create(MPIR_Request ** req)
{
    MPIR_Request *sreq = req[0];
    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__PART_SEND);

    /*allocate the cc_part array to the send_part size */
    const int n_part = sreq->u.part.partitions;
    MPIDIG_PART_REQUEST(sreq, u.send.cc_part) =
        MPL_malloc(sizeof(MPIR_cc_t) * n_part, MPL_MEM_OTHER);
    MPIDIG_PART_REQUEST(sreq, u.send.msg_part) = -1;
    /* set the counter of msgs actually sent to 0 */
    MPIR_cc_set(&MPIDIG_PART_REQUEST(sreq, u.send.cc_send), 0);
}

/* creates a MPIDIG RECV request */
void MPIDIG_part_rreq_create(MPIR_Request ** req)
{
    MPIR_Request *rreq = req[0];
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    MPIDIG_PART_REQUEST(rreq, u.recv.msg_part) = -1;
    MPIDIG_PART_REQUEST(rreq, u.recv.cc_part) = NULL;
}

/* called when a receive Request has been matched
 * - set the status
 * - allocate the cc_part
 * */
void MPIDIG_part_rreq_matched(MPIR_Request * rreq)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    /* Set status for partitioned req */
    MPI_Aint sdata_size = MPIDIG_PART_REQUEST(rreq, u.recv.send_dsize);
    MPIR_STATUS_SET_COUNT(rreq->status, sdata_size);
    rreq->status.MPI_SOURCE = MPIDI_PART_REQUEST(rreq, u.recv.source);
    rreq->status.MPI_TAG = MPIDI_PART_REQUEST(rreq, u.recv.tag);
    rreq->status.MPI_ERROR = MPI_SUCCESS;

    /* Additional check for partitioned pt2pt: require identical buffer size */
    if (rreq->status.MPI_ERROR == MPI_SUCCESS) {
        MPI_Aint rdata_size;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(rreq, datatype), rdata_size);
        rdata_size *= MPIDI_PART_REQUEST(rreq, count) * rreq->u.part.partitions;
        if (sdata_size != rdata_size) {
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(rreq->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, __FUNCTION__,
                                     __LINE__, MPI_ERR_OTHER, "**ch4|partmismatchsize",
                                     "**ch4|partmismatchsize %d %d",
                                     (int) rdata_size, (int) sdata_size);
        }
    }

    const int msg_part = MPIDIG_PART_REQUEST(rreq, u.recv.msg_part);
    MPIR_Assert(msg_part >= 0);
    MPIDIG_PART_REQUEST(rreq, u.recv.cc_part) =
        MPL_malloc(sizeof(MPIR_cc_t) * msg_part, MPL_MEM_OTHER);
}

/* partition recv request - reset cc_part array of an activated request */
void MPIDIG_part_rreq_reset_cc_part(MPIR_Request * rqst)
{
    MPIR_Assert(MPIR_Part_request_is_active(rqst));
    MPIR_Assert(rqst->kind == MPIR_REQUEST_KIND__PART_RECV);

    /* reset the counters to the init value */
    const int msg_part = MPIDIG_PART_REQUEST(rqst, u.recv.msg_part);
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(rqst, u.recv.cc_part);
    MPIR_Assert(msg_part >= 0);
    for (int i = 0; i < msg_part; ++i) {
        MPIR_cc_set(&cc_part[i], MPIDIG_PART_STATUS_RECV_INIT);
    }
}

/* partition send requests - resets the cc_part */
void MPIDIG_part_sreq_reset_cc_part(MPIR_Request * rqst)
{
    MPIR_Assert(rqst->kind == MPIR_REQUEST_KIND__PART_SEND);
    MPIR_Assert(MPIDIG_PART_REQUEST(rqst, u.send.msg_part) < 0);

    const int send_part = rqst->u.part.partitions;
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(rqst, u.send.cc_part);
    MPIR_Assert(send_part >= 0);
    for (int i = 0; i < send_part; ++i) {
        MPIR_cc_set(&cc_part[i], MPIDIG_PART_STATUS_SEND_INIT);
    }
}

/* partition recv requests - update the request with header information*/
void MPIDIG_part_rreq_update_sinfo(MPIR_Request * rreq, MPIDIG_part_send_init_msg_t * msg_hdr)
{
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART_RECV);

    MPIDIG_PART_REQUEST(rreq, u.recv.send_dsize) = msg_hdr->data_sz;
    MPIDIG_PART_REQUEST(rreq, peer_req_ptr) = msg_hdr->sreq_ptr;

    MPIDIG_PART_REQUEST(rreq, u.recv.msg_part) = msg_hdr->send_npart;

    /* 0 partition is illegual so at least one message must happen */
    MPIR_Assert(MPIDIG_PART_REQUEST(rreq, u.recv.msg_part) > 0);
}
