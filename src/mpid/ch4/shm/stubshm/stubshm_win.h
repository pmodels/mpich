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
#ifndef STUBSHM_WIN_H_INCLUDED
#define STUBSHM_WIN_H_INCLUDED

#include "stubshm_impl.h"


static inline int MPIDI_STUBSHM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_win_complete(MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_win_wait(MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_win_test(MPIR_Win * win, int *flag)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_win_unlock(int rank, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_fence(int assert, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_shared_query(MPIR_Win * win,
                                                     int rank,
                                                     MPI_Aint * size, int *disp_unit, void *baseptr)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_flush(int rank, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_flush_local_all(MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_unlock_all(MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_flush_local(int rank, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_sync(MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_flush_all(MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_win_lock_all(int assert, MPIR_Win * win)
{

    MPIR_Assert(0);

    return MPI_SUCCESS;
}


#endif /* STUBSHM_WIN_H_INCLUDED */
