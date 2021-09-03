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
#include "ipc_mem.h"

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

int MPIDI_IPC_rndv_cb(MPIR_Request * rreq);
int MPIDI_IPC_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                uint32_t attr, MPIR_Request ** req);

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_send_contig_lmt(const void *buf, MPI_Aint count,
                                                        MPI_Datatype datatype, uintptr_t data_sz,
                                                        int rank, int tag, MPIR_Comm * comm,
                                                        int context_offset, MPIDI_av_entry_t * addr,
                                                        MPIDI_IPCI_ipc_attr_t ipc_attr,
                                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIDI_IPC_ctrl_send_contig_lmt_rts_t slmt_req_hdr;

    /* FIXME: handle all send types */
    int flags = 0;
    int error_bits = 0;

    MPIR_FUNC_ENTER;

    /* Create send request */
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2, 0, 0);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, rank) = rank;
    MPIDIG_REQUEST(sreq, count) = count;
    MPIDIG_REQUEST(sreq, context_id) = comm->context_id + context_offset;

    slmt_req_hdr.ipc_hdr.ipc_type = ipc_attr.ipc_type;
    slmt_req_hdr.ipc_hdr.ipc_handle = ipc_attr.ipc_handle;
    slmt_req_hdr.ipc_hdr.src_lrank = MPIR_Process.local_rank;

    /* message matching info */
    slmt_req_hdr.hdr.src_rank = comm->rank;
    slmt_req_hdr.hdr.tag = tag;
    slmt_req_hdr.hdr.context_id = comm->context_id + context_offset;
    slmt_req_hdr.hdr.data_sz = data_sz;
    slmt_req_hdr.hdr.rndv_hdr_sz = sizeof(MPIDI_IPC_hdr);
    slmt_req_hdr.hdr.sreq_ptr = sreq;
    slmt_req_hdr.hdr.error_bits = error_bits;
    slmt_req_hdr.hdr.flags = flags;
    MPIDIG_AM_SEND_SET_RNDV(slmt_req_hdr.hdr.flags, MPIDIG_RNDV_IPC);

    if (flags & MPIDIG_AM_SEND_FLAGS_SYNC) {
        MPIR_cc_inc(sreq->cc_ptr);      /* expecting SSEND_ACK */
    }

    if (ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_ipc_handle_cache_insert(rank, comm, ipc_attr.ipc_handle.gpu);
        MPIR_ERR_CHECK(mpi_errno);
    }

    int is_local = 1;
    MPI_Aint hdr_sz = sizeof(slmt_req_hdr);
    CH4_CALL(am_send_hdr(rank, comm, MPIDIG_SEND, &slmt_req_hdr, hdr_sz, 0, 0),
             is_local, mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
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
MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_handle_lmt_recv(MPIDI_IPCI_type_t ipc_type,
                                                        MPIDI_IPCI_ipc_handle_t ipc_handle,
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
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    /* attach remote buffer */
    switch (ipc_type) {
        case MPIDI_IPCI_TYPE__XPMEM:
            mpi_errno = MPIDI_XPMEM_ipc_handle_map(ipc_handle.xpmem, &src_buf);
            break;
        case MPIDI_IPCI_TYPE__GPU:
            {
                MPL_pointer_attr_t attr;
                MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(rreq, buffer), &attr);
                int dev_id = MPL_gpu_get_dev_id_from_attr(&attr);
                mpi_errno = MPIDI_GPU_ipc_handle_map(ipc_handle.gpu, dev_id,
                                                     MPIDIG_REQUEST(rreq, datatype), &src_buf);
            }
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
    mpi_errno = MPIR_Typerep_unpack((const void *) src_buf, src_data_sz,
                                    (char *) MPIDIG_REQUEST(rreq, buffer), recv_data_sz,
                                    MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_IPCI_handle_unmap(ipc_type, src_buf, ipc_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_IPC_ctrl_send_contig_lmt_fin_t am_hdr;
    am_hdr.ipc_type = ipc_type;
    am_hdr.req_ptr = sreq_ptr;

    CH4_CALL(am_send_hdr(MPIDIG_REQUEST(rreq, rank), rreq->comm, MPIDI_IPC_ACK,
                         &am_hdr, sizeof(am_hdr), 0, 0), 1, mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPC_P2P_H_INCLUDED */
