/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_RECV_H_INCLUDED
#define GPU_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "gpu_pre.h"
#include "gpu_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_GPU_handle_ipc_recv(size_t src_data_sz,
                                                       uint64_t sreq_ptr,
                                                       MPL_gpu_ipc_mem_handle_t mem_handle,
                                                       MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *src_buf = NULL;
    size_t data_sz, recv_data_sz;
    MPIDI_SHM_ctrl_hdr_t recv_ack_hdr;
    yaksa_request_t yaksa_request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GPU_HANDLE_IPC_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GPU_HANDLE_IPC_RECV);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    /* Do copy of data */
    MPL_gpu_ipc_open_mem_handle(&src_buf, mem_handle);

    MPI_Aint actual_unpack_bytes;
    mpi_errno =
        MPIR_Typerep_unpack((const void *) src_buf, src_data_sz,
                            (char *) MPIDIG_REQUEST(rreq, buffer), recv_data_sz,
                            MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_gpu_ipc_close_mem_handle(src_buf);

    /* Send acknowledgment to sender */
    recv_ack_hdr.gpu_recv_ack.req_ptr = sreq_ptr;
    mpi_errno = MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                                       MPIDIG_context_id_to_comm(MPIDIG_REQUEST
                                                                 (rreq, context_id)),
                                       MPIDI_SHM_GPU_SEND_IPC_RECV_ACK, &recv_ack_hdr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GPU_HANDLE_IPC_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* GPU_RECV_H_INCLUDED */
