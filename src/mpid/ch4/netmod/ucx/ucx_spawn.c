/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "mpidu_bc.h"

static void dynamic_send_cb(void *request, ucs_status_t status, void *user_data)
{
    bool *done = user_data;
    *done = true;
}

static void dynamic_recv_cb(void *request, ucs_status_t status,
                            const ucp_tag_recv_info_t * info, void *user_data)
{
    bool *done = user_data;
    *done = true;
}

int MPIDI_UCX_dynamic_send(MPIR_Lpid remote_lpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    uint64_t ucx_tag = MPIDI_UCX_DYNPROC_MASK + tag;
    int vci = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(MPIDIU_lpid_to_av(remote_lpid), vci, vci);

    bool done = false;
    ucp_request_param_t param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA,
        .cb.send = dynamic_send_cb,
        .user_data = &done,
    };

    ucs_status_ptr_t status;
    status = ucp_tag_send_nbx(ep, buf, size, ucx_tag, &param);

    if (status == UCS_OK) {
        done = true;
    } else if (UCS_PTR_IS_ERR(status)) {
        mpi_errno = MPI_ERR_PORT;
        goto fn_exit;
    }

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    while (!done) {
        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
        if (timeout > 0 && time_gap > (double) timeout) {
            mpi_errno = MPI_ERR_PORT;
            goto fn_exit;
        }
        ucp_worker_progress(MPIDI_UCX_global.ctx[vci].worker);
    }

    if (status != UCS_OK) {
        ucp_request_release(status);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    return mpi_errno;
}

int MPIDI_UCX_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    uint64_t ucx_tag = MPIDI_UCX_DYNPROC_MASK + tag;
    uint64_t tag_mask = 0xffffffffffffffff;
    int vci = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    bool done = false;
    ucp_request_param_t param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA,
        .cb.recv = dynamic_recv_cb,
        .user_data = &done,
    };

    ucs_status_ptr_t status;
    status =
        ucp_tag_recv_nbx(MPIDI_UCX_global.ctx[vci].worker, buf, size, ucx_tag, tag_mask, &param);
    if (status == UCS_OK) {
        done = true;
    } else if (UCS_PTR_IS_ERR(status)) {
        mpi_errno = MPI_ERR_PORT;
        goto fn_exit;
    }

    MPL_time_t time_start, time_now;
    double time_gap;
    MPL_wtime(&time_start);
    while (!done) {
        MPL_wtime(&time_now);
        MPL_wtime_diff(&time_start, &time_now, &time_gap);
        if (timeout > 0 && time_gap > (double) timeout) {
            mpi_errno = MPI_ERR_PORT;
            goto fn_exit;
        }
        ucp_worker_progress(MPIDI_UCX_global.ctx[vci].worker);
    }

    if (status != UCS_OK) {
        ucp_request_release(status);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    return mpi_errno;
}

int MPIDI_UCX_dynamic_sendrecv(MPIR_Lpid remote_lpid, int tag,
                               const void *send_buf, int send_size, void *recv_buf, int recv_size)
{
    int mpi_errno = MPI_SUCCESS;

    /* NOTE: dynamic_sendrecv is always called inside CS of vci 0 */
    int vci = 0;
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[vci].lock));
#endif

    uint64_t ucx_tag = MPIDI_UCX_DYNPROC_MASK + tag;
    uint64_t tag_mask = 0xffffffffffffffff;     /* for recv */
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(remote_lpid);
    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(av, vci, vci);

    ucs_status_ptr_t status = UCS_OK;

    /* send */
    bool send_done = false;
    if (send_size > 0) {
        ucp_request_param_t send_param = {
            .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA,
            .cb.send = dynamic_send_cb,
            .user_data = &send_done,
        };

        status = ucp_tag_send_nbx(ep, send_buf, send_size, ucx_tag, &send_param);
        if (status == UCS_OK) {
            send_done = true;
        } else if (UCS_PTR_IS_ERR(status)) {
            /* FIXME: better error */
            mpi_errno = MPI_ERR_PORT;
            goto fn_exit;
        }
    } else {
        send_done = true;
    }

    /* recv */
    bool recv_done = false;
    if (recv_size > 0) {
        ucp_request_param_t recv_param = {
            .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA,
            .cb.recv = dynamic_recv_cb,
            .user_data = &recv_done,
        };

        status = ucp_tag_recv_nbx(MPIDI_UCX_global.ctx[vci].worker, recv_buf, recv_size,
                                  ucx_tag, tag_mask, &recv_param);
        if (status == UCS_OK) {
            recv_done = true;
        } else if (UCS_PTR_IS_ERR(status)) {
            /* FIXME: better error */
            mpi_errno = MPI_ERR_PORT;
            goto fn_exit;
        }
    } else {
        recv_done = true;
    }

    /* wait */
    while (!send_done || !recv_done) {
        ucp_worker_progress(MPIDI_UCX_global.ctx[vci].worker);
    }

  fn_exit:
    if (status != UCS_OK) {
        ucp_request_release(status);
    }
    return mpi_errno;
}

int MPIDI_UCX_get_local_upids(MPIR_Comm * comm, int **local_upid_size, char **local_upids)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_CHKPMEM_DECL(2);
    MPIR_CHKPMEM_MALLOC((*local_upid_size), int *, comm->local_size * sizeof(int),
                        mpi_errno, "local_upid_size", MPL_MEM_ADDRESS);
    MPIR_CHKPMEM_MALLOC((*local_upids), char *, comm->local_size * MPID_MAX_BC_SIZE,
                        mpi_errno, "temp_buf", MPL_MEM_BUFFER);

    int offset = 0;
    for (int i = 0; i < comm->local_size; i++) {
        MPIDI_upid_hash *t = MPIDIU_comm_rank_to_av(comm, i)->hash;
        (*local_upid_size)[i] = t->upid_len;
        memcpy((*local_upids) + offset, t->upid, t->upid_len);
        offset += t->upid_len;
    }

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_UCX_insert_upid(MPIR_Lpid lpid, const char *upid, int upid_len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(lpid);

    bool is_dynamic = (lpid & MPIR_LPID_DYNAMIC_MASK);
    bool do_insert = false;
    if (is_dynamic) {
        do_insert = true;
    } else if (!MPIDI_UCX_AV(av).dest[0][0]) {
        MPIDI_av_entry_t *dynamic_av = MPIDIU_find_dynamic_av(upid, upid_len);
        if (dynamic_av) {
            /* just copy it over */
            MPIDI_UCX_AV(av).dest[0][0] = MPIDI_UCX_AV(dynamic_av).dest[0][0];
        } else {
            do_insert = true;
        }
    }

    if (do_insert) {
        /* new entry */
        ucp_ep_params_t ep_params;
        ucs_status_t ucx_status;
        ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
        ep_params.address = (ucp_address_t *) upid;
        ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
                                   &MPIDI_UCX_AV(av).dest[0][0]);
        MPIDI_UCX_CHK_STATUS(ucx_status);
    }

    if (!is_dynamic) {
        MPIDIU_upidhash_add(upid, upid_len, lpid);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
