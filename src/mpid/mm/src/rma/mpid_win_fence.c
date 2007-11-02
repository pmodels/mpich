/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);

    return mpi_errno;
}
