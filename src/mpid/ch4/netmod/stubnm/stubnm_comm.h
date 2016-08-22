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
#ifndef NETMOD_STUBNM_COMM_H_INCLUDED
#define NETMOD_STUBNM_COMM_H_INCLUDED

#include "stubnm_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_comm_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_comm_create(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_comm_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_comm_destroy(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}


#endif /* NETMOD_STUBNM_COMM_H_INCLUDED */
