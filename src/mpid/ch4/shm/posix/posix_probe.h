/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_PROBE_H_INCLUDED
#define POSIX_PROBE_H_INCLUDED

#include "posix_impl.h"


#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_improbe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_improbe(int source,
                                                     int tag,
                                                     MPIR_Comm * comm,
                                                     int context_offset,
                                                     int *flag, MPIR_Request ** message,
                                                     MPI_Status * status)
{
    return MPIDI_CH4U_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_iprobe(int source,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int context_offset, int *flag,
                                                    MPI_Status * status)
{
    return MPIDI_CH4U_mpi_iprobe(source, tag, comm, context_offset, flag, status);
}

#endif /* POSIX_PROBE_H_INCLUDED */
