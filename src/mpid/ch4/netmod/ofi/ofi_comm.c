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

int MPIDI_OFI_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);

    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_send_counters, MPL_MEM_COMM);
    MPIDIU_map_create(&MPIDI_OFI_COMM(comm).huge_recv_counters, MPL_MEM_COMM);

    /* no connection for non-dynamic or non-root-rank of intercomm */
    MPIDI_OFI_COMM(comm).conn_id = -1;

    /* eagain defaults to off */
    if (comm->hints[MPIR_COMM_HINT_EAGAIN] == 0) {
        comm->hints[MPIR_COMM_HINT_EAGAIN] = FALSE;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_PRE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_COMM_COMMIT_POST_HOOK);
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
