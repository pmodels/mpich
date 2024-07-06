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
                                                MPIDI_av_entry_t * addr, int vci_src, int vci_dst,
                                                MPIR_Request * rreq,
                                                MPIR_Datatype * dt_ptr, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    struct iovec *originv = NULL;
    struct fi_msg_tagged msg;
    MPI_Aint num_contig, size;
    int vci_remote = vci_src;
    int vci_local = vci_dst;
    int ctx_idx;
    int receiver_nic;

    MPIR_FUNC_ENTER;

    /* if we cannot fit the entire data into a single IOV array,
     * fallback to pack */
    MPIR_Typerep_get_iov_len(count, MPIDI_OFI_REQUEST(rreq, datatype), &num_contig);
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

    size = (num_contig + 1) * sizeof(struct iovec);

    MPIDI_OFI_REQUEST(rreq, u.nopack_recv.iovs) = MPL_malloc(size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(rreq, u.nopack_recv.iovs), 0, size);

    MPI_Aint actual_iov_len;
    MPIR_Typerep_to_iov_offset(buf, count, MPIDI_OFI_REQUEST(rreq, datatype), 0,
                               MPIDI_OFI_REQUEST(rreq, u.nopack_recv.iovs), num_contig,
                               &actual_iov_len);
    assert(num_contig == actual_iov_len);

    originv = &(MPIDI_OFI_REQUEST(rreq, u.nopack_recv.iovs[0]));

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

    MPIDI_OFI_REQUEST(rreq, kind) = MPIDI_OFI_req_kind__any;
    /* preset some fields to NULL */
    if (!flags) {
        /* remote_info may get set by mprobe, so exclude mrecv. */
        MPIDI_OFI_REQUEST(rreq, u.recv.remote_info) = NULL;
    }
    MPIDI_OFI_REQUEST(rreq, u.recv.pack_buffer) = NULL;

    /* Calculate the correct NICs. */
    receiver_nic =
        MPIDI_OFI_multx_receiver_nic_index(comm, comm->recvcontext_id, rank, comm->rank, tag);
    MPIDI_OFI_REQUEST(rreq, nic_num) = receiver_nic;
    ctx_idx = MPIDI_OFI_get_ctx_index(vci_dst, MPIDI_OFI_REQUEST(rreq, nic_num));

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, rank, tag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIDI_OFI_REQUEST(rreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    recv_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    MPL_pointer_attr_t attr = {.type = MPL_GPU_POINTER_UNREGISTERED_HOST };
    MPIR_GPU_query_pointer_attr(recv_buf, &attr);

    if (MPIDI_OFI_ENABLE_HMEM && data_sz >= MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD &&
        MPIDI_OFI_ENABLE_MR_HMEM && dt_contig) {
        if (attr.type == MPL_GPU_POINTER_DEV) {
            register_mem = true;
        }
    }

    if (data_sz && attr.type == MPL_GPU_POINTER_DEV) {
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

    if (!dt_contig || force_gpu_pack) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && !force_gpu_pack &&
            ((data_sz < MPIDI_OFI_global.max_msg_size && !MPIDI_OFI_COMM(comm).enable_striping) ||
             (data_sz < MPIDI_OFI_global.stripe_threshold &&
              MPIDI_OFI_COMM(comm).enable_striping))) {
            mpi_errno =
                MPIDI_OFI_recv_iov(buf, count, data_sz, rank, match_bits, mask_bits, comm,
                                   context_id, addr, vci_src, vci_dst, rreq, dt_ptr, flags);
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

            fi_addr_t remote_addr;
            if (MPI_ANY_SOURCE == rank)
                remote_addr = FI_ADDR_UNSPEC;
            else {
                int sender_nic =
                    MPIDI_OFI_multx_sender_nic_index(comm, comm->recvcontext_id, rank, comm->rank,
                                                     MPIDI_OFI_init_get_tag(match_bits));
                remote_addr = MPIDI_OFI_av_to_phys(addr, sender_nic, vci_remote);
            }

            if (rreq->comm == NULL) {
                rreq->comm = comm;
                MPIR_Comm_add_ref(comm);
            }

            mpi_errno = MPIDI_OFI_gpu_pipeline_recv(rreq, buf, count, datatype,
                                                    remote_addr, vci_local,
                                                    match_bits, mask_bits, data_sz, ctx_idx);
            goto fn_exit;
        }

        /* Unpack */
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_PACK;
        MPIDI_OFI_REQUEST(rreq, u.recv.pack_buffer) = MPL_malloc(data_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(rreq, u.recv.pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv Pack Buffer alloc");
        recv_buf = MPIDI_OFI_REQUEST(rreq, u.recv.pack_buffer);
        MPIDI_OFI_REQUEST(rreq, u.recv.buf) = buf;
#ifdef MPL_HAVE_ZE
        if (dt_contig && attr.type == MPL_GPU_POINTER_DEV) {
            int mpl_err = MPL_SUCCESS;
            void *ptr;
            mpl_err = MPL_ze_mmap_device_pointer(buf, &attr.device_attr, attr.device, &ptr);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**mpl_ze_mmap_device_ptr");
            MPIDI_OFI_REQUEST(rreq, u.recv.buf) = ptr;
        }
#endif
        MPIDI_OFI_REQUEST(rreq, u.recv.count) = count;
        MPIDI_OFI_REQUEST(rreq, u.recv.datatype) = datatype;
    }

    if (rreq->comm == NULL) {
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    }
    /* Read ordering unnecessary for context_id, so use relaxed load */
    MPL_atomic_relaxed_store_int(&MPIDI_OFI_REQUEST(rreq, util_id), context_id);
    /* msg_iov is needed to use fi_trecvmsg (with FI_CLAIM) or the huge recv path */
    MPIDI_OFI_REQUEST(rreq, u.recv.msg_iov.iov_base) = recv_buf;
    MPIDI_OFI_REQUEST(rreq, u.recv.msg_iov.iov_len) = data_sz;

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
        msg.msg_iov = &MPIDI_OFI_REQUEST(rreq, u.recv.msg_iov);
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
