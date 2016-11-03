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
#ifndef SHM_SHMAM_EAGER_STUB_INIT_H_INCLUDED
#define SHM_SHMAM_EAGER_STUB_INIT_H_INCLUDED

#include "stub_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_init(int rank, int grank, int size)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_finalize()
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHMAM_eager_threshold()
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_connect(int grank)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_listen(int *grank)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHMAM_eager_accept(int grank)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SHM_SHMAM_EAGER_STUB_INIT_H_INCLUDED */
