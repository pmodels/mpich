/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_am_events.h"
#include "ofi_events.h"
#include "ofi_rndv.h"

#define MPIDI_OFI_CTS_FLAG__NONE   0
#define MPIDI_OFI_CTS_FLAG__PROBE  1
#define MPIDI_OFI_CTS_FLAG__NEED_PACK 2

static bool cts_is_probe(int flag)
{
    return (flag & MPIDI_OFI_CTS_FLAG__PROBE);
}

static int get_rndv_protocol(bool send_need_pack, bool recv_need_pack, MPI_Aint recv_data_sz)
{
    /* NOTE: some protocols may not work, fallback to auto */
    switch (MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL) {
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_pipeline:
            return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_pipeline;
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_read:
            if (!send_need_pack) {
                return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_read;
            }
            break;
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_write:
            if (!recv_need_pack) {
                return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_write;
            }
            break;
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_direct:
            /* libfabric can't direct send > max_msg_size. Only sender knows both sizes from
             * receiving CTS, thus we use recv_data_sz so both sides can agree.
             * NOTE: psm3 has max_msg_size at 4294963200.
             */
            if (recv_data_sz <= MPIDI_OFI_global.max_msg_size) {
                return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_direct;
            }
            break;
    }

    /* auto */
    if (send_need_pack && recv_need_pack) {
        return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_pipeline;
    } else if (send_need_pack) {
        return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_write;
    } else if (recv_need_pack) {
        return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_read;
    } else if (recv_data_sz < MPIDI_OFI_global.max_msg_size) {
        return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_direct;
    } else {
        return MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_read;
    }
}

/* receiver -> sender */
struct rndv_cts {
    MPIR_Request *rreq;
    int am_tag;
    int flag;
    MPI_Aint data_sz;
};

/* sender -> receiver */
struct rndv_info_hdr {
    MPIR_Request *sreq;
    MPIR_Request *rreq;
    MPI_Aint data_sz;
};

/* ---- receiver side ---- */

int MPIDI_OFI_recv_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* save and free up the OFI request */
    void *buf = MPIDI_OFI_REQUEST(rreq, buf);
    MPI_Aint count = MPIDI_OFI_REQUEST(rreq, count);
    MPI_Datatype datatype = MPIDI_OFI_REQUEST(rreq, datatype);;

    /* if we were expecting an eager send, free the unneeded pack_buffer or iovs array */
    switch (MPIDI_OFI_REQUEST(rreq, event_id)) {
        case MPIDI_OFI_EVENT_RECV_PACK:
            MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer));
            break;
        case MPIDI_OFI_EVENT_RECV_NOPACK:
            MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack.iovs));
            break;
    }

    int dt_contig;
    MPIR_Datatype_is_contig(datatype, &dt_contig);

    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(buf, &attr);

    MPI_Aint data_sz;
    MPIR_Datatype_get_size_macro(datatype, data_sz);
    data_sz *= count;

    MPIR_Comm *comm = rreq->comm;
    int src_rank = rreq->status.MPI_SOURCE;
    /* load these field before we overwrite them */
    int context_id = MPIDI_OFI_REQUEST(rreq, context_id);
    int vci_remote = MPIDI_OFI_REQUEST(rreq, vci_remote);
    int vci_local = MPIDI_OFI_REQUEST(rreq, vci_local);
    if (vci_remote == -1) {
        /* MPI_ANY_SOURCE */
        vci_remote = MPIDI_get_vci(SRC_VCI_FROM_RECVER, rreq->comm, rreq->status.MPI_SOURCE,
                                   rreq->comm->rank, rreq->status.MPI_TAG);
    }

    MPIDI_OFI_rndv_common_t *p = &MPIDI_OFI_AMREQ_COMMON(rreq);
    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->need_pack = MPIDI_OFI_rndv_need_pack(dt_contig, &attr);
    p->attr = attr;
    p->data_sz = data_sz;
    p->vci_local = vci_local;
    p->vci_remote = vci_remote;
    p->av = MPIDIU_comm_rank_to_av(comm, src_rank);

    MPI_Aint remote_data_sz = MPIDI_OFI_idata_get_size(wc->data);
    if (remote_data_sz > 0) {
        p->remote_data_sz = remote_data_sz;
        MPIDI_OFI_RNDV_update_count(rreq, remote_data_sz);
    } else {
        /* mark remote_data_sz as unknown */
        p->remote_data_sz = -1;
    }

    int am_tag = MPIDIG_get_next_am_tag(comm, src_rank);
    p->match_bits = MPIDI_OFI_init_sendtag(comm->recvcontext_id, 0, am_tag) | MPIDI_OFI_AM_SEND;

    bool send_need_pack = MPIDI_OFI_is_tag_rndv_pack(wc->tag);
    bool recv_need_pack = p->need_pack;

    /* prepare rndv_cts */
    struct rndv_cts hdr;
    hdr.rreq = rreq;
    hdr.am_tag = am_tag;
    hdr.flag = MPIDI_OFI_CTS_FLAG__NONE;
    hdr.data_sz = data_sz;
    if (recv_need_pack) {
        hdr.flag |= MPIDI_OFI_CTS_FLAG__NEED_PACK;
    }

    switch (get_rndv_protocol(send_need_pack, recv_need_pack, p->data_sz)) {
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_pipeline:
            mpi_errno = MPIDI_OFI_pipeline_recv(rreq, hdr.am_tag, vci_remote, vci_local);
            break;
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_read:
            mpi_errno = MPIDI_OFI_rndvread_recv(rreq, hdr.am_tag, vci_remote, vci_local);
            break;
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_write:
            mpi_errno = MPIDI_OFI_rndvwrite_recv(rreq, hdr.am_tag, vci_remote, vci_local);
            break;
        case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_direct:
            /* fall through */
        default:
            mpi_errno = MPIDI_NM_am_tag_recv(rreq->status.MPI_SOURCE, rreq->comm,
                                             -1, hdr.am_tag,
                                             (void *) p->buf, p->count, p->datatype,
                                             vci_remote, vci_local, rreq);
    }
    MPIR_ERR_CHECK(mpi_errno);

    /* send cts */
    mpi_errno = MPIDI_OFI_send_ack(rreq, context_id, &hdr, sizeof(hdr), vci_local, vci_remote);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

/* if we probe an rndv message, we need ask sender for additional info (message size) */
int MPIDI_OFI_peek_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint data_sz;
    data_sz = MPIDI_OFI_idata_get_size(wc->data);

    if (data_sz > 0) {
        /* complete probe */
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
        MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, peek_status)), MPIDI_OFI_PEEK_FOUND);
    } else {
        /* ask sender for data_sz */
        struct rndv_cts hdr;
        hdr.rreq = rreq;
        hdr.am_tag = -1;        /* don't issue am_tag_recv yet */
        hdr.flag = MPIDI_OFI_CTS_FLAG__PROBE;
        hdr.data_sz = 0;

        int context_id = MPIDI_OFI_REQUEST(rreq, context_id);
        int vci_remote = MPIDI_OFI_REQUEST(rreq, vci_remote);
        int vci_local = MPIDI_OFI_REQUEST(rreq, vci_local);

        mpi_errno = MPIDI_OFI_send_ack(rreq, context_id, &hdr, sizeof(hdr), vci_local, vci_remote);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* received the additional info that we asked for (from probe), continue  */
int MPIDI_OFI_rndv_info_handler(void *am_hdr, void *data, MPI_Aint in_data_sz, uint32_t attr,
                                MPIR_Request ** req)
{
    MPIR_FUNC_ENTER;

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

    struct rndv_info_hdr *hdr = am_hdr;
    MPIR_Request *rreq = hdr->rreq;

    /* complete probe */
    MPIR_STATUS_SET_COUNT(rreq->status, hdr->data_sz);

    MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, peek_status)), MPIDI_OFI_PEEK_FOUND);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

/* ---- sender side ---- */

int MPIDI_OFI_rndv_cts_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_ack_request_t *ack_req = (MPIDI_OFI_ack_request_t *) r;

    MPIR_Request *sreq = ack_req->signal_req;
    struct rndv_cts *hdr = ack_req->ack_hdr;
    MPIDI_OFI_rndv_common_t *p = &MPIDI_OFI_AMREQ_COMMON(sreq);

    p->remote_data_sz = hdr->data_sz;
    p->match_bits =
        MPIDI_OFI_init_sendtag(sreq->comm->context_id, 0, hdr->am_tag) | MPIDI_OFI_AM_SEND;

    if (!cts_is_probe(hdr->flag)) {
        bool send_need_pack = p->need_pack;
        bool recv_need_pack = hdr->flag & MPIDI_OFI_CTS_FLAG__NEED_PACK;

        switch (get_rndv_protocol(send_need_pack, recv_need_pack, hdr->data_sz)) {
            case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_pipeline:
                mpi_errno = MPIDI_OFI_pipeline_send(sreq, hdr->am_tag);
                break;
            case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_read:
                mpi_errno = MPIDI_OFI_rndvread_send(sreq, hdr->am_tag);
                break;
            case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_write:
                mpi_errno = MPIDI_OFI_rndvwrite_send(sreq, hdr->am_tag);
                break;
            case MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL_direct:
                /* fall through */
            default:
                if (p->data_sz < MPIDI_OFI_global.max_msg_size) {
                    mpi_errno = MPIDI_NM_am_tag_send(p->remote_rank, sreq->comm, -1, hdr->am_tag,
                                                     p->buf, p->count, p->datatype,
                                                     p->vci_local, p->vci_remote, sreq);
                } else {
                    /* Only contig data here (if this ever change, FIXME) -
                     * Send max_msg_size and receiver will get the truncation error.*/
                    MPI_Aint true_extent, true_lb;
                    MPIR_Type_get_true_extent_impl(p->datatype, &true_lb, &true_extent);
                    void *data = MPIR_get_contig_ptr(p->buf, true_lb);
                    mpi_errno = MPIDI_NM_am_tag_send(p->remote_rank, sreq->comm, -1, hdr->am_tag,
                                                     data, MPIDI_OFI_global.max_msg_size,
                                                     MPIR_BYTE_INTERNAL,
                                                     p->vci_local, p->vci_remote, sreq);
                }
        }
        MPL_free(ack_req->ack_hdr);
        MPL_free(ack_req);
    } else {
        /* re-issue the ack recv */
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ack_req->ctx_idx].rx,
                                      ack_req->ack_hdr, ack_req->ack_hdr_sz, NULL,
                                      ack_req->remote_addr, ack_req->match_bits, 0ULL,
                                      (void *) &(ack_req->context)), ack_req->vci_local, trecvsync);

        /* just reply with data_sz. Receiver will send another CTS */
        struct rndv_info_hdr rndv_info;
        rndv_info.sreq = sreq;
        rndv_info.rreq = hdr->rreq;
        rndv_info.data_sz = p->data_sz;
        mpi_errno = MPIDI_NM_am_send_hdr_reply(sreq->comm, p->remote_rank,
                                               MPIDI_OFI_RNDV_INFO, &rndv_info, sizeof(rndv_info),
                                               p->vci_local, p->vci_remote);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
