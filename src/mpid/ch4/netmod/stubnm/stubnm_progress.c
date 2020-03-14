/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPIDI_STUBNM_progress(int vci, int blocking)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_progress_test(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_progress_poke(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

void MPIDI_STUBNM_progress_start(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

void MPIDI_STUBNM_progress_end(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

int MPIDI_STUBNM_progress_wait(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_progress_register(int (*progress_fn) (int *), int *id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_progress_deregister(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_progress_activate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_STUBNM_progress_deactivate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
