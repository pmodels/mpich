/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_noinline.h"
#include "release_gather.h"
#include "nb_bcast_release_gather.h"
#include "nb_reduce_release_gather.h"

int MPIDI_POSIX_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_init_null(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIDI_POSIX_COMM(comm, nb_bcast_seq_no) = 0;
    MPIDI_POSIX_COMM(comm, nb_reduce_seq_no) = 0;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* prune selection tree */
    if (MPIDI_global.shm.posix.csel_root) {
        mpi_errno = MPIR_Csel_prune(MPIDI_global.shm.posix.csel_root, comm,
                                    &MPIDI_POSIX_COMM(comm, csel_comm));
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIDI_POSIX_COMM(comm, csel_comm) = NULL;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_POSIX_COMM(comm, csel_comm)) {
        mpi_errno = MPIR_Csel_free(MPIDI_POSIX_COMM(comm, csel_comm));
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Release_gather primitives based collective algorithm works for Intra-comms only */
    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIDI_POSIX_mpi_release_gather_comm_free(comm);
        mpi_errno = MPIDI_POSIX_nb_release_gather_comm_free(comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_comm_set_hints(MPIR_Comm * comm, MPIR_Info * info)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}
