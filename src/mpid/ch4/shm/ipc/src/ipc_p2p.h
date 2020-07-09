/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_P2P_H_INCLUDED
#define IPC_P2P_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "shm_control.h"
#include "ipc_pre.h"
#include "ipc_types.h"
#include "ipc_mem.h"
#include "../posix/posix_am.h"

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
MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_send_lmt(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, uintptr_t data_sz,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t * addr,
                                                 MPIDI_IPCI_ipc_attr_t ipc_attr,
                                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIDI_SHMI_ctrl_hdr_t *ctrl_hdr;
    MPIDI_IPC_ctrl_send_lmt_rts_t *slmt_req_hdr;
    int flattened_type_size, ctrl_hdr_size, dt_contig;
    void *flattened_type_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_SEND_LMT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_SEND_LMT);

    MPIDI_Datatype_check_contig(datatype, dt_contig);

    /* Allocate full memory for control header */
    if (!MPIR_DATATYPE_IS_PREDEFINED(datatype) && !dt_contig) {
        MPIR_Datatype_get_flattened(datatype, &flattened_type_ptr, &flattened_type_size);
    } else {
        flattened_type_size = 0;
    }

    ctrl_hdr_size = sizeof(MPIDI_SHMI_ctrl_hdr_t) + flattened_type_size;
    ctrl_hdr = (MPIDI_SHMI_ctrl_hdr_t *) MPL_malloc(ctrl_hdr_size, MPL_MEM_OTHER);
    MPIR_Assert(ctrl_hdr);

    slmt_req_hdr = &ctrl_hdr->ipc_slmt_rts;
    slmt_req_hdr->flattened_type_size = flattened_type_size;
    if (flattened_type_size)
        memcpy(slmt_req_hdr->flattened_type, flattened_type_ptr, flattened_type_size);

    /* Create send request */
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;
    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, rank) = rank;
    MPIDIG_REQUEST(sreq, count) = count;
    MPIDIG_REQUEST(sreq, context_id) = comm->context_id + context_offset;

    slmt_req_hdr->src_lrank = MPIR_Process.local_rank;
    slmt_req_hdr->data_sz = data_sz;
    slmt_req_hdr->sreq_ptr = sreq;
    slmt_req_hdr->ipc_type = ipc_attr.ipc_type;
    slmt_req_hdr->ipc_handle = ipc_attr.ipc_handle;

    /* message matching info */
    slmt_req_hdr->src_rank = comm->rank;
    slmt_req_hdr->tag = tag;
    slmt_req_hdr->context_id = comm->context_id + context_offset;

    if (ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_ipc_handle_cache_insert(rank, comm, ipc_attr.ipc_handle.gpu);
        MPIR_ERR_CHECK(mpi_errno);
    }

    IPC_TRACE("send_contig_lmt: shm ctrl_id %d, data_sz 0x%" PRIu64 ", sreq_ptr 0x%p, "
              "src_lrank %d, match info[dest %d, src_rank %d, tag %d, context_id 0x%x]\n",
              MPIDI_IPC_SEND_LMT_RTS, slmt_req_hdr->data_sz, slmt_req_hdr->sreq_ptr,
              slmt_req_hdr->src_lrank, rank, slmt_req_hdr->src_rank, slmt_req_hdr->tag,
              slmt_req_hdr->context_id);

    mpi_errno =
        MPIDI_SHM_do_ctrl_send(rank, comm, MPIDIG_IPC_DATATYPE_REQ, ctrl_hdr_size, &ctrl_hdr, sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPL_free(ctrl_hdr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_SEND_LMT);
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
MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_handle_lmt_recv(MPIDI_IPCI_type_t ipc_type,
                                                        MPIDI_IPCI_ipc_handle_t ipc_handle,
                                                        size_t src_data_sz,
                                                        MPIR_Request * sreq_ptr,
                                                        void *flattened_type, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *src_buf = NULL, *copy_src_buf;
    uintptr_t data_sz, recv_data_sz;
    MPIDI_SHMI_ctrl_hdr_t ack_ctrl_hdr;
    MPI_Datatype src_datatype;
    MPIR_Datatype *src_datatype_ptr;
    int src_dt_contig, dest_dt_contig, src_true_lb, dest_true_lb;
    uintptr_t src_dt_size, src_count;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_HANDLE_LMT_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_HANDLE_LMT_RECV);

    MPIDI_Datatype_check_contig_size_lb(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count),
                                        dest_dt_contig, data_sz, dest_true_lb);

    /* recover src datatype */
    if (flattened_type) {
        src_datatype_ptr = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        MPIR_Assert(src_datatype_ptr);

        MPIR_Object_set_ref(src_datatype_ptr, 1);
        MPIR_Typerep_unflatten(src_datatype_ptr, flattened_type);
        src_datatype = src_datatype_ptr->handle;
        MPIDI_Datatype_check_contig_size_lb(src_datatype, 1, src_dt_contig, src_dt_size,
                                            src_true_lb);
        src_count = src_data_sz / src_dt_size;
    } else {
        src_datatype_ptr = NULL;
        src_datatype = MPI_BYTE;
        src_dt_contig = 1;
        src_count = src_data_sz;
        src_true_lb = 0;
    }

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(rreq, buffer), &attr);

    /* attach remote buffer */
    switch (ipc_type) {
        case MPIDI_IPCI_TYPE__XPMEM:
            mpi_errno = MPIDI_XPMEM_ipc_handle_map(ipc_handle.xpmem, &src_buf);
            break;
        case MPIDI_IPCI_TYPE__GPU:
            mpi_errno =
                MPIDI_GPU_ipc_handle_map(ipc_handle.gpu, attr.device,
                                         MPIDIG_REQUEST(rreq, datatype), &src_buf);
            break;
        case MPIDI_IPCI_TYPE__NONE:
            /* no-op */
            break;
        default:
            /* Unknown IPC type */
            MPIR_Assert(0);
            break;
    }

    IPC_TRACE("handle_lmt_recv: handle matched rreq %p [source %d, tag %d, "
              " context_id 0x%x], copy dst %p, bytes %ld\n", rreq,
              MPIDIG_REQUEST(rreq, rank), MPIDIG_REQUEST(rreq, tag),
              MPIDIG_REQUEST(rreq, context_id), (char *) MPIDIG_REQUEST(rreq, buffer),
              recv_data_sz);

    /* Copy data to receive buffer */
    MPI_Aint actual_unpack_bytes;
    MPI_Aint actual_pack_bytes;

    copy_src_buf = (void *) ((uintptr_t) src_buf - src_true_lb);
    if (!src_dt_contig && dest_dt_contig) {
        /* source datatype is non-contiguous and destination datatype is contiguous */
        mpi_errno = MPIR_Typerep_pack(copy_src_buf, src_count, src_datatype,
                                      0,
                                      (void *) ((char *) MPIDIG_REQUEST(rreq, buffer) +
                                                dest_true_lb), recv_data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes <= recv_data_sz);
    } else if (src_dt_contig) {
        /* source datatype is contiguous */
        mpi_errno =
            MPIR_Typerep_unpack(((char *) copy_src_buf + src_true_lb),
                                src_data_sz, MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq,
                                                                                          count),
                                MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_unpack_bytes <= recv_data_sz);
    } else {
        /* both datatype are non-contiguous */
        void *tmp_buf;
        bool host_buf;
        int mpl_err;

        if (ipc_type == MPIDI_IPCI_TYPE__XPMEM || attr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
            attr.type == MPL_GPU_POINTER_REGISTERED_HOST) {
            tmp_buf = MPL_malloc(recv_data_sz, MPL_MEM_OTHER);
            host_buf = true;
        } else {
            /* both src and dest buffer are on GPU, so tmp buffer should be allocated on GPU as well */
            mpl_err = MPL_gpu_malloc(&tmp_buf, recv_data_sz, attr.device);
            MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**nomem");
            host_buf = false;
        }

        mpi_errno = MPIR_Typerep_pack(copy_src_buf, src_count, src_datatype,
                                      0, tmp_buf, recv_data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes <= recv_data_sz);

        mpi_errno = MPIR_Typerep_unpack(tmp_buf, actual_pack_bytes,
                                        MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                                        MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_unpack_bytes <= recv_data_sz);

        if (host_buf) {
            MPL_free(tmp_buf);
        } else {
            MPL_gpu_free(tmp_buf);
        }
    }

    mpi_errno = MPIDI_IPCI_handle_unmap(ipc_type, src_buf, ipc_handle);
    MPIR_ERR_CHECK(mpi_errno);

    ack_ctrl_hdr.ipc_slmt_fin.ipc_type = ipc_type;
    ack_ctrl_hdr.ipc_slmt_fin.req_ptr = sreq_ptr;
    mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                                       MPIDIG_context_id_to_comm(MPIDIG_REQUEST
                                                                 (rreq, context_id)),
                                       MPIDI_IPC_SEND_LMT_FIN, &ack_ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

    if (src_datatype_ptr)
        MPIR_Datatype_ptr_release(src_datatype_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_HANDLE_LMT_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPC_P2P_H_INCLUDED */
