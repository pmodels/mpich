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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vci_idx);

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIDI_OFI_AMREQUEST(req, sreq_hdr) = NULL;
    MPIDI_OFI_AMREQUEST(req, rreq_hdr) = NULL;
    MPIDI_OFI_AMREQUEST(req, am_type_choice) = MPIDI_AMTYPE_NONE;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_OFI_am_clear_request(req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               MPI_Aint am_hdr_sz,
                                               const void *data,
                                               MPI_Aint count, MPI_Datatype datatype,
                                               int vci_src, int vci_dst, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint data_sz = 0;
    MPIR_FUNC_ENTER;

    switch (MPIDI_OFI_AMREQUEST(sreq, am_type_choice)) {
        case MPIDI_AMTYPE_NONE:
            /* if no preselected amtype, do check here */
            MPIDI_Datatype_check_size(datatype, count, data_sz);
            if (data_sz + am_hdr_sz <= MPIDI_NM_am_eager_limit()) {
                /* EAGER */
                mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                        data, count, datatype, sreq, false, vci_src,
                                                        vci_dst);
            } else {
                if (MPIDI_OFI_ENABLE_RMA && !MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE) {
                    /* RDMA READ */
                    mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(rank, comm, handler_id, am_hdr,
                                                                am_hdr_sz, data, count, datatype,
                                                                sreq, false, vci_src, vci_dst);
                } else {
                    /* PIPELINE */
                    mpi_errno = MPIDI_OFI_do_am_isend_pipeline(rank, comm, handler_id, am_hdr,
                                                               am_hdr_sz, data, count, datatype,
                                                               sreq, data_sz, false, vci_src,
                                                               vci_dst);
                }
            }
            break;
        case MPIDI_AMTYPE_SHORT_HDR:
            MPIR_Assert(0);     /* header only should go to the send hdr interface */
            break;
        case MPIDI_AMTYPE_SHORT:
            mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz, data,
                                                    count, datatype, sreq, false, vci_src, vci_dst);
            /* cleanup preselected amtype to avoid problem with reused request */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_NONE;
            break;
        case MPIDI_AMTYPE_PIPELINE:
            mpi_errno = MPIDI_OFI_do_am_isend_pipeline(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                       data, count, datatype, sreq,
                                                       MPIDI_OFI_AMREQUEST(sreq, data_sz), false,
                                                       vci_src, vci_dst);
            /* cleanup preselected amtype to avoid problem with reused request */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_NONE;
            break;
        case MPIDI_AMTYPE_RDMA_READ:
            mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                        data, count, datatype, sreq, false, vci_src,
                                                        vci_dst);
            /* cleanup preselected amtype to avoid problem with reused request */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_NONE;
            break;
        default:
            MPIR_Assert(0);     /* header only should go to the send hdr interface */
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Comm * comm,
                                                     int src_rank,
                                                     int handler_id,
                                                     const void *am_hdr,
                                                     MPI_Aint am_hdr_sz,
                                                     const void *data,
                                                     MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_am_isend(src_rank, comm, handler_id,
                                  am_hdr, am_hdr_sz, data, count, datatype, src_vci, dst_vci, sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */
    MPI_Aint max_shortsend = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE -
        (sizeof(MPIDI_OFI_am_header_t) + sizeof(MPIDI_OFI_lmt_msg_payload_t));
    /* Maximum payload size representable by MPIDI_OFI_am_header_t::am_hdr_sz field */
    return MPL_MIN(max_shortsend, MPIDI_OFI_MAX_AM_HDR_SIZE);
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t);
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id, const void *am_hdr,
                                                  MPI_Aint am_hdr_sz, int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_OFI_do_inject(rank, comm, handler_id, am_hdr, am_hdr_sz, vci_src, vci_dst);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Comm * comm,
                                                        int src_rank,
                                                        int handler_id, const void *am_hdr,
                                                        MPI_Aint am_hdr_sz,
                                                        int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno =
        MPIDI_OFI_do_inject(src_rank, comm, handler_id, am_hdr, am_hdr_sz, src_vci, dst_vci);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_tag_send(int rank, MPIR_Comm * comm,
                                                  int handler_id, int tag,
                                                  const void *data, MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* FIXME: assert data_sz <= MPIDI_OFI_global.max_msg_size */
    uint64_t match_bits;
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id, comm->rank, tag, MPIDI_OFI_AM_TAG_SEND);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    int vci_local = src_vci;
    int vci_remote = dst_vci;
    /* Calculate the correct NICs. */
    sender_nic =
        MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    MPIDI_OFI_REQUEST(sreq, nic_num) = sender_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, MPIDI_OFI_REQUEST(sreq, nic_num));

    int dt_contig;
    MPI_Aint data_sz, dt_true_lb;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);

    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(send_buf, &attr);

    if (MPIDI_OFI_ENABLE_HMEM && data_sz >= MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD &&
        MPIDI_OFI_ENABLE_MR_HMEM && dt_contig && attr.type == MPL_GPU_POINTER_DEV) {
        register_mem = true;
    }

    bool do_pack = false;
    if (!dt_contig) {
        /* TODO: impl am_tag_iov_send */
        do_pack = true;
    }

    if (data_sz && MPL_gpu_query_pointer_is_dev(send_buf, &attr)) {
        MPIDI_OFI_register_am_bufs();
        if (!MPIDI_OFI_ENABLE_HMEM || (MPIDI_OFI_ENABLE_MR_HMEM && !register_mem)) {
            do_pack = true;
        }
    }

    if (register_mem) {
        MPIDI_OFI_register_memory_and_bind(send_buf, data_sz, &attr, ctx_idx, &mr);
        if (mr != NULL) {
            desc = fi_mr_desc(mr);
        }
    }

    if (do_pack) {
        /* Pack */
        MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer) = MPL_malloc(data_sz, MPL_MEM_OTHER);
        void *pack_buf = MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer);
        MPIR_ERR_CHKANDJUMP1(pack_buf == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "am_tag_send pack buffer alloc");

        MPIR_cc_inc(sreq->cc_ptr);
        MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND_PACK;

        MPI_Aint packed_size;
        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, 0, pack_buf, data_sz, &packed_size,
                                      MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(packed_size == data_sz);

        int fast_copy = 0;
        if (attr.type == MPL_GPU_POINTER_DEV && dt_contig &&
            data_sz <= MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE) {
            int mpl_err = MPL_gpu_fast_memcpy(send_buf, &attr, pack_buf, NULL, data_sz);
            if (mpl_err == MPL_SUCCESS)
                fast_copy = 1;
        }
        if (!fast_copy) {
            MPL_gpu_engine_type_t engine =
                MPIDI_OFI_gpu_get_send_engine_type(MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE);
            if (dt_contig && engine != MPL_GPU_ENGINE_TYPE_LAST &&
                MPL_gpu_query_pointer_is_dev(send_buf, &attr)) {
                mpi_errno = MPIR_Localcopy_gpu(send_buf, data_sz, MPI_BYTE, 0, &attr,
                                               pack_buf, data_sz, MPI_BYTE, 0, NULL,
                                               MPL_GPU_COPY_DIRECTION_NONE, engine, true);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                MPI_Aint actual_pack_bytes;
                MPIR_Typerep_pack(buf, count, datatype, 0, pack_buf, data_sz,
                                  &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            }
        }

        send_buf = pack_buf;
    }

    fi_addr_t dest_addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);

    MPIDI_OFI_CALL_RETRY_AM(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                     send_buf, data_sz, desc, dest_addr, match_bits,
                                     (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                            vci_local, tsend);
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_NM_am_check_eager(MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                      const void *data, MPI_Aint count,
                                                      MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIDI_OFI_AMREQUEST(sreq, data_sz) = data_sz;
    if ((am_hdr_sz + data_sz)
        <= (MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t))) {
        MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_SHORT;
        return true;
    } else {
        if (MPIDI_OFI_ENABLE_RMA && !MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE) {
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_RDMA_READ;
            return true;
        } else {
            /* Forced PIPELINE */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_PIPELINE;
            return false;
        }
    }
}

MPL_STATIC_INLINE_PREFIX MPIDIG_recv_data_copy_cb MPIDI_NM_am_get_data_copy_cb(uint32_t attr)
{
    MPIR_Assert(attr & MPIDI_OFI_AM_ATTR__RDMA);
    return MPIDI_OFI_am_rdma_read_recv_cb;
}

#endif /* OFI_AM_H_INCLUDED */
