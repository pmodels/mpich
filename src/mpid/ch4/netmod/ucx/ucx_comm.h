/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_COMM_H_INCLUDED
#define UCX_COMM_H_INCLUDED

#include "ucx_impl.h"
#ifdef HAVE_LIBHCOLL
#include "../../common/hcoll/hcoll.h"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);

    mpi_errno = MPIDI_CH4U_init_comm(comm);

    /* if this is MPI_COMM_WORLD, finish bc exchange */
    if (comm == MPIR_Process.comm_world && MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        ucs_status_t ucx_status;
        ucp_ep_params_t ep_params;
        int i, curr, node;
        size_t *bc_indices;

        MPIR_Assert(MPII_Comm_is_node_consecutive(comm));
        MPIDU_bc_allgather(comm, MPIDI_CH4_Global.node_map[0], MPIDI_UCX_global.if_address,
                           (int) MPIDI_UCX_global.addrname_len, FALSE,
                           (void **) &MPIDI_UCX_global.pmi_addr_table, &bc_indices);

        /* insert new addresses, skipping over node roots */
        for (i = 1, curr = 0, node = 0; i < MPIR_Comm_size(comm); i++) {
            if (comm->internode_table[i] == node + 1) {
                node++;
                continue;
            }
            ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
            ep_params.address =
                (ucp_address_t *) & MPIDI_UCX_global.pmi_addr_table[bc_indices[curr]];
            ucx_status =
                ucp_ep_create(MPIDI_UCX_global.worker, &ep_params,
                              &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
            MPIDI_UCX_CHK_STATUS(ucx_status);
            curr++;
        }
        MPIDU_bc_table_destroy(MPIDI_UCX_global.pmi_addr_table);
    }
#if defined HAVE_LIBHCOLL
    hcoll_comm_create(comm, NULL);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_comm_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);

    mpi_errno = MPIDI_CH4U_destroy_comm(comm);
#ifdef HAVE_LIBHCOLL
    hcoll_comm_destroy(comm, NULL);
#endif
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}

#endif /* UCX_COMM_H_INCLUDED */
