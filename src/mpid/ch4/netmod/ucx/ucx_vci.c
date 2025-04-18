/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

static int all_vcis_address_exchange(MPIR_Comm * comm);
static void flush_all(void);

/* Compared to MPIDI_OFI_comm_set_vcis, this is simpler because -
 *   * assume the requested num_vcis will always be created.
 *   * assume every rank will have the same number of vcis.
 *
 * TODO: remove those assumptions
 */
int MPIDI_UCX_comm_set_vcis(MPIR_Comm * comm, int num_implicit, int num_reserved,
                            MPIDI_num_vci_t * all_num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIDI_UCX_global.num_vcis == 1);
    MPIDI_UCX_global.num_vcis = num_implicit + num_reserved;

    all_num_vcis[comm->rank].n_vcis = num_implicit;
    all_num_vcis[comm->rank].n_total_vcis = num_implicit + num_reserved;;
    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                    all_num_vcis, sizeof(MPIDI_num_vci_t), MPIR_BYTE_INTERNAL,
                                    comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    for (int i = 1; i < MPIDI_UCX_global.num_vcis; i++) {
        mpi_errno = MPIDI_UCX_init_worker(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = all_vcis_address_exchange(comm);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_DEBUG_SUMMARY && comm->rank == 0) {
        printf("num_vcis: %d\n", MPIDI_UCX_global.num_vcis);
    }
    /* Flush all pending wireup operations or it may interfere with RMA flush_ops count.
     * Since this require progress in non-zero vcis, we need switch on is_initialized. */
    flush_all();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int all_vcis_address_exchange(MPIR_Comm * comm)
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
    mpi_errno = MPIR_Allgather_allcomm_auto(MPI_IN_PLACE, 0, MPIR_BYTE_INTERNAL,
                                            all_names, my_len, MPIR_BYTE_INTERNAL,
                                            comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* insert the addresses */
    ucp_ep_params_t ep_params;
    for (int vci_local = 0; vci_local < num_vcis; vci_local++) {
        for (int r = 0; r < size; r++) {
            MPIDI_UCX_addr_t *av = &MPIDI_UCX_AV(MPIDIU_lpid_to_av(r));
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
