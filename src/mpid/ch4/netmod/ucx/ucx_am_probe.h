/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef UCX_AM_PROBE_H_INCLUDED
#define UCX_AM_PROBE_H_INCLUDED

#include "ucx_impl.h"

static inline int MPIDI_NM_mpi_improbe(int source,
                                       int tag,
                                       MPIR_Comm * comm,
                                       int context_offset, MPIDI_av_entry_t *addr,
                                       int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMPROBE);

    mpi_errno = MPIDI_CH4U_mpi_improbe(source, tag, comm, context_offset, flag, message, status);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    return mpi_errno;
}

static inline int MPIDI_NM_mpi_iprobe(int source,
                                      int tag,
                                      MPIR_Comm * comm,
                                      int context_offset, MPIDI_av_entry_t *addr, int *flag, MPI_Status * status)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IPROBE);

    mpi_errno = MPIDI_CH4U_mpi_iprobe(source, tag, comm, context_offset, flag, status);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    return mpi_errno;
}

#endif /* UCX_AM_PROBE_H_INCLUDED */
