/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include <ucp/api/ucp.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : CH4_UCX
      description : A category for CH4 OFI netmod variables

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define UCX_AV_INSERT(av, lpid, name) \
    do { \
        if (MPIDI_UCX_AV(av).dest[0][0] == NULL) { \
            ucs_status_t status; \
            ucp_ep_params_t ep_params; \
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS; \
            ep_params.address = (ucp_address_t *) (name); \
            status = ucp_ep_create(MPIDI_UCX_global.ctx[0].worker, &ep_params, &MPIDI_UCX_AV(av).dest[0][0]); \
            MPIDI_UCX_CHK_STATUS(status); \
            MPIDIU_upidhash_add(ep_params.address, addrnamelen, lpid); \
        } \
    } while (0)

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

int MPIDI_UCX_comm_addr_exchange(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    /* only comm_world for now */
    MPIR_Assert(comm == MPIR_Process.comm_world);

    MPIR_Assert(comm->attr & MPIR_COMM_ATTR__HIERARCHY);

    char *addrname = (void *) MPIDI_UCX_global.ctx[0].if_address;
    int addrnamelen = MPID_MAX_BC_SIZE; /* 4096 */
    MPIR_Assert(MPIDI_UCX_global.ctx[0].addrname_len <= addrnamelen);

    int local_rank = comm->local_rank;
    int external_size = comm->num_external;

    if (external_size == 1) {
        /* skip root addrexch if we are the only node */
        goto all_addrexch;
    }

    /* PMI allgather over node roots and av_insert to activate node_roots_comm */
    if (local_rank == 0) {
        char *roots_names;
        MPIR_CHKLMEM_MALLOC(roots_names, external_size * addrnamelen);

        MPIR_PMI_DOMAIN domain = MPIR_PMI_DOMAIN_NODE_ROOTS;
        /* FIXME: use the actual addrname_len rather than MPID_MAX_BC_SIZE */
        mpi_errno = MPIR_pmi_allgather(addrname, addrnamelen, roots_names, addrnamelen, domain);
        MPIR_ERR_CHECK(mpi_errno);

        /* insert av and activate node_roots_comm */
        MPIR_Comm *node_roots_comm = MPIR_Comm_get_node_roots_comm(comm);
        for (int i = 0; i < external_size; i++) {
            char *p = (char *) roots_names + i * addrnamelen;
            MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(node_roots_comm, i);
            MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(node_roots_comm, i);
            UCX_AV_INSERT(av, lpid, p);
        }
    } else {
        /* just for the PMI_Barrier */
        MPIR_PMI_DOMAIN domain = MPIR_PMI_DOMAIN_NODE_ROOTS;
        mpi_errno = MPIR_pmi_allgather(addrname, addrnamelen, NULL, addrnamelen, domain);
        MPIR_ERR_CHECK(mpi_errno);
    }

  all_addrexch:
    if (external_size == comm->local_size) {
        /* if no local, we are done. */
        goto fn_exit;
    }

    /* -- rest of the addr exchange over node_code and node_roots_comm -- */
    /* since the orders will be rearranged by nodes, let's echange rank along with name */
    struct rankname {
        int rank;
        char name[];
    };
    int rankname_len = sizeof(int) + addrnamelen;

    struct rankname *my_rankname, *all_ranknames;
    MPIR_CHKLMEM_MALLOC(my_rankname, rankname_len);
    MPIR_CHKLMEM_MALLOC(all_ranknames, comm->local_size * rankname_len);

    my_rankname->rank = comm->rank;
    memcpy(my_rankname->name, addrname, addrnamelen);

    /* Use an smp algorithm explicitly that only require a working node_comm and node_roots_comm. */
    mpi_errno = MPIR_Allgather_intra_smp_no_order(my_rankname, rankname_len, MPIR_BYTE_INTERNAL,
                                                  all_ranknames, rankname_len, MPIR_BYTE_INTERNAL,
                                                  comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* create av, skipping existing entries */
    for (int i = 0; i < comm->local_size; i++) {
        struct rankname *p = (struct rankname *) ((char *) all_ranknames + i * rankname_len);
        MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, p->rank);
        MPIR_Lpid lpid = MPIR_comm_rank_to_lpid(comm, p->rank);
        UCX_AV_INSERT(av, lpid, p->name);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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

    /* initialize worker for vci 0 */
    mpi_errno = MPIDI_UCX_init_worker(0);
    MPIR_ERR_CHECK(mpi_errno);

    /* insert self address */
    MPIR_Lpid lpid = MPIR_Process.rank;
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av(lpid);
    char *addrname = (void *) MPIDI_UCX_global.ctx[0].if_address;
    int addrnamelen = MPIDI_UCX_global.ctx[0].addrname_len;
    UCX_AV_INSERT(av, lpid, addrname);

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
    if (MPIDI_UCX_global.ctx[0].worker != NULL) {
        ucp_worker_destroy(MPIDI_UCX_global.ctx[0].worker);
    }
    if (MPIDI_UCX_global.context != NULL) {
        ucp_cleanup(MPIDI_UCX_global.context);
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

    int n = MPIDI_UCX_global.num_vcis;
    pending = MPL_malloc(sizeof(ucs_status_ptr_t) * MPIR_Process.size * n * n, MPL_MEM_OTHER);

    if (!MPIR_CVAR_NO_COLLECTIVE_FINALIZE) {
        /* if some process are not present, the disconnect may timeout and give errors */
        mpi_errno = MPIR_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }

    int p = 0;
    for (int i = 0; i < MPIR_Process.size; i++) {
        MPIDI_UCX_addr_t *av = &MPIDI_UCX_AV(MPIDIU_lpid_to_av(i));
        for (int vci_local = 0; vci_local < MPIDI_UCX_global.num_vcis; vci_local++) {
            for (int vci_remote = 0; vci_remote < MPIDI_UCX_global.num_vcis; vci_remote++) {
                if (av->dest[vci_local][vci_remote]) {
                    ucp_request = ucp_disconnect_nb(av->dest[vci_local][vci_remote]);
                    MPIDI_UCX_CHK_REQUEST(ucp_request);
                    if (ucp_request != UCS_OK) {
                        pending[p] = ucp_request;
                        p++;
                    }
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
