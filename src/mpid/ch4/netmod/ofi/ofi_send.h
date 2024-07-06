/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_SEND_H_INCLUDED
#define OFI_SEND_H_INCLUDED

#include "ofi_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_lightweight(const void *buf,
                                                        size_t data_sz,
                                                        uint64_t cq_data,
                                                        int dst_rank,
                                                        int tag, MPIR_Comm * comm,
                                                        int context_offset,
                                                        MPIDI_av_entry_t * addr,
                                                        int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;
    int vci_local = vci_src;
    int vci_remote = vci_dst;
    int sender_nic, receiver_nic;
    int ctx_idx;
    uint64_t match_bits;

    MPIR_FUNC_ENTER;

    /* Calculate the correct NICs. */
    sender_nic =
        MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, sender_nic);

    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
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

#define MPIDI_OFI_SEND_NEEDS_PACK (-1)  /* Could not use iov; needs packing */

/*
  Tries to send non-contiguous datatype using iovec.

  Return value:
  MPI_SUCCESS: Message was sent successfully using iovec
  MPIDI_OFI_SEND_NEEDS_PACK: There was no error but send was not initiated
      due to limitations with iovec. Needs to fall back to the pack path.
  Other: An error occurred as indicated in the code.

  Note: data_sz is passed in here for reusing.
*/
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_iov(const void *buf, MPI_Aint count, size_t data_sz,
                                                uint64_t cq_data,
                                                int dst_rank, uint64_t match_bits, MPIR_Comm * comm,
                                                MPIDI_av_entry_t * addr, int vci_src, int vci_dst,
                                                MPIR_Request * sreq, MPIR_Datatype * dt_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    struct iovec *originv = NULL;
    struct fi_msg_tagged msg;
    uint64_t flags;
    MPI_Aint num_contig, size;
    int vci_local = vci_src;
    int vci_remote = vci_dst;
    int sender_nic, receiver_nic;
    int ctx_idx;

    MPIR_FUNC_ENTER;

    /* if we cannot fit the entire data into a single IOV array,
     * fallback to pack */
    MPIR_Typerep_get_iov_len(count, MPIDI_OFI_REQUEST(sreq, datatype), &num_contig);
    if (num_contig > MPIDI_OFI_global.tx_iov_limit)
        goto pack;

    /* Calculate the correct NICs. */
    sender_nic = MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank,
                                                  MPIDI_OFI_init_get_tag(match_bits));
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank,
                                           MPIDI_OFI_init_get_tag(match_bits));
    MPIDI_OFI_REQUEST(sreq, nic_num) = sender_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, MPIDI_OFI_REQUEST(sreq, nic_num));

    /* everything fits in the IOV array */
    flags = FI_COMPLETION | (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_NOPACK;

    size = (num_contig + 1) * sizeof(struct iovec);

    MPIDI_OFI_REQUEST(sreq, u.nopack_send.iovs) = MPL_malloc(size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(sreq, u.nopack_send.iovs), 0, size);

    MPI_Aint actual_iov_len;
    MPIR_Typerep_to_iov_offset(buf, count, MPIDI_OFI_REQUEST(sreq, datatype), 0,
                               MPIDI_OFI_REQUEST(sreq, u.nopack_send.iovs), num_contig,
                               &actual_iov_len);
    assert(num_contig == actual_iov_len);

    originv = &(MPIDI_OFI_REQUEST(sreq, u.nopack_send.iovs[0]));

    msg.msg_iov = originv;
    msg.desc = NULL;
    msg.iov_count = num_contig;
    msg.tag = match_bits;
    msg.ignore = 0ULL;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(sreq, context));
    msg.data = MPIDI_OFI_ENABLE_DATA ? cq_data : 0;
    msg.addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);

    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                     &msg, flags), vci_local, tsendv);
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  pack:
    mpi_errno = MPIDI_OFI_SEND_NEEDS_PACK;
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_normal(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype,
                                                   uint64_t cq_data, int dst_rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, int vci_src,
                                                   int vci_dst, MPIR_Request ** request,
                                                   int dt_contig, size_t data_sz,
                                                   MPIR_Datatype * dt_ptr, MPI_Aint dt_true_lb,
                                                   uint64_t type, MPL_pointer_attr_t attr)
{
    int mpi_errno = MPI_SUCCESS;
    char *send_buf;
    void *pack_buffer = NULL;
    uint64_t match_bits;
    bool force_gpu_pack = false;
    int vci_local = vci_src;
    int vci_remote = vci_dst;
    int sender_nic, receiver_nic;
    int ctx_idx;
    void *desc = NULL;
    struct fid_mr *mr = NULL;
    bool register_mem = false;

    MPIR_FUNC_ENTER;

    MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__SEND, vci_src);

    MPIR_Request *sreq = *request;

    bool is_huge_send = false;
    MPI_Aint huge_thresh;
    if (MPIDI_OFI_COMM(comm).enable_striping) {
        huge_thresh = MPIDI_OFI_global.stripe_threshold;
    } else {
        huge_thresh = MPIDI_OFI_global.max_msg_size;
    }
    if (data_sz >= huge_thresh) {
        is_huge_send = true;
        /* huge send will always be synchronized */
        type = 0;
    }

    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, type);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    /* Calculate the correct NICs. */
    sender_nic =
        MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    MPIDI_OFI_REQUEST(sreq, nic_num) = sender_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, MPIDI_OFI_REQUEST(sreq, nic_num));

    if (type == MPIDI_OFI_SYNC_SEND) {  /* Branch should compile out */
        uint64_t ssend_match, ssend_mask;
        MPIDI_OFI_ssendack_request_t *ackreq;
        ackreq = MPL_malloc(sizeof(MPIDI_OFI_ssendack_request_t), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(ackreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "Ssend ack request alloc");
        ackreq->event_id = MPIDI_OFI_EVENT_SSEND_ACK;
        ackreq->signal_req = sreq;
        MPIR_cc_inc(sreq->cc_ptr);
        ssend_match =
            MPIDI_OFI_init_recvtag(&ssend_mask, comm->context_id + context_offset, dst_rank, tag);
        ssend_match |= MPIDI_OFI_SYNC_SEND_ACK;
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(vci_local, receiver_nic)].rx,        /* endpoint    */
                                      NULL,     /* recvbuf     */
                                      0,        /* data sz     */
                                      NULL,     /* memregion descr  */
                                      MPIDI_OFI_av_to_phys(addr, sender_nic, vci_remote),       /* remote proc */
                                      ssend_match,      /* match bits  */
                                      0ULL,     /* mask bits   */
                                      (void *) &(ackreq->context)), vci_local, trecvsync);
    }

    send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);

    if (MPIDI_OFI_ENABLE_HMEM && data_sz >= MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD &&
        MPIDI_OFI_ENABLE_MR_HMEM && dt_contig && attr.type == MPL_GPU_POINTER_DEV) {
        register_mem = true;
    }

    if (data_sz && attr.type == MPL_GPU_POINTER_DEV) {
        MPIDI_OFI_register_am_bufs();
        if (!MPIDI_OFI_ENABLE_HMEM || !dt_contig || (MPIDI_OFI_ENABLE_MR_HMEM && !register_mem)) {
            /* Force packing of GPU buffer in host memory */
            /* FIXME: at this point, GPU data takes host-buffer staging
             * path for the whole chunk. For large memory size, pipeline
             * transfer should be applied. */
            force_gpu_pack = true;
        }
    }

    if (register_mem) {
        MPIDI_OFI_register_memory_and_bind(send_buf, data_sz, &attr, ctx_idx, &mr);
        if (mr != NULL) {
            desc = fi_mr_desc(mr);
        }
    }

    if ((!dt_contig || force_gpu_pack) && data_sz) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && !force_gpu_pack &&
            ((data_sz < MPIDI_OFI_global.max_msg_size && !MPIDI_OFI_COMM(comm).enable_striping) ||
             (data_sz < MPIDI_OFI_global.stripe_threshold &&
              MPIDI_OFI_COMM(comm).enable_striping))) {
            mpi_errno = MPIDI_OFI_send_iov(buf, count, data_sz, cq_data, dst_rank, match_bits,
                                           comm, addr, vci_src, vci_dst, sreq, dt_ptr);
            if (mpi_errno == MPI_SUCCESS)       /* Send posted using iov */
                goto fn_exit;
            else if (mpi_errno != MPIDI_OFI_SEND_NEEDS_PACK)
                goto fn_fail;
            /* send_iov returned MPIDI_OFI_SEND_NEEDS_PACK -- indicating
             * that there was no error but it couldn't initiate the send
             * due to iov limitations. We need to fall back to the pack
             * path below. Simply falling through. */
            mpi_errno = MPI_SUCCESS;    /* Reset error code */
        }

        if (force_gpu_pack && MPIR_CVAR_CH4_OFI_ENABLE_GPU_PIPELINE &&
            data_sz >= MPIR_CVAR_CH4_OFI_GPU_PIPELINE_THRESHOLD) {
            /* Pipeline path */
            fi_addr_t remote_addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);
            MPIDI_OFI_COMM(comm).pipeline_tag += 1;
            mpi_errno = MPIDI_OFI_gpu_pipeline_send(sreq, buf, count, datatype, attr, data_sz,
                                                    cq_data, remote_addr, vci_local, ctx_idx,
                                                    match_bits, MPIDI_OFI_COMM(comm).pipeline_tag);
            MPIR_ERR_CHECK(mpi_errno);

            MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);
            goto fn_exit;
        }

        /* Pack */
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_PACK;

        pack_buffer = MPL_malloc(data_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");

        int fast_copy = 0;
        if (attr.type == MPL_GPU_POINTER_DEV && dt_contig &&
            data_sz <= MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE) {
            int mpl_err = MPL_gpu_fast_memcpy(send_buf, &attr,
                                              pack_buffer,
                                              NULL, data_sz);
            if (mpl_err == MPL_SUCCESS)
                fast_copy = 1;
        }
        if (!fast_copy) {
            MPL_gpu_engine_type_t engine =
                MPIDI_OFI_gpu_get_send_engine_type(MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE);
            if (dt_contig && engine != MPL_GPU_ENGINE_TYPE_LAST &&
                MPL_gpu_query_pointer_is_dev(send_buf, &attr)) {
                mpi_errno = MPIR_Localcopy_gpu(send_buf, data_sz, MPI_BYTE, 0, &attr,
                                               pack_buffer,
                                               data_sz, MPI_BYTE, 0, NULL,
                                               MPL_GPU_COPY_DIRECTION_NONE, engine, true);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                MPI_Aint actual_pack_bytes;
                MPIR_Typerep_pack(buf, count, datatype, 0,
                                  pack_buffer, data_sz, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            }
        }
        send_buf = pack_buffer;
        MPIDI_OFI_REQUEST(sreq, u.pack_send.pack_buffer) = pack_buffer;
    }

    fi_addr_t dest_addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote);
    if (data_sz <= MPIDI_OFI_global.max_buffered_send && !MPIDI_OFI_ENABLE_HMEM) {
        if (MPIDI_OFI_ENABLE_DATA) {
            MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                                send_buf, data_sz, cq_data, dest_addr, match_bits),
                                 vci_local, tinjectdata);
        } else {
            MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                            send_buf, data_sz, dest_addr, match_bits),
                                 vci_local, tinject);
        }
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);
        MPIDI_OFI_send_event(vci_src, NULL, sreq, MPIDI_OFI_REQUEST(sreq, event_id));
    } else if (!is_huge_send) {
        if (MPIDI_OFI_ENABLE_DATA) {
            MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                              send_buf, data_sz, desc, cq_data, dest_addr,
                                              match_bits,
                                              (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                                 vci_local, tsenddata);
        } else {
            MPIDI_OFI_CALL_RETRY(fi_tsend(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, data_sz, desc, dest_addr, match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                                 vci_local, tsend);
        }
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);
    } else if (unlikely(1)) {
        /* is_huge_send */
        int num_nics = MPIDI_OFI_global.num_nics;
        uint64_t rma_keys[MPIDI_OFI_MAX_NICS];
        struct fid_mr **huge_send_mrs;
        uint64_t msg_size = MPIDI_OFI_STRIPE_CHUNK_SIZE;

        MPIR_cc_inc(sreq->cc_ptr);
        if (!MPIDI_OFI_COMM(comm).enable_striping) {
            num_nics = 1;
            msg_size = MPIDI_OFI_global.max_msg_size;
        }
        huge_send_mrs =
            (struct fid_mr **) MPL_malloc((num_nics * sizeof(struct fid_mr *)), MPL_MEM_BUFFER);
        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            /* Set up a memory region for the lmt data transfer */
            for (int i = 0; i < num_nics; i++) {
                rma_keys[i] =
                    MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
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
            MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(vci_local, i)].domain,        /* In:  Domain Object */
                                     send_buf,  /* In:  Lower memory address */
                                     data_sz,   /* In:  Length              */
                                     FI_REMOTE_READ,    /* In:  Expose MR for read  */
                                     0ULL,      /* In:  offset(not used)    */
                                     rma_keys[i],       /* In:  requested key       */
                                     0ULL,      /* In:  flags               */
                                     &huge_send_mrs[i], /* Out: memregion object    */
                                     NULL), mr_reg);    /* In:  context             */
            mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], huge_send_mrs[i],
                                          MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index
                                                               (vci_local, i)].ep, NULL);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPIDI_OFI_REQUEST(sreq, u.huge_send.mrs) = huge_send_mrs;
        MPIDI_OFI_REQUEST(sreq, u.huge_send.pack_buffer) = pack_buffer;

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
        /* Store ordering unnecessary for dst_rank, so use relaxed store */
        MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(sreq, util_id), dst_rank);

        /* send ctrl message first */
        MPIDI_OFI_send_control_t ctrl;
        ctrl.type = MPIDI_OFI_CTRL_HUGE;
        for (int i = 0; i < num_nics; i++) {
            ctrl.u.huge.info.rma_keys[i] = rma_keys[i];
        }
        ctrl.u.huge.info.comm_id = comm->context_id;
        ctrl.u.huge.info.tag = tag;
        ctrl.u.huge.info.origin_rank = comm->rank;
        ctrl.u.huge.info.vci_src = vci_src;
        ctrl.u.huge.info.vci_dst = vci_dst;
        ctrl.u.huge.info.send_buf = send_buf;
        ctrl.u.huge.info.msgsize = data_sz;
        ctrl.u.huge.info.ackreq = sreq;

        mpi_errno = MPIDI_NM_am_send_hdr(dst_rank, comm, MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                         &ctrl, sizeof(ctrl), vci_src, vci_dst);
        MPIR_ERR_CHECK(mpi_errno);

        /* send main native message next */
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_HUGE;

        match_bits |= MPIDI_OFI_HUGE_SEND;      /* Add the bit for a huge message */
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, msg_size, NULL /* desc */ ,
                                          cq_data,
                                          MPIDI_OFI_av_to_phys(addr, receiver_nic, vci_remote),
                                          match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                             vci_local, tsenddata);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], msg_size);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, striped_nic_sent_bytes_count[sender_nic], msg_size);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_fallback(const void *buf, MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int dst_rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr, int vci_src,
                                                     int vci_dst, MPIR_Request ** request,
                                                     MPIR_Errflag_t err_flag)
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

    uint64_t match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset,
                                                 comm->rank, tag, 0);
    uint64_t cq_data = comm->rank;
    MPIDI_OFI_idata_set_error_bits(&cq_data, err_flag);

    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    /* Calculate the correct NICs. */
    MPIDI_OFI_REQUEST(sreq, nic_num) = 0;

    char *send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    struct iovec iovs[1];
    iovs[0].iov_base = send_buf;
    iovs[0].iov_len = data_sz;

    /* why do we need to set this to NULL? */
    MPIDI_OFI_REQUEST(sreq, u.pack_send.pack_buffer) = NULL;

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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                            int dst_rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIDI_av_entry_t * addr,
                                            int vci_src, int vci_dst,
                                            MPIR_Request ** request, int noreq,
                                            uint64_t syncflag, MPIR_Errflag_t err_flag)
{
    int dt_contig, mpi_errno;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    MPL_pointer_attr_t attr;

    MPIR_FUNC_ENTER;

    uint64_t cq_data = comm->rank;
    MPIDI_OFI_idata_set_error_bits(&cq_data, err_flag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    void *send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    MPIR_GPU_query_pointer_attr(send_buf, &attr);

    if (likely(!syncflag && dt_contig && (data_sz <= MPIDI_OFI_global.max_buffered_send))) {
        MPI_Aint actual_pack_bytes = 0;
        if (attr.type == MPL_GPU_POINTER_DEV && data_sz) {
            MPIDI_OFI_register_am_bufs();
            if (!MPIDI_OFI_ENABLE_HMEM) {
                /* Force pack for GPU buffer. */
                void *host_buf = MPL_malloc(data_sz, MPL_MEM_OTHER);
                int fast_copy = 0;
                if (attr.type == MPL_GPU_POINTER_DEV && dt_contig &&
                    data_sz <= MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE) {
                    int mpl_err;
                    mpl_err = MPL_gpu_fast_memcpy(send_buf, &attr, host_buf, NULL, data_sz);
                    if (mpl_err == MPL_SUCCESS) {
                        fast_copy = 1;
                        actual_pack_bytes = data_sz;
                    }
                }
                if (!fast_copy) {
                    MPL_gpu_engine_type_t engine =
                        MPIDI_OFI_gpu_get_send_engine_type(MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE);
                    if (dt_contig && engine != MPL_GPU_ENGINE_TYPE_LAST &&
                        MPL_gpu_query_pointer_is_dev(send_buf, &attr)) {
                        mpi_errno =
                            MPIR_Localcopy_gpu(send_buf, data_sz, MPI_BYTE, 0, &attr, host_buf,
                                               data_sz, MPI_BYTE, 0, NULL,
                                               MPL_GPU_COPY_DIRECTION_NONE, engine, true);
                        MPIR_ERR_CHECK(mpi_errno);
                        actual_pack_bytes = data_sz;
                    } else {
                        MPIR_Typerep_pack(buf, count, datatype, 0, host_buf, data_sz,
                                          &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
                    }
                }
                MPIR_Assert(actual_pack_bytes == data_sz);
                send_buf = host_buf;
            } else {
                mpi_errno =
                    MPIDI_OFI_send_normal(buf, count, datatype, cq_data, dst_rank, tag, comm,
                                          context_offset, addr, vci_src, vci_dst, request,
                                          dt_contig, data_sz, dt_ptr, dt_true_lb, syncflag, attr);
                goto fn_exit;
            }
        }
        mpi_errno = MPIDI_OFI_send_lightweight(send_buf, data_sz, cq_data, dst_rank, tag, comm,
                                               context_offset, addr, vci_src, vci_dst);
        if (actual_pack_bytes > 0) {
            /* Free stage host buf (assigned to send_buf already) after
             * lightweight_send. */
            MPL_free(send_buf);
        }
        if (!noreq) {
            *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        }
    } else {
        mpi_errno = MPIDI_OFI_send_normal(buf, count, datatype, cq_data, dst_rank, tag, comm,
                                          context_offset, addr, vci_src, vci_dst, request,
                                          dt_contig, data_sz, dt_ptr, dt_true_lb, syncflag, attr);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIR_Errflag_t errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);

    int vci_src, vci_dst;
    MPIDI_OFI_SEND_VNIS(vci_src, vci_dst);      /* defined just above */

    MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_src);
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        bool syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr) ? MPIDIG_AM_SEND_FLAGS_SYNC : 0;
        mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                     vci_src, vci_dst, request, syncflag, errflag);
    } else if (!MPIDI_global.is_initialized) {
        MPIR_Assert(!MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr));
        mpi_errno = MPIDI_OFI_send_fallback(buf, count, datatype, rank, tag, comm,
                                            context_offset, addr, vci_src, vci_dst,
                                            request, errflag);
    } else {
        uint64_t syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr) ? MPIDI_OFI_SYNC_SEND : 0;
        mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, vci_src, vci_dst,
                                   request, 0, syncflag, errflag);
    }
    MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_src);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    /* Sends cannot be cancelled */

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* OFI_SEND_H_INCLUDED */
