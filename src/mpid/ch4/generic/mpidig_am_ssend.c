#include "mpidimpl.h"

struct am_ssend_hdr {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
    int error_bits;
    MPIR_Request *sreq_ptr;
};

static int MPIDIG_ssend_origin_cb(MPIR_Request * sreq);
static int MPIDIG_ssend_target_msg_cb(int handler_id, void *am_hdr,
                                      void **data, size_t * p_data_sz,
                                      int is_local, int *is_contig,
                                      MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                      MPIR_Request ** req);
static int MPIDIG_ssend_ack_origin_cb(MPIR_Request * req);
static int MPIDIG_ssend_ack_target_msg_cb(int handler_id, void *am_hdr,
                                          void **data, size_t * p_data_sz,
                                          int is_local, int *is_contig,
                                          MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                          MPIR_Request ** req);

int MPIDIG_am_ssend_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SSEND_REQ,
                                 &MPIDIG_ssend_origin_cb, &MPIDIG_ssend_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDIG_am_reg_cb(MPIDIG_SSEND_ACK,
                                 &MPIDIG_ssend_ack_origin_cb, &MPIDIG_ssend_ack_target_msg_cb);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_do_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int context_offset,
                    MPIDI_av_entry_t * addr, MPIR_Request * sreq, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_ssend_hdr am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;
    /* pointer to sreq is needed for tracking the ack */
    am_hdr.sreq_ptr = sreq;
    /* Increment the completion counter once to account for the extra message that needs to come
     * back from the receiver to indicate completion. */
    int c;
    MPIR_cc_incr(sreq->cc_ptr, &c);

    MPIDIG_AM_SEND(MPIDI_av_is_local(addr), rank, comm, MPIDIG_SSEND_REQ,
                   buf, count, datatype, sreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDIG_ssend_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

int MPIDIG_ssend_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                               int is_local, int *is_contig,
                               MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_ssend_hdr *hdr = (struct am_ssend_hdr *) am_hdr;

    mpi_errno = MPIDIG_handle_send(hdr->src_rank, hdr->tag, hdr->context_id, hdr->error_bits,
                                   data, p_data_sz, is_local, is_contig, target_cmpl_cb, req);
    MPIR_ERR_CHECK(mpi_errno);

    /* reply potentially will be sent asynchronously due to matching complications */
    MPIR_Assert(req);
    MPIDIG_REQUEST(*req, req->rreq.peer_req_ptr) = hdr->sreq_ptr;
    MPIDIG_REQUEST(*req, req->status) |= MPIDIG_REQ_PEER_SSEND;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ssend ack */
struct am_ssend_ack_hdr {
    MPIR_Request *sreq_ptr;
};

int MPIDIG_reply_ssend(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_ssend_ack_hdr am_hdr;
    am_hdr.sreq_ptr = MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr);

    /* add ref for origin_cb */
    int c;
    MPIR_cc_incr(rreq->cc_ptr, &c);

    MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));
    int src_rank = MPIDIG_REQUEST(rreq, rank);
    MPIDIG_AM_SEND(MPIDI_REQUEST(rreq, is_local), src_rank, comm, MPIDIG_SSEND_ACK,
                   NULL, 0, MPI_DATATYPE_NULL, rreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_ssend_ack_origin_cb(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(req);
    return mpi_errno;
}

int MPIDIG_ssend_ack_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                   int is_local, int *is_contig,
                                   MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_ssend_ack_hdr *hdr = am_hdr;
    MPIR_Request *sreq = hdr->sreq_ptr;
    MPID_Request_complete(sreq);

    if (req)
        *req = NULL;
    if (target_cmpl_cb)
        *target_cmpl_cb = NULL;

    return mpi_errno;
}
