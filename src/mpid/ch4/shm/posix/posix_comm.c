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
#include "posix_noinline.h"
#include "release_gather.h"

int MPIDI_POSIX_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_COMMIT_PRE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_COMMIT_PRE_HOOK);

    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_init_null(comm);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_COMMIT_PRE_HOOK);
    return mpi_errno;
}

int MPIDI_POSIX_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_COMMIT_POST_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_COMMIT_POST_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_COMMIT_POST_HOOK);
    return mpi_errno;
}

int MPIDI_POSIX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);

    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_free(comm);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);
    return mpi_errno;
}
