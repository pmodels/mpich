/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "rndv.h"
#include "shm_control.h"

/* Set up the CTS message to go to the sender of the LMT. It should include a handle for a shared
 * memory buffer.
 * req - Request representing the large message transfer operation
 */
int MPIDI_POSIX_lmt_recv(MPIR_Request * req)
{
    char *serialized_handle;
    int mpi_errno;
    int dt_contig ATTRIBUTE((unused));
    MPIR_Datatype *dt_ptr;
    MPI_Aint data_sz, dt_true_lb ATTRIBUTE((unused));

    MPIR_CHKPMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_LMT_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_LMT_RECV);

    MPIDI_Datatype_get_info(MPIDIG_REQUEST(req, user_count), MPIDIG_REQUEST(req, datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (data_sz < MPIDIG_REQUEST(req, count)) {
        MPIR_ERR_SET2(req->status.MPI_ERROR, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d",
                      MPIDIG_REQUEST(req, count), data_sz);
        MPIDIG_REQUEST(req, count) = data_sz;
    }

    /* Calculate local rank of the sending process. */
    const int grank = MPIDIU_rank_to_lpid(MPIDIG_REQUEST(req, rank),
                                          MPIDIG_context_id_to_comm(MPIDIG_REQUEST(req,
                                                                                   context_id)));
    int sender_local_rank = MPIDI_POSIX_global.local_ranks[grank];

    /* TODO - Add a second array of shared memory buffers so there can be one ongoing message in
     * both directions. */
    /* Check to see if there is already a buffer allocated and if not, create one and store it in
     * the local array to track them. */
    if (NULL == rndv_recv_copy_bufs[sender_local_rank]) {
        int i;

        mpi_errno = MPL_shm_seg_create_and_attach(rndv_recv_copy_buf_handles[sender_local_rank],
                                                  sizeof(lmt_rndv_copy_buf_t),
                                                  (void **) &rndv_recv_copy_bufs[sender_local_rank],
                                                  0);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        rndv_recv_copy_bufs[sender_local_rank]->sender_present.val = 0;
        rndv_recv_copy_bufs[sender_local_rank]->receiver_present.val = 0;

        for (i = 0; i < MPIDI_LMT_NUM_BUFS; i++) {
            rndv_recv_copy_bufs[sender_local_rank]->len[i].val = 0;
        }
    }
    /* Get a serialized version of the buffer handle */
    mpi_errno = MPL_shm_hnd_get_serialized_by_ref(rndv_recv_copy_buf_handles[sender_local_rank],
                                                  &serialized_handle);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Send CTS message to the sender */
    MPIDI_SHM_ctrl_hdr_t msg;
    msg.lmt_rndv_long_ack.sreq_ptr = (uint64_t) MPIDIG_REQUEST(req, req->rreq.peer_req_ptr);
    msg.lmt_rndv_long_ack.rreq_ptr = (uint64_t) req;
    if (MPIDIG_REQUEST(req, req->status) & MPIDIG_REQ_PEER_SSEND)
        msg.lmt_rndv_long_ack.handler_id = MPIDIG_SSEND_REQ;
    else
        msg.lmt_rndv_long_ack.handler_id = 0;
    MPL_strncpy(msg.lmt_rndv_long_ack.copy_buf_serialized_handle, serialized_handle,
                MPIDI_MAX_HANDLE_LEN);
    msg.lmt_rndv_long_ack.copy_buf_serialized_handle_len = (int) strlen(serialized_handle) + 1;
    MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(req, rank),
                           MPIDIG_context_id_to_comm(MPIDIG_REQUEST(req, context_id)),
                           MPIDI_SHM_SEND_LMT_ACK, &msg);

    /* Create a new "wait element" for this lmt transfer in case more than one is started at the
     * same time. */
    lmt_rndv_wait_element_t *element;
    MPIR_CHKPMEM_MALLOC(element, lmt_rndv_wait_element_t *, sizeof(lmt_rndv_wait_element_t),
                        mpi_errno, "LMT RNDV wait element", MPL_MEM_SHM);
    element->progress = lmt_rndv_recv_progress;
    element->req = req;
    GENERIC_Q_ENQUEUE(&rndv_recv_queues[sender_local_rank], element, next);

    /* TODO - Create a queue where in progress messages go to shorten the progress polling time. */

    /* Try to make progress on lmt messages before queuing the new one. */
    mpi_errno = MPIDI_POSIX_lmt_progress();
    /* Want to capture the output of the progress function, but not return an error immediately
     * because just kicking the progress engine may not indicate a problem with this particular
     * receive. */

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_LMT_RECV);
    return MPI_SUCCESS;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

void MPIDI_POSIX_lmt_recv_posted_hook(int grank)
{
    return;
}

void MPIDI_POSIX_lmt_recv_completed_hook(int grank)
{
    return;
}
