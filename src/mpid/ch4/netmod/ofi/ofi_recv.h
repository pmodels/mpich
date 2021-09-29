/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_RECV_H_INCLUDED
#define OFI_RECV_H_INCLUDED

#include "ofi_impl.h"

#define MPIDI_OFI_ON_HEAP      0
#define MPIDI_OFI_USE_EXISTING 1

#define MPIDI_OFI_RECV_NEEDS_UNPACK (-1)
/*
  Tries to post receive for non-contiguous datatype using iovec.

  Return value:
  MPI_SUCCESS: Message was sent successfully using iovec
  MPIDI_OFI_RECV_NEEDS_UNPACK: There was no error but receive was not posted
      due to limitations with iovec. Needs to fall back to the unpack path.
  Other: An error occurred as indicated in the code.
*/
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_iov(void *buf, MPI_Aint count, size_t data_sz,      /* data_sz passed in here for reusing */
                                                int rank, uint64_t match_bits, uint64_t mask_bits,
                                                MPIR_Comm * comm, MPIR_Context_id_t context_id,
                                                MPIDI_av_entry_t * addr, int vni_src, int vni_dst,
                                                MPIR_Request * rreq,
                                                MPIR_Datatype * dt_ptr, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    struct iovec *originv = NULL;
    struct fi_msg_tagged msg;
    MPI_Aint num_contig, size;
    int vni_remote = vni_src;
    int vni_local = vni_dst;
    int ctx_idx = 0;
    int sender_nic = 0, receiver_nic = 0;

    MPIR_FUNC_ENTER;

    /* if we cannot fit the entire data into a single IOV array,
     * fallback to pack */
    MPIR_Typerep_iov_len(count, MPIDI_OFI_REQUEST(rreq, datatype), dt_ptr->size * count,
                         &num_contig);
    if (num_contig > MPIDI_OFI_global.rx_iov_limit)
        goto unpack;

    /* Calculate the correct NICs. */
    sender_nic = MPIDI_OFI_multx_sender_nic_index(comm, comm->recvcontext_id, MPIR_Process.rank,
                                                  MPIDI_OFI_init_get_tag(match_bits));
    receiver_nic = MPIDI_OFI_multx_receiver_nic_index(comm, comm->recvcontext_id, rank,
                                                      MPIDI_OFI_init_get_tag(match_bits));
    MPIDI_OFI_REQUEST(rreq, nic_num) = receiver_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_dst, MPIDI_OFI_REQUEST(rreq, nic_num));

    if (!flags) {
        flags = FI_COMPLETION;
    }

    size = num_contig * sizeof(struct iovec) + sizeof(*(MPIDI_OFI_REQUEST(rreq, noncontig.nopack)));

    MPIDI_OFI_REQUEST(rreq, noncontig.nopack) = MPL_malloc(size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(rreq, noncontig.nopack), 0, size);

    MPI_Aint actual_iov_len, actual_iov_bytes;
    MPIR_Typerep_to_iov(buf, count, MPIDI_OFI_REQUEST(rreq, datatype), 0,
                        MPIDI_OFI_REQUEST(rreq, noncontig.nopack), num_contig, dt_ptr->size * count,
                        &actual_iov_len, &actual_iov_bytes);
    assert(num_contig == actual_iov_len);

    originv = &(MPIDI_OFI_REQUEST(rreq, noncontig.nopack[0]));

    if (rreq->comm == NULL) {
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    }
    /* Read ordering unnecessary for context_id, so use relaxed load */
    MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(rreq, util_id), context_id);

    MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_NOPACK;

    msg.msg_iov = originv;
    msg.desc = NULL;
    msg.iov_count = num_contig;
    msg.tag = match_bits;
    msg.ignore = mask_bits;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
    msg.data = 0;
    msg.addr =
        (MPI_ANY_SOURCE == rank) ? FI_ADDR_UNSPEC : MPIDI_OFI_av_to_phys(addr, sender_nic,
                                                                         vni_local, vni_remote);

    MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx, &msg, flags), vni_local,
                         trecv, FALSE);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  unpack:
    mpi_errno = MPIDI_OFI_RECV_NEEDS_UNPACK;
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm,
                                                int context_offset,
                                                MPIDI_av_entry_t * addr, int vni_src, int vni_dst,
                                                MPIR_Request ** request, int mode, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    uint64_t match_bits, mask_bits;
    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    struct fi_msg_tagged msg;
    char *recv_buf;
    bool force_gpu_pack = false;
    int vni_remote = vni_src;
    int vni_local = vni_dst;
    int sender_nic = 0, receiver_nic = 0;
    int ctx_idx = 0;

    MPIR_FUNC_ENTER;

    if (mode == MPIDI_OFI_ON_HEAP) {    /* Branch should compile out */
#ifdef MPIDI_CH4_USE_WORK_QUEUES
        /* TODO: what cases when *request is NULL under workq? */
        if (*request) {
            MPIR_Request_add_ref(*request);
        } else
#endif
        {
            MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__RECV, vni_dst);
        }
        rreq = *request;

        /* Need to set the source to UNDEFINED for anysource matching */
        rreq->status.MPI_SOURCE = MPI_UNDEFINED;
    } else if (mode == MPIDI_OFI_USE_EXISTING) {
        rreq = *request;
    } else {
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nullptr");
        goto fn_fail;
    }

    *request = rreq;

    /* Calculate the correct NICs. */
    sender_nic = MPIDI_OFI_multx_sender_nic_index(comm, comm->recvcontext_id, MPIR_Process.rank,
                                                  tag);
    receiver_nic = MPIDI_OFI_multx_receiver_nic_index(comm, comm->recvcontext_id, rank, tag);
    MPIDI_OFI_REQUEST(rreq, nic_num) = receiver_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_dst, MPIDI_OFI_REQUEST(rreq, nic_num));

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, tag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIDI_OFI_REQUEST(rreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    recv_buf = (char *) buf + dt_true_lb;
    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(recv_buf, &attr);
    if (data_sz && attr.type == MPL_GPU_POINTER_DEV) {
        if (!MPIDI_OFI_ENABLE_HMEM) {
            /* FIXME: at this point, GPU data takes host-buffer staging
             * path for the whole chunk. For large memory size, pipeline
             * transfer should be applied. */
            force_gpu_pack = true;
            dt_contig = 0;
        }
    }

    if (!dt_contig) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && !force_gpu_pack &&
            ((data_sz < MPIDI_OFI_global.max_msg_size && !MPIDI_OFI_COMM(comm).enable_striping) ||
             (data_sz < MPIDI_OFI_global.stripe_threshold &&
              MPIDI_OFI_COMM(comm).enable_striping))) {
            mpi_errno =
                MPIDI_OFI_recv_iov(buf, count, data_sz, rank, match_bits, mask_bits, comm,
                                   context_id, addr, vni_src, vni_dst, rreq, dt_ptr, flags);
            if (mpi_errno == MPI_SUCCESS)       /* Receive posted using iov */
                goto fn_exit;
            else if (mpi_errno != MPIDI_OFI_RECV_NEEDS_UNPACK)
                goto fn_fail;
            /* recv_iov returned MPIDI_OFI_RECV_NEEDS_UNPACK -- indicating
             * that there was no error but it couldn't post the receive
             * due to iov limitations. We need to fall back to the unpack
             * path below. Simply falling through. */
            mpi_errno = MPI_SUCCESS;    /* Reset error code */
        }
        /* Unpack */
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_PACK;
        /* FIXME: allocating a GPU registered host buffer adds some additional overhead.
         * However, once the new buffer pool infrastructure is setup, we would simply be
         * allocating a buffer from the pool, so whether it's a regular malloc buffer or a GPU
         * registered buffer should be equivalent with respect to performance. */
        MPIR_gpu_malloc_host((void **) &MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer),
                             data_sz);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv Pack Buffer alloc");
        recv_buf = MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer);
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.buf) = buf;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.count) = count;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.datatype) = datatype;
    } else {
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer) = NULL;
        MPIDI_OFI_REQUEST(rreq, noncontig.nopack) = NULL;
    }

    if (rreq->comm == NULL) {
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    }
    /* Read ordering unnecessary for context_id, so use relaxed load */
    MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(rreq, util_id), context_id);

    if (unlikely(data_sz >= MPIDI_OFI_global.max_msg_size) && !MPIDI_OFI_COMM(comm).enable_striping) {
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_HUGE;
        data_sz = MPIDI_OFI_global.max_msg_size;
    } else if (MPIDI_OFI_COMM(comm).enable_striping &&
               (data_sz >= MPIDI_OFI_global.stripe_threshold)) {
        MPIDI_OFI_huge_recv_t *huge_recv = (MPIDI_OFI_huge_recv_t *) rreq;
        huge_recv->chunks_outstanding = 0;
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_HUGE;
        /* Receive has to be posted with size MPIDI_OFI_global.stripe_threshold to handle underflow */
        data_sz = MPIDI_OFI_global.stripe_threshold;
    } else if (MPIDI_OFI_REQUEST(rreq, event_id) != MPIDI_OFI_EVENT_RECV_PACK)
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV;

    MPIDI_OFI_REQUEST(rreq, util.iov.iov_base) = recv_buf;
    MPIDI_OFI_REQUEST(rreq, util.iov.iov_len) = data_sz;
    if (!flags) {
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                      recv_buf,
                                      data_sz,
                                      NULL,
                                      (MPI_ANY_SOURCE == rank) ? FI_ADDR_UNSPEC :
                                      MPIDI_OFI_av_to_phys(addr, sender_nic, vni_local, vni_remote),
                                      match_bits, mask_bits,
                                      (void *) &(MPIDI_OFI_REQUEST(rreq, context))), vni_local,
                             trecv, FALSE);
    } else {
        msg.msg_iov = &MPIDI_OFI_REQUEST(rreq, util.iov);
        msg.desc = NULL;
        msg.iov_count = 1;
        msg.tag = match_bits;
        msg.ignore = mask_bits;
        msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
        msg.data = 0;
        msg.addr = FI_ADDR_UNSPEC;

        MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx, &msg, flags), vni_local,
                             trecv, FALSE);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Common macro used by all MPIDI_NM_mpi_irecv routines to facilitate tuning */
#define MPIDI_OFI_RECV_VNIS(vni_src_, vni_dst_) \
    do { \
        if (*request != NULL) { \
            /* workq path  or collectives */ \
            vni_src_ = 0; \
            vni_dst_ = 0; \
        } else { \
            /* NOTE: hashing is based on target rank */ \
            vni_src_ = MPIDI_OFI_get_vni(SRC_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
            vni_dst_ = MPIDI_OFI_get_vni(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
        } \
    } while (0)


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype, MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_av_entry_t *av;
    MPIR_FUNC_ENTER;

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    } else {
        rreq = message;
        int vci = MPIDI_Request_get_vci(rreq);
        MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci);
        av = MPIDIU_comm_rank_to_av(rreq->comm, message->status.MPI_SOURCE);
        /* FIXME: need get vni_src in the request */
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, message->status.MPI_SOURCE,
                                       message->status.MPI_TAG, rreq->comm, 0, av, 0, vci,
                                       &rreq, MPIDI_OFI_USE_EXISTING, FI_CLAIM | FI_COMPLETION);
        MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                MPIR_Request * partner)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vni_src, vni_dst;
    MPIDI_OFI_RECV_VNIS(vni_src, vni_dst);
    /* For anysource recv, we may be called while holding the vci lock of shm request (to
     * prevent shm progress). Therefore, recursive locking is allowed here */
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                     vni_dst, request, 0, partner);
    } else {
        MPIDI_OFI_THREAD_CS_ENTER_REC_VCI_OPTIONAL(vni_dst);
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, rank, tag, comm,
                                       context_offset, addr, vni_src, vni_dst, request,
                                       MPIDI_OFI_ON_HEAP, 0ULL);
        MPIDI_REQUEST_SET_LOCAL(*request, 0, partner);
        MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vni_dst);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{

    int mpi_errno = MPI_SUCCESS;
    ssize_t ret;
    MPIR_FUNC_ENTER;

    int vni = MPIDI_Request_get_vci(rreq);
    int ctx_idx = MPIDI_OFI_get_ctx_index(rreq->comm, vni, MPIDI_OFI_REQUEST(rreq, nic_num));

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_cancel_recv(rreq);
        goto fn_exit;
    }

    /* Not using the OFI_CALL macro because there are error cases here that we want to catch */
    ret = fi_cancel((fid_t) MPIDI_OFI_global.ctx[ctx_idx].rx, &(MPIDI_OFI_REQUEST(rreq, context)));
    if (ret == -FI_ENOENT) {
        /* The context was not found inside libfabric which means it was matched previously and has
         * already been handled. Note that it is impossible to tell the difference in this case
         * between a request that was previously matched and one that never existed. So this will
         * probably never return an error if the user tries to cancel a request that was not
         * previously started. */
        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "Request not found. Assuming already cancelled"));
        goto fn_exit;
    } else if (ret < 0) {
        MPIR_ERR_CHKANDJUMP4(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ofid_cancel",
                             "**ofid_cancel %s %d %s %s", __SHORT_FILE__, __LINE__, __func__,
                             fi_strerror(-ret));
    }

    if (ret == 0) {
        while ((!MPIR_STATUS_GET_CANCEL_BIT(rreq->status)) && (!MPIR_cc_is_complete(&rreq->cc))) {
            /* The cancel is local and must complete, so only poll this device (not global progress) */
            mpi_errno = MPIDI_OFI_progress_uninlined(vni);
            MPIR_ERR_CHECK(mpi_errno);
        }

        if (MPIR_STATUS_GET_CANCEL_BIT(rreq->status)) {
            MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
            MPIR_STATUS_SET_COUNT(rreq->status, 0);
            MPIDIU_request_complete(rreq);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_RECV_H_INCLUDED */
