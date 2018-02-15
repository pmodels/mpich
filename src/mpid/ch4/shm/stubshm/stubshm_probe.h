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
#ifndef STUBSHM_PROBE_H_INCLUDED
#define STUBSHM_PROBE_H_INCLUDED

#include "stubshm_impl.h"


static inline int MPIDI_STUBSHM_mpi_improbe(int source,
                                            int tag,
                                            MPIR_Comm * comm,
                                            int context_offset,
                                            int *flag, MPIR_Request ** message, MPI_Status * status)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IMPROBE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IMPROBE);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iprobe(int source,
                                           int tag,
                                           MPIR_Comm * comm,
                                           int context_offset, int *flag, MPI_Status * status)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IPROBE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IPROBE);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_PROBE_H_INCLUDED */
