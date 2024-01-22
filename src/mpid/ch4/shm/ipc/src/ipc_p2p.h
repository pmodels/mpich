/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_P2P_H_INCLUDED
#define IPC_P2P_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "ipc_pre.h"
#include "ipc_types.h"
#include "../xpmem/xpmem_post.h"
#include "../gpu/gpu_post.h"

/* Generic IPC protocols for P2P. */

/* Generic sender-initialized LMT routine with contig send buffer.
 *
 * If the send buffer is noncontiguous the submodule can first pack the
 * data into a temporary buffer and use the temporary buffer as the send
 * buffer with this call. The sender gets the memory attributes of the
 * specified buffer (which include IPC type and memory handle), and sends
 * to the receiver. The receiver will then open the remote memory handle
 * and perform direct data transfer.
 */

int MPIDI_IPC_complete(MPIR_Request * rreq, int ipc_type);
int MPIDI_IPC_rndv_cb(MPIR_Request * rreq);
int MPIDI_IPC_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                uint32_t attr, MPIR_Request ** req);

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_send_lmt(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, uintptr_t data_sz,
                                                 int is_contig,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t * addr,
                                                 MPIDI_IPCI_ipc_attr_t ipc_attr,
                                                 int vci_src, int vci_dst, MPIR_Request ** request,
                                                 bool syncflag, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIDI_IPC_rts_t am_hdr;
    MPIR_CHKLMEM_DECL(1);       /* we may need allocate hdr for non-contig case */

    int flags = syncflag ? MPIDIG_AM_SEND_FLAGS_SYNC : MPIDIG_AM_SEND_FLAGS_NONE;
    MPIR_FUNC_ENTER;

    /* Create send request */
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2, vci_src, vci_dst);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, u.send.dest) = rank;
    MPIDIG_REQUEST(sreq, count) = count;

    am_hdr.ipc_hdr.ipc_type = ipc_attr.ipc_type;
    am_hdr.ipc_hdr.ipc_handle = ipc_attr.ipc_handle;
    am_hdr.ipc_hdr.is_contig = is_contig;

    /* message matching info */
    am_hdr.hdr.src_rank = comm->rank;
    am_hdr.hdr.tag = tag;
    am_hdr.hdr.context_id = comm->context_id + context_offset;
    am_hdr.hdr.data_sz = data_sz;
    am_hdr.hdr.rndv_hdr_sz = sizeof(MPIDI_IPC_hdr);
    am_hdr.hdr.sreq_ptr = sreq;
    am_hdr.hdr.error_bits = errflag;
    am_hdr.hdr.flags = flags;
    MPIDIG_AM_SEND_SET_RNDV(am_hdr.hdr.flags, MPIDIG_RNDV_IPC);

    if (flags & MPIDIG_AM_SEND_FLAGS_SYNC) {
        MPIR_cc_inc(sreq->cc_ptr);      /* expecting SSEND_ACK */
    }

    int is_local = 1;
    void *hdr;
    MPI_Aint hdr_sz;
    if (is_contig) {
        hdr = &am_hdr;
        hdr_sz = sizeof(am_hdr);
    } else {
        int flattened_sz;
        void *flattened_dt;
        MPIR_Datatype_get_flattened(datatype, &flattened_dt, &flattened_sz);

        am_hdr.hdr.rndv_hdr_sz += flattened_sz;
        am_hdr.ipc_hdr.flattened_sz = flattened_sz;
        am_hdr.ipc_hdr.count = count;

        hdr_sz = sizeof(am_hdr) + flattened_sz;
        MPIR_CHKLMEM_MALLOC(hdr, void *, hdr_sz, mpi_errno, "hdr", MPL_MEM_OTHER);
        memcpy(hdr, &am_hdr, sizeof(am_hdr));
        memcpy((char *) hdr + sizeof(am_hdr), flattened_dt, flattened_sz);

    }
    CH4_CALL(am_send_hdr(rank, comm, MPIDIG_SEND, hdr, hdr_sz, vci_src, vci_dst),
             is_local, mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Generic receiver side handler for sender-initialized LMT with contig send buffer.
 *
 * The receiver opens the memory handle issued by sender and then performs unpack
 * to its recv buffer. It closes the memory handle after unpack and finally issues
 * LMT_FIN ack to the sender.
 */

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_copy_data(MPIDI_IPC_hdr * ipc_hdr, MPIR_Request * rreq,
                                                  const void *src_buf, MPI_Aint src_data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    if (ipc_hdr->is_contig) {
        MPI_Aint actual_unpack_bytes;
        mpi_errno = MPIR_Typerep_unpack(src_buf, src_data_sz,
                                        MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq,
                                                                                     count),
                                        MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes,
                                        MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        if (actual_unpack_bytes < src_data_sz) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
        }
        MPIR_Assert(actual_unpack_bytes == src_data_sz);
    } else {
        void *flattened_type = ipc_hdr + 1;
        MPIR_Datatype *dt = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        MPIR_Assert(dt);
        mpi_errno = MPIR_Typerep_unflatten(dt, flattened_type);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Localcopy(src_buf, ipc_hdr->count, dt->handle,
                                   MPIDIG_REQUEST(rreq, buffer),
                                   MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype));
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Datatype_free(dt);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX MPL_gpu_engine_type_t MPIDI_IPCI_choose_engine(int dev1, int dev2)
{
    if (MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE == MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE_auto) {
        /* Use the high-bandwidth copy engine when either 1) one of the buffers is a host buffer, or
         * 2) the copy is to the same device. Otherwise use the low-latency copy engine. */
        if (dev1 == -1 || dev2 == -1) {
            return MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;
        }

        if (MPL_gpu_query_is_same_dev(dev1, dev2)) {
            return MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;
        }

        return MPL_GPU_ENGINE_TYPE_COPY_LOW_LATENCY;
    } else if (MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE == MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE_compute) {
        return MPL_GPU_ENGINE_TYPE_COMPUTE;
    } else if (MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE ==
               MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE_copy_high_bandwidth) {
        return MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;
    } else if (MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE ==
               MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE_copy_low_latency) {
        return MPL_GPU_ENGINE_TYPE_COPY_LOW_LATENCY;
    } else {
        return MPL_GPU_ENGINE_TYPE_LAST;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_handle_lmt_recv(MPIDI_IPC_hdr * ipc_hdr,
                                                        size_t src_data_sz,
                                                        MPIR_Request * sreq_ptr,
                                                        MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *src_buf = NULL;
    uintptr_t data_sz, recv_data_sz;

    MPIR_FUNC_ENTER;

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, u.recv.source);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, u.recv.tag);

    /* attach remote buffer */
    if (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__XPMEM) {
        /* map */
        mpi_errno = MPIDI_XPMEM_ipc_handle_map(ipc_hdr->ipc_handle.xpmem, &src_buf);
        MPIR_ERR_CHECK(mpi_errno);
        /* copy */
        mpi_errno = MPIDI_IPCI_copy_data(ipc_hdr, rreq, src_buf, src_data_sz);
        MPIR_ERR_CHECK(mpi_errno);
        /* skip unmap */
    } else if (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__GPU) {
#ifdef MPL_HAVE_ZE
        bool do_mmap = (src_data_sz <= MPIR_CVAR_CH4_IPC_GPU_FAST_COPY_MAX_SIZE);
#else
        bool do_mmap = false;
#endif
        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(rreq, buffer), &attr);
        int dev_id = MPL_gpu_get_dev_id_from_attr(&attr);
        int map_dev = MPIDI_GPU_ipc_get_map_dev(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id,
                                                MPIDIG_REQUEST(rreq, datatype));
        mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_hdr->ipc_handle.gpu, map_dev, &src_buf, do_mmap);
        MPIR_ERR_CHECK(mpi_errno);

        /* copy */
        MPI_Aint src_count;
        MPI_Datatype src_dt;
        MPIR_Datatype *src_dt_ptr = NULL;
        if (ipc_hdr->is_contig) {
            src_count = src_data_sz;
            src_dt = MPI_BYTE;
        } else {
            /* TODO: get sender datatype and call MPIR_Typerep_op with mapped_device set to dev_id */
            void *flattened_type = ipc_hdr + 1;
            src_dt_ptr = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
            MPIR_Assert(src_dt_ptr);
            mpi_errno = MPIR_Typerep_unflatten(src_dt_ptr, flattened_type);
            MPIR_ERR_CHECK(mpi_errno);

            src_count = ipc_hdr->count;
            src_dt = src_dt_ptr->handle;
        }
        MPIR_gpu_req yreq;
        MPL_gpu_engine_type_t engine =
            MPIDI_IPCI_choose_engine(ipc_hdr->ipc_handle.gpu.global_dev_id, dev_id);
        mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, src_count, src_dt, 0, NULL,
                                        MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                                        MPIDIG_REQUEST(rreq, datatype), 0, &attr,
                                        MPL_GPU_COPY_DIRECTION_NONE, engine, true, &yreq);
        MPIR_ERR_CHECK(mpi_errno);
        if (src_dt_ptr) {
            MPIR_Datatype_free(src_dt_ptr);
        }

        mpi_errno = MPIDI_GPU_ipc_async_start(rreq, &yreq, src_buf, ipc_hdr->ipc_handle.gpu);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    } else if (ipc_hdr->ipc_type == MPIDI_IPCI_TYPE__NONE) {
        /* no-op */
    } else {
        MPIR_Assert(0);
    }

    IPC_TRACE("handle_lmt_recv: handle matched rreq %p [source %d, tag %d, "
              " context_id 0x%x], copy dst %p, bytes %ld\n", rreq,
              MPIDIG_REQUEST(rreq, u.recv.source), MPIDIG_REQUEST(rreq, u.recv.tag),
              MPIDIG_REQUEST(rreq, u.recv.context_id), (char *) MPIDIG_REQUEST(rreq, buffer),
              recv_data_sz);

  fn_cont:
    mpi_errno = MPIDI_IPC_complete(rreq, ipc_hdr->ipc_type);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* Need to send ack and complete the request so we don't block sender and receiver */
    if (!rreq->status.MPI_ERROR) {
        rreq->status.MPI_ERROR = mpi_errno;
    }
    goto fn_cont;
}

#endif /* IPC_P2P_H_INCLUDED */
