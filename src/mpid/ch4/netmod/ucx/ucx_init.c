/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "mpidu_bc.h"
#include <ucp/api/ucp.h>

static void request_init_callback(void *request);

static void request_init_callback(void *request)
{

    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    ucp_request->req = NULL;

}

int MPIDI_UCX_mpi_init_hook(int rank, int size, int appnum, int *tag_bits, MPIR_Comm * init_comm)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_config_t *config;
    ucs_status_t ucx_status;
    uint64_t features = 0;
    int i;
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    ucp_ep_params_t ep_params;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MPI_INIT_HOOK);

    /* unable to support extended context id in current match bit configuration */
    MPL_COMPILE_TIME_ASSERT(MPIR_CONTEXT_ID_BITS <= MPIDI_UCX_CONTEXT_TAG_BITS);

    ucx_status = ucp_config_read(NULL, NULL, &config);
    MPIDI_UCX_CHK_STATUS(ucx_status);

    /* For now use only the tag feature */
    features = UCP_FEATURE_TAG | UCP_FEATURE_RMA;
    ucp_params.features = features;
    ucp_params.request_size = sizeof(MPIDI_UCX_ucp_request_t);
    ucp_params.request_init = request_init_callback;
    ucp_params.request_cleanup = NULL;
    ucp_params.estimated_num_eps = size;

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_ESTIMATED_NUM_EPS | UCP_PARAM_FIELD_REQUEST_INIT;

    ucx_status = ucp_init(&ucp_params, config, &MPIDI_UCX_global.context);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucp_config_release(config);

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    ucx_status = ucp_worker_create(MPIDI_UCX_global.context, &worker_params,
                                   &MPIDI_UCX_global.worker);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucx_status =
        ucp_worker_get_address(MPIDI_UCX_global.worker, &MPIDI_UCX_global.if_address,
                               &MPIDI_UCX_global.addrname_len);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    MPIR_Assert(MPIDI_UCX_global.addrname_len <= INT_MAX);

    void *table;
    int recv_bc_len;
    mpi_errno = MPIDU_bc_table_create(rank, size, MPIDI_global.node_map[0],
                                      MPIDI_UCX_global.if_address,
                                      (int) MPIDI_UCX_global.addrname_len, FALSE,
                                      MPIR_CVAR_CH4_ROOTS_ONLY_PMI, &table, &recv_bc_len);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        int *node_roots = MPIR_Process.node_root_map;
        int num_nodes = MPIR_Process.num_nodes;
        int *rank_map;

        for (i = 0; i < num_nodes; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, node_roots[i])).dest);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPIDU_bc_allgather(init_comm, MPIDI_UCX_global.if_address,
                           (int) MPIDI_UCX_global.addrname_len, FALSE, (void **) &table, &rank_map,
                           &recv_bc_len);

        /* insert new addresses, skipping over node roots */
        for (i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {

                ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
                ucx_status = ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                                           &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
                MPIDI_UCX_CHK_STATUS(ucx_status);
            }
        }
        MPIDU_bc_table_destroy();
    } else {
        for (i = 0; i < size; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPIDU_bc_table_destroy();
    }

    *tag_bits = MPIR_TAG_BITS_DEFAULT;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    goto fn_exit;

}

int MPIDI_UCX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, p = 0, completed;
    MPIR_Comm *comm;
    ucs_status_ptr_t ucp_request;
    ucs_status_ptr_t *pending;

    comm = MPIR_Process.comm_world;
    pending = MPL_malloc(sizeof(ucs_status_ptr_t) * comm->local_size, MPL_MEM_OTHER);

    for (i = 0; i < comm->local_size; i++) {
        ucp_request = ucp_disconnect_nb(MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
        MPIDI_UCX_CHK_REQUEST(ucp_request);
        if (ucp_request != UCS_OK) {
            pending[p] = ucp_request;
            p++;
        }
    }

    /* now complete the outstaning requests! Important: call progress inbetween, otherwise we
     * deadlock! */
    completed = p;
    while (completed != 0) {
        ucp_worker_progress(MPIDI_UCX_global.worker);
        completed = p;
        for (i = 0; i < p; i++) {
            if (ucp_request_is_completed(pending[i]) != 0)
                completed -= 1;
        }
    }

    for (i = 0; i < p; i++) {
        ucp_request_release(pending[i]);
    }

    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_UCX_global.worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

  fn_exit:
    MPL_free(pending);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

int MPIDI_UCX_post_init(void)
{
    return MPI_SUCCESS;
}

int MPIDI_UCX_get_vci_attr(int vci)
{
    MPIR_Assert(0 <= vci && vci < 1);
    return MPIDI_VCI_TX | MPIDI_VCI_RX;
}

int MPIDI_UCX_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_UCX_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                              int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_UCX_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    return MPI_SUCCESS;
}

int MPIDI_UCX_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

void *MPIDI_UCX_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}
