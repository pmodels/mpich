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
#ifndef STUBNM_PROBE_H_INCLUDED
#define STUBNM_PROBE_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_mpi_improbe(int source,
                                       int tag,
                                       MPIR_Comm * comm,
                                       int context_offset,
                                       MPIDI_av_entry_t *addr,
                                       int *flag, MPIR_Request ** message, MPI_Status * status)
{
    return MPIDI_CH4U_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
}

static inline int MPIDI_NM_mpi_iprobe(int source,
                                      int tag,
                                      MPIR_Comm * comm,
                                      int context_offset,
                                      MPIDI_av_entry_t *addr,
                                      int *flag, MPI_Status * status)
{
    return MPIDI_CH4U_mpi_iprobe(source, tag, comm, context_offset, flag, status);
}

#endif /* STUBNM_PROBE_H_INCLUDED */
