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

static void init_num_vcis(void)
{
    /* TODO: check capabilities, abort if we can't support the requested number of vcis. */
    MPIDI_UCX_global.num_vcis = MPIDI_global.n_total_vcis;
}

static int init_worker(int vci)
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
        }
        MPIDU_bc_allgather(init_comm, MPIDI_UCX_global.ctx[0].if_address,
                           (int) MPIDI_UCX_global.ctx[0].addrname_len, FALSE,
                           (void **) &table, &rank_map, &recv_bc_len);

        /* insert new addresses, skipping over node roots */
        for (int i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {

                ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
                ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
                                           &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[0][0]);
                MPIDI_UCX_CHK_STATUS(ucx_status);
            }
        }
        MPIDU_bc_table_destroy();
    } else {
        for (int i = 0; i < size; i++) {
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[0][0]);
            MPIDI_UCX_CHK_STATUS(ucx_status);
        }
        MPIDU_bc_table_destroy();
    }

  fn_exit:
    if (init_comm) {
        MPIDI_destroy_init_comm(&init_comm);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int all_vcis_address_exchange(void)
{
    int mpi_errno = MPI_SUCCESS;

    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    int num_vcis = MPIDI_UCX_global.num_vcis;

    /* ucx address lengths are non-uniform, use MPID_MAX_BC_SIZE */
    size_t name_len = MPID_MAX_BC_SIZE;

    int my_len = num_vcis * name_len;
    char *all_names = MPL_malloc(size * my_len, MPL_MEM_ADDRESS);
    MPIR_Assert(all_names);

    char *my_names = all_names + rank * my_len;

    /* put in my addrnames */
    for (int i = 0; i < num_vcis; i++) {
        char *vci_addrname = my_names + i * name_len;
        memcpy(vci_addrname, MPIDI_UCX_global.ctx[i].if_address,
               MPIDI_UCX_global.ctx[i].addrname_len);
    }
    /* Allgather */
    MPIR_Comm *comm = MPIR_Process.comm_world;
    mpi_errno = MPIR_Allgather_allcomm_auto(MPI_IN_PLACE, 0, MPI_BYTE,
                                            all_names, my_len, MPI_BYTE, comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* insert the addresses */
    ucp_ep_params_t ep_params;
    for (int vci_local = 0; vci_local < num_vcis; vci_local++) {
        for (int r = 0; r < size; r++) {
            MPIDI_UCX_addr_t *av = &MPIDI_UCX_AV(&MPIDIU_get_av(0, r));
            for (int vci_remote = 0; vci_remote < num_vcis; vci_remote++) {
                if (vci_local == 0 && vci_remote == 0) {
                    /* don't overwrite existing addr, or bad things will happen */
                    continue;
                }
                int idx = r * num_vcis + vci_remote;
                ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                ep_params.address = (ucp_address_t *) (all_names + idx * name_len);

                ucs_status_t ucx_status;
                ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[vci_local].worker,
                                           &ep_params, &av->dest[vci_local][vci_remote]);
                MPIDI_UCX_CHK_STATUS(ucx_status);
            }
        }
    }
  fn_exit:
    MPL_free(all_names);
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

    init_num_vcis();

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

    if (MPIDI_UCX_global.num_vcis > 1) {
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
        printf("num_vcis: %d\n", MPIDI_UCX_global.num_vcis);
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
    mpi_errno = init_worker(0);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = initial_address_exchange();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.ctx[0].worker != NULL) {
        ucp_worker_destroy(MPIDI_UCX_global.ctx[0].worker);
    }
    goto fn_exit;
}

/* static functions for MPIDI_UCX_post_init */
static void flush_cb(void *request, ucs_status_t status)
{
}

static void flush_all(void)
{
    void *reqs[MPIDI_CH4_MAX_VCIS];
    for (int vci = 0; vci < MPIDI_UCX_global.num_vcis; vci++) {
        reqs[vci] = ucp_worker_flush_nb(MPIDI_UCX_global.ctx[vci].worker, 0, &flush_cb);
    }
    for (int vci = 0; vci < MPIDI_UCX_global.num_vcis; vci++) {
        if (reqs[vci] == NULL) {
            continue;
        } else if (UCS_PTR_IS_ERR(reqs[vci])) {
            continue;
        } else {
            ucs_status_t status;
            do {
                MPID_Progress_test(NULL);
                status = ucp_request_check_status(reqs[vci]);
            } while (status == UCS_INPROGRESS);
            ucp_request_release(reqs[vci]);
        }
    }
}

int MPIDI_UCX_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_UCX_global.num_vcis == 1) {
        goto fn_exit;
    }

    for (int i = 1; i < MPIDI_UCX_global.num_vcis; i++) {
        mpi_errno = init_worker(i);
        MPIR_ERR_CHECK(mpi_errno);
    }
    mpi_errno = all_vcis_address_exchange();
    MPIR_ERR_CHECK(mpi_errno);

    /* Flush all pending wireup operations or it may interfere with RMA flush_ops count.
     * Since this require progress in non-zero vcis, we need switch on is_initialized. */
    MPIDI_global.is_initialized = 1;
    flush_all();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIDI_global.is_initialized) {
        /* Nothing to do */
        return mpi_errno;
    }

    ucs_status_ptr_t ucp_request;
    ucs_status_ptr_t *pending;

    int n = MPIDI_UCX_global.num_vcis;
    pending = MPL_malloc(sizeof(ucs_status_ptr_t) * MPIR_Process.size * n * n, MPL_MEM_OTHER);

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
