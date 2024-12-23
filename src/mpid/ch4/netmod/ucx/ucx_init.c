/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "mpidu_bc.h"
#include <ucp/api/ucp.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : CH4_UCX
      description : A category for CH4 OFI netmod variables

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static bool ucx_initialized = false;

static void request_init_callback(void *request)
{

    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    ucp_request->req = NULL;

}

int MPIDI_UCX_init_worker(int vci)
{
    int mpi_errno = MPI_SUCCESS;

    ucp_worker_params_t worker_params;
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    ucs_status_t ucx_status;
    ucx_status = ucp_worker_create(MPIDI_UCX_global.context, &worker_params,
                                   &MPIDI_UCX_global.ctx[vci].worker);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucx_status = ucp_worker_get_address(MPIDI_UCX_global.ctx[vci].worker,
                                        &MPIDI_UCX_global.ctx[vci].if_address,
                                        &MPIDI_UCX_global.ctx[vci].addrname_len);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    MPIR_Assert(MPIDI_UCX_global.ctx[vci].addrname_len <= INT_MAX);

    ucx_status = ucp_worker_set_am_handler(MPIDI_UCX_global.ctx[vci].worker,
                                           MPIDI_UCX_AM_HANDLER_ID,
                                           &MPIDI_UCX_am_handler, NULL, UCP_AM_FLAG_WHOLE_MSG);
    MPIDI_UCX_CHK_STATUS(ucx_status);
#ifdef HAVE_UCP_AM_NBX
    ucp_am_handler_param_t param = {
        .field_mask = UCP_AM_HANDLER_PARAM_FIELD_ID | UCP_AM_HANDLER_PARAM_FIELD_CB,
        .id = MPIDI_UCX_AM_NBX_HANDLER_ID,
        .cb = &MPIDI_UCX_am_nbx_handler,
    };
    ucx_status = ucp_worker_set_am_recv_handler(MPIDI_UCX_global.ctx[vci].worker, &param);
    MPIDI_UCX_CHK_STATUS(ucx_status);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int initial_address_exchange(void)
{
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucx_status;
    MPIR_Comm *init_comm = NULL;

    void *table;
    int recv_bc_len;
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    mpi_errno = MPIDU_bc_table_create(rank, size, MPIR_Process.node_map,
                                      MPIDI_UCX_global.ctx[0].if_address,
                                      (int) MPIDI_UCX_global.ctx[0].addrname_len, FALSE,
                                      MPIR_CVAR_CH4_ROOTS_ONLY_PMI, &table, &recv_bc_len);
    MPIR_ERR_CHECK(mpi_errno);

    ucp_ep_params_t ep_params;
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        int *node_roots = MPIR_Process.node_root_map;
        int num_nodes = MPIR_Process.num_nodes;
        int *rank_map;

        mpi_errno = MPIDI_create_init_comm(&init_comm);
        MPIR_ERR_CHECK(mpi_errno);

        for (int i = 0; i < num_nodes; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, node_roots[i])).dest[0][0]);
            MPIDI_UCX_CHK_STATUS(ucx_status);
            MPIDIU_upidhash_add(ep_params.address, recv_bc_len, node_roots[i]);
        }
        mpi_errno = MPIDU_bc_allgather(init_comm, MPIDI_UCX_global.ctx[0].if_address,
                                       (int) MPIDI_UCX_global.ctx[0].addrname_len, FALSE,
                                       (void **) &table, &rank_map, &recv_bc_len);
        MPIR_ERR_CHECK(mpi_errno);

        /* insert new addresses, skipping over node roots */
        for (int i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {
                ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                ep_params.address = (ucp_address_t *) ((char *) table + rank_map[i] * recv_bc_len);
                ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
                                           &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[0][0]);
                MPIDI_UCX_CHK_STATUS(ucx_status);
                MPIDIU_upidhash_add(ep_params.address, recv_bc_len, i);
            }
        }
        mpi_errno = MPIDU_bc_table_destroy();
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        for (int i = 0; i < size; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[0][0]);
            MPIDI_UCX_CHK_STATUS(ucx_status);
            MPIDIU_upidhash_add(ep_params.address, recv_bc_len, i);
        }
        mpi_errno = MPIDU_bc_table_destroy();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    if (init_comm && !mpi_errno) {
        MPIDI_destroy_init_comm(&init_comm);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_init_local(int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;

    ucp_config_t *config;
    ucs_status_t ucx_status;
    uint64_t features = 0;
    ucp_params_t ucp_params;

    MPIDI_UCX_global.num_vcis = 1;

    /* unable to support extended context id in current match bit configuration */
    MPL_COMPILE_TIME_ASSERT(MPIR_CONTEXT_ID_BITS <= MPIDI_UCX_CONTEXT_ID_BITS);

    ucx_status = ucp_config_read(NULL, NULL, &config);
    MPIDI_UCX_CHK_STATUS(ucx_status);

    /* For now use only the tag feature */
    features = UCP_FEATURE_TAG | UCP_FEATURE_RMA | UCP_FEATURE_AM;
    ucp_params.features = features;
    ucp_params.request_size = sizeof(MPIDI_UCX_ucp_request_t);
    ucp_params.request_init = request_init_callback;
    ucp_params.request_cleanup = NULL;
    ucp_params.estimated_num_eps = MPIR_Process.size;

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_ESTIMATED_NUM_EPS | UCP_PARAM_FIELD_REQUEST_INIT;

    if (MPICH_IS_THREADED) {
        ucp_params.mt_workers_shared = 1;
        ucp_params.field_mask |= UCP_PARAM_FIELD_MT_WORKERS_SHARED;
    }

    ucx_status = ucp_init(&ucp_params, config, &MPIDI_UCX_global.context);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucp_config_release(config);

    if (MPIDI_UCX_TAG_BITS > MPIR_TAG_BITS_DEFAULT) {
        *tag_bits = MPIR_TAG_BITS_DEFAULT;
    } else {
        *tag_bits = MPIDI_UCX_TAG_BITS;
    }

    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
        printf("==== UCX netmod Capability ====\n");
        printf("MPIDI_UCX_CONTEXT_ID_BITS: %d\n", MPIDI_UCX_CONTEXT_ID_BITS);
        printf("MPIDI_UCX_RANK_BITS: %d\n", MPIDI_UCX_RANK_BITS);
        printf("tag_bits: %d\n", *tag_bits);
        printf("===============================\n");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    goto fn_exit;
}

int MPIDI_UCX_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* initialize worker for vci 0 */
    mpi_errno = MPIDI_UCX_init_worker(0);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = initial_address_exchange();
    MPIR_ERR_CHECK(mpi_errno);

    ucx_initialized = true;

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.ctx[0].worker != NULL) {
        ucp_worker_destroy(MPIDI_UCX_global.ctx[0].worker);
    }
    goto fn_exit;
}

int MPIDI_UCX_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPIDI_UCX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    ucs_status_ptr_t ucp_request;
    ucs_status_ptr_t *pending = NULL;

    if (!ucx_initialized) {
        goto fn_exit;
    }

    int n = MPIDI_UCX_global.num_vcis;
    pending = MPL_malloc(sizeof(ucs_status_ptr_t) * MPIR_Process.size * n * n, MPL_MEM_OTHER);

    if (!MPIR_CVAR_NO_COLLECTIVE_FINALIZE) {
        /* if some process are not present, the disconnect may timeout and give errors */
        mpi_errno = MPIR_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }

    int p = 0;
    for (int i = 0; i < MPIR_Process.size; i++) {
        MPIDI_UCX_addr_t *av = &MPIDI_UCX_AV(&MPIDIU_get_av(0, i));
        for (int vci_local = 0; vci_local < MPIDI_UCX_global.num_vcis; vci_local++) {
            for (int vci_remote = 0; vci_remote < MPIDI_UCX_global.num_vcis; vci_remote++) {
                ucp_request = ucp_disconnect_nb(av->dest[vci_local][vci_remote]);
                MPIDI_UCX_CHK_REQUEST(ucp_request);
                if (ucp_request != UCS_OK) {
                    pending[p] = ucp_request;
                    p++;
                }
            }
        }
    }

    /* now complete the outstaning requests! Important: call progress in between, otherwise we
     * deadlock! */
    int completed;
    do {
        for (int i = 0; i < MPIDI_UCX_global.num_vcis; i++) {
            ucp_worker_progress(MPIDI_UCX_global.ctx[i].worker);
        }
        completed = p;
        for (int i = 0; i < p; i++) {
            if (ucp_request_is_completed(pending[i]) != 0)
                completed -= 1;
        }
    } while (completed != 0);

    for (int i = 0; i < p; i++) {
        ucp_request_release(pending[i]);
    }

    for (int i = 0; i < MPIDI_UCX_global.num_vcis; i++) {
        if (MPIDI_UCX_global.ctx[i].worker != NULL) {
            ucp_worker_release_address(MPIDI_UCX_global.ctx[i].worker,
                                       MPIDI_UCX_global.ctx[i].if_address);
            ucp_worker_destroy(MPIDI_UCX_global.ctx[i].worker);
        }
    }

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

#ifdef MPIDI_BUILD_CH4_UPID_HASH
    MPIDIU_upidhash_free();
#endif

  fn_exit:
    MPL_free(pending);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

int MPIDI_UCX_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}

void *MPIDI_UCX_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}
