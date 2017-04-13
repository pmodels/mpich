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
#ifndef STUBNM_PROGRESS_H_INCLUDED
#define STUBNM_PROGRESS_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_progress(int vni, int blocking)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_test(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_poke(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline void MPIDI_NM_progress_start(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

static inline void MPIDI_NM_progress_end(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

static inline int MPIDI_NM_progress_wait(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_register(int (*progress_fn) (int *), int *id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_deregister(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_activate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_deactivate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* STUBNM_PROGRESS_H_INCLUDED */
