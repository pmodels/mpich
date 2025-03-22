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
    ucp_request_release(request);
}

static void dynamic_recv_cb(void *request, ucs_status_t status,
                            const ucp_tag_recv_info_t * info, void *user_data)
{
    bool *done = user_data;
    *done = true;
    /* request always released in MPIDI_UCX_dynamic_recv due to its blocking design */
}

int MPIDI_UCX_dynamic_send(MPIR_Lpid remote_lpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    uint64_t ucx_tag = MPIDI_UCX_DYNPROC_MASK + tag;
    int vci = 0;
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(vci));
#endif

    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(MPIDIU_lpid_to_av_slow(remote_lpid), vci, vci);

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
    return mpi_errno;
}

int MPIDI_UCX_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    uint64_t ucx_tag = MPIDI_UCX_DYNPROC_MASK + tag;
    uint64_t tag_mask = 0xffffffffffffffff;
    int vci = 0;
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(vci));
#endif

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
    return mpi_errno;
}

int MPIDI_UCX_dynamic_sendrecv(MPIR_Lpid remote_lpid, int tag,
                               const void *send_buf, int send_size, void *recv_buf, int recv_size,
                               int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    /* NOTE: dynamic_sendrecv is always called inside CS of vci 0 */
    int vci = 0;
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(vci));
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
    MPL_time_t time_start;
    MPL_wtime(&time_start);
    while (!send_done || !recv_done) {
        ucp_worker_progress(MPIDI_UCX_global.ctx[vci].worker);

        if (timeout > 0) {
            MPL_time_t time_now;
            double time_gap;
            MPL_wtime(&time_now);
            MPL_wtime_diff(&time_start, &time_now, &time_gap);
            if (time_gap > (double) timeout) {
                mpi_errno = MPIX_ERR_TIMEOUT;
                break;
            }
        }
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

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    MPIR_CHKPMEM_DECL();
    MPIR_CHKPMEM_MALLOC((*local_upid_size), comm->local_size * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_CHKPMEM_MALLOC((*local_upids), comm->local_size * MPID_MAX_BC_SIZE, MPL_MEM_BUFFER);

    int offset = 0;
    for (int i = 0; i < comm->local_size; i++) {
        MPIDI_upid_hash *t = MPIDIU_comm_rank_to_av(comm, i)->hash;
        (*local_upid_size)[i] = t->upid_len;
        memcpy((*local_upids) + offset, t->upid, t->upid_len);
        offset += t->upid_len;
    }

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

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
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
        int avtid = MPIR_LPID_WORLD_INDEX(lpid);
        int wrank = MPIR_LPID_WORLD_RANK(lpid);
        MPIDIU_upidhash_add(upid, upid_len, avtid, wrank);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_upids_to_lpids(int size, int *remote_upid_size, char *remote_upids,
                             MPIR_Lpid * remote_lpids)
{
    int mpi_errno = MPI_SUCCESS;

    int n_new_procs = 0;
    int *new_avt_procs;
    char **new_upids;
    int vci = 0;
    MPIR_CHKLMEM_DECL();

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    MPIR_CHKLMEM_MALLOC(new_avt_procs, sizeof(int) * size);
    MPIR_CHKLMEM_MALLOC(new_upids, sizeof(char *) * size);

    char *curr_upid = remote_upids;
    for (int i = 0; i < size; i++) {
        MPIDI_upid_hash *t = MPIDIU_upidhash_find(curr_upid, remote_upid_size[i]);
        if (t) {
            remote_lpids[i] = MPIDIU_GPID_CREATE(t->avtid, t->lpid);
        } else {
            new_avt_procs[n_new_procs] = i;
            new_upids[n_new_procs] = curr_upid;
            n_new_procs++;

        }
        curr_upid += remote_upid_size[i];
    }

    /* create new av_table, insert processes */
    if (n_new_procs > 0) {
        int avtid;
        mpi_errno = MPIDIU_new_avt(n_new_procs, &avtid);
        MPIR_ERR_CHECK(mpi_errno);

        for (int i = 0; i < n_new_procs; i++) {
            ucp_ep_params_t ep_params;
            ucs_status_t ucx_status;
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) new_upids[i];
            ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[vci].worker, &ep_params,
                                       &MPIDI_UCX_AV(&MPIDIU_get_av(avtid, i)).dest[0][0]);
            MPIDI_UCX_CHK_STATUS(ucx_status);
            MPIDIU_upidhash_add(new_upids[i], remote_upid_size[new_avt_procs[i]], avtid, i);

            remote_lpids[new_avt_procs[i]] = MPIDIU_GPID_CREATE(avtid, i);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
