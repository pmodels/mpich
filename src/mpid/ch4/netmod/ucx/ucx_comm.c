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
#ifdef HAVE_LIBHCOLL
#include "../../common/hcoll/hcoll.h"
#endif

int MPIDI_UCX_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MPI_COMM_CREATE_HOOK);

    /* if this is MPI_COMM_WORLD, finish bc exchange */
    if (comm == MPIR_Process.comm_world && MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        void *table;
        int *rank_map;
        int recv_bc_len;
        MPIDU_bc_allgather(MPIDI_UCX_global.if_address, (int) MPIDI_UCX_global.addrname_len, FALSE,
                           (void **) &table, &rank_map, &recv_bc_len);

        /* insert new addresses, skipping over node roots */
        int i;
        for (i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {
                ucs_status_t ucx_status;
                ucp_ep_params_t ep_params;

                ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                ep_params.address = (ucp_address_t *) ((char *) table + i * recv_bc_len);
                ucx_status = ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                                           &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
                MPIDI_UCX_CHK_STATUS(ucx_status);
            }
        }
        MPIDU_bc_table_destroy();
    }
#if defined HAVE_LIBHCOLL
    hcoll_comm_create(comm, NULL);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_UCX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_MPI_COMM_FREE_HOOK);

#ifdef HAVE_LIBHCOLL
    hcoll_comm_destroy(comm, NULL);
#endif
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}
