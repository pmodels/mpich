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
                                                        int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    int vni_local = vni_src;
    int vni_remote = vni_dst;
    int sender_nic = 0, receiver_nic = 0;
    int ctx_idx = 0;
    uint64_t match_bits;

    MPIR_FUNC_ENTER;

    /* Calculate the correct NICs. */
    sender_nic =
        MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_local, sender_nic);

    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, tag, 0);
    MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                        buf,
                                        data_sz,
                                        cq_data,
                                        MPIDI_OFI_av_to_phys(addr, receiver_nic, vni_local,
                                                             vni_remote),
                                        match_bits),
                         vni_local, tinjectdata, comm->hints[MPIR_COMM_HINT_EAGAIN]);
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
                                                MPIDI_av_entry_t * addr, int vni_src, int vni_dst,
                                                MPIR_Request * sreq, MPIR_Datatype * dt_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    struct iovec *originv = NULL;
    struct fi_msg_tagged msg;
    uint64_t flags;
    MPI_Aint num_contig, size;
    int vni_local = vni_src;
    int vni_remote = vni_dst;
    int sender_nic = 0, receiver_nic = 0;
    int ctx_idx = 0;

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
    ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_local, MPIDI_OFI_REQUEST(sreq, nic_num));

    /* everything fits in the IOV array */
    flags = FI_COMPLETION | FI_REMOTE_CQ_DATA;
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_NOPACK;

    size = num_contig * sizeof(struct iovec) + sizeof(*(MPIDI_OFI_REQUEST(sreq, noncontig.nopack)));

    MPIDI_OFI_REQUEST(sreq, noncontig.nopack) = MPL_malloc(size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(sreq, noncontig.nopack), 0, size);

    MPI_Aint actual_iov_len;
    MPIR_Typerep_to_iov_offset(buf, count, MPIDI_OFI_REQUEST(sreq, datatype), 0,
                               MPIDI_OFI_REQUEST(sreq, noncontig.nopack), num_contig,
                               &actual_iov_len);
    assert(num_contig == actual_iov_len);

    originv = &(MPIDI_OFI_REQUEST(sreq, noncontig.nopack[0]));

    msg.msg_iov = originv;
    msg.desc = NULL;
    msg.iov_count = num_contig;
    msg.tag = match_bits;
    msg.ignore = 0ULL;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(sreq, context));
    msg.data = cq_data;
    msg.addr = MPIDI_OFI_av_to_phys(addr, receiver_nic, vni_local, vni_remote);

    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                     &msg, flags), vni_local, tsendv, FALSE);
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
                                                   MPIDI_av_entry_t * addr, int vni_src,
                                                   int vni_dst, MPIR_Request ** request,
                                                   int dt_contig, size_t data_sz,
                                                   MPIR_Datatype * dt_ptr, MPI_Aint dt_true_lb,
                                                   uint64_t type)
{
    int mpi_errno = MPI_SUCCESS;
    char *send_buf;
    uint64_t match_bits;
    bool force_gpu_pack = false;
    int vni_local = vni_src;
    int vni_remote = vni_dst;
    int sender_nic = 0, receiver_nic = 0;
    int ctx_idx = 0;

    MPIR_FUNC_ENTER;

    MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__SEND, vni_src);

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

    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, tag, type);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    /* Calculate the correct NICs. */
    sender_nic =
        MPIDI_OFI_multx_sender_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->context_id, comm->rank, dst_rank, tag);
    MPIDI_OFI_REQUEST(sreq, nic_num) = sender_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_local, MPIDI_OFI_REQUEST(sreq, nic_num));

    if (type == MPIDI_OFI_SYNC_SEND) {  /* Branch should compile out */
        uint64_t ssend_match, ssend_mask;
        MPIDI_OFI_ssendack_request_t *ackreq;
        ackreq = MPL_malloc(sizeof(MPIDI_OFI_ssendack_request_t), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(ackreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "Ssend ack request alloc");
        ackreq->event_id = MPIDI_OFI_EVENT_SSEND_ACK;
        ackreq->signal_req = sreq;
        MPIR_cc_inc(sreq->cc_ptr);
        ssend_match = MPIDI_OFI_init_recvtag(&ssend_mask, comm->context_id + context_offset, tag);
        ssend_match |= MPIDI_OFI_SYNC_SEND_ACK;
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(comm, vni_local, receiver_nic)].rx,  /* endpoint    */
                                      NULL,     /* recvbuf     */
                                      0,        /* data sz     */
                                      NULL,     /* memregion descr  */
                                      MPIDI_OFI_av_to_phys(addr, sender_nic, vni_local, vni_remote),    /* remote proc */
                                      ssend_match,      /* match bits  */
                                      0ULL,     /* mask bits   */
                                      (void *) &(ackreq->context)), vni_local, trecvsync, FALSE);
    }

    send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(send_buf, &attr);
    if (data_sz && attr.type == MPL_GPU_POINTER_DEV) {
        if (!MPIDI_OFI_ENABLE_HMEM) {
            /* Force packing of GPU buffer in host memory */
            /* FIXME: at this point, GPU data takes host-buffer staging
             * path for the whole chunk. For large memory size, pipeline
             * transfer should be applied. */
            dt_contig = 0;
            force_gpu_pack = true;
        }
    }

    if (!dt_contig && data_sz) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && !force_gpu_pack &&
            ((data_sz < MPIDI_OFI_global.max_msg_size && !MPIDI_OFI_COMM(comm).enable_striping) ||
             (data_sz < MPIDI_OFI_global.stripe_threshold &&
              MPIDI_OFI_COMM(comm).enable_striping))) {
            mpi_errno = MPIDI_OFI_send_iov(buf, count, data_sz, cq_data, dst_rank, match_bits,
                                           comm, addr, vni_src, vni_dst, sreq, dt_ptr);
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
        /* Pack */
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_PACK;

        MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) = MPL_malloc(data_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");

        MPI_Aint actual_pack_bytes;
        MPIR_Typerep_pack(buf, count, datatype, 0,
                          MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer), data_sz,
                          &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        send_buf = MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer);
    } else {
        MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) = NULL;
        MPIDI_OFI_REQUEST(sreq, noncontig.nopack) = NULL;
    }

    if (data_sz <= MPIDI_OFI_global.max_buffered_send) {
        MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                            send_buf,
                                            data_sz,
                                            cq_data,
                                            MPIDI_OFI_av_to_phys(addr, receiver_nic, vni_local,
                                                                 vni_remote), match_bits),
                             vni_local, tinjectdata, FALSE /* eagain */);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);
        MPIDI_OFI_send_event(vni_src, NULL, sreq, MPIDI_OFI_REQUEST(sreq, event_id));
    } else if (!is_huge_send) {
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, data_sz, NULL /* desc */ ,
                                          cq_data,
                                          MPIDI_OFI_av_to_phys(addr, receiver_nic, vni_local,
                                                               vni_remote), match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))), vni_local,
                             tsenddata, FALSE /* eagain */);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], data_sz);
    } else if (unlikely(1)) {
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
            MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(comm, vni_local, i)].domain,  /* In:  Domain Object */
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
                                                               (comm, vni_local, i)].ep, NULL);
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
        ctrl.u.huge.info.vni_src = vni_src;
        ctrl.u.huge.info.vni_dst = vni_dst;
        ctrl.u.huge.info.send_buf = send_buf;
        ctrl.u.huge.info.msgsize = data_sz;
        ctrl.u.huge.info.ackreq = sreq;

        mpi_errno = MPIDI_NM_am_send_hdr(dst_rank, comm, MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                         &ctrl, sizeof(ctrl), vni_src, vni_dst);
        MPIR_ERR_CHECK(mpi_errno);

        /* send main native message next */
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_HUGE;

        match_bits |= MPIDI_OFI_HUGE_SEND;      /* Add the bit for a huge message */
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, msg_size, NULL /* desc */ ,
                                          cq_data,
                                          MPIDI_OFI_av_to_phys(addr, receiver_nic, vni_local,
                                                               vni_remote),
                                          match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                             vni_local, tsenddata, FALSE /* eagain */);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_sent_bytes_count[sender_nic], msg_size);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, striped_nic_sent_bytes_count[sender_nic], msg_size);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                            int dst_rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIDI_av_entry_t * addr,
                                            int vni_src, int vni_dst,
                                            MPIR_Request ** request, int noreq,
                                            uint64_t syncflag, MPIR_Errflag_t err_flag)
{
    int dt_contig, mpi_errno;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_ENTER;

    uint64_t cq_data = comm->rank;
    MPIDI_OFI_idata_set_error_bits(&cq_data, err_flag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (likely(!syncflag && dt_contig && (data_sz <= MPIDI_OFI_global.max_buffered_send))) {
        MPI_Aint actual_pack_bytes = 0;
        void *send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(send_buf, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV) {
            if (!MPIDI_OFI_ENABLE_HMEM) {
                /* Force pack for GPU buffer. */
                void *host_buf = NULL;
                host_buf = MPL_malloc(data_sz, MPL_MEM_OTHER);
                MPIR_Typerep_pack(buf, count, datatype, 0, host_buf, data_sz, &actual_pack_bytes,
                                  MPIR_TYPEREP_FLAG_NONE);
                MPIR_Assert(actual_pack_bytes == data_sz);
                send_buf = host_buf;
            }
        }
        mpi_errno = MPIDI_OFI_send_lightweight(send_buf, data_sz, cq_data, dst_rank, tag, comm,
                                               context_offset, addr, vni_src, vni_dst);
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
                                          context_offset, addr, vni_src, vni_dst, request,
                                          dt_contig, data_sz, dt_ptr, dt_true_lb, syncflag);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Common macro used by all MPIDI_NM_mpi_send routines to facilitate tuning */
#define MPIDI_OFI_SEND_VNIS(vni_src_, vni_dst_) \
    do { \
        if (*request != NULL) { \
            /* workq path */ \
            vni_src_ = 0; \
            vni_dst_ = 0; \
        } else { \
            MPIDI_EXPLICIT_VCIS(comm, attr, comm->rank, rank, vni_src_, vni_dst_); \
            if (vni_src_ == 0 && vni_dst_ == 0) { \
                vni_src_ = MPIDI_get_vci(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
                vni_dst_ = MPIDI_get_vci(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
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

    int vni_src, vni_dst;
    MPIDI_OFI_SEND_VNIS(vni_src, vni_dst);      /* defined just above */

    MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vni_src);
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        bool syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr) ? MPIDIG_AM_SEND_FLAGS_SYNC : 0;
        mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                     vni_src, vni_dst, request, syncflag, errflag);
    } else {
        uint64_t syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr) ? MPIDI_OFI_SYNC_SEND : 0;
        mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, vni_src, vni_dst,
                                   request, 0, syncflag, errflag);
    }
    MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vni_src);

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
