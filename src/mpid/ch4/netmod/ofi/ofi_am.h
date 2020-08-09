/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_H_INCLUDED
#define OFI_AM_H_INCLUDED
#include "ch4_impl.h"
#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"
#include "mpidu_genq.h"

static inline int MPIDI_OFI_progress_do_queue(int vni_idx);

static inline void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIDI_OFI_AMREQUEST(req, req_hdr) = NULL;
}

static inline void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_OFI_am_clear_request(req);
}

static inline int MPIDI_NM_am_isend(int rank,
                                    MPIR_Comm * comm,
                                    int handler_id,
                                    const void *am_hdr,
                                    size_t am_hdr_sz,
                                    const void *data,
                                    MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count,
                                            datatype, sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return mpi_errno;
}

static inline int MPIDI_NM_am_isendv(int rank,
                                     MPIR_Comm * comm,
                                     int handler_id,
                                     struct iovec *am_hdr,
                                     size_t iov_len,
                                     const void *data,
                                     MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS, is_allocated;
    size_t am_hdr_sz = 0, i;
    char *am_hdr_buf;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISENDV);

    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (am_hdr_sz > MPIDI_OFI_AM_HDR_POOL_CELL_SIZE) {
        am_hdr_buf = (char *) MPL_malloc(am_hdr_sz, MPL_MEM_BUFFER);
        is_allocated = 1;
    } else {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.am_hdr_buf_pool, (void **) &am_hdr_buf);
        MPIR_Assert(am_hdr_buf);
        is_allocated = 0;
    }

    MPIR_Assert(am_hdr_buf);
    am_hdr_sz = 0;

    for (i = 0; i < iov_len; i++) {
        MPIR_Memcpy(am_hdr_buf + am_hdr_sz, am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr_buf, am_hdr_sz, data,
                                            count, datatype, sreq);

    if (is_allocated)
        MPL_free(am_hdr_buf);
    else
        MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.am_hdr_buf_pool, am_hdr_buf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISENDV);
    return mpi_errno;
}

static inline int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id,
                                          int src_rank,
                                          int handler_id,
                                          const void *am_hdr,
                                          size_t am_hdr_sz,
                                          const void *data,
                                          MPI_Count count,
                                          MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);

    mpi_errno = MPIDI_OFI_do_am_isend_eager(src_rank, MPIDIG_context_id_to_comm(context_id),
                                            handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                            sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    return mpi_errno;
}

static inline int MPIDI_NM_am_isend_pipeline_rts(int rank, MPIR_Comm * comm, int handler_id,
                                                 const void *am_hdr, size_t am_hdr_sz,
                                                 const void *data, MPI_Count count,
                                                 MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_PIPELINE_RTS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_PIPELINE_RTS);

    mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count,
                                            datatype, sreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_PIPELINE_RTS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_isend_pipeline_seg(MPIR_Context_id_t context_id, int src_rank,
                                                 int handler_id, const void *am_hdr,
                                                 size_t am_hdr_sz, const void *data,
                                                 MPI_Count count, MPI_Datatype datatype,
                                                 MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_PIPELINE_SEG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_PIPELINE_SEG);

    mpi_errno = MPIDI_OFI_do_am_isend_pipeline(src_rank, MPIDIG_context_id_to_comm(context_id),
                                               handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                               sreq);


    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_PIPELINE_SEG);
    return mpi_errno;
}

static inline int MPIDI_NM_am_isend_rdma_read_req(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz,
                                                  const void *data, MPI_Count count,
                                                  MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_RDMA_READ_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_RDMA_READ_REQ);

    mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(rank, comm, handler_id, am_hdr, am_hdr_sz, data,
                                                count, datatype, sreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_RDMA_READ_REQ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_recv_rdma_read(void *lmt_msg, size_t recv_data_sz,
                                             MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    int dt_contig = 0;
    size_t data_sz = 0;
    MPI_Aint dt_true_lb = 0;
    MPIR_Datatype *dt_ptr = NULL;
    MPL_pointer_attr_t attr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_AM_RECV_RDMA_READ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_AM_RECV_RDMA_READ);

    /* check if receive buffer is okay for rdma_read */
    MPIDI_Datatype_get_info(MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype), dt_contig,
                            data_sz, dt_ptr, dt_true_lb);
    MPIR_GPU_query_pointer_attr((char *) MPIDIG_REQUEST(rreq, buffer) + dt_true_lb, &attr);
    if (!dt_contig || attr.type == MPL_GPU_POINTER_DEV) {
        MPIDIG_do_rdma_read_nak(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_am_clear_request(rreq);
    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_cc_incr(rreq->cc_ptr, &c);

    if (!recv_data_sz) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPIR_Request_complete(rreq);
    }

    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = (*(MPIDI_OFI_lmt_msg_payload_t *) lmt_msg);
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;

    do_long_am_recv(recv_data_sz, rreq, lmt_msg);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_AM_RECV_RDMA_READ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_rdma_read_unreg(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_RDMA_READ_UNREG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_RDMA_READ_UNREG);

    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        uint64_t mr_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
        MPIDI_OFI_mr_key_free(mr_key);
    }
    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr)->fid), mr_unreg);
    MPL_atomic_fetch_sub_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs, 1);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_RDMA_READ_UNREG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline size_t MPIDI_NM_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */
    size_t max_shortsend = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE -
        (sizeof(MPIDI_OFI_am_header_t) + sizeof(MPIDI_OFI_lmt_msg_payload_t));
    /* Maximum payload size representable by MPIDI_OFI_am_header_t::am_hdr_sz field */
    size_t max_representable = (1 << MPIDI_OFI_AM_HDR_SZ_BITS) - 1;

    return MPL_MIN(max_shortsend, max_representable);
}

static inline size_t MPIDI_NM_am_eager_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t);
}

static inline size_t MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;
}

static inline int MPIDI_NM_am_send_hdr(int rank,
                                       MPIR_Comm * comm,
                                       int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    mpi_errno = MPIDI_OFI_do_inject(rank, comm, handler_id, am_hdr, am_hdr_sz);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                             int src_rank,
                                             int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);

    mpi_errno = MPIDI_OFI_do_inject(src_rank, MPIDIG_context_id_to_comm(context_id), handler_id,
                                    am_hdr, am_hdr_sz);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_choose_protocol(const void *buf, MPI_Count count,
                                              MPI_Datatype datatype, size_t am_ext_sz,
                                              int handler_id)
{
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    int dt_contig;
    size_t data_sz;
    MPL_pointer_attr_t attr;

    MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);
    MPIR_GPU_query_pointer_attr((char *) buf, &attr);

    if (!MPIDIG_am_check_size_le_eager_limit
        (data_sz + am_ext_sz, handler_id, MPIDI_NM_am_eager_limit())) {
        if (!dt_contig || attr.type == MPL_GPU_POINTER_DEV) {
            protocol = MPIDIG_AM_PROTOCOL__PIPELINE;
        } else {
            // protocol = MPIDIG_AM_PROTOCOL__RDMA_READ;
            protocol = MPIDIG_AM_PROTOCOL__PIPELINE;
        }
    }

    return protocol;
}

#endif /* OFI_AM_H_INCLUDED */
