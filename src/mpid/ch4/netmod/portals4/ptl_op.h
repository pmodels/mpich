/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef PTL_OP_H_INCLUDED
#define PTL_OP_H_INCLUDED

#include "ptl_impl.h"

static inline int MPIDI_NM_mpi_op_free_hook(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return 0;
}

static inline int MPIDI_NM_mpi_op_commit_hook(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return 0;
}

#endif /* PTL_OP_H_INCLUDED */
