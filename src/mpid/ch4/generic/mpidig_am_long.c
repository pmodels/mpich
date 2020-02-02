#include "mpidimpl.h"

static int MPIDIG_send_long_req_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                              size_t * p_data_sz, int is_local, int *is_contig,
                                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                              MPIR_Request ** req);
int MPIDIG_do_long_ack(MPIR_Request * rreq);
static int MPIDIG_send_long_ack_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                              size_t * p_data_sz, int is_local, int *is_contig,
                                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                              MPIR_Request ** req);
static int do_long_lmt(int is_local, MPIR_Request * sreq, MPIR_Request * rreq);
static int MPIDIG_send_long_lmt_origin_cb(MPIR_Request * sreq);
static int MPIDIG_send_long_lmt_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                              size_t * p_data_sz, int is_local, int *is_contig,
                                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                              MPIR_Request ** req);

int MPIDIG_am_long_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_REQ, NULL /* Injection only */ ,
                                 &MPIDIG_send_long_req_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_ACK, NULL /* Injection only */ ,
                                 &MPIDIG_send_long_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SEND_LONG_LMT,
                                 &MPIDIG_send_long_lmt_origin_cb,
                                 &MPIDIG_send_long_lmt_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_SEND_LONG_REQ */
struct am_long_req_hdr {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
    int error_bits;
    size_t data_sz;             /* Message size in bytes */
    MPIR_Request *sreq_ptr;     /* Pointer value of the request object at the sender side */
};

int MPIDIG_do_long(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                   int rank, int tag, MPIR_Comm * comm, int context_offset,
                   MPIDI_av_entry_t * addr, MPIR_Request * sreq, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_long_req_hdr am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;
    /* pointer to sreq is needed for tracking the ack */
    am_hdr.sreq_ptr = sreq;
    /* data_sz */
    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_size_dt_ptr(count, datatype, data_sz, dt_ptr);
    am_hdr.data_sz = data_sz;

    /* send-side request */
    MPIDIG_REQUEST(sreq, req->lreq).src_buf = buf;
    MPIDIG_REQUEST(sreq, req->lreq).count = count;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDIG_REQUEST(sreq, req->lreq).datatype = datatype;
    MPIDIG_REQUEST(sreq, req->lreq).tag = tag;
    MPIDIG_REQUEST(sreq, req->lreq).rank = am_hdr.src_rank;
    MPIDIG_REQUEST(sreq, req->lreq).context_id = am_hdr.context_id;
    MPIDIG_REQUEST(sreq, rank) = rank;

    MPIDIG_AM_SEND_HDR(MPIDI_rank_is_local(rank, comm), rank, comm, MPIDIG_SEND_LONG_REQ);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_send_long_req_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                              size_t * p_data_sz, int is_local, int *is_contig,
                                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                              MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_long_req_hdr *hdr = am_hdr;

    int got_match;
    size_t data_sz = p_data_sz ? *p_data_sz : 0;
    mpi_errno = MPIDIG_match_msg(hdr->src_rank, hdr->tag, hdr->context_id, hdr->error_bits,
                                 data_sz, is_local, &got_match, req);
    MPIR_ERR_CHECK(mpi_errno);

    if (got_match) {
        MPIR_Request *rreq = *req;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_LONG_RTS;
        MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;
        /* Mark `match_req` as NULL so that we know nothing else to complete when
         * `unexp_req` finally completes. (See MPIDI_recv_target_cmpl_cb) */
        MPIDIG_REQUEST(rreq, req->rreq.match_req) = NULL;

        mpi_errno = MPIDIG_do_long_ack(rreq);
        MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (unlikely(MPIDI_REQUEST_ANYSOURCE_PARTNER(rreq))) {
            anysrc_free_partner(rreq);
        }
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDIG_SEND_LONG_ACK */
struct am_long_ack_hdr {
    MPIR_Request *sreq_ptr;
    MPIR_Request *rreq_ptr;
};

int MPIDIG_do_long_ack(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_long_ack_hdr am_hdr;
    am_hdr.sreq_ptr = (MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr));
    am_hdr.rreq_ptr = rreq;
    MPIR_Assert((void *) am_hdr.sreq_ptr != NULL);

    MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));
    int rank = MPIDIG_REQUEST(rreq, rank);
    MPIDIG_AM_SEND_HDR(MPIDI_REQUEST(rreq, is_local), rank, comm, MPIDIG_SEND_LONG_ACK);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_request_complete(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

static int MPIDIG_send_long_ack_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                              size_t * p_data_sz, int is_local, int *is_contig,
                                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                              MPIR_Request ** req)
{
    struct am_long_ack_hdr *hdr = am_hdr;
    MPIR_Assert(hdr->sreq_ptr != NULL);

    if (target_cmpl_cb)
        *target_cmpl_cb = MPIDIG_request_complete;

    return do_long_lmt(is_local, hdr->sreq_ptr, hdr->rreq_ptr);

}

/* MPIDIG_SEND_LONG_LMT */
struct am_long_lmt_hdr {
    MPIR_Request *rreq_ptr;
};

static int do_long_lmt(int is_local, MPIR_Request * sreq, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_long_lmt_hdr am_hdr;
    am_hdr.rreq_ptr = rreq;

    int rank = MPIDIG_REQUEST(sreq, rank);
    MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(sreq, context_id));
    const void *buf = MPIDIG_REQUEST(sreq, req->lreq).src_buf;
    MPI_Aint count = MPIDIG_REQUEST(sreq, req->lreq).count;
    MPI_Datatype datatype = MPIDIG_REQUEST(sreq, req->lreq).datatype;
    MPIDIG_AM_SEND(is_local, rank, comm, MPIDIG_SEND_LONG_LMT, buf, count, datatype, sreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_send_long_lmt_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, req->lreq).datatype);
    MPID_Request_complete(sreq);
    return mpi_errno;
}

static int MPIDIG_send_long_lmt_target_msg_cb(int handler_id, void *am_hdr, void **data,
                                              size_t * p_data_sz, int is_local, int *is_contig,
                                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                              MPIR_Request ** req)
{
    int mpi_errno;

    struct am_long_lmt_hdr *hdr = am_hdr;
    MPIR_Request *rreq = hdr->rreq_ptr;
    MPIR_Assert(rreq);
    mpi_errno = do_send_target(data, p_data_sz, is_contig, target_cmpl_cb, rreq);
    *req = rreq;

    return mpi_errno;
}
