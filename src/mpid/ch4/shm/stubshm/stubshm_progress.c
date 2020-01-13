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
#include "stubshm_impl.h"

int MPIDI_STUBSHM_do_progress_recv(int blocking, int *completion_count)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_do_progress_send(int blocking, int *completion_count)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress(int vci, int blocking)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_test(void)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_poke(void)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

void MPIDI_STUBSHM_progress_start(MPID_Progress_state * state)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

void MPIDI_STUBSHM_progress_end(MPID_Progress_state * state)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_wait(MPID_Progress_state * state)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_register(int (*progress_fn) (int *))
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_deregister(int id)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_activate(int id)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

int MPIDI_STUBSHM_progress_deactivate(int id)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}
