/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_INIT_H_INCLUDED
#define UCX_INIT_H_INCLUDED

#include "ucx_impl.h"
#include "mpir_cvars.h"
#include "ucx_types.h"
#include "mpidu_bc.h"
#include <ucp/api/ucp.h>
#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_init_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_init_hook(int rank,
                                         int size,
                                         int appnum,
                                         int *tag_bits,
                                         MPIR_Comm * comm_world,
                                         MPIR_Comm * comm_self, int spawned, int *n_vnis_provided)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_config_t *config;
    ucs_status_t ucx_status;
    uint64_t features = 0;
    int i;
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    ucp_ep_params_t ep_params;
    size_t *bc_indices;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT);

    *n_vnis_provided = 1;

    ucx_status = ucp_config_read(NULL, NULL, &config);
    MPIDI_UCX_CHK_STATUS(ucx_status);

    /* For now use only the tag feature */
    features = UCP_FEATURE_TAG | UCP_FEATURE_RMA;
    ucp_params.features = features;
    ucp_params.request_size = sizeof(MPIDI_UCX_ucp_request_t);
    ucp_params.request_init = MPIDI_UCX_Request_init_callback;
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

    MPIDU_bc_table_create(rank, size, MPIDI_CH4_Global.node_map[0], MPIDI_UCX_global.if_address,
                          (int) MPIDI_UCX_global.addrname_len, FALSE, MPIR_CVAR_CH4_ROOTS_ONLY_PMI,
                          (void **) &MPIDI_UCX_global.pmi_addr_table, &bc_indices);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        int *node_roots, num_nodes;

        MPIR_NODEMAP_get_node_roots(MPIDI_CH4_Global.node_map[0], size, &node_roots, &num_nodes);
        for (i = 0; i < num_nodes; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) & MPIDI_UCX_global.pmi_addr_table[bc_indices[i]];
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, node_roots[i])).dest);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPL_free(node_roots);
    } else {
        for (i = 0; i < size; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) & MPIDI_UCX_global.pmi_addr_table[bc_indices[i]];
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPIDU_bc_table_destroy(MPIDI_UCX_global.pmi_addr_table);
    }

    MPIDIG_init(comm_world, comm_self, *n_vnis_provided);

#ifndef HAVE_DEBUGGER_SUPPORT
    MPIDI_UCX_global.lw_send_req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    if (MPIDI_UCX_global.lw_send_req == NULL) {
        MPIR_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }
    MPIR_cc_set(&MPIDI_UCX_global.lw_send_req->cc, 0);
#endif

    *tag_bits = MPIR_TAG_BITS_DEFAULT;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_EXIT);
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    goto fn_exit;

}

static inline int MPIDI_NM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
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

#ifdef USE_PMIX_API
    pmix_value_t value;
    pmix_info_t *info;
    int flag = 1;

    PMIX_INFO_CREATE(info, 1);
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);
    pmi_errno = PMIx_Fence(&MPIR_Process.pmix_wcproc, 1, info, 1);
    if (pmi_errno != PMIX_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmix_fence", "**pmix_fence %d",
                             pmi_errno);
    }
    PMIX_INFO_FREE(info, 1);
#elif defined(USE_PMI2_API)
    pmi_errno = PMI2_KVS_Fence();
    if (pmi_errno != PMI2_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**pmi_kvsfence");
    }
#else
    pmi_errno = PMI_Barrier();
    MPIDI_UCX_PMI_ERROR(pmi_errno);
#endif


    if (MPIDI_UCX_global.worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    MPIR_Comm_release_always(comm);

    comm = MPIR_Process.comm_self;
    MPIR_Comm_release_always(comm);

    MPIDIG_finalize();

#ifndef HAVE_DEBUGGER_SUPPORT
    MPIR_Request_free(MPIDI_UCX_global.lw_send_req);
#endif

  fn_exit:
    MPL_free(pending);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_get_vni_attr(int vni)
{
    MPIR_Assert(0 <= vni && vni < 1);
    return MPIDI_VNI_TX | MPIDI_VNI_RX;
}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    } else if (is_remote) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    } else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LUPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;

}

static inline int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                           char **local_upids)
{
    MPIR_Assert(0);
}

static inline int MPIDI_NM_upids_to_lupids(int size,
                                           size_t * remote_upid_size,
                                           char *remote_upids, int **remote_lupids)
{
    MPIR_Assert(0);
}

static inline int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                       int size, const int lpids[])
{
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_free_mem(void *ptr)
{
    return MPIDI_CH4U_mpi_free_mem(ptr);
}

static inline void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDI_CH4U_mpi_alloc_mem(size, info_ptr);
}

#endif /* UCX_INIT_H_INCLUDED */
