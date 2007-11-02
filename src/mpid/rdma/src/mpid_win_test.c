/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_test(MPID_Win *win_ptr, int *flag)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_TEST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_TEST);

    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
        return mpi_errno;
    }

    *flag = (win_ptr->my_counter) ? 0 : 1;

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
    return mpi_errno;
}
