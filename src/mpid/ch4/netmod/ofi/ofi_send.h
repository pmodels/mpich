/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_SEND_H_INCLUDED
#define OFI_SEND_H_INCLUDED

#include "ofi_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_OFI_ENABLE_INJECT
      category    : DEVELOPER
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Set MPIR_CVAR_CH4_OFI_ENABLE_INJECT=0 to disable buffered send for small messages. This
        may help avoid hang due to lack of global progress.

    - name        : MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE
      category    : CH4_OFI
      type        : enum
      default     : copy_low_latency
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : |-
        Specifies GPU engine type for GPU pt2pt on the sender side.
        compute - use a compute engine
        copy_high_bandwidth - use a high-bandwidth copy engine
        copy_low_latency - use a low-latency copy engine
        yaksa - use Yaksa

    - name        : MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD
      category    : CH4_OFI
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Messages below MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD will be sent eagerly using
        fi_tagged interfaces. Messages above the threshold will perform an MPICH-level
        rendezvous handshake before sending the data. If set to -1, MPICH will only
        perform rendezvous for messages larger than the provider max_msg_size.
        Note the MPICH eager/rendezvous threshold is independent of any internal
        libfabric provider threshold.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX MPL_gpu_engine_type_t MPIDI_OFI_gpu_get_send_engine_type(void)
{
    if (MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE == MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE_compute) {
        return MPL_GPU_ENGINE_TYPE_COMPUTE;
    } else if (MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE ==
               MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE_copy_high_bandwidth) {
        return MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;
    } else if (MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE ==
               MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE_copy_low_latency) {
        return MPL_GPU_ENGINE_TYPE_COPY_LOW_LATENCY;
    } else {
        return MPL_GPU_ENGINE_TYPE_LAST;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_issue_ack_recv(MPIR_Request * sreq, MPIR_Comm * comm,
                                                      int context_offset, int dst_rank,
                                                      int tag, MPIDI_av_entry_t * addr,
                                                      int vci_local, int vci_remote,
                                                      int event_id, int hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_ack_request_t *ackreq;
    ackreq = MPL_malloc(sizeof(MPIDI_OFI_ack_request_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP1(ackreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "OFI ack request alloc");
    ackreq->event_id = event_id;
    ackreq->signal_req = sreq;
    if (hdr_sz > 0) {
        ackreq->ack_hdr = MPL_malloc(hdr_sz, MPL_MEM_OTHER);
        MPIR_Assert(ackreq->ack_hdr);
    } else {
        ackreq->ack_hdr = NULL;
    }

    uint64_t match_bits, mask_bits;
    /* NOTE: for reply, we don't need rank in the match_bits */
    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, comm->context_id + context_offset, 0, tag);
    match_bits |= MPIDI_OFI_ACK_SEND;

    int nic = 0;                /* sync message always use nic 0 */
    /* save enough field so that we can re-issue ack_recv if needed */
    ackreq->ack_hdr_sz = hdr_sz;
    ackreq->ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic);
    ackreq->vci_local = vci_local;
    ackreq->remote_addr = MPIDI_OFI_av_to_phys(addr, nic, vci_remote);
    ackreq->match_bits = match_bits;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    /* set is_local in case we go into active messages later */
    MPIDI_REQUEST(sreq, is_local) = 0;
#endif
    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ackreq->ctx_idx].rx,
                                  ackreq->ack_hdr, ackreq->ack_hdr_sz, NULL, ackreq->remote_addr,
                                  ackreq->match_bits, 0ULL, (void *) &(ackreq->context)),
                         ackreq->vci_local, trecvsync);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_lightweight(const void *buf, size_t data_sz,
                                                        uint64_t cq_data, int dst_rank, int tag,
                                                        MPIR_Comm * comm, uint64_t match_bits,
                                                        MPIDI_av_entry_t * addr,
                                                        int vci_local, int vci_remote,
                                                        int sender_nic, int receiver_nic)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, sender_nic);

    fi_addr_t dest_addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);
    if (MPIDI_OFI_ENABLE_DATA) {
        MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                            buf, data_sz, cq_data, dest_addr, match_bits),
                             vci_local, tinjectdata);
    } else {
        MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                        buf, data_sz, dest_addr, match_bits), vci_local, tinject);
    }
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_iov(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, size_t data_sz,
                                                uint64_t cq_data, int dst_rank, int tag,
                                                MPIR_Comm * comm, uint64_t match_bits,
                                                MPIDI_av_entry_t * addr,
                                                int vci_local, int vci_remote,
                                                int sender_nic, int receiver_nic,
                                                MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* if we cannot fit the entire data into a single IOV array,
     * fallback to pack */
    MPI_Aint num_contig;
    MPIR_Typerep_get_iov_len(count, datatype, &num_contig);
    MPIR_Assert(num_contig <= MPIDI_OFI_global.tx_iov_limit);

    /* everything fits in the IOV array */
    uint64_t flags;
    flags = FI_COMPLETION | (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_NOPACK;

    struct iovec *iovs;
    iovs = MPL_malloc(num_contig * sizeof(struct iovec), MPL_MEM_BUFFER);
    MPIDI_OFI_REQUEST(sreq, noncontig.nopack.iovs) = iovs;

    MPI_Aint actual_iov_len;
    MPIR_Typerep_to_iov_offset(buf, count, datatype, 0, iovs, num_contig, &actual_iov_len);
    assert(actual_iov_len == num_contig);

    struct fi_msg_tagged msg;
    msg.msg_iov = iovs;
    msg.desc = NULL;
    msg.iov_count = num_contig;
    msg.tag = match_bits;
    msg.ignore = 0ULL;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(sreq, context));
    msg.data = MPIDI_OFI_ENABLE_DATA ? cq_data : 0;
    msg.addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, sender_nic);

    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[ctx_idx].tx, &msg, flags),
                         vci_local, tsendv);
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_normal(const void *data, MPI_Aint data_sz,
                                                   uint64_t cq_data, int dst_rank, int tag,
                                                   MPIR_Comm * comm, uint64_t match_bits,
                                                   MPIDI_av_entry_t * addr,
                                                   int vci_local, int vci_remote,
                                                   int sender_nic, int receiver_nic,
                                                   MPIR_Request * sreq,
                                                   MPL_pointer_attr_t attr, bool need_mr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, sender_nic);

    if (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer)) {
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_PACK;
    } else {
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    }

    void *desc = NULL;
    struct fid_mr *mr = NULL;
    if (need_mr) {
        MPIDI_OFI_register_memory_and_bind((void *) data, data_sz, &attr, ctx_idx, &mr);
        if (mr != NULL) {
            desc = fi_mr_desc(mr);
        }
    }

    fi_addr_t dest_addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);
    if (MPIDI_OFI_ENABLE_DATA) {
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          data, data_sz, desc, cq_data, dest_addr,
                                          match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                             vci_local, tsenddata);
    } else {
        MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                      data, data_sz, desc, dest_addr, match_bits,
                                      (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                             vci_local, tsend);
    }
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_huge(const void *data, MPI_Aint data_sz,
                                                 uint64_t cq_data, int dst_rank, int tag,
                                                 MPIR_Comm * comm, uint64_t match_bits,
                                                 MPIDI_av_entry_t * addr,
                                                 int vci_local, int vci_remote,
                                                 int sender_nic, int receiver_nic,
                                                 MPIR_Request * sreq,
                                                 MPL_pointer_attr_t attr, bool do_striping)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, sender_nic);

    int num_nics;
    uint64_t msg_size;
    if (do_striping) {
        num_nics = MPIDI_OFI_global.num_nics;
        msg_size = MPIDI_OFI_STRIPE_CHUNK_SIZE;
    } else {
        num_nics = 1;
        msg_size = MPIDI_OFI_global.max_msg_size;
    }

    uint64_t rma_keys[MPIDI_OFI_MAX_NICS];
    struct fid_mr **huge_send_mrs;
    huge_send_mrs =
        (struct fid_mr **) MPL_malloc((num_nics * sizeof(struct fid_mr *)), MPL_MEM_BUFFER);
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* Set up a memory region for the lmt data transfer */
        for (int i = 0; i < num_nics; i++) {
            rma_keys[i] = MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
            MPIR_ERR_CHKANDJUMP(rma_keys[i] == MPIDI_OFI_INVALID_MR_KEY, mpi_errno,
                                MPI_ERR_OTHER, "**ofid_mr_key");
        }
    } else {
        /* zero them to avoid warnings */
        for (int i = 0; i < num_nics; i++) {
            rma_keys[i] = 0;
        }
    }

    for (int i = 0; i < num_nics; i++) {
        MPIDI_OFI_context_t *ctx = &MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(vci_local, i)];
        MPIDI_OFI_CALL(fi_mr_reg(ctx->domain, data, data_sz, FI_REMOTE_READ, 0ULL, rma_keys[i],
                                 0ULL, &huge_send_mrs[i], NULL), mr_reg);
        mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], huge_send_mrs[i], ctx->ep,
                                      NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPIDI_OFI_REQUEST(sreq, huge.send_mrs) = huge_send_mrs;
    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* MR_BASIC */
        for (int i = 0; i < num_nics; i++) {
            rma_keys[i] = fi_mr_key(huge_send_mrs[i]);
        }
    }

    /* Send the maximum amount of data that we can here to get things
     * started, then do the rest using the MR below. This can be confirmed
     * in the MPIDI_OFI_get_huge code where we start the offset at
     * MPIDI_OFI_global.max_msg_size */
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    /* send ctrl message first */
    MPIDI_OFI_send_control_t ctrl;
    ctrl.type = MPIDI_OFI_CTRL_HUGE;
    for (int i = 0; i < num_nics; i++) {
        ctrl.u.huge.info.rma_keys[i] = rma_keys[i];
    }
    ctrl.u.huge.info.comm_id = comm->context_id;
    ctrl.u.huge.info.tag = tag;
    ctrl.u.huge.info.origin_rank = comm->rank;
    ctrl.u.huge.info.vci_src = vci_local;
    ctrl.u.huge.info.vci_dst = vci_remote;
    ctrl.u.huge.info.send_buf = (void *) data;
    ctrl.u.huge.info.msgsize = data_sz;
    ctrl.u.huge.info.ackreq = sreq;

    mpi_errno = MPIDI_NM_am_send_hdr(dst_rank, comm, MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                     &ctrl, sizeof(ctrl), vci_local, vci_remote);
    MPIR_ERR_CHECK(mpi_errno);

    /* send main native message next */
    MPIR_cc_inc(sreq->cc_ptr);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_HUGE;

    match_bits |= MPIDI_OFI_HUGE_SEND;  /* Add the bit for a huge message */
    MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                      data, msg_size, NULL /* desc */ ,
                                      cq_data,
                                      MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote),
                                      match_bits,
                                      (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                         vci_local, tsenddata);
    /* FIXME: sender_nic may not be the actual nic */
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], msg_size);
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, striped_nic_sent_bytes_count[sender_nic], msg_size);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_pipeline(const void *buf, MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     uint64_t cq_data, int dst_rank, int tag,
                                                     MPIR_Comm * comm, uint64_t match_bits,
                                                     MPIDI_av_entry_t * addr,
                                                     int vci_local, int vci_remote,
                                                     int sender_nic, int receiver_nic,
                                                     MPIR_Request * sreq,
                                                     int dt_contig, size_t data_sz,
                                                     MPL_pointer_attr_t attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, sender_nic);

    uint32_t n_chunks = 0;
    int chunk_size = MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ;
    /* Update correct number of chunks in immediate data. */
    chunk_size = MPIDI_OFI_gpu_pipeline_chunk_size(data_sz);
    n_chunks = data_sz / chunk_size;
    if (data_sz % chunk_size)
        n_chunks++;
    MPIDI_OFI_idata_set_gpuchunk_bits(&cq_data, n_chunks);

    /* Update sender packed bit if necessary. */
    uint64_t is_packed = datatype == MPI_PACKED ? 1 : 0;
    MPIDI_OFI_idata_set_gpu_packed_bit(&cq_data, is_packed);
    MPIR_ERR_CHKANDJUMP(is_packed, mpi_errno, MPI_ERR_OTHER, "**gpu_pipeline_packed");

    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;

    /* Save pipeline information. */
    MPIDI_OFI_REQUEST(sreq, pipeline_info.chunk_sz) = chunk_size;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.cq_data) = cq_data;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.remote_addr) =
        MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);
    MPIDI_OFI_REQUEST(sreq, pipeline_info.vci_local) = vci_local;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.ctx_idx) = ctx_idx;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.match_bits) = match_bits;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.data_sz) = data_sz;

    /* send an empty message for tag matching */
    MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                        NULL,
                                        0,
                                        cq_data,
                                        MPIDI_OFI_REQUEST(sreq, pipeline_info.remote_addr),
                                        match_bits), vci_local, tinjectdata);
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);

    MPIDI_OFI_gpu_pending_send_t *send_task =
        MPIDI_OFI_create_send_task(sreq, (void *) buf, count, datatype, attr, data_sz, dt_contig);
    DL_APPEND(MPIDI_OFI_global.gpu_send_queue, send_task);
    MPIDI_OFI_gpu_progress_send();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_fallback(const void *buf, MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int dst_rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr, int vci_src,
                                                     int vci_dst, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__SEND, vci_src);

    MPIR_Request *sreq = *request;

    MPIR_Assert(vci_src == 0);
    MPIR_Assert(vci_dst == 0);
    /* we also assume buf to be cpu buffer */

    int dt_contig;
    MPIR_Datatype *dt_ptr;
    MPI_Aint data_sz, dt_true_lb;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIR_Assert(data_sz < MPIDI_OFI_global.max_msg_size);
    MPIR_Assertp(dt_contig);

    uint64_t cq_data = comm->rank;

    uint64_t match_bits
        = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag);

    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;

    /* Calculate the correct NICs. */
    MPIDI_OFI_REQUEST(sreq, nic_num) = 0;

    char *send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    struct iovec iovs[1];
    iovs[0].iov_base = send_buf;
    iovs[0].iov_len = data_sz;

    MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) = NULL;

    struct fi_msg_tagged msg;
    msg.msg_iov = iovs;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.tag = match_bits;
    msg.ignore = 0ULL;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(sreq, context));
    msg.data = 0;
    msg.addr = MPIDI_OFI_av_to_phys(addr, 0, 0);

    int flags = FI_COMPLETION | FI_TRANSMIT_COMPLETE;
    if (MPIDI_OFI_ENABLE_DATA) {
        msg.data = cq_data;
        flags |= FI_REMOTE_CQ_DATA;
    }

    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[0].tx, &msg, flags), 0, tsendv);
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[0], data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define EAGER_THRESH (MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD == -1 ? MPIDI_OFI_global.max_msg_size : MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD)

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                            int dst_rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIDI_av_entry_t * addr,
                                            int vci_src, int vci_dst,
                                            MPIR_Request ** request, bool is_am, bool syncflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    uint64_t cq_data = comm->rank;

    int dt_contig;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    bool is_gpu = false;
    bool need_pack = false;
    bool need_mr = false;
    bool do_inject = false;
    bool do_iov = false;
    bool do_gpu_pipelining = false;
    bool do_striping = false;
    bool do_huge = false;

    /* check gpu */
    MPL_pointer_attr_t attr;
    void *send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    MPIR_GPU_query_pointer_attr(send_buf, &attr);
    if (data_sz && MPL_gpu_attr_is_dev(&attr)) {
        is_gpu = true;
        MPIDI_OFI_register_am_bufs();
    }

    /* check gpu paths */
    if (is_gpu) {
        if (!MPIDI_OFI_ENABLE_HMEM) {
            /* HMEM (any kind) not supported */
            need_pack = true;
        } else if (!MPL_gpu_attr_is_strict_dev(&attr)) {
            /* non-strict gpu ptr (ZE shared host) */
            need_pack = true;
        } else {
            if (MPIDI_OFI_ENABLE_MR_HMEM) {
                need_mr = true; /* we'll update this later after need_pack settles */
                if (!dt_contig) {
                    /* can't register non-contig iovs */
                    need_pack = true;
                } else if (data_sz < MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD) {
                    /* small message does worth the MR overhead */
                    need_pack = true;
                }
            }
        }

        if (need_pack && MPIR_CVAR_CH4_OFI_ENABLE_GPU_PIPELINE &&
            data_sz >= MPIR_CVAR_CH4_OFI_GPU_PIPELINE_THRESHOLD) {
            do_gpu_pipelining = true;
            need_pack = false;
        }
    }

    if (MPIR_CVAR_CH4_OFI_ENABLE_INJECT && !syncflag &&
        (data_sz <= MPIDI_OFI_global.max_buffered_send)) {
        do_inject = true;
    }

    /* check striping path */
    if (MPIDI_OFI_COMM(comm).enable_striping && data_sz >= MPIDI_OFI_global.stripe_threshold) {
        syncflag = false;
        do_striping = true;
    }

    /* check striping path */
    if (!do_striping && data_sz >= MPIDI_OFI_global.max_msg_size) {
        syncflag = false;
        do_huge = true;
    }

    /* noncontig? try iov or need pack */
    if (!need_pack && !dt_contig) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && !do_inject && !do_striping && !do_huge &&
            !do_gpu_pipelining) {
            MPI_Aint num_contig;
            MPIR_Typerep_get_iov_len(count, datatype, &num_contig);
            if (num_contig <= MPIDI_OFI_global.tx_iov_limit) {
                do_iov = true;
            }
        }

        if (!do_iov) {
            need_pack = true;
        }
    }

    if (need_pack && need_mr) {
        need_mr = false;
    }

    /* do send */
    int sender_nic, receiver_nic;
    sender_nic =
        MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);

    uint64_t match_bits;
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag);

    if (MPIR_CVAR_CH4_OFI_ENABLE_INJECT && !syncflag && !is_am &&
        (data_sz <= MPIDI_OFI_global.max_buffered_send)) {
        /* inject path */
        void *pack_buf = NULL;
        if (need_pack) {
            pack_buf = MPL_aligned_alloc(64, data_sz, MPL_MEM_OTHER);
            mpi_errno = MPIR_Localcopy_gpu(buf, count, datatype, 0, &attr,
                                           pack_buf, data_sz, MPI_BYTE, 0, MPIR_GPU_ATTR_HOST,
                                           MPL_GPU_COPY_D2H,
                                           MPIDI_OFI_gpu_get_send_engine_type(), true);
            MPIR_ERR_CHECK(mpi_errno);
            send_buf = pack_buf;
        }
        mpi_errno = MPIDI_OFI_send_lightweight(send_buf, data_sz, cq_data, dst_rank, tag, comm,
                                               match_bits, addr,
                                               vci_src, vci_dst, sender_nic, receiver_nic);
        if (pack_buf) {
            MPL_free(pack_buf);
        }

        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (!is_am && data_sz > EAGER_THRESH) {
        MPIR_Request *sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2,
                                                   vci_src /* local */ , vci_dst /* remote */);
        MPIR_ERR_CHKANDJUMP(!sreq, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        *request = sreq;
        sreq->comm = comm;
        MPIR_Comm_add_ref(comm);
        MPIDIG_REQUEST(sreq, req->local_vci) = vci_src;
        MPIDIG_REQUEST(sreq, req->remote_vci) = vci_dst;
        MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
        MPIDIG_REQUEST(sreq, count) = count;
        MPIDIG_REQUEST(sreq, datatype) = datatype;
        MPIDIG_REQUEST(sreq, u.send.dest) = dst_rank;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);

        mpi_errno = MPIDI_OFI_issue_ack_recv(sreq, comm, context_offset, dst_rank, tag, addr,
                                             vci_src, vci_dst, MPIDI_OFI_EVENT_RNDV_CTS,
                                             sizeof(MPIDI_OFI_ack_request_t));
        MPIR_ERR_CHECK(mpi_errno);
        /* inject a zero-size message with MPIDI_OFI_RNDV_SEND in match_bits */
        match_bits |= MPIDI_OFI_RNDV_SEND;
        MPIDI_OFI_idata_set_size(&cq_data, data_sz);    /* optionally use cq_data to carry data_sz */
        mpi_errno = MPIDI_OFI_send_lightweight(NULL, 0, cq_data, dst_rank, tag, comm,
                                               match_bits, addr,
                                               vci_src, vci_dst, sender_nic, receiver_nic);

    } else {
        /* normal path */
        MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__SEND, vci_src);
        MPIR_Request *sreq = *request;

        if (syncflag) {
            match_bits |= MPIDI_OFI_SYNC_SEND;
            MPIR_cc_inc(sreq->cc_ptr);
            mpi_errno = MPIDI_OFI_issue_ack_recv(sreq, comm, context_offset, dst_rank, tag, addr,
                                                 vci_src, vci_dst, MPIDI_OFI_EVENT_SSEND_ACK, 0);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (is_am) {
            MPIR_Assert(!syncflag);
            match_bits |= MPIDI_OFI_AM_SEND;
        }

        void *data = NULL;
        if (need_pack) {
            void *pack_buf = MPL_aligned_alloc(64, data_sz, MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP1(pack_buf == NULL, mpi_errno,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");

            mpi_errno = MPIR_Localcopy_gpu(buf, count, datatype, 0, &attr,
                                           pack_buf, data_sz, MPI_BYTE, 0, MPIR_GPU_ATTR_HOST,
                                           MPL_GPU_COPY_D2H,
                                           MPIDI_OFI_gpu_get_send_engine_type(), true);
            MPIR_ERR_CHECK(mpi_errno);

            data = pack_buf;
            MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) = pack_buf;
        } else {
            data = MPIR_get_contig_ptr(buf, dt_true_lb);
            MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) = NULL;
        }

        if (do_gpu_pipelining) {
            mpi_errno = MPIDI_OFI_send_pipeline(buf, count, datatype, cq_data, dst_rank, tag, comm,
                                                match_bits, addr, vci_src, vci_dst,
                                                sender_nic, receiver_nic,
                                                *request, dt_contig, data_sz, attr);
            MPIR_ERR_CHECK(mpi_errno);
            goto fn_exit;
        }

        if (do_huge) {
            mpi_errno = MPIDI_OFI_send_huge(data, data_sz, cq_data, dst_rank, tag, comm,
                                            match_bits, addr, vci_src, vci_dst,
                                            sender_nic, receiver_nic, *request, attr, false);
            MPIR_ERR_CHECK(mpi_errno);
            goto fn_exit;
        }

        if (do_striping) {
            mpi_errno = MPIDI_OFI_send_huge(data, data_sz, cq_data, dst_rank, tag, comm,
                                            match_bits, addr, vci_src, vci_dst,
                                            sender_nic, receiver_nic, *request, attr, true);
            MPIR_ERR_CHECK(mpi_errno);
            goto fn_exit;
        }

        /* NOTE: all previous send modes contains sync semantics already */

        if (do_iov) {
            mpi_errno = MPIDI_OFI_send_iov(buf, count, datatype, data_sz, cq_data, dst_rank, tag,
                                           comm, match_bits, addr, vci_src, vci_dst,
                                           sender_nic, receiver_nic, *request);
            MPIR_ERR_CHECK(mpi_errno);
            goto fn_exit;
        }

        mpi_errno = MPIDI_OFI_send_normal(data, data_sz, cq_data, dst_rank, tag, comm,
                                          match_bits, addr, vci_src, vci_dst,
                                          sender_nic, receiver_nic, *request, attr, need_mr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef EAGER_THRESH

/* Common macro used by all MPIDI_NM_mpi_send routines to facilitate tuning */
#define MPIDI_OFI_SEND_VNIS(vci_src_, vci_dst_) \
    do { \
        if (*request != NULL) { \
            /* workq path */ \
            vci_src_ = 0; \
            vci_dst_ = 0; \
        } else { \
            MPIDI_EXPLICIT_VCIS(comm, attr, comm->rank, rank, vci_src_, vci_dst_); \
            if (vci_src_ == 0 && vci_dst_ == 0) { \
                vci_src_ = MPIDI_get_vci(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
                vci_dst_ = MPIDI_get_vci(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
            } \
        } \
    } while (0)

#ifndef MPIDI_ENABLE_AM_ONLY
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    MPIR_Assert(errflag == 0);

    int vci_src, vci_dst;
    MPIDI_OFI_SEND_VNIS(vci_src, vci_dst);      /* defined just above */

    MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_src);
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        bool syncflag = (bool) MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr);
        mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                     vci_src, vci_dst, request, syncflag, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (!MPIDI_global.is_initialized) {
        MPIR_Assert(!MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr));
        mpi_errno = MPIDI_OFI_send_fallback(buf, count, datatype, rank, tag, comm,
                                            context_offset, addr, vci_src, vci_dst, request);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_OFI_REQUEST(*request, am_req) = NULL;
    } else {
        bool syncflag = (bool) MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr);
        mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, vci_src, vci_dst, request,
                                   false /* is_am */ , syncflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_OFI_REQUEST(*request, am_req) = NULL;
    }

  fn_exit:
    MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_src);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    /* Sends cannot be cancelled */

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
#endif /* ifndef MPIDI_ENABLE_AM_ONLY */

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_tag_send(int rank, MPIR_Comm * comm,
                                                  int handler_id, int tag,
                                                  const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int vci_src, int vci_dst, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* am_tag_send is similar to normal send with -
     * * already inside VCI critical section
     * * match_bits need have MPIDI_OFI_AM_SEND protocol bit set
     */
    MPIR_Assert(MPIDI_OFI_ENABLE_TAGGED);

    /* NOTE: it is possible to reuse sreq, but then we need check and handle am path inside
     *       the native paths. To reduce complexity, let's simply ask for a new request.
     *       The overhead should be negligible since am_tag_send should be used for large-ish messages.
     */
    MPIR_Request *ofi_req = NULL;
    MPIDI_av_entry_t *addr = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                               0 /* context_offset */ , addr, vci_src, vci_dst, &ofi_req,
                               true /* is_am */ , false /* syncflag */);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_REQUEST(ofi_req, am_req) = sreq;
    MPIDI_OFI_REQUEST(ofi_req, am_handler_id) = handler_id;

    /* This is an internal am step that user do not need to track */
    MPIR_Request_free_unsafe(ofi_req);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_SEND_H_INCLUDED */
