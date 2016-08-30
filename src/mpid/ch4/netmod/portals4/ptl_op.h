/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef NETMOD_PTL_OP_H_INCLUDED
#define NETMOD_PTL_OP_H_INCLUDED

#include "ptl_impl.h"

static inline int MPIDI_NM_mpi_op_free_hook(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return 0;
}

static inline int MPIDI_NM_mpi_op_create_hook(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return 0;
}

#endif /* NETMOD_PTL_OP_H_INCLUDED */
