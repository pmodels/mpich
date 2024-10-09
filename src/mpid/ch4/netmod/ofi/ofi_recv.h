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
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_iov(void *buf, MPI_Aint count, MPI_Datatype datatype, size_t data_sz,       /* data_sz passed in here for reusing */
                                                int rank, uint64_t match_bits, uint64_t mask_bits,
                                                MPIR_Comm * comm, MPIR_Context_id_t context_id,
                                                MPIDI_av_entry_t * addr, int vci_src, int vci_dst,
                                                MPIR_Request * rreq,
                                                MPIR_Datatype * dt_ptr, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_msg_tagged msg;
    MPI_Aint num_contig;
    int vci_remote = vci_src;
    int vci_local = vci_dst;
    int ctx_idx;
    int receiver_nic;

    MPIR_FUNC_ENTER;

    /* if we cannot fit the entire data into a single IOV array,
     * fallback to pack */
    MPIR_Typerep_get_iov_len(count, datatype, &num_contig);
    if (num_contig > MPIDI_OFI_global.rx_iov_limit)
        goto unpack;

    /* Calculate the correct NICs. */
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->recvcontext_id, rank, comm->rank,
                                           MPIDI_OFI_init_get_tag(match_bits));
    MPIDI_OFI_REQUEST(rreq, nic_num) = receiver_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_dst, MPIDI_OFI_REQUEST(rreq, nic_num));

    if (!flags) {
        flags = FI_COMPLETION;
    }

    struct iovec *iovs;
    iovs = MPL_malloc(num_contig * sizeof(struct iovec), MPL_MEM_BUFFER);
    MPIDI_OFI_REQUEST(rreq, noncontig.nopack.iovs) = iovs;
    MPIDI_OFI_REQUEST(rreq, noncontig.nopack.datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    MPI_Aint actual_iov_len;
    MPIR_Typerep_to_iov_offset(buf, count, datatype, 0, iovs, num_contig, &actual_iov_len);
    assert(num_contig == actual_iov_len);

    if (rreq->comm == NULL) {
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    }
    /* Read ordering unnecessary for context_id, so use relaxed load */
    MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(rreq, util_id), context_id);

    MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_NOPACK;

    msg.msg_iov = iovs;
    msg.desc = NULL;
    msg.iov_count = num_contig;
    msg.tag = match_bits;
    msg.ignore = mask_bits;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
    msg.data = 0;
    if (MPI_ANY_SOURCE == rank) {
        msg.addr = FI_ADDR_UNSPEC;
    } else {
        int sender_nic = MPIDI_OFI_multx_sender_nic_index(comm, comm->recvcontext_id,
                                                          rank, comm->rank,
                                                          MPIDI_OFI_init_get_tag(match_bits));
        msg.addr = MPIDI_OFI_av_to_phys(addr, sender_nic, vci_remote);
    }

    MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx, &msg, flags), vci_local,
                         trecv);

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
                                                MPIDI_av_entry_t * addr, int vci_src, int vci_dst,
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
    int vci_remote = vci_src;
    int vci_local = vci_dst;
    int receiver_nic;
    int ctx_idx;
    bool register_mem = false;
    struct fid_mr *mr = NULL;
    void *desc = NULL;

    MPIR_FUNC_ENTER;

    if (mode == MPIDI_OFI_ON_HEAP) {    /* Branch should compile out */
        MPIDI_OFI_REQUEST_CREATE(*request, MPIR_REQUEST_KIND__RECV, vci_dst);
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
    MPIDI_OFI_REQUEST(rreq, kind) = MPIDI_OFI_req_kind__any;
    if (!flags) {
        MPIDI_OFI_REQUEST(rreq, huge.remote_info) = NULL;       /* for huge recv remote info */
    }

    /* Calculate the correct NICs. */
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->recvcontext_id, rank, comm->rank, tag);
    MPIDI_OFI_REQUEST(rreq, nic_num) = receiver_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_dst, MPIDI_OFI_REQUEST(rreq, nic_num));

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, rank, tag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    recv_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    MPL_pointer_attr_t attr = {.type = MPL_GPU_POINTER_UNREGISTERED_HOST };
    MPIR_GPU_query_pointer_attr(recv_buf, &attr);

    if (MPIDI_OFI_ENABLE_HMEM && data_sz >= MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD &&
        MPIDI_OFI_ENABLE_MR_HMEM && dt_contig) {
        if (MPL_gpu_attr_is_strict_dev(&attr)) {
            register_mem = true;
        }
    }

    if (data_sz && MPL_gpu_attr_is_dev(&attr)) {
        MPIDI_OFI_register_am_bufs();
        if (!MPIDI_OFI_ENABLE_HMEM || !dt_contig || (MPIDI_OFI_ENABLE_MR_HMEM && !register_mem)) {
            /* FIXME: at this point, GPU data takes host-buffer staging
             * path for the whole chunk. For large memory size, pipeline
             * transfer should be applied. */
            force_gpu_pack = true;
        }
    }

    if (register_mem) {
        MPIDI_OFI_register_memory_and_bind(recv_buf, data_sz, &attr, ctx_idx, &mr);
        if (mr != NULL) {
            desc = fi_mr_desc(mr);
        }
    }

    MPIR_REQUEST_SET_INFO(rreq, "MPIDI_OFI_do_irecv: source=%d, tag=%d, data_sz=%ld", rank, tag,
                          data_sz);
    if (!dt_contig || force_gpu_pack) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && !force_gpu_pack &&
            ((data_sz < MPIDI_OFI_global.max_msg_size && !MPIDI_OFI_COMM(comm).enable_striping) ||
             (data_sz < MPIDI_OFI_global.stripe_threshold &&
              MPIDI_OFI_COMM(comm).enable_striping))) {
            mpi_errno = MPIDI_OFI_recv_iov(buf, count, datatype, data_sz, rank,
                                           match_bits, mask_bits, comm, context_id, addr,
                                           vci_src, vci_dst, rreq, dt_ptr, flags);
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

        if (force_gpu_pack && MPIR_CVAR_CH4_OFI_ENABLE_GPU_PIPELINE &&
            data_sz >= MPIR_CVAR_CH4_OFI_GPU_PIPELINE_THRESHOLD) {
            /* Pipeline path */
            MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(rreq, util_id), context_id);
            MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_GPU_PIPELINE_INIT;
            /* Only post first recv with pipeline chunk size. */
            char *host_buf = NULL;
            MPIDU_genq_private_pool_force_alloc_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool,
                                                     (void **) &host_buf);
            MPIR_ERR_CHKANDJUMP1(host_buf == NULL, mpi_errno,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "Pipeline Init recv alloc");

            fi_addr_t remote_addr;
            if (MPI_ANY_SOURCE == rank)
                remote_addr = FI_ADDR_UNSPEC;
            else {
                int sender_nic =
                    MPIDI_OFI_multx_sender_nic_index(comm, comm->recvcontext_id, rank, comm->rank,
                                                     MPIDI_OFI_init_get_tag(match_bits));
                remote_addr = MPIDI_OFI_av_to_phys(addr, sender_nic, vci_remote);
            }

            /* Save pipeline information. */
            MPIDI_OFI_REQUEST(rreq, pipeline_info.offset) = 0;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.is_sync) = false;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.remote_addr) = remote_addr;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.vci_local) = vci_local;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.match_bits) = match_bits;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.mask_bits) = mask_bits;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.data_sz) = data_sz;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.ctx_idx) = ctx_idx;

            /* Save original buf, datatype and count */
            MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer) = host_buf;
            MPIDI_OFI_REQUEST(rreq, noncontig.pack.buf) = buf;
            MPIDI_OFI_REQUEST(rreq, noncontig.pack.count) = count;
            MPIDI_OFI_REQUEST(rreq, noncontig.pack.datatype) = datatype;
            MPIR_Datatype_add_ref_if_not_builtin(datatype);

            if (rreq->comm == NULL) {
                rreq->comm = comm;
                MPIR_Comm_add_ref(comm);
            }

            MPIDI_OFI_gpu_pipeline_request *chunk_req;
            chunk_req = (MPIDI_OFI_gpu_pipeline_request *)
                MPL_malloc(sizeof(MPIDI_OFI_gpu_pipeline_request), MPL_MEM_BUFFER);
            MPIR_ERR_CHKANDJUMP1(chunk_req == NULL, mpi_errno,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv chunk_req alloc");
            chunk_req->event_id = MPIDI_OFI_EVENT_RECV_GPU_PIPELINE_INIT;
            chunk_req->parent = rreq;
            chunk_req->buf = host_buf;
            int ret = 0;
            ret = fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                           host_buf,
                           MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ,
                           NULL, remote_addr, match_bits, mask_bits, (void *) &chunk_req->context);
            if (MPIDI_OFI_global.gpu_recv_queue || !host_buf || ret != 0) {
                MPIDI_OFI_gpu_pending_recv_t *recv_task =
                    MPIDI_OFI_create_recv_task(chunk_req, 0, -1);
                DL_APPEND(MPIDI_OFI_global.gpu_recv_queue, recv_task);
            }
            goto fn_exit;
        }

        /* Unpack */
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_PACK;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer) =
            MPL_aligned_alloc(64, data_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv Pack Buffer alloc");
        recv_buf = MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer);
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.buf) = buf;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.count) = count;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.datatype) = datatype;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
    } else {
        MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer) = NULL;
    }

    if (rreq->comm == NULL) {
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    }
    /* Read ordering unnecessary for context_id, so use relaxed load */
    MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(rreq, util_id), context_id);
    MPIDI_OFI_REQUEST(rreq, util.iov.iov_base) = recv_buf;
    MPIDI_OFI_REQUEST(rreq, util.iov.iov_len) = data_sz;

    if (unlikely(data_sz >= MPIDI_OFI_global.max_msg_size) && !MPIDI_OFI_COMM(comm).enable_striping) {
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_HUGE;
        data_sz = MPIDI_OFI_global.max_msg_size;
    } else if (MPIDI_OFI_COMM(comm).enable_striping &&
               (data_sz >= MPIDI_OFI_global.stripe_threshold)) {
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_HUGE;
        /* Receive has to be posted with size MPIDI_OFI_global.stripe_threshold to handle underflow */
        data_sz = MPIDI_OFI_global.stripe_threshold;
    } else if (MPIDI_OFI_REQUEST(rreq, event_id) != MPIDI_OFI_EVENT_RECV_PACK)
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV;

    if (!flags) {
        fi_addr_t sender_addr;
        if (MPI_ANY_SOURCE == rank) {
            sender_addr = FI_ADDR_UNSPEC;
        } else {
            int sender_nic = MPIDI_OFI_multx_sender_nic_index(comm, comm->recvcontext_id,
                                                              rank, comm->rank,
                                                              MPIDI_OFI_init_get_tag(match_bits));
            sender_addr = MPIDI_OFI_av_to_phys(addr, sender_nic, vci_remote);
        }
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                      recv_buf, data_sz, desc, sender_addr, match_bits, mask_bits,
                                      (void *) &(MPIDI_OFI_REQUEST(rreq, context))), vci_local,
                             trecv);
    } else {
        msg.msg_iov = &MPIDI_OFI_REQUEST(rreq, util.iov);
        msg.desc = desc;
        msg.iov_count = 1;
        msg.tag = match_bits;
        msg.ignore = mask_bits;
        msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
        msg.data = 0;
        msg.addr = FI_ADDR_UNSPEC;

        MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx, &msg, flags), vci_local,
                             trecv);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Common macro used by all MPIDI_NM_mpi_irecv routines to facilitate tuning */
#define MPIDI_OFI_RECV_VNIS(vci_src_, vci_dst_) \
    do { \
        if (*request != NULL) { \
            /* workq path  or collectives */ \
            vci_src_ = 0; \
            vci_dst_ = 0; \
        } else { \
            /* NOTE: hashing is based on target rank */ \
            MPIDI_EXPLICIT_VCIS(comm, attr, rank, comm->rank, vci_src_, vci_dst_); \
            if (vci_src_ == 0 && vci_dst_ == 0) { \
                vci_src_ = MPIDI_get_vci(SRC_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
                vci_dst_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
            } \
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

    int vci = MPIDI_Request_get_vci(message);
    MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci);
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    } else {
        rreq = message;
        av = MPIDIU_comm_rank_to_av(rreq->comm, message->status.MPI_SOURCE);
        /* FIXME: need get vci_src in the request */
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, message->status.MPI_SOURCE,
                                       message->status.MPI_TAG, rreq->comm, 0, av, 0, vci,
                                       &rreq, MPIDI_OFI_USE_EXISTING, FI_CLAIM | FI_COMPLETION);
    }
    MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                MPIR_Request * partner)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vci_src, vci_dst;
    MPIDI_OFI_RECV_VNIS(vci_src, vci_dst);

    /* For anysource recv, we may be called while holding the vci lock of shm request (to
     * prevent shm progress). */
    int need_cs;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    need_cs = true;
#else
    need_cs = (rank != MPI_ANY_SOURCE);
#endif

    if (need_cs) {
        MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci_dst);
    } else {
#ifdef MPICH_DEBUG_MUTEX
        MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI(vci_dst).lock);
#endif
    }
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                     vci_dst, request, 0, partner);
    } else {
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, rank, tag, comm,
                                       context_offset, addr, vci_src, vci_dst, request,
                                       MPIDI_OFI_ON_HEAP, 0ULL);
        MPIDI_REQUEST_SET_LOCAL(*request, 0, partner);
    }
    if (need_cs) {
        MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci_dst);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq, bool is_blocking)
{

    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_Request_get_vci(rreq);
    int ctx_idx = MPIDI_OFI_get_ctx_index(vci, MPIDI_OFI_REQUEST(rreq, nic_num));

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_cancel_recv(rreq);
        goto fn_exit;
    }

    /* We can't rely on the return code of fi_cancel, assume always successful.
     * ref: https://github.com/ofiwg/libfabric/issues/7795
     */
    fi_cancel((fid_t) MPIDI_OFI_global.ctx[ctx_idx].rx, &(MPIDI_OFI_REQUEST(rreq, context)));

    if (is_blocking) {
        /* progress until the rreq completes, either with cancel-bit set,
         * or with message received */
        while (!MPIR_cc_is_complete(&rreq->cc)) {
            mpi_errno = MPIDI_OFI_progress_uninlined(vci);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        /* run progress once to prevent accumulating cq errors. */
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_RECV_H_INCLUDED */
