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
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(vni_local, nic);
    uint64_t match_bits;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, tag, 0);
    MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                        buf,
                                        data_sz,
                                        cq_data,
                                        MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote),
                                        match_bits),
                         vni_local, tinjectdata, comm->hints[MPIR_COMM_HINT_EAGAIN]);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
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
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(vni_local, nic);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_IOV);

    /* if we cannot fit the entire data into a single IOV array,
     * fallback to pack */
    MPIR_Typerep_iov_len(count, MPIDI_OFI_REQUEST(sreq, datatype), dt_ptr->size * count,
                         &num_contig);
    if (num_contig > MPIDI_OFI_global.tx_iov_limit)
        goto pack;

    /* everything fits in the IOV array */
    flags = FI_COMPLETION | FI_REMOTE_CQ_DATA;
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_NOPACK;

    size = num_contig * sizeof(struct iovec) + sizeof(*(MPIDI_OFI_REQUEST(sreq, noncontig.nopack)));

    MPIDI_OFI_REQUEST(sreq, noncontig.nopack) = MPL_malloc(size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(sreq, noncontig.nopack), 0, size);

    MPI_Aint actual_iov_len, actual_iov_bytes;
    MPIR_Typerep_to_iov(buf, count, MPIDI_OFI_REQUEST(sreq, datatype), 0,
                        MPIDI_OFI_REQUEST(sreq, noncontig.nopack), num_contig, dt_ptr->size * count,
                        &actual_iov_len, &actual_iov_bytes);
    assert(num_contig == actual_iov_len);

    originv = &(MPIDI_OFI_REQUEST(sreq, noncontig.nopack[0]));

    msg.msg_iov = originv;
    msg.desc = NULL;
    msg.iov_count = num_contig;
    msg.tag = match_bits;
    msg.ignore = 0ULL;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(sreq, context));
    msg.data = cq_data;
    msg.addr = MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote);

    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                     &msg, flags), vni_local, tsendv, FALSE);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_IOV);
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
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(vni_local, nic);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_NORMAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_NORMAL);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    /* TODO: what cases when *request is NULL under workq? */
    if (*request) {
        MPIR_Request_add_ref(*request);
    } else
#endif
    {
        MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__SEND, vni_src);
    }

    MPIR_Request *sreq = *request;

    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, tag, type);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

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
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx, /* endpoint    */
                                      NULL,     /* recvbuf     */
                                      0,        /* data sz     */
                                      NULL,     /* memregion descr  */
                                      MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote),   /* remote proc */
                                      ssend_match,      /* match bits  */
                                      0ULL,     /* mask bits   */
                                      (void *) &(ackreq->context)), vni_local, trecvsync, FALSE);
    }

    send_buf = (char *) buf + dt_true_lb;
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
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && data_sz < MPIDI_OFI_global.max_msg_size &&
            !force_gpu_pack) {
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

        /* FIXME: allocating a GPU registered host buffer adds some additional overhead.
         * However, once the new buffer pool infrastructure is setup, we would simply be
         * allocating a buffer from the pool, so whether it's a regular malloc buffer or a GPU
         * registered buffer should be equivalent with respect to performance. */
        MPIR_gpu_malloc_host((void **) &MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer),
                             data_sz);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");

        MPI_Aint actual_pack_bytes;
        MPIR_Typerep_pack(buf, count, datatype, 0,
                          MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer), data_sz,
                          &actual_pack_bytes);
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
                                            MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote),
                                            match_bits), vni_local, tinjectdata,
                             FALSE /* eagain */);
        MPIDI_OFI_send_event(NULL, sreq, MPIDI_OFI_REQUEST(sreq, event_id));
    } else if (data_sz < MPIDI_OFI_global.max_msg_size) {
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, data_sz, NULL /* desc */ ,
                                          cq_data,
                                          MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote),
                                          match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                             vni_local, tsenddata, FALSE /* eagain */);
    } else if (unlikely(1)) {
        MPIDI_OFI_send_control_t ctrl;
        uint64_t rma_key = 0;
        struct fid_mr *huge_send_mr;

        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_HUGE;
        MPIR_cc_inc(sreq->cc_ptr);

        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            /* Set up a memory region for the lmt data transfer */
            ctrl.rma_key = MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
            rma_key = ctrl.rma_key;
        }

        MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[ctx_idx].domain,  /* In:  Domain Object */
                                 send_buf,      /* In:  Lower memory address */
                                 data_sz,       /* In:  Length              */
                                 FI_REMOTE_READ,        /* In:  Expose MR for read  */
                                 0ULL,  /* In:  offset(not used)    */
                                 rma_key,       /* In:  requested key       */
                                 0ULL,  /* In:  flags               */
                                 &huge_send_mr, /* Out: memregion object    */
                                 NULL), mr_reg);        /* In:  context             */

        /* Create map to the memory region */
        MPIDIU_map_set(MPIDI_OFI_COMM(comm).huge_send_counters, sreq->handle, huge_send_mr,
                       MPL_MEM_BUFFER);

        if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            /* MR_BASIC */
            ctrl.rma_key = fi_mr_key(huge_send_mr);
        }

        /* Send the maximum amount of data that we can here to get things
         * started, then do the rest using the MR below. This can be confirmed
         * in the MPIDI_OFI_get_huge code where we start the offset at
         * MPIDI_OFI_global.max_msg_size */
        sreq->comm = comm;
        MPIR_Comm_add_ref(comm);
        MPIDI_OFI_REQUEST(sreq, util_id) = dst_rank;
        match_bits |= MPIDI_OFI_HUGE_SEND;      /* Add the bit for a huge message */
        MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                          send_buf, MPIDI_OFI_global.max_msg_size, NULL /* desc */ ,
                                          cq_data,
                                          MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote),
                                          match_bits,
                                          (void *) &(MPIDI_OFI_REQUEST(sreq, context))),
                             vni_local, tsenddata, FALSE /* eagain */);
        ctrl.type = MPIDI_OFI_CTRL_HUGE;
        ctrl.seqno = 0;
        ctrl.tag = tag;
        ctrl.vni_src = vni_src;
        ctrl.vni_dst = vni_dst;

        /* Send information about the memory region here to get the lmt going. */
        mpi_errno = MPIDI_OFI_do_control_send(&ctrl, send_buf, data_sz, dst_rank, comm, sreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_NORMAL);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND);

    uint64_t cq_data = comm->rank;
    MPIDI_OFI_idata_set_error_bits(&cq_data, err_flag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (likely(!syncflag && dt_contig && (data_sz <= MPIDI_OFI_global.max_buffered_send))) {
        MPI_Aint actual_pack_bytes = 0;
        void *send_buf = (char *) buf + dt_true_lb;
        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(send_buf, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV) {
            if (!MPIDI_OFI_ENABLE_HMEM) {
                /* Force pack for GPU buffer. */
                void *host_buf = NULL;
                MPIR_gpu_malloc_host(&host_buf, data_sz);
                MPIR_Typerep_pack(buf, count, datatype, 0, host_buf, data_sz, &actual_pack_bytes);
                MPIR_Assert(actual_pack_bytes == data_sz);
                send_buf = host_buf;
            }
        }
        mpi_errno = MPIDI_OFI_send_lightweight(send_buf, data_sz, cq_data, dst_rank, tag, comm,
                                               context_offset, addr, vni_src, vni_dst);
        if (actual_pack_bytes > 0) {
            /* Free stage host buf (assigned to send_buf already) after
             * lightweight_send. */
            MPIR_gpu_free_host(send_buf);
        }
        if (!noreq) {
            *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        }
    } else {
        mpi_errno = MPIDI_OFI_send_normal(buf, count, datatype, cq_data, dst_rank, tag, comm,
                                          context_offset, addr, vni_src, vni_dst, request,
                                          dt_contig, data_sz, dt_ptr, dt_true_lb, syncflag);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND);
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
            vni_src_ = MPIDI_OFI_get_vni(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
            vni_dst_ = MPIDI_OFI_get_vni(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
    } else {
        int vni_src, vni_dst;
        MPIDI_OFI_SEND_VNIS(vni_src, vni_dst);  /* defined just above */
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);
        mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, vni_src, vni_dst,
                                   request, 0, 0ULL, MPIR_ERR_NONE);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISEND);
    return mpi_errno;
}

/*@
    MPIDI_NM_isend_coll - NM_mpi_isend function for collectives which takes an additional error
    flag argument.
Input Parameters:
    buf - starting address of buffer to send
    count - number of elements to send
    datatype - data type of each send buffer element
    dst_rank - rank of destination rank
    tag - message tag
    comm - handle to communicator
    context_offset - offset into context object
    addr - address vector entry for destination
    request - handle to request pointer
    errflag - the error flag to be passed along with the message
@*/
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend_coll(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr,
                                                 MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_ISEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_ISEND_COLL);

    /* NOTE: collective use vci 0 and critical section taken at ch4-layer */
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, addr,
                              request, errflag);
    } else {
        int vni_src, vni_dst;
        MPIDI_OFI_SEND_VNIS(vni_src, vni_dst);  /* defined just above */
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);
        mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, vni_src, vni_dst, request, 0, 0ULL,
                                   *errflag);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_ISEND_COLL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISSEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
    } else {
        int vni_src, vni_dst;
        MPIDI_OFI_SEND_VNIS(vni_src, vni_dst);  /* defined just above */
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);
        mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, vni_src, vni_dst, request, 0,
                                   MPIDI_OFI_SYNC_SEND, MPIR_ERR_NONE);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    /* Sends cannot be cancelled */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    return mpi_errno;
}

#endif /* OFI_SEND_H_INCLUDED */
