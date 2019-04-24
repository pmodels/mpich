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

    mpi_errno = MPIDIG_init_comm(comm);

    /* if this is MPI_COMM_WORLD, finish bc exchange */
    /* TODO: extend the following to support multiple VNIs. Currently non-functional
     * and program will fail if run with ROOTS_ONLY. Putting in placeholder value of
     * 0 so that build doesn't fail. */
    if (comm == MPIR_Process.comm_world) {
        ucs_status_t ucx_status;
        ucp_ep_params_t ep_params;
        int i;
        if (MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
            int curr, node;
            size_t *bc_indices;

            MPIR_Assert(MPII_Comm_is_node_consecutive(comm));
            MPIDU_bc_allgather(comm, MPIDI_global.node_map[0], MPIDI_UCX_VNI(0 /*may be WRONG*/).if_address,
                               (int) MPIDI_UCX_VNI(0 /*may be WRONG*/).addrname_len, FALSE,
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
                    ucp_ep_create(MPIDI_UCX_VNI(0 /*WRONG*/).worker, &ep_params,
                                  &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[0 /*WRONG*/]);
                MPIDI_UCX_CHK_STATUS(ucx_status);
                curr++;
            }
            MPIDU_bc_table_destroy(MPIDI_UCX_global.pmi_addr_table);
        } else {
           /* Exchange the addresses of the rest of the VNIs using the ROOT VNI */
            int vni;
            int my_rank, world_size;
            MPIR_Errflag_t errflag = MPIR_ERR_NONE;
            void *vni_addr_table;

            world_size = MPIR_Comm_size(comm);
            my_rank = MPIR_Comm_rank(comm);

            for (vni = 1; vni < MPIDI_UCX_VNI_POOL(total_vnis); vni++) {
                /* Insert my share of the data */
                vni_addr_table = MPL_malloc(world_size * MPIDI_UCX_VNI(vni).addrname_len, MPL_MEM_ADDRESS);
                memset(vni_addr_table, 0, world_size * MPIDI_UCX_VNI(vni).addrname_len);
                memcpy(((char *) vni_addr_table) + my_rank * MPIDI_UCX_VNI(vni).addrname_len,
                        MPIDI_UCX_VNI(vni).if_address,
                        MPIDI_UCX_VNI(vni).addrname_len);

                /* Broadcast my address and receive everyone else's address for this VNI */
                MPIR_Allgather(MPI_IN_PLACE, MPIDI_UCX_VNI(vni).addrname_len, MPI_BYTE, vni_addr_table,
                        MPIDI_UCX_VNI(vni).addrname_len, MPI_BYTE, comm, &errflag);
                
                /* Create endpoints to connect to VNI[vni] of all ranks */
                for (i = 0; i < world_size; i++) {
                    ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
                    ep_params.address = (ucp_address_t *) ((char *) vni_addr_table + i * MPIDI_UCX_VNI(vni).addrname_len);
                    ucx_status =
                        ucp_ep_create(MPIDI_UCX_VNI(vni).worker, &ep_params,
                                      &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest[vni]);
                    MPIDI_UCX_CHK_STATUS(ucx_status);
                }
                free(vni_addr_table);
            }
        }
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

    mpi_errno = MPIDIG_destroy_comm(comm);
#ifdef HAVE_LIBHCOLL
    hcoll_comm_destroy(comm, NULL);
#endif
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}
