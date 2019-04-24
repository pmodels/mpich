/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "mpidu_bc.h"
#include <ucp/api/ucp.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : CH4_UCX
      description : A category for CH4 UCX netmod variables

cvars:
    - name        : MPIR_CVAR_CH4_UCX_MAX_VNIS
      category    : CH4_UCX
      type        : int
      default     : 1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of VNIs
        that UCX netmod exposes.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static void request_init_callback(void *request);

static void request_init_callback(void *request)
{

    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    ucp_request->req = NULL;

}

int MPIDI_UCX_mpi_init_hook(int rank, int size, int appnum, int *tag_bits, MPIR_Comm * comm_world,
                            MPIR_Comm * comm_self, int spawned, int *n_vnis_provided)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_config_t *config;
    ucs_status_t ucx_status;
    uint64_t features = 0;
    int i, vni;
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    ucp_ep_params_t ep_params;
    size_t *bc_indices;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_INIT_HOOK);

    *n_vnis_provided = MPIR_CVAR_CH4_UCX_MAX_VNIS;
    MPIDI_UCX_VNI_POOL(total_vnis) = *n_vnis_provided;

    ucx_status = ucp_config_read(NULL, NULL, &config);
    MPIDI_UCX_CHK_STATUS(ucx_status);

    /* For now use only the tag feature */
    features = UCP_FEATURE_TAG | UCP_FEATURE_RMA;
    ucp_params.features = features;
    ucp_params.request_size = sizeof(MPIDI_UCX_ucp_request_t);
    ucp_params.request_init = request_init_callback;
    ucp_params.request_cleanup = NULL;
    ucp_params.estimated_num_eps = size * MPIDI_UCX_VNI_POOL(total_vnis);
    /* multiple threads can access the context from different workers in the same context */
    ucp_params.mt_workers_shared = 1;

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_ESTIMATED_NUM_EPS | UCP_PARAM_FIELD_REQUEST_INIT |
        UCP_PARAM_FIELD_MT_WORKERS_SHARED;

    ucx_status = ucp_init(&ucp_params, config, &MPIDI_UCX_global.context);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucp_config_release(config);

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    for (vni = 0; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
        ucx_status = ucp_worker_create(MPIDI_UCX_global.context, &worker_params,
                                       &MPIDI_UCX_VNI(vni).worker);
        MPIDI_UCX_CHK_STATUS(ucx_status);
        ucx_status =
            ucp_worker_get_address(MPIDI_UCX_VNI(vni).worker, &MPIDI_UCX_VNI(vni).if_address,
                                   &MPIDI_UCX_VNI(vni).addrname_len);
        MPIDI_UCX_CHK_STATUS(ucx_status);
        MPIR_Assert(MPIDI_UCX_VNI(vni).addrname_len <= INT_MAX);
    }
    
    mpi_errno =
        MPIDU_bc_table_create(rank, size, MPIDI_global.node_map[0], MPIDI_UCX_VNI(0).if_address,
                              (int) MPIDI_UCX_VNI(0).addrname_len, FALSE,
                              MPIR_CVAR_CH4_ROOTS_ONLY_PMI,
                              (void **) &MPIDI_UCX_global.pmi_addr_table, &bc_indices);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        /* TODO: implement this for multiple VNIs */
        int *node_roots, num_nodes;

        MPIR_NODEMAP_get_node_roots(MPIDI_global.node_map[0], size, &node_roots, &num_nodes);
        for (i = 0; i < num_nodes; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) & MPIDI_UCX_global.pmi_addr_table[bc_indices[i]];
            ucx_status =
                ucp_ep_create(MPIDI_UCX_VNI(0 /*may be WRONG*/).worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, node_roots[i])).dest[0 /*may be WRONG*/]);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPL_free(node_roots);
    } else {
        /* Exchange address only for the first VNI here
         * Use the first VNI to exchange addresses of other VNIs during the commit of COMM_WORLD */
        for (i = 0; i < size; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) & MPIDI_UCX_global.pmi_addr_table[bc_indices[i]];
            ucx_status =
                ucp_ep_create(MPIDI_UCX_VNI(0).worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[0]);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPIDU_bc_table_destroy(MPIDI_UCX_global.pmi_addr_table);
    }

    MPIDIG_init(comm_world, comm_self);

    *tag_bits = MPIR_TAG_BITS_DEFAULT;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_EXIT);
    return mpi_errno;
  fn_fail:
    for (vni = 0; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
        if (MPIDI_UCX_VNI(vni).worker != NULL)
            ucp_worker_destroy(MPIDI_UCX_VNI(vni).worker);
    }

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    goto fn_exit;

}

int MPIDI_UCX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    int i, vni, p = 0, completed;
    MPIR_Comm *comm;
    ucs_status_ptr_t ucp_request;
    ucs_status_ptr_t *pending;

    comm = MPIR_Process.comm_world;
    pending =
        MPL_malloc(sizeof(ucs_status_ptr_t) * comm->local_size * MPIDI_UCX_VNI_POOL(total_vnis),
                   MPL_MEM_OTHER);

    for (i = 0; i < comm->local_size; i++) {
        for (vni = 0; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
            ucp_request = ucp_disconnect_nb(MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[vni]);
            MPIDI_UCX_CHK_REQUEST(ucp_request);
            if (ucp_request != UCS_OK) {
                pending[p] = ucp_request;
                p++;
            }
        }
    }

    /* now complete the outstaning requests! Important: call progress inbetween, otherwise we
     * deadlock! */
    completed = p;
    while (completed != 0) {
        for (vni = 0; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
            ucp_worker_progress(MPIDI_UCX_VNI(vni).worker);
        }
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


    for (vni = 0; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
        if (MPIDI_UCX_VNI(vni).worker != NULL)
            ucp_worker_destroy(MPIDI_UCX_VNI(vni).worker);
    }

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    MPIR_Comm_release_always(comm);

    comm = MPIR_Process.comm_self;
    MPIR_Comm_release_always(comm);

    MPIDIG_finalize();

  fn_exit:
    MPL_free(pending);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPIDI_vci_resource_t MPIDI_UCX_vni_get_resource_info(int vni)
{
    MPIR_Assert(0 <= vni && vni < 1);
    return MPIDI_VCI_RESOURCE__TX | MPIDI_VCI_RESOURCE__RX;
}

int MPIDI_UCX_comm_get_lpid(MPIR_Comm * comm_ptr, int idx, int *lpid_ptr, bool is_remote)
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
