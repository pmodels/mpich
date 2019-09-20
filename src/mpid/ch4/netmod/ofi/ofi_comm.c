/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpidu_bc.h"
#include "ofi_noinline.h"

int MPIDI_OFI_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_CREATE_HOOK);

    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_send_counters, MPL_MEM_COMM);
    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_recv_counters, MPL_MEM_COMM);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* eagain defaults to off */
    MPIDI_OFI_COMM(comm).eagain = FALSE;

    /* if this is MPI_COMM_WORLD, finish bc exchange */
    if (comm == MPIR_Process.comm_world && MPIR_CVAR_CH4_ROOTS_ONLY_PMI) {
        void *table;
        fi_addr_t *mapped_table;
        size_t rem_bcs, num_nodes = MPIDI_global.max_node_id + 1;
        int i, curr, node;

        MPIR_Assert(MPII_Comm_is_node_consecutive(comm));
        rem_bcs = MPIR_Comm_size(comm) - num_nodes;
        MPIDU_bc_allgather(comm, MPIDI_global.node_map[0], &MPIDI_OFI_global.addrname,
                           MPIDI_OFI_global.addrnamelen, TRUE, &table, NULL);
        MPIR_CHKLMEM_MALLOC(mapped_table, fi_addr_t *, rem_bcs * sizeof(fi_addr_t),
                            mpi_errno, "mapped_table", MPL_MEM_ADDRESS);
        MPIDI_OFI_CALL(fi_av_insert(MPIDI_OFI_global.av, table, rem_bcs, mapped_table, 0ULL, NULL),
                       avmap);

        /* insert new addresses, skipping over node roots */
        for (i = 1, curr = 0, node = 0; i < MPIR_Comm_size(comm); i++) {
            if (comm->internode_table[i] == node + 1) {
                node++;
                continue;
            }
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).dest = mapped_table[curr];
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#else
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
            MPIDI_OFI_AV(&MPIDIU_get_av(0, i)).ep_idx = 0;
#endif
#endif
            curr++;
        }
        MPIDU_bc_table_destroy(table);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);

    MPIDIU_map_destroy(MPIDI_OFI_COMM(comm).huge_send_counters);
    MPIDIU_map_destroy(MPIDI_OFI_COMM(comm).huge_recv_counters);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}
