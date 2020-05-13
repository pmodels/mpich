/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

int MPIDI_OFI_mpi_op_free_hook(MPIR_Op * op_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_OP_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_OP_FREE_HOOK);
    return 0;
}

int MPIDI_OFI_mpi_op_commit_hook(MPIR_Op * op_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_MPI_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_MPI_OP_COMMIT_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_MPI_OP_COMMIT_HOOK);
    return 0;
}
