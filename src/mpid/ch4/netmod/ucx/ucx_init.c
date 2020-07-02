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
      description : A category for CH4 UCX netmod variables

cvars:
    - name        : MPIR_CVAR_CH4_UCX_MAX_VNIS
      category    : CH4_UCX
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive, this CVAR specifies the maximum number of CH4 VNIs
        that UCX netmod exposes. If set to 0 (the default) or bigger than
        MPIR_CVAR_CH4_NUM_VCIS, the number of exposed VNIs is set to MPIR_CVAR_CH4_NUM_VCIS.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static void request_init_callback(void *request);

static void request_init_callback(void *request)
{

    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    ucp_request->req = NULL;

}

static void init_num_vnis(void)
{
    int num_vnis = 1;
    if (MPIR_CVAR_CH4_UCX_MAX_VNIS == 0 || MPIR_CVAR_CH4_UCX_MAX_VNIS > MPIDI_global.n_vcis) {
        num_vnis = MPIDI_global.n_vcis;
    } else {
        num_vnis = MPIR_CVAR_CH4_UCX_MAX_VNIS;
    }

    /* for best performance, we ensure 1-to-1 vci/vni mapping. ref: MPIDI_OFI_vci_to_vni */
    /* TODO: allow less num_vnis. Option 1. runtime MOD; 2. overide MPIDI_global.n_vcis */
    MPIR_Assert(num_vnis == MPIDI_global.n_vcis);

    MPIDI_UCX_global.num_vnis = num_vnis;
}

static int init_worker(int vni)
{
    int mpi_errno = MPI_SUCCESS;

    ucp_worker_params_t worker_params;
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    ucs_status_t ucx_status;
    ucx_status = ucp_worker_create(MPIDI_UCX_global.context, &worker_params,
                                   &MPIDI_UCX_global.ctx[vni].worker);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucx_status = ucp_worker_get_address(MPIDI_UCX_global.ctx[vni].worker,
                                        &MPIDI_UCX_global.ctx[vni].if_address,
                                        &MPIDI_UCX_global.ctx[vni].addrname_len);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    MPIR_Assert(MPIDI_UCX_global.ctx[vni].addrname_len <= INT_MAX);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int initial_address_exchange(MPIR_Comm * init_comm)
{
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t ucx_status;

    void *table;
    int recv_bc_len;
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    mpi_errno = MPIDU_bc_table_create(rank, size, MPIDI_global.node_map[0],
                                      MPIDI_UCX_global.ctx[0].if_address,
                                      (int) MPIDI_UCX_global.ctx[0].addrname_len, FALSE,
                                      MPIR_CVAR_CH4_ROOTS_ONLY_PMI, &table, &recv_bc_len);
    MPIR_ERR_CHECK(mpi_errno);

    ucp_ep_params_t ep_params;
    if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        int *node_roots = MPIR_Process.node_root_map;
        int num_nodes = MPIR_Process.num_nodes;
        int *rank_map;

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
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int all_vnis_address_exchange(void)
{
    int mpi_errno = MPI_SUCCESS;

    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;
    int num_vnis = MPIDI_UCX_global.num_vnis;

    /* ucx address lengths are non-uniform, use MPID_MAX_BC_SIZE */
    size_t name_len = MPID_MAX_BC_SIZE;

    int my_len = num_vnis * name_len;
    char *all_names = MPL_malloc(size * my_len, MPL_MEM_ADDRESS);
    MPIR_Assert(all_names);

    char *my_names = all_names + rank * my_len;

    /* put in my addrnames */
    for (int i = 0; i < num_vnis; i++) {
        char *vni_addrname = my_names + i * name_len;
        memcpy(vni_addrname, MPIDI_UCX_global.ctx[i].if_address,
               MPIDI_UCX_global.ctx[i].addrname_len);
    }
    /* Allgather */
    MPIR_Comm *comm = MPIR_Process.comm_world;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Allgather_allcomm_auto(MPI_IN_PLACE, 0, MPI_BYTE,
                                            all_names, my_len, MPI_BYTE, comm, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* insert the addresses */
    ucp_ep_params_t ep_params;
    for (int vni_local = 0; vni_local < num_vnis; vni_local++) {
        for (int r = 0; r < size; r++) {
            MPIDI_UCX_addr_t *av = &MPIDI_UCX_AV(&MPIDIU_get_av(0, r));
            for (int vni_remote = 0; vni_remote < num_vnis; vni_remote++) {
                if (vni_local == 0 && vni_remote == 0) {
                    /* don't overwrite existing addr, or bad things will happen */
                    continue;
                }
                int idx = r * num_vnis + vni_remote;
                ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                ep_params.address = (ucp_address_t *) (all_names + idx * name_len);

                ucs_status_t ucx_status;
                ucx_status = ucp_ep_create(MPIDI_UCX_global.ctx[vni_local].worker,
                                           &ep_params, &av->dest[vni_local][vni_remote]);
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

int MPIDI_UCX_mpi_init_hook(int rank, int size, int appnum, int *tag_bits, MPIR_Comm * init_comm)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_config_t *config;
    ucs_status_t ucx_status;
    uint64_t features = 0;
    ucp_params_t ucp_params;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MPI_INIT_HOOK);

    init_num_vnis();

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

    if (MPIDI_UCX_global.num_vnis > 1) {
        ucp_params.mt_workers_shared = 1;
        ucp_params.field_mask |= UCP_PARAM_FIELD_MT_WORKERS_SHARED;
    }

    ucx_status = ucp_init(&ucp_params, config, &MPIDI_UCX_global.context);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucp_config_release(config);

    /* initialize worker for vni 0 */
    mpi_errno = init_worker(0);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = initial_address_exchange(init_comm);
    MPIR_ERR_CHECK(mpi_errno);

    *tag_bits = MPIR_TAG_BITS_DEFAULT;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.ctx[0].worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.ctx[0].worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    goto fn_exit;
}

int MPIDI_UCX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm;
    ucs_status_ptr_t ucp_request;
    ucs_status_ptr_t *pending;

    comm = MPIR_Process.comm_world;
    int n = MPIDI_UCX_global.num_vnis;
    pending = MPL_malloc(sizeof(ucs_status_ptr_t) * comm->local_size * n * n, MPL_MEM_OTHER);

    int p = 0;
    for (int i = 0; i < comm->local_size; i++) {
        MPIDI_UCX_addr_t *av = &MPIDI_UCX_AV(&MPIDIU_get_av(0, i));
        for (int vni_local = 0; vni_local < MPIDI_UCX_global.num_vnis; vni_local++) {
            for (int vni_remote = 0; vni_remote < MPIDI_UCX_global.num_vnis; vni_remote++) {
                ucp_request = ucp_disconnect_nb(av->dest[vni_local][vni_remote]);
                MPIDI_UCX_CHK_REQUEST(ucp_request);
                if (ucp_request != UCS_OK) {
                    pending[p] = ucp_request;
                    p++;
                }
            }
        }
    }

    /* now complete the outstaning requests! Important: call progress inbetween, otherwise we
     * deadlock! */
    int completed = p;
    while (completed != 0) {
        for (int i = 0; i < MPIDI_UCX_global.num_vnis; i++) {
            ucp_worker_progress(MPIDI_UCX_global.ctx[i].worker);
        }
        completed = p;
        for (int i = 0; i < p; i++) {
            if (ucp_request_is_completed(pending[i]) != 0)
                completed -= 1;
        }
    }

    for (int i = 0; i < p; i++) {
        ucp_request_release(pending[i]);
    }

    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    for (int i = 0; i < MPIDI_UCX_global.num_vnis; i++) {
        if (MPIDI_UCX_global.ctx[i].worker != NULL) {
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

/* static functions for MPIDI_UCX_post_init */
static void flush_cb(void *request, ucs_status_t status)
{
}

static void flush_all(void)
{
    void *reqs[MPIDI_CH4_MAX_VCIS];
    for (int vni = 0; vni < MPIDI_UCX_global.num_vnis; vni++) {
        reqs[vni] = ucp_worker_flush_nb(MPIDI_UCX_global.ctx[vni].worker, 0, &flush_cb);
    }
    for (int vni = 0; vni < MPIDI_UCX_global.num_vnis; vni++) {
        if (reqs[vni] == NULL) {
            continue;
        } else if (UCS_PTR_IS_ERR(reqs[vni])) {
            continue;
        } else {
            ucs_status_t status;
            do {
                MPID_Progress_test(NULL);
                status = ucp_request_check_status(reqs[vni]);
            } while (status == UCS_INPROGRESS);
            ucp_request_release(reqs[vni]);
        }
    }
}

int MPIDI_UCX_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 1; i < MPIDI_UCX_global.num_vnis; i++) {
        mpi_errno = init_worker(i);
        MPIR_ERR_CHECK(mpi_errno);
    }
    mpi_errno = all_vnis_address_exchange();
    MPIR_ERR_CHECK(mpi_errno);

    /* flush all pending wireup operations or it may interfere with RMA flush_ops count */
    flush_all();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
