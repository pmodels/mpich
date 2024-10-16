/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_am_events.h"
#include "ofi_events.h"

#define MPIDI_OFI_CTS_FLAG__NONE   0
#define MPIDI_OFI_CTS_FLAG__PROBE  1

struct rndv_cts {
    MPIR_Request *rreq;
    int am_tag;
    int flag;
};

/* ---- receiver side ---- */

static int rndv_event_common(int vci, MPIR_Request * rreq, int *vci_src_out, int *vci_dst_out)
{
    int mpi_errno = MPI_SUCCESS;

    /* if we were expecting an eager send, free the unneeded pack_buffer or iovs array */
    switch (MPIDI_OFI_REQUEST(rreq, event_id)) {
        case MPIDI_OFI_EVENT_RECV_PACK:
        case MPIDI_OFI_EVENT_RECV_HUGE:
            MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer));
            break;
        case MPIDI_OFI_EVENT_RECV_NOPACK:
            MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack.iovs));
            break;
    }

    /* save and free up the OFI request */
    void *buf = MPIDI_OFI_REQUEST(rreq, buf);
    MPI_Aint count = MPIDI_OFI_REQUEST(rreq, count);
    MPI_Datatype datatype = MPIDI_OFI_REQUEST(rreq, datatype);;

    /* next, convert it to an MPIDIG request */
    /* the vci need be consistent with MPIDI_OFI_RECV_VNIS in ofi_recv.h */
    MPIR_Comm *comm = rreq->comm;
    int src_rank = rreq->status.MPI_SOURCE;
    int tag = rreq->status.MPI_TAG;
    int vci_src = MPIDI_get_vci(SRC_VCI_FROM_RECVER, comm, src_rank, comm->rank, tag);
    int vci_dst = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, src_rank, comm->rank, tag);
    MPIR_Assert(vci == vci_dst);


    /* TODO: optimize - since we are going to use am_tag_recv, we won't need most of the MPIDIG request fields */
    mpi_errno = MPIDIG_request_init_internal(rreq, vci_dst /* local */ , vci_src /* remote */);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_REQUEST(rreq, buffer) = buf;
    MPIDIG_REQUEST(rreq, count) = count;
    MPIDIG_REQUEST(rreq, datatype) = datatype;

  fn_exit:
    *vci_src_out = vci_src;
    *vci_dst_out = vci_dst;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_recv_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Convert rreq to an MPIDIG request */
    int vci_src, vci_dst;
    mpi_errno = rndv_event_common(vci, rreq, &vci_src, &vci_dst);
    MPIR_ERR_CHECK(mpi_errno);

    /* prepare rndv_cts */
    struct rndv_cts hdr;
    hdr.rreq = rreq;
    hdr.am_tag = MPIDIG_get_next_am_tag(rreq->comm);
    hdr.flag = MPIDI_OFI_CTS_FLAG__NONE;

    /* am_tag_recv */
    mpi_errno = MPIDI_NM_am_tag_recv(rreq->status.MPI_SOURCE, rreq->comm,
                                     MPIDIG_TAG_RECV_COMPLETE, hdr.am_tag,
                                     MPIDIG_REQUEST(rreq, buffer),
                                     MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype),
                                     vci_src, vci_dst, rreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* send cts */
    mpi_errno = MPIDI_OFI_send_ack(rreq, &hdr, sizeof(hdr));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_peek_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

/* ---- sender side ---- */

int MPIDI_OFI_rndv_cts_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_ack_request_t *ack_req = (MPIDI_OFI_ack_request_t *) req;

    MPIR_Request *sreq = ack_req->signal_req;
    struct rndv_cts *hdr = ack_req->ack_hdr;

    /* sreq is already an MPIDIG request (ref. ofi_send.h) */
    if (hdr->flag == MPIDI_OFI_CTS_FLAG__NONE) {
        /* issue MPIDI_NM_am_tag_send, ref. MPIDIG_send_cts_target_msg_cb */
        mpi_errno = MPIDI_NM_am_tag_send(MPIDIG_REQUEST(sreq, u.send.dest), sreq->comm,
                                         MPIDIG_SEND_DATA, hdr->am_tag,
                                         MPIDIG_REQUEST(sreq, buffer),
                                         MPIDIG_REQUEST(sreq, count),
                                         MPIDIG_REQUEST(sreq, datatype),
                                         MPIDIG_REQUEST(sreq, req->local_vci),
                                         MPIDIG_REQUEST(sreq, req->remote_vci), sreq);

        MPL_free(ack_req->ack_hdr);
        MPL_free(ack_req);
    } else {
        MPIR_Assert(0);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
