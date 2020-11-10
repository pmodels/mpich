/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_H_INCLUDED
#define OFI_AM_H_INCLUDED
#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"
#include "mpidu_genq.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE
      category    : DEVELOPER
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        For long message to be sent using pipeline rather than default
        RDMA read.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vni_idx);

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIDI_OFI_AMREQUEST(req, req_hdr) = NULL;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_OFI_am_clear_request(req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_prepare_send(int handler_id, const void *buf,
                                                      MPI_Count count, MPI_Datatype datatype,
                                                      const void *am_hdr, MPI_Aint am_hdr_sz,
                                                      void **ext_hdr, MPI_Aint * ext_hdr_sz,
                                                      MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    int dt_contig = 0;
    MPI_Aint data_sz = 0;
    MPL_pointer_attr_t attr;
    void *data = NULL;
    MPI_Aint dt_true_lb = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_PREPARE_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_PREPARE_SEND);

    MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);
    MPIR_GPU_query_pointer_attr(buf, &attr);

    if (!dt_contig || (am_hdr_sz + data_sz <= MPIDI_NM_am_eager_limit())
        || (attr.type == MPL_GPU_POINTER_DEV && !MPIDI_OFI_ENABLE_HMEM)) {
        /* smaller than eager, noncontig or GPU buffer needs packing, do not do zcopy */
        *ext_hdr = NULL;
        *ext_hdr_sz = 0;
        goto fn_exit;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "sreq handle=0x%x send ZCOPY", sreq->handle));
    /* 1. Create memory for am_hdr + lmt_info and copy the am_hdr */
    void *header = MPL_malloc(am_hdr_sz + sizeof(MPIDI_OFI_lmt_msg_payload_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDSTMT((header) == NULL, mpi_errno, MPI_ERR_OTHER, goto fn_fail, "**nomem");
    MPIR_Memcpy(header, am_hdr, am_hdr_sz);

    MPIDI_OFI_am_init_request(am_hdr, am_hdr_sz, sreq);
    MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
    MPIDI_OFI_AMREQUEST_HDR(sreq, ext_hdr) = header;
    /* also save handler id in request */
    MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr).handler_id = handler_id;

    /* 2. register buffer */
    MPIDI_OFI_lmt_msg_payload_t *lmt_info =
        (MPIDI_OFI_lmt_msg_payload_t *) ((char *) header + am_hdr_sz);

    MPIDI_Datatype_check_lb(datatype, dt_true_lb);
    data = (char *) buf + dt_true_lb;

    lmt_info->src_offset =
        !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS ? (uint64_t) 0 : (uint64_t) (uintptr_t) data;

    lmt_info->sreq_ptr = sreq;
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        lmt_info->rma_key = MPIDI_OFI_mr_key_alloc();
    } else {
        lmt_info->rma_key = 0;
    }
    lmt_info->reg_sz = data_sz;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "lmt_info.data_sz %ld", lmt_info->reg_sz));

    MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[0].domain,
                             data,
                             data_sz,
                             FI_REMOTE_READ,
                             0ULL,
                             lmt_info->rma_key,
                             0ULL, &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr), NULL), mr_reg);
    MPL_atomic_fetch_add_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs, 1);

    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* MR_BASIC */
        lmt_info->rma_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
    }

    MPIR_cc_incr(sreq->cc_ptr, &c);     /* lmt ack handler */

    *ext_hdr = header;
    *ext_hdr_sz = am_hdr_sz + sizeof(MPIDI_OFI_lmt_msg_payload_t);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_PREPARE_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               size_t am_hdr_sz,
                                               const void *data,
                                               MPI_Count count, MPI_Datatype datatype,
                                               MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint data_sz = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    MPIDI_Datatype_check_size(datatype, count, data_sz);
    /* TODO: check for RDMA read too */
    if (data_sz + am_hdr_sz <= MPIDI_NM_am_eager_limit()) {
        /* EAGER */
        mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz, data,
                                                count, datatype, sreq,
                                                false /* not for issue deferred */);
    } else {
        if (MPIDI_OFI_ENABLE_RMA && !MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE) {
            /* RDMA READ */
            mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                        data, count, datatype, sreq,
                                                        false /* not for issue deferred */);
        } else {
            /* PIPELINE */
            mpi_errno = MPIDI_OFI_do_am_isend_pipeline(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                       data, count, datatype, sreq, data_sz,
                                                       false /* not for issue deferred */);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank,
                                                MPIR_Comm * comm,
                                                int handler_id,
                                                struct iovec *am_hdr,
                                                size_t iov_len,
                                                const void *data,
                                                MPI_Count count, MPI_Datatype datatype,
                                                MPIR_Request * sreq)
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

    mpi_errno = MPIDI_NM_am_isend(rank, comm, handler_id, am_hdr_buf, am_hdr_sz, data,
                                  count, datatype, sreq);

    if (is_allocated)
        MPL_free(am_hdr_buf);
    else
        MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.am_hdr_buf_pool, am_hdr_buf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISENDV);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id,
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

    mpi_errno = MPIDI_NM_am_isend(src_rank, MPIDIG_context_id_to_comm(context_id), handler_id,
                                  am_hdr, am_hdr_sz, data, count, datatype, sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */
    size_t max_shortsend = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE -
        (sizeof(MPIDI_OFI_am_header_t) + sizeof(MPIDI_OFI_lmt_msg_payload_t));
    /* Maximum payload size representable by MPIDI_OFI_am_header_t::am_hdr_sz field */
    return MPL_MIN(max_shortsend, MPIDI_OFI_MAX_AM_HDR_SIZE);
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_eager_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t);
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id, const void *am_hdr,
                                                  size_t am_hdr_sz)
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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank,
                                                        int handler_id, const void *am_hdr,
                                                        size_t am_hdr_sz)
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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_recv(int src_rank, MPIR_Context_id_t context_id,
                                              const void *ext_hdr, MPI_Aint ext_hdr_sz,
                                              MPIR_Request * rreq)
{
    int ret = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_RECV);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_RECV);
    return ret;
}

#endif /* OFI_AM_H_INCLUDED */
