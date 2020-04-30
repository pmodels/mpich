/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_SEND_H_INCLUDED
#define GPU_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "gpu_pre.h"
#include "gpu_impl.h"

static inline int MPIDI_GPU_ipc_isend_recv_req(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank, int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    size_t data_sz;
    MPI_Aint true_lb;
    bool is_contig = true;
    void *buf_ptr = (void *) buf;
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_gpu_send_recv_req_t *send_req_hdr = &ctrl_hdr.gpu_recv_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GPU_IPC_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GPU_IPC_ISEND);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;
    MPIDIG_REQUEST(sreq, buffer) = NULL;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, rank) = rank;
    MPIDIG_REQUEST(sreq, count) = count;
    MPIDIG_REQUEST(sreq, context_id) = comm->context_id + context_offset;

    MPIDI_Datatype_check_contig_size_lb(datatype, count, is_contig, data_sz, true_lb);
    if (!is_contig) {
        mpi_errno = MPL_gpu_malloc(&MPIDIG_REQUEST(sreq, buffer), data_sz);
        MPIR_ERR_CHECK(mpi_errno);
        buf_ptr = MPIDIG_REQUEST(sreq, buffer);
        MPI_Aint actual_pack_bytes;
        MPIR_Typerep_pack(buf, count, datatype, 0, buf_ptr, data_sz, &actual_pack_bytes);
        MPIR_Assert(actual_pack_bytes == data_sz);
        true_lb = 0;
    }
    buf = (void const *) buf_ptr;

    /* GPU IPC internal info */
    send_req_hdr->data_sz = data_sz;
    send_req_hdr->sreq_ptr = (uint64_t) sreq;
    MPL_gpu_ipc_get_mem_handle(&send_req_hdr->mem_handle, (char *) buf + true_lb);

    /* message matching info */
    send_req_hdr->src_rank = comm->rank;
    send_req_hdr->tag = tag;
    send_req_hdr->context_id = comm->context_id + context_offset;

    mpi_errno = MPIDI_SHM_do_ctrl_send(rank, comm, MPIDI_SHM_GPU_SEND_IPC_RECV_REQ, &ctrl_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GPU_IPC_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_GPU_ipc_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 int ctrl_id, MPIR_Request ** request)
{
    if (ctrl_id == MPIDI_SHM_GPU_SEND_IPC_RECV_REQ)
        return MPIDI_GPU_ipc_isend_recv_req(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
    else {
        /* TODO: to be replaced by sender driven protocol */
        MPIR_Assert(0);
        return MPI_SUCCESS;
    }
}

#endif /* GPU_SEND_H_INCLUDED */
