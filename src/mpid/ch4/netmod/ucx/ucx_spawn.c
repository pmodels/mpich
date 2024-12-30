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

int MPIDI_UCX_dynamic_send(uint64_t remote_gpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    uint64_t ucx_tag = MPIDI_UCX_DYNPROC_MASK + tag;
    int vci = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    int avtid = MPIDIU_GPID_GET_AVTID(remote_gpid);
    int lpid = MPIDIU_GPID_GET_LPID(remote_gpid);
    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(&MPIDIU_get_av(avtid, lpid), vci, vci);

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

int MPIDI_UCX_upids_to_gpids(int size, int *remote_upid_size, char *remote_upids,
                             uint64_t * remote_gpids)
{
    int mpi_errno = MPI_SUCCESS;

    int n_new_procs = 0;
    int *new_avt_procs;
    char **new_upids;
    int vci = 0;
    MPIR_CHKLMEM_DECL();

    MPIR_CHKLMEM_MALLOC(new_avt_procs, sizeof(int) * size);
    MPIR_CHKLMEM_MALLOC(new_upids, sizeof(char *) * size);

    char *curr_upid = remote_upids;
    for (int i = 0; i < size; i++) {
        MPIDI_upid_hash *t = MPIDIU_upidhash_find(curr_upid, remote_upid_size[i]);
        if (t) {
            remote_gpids[i] = MPIDIU_GPID_CREATE(t->avtid, t->lpid);
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

            remote_gpids[new_avt_procs[i]] = MPIDIU_GPID_CREATE(avtid, i);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
