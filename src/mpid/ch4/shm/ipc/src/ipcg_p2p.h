/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPCG_P2P_H_INCLUDED
#define IPCG_P2P_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "shm_control.h"
#include "ipc_pre.h"
#include "ipc_mem.h"

/* Generic IPC protocols for P2P. */

/* Generic sender-initialized LMT routine with contig send buffer.
 *
 * If the send buffer is noncontiguous the submodule can first pack the
 * data into a temporary buffer and use the temporary buffer as the send
 * buffer with this call. The sender gets the memory handle of the specified
 * buffer, and sends to the receiver. The receiver will then open the remote
 * memory handle and perform direct data transfer.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_IPCG_send_contig_lmt(const void *buf, MPI_Aint count,
                                                        MPI_Datatype datatype, int rank, int tag,
                                                        MPIR_Comm * comm, int context_offset,
                                                        MPIDI_av_entry_t * addr,
                                                        MPIDI_SHM_IPC_type_t ipc_type,
                                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    MPI_Aint true_lb;
    bool is_contig ATTRIBUTE((unused)) = 0;
    MPIR_Request *sreq = NULL;
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_ipc_send_lmt_rts_t *slmt_req_hdr = &ctrl_hdr.ipc_slmt_rts;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCG_SEND_CONTIG_LMT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCG_SEND_CONTIG_LMT);

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

    MPIDI_Datatype_check_contig_size_lb(datatype, count, is_contig, data_sz, true_lb);
    MPIR_Assert(is_contig && data_sz > 0);

    slmt_req_hdr->src_lrank = MPIR_Process.local_rank;
    slmt_req_hdr->data_sz = data_sz;
    slmt_req_hdr->sreq_ptr = (uint64_t) sreq;
    slmt_req_hdr->ipc_type = ipc_type;

    mpi_errno = MPIDI_IPC_get_mem_handle(ipc_type, (char *) buf + true_lb, data_sz,
                                         &slmt_req_hdr->mem_handle);
    MPIR_ERR_CHECK(mpi_errno);

    /* message matching info */
    slmt_req_hdr->src_rank = comm->rank;
    slmt_req_hdr->tag = tag;
    slmt_req_hdr->context_id = comm->context_id + context_offset;

    IPC_TRACE("send_contig_lmt: shm ctrl_id %d, data_sz 0x%lx, sreq_ptr 0x%lx, "
              "src_lrank %d, match info[dest %d, src_rank %d, tag %d, context_id 0x%x]\n",
              MPIDI_SHM_IPC_SEND_CONTIG_LMT_RTS, slmt_req_hdr->data_sz, slmt_req_hdr->sreq_ptr,
              slmt_req_hdr->src_lrank, rank, slmt_req_hdr->src_rank, slmt_req_hdr->tag,
              slmt_req_hdr->context_id);

    mpi_errno = MPIDI_SHM_do_ctrl_send(rank, comm, MPIDI_SHM_IPC_SEND_CONTIG_LMT_RTS, &ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCG_SEND_CONTIG_LMT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCG_handle_lmt_single_recv(MPIDI_SHM_IPC_type_t ipc_type,
                                                               MPIDI_IPC_mem_handle_t mem_handle,
                                                               size_t src_data_sz,
                                                               uint64_t sreq_ptr,
                                                               int src_lrank, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_IPC_mem_seg_t mem_seg;
    void *src_buf = NULL;
    size_t data_sz, recv_data_sz;
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCG_HANDLE_SEND_LMT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCG_HANDLE_SEND_LMT);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);
    memset(&mem_seg, 0, sizeof(mem_seg));

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    mpi_errno = MPIDI_IPC_attach_mem(ipc_type, src_lrank, mem_handle, src_data_sz,
                                     &mem_seg, &src_buf);
    MPIR_ERR_CHECK(mpi_errno);

    IPC_TRACE("handle_lmt_single_recv: handle matched rreq %p [source %d, tag %d, "
              " context_id 0x%x], copy dst %p, src %lx, bytes %ld\n", rreq,
              MPIDIG_REQUEST(rreq, rank), MPIDIG_REQUEST(rreq, tag),
              MPIDIG_REQUEST(rreq, context_id), (char *) MPIDIG_REQUEST(rreq, buffer),
              mem_handle.xpmem.src_offset, recv_data_sz);

    /* Copy data to receive buffer */
    MPI_Aint actual_unpack_bytes;
    mpi_errno =
        MPIR_Typerep_unpack((const void *) src_buf, src_data_sz,
                            (char *) MPIDIG_REQUEST(rreq, buffer), recv_data_sz,
                            MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_IPC_close_mem(mem_seg, src_buf);
    MPIR_ERR_CHECK(mpi_errno);

    ack_ctrl_hdr.ipc_slmt_fin.ipc_type = ipc_type;
    ack_ctrl_hdr.ipc_slmt_fin.req_ptr = sreq_ptr;
    mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                                       MPIDIG_context_id_to_comm(MPIDIG_REQUEST
                                                                 (rreq, context_id)),
                                       MPIDI_SHM_IPC_SEND_CONTIG_LMT_FIN, &ack_ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCG_HANDLE_SEND_LMT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPCG_P2P_H_INCLUDED */
