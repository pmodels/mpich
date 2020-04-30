/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "gpu_pre.h"
#include "gpu_impl.h"
#include "gpu_recv.h"
#include "gpu_control.h"

int MPIDI_GPU_ctrl_send_ipc_recv_req_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_SHM_ctrl_gpu_send_recv_req_t *recv_req_hdr = &ctrl_hdr->gpu_recv_req;
    MPIR_Request *rreq = NULL;
    MPIR_Comm *root_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GPU_CTRL_SEND_IPC_RECV_REQ_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GPU_CTRL_SEND_IPC_RECV_REQ_CB);

    root_comm = MPIDIG_context_id_to_comm(recv_req_hdr->context_id);
    if (root_comm) {
        while (TRUE) {
            rreq =
                MPIDIG_dequeue_posted(recv_req_hdr->src_rank, recv_req_hdr->tag,
                                      recv_req_hdr->context_id, 1, &MPIDIG_COMM(root_comm,
                                                                                posted_list));
            if (rreq) {
                int is_cancelled;
                mpi_errno = MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
                MPIR_ERR_CHECK(mpi_errno);
                if (!is_cancelled) {
                    MPIR_Comm_release(root_comm);       /* -1 for posted_list */
                    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
                    continue;
                }
            }
            break;
        }
    }

    if (rreq) {
        /* Matching receive was posted */
        MPIR_Comm_release(root_comm);   /* -1 for posted list */
        MPIDIG_REQUEST(rreq, rank) = recv_req_hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = recv_req_hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = recv_req_hdr->context_id;

        /* Complete GPU IPC receive */
        mpi_errno =
            MPIDI_GPU_handle_ipc_recv(recv_req_hdr->data_sz, recv_req_hdr->sreq_ptr,
                                      recv_req_hdr->mem_handle, rreq);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* We got a send request: sender sent a GPU buffer memory handle. Handle
         * data transfer in the receiver. */
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

        /* store CH4 am rreq info */
        MPIDIG_REQUEST(rreq, buffer) = NULL;
        MPIDIG_REQUEST(rreq, datatype) = MPI_BYTE;
        MPIDIG_REQUEST(rreq, count) = recv_req_hdr->data_sz;
        MPIDIG_REQUEST(rreq, rank) = recv_req_hdr->src_rank;
        MPIDIG_REQUEST(rreq, tag) = recv_req_hdr->tag;
        MPIDIG_REQUEST(rreq, context_id) = recv_req_hdr->context_id;
        MPIDI_REQUEST(rreq, is_local) = 1;

        /* store GPU IPC internal info */
        MPIDI_GPU_IPC_REQUEST(rreq, unexp_rreq).data_sz = recv_req_hdr->data_sz;
        MPIDI_GPU_IPC_REQUEST(rreq, unexp_rreq).sreq_ptr = recv_req_hdr->sreq_ptr;
        MPIDI_GPU_IPC_REQUEST(rreq, unexp_rreq).mem_handle = recv_req_hdr->mem_handle;
        MPIDI_SHM_REQUEST(rreq, status) |= MPIDI_SHM_REQ_GPU_IPC_RECV;

        if (root_comm) {
            MPIR_Comm_add_ref(root_comm);
            MPIDIG_enqueue_unexp(rreq, &MPIDIG_COMM(root_comm, unexp_list));
        } else {
            MPIDIG_enqueue_unexp(rreq,
                                 MPIDIG_context_id_to_uelist(MPIDIG_REQUEST(rreq, context_id)));
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GPU_CTRL_SEND_IPC_RECV_REQ_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_GPU_ctrl_send_ipc_recv_ack_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = (MPIR_Request *) ctrl_hdr->gpu_recv_ack.req_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_GPU_CTRL_SEND_IPC_RECV_ACK_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_GPU_CTRL_SEND_IPC_RECV_ACK_CB);

    if (MPIDIG_REQUEST(rreq, buffer))
        MPL_gpu_free(MPIDIG_REQUEST(rreq, buffer));

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_GPU_CTRL_SEND_IPC_RECV_ACK_CB);
    return mpi_errno;
}
