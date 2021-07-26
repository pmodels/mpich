/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

int MPIDI_UCX_dynamic_send(uint64_t remote_gpid, int tag, const void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

    return mpi_errno;
}

int MPIDI_UCX_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ucx_nm_notsupported");

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
    MPIR_CHKLMEM_DECL(2);

    MPIR_CHKLMEM_MALLOC(new_avt_procs, int *, sizeof(int) * size, mpi_errno, "new_avt_procs",
                        MPL_MEM_ADDRESS);
    MPIR_CHKLMEM_MALLOC(new_upids, char **, sizeof(char *) * size, mpi_errno, "new_upids",
                        MPL_MEM_ADDRESS);

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
            ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
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
