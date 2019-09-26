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

static char t[100];
char * print_name(unsigned char * s){
    sprintf(t, "%02x%02x", s[2],s[3]);
    return t;
}

int MPIDI_OFI_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
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
        int *rank_map;
        int recv_bc_len;
        printf("[%d] addr = %s\n", comm->rank, print_name(MPIDI_OFI_global.addrname));
        MPIDU_bc_allgather(MPIDI_OFI_global.addrname,
                           MPIDI_OFI_global.addrnamelen, TRUE, &table, &rank_map, &recv_bc_len);

        int i;
        for (i = 0; i < MPIR_Process.size; i++) {
            if (rank_map[i] >= 0) {
                mpi_errno = MPIDI_OFI_av_insert(i, (char *) table + recv_bc_len * rank_map[i]);
                MPIR_ERR_CHECK(mpi_errno);
                if (comm->rank==0){
                    printf("--[%d] inserting addr %d = %s\n", comm->rank, i, print_name((char*)table+recv_bc_len*rank_map[i]));
                }
            }
        }
        MPIDU_bc_table_destroy();
    }

  fn_exit:
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
