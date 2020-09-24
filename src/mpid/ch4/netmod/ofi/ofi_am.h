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

#endif /* OFI_AM_H_INCLUDED */
